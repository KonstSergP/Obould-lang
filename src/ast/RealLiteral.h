#pragma once
#include "Expression.h"


class RealLiteral : public Expression
{
public:
    explicit RealLiteral(double val) : value(val) {}

    void accept(ASTVisitor& v) override;


    double value;
};
