#include <iostream>
#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
std::unique_ptr<llvm::Module> LLVMCodegenVisitor::codegen(Module& moduleAst, bool isMain)
{
    locals.clear();
    functions.clear();
    structTypes.clear();
    descriptors.clear();
    lengths.clear();
    lvalue = false;
    importedModule = false;
    isMainModule = isMain;
    lastValue = nullptr;
    currentFunction = nullptr;
    module = std::make_unique<llvm::Module>(moduleAst.name, context);
    builder = std::make_unique<llvm::IRBuilder<>>(context);
    currentModule = moduleAst.name;
    rootModule = &moduleAst;

    moduleAst.accept(*this);
    return std::move(module);
}

std::string LLVMCodegenVisitor::getMangledName(const std::string& moduleName, const std::string& name)
{
    return "ob_" + std::to_string(moduleName.length()) + moduleName + "_" + name;
}

llvm::FunctionType* LLVMCodegenVisitor::createFunctionType(const std::shared_ptr<TypeInfo>& type)
{
    auto* retType = toLLVMType(type->returnType);
    std::vector<llvm::Type*> paramTypes, hiddenParams;
    for (const auto& param : type->parameters) {
        auto paramType = param.type;
        if (param.isReference || paramType->kind == TypeKind::Array || paramType->kind == TypeKind::Struct) {
            paramTypes.push_back(builder->getPtrTy());
            while (paramType->isOpenArray) {
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
    case TypeKind::Set:
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
        auto it = structTypes.find(typeInfo->id);
        if (it != structTypes.end()) {
            return it->second;
        }

        auto* structTy = llvm::StructType::create(context, "struct_" + typeInfo->name);
        structTypes[typeInfo->id] = structTy;

        std::vector<TypeInfo*> chain;
        auto current = typeInfo;
        while (current) {
            chain.push_back(current.get());
            current = current->baseType;
        }
        std::reverse(chain.begin(), chain.end());

        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.push_back(builder->getPtrTy());
        for (const auto* info : chain) {
            for (const auto& field : info->fields) {
                fieldTypes.push_back(toLLVMType(field.type));
            }
        }

        structTy->setBody(fieldTypes, false);

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
        return builder->CreateGlobalString(std::get<std::string>(val), "str", 0, module.get());
    }
    return nullptr;
}

llvm::AllocaInst* LLVMCodegenVisitor::createEntryAlloca(llvm::Type* type, const std::string& name) const
{
    auto& entryBB = currentFunction->getEntryBlock();
    llvm::IRBuilder entryBuilder(&entryBB, entryBB.begin());
    return entryBuilder.CreateAlloca(type, nullptr, name);
}

static bool needsDescriptorInit(const std::shared_ptr<TypeInfo>& type) {
    if (type->kind == TypeKind::Struct) return true;
    if (type->kind == TypeKind::Array) return needsDescriptorInit(type->baseType);
    return false;
}

void LLVMCodegenVisitor::initializeDescriptors(llvm::Value* ptr, const std::shared_ptr<TypeInfo>& type)
{
    if (!needsDescriptorInit(type)) return;
    if (type->kind == TypeKind::Struct) {
        builder->CreateStore(descriptors[type->id], ptr);

        auto* ty = toLLVMType(type);

        std::vector<TypeInfo*> chain;
        auto current = type;
        while (current) {
            chain.push_back(current.get());
            current = current->baseType;
        }
        std::reverse(chain.begin(), chain.end());

        int i = 1;
        for (const auto* typeInfo : chain) {
            for (const auto& f : typeInfo->fields) {
                auto* fieldPtr = builder->CreateStructGEP(ty, ptr, i++);
                initializeDescriptors(fieldPtr, f.type);
            }
        }
    }
    if (type->kind == TypeKind::Array) {
        auto* arrayTy = llvm::cast<llvm::ArrayType>(toLLVMType(type));

        auto* condBB = llvm::BasicBlock::Create(context, "descInit.cond", currentFunction);
        auto* bodyBB = llvm::BasicBlock::Create(context, "descInit.body", currentFunction);
        auto* stepBB = llvm::BasicBlock::Create(context, "descInit.step", currentFunction);
        auto* afterBB = llvm::BasicBlock::Create(context, "descInit.end", currentFunction);
        auto* counterPtr = createEntryAlloca(builder->getInt64Ty(), "descInit.counter");
        builder->CreateStore(builder->getInt64(0), counterPtr);

        builder->CreateBr(condBB);
        builder->SetInsertPoint(condBB);

        auto* cur = builder->CreateLoad(builder->getInt64Ty(), counterPtr, "descInit.counter");
        auto* cond = builder->CreateICmpSLT(cur, builder->getInt64(type->length));
        builder->CreateCondBr(cond, bodyBB, afterBB);

        builder->SetInsertPoint(bodyBB);
        auto* elemPtr = builder->CreateGEP(arrayTy, ptr, {builder->getInt64(0), cur});
        initializeDescriptors(elemPtr, type->baseType);

        builder->CreateBr(stepBB);
        builder->SetInsertPoint(stepBB);
        auto* next = builder->CreateAdd(cur, builder->getInt64(1), "descInit.counter.inc");
        builder->CreateStore(next, counterPtr);
        builder->CreateBr(condBB);

        builder->SetInsertPoint(afterBB);
    }
}

llvm::GlobalVariable* LLVMCodegenVisitor::createStructDescriptor(const std::shared_ptr<TypeInfo>& type)
{
    std::string name = "struct_desc_" + type->name;
    int64_t depth = type->depth;

    auto* ptrType = builder->getPtrTy();
    auto* intType = builder->getInt64Ty();
    auto* arrayType = llvm::ArrayType::get(ptrType, depth + 1);
    auto* structTy = llvm::StructType::create({intType, arrayType}, getMangledName(currentModule, name));
    auto linkage = (importedModule || exportedType)
                       ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::InternalLinkage;

    auto* gVar = new llvm::GlobalVariable(
        *module,
        structTy,
        true,
        linkage,
        nullptr,
        getMangledName(currentModule, name)
    );
    if (importedModule)
        return gVar;

    auto* depthVal = llvm::ConstantInt::get(intType, depth);
    std::vector<llvm::Constant*> basePtrs(depth + 1);

    if (type->baseType && descriptors.find(type->baseType->id) == descriptors.end()) {
        descriptors[type->baseType->id] = createStructDescriptor(type->baseType);
    }
    auto curType = type;
    while (curType != nullptr) {
        auto* descPtr = (curType->id != type->id) ? descriptors[curType->id] : gVar;
        basePtrs[curType->depth] = descPtr;
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

    builder->SetInsertPoint(startBB);
    auto* objDepth = builder->CreateLoad(builder->getInt64Ty(), left);
    auto* depthCheck = builder->CreateICmpSGE(objDepth, builder->getInt64(rDepth));
    builder->CreateCondBr(depthCheck, contBB, endBB);

    builder->SetInsertPoint(contBB);

    auto* arrayTy = llvm::ArrayType::get(builder->getPtrTy(), 0);
    auto* structTy = llvm::StructType::get(context, {builder->getInt64Ty(), arrayTy});

    std::vector<llvm::Value*> indices = {
        builder->getInt64(0),
        builder->getInt32(1),
        builder->getInt64(rDepth)
    };

    auto* ancestorPtrAddr = builder->CreateInBoundsGEP(structTy, left, indices, "ancestor.addr");
    auto* ancestorPtr = builder->CreateLoad(builder->getPtrTy(), ancestorPtrAddr, "ancestor.tag");
    auto* instCheck = builder->CreateICmpEQ(ancestorPtr, right, "is.inst");
    builder->CreateBr(endBB);

    builder->SetInsertPoint(endBB);
    auto* phi = builder->CreatePHI(builder->getInt1Ty(), 2, "is.res");
    phi->addIncoming(builder->getFalse(), startBB);
    phi->addIncoming(instCheck, contBB);
    lastValue = phi;
}

void LLVMCodegenVisitor::createModuleInitializer(Module& node)
{
    auto funcName = getMangledName(node.name, std::to_string(node.name.length()) + node.name);
    auto fnType = llvm::FunctionType::get(builder->getVoidTy(), false);
    auto func = llvm::Function::Create(fnType, llvm::GlobalValue::ExternalLinkage, funcName, *module);
    functions[funcName] = func;

    if (importedModule) return;
    currentFunction = func;

    auto* gVar = new llvm::GlobalVariable(
        *module,
        builder->getInt1Ty(),
        false,
        llvm::GlobalValue::InternalLinkage,
        builder->getFalse(),
        "ob_" + node.name + "_visited"
    );

    auto* entryBB = llvm::BasicBlock::Create(context, "entry", func);
    auto* firstBB = llvm::BasicBlock::Create(context, "first", func);
    auto* otherBB = llvm::BasicBlock::Create(context, "other", func);
    builder->SetInsertPoint(entryBB);

    auto* visited = builder->CreateLoad(builder->getInt1Ty(), gVar);
    builder->CreateCondBr(visited, otherBB, firstBB);

    builder->SetInsertPoint(otherBB);
    builder->CreateRetVoid();

    builder->SetInsertPoint(firstBB);
    builder->CreateStore(builder->getTrue(), gVar);

    for (auto& import : node.imports) {
        auto callee = functions[getMangledName(import->realName,
                                               std::to_string(import->realName.length()) + import->realName)];
        builder->CreateCall(fnType, callee);
    }

    for (auto& [t, v] : globalsForDescInit) {
        initializeDescriptors(v, t);
    }

    if (auto it = functions.find(getMangledName(node.name, "init")); it != functions.end()) {
        builder->CreateCall(fnType, it->second);
    }

    builder->CreateRetVoid();
    currentFunction = nullptr;
}

void LLVMCodegenVisitor::createEntryPoint(Module& node)
{
    if (importedModule) return;

    auto* mainTy = llvm::FunctionType::get(
        builder->getInt32Ty(),
        {builder->getInt32Ty(), builder->getPtrTy()},
        false);
    auto* mainFn = llvm::Function::Create(mainTy, llvm::GlobalValue::ExternalLinkage, "main", *module);
    currentFunction = mainFn;

    auto* entry = llvm::BasicBlock::Create(context, "entry", mainFn);
    builder->SetInsertPoint(entry);

    if (rootModule->properties.needsGC) {
        auto gcInitFn = module->getOrInsertFunction(
            "GC_init",
            llvm::FunctionType::get(builder->getVoidTy(), false));
        builder->CreateCall(gcInitFn);
    }
    if (rootModule->properties.needsArgs) {
        auto setArgsFn = module->getOrInsertFunction(
            getMangledName("Args", "SetArgs"),
            llvm::FunctionType::get(
                builder->getVoidTy(),
                {builder->getInt64Ty(), builder->getPtrTy()},
                false));
        auto argIt = mainFn->arg_begin();
        llvm::Value* argc = argIt++;
        llvm::Value* argv = argIt;
        auto argc64 = builder->CreateSExtOrTrunc(argc, builder->getInt64Ty());
        builder->CreateCall(setArgsFn, {argc64, argv});
    }

    builder->CreateCall(functions[getMangledName(node.name, std::to_string(node.name.length()) + node.name)]);

    builder->CreateRet(builder->getInt32(0));
    currentFunction = nullptr;
}
}
