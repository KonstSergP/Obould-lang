#pragma once
#include <string>
#include "Expression.h"


class StringLiteral : public Expression
{
public:
    explicit StringLiteral(const std::string& val) : value(val) {}

    void accept(ASTVisitor& v) override;


    std::string value;
};
