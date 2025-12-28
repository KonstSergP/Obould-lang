#pragma once
#include <memory>
#include <string>
#include "ASTNode.h"
#include "Expression.h"


class ConstantDeclaration : public ASTNode
{
public:
    ConstantDeclaration(const std::string& name, bool isExported, std::unique_ptr<Expression> value)
        : name(name),
          isExported(isExported),
          value(std::move(value)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::unique_ptr<Expression> value;
};
