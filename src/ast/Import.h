#pragma once
#include <string>

#include "ASTNode.h"


class Import : public ASTNode
{
public:
    Import(const std::string& localName, const std::string& realName)
        : localName(localName),
          realName(realName) {}

    void accept(ASTVisitor& v) override;


    std::string localName;
    std::string realName;
};
