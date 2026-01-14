#include "SemanticAnalyzer.h"
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "ast/ASTDeclarations.h"


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

    if (!symbolTable.addSymbol(sym)) {
        addError("Redeclaration of constant '" + node.name + "'");
    }
}

void SemanticAnalyzer::visit(TypeDeclaration& node)
{
    if (analyzeStage == AnalyzeStages::CreateType) {
        if (symbolTable.lookupSymbolLocal(node.name)) {
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

        symbolTable.addSymbol(sym);
    }
    else if (analyzeStage == AnalyzeStages::FillType) {
        node.type->accept(*this);

        Symbol* sym = symbolTable.lookupSymbolLocal(node.name);
        auto realType = node.type->resolvedType;
        auto stubType = sym->type;

        *stubType = *realType;
        stubType->name = node.name;
    }
    else if (analyzeStage == AnalyzeStages::ValidateType) {
        Symbol* sym = symbolTable.lookupSymbolLocal(node.name);
        auto type = sym->type;

        if (type->kind == TypeKind::Pointer) {
            auto base = type->baseType;
            if (base->kind != TypeKind::Struct) {
                addError("Pointer base type must be a Struct in '" + node.name + "'");
            }
        }
    }
}

void SemanticAnalyzer::visit(VariableDeclaration& node)
{
    node.type->accept(*this);

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

    if (!symbolTable.addSymbol(sym)) {
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

    if (node.returnType) {
        node.returnType->accept(*this);
        procType->returnType = node.returnType->resolvedType;
    }
    else {
        procType->returnType = std::make_shared<TypeInfo>(TypeKind::Void);
    }

    for (const auto& param : node.parameters) {
        param->type->accept(*this);

        ParamInfo info;
        info.name = param->name;
        info.type = param->type->resolvedType;
        info.isReference = param->isReference;

        procType->parameters.push_back(info);
    }

    Symbol procSym;
    procSym.name = node.name;
    procSym.kind = SymbolKind::Procedure;
    procSym.type = procType;
    procSym.isExported = node.isExported;
    procSym.isReference = false;
    procSym.isReadOnly = true;

    if (!symbolTable.addSymbol(procSym)) {
        addError("Redeclaration of symbol '" + node.name + "'");
    }

    symbolTable.enterScope();

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

        if (!symbolTable.addSymbol(paramSym)) {
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

    if (!node.returnType && node.returnExpression) {
        addError("Procedure '" + node.name + "' does not have a return type");
    }
    else if (node.returnType && !node.returnExpression) {
        addError("Procedure '" + node.name + "' does not have a return expression");
    }
    else if (node.returnType && node.returnExpression) {
        node.returnExpression->accept(*this);
        if (node.returnExpression->resolvedType
            && !procType->returnType->isAssignableFrom(node.returnExpression->resolvedType)) {
            addError("Return expression in procedure '" + node.name + "' does not match with return type");
        }
    }

    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(ProcedureParameter& node)
{
    addError("SemanticAnalyzer::visit(ProcedureParameter& node) must not be used");
}

void SemanticAnalyzer::visit(Import& node)
{
    Symbol modSym;
    modSym.name = node.localName;
    modSym.kind = SymbolKind::Module;
    modSym.type = std::make_shared<TypeInfo>(TypeKind::Void);
    modSym.isExported = false;
    modSym.isReference = false;
    modSym.isReadOnly = true;

    if (!symbolTable.addSymbol(modSym)) {
        addError("Module '" + node.localName + "' already imported");
    }
    // TODO: реализовать логику импорта модулей
}

static void addBuiltinTypes(SymbolTable& symTable)
{
    auto add = [&](const std::string& name, TypeKind kind)
    {
        Symbol s;
        s.name = name;
        s.kind = SymbolKind::Type;
        s.type = std::make_shared<TypeInfo>(kind);
        s.isExported = false;
        s.isReference = false;
        s.isReadOnly = true;
        symTable.addSymbol(s);
    };

    add("i64", TypeKind::i64);
    add("f64", TypeKind::f64);
    add("bool", TypeKind::Bool);
    add("byte", TypeKind::Byte);
    add("char", TypeKind::Char);
    add("void", TypeKind::Void);
}

void SemanticAnalyzer::visit(Module& node)
{
    symbolTable.enterScope();

    addBuiltinTypes(symbolTable);

    for (const auto& imp : node.imports) {
        imp->accept(*this);
    }

    if (node.declarations) {
        node.declarations->accept(*this);
    }

    for (const auto& proc : node.procedures) {
        proc->accept(*this);
    }

    symbolTable.exitScope();
}
