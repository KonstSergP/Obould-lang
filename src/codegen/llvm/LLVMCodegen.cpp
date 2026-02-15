#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"

namespace obould
{
std::unique_ptr<llvm::Module> LLVMCodegenVisitor::codegen(Module& moduleAst)
{
    locals.clear();
    functions.clear();
    structTypes.clear();
    lvalue = false;
    lastValue = nullptr;
    currentFunction = nullptr;
    module = std::make_unique<llvm::Module>(moduleAst.name, context);
    builder = std::make_unique<llvm::IRBuilder<>>(context);

    moduleAst.accept(*this);
    return std::move(module);
}

llvm::FunctionType* LLVMCodegenVisitor::createFunctionType(const std::shared_ptr<TypeInfo>& type)
{
    auto* retType = toLLVMType(type->returnType);
    std::vector<llvm::Type*> paramTypes, hiddenParams;
    for (const auto& param : type->parameters) {
        auto paramType = param.type;
        if (param.isReference || paramType->kind == TypeKind::Array || paramType->kind == TypeKind::Struct) {
            paramTypes.push_back(builder->getPtrTy());
            while (paramType->kind == TypeKind::Array) {
                hiddenParams.push_back(builder->getInt64Ty());
                paramType = paramType->baseType;
            }
        }
        else {
            paramTypes.push_back(toLLVMType(paramType));
        }
    }
    paramTypes.insert(paramTypes.end(), hiddenParams.begin(), hiddenParams.end());
    return llvm::FunctionType::get(retType, paramTypes, false);
}

llvm::Type* LLVMCodegenVisitor::toLLVMType(const std::shared_ptr<TypeInfo>& typeInfo)
{
    if (!typeInfo) {
        return llvm::Type::getVoidTy(context);
    }

    switch (typeInfo->kind) {
    case TypeKind::i64:
        return builder->getInt64Ty();
    case TypeKind::Byte:
    case TypeKind::Char:
        return builder->getInt8Ty();
    case TypeKind::Bool:
        return builder->getInt1Ty();
    case TypeKind::f64:
        return builder->getDoubleTy();
    case TypeKind::Void:
        return builder->getVoidTy();
    case TypeKind::Pointer:
    case TypeKind::Procedure:
    case TypeKind::String:
        return builder->getPtrTy();
    case TypeKind::Array:
    {
        if (typeInfo->isOpenArray) {
            return builder->getPtrTy();
        }
        auto* elemTy = toLLVMType(typeInfo->baseType);
        const uint64_t len = static_cast<uint64_t>(std::max<int64_t>(typeInfo->length, 0));
        return llvm::ArrayType::get(elemTy, len);
    }
    case TypeKind::Struct:
    {
        auto it = structTypes.find(typeInfo.get());
        if (it != structTypes.end()) {
            return it->second;
        }

        std::vector<llvm::Type*> fieldTypes;
        std::vector<TypeInfo*> chain;
        auto current = typeInfo;
        while (current) {
            chain.push_back(current.get());
            current = current->baseType;
        }
        std::reverse(chain.begin(), chain.end());

        fieldTypes.push_back(builder->getPtrTy());
        for (const auto* info : chain) {
            for (const auto& field : info->fields) {
                fieldTypes.push_back(toLLVMType(field.type));
            }
        }

        std::string structName = "struct." + typeInfo->name;
        auto* structTy = llvm::StructType::create(context, structName);
        structTy->setBody(fieldTypes, false);
        structTypes[typeInfo.get()] = structTy;
        return structTy;
    }
    default:
        return llvm::Type::getVoidTy(context);
    }
}

llvm::Value* LLVMCodegenVisitor::getConstantValue(const Expression& node)
{
    if (!node.constantValue.has_value()) return nullptr;
    auto& val = node.constantValue.value();

    if (std::holds_alternative<int64_t>(val)) {
        return builder->getInt64(std::get<int64_t>(val));
    }
    if (std::holds_alternative<double>(val)) {
        auto* ty = llvm::Type::getDoubleTy(context);
        return llvm::ConstantFP::get(ty, std::get<double>(val));
    }
    if (std::holds_alternative<bool>(val)) {
        return builder->getInt1(std::get<bool>(val));
    }
    if (std::holds_alternative<std::string>(val)) {
        return builder->CreateGlobalStringPtr(std::get<std::string>(val), "str", 0, module.get());
    }
    return nullptr;
}

llvm::AllocaInst* LLVMCodegenVisitor::createEntryAlloca(llvm::Type* type, const std::string& name) const
{
    auto& entryBB = currentFunction->getEntryBlock();
    llvm::IRBuilder entryBuilder(&entryBB, entryBB.begin());
    return entryBuilder.CreateAlloca(type, nullptr, name);
}

llvm::GlobalVariable* LLVMCodegenVisitor::createStructDescriptor(const std::shared_ptr<TypeInfo>& type)
{
    std::string name = "struct_desc." + type->name;
    int64_t depth = type->depth;

    auto* ptrType = builder->getPtrTy();
    auto* intType = builder->getInt64Ty();
    auto* arrayType = llvm::ArrayType::get(ptrType, depth + 1);
    auto* structTy = llvm::StructType::create({intType, arrayType}, name);

    auto* gVar = new llvm::GlobalVariable(
        *module,
        structTy,
        true,
        llvm::GlobalValue::ExternalLinkage, // TODO: придумать как ставить правильную линковку
        nullptr,
        name
    );

    auto* depthVal = llvm::ConstantInt::get(intType, depth);
    std::vector<llvm::Constant*> basePtrs(depth + 1);

    if (type->baseType && descriptors.find(type->baseType.get()) == descriptors.end()) {
        descriptors[type->baseType.get()] = createStructDescriptor(type->baseType);
    }
    auto curType = type;
    while (curType != nullptr) {
        auto* descPtr = (curType != type) ? descriptors[curType.get()] : gVar;
        basePtrs[curType->depth] = llvm::ConstantExpr::getBitCast(descPtr, ptrType);
        curType = curType->baseType;
    }

    auto* arrayVal = llvm::ConstantArray::get(arrayType, basePtrs);
    auto* initValue = llvm::ConstantStruct::get(structTy, {depthVal, arrayVal});
    gVar->setInitializer(initValue);
    return gVar;
}

void LLVMCodegenVisitor::makeStructCastCheck(llvm::Value* left, llvm::Value* right, int64_t rDepth)
{
    auto* startBB = llvm::BasicBlock::Create(context, "is.start", currentFunction);
    auto* contBB = llvm::BasicBlock::Create(context, "is.cont", currentFunction);
    auto* endBB = llvm::BasicBlock::Create(context, "is.end", currentFunction);

    builder->CreateBr(startBB);
    auto* objDepth = builder->CreateLoad(builder->getInt64Ty(), left);
    auto* depthCheck = builder->CreateICmpSGE(objDepth, builder->getInt64(rDepth));
    builder->CreateCondBr(depthCheck, contBB, endBB);

    auto* structTy = llvm::StructType::create({builder->getInt64Ty(), builder->getPtrTy()});

    builder->SetInsertPoint(contBB);
    auto* arrayPtr = builder->CreateStructGEP(structTy, left, 1);
    auto* ancestorPtrAddr = builder->CreateGEP(builder->getPtrTy(), arrayPtr, builder->getInt64(rDepth));
    auto* ancestorPtr = builder->CreateLoad(builder->getPtrTy(), ancestorPtrAddr, "ancestor.tag");
    auto* instCheck = builder->CreateICmpEQ(ancestorPtr, right, "is.inst");
    builder->CreateBr(endBB);

    builder->SetInsertPoint(endBB);
    auto* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "is.res");
    phi->addIncoming(builder->getFalse(), startBB);
    phi->addIncoming(instCheck, contBB);
    lastValue = phi;
}
}
