#pragma once
#include <memory>
#include "Expression.h"


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
