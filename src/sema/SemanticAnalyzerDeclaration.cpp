#include <set>
#include "SemanticAnalyzer.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "ast/ASTDeclarations.h"


namespace obould
{
void SemanticAnalyzer::visit(ConstantDeclaration& node)
{
    node.value->accept(*this);

    if (!node.value->resolvedType) {
        addError("Can't resolve type of constant: " + node.name);
        return;
    }

    if (!node.value->constantValue.has_value()) {
        addError("Expression must be constant: " + node.name);
        return;
    }

    Symbol sym;
    sym.name = node.name;
    sym.kind = SymbolKind::Constant;
    sym.type = node.value->resolvedType;
    sym.isExported = node.isExported;
    sym.isReference = false;
    sym.isReadOnly = true;
    sym.value = node.value->constantValue;

    if (!symbolTables[currentModuleName].addSymbol(sym)) {
        addError("Redeclaration of constant '" + node.name + "'");
    }
}

static void updateStructDepth(const std::shared_ptr<TypeInfo>& type)
{
    if (auto base = type->baseType) {
        if (base->depth == -1) updateStructDepth(base);
        type->depth = base->depth + 1;
    }
    else type->depth = 0;
}

static bool containsRecursive(const std::shared_ptr<TypeInfo>& t,
                              const TypeInfo* target,
                              std::set<TypeInfo*>& visiting)
{
    if (!t) return false;
    if (!visiting.insert(t.get()).second) return false;
    if (t.get() == target) return true;

    switch (t->kind) {
    case TypeKind::Struct:
        if (t->baseType && containsRecursive(t->baseType, target, visiting)) return true;
        for (const auto& f : t->fields) {
            if (containsRecursive(f.type, target, visiting)) return true;
        }
        return false;
    case TypeKind::Array:
        return containsRecursive(t->baseType, target, visiting);
    default:
        return false;
    }
}

void SemanticAnalyzer::validateType(const std::shared_ptr<TypeInfo>& type)
{
    std::set<TypeInfo*> visits;
    if (type->kind == TypeKind::Pointer) {
        auto base = type->baseType;
        if (base->kind != TypeKind::Struct) {
            addError("Pointer base type must be a Struct in '" + type->name + "'");
        }
    }
    else if (type->kind == TypeKind::Struct) {
        if (type->baseType && type->baseType->kind != TypeKind::Struct) {
            addError("Struct base type must be a struct");
        }
        if (type->baseType && type->name == type->baseType->name) {
            addError("Struct cannot use itself as base type");
        }
        if (type->baseType && containsRecursive(type->baseType, type.get(), visits)) {
            addError("Struct " + type->name + " cannot be extension of itself");
        }
        visits.clear();
        for (const auto& f : type->fields) {
            if (containsRecursive(f.type, type.get(), visits)) {
                addError("Struct " + type->name + " cannot have itself as a field");
            }
        }
        updateStructDepth(type);
    }
    else if (type->kind == TypeKind::Array) {
        if (containsRecursive(type->baseType, type.get(), visits)) {
            addError("Array " + type->name + " cannot use itself as element type");
        }
    }
    else if (type->kind == TypeKind::Procedure) {
        if (type->returnType->kind == TypeKind::Array || type->returnType->kind == TypeKind::Struct) {
            addError(
                "Procedure cannot return a structured type (Array or Record). Use a pointer or a reference parameter instead.");
        }
        // TODO: проверка параметров
        // TODO: проверка что инит имеет правильную сигнатуру
    }
    // TODO: проверки типов элементов
}

void SemanticAnalyzer::visit(TypeDeclaration& node)
{
    if (analyzeStage == AnalyzeStages::CreateType) {
        if (symbolTables[currentModuleName].lookupSymbolLocal(node.name)) {
            addError("Redeclaration of type '" + node.name + "'");
            return;
        }

        auto stubType = std::make_shared<TypeInfo>(TypeKind::StubType);
        stubType->name = node.name;

        Symbol sym;
        sym.name = node.name;
        sym.kind = SymbolKind::Type;
        sym.type = stubType;
        sym.isExported = node.isExported;
        sym.isReference = false;
        sym.isReadOnly = true;

        symbolTables[currentModuleName].addSymbol(sym);
    }
    else if (analyzeStage == AnalyzeStages::FillType) {
        node.type->accept(*this);

        auto* sym = symbolTables[currentModuleName].lookupSymbolLocal(node.name);
        if (!sym) return;
        auto& realType = node.type->resolvedType;
        auto& symType = sym->type;

        *symType = *realType;
        realType = symType;
        symType->name = node.name;
    }
    else if (analyzeStage == AnalyzeStages::ValidateType) {
        auto* sym = symbolTables[currentModuleName].lookupSymbolLocal(node.name);
        if (!sym) return;
        validateType(sym->type);
    }
}

void SemanticAnalyzer::visit(VariableDeclaration& node)
{
    if (!node.type->resolvedType) {
        node.type->accept(*this);
        validateType(node.type->resolvedType);
    }

    auto type = node.type->resolvedType;
    if (!type || !isValidVariableType(type->kind)) {
        addError("Variable '" + node.name + "' has invalid type");
        return;
    }

    if (type->isOpenArray) {
        addError("Variable '" + node.name + "' cannot be an open array");
    }

    Symbol sym;
    sym.name = node.name;
    sym.kind = SymbolKind::Variable;
    sym.type = type;
    sym.isExported = node.isExported;
    sym.isReference = false;
    sym.isReadOnly = false;

    if (!symbolTables[currentModuleName].addSymbol(sym)) {
        addError("Redeclaration of variable '" + node.name + "'");
    }
}

void SemanticAnalyzer::visit(ConstantDeclarations& node)
{
    for (const auto& decl : node.constants) {
        decl->accept(*this);
    }
}

void SemanticAnalyzer::visit(TypeDeclarations& node)
{
    analyzeStage = AnalyzeStages::CreateType;
    for (const auto& decl : node.types) {
        decl->accept(*this);
    }
    analyzeStage = AnalyzeStages::FillType;
    for (const auto& decl : node.types) {
        decl->accept(*this);
    }
    analyzeStage = AnalyzeStages::ValidateType;
    for (const auto& decl : node.types) {
        decl->accept(*this);
    }
    analyzeStage = AnalyzeStages::Default;
}

void SemanticAnalyzer::visit(VariableDeclarations& node)
{
    for (const auto& decl : node.variables) {
        decl->accept(*this);
    }
}

void SemanticAnalyzer::visit(DeclarationsBlock& node)
{
    if (node.constants) node.constants->accept(*this);
    if (node.types) node.types->accept(*this);
    if (node.variables) node.variables->accept(*this);
}

void SemanticAnalyzer::visit(ProcedureDeclaration& node)
{
    auto procType = std::make_shared<TypeInfo>(TypeKind::Procedure);
    node.resolvedType = procType;

    if (node.returnType) {
        node.returnType->accept(*this);
        procType->returnType = node.returnType->resolvedType;
    }
    else {
        procType->returnType = getBuiltinType(TypeKind::Void);
    }

    for (const auto& param : node.parameters) {
        param->type->accept(*this);
        ParamInfo info;
        info.name = param->name;
        info.type = param->type->resolvedType;
        info.isReference = param->isReference;
        procType->parameters.push_back(info);
    }
    validateType(procType);

    Symbol procSym;
    procSym.name = node.name;
    procSym.kind = SymbolKind::Procedure;
    procSym.type = procType;
    procSym.isExported = node.isExported;
    procSym.isReference = false;
    procSym.isReadOnly = true;

    if (!symbolTables[currentModuleName].addSymbol(procSym)) {
        addError("Redeclaration of symbol '" + node.name + "'");
    }

    if (importedModule)
        return;

    symbolTables[currentModuleName].enterScope();

    for (const auto& param : node.parameters) {
        auto& paramType = param->type->resolvedType;

        bool isReadOnly = false;
        if (!param->isReference) {
            isReadOnly = paramType->kind == TypeKind::Array || paramType->kind == TypeKind::Struct;
        }

        Symbol paramSym;
        paramSym.name = param->name;
        paramSym.kind = SymbolKind::Variable;
        paramSym.type = paramType;
        paramSym.isExported = false;
        paramSym.isReference = param->isReference;
        paramSym.isReadOnly = isReadOnly;

        if (!symbolTables[currentModuleName].addSymbol(paramSym)) {
            addError("Duplicate parameter name '" + param->name + "'");
        }
    }

    if (node.declarations) {
        node.declarations->accept(*this);
    }

    if (node.body) {
        node.body->accept(*this);
    }
    else {
        addError("Procedure '" + node.name + "' does not have a body");
    }

    bool isVoid = procType->returnType->kind == TypeKind::Void;

    if (isVoid && node.returnExpression) {
        addError("Procedure '" + node.name + "' is void but returns a value");
    }
    else if (!isVoid && !node.returnExpression) {
        addError("Procedure '" + node.name + "' must return a value");
    }
    else if (!isVoid && node.returnExpression) {
        node.returnExpression->accept(*this);
        if (node.returnExpression->resolvedType
            && !procType->returnType->isAssignableFrom(node.returnExpression->resolvedType)) {
            addError("Return expression in procedure '" + node.name + "' does not match with return type");
        }
    }
    symbolTables[currentModuleName].exitScope();
}

void SemanticAnalyzer::visit(ProcedureParameter& node) {}

void SemanticAnalyzer::visit(Import& node)
{
    if (node.localName.empty())
        node.localName = node.realName;
    moduleRealNames[node.localName] = node.realName;
    if (symbolTables.find(node.realName) != symbolTables.end())
        return;
    auto mod = symbolFileParser.parse(node.realName);
    auto curName = currentModuleName;
    auto curModuleNames = std::move(moduleRealNames);

    importedModule = true;
    currentModuleName = node.realName;
    moduleRealNames[node.realName] = node.realName;

    mod->accept(*this);

    importedModule = false;
    currentModuleName = curName;
    moduleRealNames = curModuleNames;
    node.module = std::move(mod);
}

void SemanticAnalyzer::visit(Module& node)
{
    symbolTables.try_emplace(currentModuleName);
    symbolTables[currentModuleName].enterScope();
    addBuiltinTypes(symbolTables[currentModuleName]);
    if (!importedModule)
        addBuiltinProcedures(symbolTables[currentModuleName]);

    for (const auto& imp : node.imports) {
        imp->accept(*this);
    }

    if (node.declarations) {
        node.declarations->accept(*this);
    }

    for (const auto& proc : node.procedures) {
        proc->accept(*this);
    }
}
}
