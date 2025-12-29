#pragma once
#include <memory>
#include <string>
#include "ASTCore.h"


class BinaryExpression : public Expression
{
public:
    enum class Op { Add, Sub, Mul, FDiv, IDiv, Mod, And, Or, Eq, Neq, Less, Greater, Leq, Geq, Is };

    BinaryExpression(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r, Op op)
        : left(std::move(l)), right(std::move(r)), op(op) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
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
    explicit IdentifierExpression(std::string name) : name(std::move(name)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::string moduleName;
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
    explicit StringLiteral(const std::string& val) : value(val) {}

    void accept(ASTVisitor& v) override;


    std::string value;
};


class Nil : public Expression
{
public:
    void accept(ASTVisitor& v) override;
};
