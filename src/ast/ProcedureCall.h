#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Expression.h"
#include "Statement.h"


class ProcedureCall : public Statement, public Expression
{
public:
    ProcedureCall(std::string procedure_name, std::vector<std::unique_ptr<Expression>> args)
        : procedureName(std::move(procedure_name)),
          args(std::move(args)) {}

    void accept(ASTVisitor& v) override;


    std::string procedureName;
    std::vector<std::unique_ptr<Expression>> args;

    bool isTypeGuard = false;
};
