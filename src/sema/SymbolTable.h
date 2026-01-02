#pragma once
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

class TypeInfo;


struct Symbol
{
    std::string name;
    std::shared_ptr<TypeInfo> type;
};


class SymbolTable
{
public:
    void enterScope();
    void exitScope();
    bool addSymbol(Symbol symbol);
    Symbol* lookupSymbol(const std::string& name);

private:
    std::deque<std::unordered_map<std::string, Symbol>> scopes;
};
