#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


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

llvm::Type* LLVMCodegenVisitor::toLLVMType(const std::shared_ptr<TypeInfo>& typeInfo)
{
    if (!typeInfo) {
        return llvm::Type::getVoidTy(context);
    }

    switch (typeInfo->kind) {
    case TypeKind::i64:
        return llvm::Type::getInt64Ty(context);
    case TypeKind::Byte:
    case TypeKind::Char:
        return llvm::Type::getInt8Ty(context);
    case TypeKind::Bool:
        return llvm::Type::getInt1Ty(context);
    case TypeKind::f64:
        return llvm::Type::getDoubleTy(context);
    case TypeKind::Void:
        return llvm::Type::getVoidTy(context);
    case TypeKind::Pointer:
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    case TypeKind::Array:
    {
        llvm::Type* elemTy = toLLVMType(typeInfo->baseType);
        if (typeInfo->isOpenArray) {
            return llvm::PointerType::getUnqual(elemTy);
        }
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

        for (const auto* info : chain) {
            for (const auto& field : info->fields) {
                fieldTypes.push_back(toLLVMType(field.type));
            }
        }

        std::string structName = "struct." + typeInfo->name;
        llvm::StructType* structTy = llvm::StructType::create(context, structName);
        structTy->setBody(fieldTypes, false);
        structTypes[typeInfo.get()] = structTy;
        return structTy;
    }
    case TypeKind::Procedure:
        return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
    default:
        return llvm::Type::getVoidTy(context);
    }
}

llvm::Value* LLVMCodegenVisitor::getConstantValue(const Expression& node)
{
    if (!node.constantValue.has_value()) return nullptr;
    auto& val = node.constantValue.value();

    if (std::holds_alternative<int64_t>(val)) {
        auto* ty = llvm::Type::getInt64Ty(context);
        return llvm::ConstantInt::getSigned(ty, std::get<int64_t>(val));
    }
    if (std::holds_alternative<double>(val)) {
        auto* ty = llvm::Type::getDoubleTy(context);
        return llvm::ConstantFP::get(ty, std::get<double>(val));
    }
    if (std::holds_alternative<bool>(val)) {
        return llvm::ConstantInt::getBool(context, std::get<bool>(val));
    }
    if (std::holds_alternative<std::string>(val)) {
        return builder->CreateGlobalStringPtr(std::get<std::string>(val), "str", 0, module.get());
    }
    return nullptr;
}

llvm::AllocaInst* LLVMCodegenVisitor::createEntryAlloca(llvm::Type* type, const std::string& name) const
{
    llvm::BasicBlock& entryBB = currentFunction->getEntryBlock();
    llvm::IRBuilder entryBuilder(&entryBB, entryBB.begin());
    return entryBuilder.CreateAlloca(type, nullptr, name);
}
