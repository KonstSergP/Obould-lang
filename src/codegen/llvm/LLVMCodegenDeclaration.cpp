#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(ConstantDeclaration& node) {} // константы вычисляются на этапе семантического анализа

void LLVMCodegenVisitor::visit(TypeDeclaration& node)
{
    node.type->accept(*this);
}

void LLVMCodegenVisitor::visit(VariableDeclaration& node)
{
    node.type->accept(*this);
    auto info = node.type->resolvedType;
    auto* ty = toLLVMType(info);

    auto* initValue = llvm::Constant::getNullValue(ty);
    if (info->kind == TypeKind::Struct) {
        auto* structTy = llvm::cast<llvm::StructType>(ty);
        std::vector<llvm::Constant*> fields;
        fields.push_back(descriptors[info.get()]);
        for (int i = 1; i < structTy->getNumElements(); i++) {
            fields.push_back(llvm::Constant::getNullValue(structTy->getElementType(i)));
        }
        initValue = llvm::ConstantStruct::get(structTy, fields);
    }

    auto linkage = (node.isExported || importedModule)
                       ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::InternalLinkage;
    if (currentFunction == nullptr) {
        new llvm::GlobalVariable(
            *module,
            ty,
            false,
            linkage,
            importedModule ? nullptr : initValue,
            getMangledName(currentModule, node.name)
        );
    }
    else {
        auto* allocaInst = createEntryAlloca(ty, node.name);
        builder->CreateStore(initValue, allocaInst);
        if (info->kind == TypeKind::Struct) {
            auto* fieldPtr = builder->CreateStructGEP(ty, allocaInst, 0);
            builder->CreateStore(descriptors[info.get()], fieldPtr);
        }
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
    if (auto it = functions.find(node.name); it != functions.end()) {
        return it->second;
    }

    auto* fnType = createFunctionType(node.resolvedType);
    auto linkage = (node.isExported || importedModule)
                       ? llvm::GlobalValue::ExternalLinkage
                       : llvm::Function::InternalLinkage;
    auto* fn = llvm::Function::Create(fnType, linkage, getMangledName(currentModule, node.name), *module);

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
    auto* fn = declareFunction(node);
    if (!fn->empty() || importedModule) {
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
            auto info = paramDecl->type->resolvedType;
            llvm::Value* val;
            if (paramDecl->isReference || info->kind == TypeKind::Array || info->kind == TypeKind::Struct) {
                val = &arg;
            }
            else {
                val = createEntryAlloca(arg.getType(), std::string(arg.getName()));
                builder->CreateStore(&arg, val);
            }
            locals[paramDecl->name] = val;

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

    auto* retType = fn->getReturnType();
    if (retType->isVoidTy()) {
        builder->CreateRetVoid();
    }
    else {
        node.returnExpression->accept(*this);
        auto* retVal = lastValue;
        if (!retVal) {
            retVal = llvm::Constant::getNullValue(retType);
        }
        builder->CreateRet(retVal);
    }
    currentFunction = nullptr;
}

void LLVMCodegenVisitor::visit(ProcedureParameter& node) {}

void LLVMCodegenVisitor::visit(Import& node)
{
    importedModule = true;
    auto curModule = std::move(currentModule);
    currentModule = node.module->name;
    node.module->accept(*this);
    currentModule = curModule;
    importedModule = false;
}

void LLVMCodegenVisitor::visit(Module& node)
{
    if (node.declarations)
        node.declarations->accept(*this);
    for (const auto& proc : node.procedures)
        declareFunction(*proc);
    for (const auto& proc : node.procedures)
        proc->accept(*this);
}
}
