#pragma once
#include <string>
#include "Type.h"


class IdentifierType : public Type
{
public:
    IdentifierType(const std::string& name, const std::string& moduleName)
        : name(name),
          moduleName(moduleName) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::string moduleName;
};
