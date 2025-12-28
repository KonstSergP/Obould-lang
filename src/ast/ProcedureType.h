#pragma once
#include <memory>
#include <vector>
#include "ProcedureParameter.h"
#include "Type.h"


class ProcedureType : public Type
{
public:
    ProcedureType(std::vector<std::unique_ptr<ProcedureParameter>> parameters, std::unique_ptr<Type> returnType)
        : parameters(std::move(parameters)),
          returnType(std::move(returnType)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    std::unique_ptr<Type> returnType;
};
