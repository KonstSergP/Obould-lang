#pragma once
#include <filesystem>
#include <memory>
#include "ast/ASTDeclarations.h"


namespace obould
{
class SymbolFileParser
{
public:
    SymbolFileParser();
    explicit SymbolFileParser(std::filesystem::path symbolDir);
    void setSymbolDir(std::filesystem::path symbolDir);
    std::unique_ptr<Module> parse(std::string moduleName);

private:
    std::filesystem::path symbolDir_;
    std::string findSymbolFile(const std::string& moduleName) const;
};
}
