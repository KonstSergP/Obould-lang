#pragma once
#include <memory>
#include <string>
#include <utility>
#include "ASTCore.h"


namespace obould
{
class BinaryExpression : public Expression
{
public:
    enum class Op { Add, Sub, Mul, FDiv, IDiv, Mod, And, Or, Eq, Neq, Lt, Gt, Lte, Gte, Is, In };

    using RightType = std::variant<std::unique_ptr<Expression>, std::unique_ptr<Type>>;

    BinaryExpression(std::unique_ptr<Expression> l, RightType r, Op op)
        : left(std::move(l)), right(std::move(r)), op(op) {}

    void accept(ASTVisitor& v) override;

    std::unique_ptr<Expression> left;
    RightType right;
    Op op;
};


class UnaryExpression : public Expression
{
public:
    enum class Op { Negate, Not, Plus };

    UnaryExpression(std::unique_ptr<Expression> operand, Op op)
        : operand(std::move(operand)), op(op) {}

    void accept(ASTVisitor& v) override;

    std::unique_ptr<Expression> operand;
    Op op;
};


class ArrayAccessExpression : public Expression
{
public:
    ArrayAccessExpression(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx)
        : array(std::move(arr)), index(std::move(idx)) {}

    void accept(ASTVisitor& v) override;

    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
};


class MemberAccessExpression : public Expression
{
public:
    MemberAccessExpression(std::unique_ptr<Expression> obj, std::string field)
        : object(std::move(obj)), memberName(std::move(field)) {}

    void accept(ASTVisitor& v) override;

    std::unique_ptr<Expression> object;
    std::string memberName;
};


class DereferenceExpression : public Expression
{
public:
    explicit DereferenceExpression(std::unique_ptr<Expression> ptr) : ptr(std::move(ptr)) {}

    void accept(ASTVisitor& v) override;

    std::unique_ptr<Expression> ptr;
};


class IdentifierExpression : public Expression
{
public:
    IdentifierExpression(std::string moduleName, std::string name)
        : name(std::move(name)), moduleName(std::move(moduleName)) {}

    void accept(ASTVisitor& v) override;

    std::string name;
    std::string moduleName;
};


class QualifiedNameNode : public Expression
{
public:
    using Type = std::variant<std::monostate, std::unique_ptr<IdentifierExpression>, std::unique_ptr<
                                  MemberAccessExpression>>;

    QualifiedNameNode(std::string first, std::string second)
        : first(std::move(first)), second(std::move(second)) {}

    void accept(ASTVisitor& v) override;

    std::string first, second;
    Type realisation;
};


class BooleanLiteral : public Expression
{
public:
    explicit BooleanLiteral(bool val) : value(val) {}

    void accept(ASTVisitor& v) override;

    bool value;
};


class IntegerLiteral : public Expression
{
public:
    explicit IntegerLiteral(int64_t val) : value(val) {}

    void accept(ASTVisitor& v) override;

    int64_t value;
};


class RealLiteral : public Expression
{
public:
    explicit RealLiteral(double val) : value(val) {}

    void accept(ASTVisitor& v) override;

    double value;
};


class StringLiteral : public Expression
{
public:
    explicit StringLiteral(std::string val) : value(std::move(val)) {}

    void accept(ASTVisitor& v) override;

    std::string value;
};


class SetLiteral : public Expression
{
public:
    explicit SetLiteral(uint64_t val) : value(val) {}

    void accept(ASTVisitor& v) override;

    uint64_t value;
};


class Nil : public Expression
{
public:
    void accept(ASTVisitor& v) override;
};
}
