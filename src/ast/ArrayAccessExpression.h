#pragma once
#include <memory>
#include "Expression.h"


class ArrayAccessExpression : public Expression
{
public:
    ArrayAccessExpression(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx)
    : array(std::move(arr)), index(std::move(idx)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;
};
