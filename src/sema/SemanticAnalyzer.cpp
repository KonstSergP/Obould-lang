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

std::shared_ptr<TypeInfo> SemanticAnalyzer::getBuiltinType(TypeKind kind) const
{
    switch (kind) {
    case TypeKind::i64: return builtinTypes.at("i64");
    case TypeKind::f64: return builtinTypes.at("f64");
    case TypeKind::Bool: return builtinTypes.at("bool");
    case TypeKind::Byte: return builtinTypes.at("byte");
    case TypeKind::Char: return builtinTypes.at("char");
    case TypeKind::Void: return builtinTypes.at("void");
    default:
        return std::make_shared<TypeInfo>(kind);
    }
}

void SemanticAnalyzer::addBuiltinTypes(SymbolTable& symTable) const
{
    auto add = [&](const std::string& name)
    {
        Symbol s;
        s.name = name;
        s.kind = SymbolKind::Type;
        s.type = builtinTypes.at(name);
        s.isExported = false;
        s.isReference = false;
        s.isReadOnly = true;
        symTable.addSymbol(s);
    };

    for (auto& [k, v] : builtinTypes) {
        add(k);
    }
}

void SemanticAnalyzer::addBuiltinProcedures(SymbolTable& symTable) const
{
    auto registerBuiltin = [&](const std::string& name, BuiltinKind builtin, TypeKind retKind = TypeKind::Void)
    {
        auto type = std::make_shared<TypeInfo>(TypeKind::Procedure);
        type->builtin = builtin;
        type->returnType = getBuiltinType(retKind);

        Symbol sym;
        sym.kind = SymbolKind::Procedure;
        sym.name = name;
        sym.type = type;
        sym.isExported = true;
        sym.isReference = false;
        sym.isReadOnly = true;

        symTable.addSymbol(std::move(sym));
    };

    registerBuiltin("new", BuiltinKind::NEW);
    registerBuiltin("len", BuiltinKind::LEN, TypeKind::i64);
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

void SemanticAnalyzer::visitBuiltinProcedure(ProcedureCall& node)
{
    for (auto& arg : node.args) {
        arg->accept(*this);
    }

    switch (node.procedureName->resolvedType->builtin) {
    case BuiltinKind::NEW:
    {
        if (node.args.size() != 1) {
            addError("'new' expects 1 argument");
            return;
        }
        if (node.args[0]->resolvedType->kind != TypeKind::Pointer) {
            addError("'new' requires a pointer variable");
        }
        if (!node.args[0]->isLvalue) {
            addError("'new' requires a variable (l-value)");
        }
        break;
    }
    case BuiltinKind::LEN:
    {
        if (node.args.size() != 1) {
            addError("'len' expects 1 argument");
            return;
        }
        auto argType = node.args[0]->resolvedType;
        if (argType->kind != TypeKind::Array && argType->kind != TypeKind::String) {
            addError("'len' expects an array or string");
        }
        break;
    }
    default:
        addError("Unknown builtin kind");
    }
    node.resolvedType = node.procedureName->resolvedType->returnType;
    node.isLvalue = false;
}
}
