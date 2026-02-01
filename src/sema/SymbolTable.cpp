#include "SymbolTable.h"

namespace obould
{
void SymbolTable::enterScope()
{
    scopes.push_back(std::unordered_map<std::string, Symbol>());
}


void SymbolTable::exitScope()
{
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}


bool SymbolTable::addSymbol(Symbol symbol)
{
    if (!scopes.empty()) {
        auto& currentScope = scopes.back();
        if (currentScope.find(symbol.name) != currentScope.end()) {
            return false;
        }
        currentScope[symbol.name] = std::move(symbol);
        return true;
    }
    return false;
}


Symbol* SymbolTable::lookupSymbol(const std::string& name)
{
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (auto found = it->find(name); found != it->end()) {
            return &(found->second);
        }
    }
    return nullptr;
}

Symbol* SymbolTable::lookupSymbolLocal(const std::string& name)
{
    if (!scopes.empty()) {
        auto& currentScope = scopes.back();
        if (currentScope.find(name) != currentScope.end()) {
            return &(currentScope.at(name));
        }
    }
    return nullptr;
}
}
