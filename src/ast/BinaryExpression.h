#pragma once
#include <memory>
#include "Expression.h"


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
