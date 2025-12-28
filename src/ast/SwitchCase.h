#pragma once
#include "CaseLabel.h"
#include "StatementsBlock.h"


class SwitchCase : public ASTNode
{
public:
    SwitchCase(std::vector<CaseLabel> labels, std::unique_ptr<StatementsBlock> body)
        : labels(std::move(labels)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override;


    std::vector<CaseLabel> labels;
    std::unique_ptr<StatementsBlock> body;
};
