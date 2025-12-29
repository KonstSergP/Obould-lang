#pragma once
#include <memory>
#include "Expression.h"


class DereferenceExpression : public Expression
{
public:
    explicit DereferenceExpression(std::unique_ptr<Expression> ptr) : ptr(std::move(ptr)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> ptr;
};
