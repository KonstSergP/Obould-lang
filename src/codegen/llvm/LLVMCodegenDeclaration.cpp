#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"

namespace obould
{
void LLVMCodegenVisitor::visit(ConstantDeclaration& node) {}

void LLVMCodegenVisitor::visit(TypeDeclaration& node)
{
    node.type->accept(*this);
}

void LLVMCodegenVisitor::visit(VariableDeclaration& node)
{
    node.type->accept(*this);
    llvm::Type* ty = toLLVMType(node.type->resolvedType);

    if (currentFunction == nullptr) {
        auto* initValue = llvm::Constant::getNullValue(ty);
        auto* gVar = new llvm::GlobalVariable(
            *module,
            ty,
            false,
            llvm::GlobalValue::InternalLinkage,
            initValue,
            node.name
        );
        if (node.isExported) {
            gVar->setLinkage(llvm::GlobalValue::ExternalLinkage);
        }
    }
    else {
        auto* allocaInst = createEntryAlloca(ty, node.name);
        builder->CreateStore(llvm::Constant::getNullValue(ty), allocaInst);
        locals[node.name] = allocaInst;
    }
}

void LLVMCodegenVisitor::visit(ConstantDeclarations& node)
{
    for (const auto& decl : node.constants) {
        if (decl) {
            decl->accept(*this);
        }
    }
}

void LLVMCodegenVisitor::visit(TypeDeclarations& node)
{
    for (const auto& decl : node.types) {
        if (decl) {
            decl->accept(*this);
        }
    }
}

void LLVMCodegenVisitor::visit(VariableDeclarations& node)
{
    for (const auto& decl : node.variables) {
        if (decl) {
            decl->accept(*this);
        }
    }
}

void LLVMCodegenVisitor::visit(DeclarationsBlock& node)
{
    if (node.constants) {
        node.constants->accept(*this);
    }
    if (node.types) {
        node.types->accept(*this);
    }
    if (node.variables) {
        node.variables->accept(*this);
    }
}

llvm::Function* LLVMCodegenVisitor::declareFunction(ProcedureDeclaration& node)
{
    auto it = functions.find(node.name);
    if (it != functions.end()) {
        return it->second;
    }

    std::vector<llvm::Type*> paramTypes, hiddenParams;
    paramTypes.reserve(node.parameters.size());

    for (const auto& param : node.parameters) {
        auto paramType = param->type->resolvedType;
        if (param->isReference || paramType->kind == TypeKind::Array || paramType->kind == TypeKind::Struct) {
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

    auto* fnType = llvm::FunctionType::get(toLLVMType(node.returnType->resolvedType), paramTypes, false);
    auto* fn = llvm::Function::Create(fnType, llvm::Function::InternalLinkage, node.name, *module);
    if (node.isExported) {
        fn->setLinkage(llvm::GlobalValue::ExternalLinkage);
    }

    unsigned idx = 0;
    for (auto& arg : fn->args()) {
        arg.setName(node.parameters[idx]->name);
        ++idx;
        if (idx == node.parameters.size()) break;
    }

    functions[node.name] = fn;
    return fn;
}

void LLVMCodegenVisitor::visit(ProcedureDeclaration& node)
{
    llvm::Function* fn = declareFunction(node);
    if (!fn->empty()) {
        return;
    }
    currentFunction = fn;
    locals.clear();

    auto* entryBB = llvm::BasicBlock::Create(context, "entry", fn);
    builder->SetInsertPoint(entryBB);

    size_t idx = 0;
    std::vector<TypeInfo*> arrays;
    for (auto& arg : fn->args()) {
        if (idx < node.parameters.size()) {
            const auto& paramDecl = node.parameters[idx];
            auto* allocaInst = createEntryAlloca(arg.getType(), std::string(arg.getName()));
            builder->CreateStore(&arg, allocaInst);
            locals[paramDecl->name] = allocaInst;

            auto& info = node.parameters[idx]->type->resolvedType;
            while (info->isOpenArray) {
                arrays.push_back(info.get());
                info = info->baseType;
            }
        }
        else {
            auto* allocaInst = createEntryAlloca(arg.getType(), std::string(arg.getName()));
            builder->CreateStore(&arg, allocaInst);
            lengths[arrays[idx - node.parameters.size()]] = allocaInst;
        }
        ++idx;
    }

    if (node.declarations) {
        node.declarations->accept(*this);
    }
    if (node.body) {
        node.body->accept(*this);
    }

    llvm::Type* retType = fn->getReturnType();
    if (retType->isVoidTy()) {
        builder->CreateRetVoid();
    }
    else {
        node.returnExpression->accept(*this);
        llvm::Value* retVal = lastValue;
        if (!retVal) {
            retVal = llvm::Constant::getNullValue(retType);
        }
        builder->CreateRet(retVal);
    }
}

void LLVMCodegenVisitor::visit(ProcedureParameter& node) {}

void LLVMCodegenVisitor::visit(Import& node) {}

void LLVMCodegenVisitor::visit(Module& node)
{
    node.declarations->accept(*this);

    for (const auto& proc : node.procedures) {
        declareFunction(*proc);
    }
    for (const auto& proc : node.procedures) {
        proc->accept(*this);
    }
}
}
