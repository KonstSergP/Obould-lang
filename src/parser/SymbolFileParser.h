#pragma once
#include <memory>
#include "ast/ASTDeclarations.h"


namespace obould
{
class SymbolFileParser
{
public:
    SymbolFileParser();
    std::unique_ptr<Module> parse(std::string moduleName);
};
}
