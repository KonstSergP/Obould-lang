#pragma once
#include <string>
#include "Expression.h"


class IdentifierExpression : public Expression
{
public:
    explicit IdentifierExpression(std::string name) : name(std::move(name)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::string moduleName;
};
