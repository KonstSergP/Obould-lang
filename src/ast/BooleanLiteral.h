#pragma once
#include "Expression.h"


class BooleanLiteral : public Expression
{
public:
    explicit BooleanLiteral(bool val) : value(val) {}

    void accept(ASTVisitor& v) override;


    bool value;
};
