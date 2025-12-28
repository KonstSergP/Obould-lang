#pragma once
#include <memory>
#include <string>
#include "Expression.h"
#include "Statement.h"


class ForStatement : public Statement
{
public:
    ForStatement(const std::string& counterName, std::unique_ptr<Expression> rangeStart,
                 std::unique_ptr<Expression> rangeEnd, std::unique_ptr<Expression> step,
                 std::unique_ptr<Statement> body)
        : counterName(counterName),
          rangeStart(std::move(rangeStart)),
          rangeEnd(std::move(rangeEnd)),
          step(std::move(step)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override;


    std::string counterName;
    std::unique_ptr<Expression> rangeStart;
    std::unique_ptr<Expression> rangeEnd;
    std::unique_ptr<Expression> step;
    std::unique_ptr<Statement> body;
};
