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
    rootModule = &module;

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

static void updateStructDepth(const std::shared_ptr<TypeInfo>& type)
{
    if (auto base = type->baseType) {
        if (base->depth == -1) updateStructDepth(base);
        type->depth = base->depth + 1;
    }
    else type->depth = 0;
}

static bool containsRecursiveInternal(const std::shared_ptr<TypeInfo>& t,
                                      const TypeInfo* target,
                                      std::set<TypeInfo*>& visiting)
{
    if (!t) return false;
    if (!visiting.insert(t.get()).second) return false;
    if (t.get() == target) return true;

    switch (t->kind) {
    case TypeKind::Struct:
        if (t->baseType && containsRecursiveInternal(t->baseType, target, visiting)) return true;
        for (const auto& f : t->fields) {
            if (containsRecursiveInternal(f.type, target, visiting)) return true;
        }
        return false;
    case TypeKind::Array:
        return containsRecursiveInternal(t->baseType, target, visiting);
    default:
        return false;
    }
}

static bool containsRecursive(const std::shared_ptr<TypeInfo>& t,
                              const TypeInfo* target)
{
    std::set<TypeInfo*> visiting;
    return containsRecursiveInternal(t, target, visiting);
}

void SemanticAnalyzer::validateTypeInternal(const std::shared_ptr<TypeInfo>& type, std::set<TypeInfo*>& visiting)
{
    if (!type) return;
    if (!visiting.insert(type.get()).second) return;

    auto checkElementType = [&](const std::shared_ptr<TypeInfo>& elem,
                                const std::string& context,
                                bool allowOpenArray)
    {
        if (!elem || !isValidVariableType(elem->kind)) {
            addError(context + " has invalid element type");
            return;
        }
        if (!allowOpenArray && elem->isOpenArray) {
            addError(context + " cannot use open array as element type");
        }
    };

    if (type->kind == TypeKind::Pointer) {
        if (!type->baseType || type->baseType->kind != TypeKind::Struct) {
            addError("Pointer base type must be a Struct in '" + type->name + "'");
        }
        validateTypeInternal(type->baseType, visiting);
    }
    else if (type->kind == TypeKind::Struct) {
        if (type->baseType && type->baseType->kind != TypeKind::Struct) {
            addError("Struct base type must be a struct");
        }
        if (type->baseType && containsRecursive(type->baseType, type.get())) {
            addError("Struct " + type->name + " cannot be used in its base");
        }
        for (const auto& f : type->fields) {
            if (containsRecursive(f.type, type.get())) {
                addError("Struct " + type->name + " cannot have itself as a field");
            }
            checkElementType(f.type, "Field '" + f.name + "' in struct " + type->name, false);
            validateTypeInternal(f.type, visiting);
        }
        updateStructDepth(type);
    }
    else if (type->kind == TypeKind::Array) {
        if (containsRecursive(type->baseType, type.get())) {
            addError("Array " + type->name + " cannot use itself as element type");
        }
        checkElementType(type->baseType, "Array " + type->name, type->isOpenArray);
        validateTypeInternal(type->baseType, visiting);
    }
    else if (type->kind == TypeKind::Procedure) {
        if (type->returnType->kind == TypeKind::Array || type->returnType->kind == TypeKind::Struct) {
            addError("Procedure cannot return a structured type.");
        }
        else {
            validateTypeInternal(type->returnType, visiting);
        }
        for (const auto& param : type->parameters) {
            if (!param.type || !isValidVariableType(param.type->kind)) {
                addError("Parameter '" + param.name + "' of procedure '" + type->name + "' has invalid type");
                continue;
            }
            if (param.type->kind == TypeKind::Array && !param.type->isOpenArray) {
                addError("Parameter '" + param.name + "' of procedure '" + type->name + "' must be an open array");
            }
            validateTypeInternal(param.type, visiting);
        }
    }
    visiting.erase(type.get());
}

void SemanticAnalyzer::validateType(const std::shared_ptr<TypeInfo>& type)
{
    std::set<TypeInfo*> visiting;
    validateTypeInternal(type, visiting);
}
}
