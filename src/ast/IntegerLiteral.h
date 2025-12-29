#pragma once
#include <cstdint>
#include "Expression.h"


class IntegerLiteral : public Expression
{
public:
    explicit IntegerLiteral(int64_t val) : value(val) {}

    void accept(ASTVisitor& v) override;


    int64_t value;
};
