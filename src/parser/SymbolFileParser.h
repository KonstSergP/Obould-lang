#pragma once
#include <filesystem>
#include <memory>
#include "ast/ASTDeclarations.h"


namespace obould
{
namespace fs = std::filesystem;

class SymbolFileParser
{
public:
    SymbolFileParser() = default;
    explicit SymbolFileParser(const std::vector<fs::path>& dirs);
    void setSearchDirs(const std::vector<fs::path>& dirs);
    std::unique_ptr<Module> parse(const std::string& moduleName) const;

private:
    [[nodiscard]] std::string findSymbolFile(const std::string& moduleName) const;

    std::vector<fs::path> searchDirs;
};
}
