#pragma once
#include <memory>
#include <string>
#include <vector>
#include "DeclarationsBlock.h"
#include "Import.h"
#include "ProcedureDeclaration.h"


class Module : public ASTNode
{
public:
    Module(const std::string& name,
              std::vector<std::unique_ptr<Import>> imports,
              std::unique_ptr<DeclarationsBlock> declarations,
              std::vector<std::unique_ptr<ProcedureDeclaration>> procedures)
        : name(name),
          imports(std::move(imports)),
          declarations(std::move(declarations)),
          procedures(std::move(procedures)) {}

    void accept(ASTVisitor& v) override;


    std::string name;
    std::vector<std::unique_ptr<Import>> imports;
    std::unique_ptr<DeclarationsBlock> declarations;
    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
};
