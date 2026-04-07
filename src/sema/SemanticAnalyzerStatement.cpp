#include "SemanticAnalyzer.h"
#include "TypeInfo.h"


namespace obould
{
void SemanticAnalyzer::visitBuiltinProcedure(ProcedureCall& node)
{
    for (auto& arg : node.args) {
        arg->accept(*this);
    }

    switch (node.procedureName->resolvedType->builtin) {
    case BuiltinKind::NEW:
    {
        if (node.args.size() != 1) {
            addError("'new' expects 1 argument");
            return;
        }
        auto& arg = node.args[0];
        if (arg->resolvedType->kind != TypeKind::Pointer || arg->resolvedType->baseType->kind != TypeKind::Struct) {
            addError("'new' requires a pointer to struct");
        }
        else if (!arg->isLvalue) {
            addError("'new' requires a variable (l-value)");
        }
        rootModule->properties.needsGC = true;
    }
    break;
    case BuiltinKind::LEN:
    {
        if (node.args.size() != 1) {
            addError("'len' expects 1 argument");
            return;
        }
        auto argType = node.args[0]->resolvedType;
        if (argType->kind != TypeKind::Array && argType->kind != TypeKind::String) {
            addError("'len' expects an array or string");
        }
    }
    break;
    case BuiltinKind::ASSERT:
    {
        if (node.args.empty() || node.args.size() > 2) {
            addError("ASSERT expects 1 or 2 arguments");
            return;
        }
        if (node.args[0]->resolvedType->kind != TypeKind::Bool) {
            addError("First argument of ASSERT must be a boolean expression");
        }
        if (node.args.size() == 2) {
            if (node.args[1]->resolvedType->kind != TypeKind::String) {
                addError("Second argument of ASSERT must be a string literal");
            }
        }
    }
    break;
    default:
        addError("Unknown builtin kind");
    }
    node.resolvedType = node.procedureName->resolvedType->returnType;
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(ProcedureCall& node)
{
    node.procedureName->accept(*this);

    auto procTypeInfo = node.procedureName->resolvedType;

    if (procTypeInfo->builtin != BuiltinKind::None) {
        visitBuiltinProcedure(node);
        return;
    }

    if (procTypeInfo->kind == TypeKind::Procedure) {
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

            bool correctArgType;
            if (paramInfo.isReference) {
                correctArgType = (paramInfo.type.get() == argType.get())
                    || (paramInfo.type->kind == TypeKind::Struct && paramInfo.type->isBaseTypeOf(argType))
                    || paramInfo.type->isArrayConvertibleFrom(argType);
            }
            else {
                correctArgType = paramInfo.type->isAssignableFrom(argType)
                    || paramInfo.type->isArrayConvertibleFrom(argType);
            }

            if (!correctArgType) {
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
        node.isLvalue = false;
    }
    else if (auto baseType = getPolymorphicBase(node.procedureName.get())) {
        node.isTypeGuard = true;
        if (node.args.size() != 1) {
            addError("Type guard requires only one argument");
            return;
        }

        auto* idExpr = dynamic_cast<IdentifierExpression*>(node.args[0].get());
        if (!idExpr) {
            addError("Type guard argument must be a type identifier");
            return;
        }

        IdentifierType tempTypeNode(idExpr->moduleName, idExpr->name);
        tempTypeNode.accept(*this);
        auto targetType = tempTypeNode.resolvedType;
        node.args[0]->resolvedType = targetType;

        if (!targetType || targetType->kind != TypeKind::Struct) {
            addError("Type guard target type must be a struct");
            node.resolvedType = getBuiltinType(TypeKind::Void);
            return;
        }

        if (!baseType->isBaseTypeOf(targetType)) {
            addError("Type guard extension mismatch");
            return;
        }

        if (node.procedureName->resolvedType->kind == TypeKind::Pointer) {
            auto res = std::make_shared<TypeInfo>(TypeKind::Pointer);
            res->baseType = targetType;
            node.resolvedType = res;
            node.isLvalue = false;
        }
        else {
            node.resolvedType = targetType;
            node.isLvalue = true;
        }
    }
    else {
        addError("Expression is not a procedure or type guard");
        node.resolvedType = getBuiltinType(TypeKind::Void);
    }
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
    auto* sym = symbolTables[currentModuleName].lookupSymbol(node.counterName);
    if (!sym) {
        addError("For counter " + node.counterName + " is not declared.");
        return;
    }
    if (sym->kind != SymbolKind::Variable) {
        addError("For counter " + node.counterName + " must be a variable.");
        return;
    }

    bool prevReadOnly = sym->isReadOnly;
    sym->isReadOnly = true;

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
    sym->isReadOnly = prevReadOnly;
}

void SemanticAnalyzer::visit(SwitchStatement& node)
{
    node.selector->accept(*this);
    auto selType = node.selector->resolvedType;
    if (!selType) return;

    if (selType->kind == TypeKind::Pointer || selType->kind == TypeKind::Struct) {
        switchSelectorType = getPolymorphicBase(node.selector.get());
        if (!switchSelectorType) {
            addError("Type switch selector must be ptr/ref to struct");
            return;
        }
        for (const auto& caseBlock : node.cases) {
            caseBlock->accept(*this);
        }
    }
    else if (isIntegerType(selType->kind) || selType->kind == TypeKind::Char) {
        switchSelectorType = selType;
        for (const auto& caseBlock : node.cases) {
            caseBlock->accept(*this);
        }
    }
    else {
        addError("CASE selector must be integer, char or ptr/ref to struct");
    }
    switchSelectorType = nullptr;
}

void SemanticAnalyzer::visit(SwitchCase& node)
{
    for (const auto& label : node.labels) {
        label->accept(*this);
    }
    const auto& cntrType = switchSelectorType;
    node.body->accept(*this);
    switchSelectorType = cntrType;
}

void SemanticAnalyzer::visit(CaseLabel& node)
{
    if (switchSelectorType->kind == TypeKind::Struct) {
        auto* idExpr = dynamic_cast<IdentifierExpression*>(node.value.get());
        if (!idExpr) {
            addError("Struct switch label must be a type identifier");
            return;
        }
        IdentifierType tempTypeNode(idExpr->moduleName, idExpr->name);
        tempTypeNode.accept(*this);
        auto targetType = tempTypeNode.resolvedType;
        node.value->resolvedType = targetType;

        if (!switchSelectorType->isAssignableFrom(node.value->resolvedType)) {
            addError("Switch selector must be assignable of case label type");
        }
        if (node.endValue) {
            addError("Struct label must have only one value");
        }
        return;
    }

    node.value->accept(*this);
    if (!switchSelectorType->isAssignableFrom(node.value->resolvedType)) {
        addError("Case label type mismatch with selector");
    }
    if (!node.value->constantValue.has_value()) {
        addError("Case label must be a constant");
    }

    if (node.endValue) {
        node.endValue->accept(*this);
        if (!node.endValue->resolvedType || !switchSelectorType->isAssignableFrom(node.endValue->resolvedType)) {
            addError("Case range end type mismatch with selector");
        }
        if (!node.endValue->constantValue.has_value()) {
            addError("Case range end must be a constant");
        }
    }
}
}
