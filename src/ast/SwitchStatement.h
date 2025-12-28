#pragma once
#include <memory>
#include <vector>
#include "Expression.h"
#include "SwitchCase.h"


class SwitchStatement : public Statement
{
public:
    SwitchStatement(std::unique_ptr<Expression> selector, std::vector<std::unique_ptr<SwitchCase>> cases)
        : selector(std::move(selector)),
          cases(std::move(cases)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> selector;
    std::vector<std::unique_ptr<SwitchCase>> cases;
};
