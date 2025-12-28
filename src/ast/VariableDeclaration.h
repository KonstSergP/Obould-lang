#pragma once
#include <memory>
#include <string>
#include "ASTNode.h"
#include "Type.h"


class VariableDeclaration : public ASTNode
{
public:
    VariableDeclaration(const std::string& name, bool isExported, std::unique_ptr<Type> type)
        : name(name),
          isExported(isExported),
          type(std::move(type)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::unique_ptr<Type> type;
};
