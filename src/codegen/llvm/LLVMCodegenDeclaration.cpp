#include "LLVMCodegen.h"
#include "sema/TypeInfo.h"


namespace obould
{
void LLVMCodegenVisitor::visit(ConstantDeclaration& node) {} // константы вычисляются на этапе семантического анализа

void LLVMCodegenVisitor::visit(TypeDeclaration& node)
{
    exportedType = node.isExported;
    node.type->accept(*this);
    exportedType = false;
}

void LLVMCodegenVisitor::visit(VariableDeclaration& node)
{
    exportedType = node.isExported;
    node.type->accept(*this);
    exportedType = false;
    auto& info = node.type->resolvedType;
    auto* ty = toLLVMType(info);

    auto linkage = (node.isExported || importedModule)
                       ? llvm::GlobalValue::ExternalLinkage
                       : llvm::GlobalValue::InternalLinkage;
    if (currentFunction == nullptr) {
        auto* g = new llvm::GlobalVariable(
            *module,
            ty,
            false,
            linkage,
            nullptr,
            getMangledName(currentModule, node.name)
        );
        if (!importedModule) {
            g->setInitializer(llvm::Constant::getNullValue(ty));
            globalsForDescInit.emplace_back(info, g);
        }
    }
    else {
        auto* allocaInst = createEntryAlloca(ty, node.name);
        uint64_t allocSize = module->getDataLayout().getTypeAllocSize(ty);
        if (allocSize <= 1024) {
            builder->CreateStore(llvm::Constant::getNullValue(ty), allocaInst);
        } else {
            builder->CreateMemSet(allocaInst, builder->getInt8(0), allocSize, llvm::MaybeAlign(1));
        }
        initializeDescriptors(allocaInst, info);
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
    if (auto it = functions.find(getMangledName(currentModule, node.name)); it != functions.end()) {
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
    functions[getMangledName(currentModule, node.name)] = fn;
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
    std::vector<uint32_t> arrays;
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
                arrays.push_back(info->id);
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
        if (node.resolvedType->returnType->kind == TypeKind::Char
            && node.returnExpression->resolvedType->kind == TypeKind::String
            && node.returnExpression->resolvedType->length == 1) {
            retVal = builder->CreateLoad(builder->getInt8Ty(), retVal);
        }
        builder->CreateRet(retVal);
    }
    currentFunction = nullptr;
}

void LLVMCodegenVisitor::visit(ProcedureParameter& node) {}

void LLVMCodegenVisitor::visit(Import& node)
{
    if (!node.module) {
        return;
    }
    auto curModule = std::move(currentModule);
    auto curMain = isMainModule;
    auto wasImportedModule = importedModule;
    currentModule = node.module->name;
    importedModule = true;
    isMainModule = false;

    node.module->accept(*this);

    currentModule = curModule;
    importedModule = wasImportedModule;
    isMainModule = curMain;
}

void LLVMCodegenVisitor::visit(Module& node)
{
    for (auto& import : node.imports) {
        import->accept(*this);
    }

    if (node.declarations)
        node.declarations->accept(*this);
    for (const auto& proc : node.procedures)
        declareFunction(*proc);
    for (const auto& proc : node.procedures)
        proc->accept(*this);

    createModuleInitializer(node);

    if (isMainModule)
        createEntryPoint(node);
}
}
