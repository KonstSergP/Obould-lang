#include "SemanticAnalyzer.h"
#include "TypeInfo.h"


void SemanticAnalyzer::visit(ProcedureCall& node)
{
    node.procedureName->accept(*this);

    // TODO: реализовать Type Guard
    auto procTypeInfo = node.procedureName->resolvedType;
    if (!procTypeInfo || procTypeInfo->kind != TypeKind::Procedure) {
        addError("Expression is not a procedure");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    const auto& params = procTypeInfo->parameters;

    if (node.args.size() != params.size()) {
        addError("Incorrect number of arguments. Expected " + std::to_string(params.size()) +
            ", got " + std::to_string(node.args.size()));
        return;
    }

    for (size_t i = 0; i < node.args.size(); ++i) {
        node.args[i]->accept(*this);
        auto argType = node.args[i]->resolvedType;
        const auto& paramInfo = params[i];

        if (!paramInfo.type->isAssignableFrom(argType)) {
            addError("Argument " + std::to_string(i + 1) + " type mismatch");
        }

        if (paramInfo.isReference) {
            if (!node.args[i]->isLvalue) {
                addError(
                    "Argument " + std::to_string(i + 1) +
                    " corresponds to a reference parameter and must be a l-value");
            }
        }
    }

    node.resolvedType = procTypeInfo->returnType;
}

void SemanticAnalyzer::visit(StatementsBlock& node)
{
    for (const auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void SemanticAnalyzer::visit(AssignmentStatement& node)
{
    node.target->accept(*this);
    node.value->accept(*this);

    if (!node.target->isLvalue) {
        addError("Left side of assignment must be a l-value");
        return;
    }

    auto lhsType = node.target->resolvedType;
    auto rhsType = node.value->resolvedType;

    if (!lhsType || !rhsType || !lhsType->isAssignableFrom(rhsType)) {
        addError("Type mismatch in assignment");
    }
}

void SemanticAnalyzer::visit(IfStatement& node)
{
    node.condition->accept(*this);

    if (!node.condition->resolvedType || node.condition->resolvedType->kind != TypeKind::Bool) {
        addError("'if' condition must be boolean");
    }

    node.thenBranch->accept(*this);
    if (node.elseBranch) node.elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(WhileStatement& node)
{
    for (const auto& branch : node.branches) {
        branch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileBranch& node)
{
    node.condition->accept(*this);

    if (!node.condition->resolvedType || node.condition->resolvedType->kind != TypeKind::Bool) {
        addError("'while/elsif' condition must be boolean");
    }

    node.body->accept(*this);
}

void SemanticAnalyzer::visit(DoWhileStatement& node)
{
    node.body->accept(*this);
    node.condition->accept(*this);

    if (!node.condition->resolvedType || node.condition->resolvedType->kind != TypeKind::Bool) {
        addError("'Do-While' condition must be boolean");
    }
}

void SemanticAnalyzer::visit(ForStatement& node)
{
    symbolTable.enterScope();

    Symbol sym;
    sym.name = node.counterName;
    sym.kind = SymbolKind::Variable;
    sym.type = std::make_shared<TypeInfo>(TypeKind::i64);
    sym.isExported = false;
    sym.readOnly = true;
    symbolTable.addSymbol(sym);

    node.rangeStart->accept(*this);
    if (!node.rangeStart->resolvedType || !isIntegerType(node.rangeStart->resolvedType->kind)) {
        addError("FOR start value must be integer");
    }

    node.rangeEnd->accept(*this);
    if (!node.rangeEnd->resolvedType || !isIntegerType(node.rangeEnd->resolvedType->kind)) {
        addError("FOR end value must be integer");
    }

    if (node.step) {
        node.step->accept(*this);
        if (!node.step->resolvedType || !isIntegerType(node.step->resolvedType->kind)) {
            addError("FOR step value must be integer");
        }
        if (!node.step->constantValue.has_value()) {
            addError("FOR step must be a constant expression");
        }
    }

    node.body->accept(*this);
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(SwitchStatement& node)
{
    node.selector->accept(*this);
    auto selType = node.selector->resolvedType;
    if (!selType) return;
    forCounterType = selType;

    bool isValidSelector = isIntegerType(selType->kind) || selType->kind == TypeKind::Char;
    if (!isValidSelector) {
        addError("CASE selector must be integer or char");
    }

    for (const auto& caseBlock : node.cases) {
        caseBlock->accept(*this);
    }
    forCounterType = nullptr;
}

void SemanticAnalyzer::visit(SwitchCase& node)
{
    for (const auto& label : node.labels) {
        label->accept(*this);
    }
    const auto& cntrType = forCounterType; // если в теле будет еще один switch
    node.body->accept(*this);
    forCounterType = cntrType;
}

void SemanticAnalyzer::visit(CaseLabel& node)
{
    node.value->accept(*this);
    if (!node.value->resolvedType || !forCounterType->isAssignableFrom(node.value->resolvedType)) {
        addError("Case label type mismatch with selector");
    }
    if (!node.value->constantValue.has_value()) {
        addError("Case label must be a constant");
    }

    if (node.endValue) {
        node.endValue->accept(*this);
        if (!node.endValue->resolvedType || !forCounterType->isAssignableFrom(node.endValue->resolvedType)) {
            addError("Case range end type mismatch with selector");
        }
        if (!node.endValue->constantValue.has_value()) {
            addError("Case range end must be a constant");
        }
    }
}
