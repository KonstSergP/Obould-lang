#pragma once
#include "Expression.h"


class Nil : public Expression
{
public:
    void accept(ASTVisitor& v) override;
};
