#include "SemanticAnalyzer.h"
#include "TypeInfo.h"


namespace obould
{
void SemanticAnalyzer::visit(IntegerLiteral& node)
{
    node.resolvedType = std::make_shared<TypeInfo>(TypeKind::i64);
    node.constantValue = node.value;
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(RealLiteral& node)
{
    node.resolvedType = std::make_shared<TypeInfo>(TypeKind::f64);
    node.constantValue = node.value;
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(BooleanLiteral& node)
{
    node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);
    node.constantValue = node.value;
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(StringLiteral& node)
{
    auto type = std::make_shared<TypeInfo>(TypeKind::String);
    type->length = static_cast<int64_t>(node.value.length());
    node.resolvedType = type;
    node.constantValue = node.value;
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(Nil& node)
{
    node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Nil);
    node.isLvalue = false;
}

void SemanticAnalyzer::visit(IdentifierExpression& node)
{
    Symbol* sym = nullptr;
    if (node.moduleName.empty()) {
        sym = symbolTables[currentTableName].lookupSymbol(node.name);
    }
    else {
        if (symbolTables.find(node.moduleName) == symbolTables.end()) {
            addError("Can't find module '" + node.moduleName + "'");
            node.resolvedType = std::make_shared<TypeInfo>();
            return;
        }
        sym = symbolTables[node.moduleName].lookupSymbol(node.name);
    }

    if (!sym) {
        addError("Undefined identifier: " + node.name);
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    if (sym->kind == SymbolKind::Type) {
        addError("Identifier '" + node.name + "' cannot be used as an expression");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    node.resolvedType = sym->type;

    if (sym->kind == SymbolKind::Constant && sym->value.has_value()) {
        node.constantValue = sym->value;
        node.isLvalue = false;
    }
    else if (sym->kind == SymbolKind::Procedure) {
        node.isLvalue = false;
    }
    else {
        node.isLvalue = not sym->isReadOnly;
    }
}

void SemanticAnalyzer::visit(UnaryExpression& node)
{
    node.operand->accept(*this);
    auto type = node.operand->resolvedType;

    if (!type) {
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    switch (node.op) {
    case UnaryExpression::Op::Not:
        if (type->kind != TypeKind::Bool) {
            addError("Operator '!' requires boolean operand");
            return;
        }
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);
        break;

    case UnaryExpression::Op::Negate:
    case UnaryExpression::Op::Plus:
        if (type->kind != TypeKind::i64 && type->kind != TypeKind::f64 && type->kind != TypeKind::Byte) {
            addError("Unary operator requires numeric operand");
        }
        node.resolvedType = type;
        break;
    }

    if (node.operand->constantValue.has_value()) {
        auto& val = node.operand->constantValue.value();
        switch (node.op) {
        case UnaryExpression::Op::Not:
            node.constantValue = not std::get<bool>(val);
            break;
        case UnaryExpression::Op::Negate:
            if (std::holds_alternative<int64_t>(val))
                node.constantValue = -std::get<int64_t>(val);
            else if (std::holds_alternative<double>(val))
                node.constantValue = -std::get<double>(val);
            break;
        case UnaryExpression::Op::Plus:
            node.constantValue = val;
            break;
        }
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node)
{
    using Op = BinaryExpression::Op;
    const bool isComparison =
        node.op == Op::Eq || node.op == Op::Neq ||
        node.op == Op::Lt || node.op == Op::Gt ||
        node.op == Op::Lte || node.op == Op::Gte;

    const bool isLogical =
        node.op == Op::And || node.op == Op::Or;

    const bool isArithmetic =
        node.op == Op::Add || node.op == Op::Sub ||
        node.op == Op::Mul || node.op == Op::IDiv ||
        node.op == Op::Mod || node.op == Op::FDiv;

    const auto isStringLike = [](const TypeInfo& type)
    {
        return type.kind == TypeKind::String ||
            (type.kind == TypeKind::Array && type.baseType->kind == TypeKind::Char);
    };

    node.isLvalue = false;
    node.left->accept(*this);
    std::visit([this](auto&& nd) { nd->accept(*this); }, node.right);

    auto lType = node.left->resolvedType;
    auto rType = std::visit([](auto&& nd) { return nd->resolvedType; }, node.right);
    if (!lType || !rType) {
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    auto lConst = node.left->constantValue;
    std::optional<ConstValue> rConst;
    if (std::holds_alternative<std::unique_ptr<Expression>>(node.right))
        rConst = std::get<std::unique_ptr<Expression>>(node.right)->constantValue;
    bool isConstExpr = lConst.has_value() && rConst.has_value();
    bool correct = true;


    switch (node.op) {
    case Op::Add:
    case Op::Sub:
    case Op::Mul:
        if (lType->kind == TypeKind::i64 || rType->kind == TypeKind::i64) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::i64);
        }
        else if (lType->kind == TypeKind::Byte && rType->kind == TypeKind::Byte) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Byte);
        }
        else if (isRealType(lType->kind) && isRealType(rType->kind)) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::f64);
        }
        else {
            addError("Arithmetic operators require compatible numeric operands (both int/byte or both real)");
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
            correct = false;
        }
        break;

    case Op::FDiv:
        if (isRealType(lType->kind) && isRealType(rType->kind)) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::f64);
        }
        else {
            addError("Real division requires real operands");
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
            correct = false;
        }
        break;

    case Op::IDiv:
    case Op::Mod:
        if (isIntegerType(lType->kind) && isIntegerType(rType->kind)) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::i64);
        }
        else {
            addError("Integer division/mod requires integer operands");
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
            correct = false;
        }
        break;

    case Op::And:
    case Op::Or:
        if (lType->kind == TypeKind::Bool && rType->kind == TypeKind::Bool) {
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);
        }
        else {
            addError("Logical operators require boolean operands");
            node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);
            correct = false;
        }
        break;

    case Op::Eq:
    case Op::Neq:
    case Op::Lt:
    case Op::Gt:
    case Op::Lte:
    case Op::Gte:
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);

        if (isIntegerType(lType->kind) && isIntegerType(rType->kind)) {}
        else if (isRealType(lType->kind) && isRealType(rType->kind)) {}
        else if (lType->kind == TypeKind::Char && rType->kind == TypeKind::Char) {}
        else if (lType->kind == TypeKind::Char && rType->kind == TypeKind::String && rType->length == 1) {}
        else if (rType->kind == TypeKind::Char && lType->kind == TypeKind::String && lType->length == 1) {}
        else if (isStringLike(*lType) && isStringLike(*rType)) {}
        else if (((lType->kind == TypeKind::Pointer && rType->kind == TypeKind::Nil)
            || (lType->kind == TypeKind::Nil && rType->kind == TypeKind::Pointer)
            || (lType->kind == TypeKind::Pointer && rType->kind == TypeKind::Pointer))) {
            if (node.op == Op::Eq || node.op == Op::Neq) {}
            else {
                addError("Pointers allow only equality comparisons");
                correct = false;
            }
        }
        else {
            addError("Comparison requires compatible types");
            correct = false;
        }
        break;

    case Op::Is:
    {
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Bool);

        auto baseType = getPolymorphicBase(node.left.get());
        if (!baseType) {
            addError("'IS' requires a pointer to struct or a reference struct parameter");
            correct = false;
            break;
        }

        if (!std::holds_alternative<std::unique_ptr<Type>>(node.right)) {
            addError("'IS' right operand must be a type");
            correct = false;
            break;
        }
        if (rType->kind != TypeKind::Struct) {
            addError("'IS' target type must be a struct");
            correct = false;
            break;
        }
        if (!baseType->isBaseTypeOf(rType)) {
            addError("'IS' target type must be an extension of the variable type");
            correct = false;
        }
        break;
    }

    default:
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        correct = false;
    }
    if (!correct) return;


    if (isConstExpr) {
        if (isIntegerType(lType->kind) && isIntegerType(rType->kind)) {
            int64_t lv = std::get<int64_t>(*lConst);
            int64_t rv = std::get<int64_t>(*rConst);

            if (isArithmetic || isComparison) {
                switch (node.op) {
                case Op::Add: node.constantValue = lv + rv;
                    break;
                case Op::Sub: node.constantValue = lv - rv;
                    break;
                case Op::Mul: node.constantValue = lv * rv;
                    break;
                case Op::IDiv:
                    if (rv <= 0) {
                        node.constantValue = 0;
                        addError("Divisor must be positive");
                        return;
                    }
                    node.constantValue = lv / rv - ((lv % rv != 0) && (lv < 0));
                    break;
                case Op::Mod:
                    if (rv == 0) {
                        node.constantValue = 0;
                        addError("Modulo by zero");
                        return;
                    }
                    node.constantValue = (lv % rv + rv) % rv;
                    break;
                case Op::Eq: node.constantValue = lv == rv;
                    break;
                case Op::Neq: node.constantValue = lv != rv;
                    break;
                case Op::Lt: node.constantValue = lv < rv;
                    break;
                case Op::Gt: node.constantValue = lv > rv;
                    break;
                case Op::Lte: node.constantValue = lv <= rv;
                    break;
                case Op::Gte: node.constantValue = lv >= rv;
                    break;
                default: break;
                }
            }
        }
        else if (lType->kind == TypeKind::f64 && rType->kind == TypeKind::f64) {
            double lv = std::get<double>(*lConst);
            double rv = std::get<double>(*rConst);

            if (isArithmetic || isComparison) {
                switch (node.op) {
                case Op::Add: node.constantValue = lv + rv;
                    break;
                case Op::Sub: node.constantValue = lv - rv;
                    break;
                case Op::Mul: node.constantValue = lv * rv;
                    break;
                case Op::FDiv:
                    if (rv == 0.0) {
                        node.constantValue = 0.0;
                        addError("Division by zero");
                        return;
                    }
                    node.constantValue = lv / rv;
                    break;
                case Op::Eq: node.constantValue = lv == rv;
                    break;
                case Op::Neq: node.constantValue = lv != rv;
                    break;
                case Op::Lt: node.constantValue = lv < rv;
                    break;
                case Op::Gt: node.constantValue = lv > rv;
                    break;
                case Op::Lte: node.constantValue = lv <= rv;
                    break;
                case Op::Gte: node.constantValue = lv >= rv;
                    break;
                default:
                    break;
                }
            }
        }
        else if (isLogical) {
            bool lv = std::get<bool>(*lConst);
            bool rv = std::get<bool>(*rConst);
            if (node.op == Op::And) node.constantValue = lv && rv;
            if (node.op == Op::Or) node.constantValue = lv || rv;
        }
        else if (lType->kind == TypeKind::String && rType->kind == TypeKind::String) {
            std::string lv = std::get<std::string>(*lConst);
            std::string rv = std::get<std::string>(*rConst);

            switch (node.op) {
            case Op::Eq: node.constantValue = lv == rv;
                break;
            case Op::Neq: node.constantValue = lv != rv;
                break;
            case Op::Lt: node.constantValue = lv < rv;
                break;
            case Op::Gt: node.constantValue = lv > rv;
                break;
            case Op::Lte: node.constantValue = lv <= rv;
                break;
            case Op::Gte: node.constantValue = lv >= rv;
                break;
            default: break;
            }
        }
    }
}

void SemanticAnalyzer::visit(ArrayAccessExpression& node)
{
    node.array->accept(*this);
    node.index->accept(*this);

    auto arrType = node.array->resolvedType;
    auto idxType = node.index->resolvedType;

    if (!arrType || arrType->kind != TypeKind::Array) {
        addError("Indexing requires an array type");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    if (!idxType || !isIntegerType(idxType->kind)) {
        addError("Array index must be an integer");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    node.resolvedType = arrType->baseType;
    node.isLvalue = node.array->isLvalue;
}

void SemanticAnalyzer::visit(MemberAccessExpression& node)
{
    node.object->accept(*this);
    auto objType = node.object->resolvedType;

    if (!objType) {
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    std::shared_ptr<TypeInfo> structInfo = nullptr;
    if (objType->kind == TypeKind::Pointer) {
        if (objType->baseType && objType->baseType->kind == TypeKind::Struct) {
            structInfo = objType->baseType;
        }
    }
    else if (objType->kind == TypeKind::Struct) {
        structInfo = objType;
    }

    if (!structInfo) {
        addError("Member access requires a record or a pointer to record");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    std::shared_ptr<TypeInfo> fieldType = nullptr;
    auto current = structInfo;
    while (current) {
        for (const auto& field : current->fields) {
            if (field.name == node.memberName) {
                fieldType = field.type;
                break;
            }
        }
        if (fieldType) break;
        current = current->baseType;
    }

    if (fieldType) {
        node.resolvedType = fieldType;
        if (objType->kind == TypeKind::Pointer) {
            node.isLvalue = true;
        }
        else {
            node.isLvalue = node.object->isLvalue;
        }
    }
    else {
        addError("Field '" + node.memberName + "' not found in record");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
    }
}

void SemanticAnalyzer::visit(DereferenceExpression& node)
{
    node.ptr->accept(*this);
    auto type = node.ptr->resolvedType;

    if (!type || type->kind != TypeKind::Pointer) {
        addError("Dereference operator requires a pointer");
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    node.resolvedType = type->baseType;
    node.isLvalue = true;
}

void SemanticAnalyzer::visit(QualifiedNameNode& node)
{
    if (std::holds_alternative<std::monostate>(node.realisation)) {
        if (symbolTables.find(node.first) != symbolTables.end()) {
            node.realisation = std::make_unique<IdentifierExpression>(node.first, node.second);
        }
        else {
            auto id = std::make_unique<IdentifierExpression>("", node.first);
            node.realisation = std::make_unique<MemberAccessExpression>(std::move(id), node.second);
        }
    }
    if (auto id = std::get_if<std::unique_ptr<IdentifierExpression>>(&node.realisation)) {
        (*id)->accept(*this);
        node.constantValue = (*id)->constantValue;
        node.isLvalue = (*id)->isLvalue;
        node.resolvedType = (*id)->resolvedType;
    }
    else if (auto member = std::get_if<std::unique_ptr<MemberAccessExpression>>(&node.realisation)) {
        (*member)->accept(*this);
        node.constantValue = (*member)->constantValue;
        node.isLvalue = (*member)->isLvalue;
        node.resolvedType = (*member)->resolvedType;
    }
}
}
