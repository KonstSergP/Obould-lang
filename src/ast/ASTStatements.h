#pragma once
#include <memory>
#include <string>
#include "ASTCore.h"
#include "WhileBranch.h"


class AssignmentStatement : public Statement
{
public:
    AssignmentStatement(std::unique_ptr<Expression> target, std::unique_ptr<Expression> value)
        : target(std::move(target)),
          value(std::move(value)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
};


class WhileStatement : public Statement
{
public:
    explicit WhileStatement(std::vector<std::unique_ptr<WhileBranch>> branches) : branches(std::move(branches)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<WhileBranch>> branches;
};


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


class IfStatement : public Statement
{
public:
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch,
                std::unique_ptr<Statement> else_branch)
        : condition(std::move(condition)),
          thenBranch(std::move(then_branch)),
          elseBranch(std::move(else_branch)) {}

    void accept(ASTVisitor& v) override;


    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};


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


class StatementsBlock : public Statement
{
public:
    explicit StatementsBlock(std::vector<std::unique_ptr<Statement>> statements)
        : statements(std::move(statements)) {}

    void accept(ASTVisitor& v) override;


    std::vector<std::unique_ptr<Statement>> statements;
};
