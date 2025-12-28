#pragma once
#include <memory>
#include "Expression.h"
#include "Statement.h"


class DoWhileStatement : public Statement
{
public:
    DoWhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
        : condition(std::move(condition)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
};
