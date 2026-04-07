#include "SymbolFileParser.h"
#include "ast/ASTDeclarations.h"
#include "ast/ASTExpressions.h"
#include "ast/ASTStatements.h"
#include "ast/ASTTypes.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace obould
{
using json = nlohmann::json;

SymbolFileParser::SymbolFileParser(const std::vector<std::filesystem::path>& dirs) : searchDirs(dirs) {}

void SymbolFileParser::setSearchDirs(const std::vector<std::filesystem::path>& dirs) {
    searchDirs = dirs;
}

namespace
{
std::unique_ptr<Module> parseModule(const json& j);

std::unique_ptr<Expression> parseConstantValue(const json& j)
{
    if (j.is_number_integer())
        return std::make_unique<IntegerLiteral>(j.get<int64_t>());
    if (j.is_number_float())
        return std::make_unique<RealLiteral>(j.get<double>());
    if (j.is_boolean())
        return std::make_unique<BooleanLiteral>(j.get<bool>());
    if (j.is_string())
        return std::make_unique<StringLiteral>(j.get<std::string>());
    return std::make_unique<Nil>();
}

std::unique_ptr<Type> parseType(const json& j);

std::unique_ptr<ProcedureParameter> parseProcedureParameter(const json& j)
{
    std::string name = j["name"].get<std::string>();
    bool isRef = j.value("reference", false);
    auto type = parseType(j["type"]);
    return std::make_unique<ProcedureParameter>(name, isRef, std::shared_ptr<Type>(std::move(type)));
}

std::unique_ptr<Type> parseType(const json& j)
{
    if (j.is_string()) {
        std::string full = j.get<std::string>();
        std::string moduleName, name;
        auto dot = full.find('.');
        if (dot != std::string::npos) {
            moduleName = full.substr(0, dot);
            name = full.substr(dot + 1);
        } else {
            name = full;
        }
        return std::make_unique<IdentifierType>(moduleName, name);
    }
    if (j.contains("parameters") && j.contains("returnType")) {
        std::vector<std::unique_ptr<ProcedureParameter>> params;
        for (const auto& p : j["parameters"])
            params.push_back(parseProcedureParameter(p));
        auto ret = parseType(j["returnType"]);
        return std::make_unique<ProcedureType>(std::move(params), std::move(ret));
    }
    std::string kind = j.value("kind", "");
    if (kind == "array") {
        auto elem = parseType(j["elementType"]);
        int64_t len = j["length"].get<int64_t>();
        auto lenExpr = std::make_unique<IntegerLiteral>(len);
        return std::make_unique<ArrayType>(std::move(elem), std::move(lenExpr));
    }
    if (kind == "open array") {
        if (j.contains("targetType")) {
            auto target = parseType(j["targetType"]);
            return std::make_unique<PointerType>(std::move(target));
        }
        auto elem = parseType(j["elementType"]);
        return std::make_unique<OpenArrayType>(std::move(elem));
    }
    if (kind == "struct") {
        std::unique_ptr<IdentifierType> base;
        if (j.contains("baseType") && !j["baseType"].is_null()) {
            std::string full = j["baseType"].get<std::string>();
            std::string moduleName, name;
            auto dot = full.find('.');
            if (dot != std::string::npos) {
                moduleName = full.substr(0, dot);
                name = full.substr(dot + 1);
            } else {
                name = full;
            }
            base = std::make_unique<IdentifierType>(moduleName, name);
        }
        std::vector<std::unique_ptr<VariableDeclaration>> fields;
        for (const auto& f : j["fields"]) {
            std::string name = f["name"].get<std::string>();
            bool exported = f.value("isExported", false);
            auto type = parseType(f["type"]);
            fields.push_back(std::make_unique<VariableDeclaration>(name, exported, std::shared_ptr(std::move(type))));
        }
        return std::make_unique<StructType>(std::move(base), std::move(fields));
    }
    if (kind == "incomplete") {
        return std::make_unique<IncompleteType>();
    }
    return std::make_unique<IdentifierType>("", "unknown");
}

std::unique_ptr<ConstantDeclaration> parseConstant(const json& j)
{
    std::string name = j["name"].get<std::string>();
    auto value = parseConstantValue(j["value"]);
    return std::make_unique<ConstantDeclaration>(name, true, std::move(value));
}

std::unique_ptr<TypeDeclaration> parseTypeDeclaration(const json& j)
{
    std::string name = j["name"].get<std::string>();
    auto type = parseType(j["type"]);
    return std::make_unique<TypeDeclaration>(name, true, std::move(type));
}

std::unique_ptr<VariableDeclaration> parseVariableDeclaration(const json& j)
{
    std::string name = j["name"].get<std::string>();
    auto type = parseType(j["type"]);
    return std::make_unique<VariableDeclaration>(name, true, std::shared_ptr<Type>(std::move(type)));
}

std::unique_ptr<ConstantDeclarations> parseConstantDeclarations(const json& j)
{
    std::vector<std::unique_ptr<ConstantDeclaration>> constants;
    if (j.is_array())
        for (const auto& c : j)
            constants.push_back(parseConstant(c));
    return std::make_unique<ConstantDeclarations>(std::move(constants));
}

std::unique_ptr<TypeDeclarations> parseTypeDeclarations(const json& j)
{
    std::vector<std::unique_ptr<TypeDeclaration>> types;
    if (j.is_array())
        for (const auto& t : j)
            types.push_back(parseTypeDeclaration(t));
    return std::make_unique<TypeDeclarations>(std::move(types));
}

std::unique_ptr<VariableDeclarations> parseVariableDeclarations(const json& j)
{
    std::vector<std::unique_ptr<VariableDeclaration>> variables;
    if (j.is_array())
        for (const auto& v : j)
            variables.push_back(parseVariableDeclaration(v));
    return std::make_unique<VariableDeclarations>(std::move(variables));
}

std::unique_ptr<DeclarationsBlock> parseDeclarationsBlock(const json& j)
{
    auto constants = parseConstantDeclarations(j.value("constants", json::array()));
    auto types = parseTypeDeclarations(j.value("types", json::array()));
    auto variables = parseVariableDeclarations(j.value("variables", json::array()));
    return std::make_unique<DeclarationsBlock>(std::move(constants), std::move(types), std::move(variables));
}

std::unique_ptr<ProcedureDeclaration> parseProcedureDeclaration(const json& j)
{
    std::string name = j["name"].get<std::string>();
    std::vector<std::unique_ptr<ProcedureParameter>> parameters;
    for (const auto& p : j["parameters"])
        parameters.push_back(parseProcedureParameter(p));
    auto returnType = parseType(j["returnType"]);
    auto emptyDecls = std::make_unique<DeclarationsBlock>(
        std::make_unique<ConstantDeclarations>(std::vector<std::unique_ptr<ConstantDeclaration>>{}),
        std::make_unique<TypeDeclarations>(std::vector<std::unique_ptr<TypeDeclaration>>{}),
        std::make_unique<VariableDeclarations>(std::vector<std::unique_ptr<VariableDeclaration>>{}));
    auto emptyBody = std::make_unique<StatementsBlock>(std::vector<std::unique_ptr<Statement>>{});
    return std::make_unique<ProcedureDeclaration>(name, true, std::move(parameters), std::move(returnType),
                                                  std::move(emptyDecls), std::move(emptyBody), nullptr);
}

Properties parseProperties(const json& j)
{
    Properties properties;
    properties.needsGC = j.value("needsGC", false);
    return properties;
}

std::unique_ptr<Module> parseModule(const json& j);
}

std::string SymbolFileParser::findSymbolFile(const std::string& moduleName) const
{
    for (const auto& dir : searchDirs) {
        auto tryPath = dir / (moduleName + ".json");

        if (fs::exists(tryPath)) {
            return tryPath;
        }
    }

    std::string searchList;
    for (const auto& dir : searchDirs) {
        searchList += "  - " + dir.string() + "\n";
    }
    throw std::runtime_error("Critical Error: Symbol file not found for module: " + moduleName +
                             "\nSearched in:\n" + searchList);
}

std::unique_ptr<Module> SymbolFileParser::parse(const std::string& moduleName) const
{
    auto path = findSymbolFile(moduleName);
    if (path.empty())
        throw std::runtime_error("Symbol file not found for module: " + moduleName);

    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open symbol file: " + path);

    json j = json::parse(file);

    return parseModule(j);
}

namespace
{
std::unique_ptr<Module> parseModule(const json& j)
{
    std::string name = j["moduleName"].get<std::string>();
    std::vector<std::unique_ptr<Import>> imports;
    for (const auto& imp : j.value("imports", json::array())) {
        std::string realName = imp.get<std::string>();
        imports.push_back(std::make_unique<Import>("", realName));
    }
    auto declarations = parseDeclarationsBlock(j.value("declarations", json::object()));
    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
    for (const auto& proc : j.value("procedures", json::array()))
        procedures.push_back(parseProcedureDeclaration(proc));
    auto properties = parseProperties(j.value("properties", json::object()));
    return std::make_unique<Module>(name, std::move(imports), std::move(declarations), std::move(procedures), properties);
}
}

}
