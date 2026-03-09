#include "SemanticAnalyzer.h"
#include "TypeInfo.h"

namespace obould
{
SemanticAnalyzer::SemanticAnalyzer()
{
    createBuiltinTypes();
}

void SemanticAnalyzer::createBuiltinTypes()
{
    builtinTypes["i64"] = std::make_shared<TypeInfo>(TypeKind::i64);
    builtinTypes["f64"] = std::make_shared<TypeInfo>(TypeKind::f64);
    builtinTypes["bool"] = std::make_shared<TypeInfo>(TypeKind::Bool);
    builtinTypes["byte"] = std::make_shared<TypeInfo>(TypeKind::Byte);
    builtinTypes["char"] = std::make_shared<TypeInfo>(TypeKind::Char);
    builtinTypes["void"] = std::make_shared<TypeInfo>(TypeKind::Void);
}

void SemanticAnalyzer::addBuiltinTypes(SymbolTable& symTable)
{
    auto add = [&](const std::string& name)
    {
        Symbol s;
        s.name = name;
        s.kind = SymbolKind::Type;
        s.type = builtinTypes[name];
        s.isExported = false;
        s.isReference = false;
        s.isReadOnly = true;
        symTable.addSymbol(s);
    };

    for (auto& [k, v] : builtinTypes) {
        add(k);
    }
}

bool SemanticAnalyzer::analyze(Module& module)
{
    errors.clear();
    importedModule = false;
    currentModuleName = module.name;
    moduleRealNames[module.name] = module.name;
    module.accept(*this);
    return errors.empty();
}

void SemanticAnalyzer::setSymbolFileDir(const std::filesystem::path& dir)
{
    symbolFileParser.setSymbolDir(dir);
}

const std::vector<std::string>& SemanticAnalyzer::getErrors() const
{
    return errors;
}

void SemanticAnalyzer::addError(const std::string& error)
{
    errors.push_back(error);
}

std::shared_ptr<TypeInfo> SemanticAnalyzer::getPolymorphicBase(Expression* expr)
{
    auto type = expr->resolvedType;
    if (!type) return nullptr;

    if (type->kind == TypeKind::Pointer && type->baseType->kind == TypeKind::Struct) {
        return type->baseType;
    }

    if (type->kind == TypeKind::Struct) {
        if (auto* idExpr = dynamic_cast<IdentifierExpression*>(expr)) {
            auto* sym = symbolTables[currentModuleName].lookupSymbol(idExpr->name);
            if (sym && sym->isReference) {
                return type;
            }
        }
    }

    return nullptr;
}
}
