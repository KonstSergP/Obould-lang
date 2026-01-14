#pragma once
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include "ast/ASTCore.h"


enum class SymbolKind { Variable, Constant, Type, Procedure, Module };


struct Symbol
{
    SymbolKind kind;
    std::string name;
    std::shared_ptr<TypeInfo> type;
    bool isExported;
    bool isReference;
    bool isReadOnly;
    std::optional<ConstValue> value;
};


class SymbolTable
{
public:
    void enterScope();
    void exitScope();
    bool addSymbol(Symbol symbol);
    Symbol* lookupSymbol(const std::string& name);
    Symbol* lookupSymbolLocal(const std::string& name);

private:
    std::deque<std::unordered_map<std::string, Symbol>> scopes;
};
