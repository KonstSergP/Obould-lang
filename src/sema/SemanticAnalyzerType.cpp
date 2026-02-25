#include "SemanticAnalyzer.h"
#include "SymbolTable.h"
#include "TypeInfo.h"


namespace obould
{
void SemanticAnalyzer::visit(IdentifierType& node)
{
    Symbol* sym = nullptr;

    if (node.moduleName.empty()) {
        sym = symbolTables[currentTableName].lookupSymbol(node.name);
    }
    else {
        if (symbolTables.find(node.moduleName) == symbolTables.end()) {
            addError("Can't find module '" + node.moduleName + "'");
            node.resolvedType = std::make_shared<TypeInfo>();
            return;
        }
        sym = symbolTables[node.moduleName].lookupSymbol(node.name);
    }

    if (!sym) {
        addError("Unknown type: " + node.name);
        node.resolvedType = std::make_shared<TypeInfo>();
        return;
    }

    if (sym->kind != SymbolKind::Type) {
        addError("'" + node.name + "' is not a type");
        node.resolvedType = std::make_shared<TypeInfo>();
        return;
    }

    node.resolvedType = sym->type;
}

void SemanticAnalyzer::visit(ArrayType& node)
{
    node.length->accept(*this);

    int64_t lengthValue;

    auto val = node.length->constantValue;
    if (val.has_value() && std::holds_alternative<int64_t>(*val)) {
        lengthValue = std::get<int64_t>(*val);

        if (lengthValue <= 0) {
            addError("Array length must be positive");
            return;
        }
    }
    else {
        addError("Array length must be a constant integer expression");
        return;
    }

    node.elementType->accept(*this);

    if (!node.elementType->resolvedType) {
        node.resolvedType = std::make_shared<TypeInfo>(TypeKind::Void);
        return;
    }

    auto arrayInfo = std::make_shared<TypeInfo>(TypeKind::Array);
    arrayInfo->baseType = node.elementType->resolvedType;
    arrayInfo->length = lengthValue;
    arrayInfo->isOpenArray = false;

    node.resolvedType = arrayInfo;
}

void SemanticAnalyzer::visit(OpenArrayType& node)
{
    node.elementType->accept(*this);

    auto arrayInfo = std::make_shared<TypeInfo>(TypeKind::Array);
    arrayInfo->baseType = node.elementType->resolvedType;
    arrayInfo->isOpenArray = true;

    node.resolvedType = arrayInfo;
}

static bool findFieldDuplication(const std::string& name, const std::shared_ptr<TypeInfo>& structType)
{
    for (const auto& existing : structType->fields) {
        if (existing.name == name) {
            return true;
        }
    }

    if (structType->baseType) {
        return findFieldDuplication(name, structType->baseType);
    }
    return false;
}

void SemanticAnalyzer::visit(StructType& node)
{
    auto structInfo = std::make_shared<TypeInfo>(TypeKind::Struct);
    if (node.baseType) {
        node.baseType->accept(*this);
        structInfo->baseType = node.baseType->resolvedType;
    }

    for (const auto& varDecl : node.fields) {
        varDecl->type->accept(*this);

        FieldInfo field;
        field.name = varDecl->name;
        field.type = varDecl->type->resolvedType;
        field.isExported = varDecl->isExported;

        if (findFieldDuplication(field.name, structInfo)) {
            addError("Duplicate field name in record: " + field.name);
        }

        structInfo->fields.push_back(field);
    }

    node.resolvedType = structInfo;
}

void SemanticAnalyzer::visit(PointerType& node)
{
    node.type->accept(*this);

    auto ptrInfo = std::make_shared<TypeInfo>(TypeKind::Pointer);
    ptrInfo->baseType = node.type->resolvedType;
    node.resolvedType = ptrInfo;
}

void SemanticAnalyzer::visit(ProcedureType& node)
{
    auto procInfo = std::make_shared<TypeInfo>(TypeKind::Procedure);

    for (const auto& param : node.parameters) {
        param->type->accept(*this);

        ParamInfo pInfo;
        pInfo.name = param->name;
        pInfo.type = param->type->resolvedType;
        pInfo.isReference = param->isReference;

        procInfo->parameters.push_back(pInfo);
    }

    if (node.returnType) {
        node.returnType->accept(*this);
        procInfo->returnType = node.returnType->resolvedType;
    }
    else {
        procInfo->returnType = std::make_shared<TypeInfo>(TypeKind::Void);
    }
    node.resolvedType = procInfo;
}
}
