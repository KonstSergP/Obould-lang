#pragma once
#include <memory>
#include <string>
#include "ASTNode.h"
#include "Type.h"


class ProcedureParameter : public ASTNode
{
public:
    ProcedureParameter(const std::string& name, bool isReference, const std::shared_ptr<Type>& type)
        : name(name),
          isReference(isReference),
          type(type) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isReference;
    std::shared_ptr<Type> type;
};
