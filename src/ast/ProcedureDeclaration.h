#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ASTNode.h"
#include "Expression.h"
#include "ProcedureParameter.h"
#include "StatementsBlock.h"
#include "Type.h"


class ProcedureDeclaration : public ASTNode
{
public:
    ProcedureDeclaration(const std::string& name, bool isExported,
                         std::vector<std::unique_ptr<ProcedureParameter>> parameters, std::unique_ptr<Type> returnType,
                         std::unique_ptr<StatementsBlock> body, std::unique_ptr<Expression> returnExpression)
        : name(name),
          isExported(isExported),
          parameters(std::move(parameters)),
          returnType(std::move(returnType)),
          body(std::move(body)),
          returnExpression(std::move(returnExpression)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    bool isExported;
    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    std::unique_ptr<Type> returnType;
    std::unique_ptr<StatementsBlock> body;
    std::unique_ptr<Expression> returnExpression;
};
