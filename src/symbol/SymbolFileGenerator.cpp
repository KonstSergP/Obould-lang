#include "SymbolFileGenerator.h"
#include <fstream>


namespace obould
{
using json = nlohmann::json;

SymbolFileGenerator::SymbolFileGenerator()
{
    json_ = json::object();
}

json SymbolFileGenerator::getSymbolFileJson() const
{
    return json_;
}

void SymbolFileGenerator::saveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << json_.dump(4) << std::endl;
    }
}

// Expressions

void SymbolFileGenerator::visit(IntegerLiteral& node) {}
void SymbolFileGenerator::visit(RealLiteral& node) {}
void SymbolFileGenerator::visit(StringLiteral& node) {}
void SymbolFileGenerator::visit(BooleanLiteral& node) {}
void SymbolFileGenerator::visit(Nil& node) {}
void SymbolFileGenerator::visit(BinaryExpression& node) {}
void SymbolFileGenerator::visit(UnaryExpression& node) {}
void SymbolFileGenerator::visit(IdentifierExpression& node) {}
void SymbolFileGenerator::visit(ArrayAccessExpression& node) {}
void SymbolFileGenerator::visit(MemberAccessExpression& node) {}
void SymbolFileGenerator::visit(QualifiedNameNode& node) {}
void SymbolFileGenerator::visit(DereferenceExpression& node) {}


// Statements

void SymbolFileGenerator::visit(StatementsBlock& node) {}
void SymbolFileGenerator::visit(AssignmentStatement& node) {}
void SymbolFileGenerator::visit(IfStatement& node) {}
void SymbolFileGenerator::visit(WhileStatement& node) {}
void SymbolFileGenerator::visit(WhileBranch& node) {}
void SymbolFileGenerator::visit(DoWhileStatement& node) {}
void SymbolFileGenerator::visit(ForStatement& node) {}
void SymbolFileGenerator::visit(SwitchStatement& node) {}
void SymbolFileGenerator::visit(SwitchCase& node) {}
void SymbolFileGenerator::visit(CaseLabel& node) {}
void SymbolFileGenerator::visit(ProcedureCall& node) {}

// Types

void SymbolFileGenerator::visit(IdentifierType& node)
{
    if (node.moduleName.empty())
        json_ = node.name;
    else
        json_ = node.moduleName + "." + node.name;
}

void SymbolFileGenerator::visit(ArrayType& node)
{
    json arrJson = json::object();
    arrJson["kind"] = "array";
    std::visit([&arrJson](const auto& v) {
            arrJson["length"] = v;
        }, *node.length->constantValue);
    node.elementType->accept(*this);
    arrJson["elementType"] = json_;
    json_ = arrJson;
}

void SymbolFileGenerator::visit(OpenArrayType& node)
{
    json arrJson = json::object();
    arrJson["kind"] = "open array";
    node.elementType->accept(*this);
    arrJson["elementType"] = json_;
    json_ = arrJson;
}

void SymbolFileGenerator::visit(StructType& node)
{
    json structJson = json::object();
    structJson["kind"] = "struct";
    if (node.baseType) {
        node.baseType->accept(*this);
        structJson["baseType"] = json_;
    }
    structJson["fields"] = json::array();
    for (auto& f : node.fields) {
        json fieldJson = json::object();
        fieldJson["name"] = f->name;
        f->type->accept(*this);
        fieldJson["type"] = json_;
        fieldJson["isExported"] = f->isExported;
        structJson["fields"].push_back(fieldJson);
    }
    json_ = structJson;
}

void SymbolFileGenerator::visit(PointerType& node)
{
    json ptrJson = json::object();
    ptrJson["kind"] = "open array";
    node.type->accept(*this);
    ptrJson["targetType"] = json_;
    json_ = ptrJson;
}

void SymbolFileGenerator::visit(ProcedureType& node)
{
    json procJson = json::object();
    procJson["parameters"] = json::array();
    for (auto& parameter : node.parameters) {
        parameter->accept(*this);
        procJson["parameters"].push_back(json_);
    }
    node.returnType->accept(*this);
    procJson["returnType"] = json_;
    json_ = procJson;
}


// Declarations

void SymbolFileGenerator::visit(ConstantDeclaration& node)
{
    json constJson = json::object();
    constJson["name"] = node.name;
    std::visit([&constJson](const auto& v) {
            constJson["value"] = v;
        }, *node.value->constantValue);
    json_ = constJson;
}

void SymbolFileGenerator::visit(TypeDeclaration& node)
{
    json typeJson = json::object();
    typeJson["name"] = node.name;
    node.type->accept(*this);
    typeJson["type"] = json_;
    json_ = typeJson;
}

void SymbolFileGenerator::visit(VariableDeclaration& node)
{
    json varJson = json::object();
    varJson["name"] = node.name;
    node.type->accept(*this);
    varJson["type"] = json_;
    json_ = varJson;
}

void SymbolFileGenerator::visit(ProcedureDeclaration& node)
{
    json procJson = json::object();
    procJson["name"] = node.name;
    procJson["parameters"] = json::array();
    for (auto& parameter : node.parameters) {
        parameter->accept(*this);
        procJson["parameters"].push_back(json_);
    }
    node.returnType->accept(*this);
    procJson["returnType"] = json_;
    json_ = procJson;
}

void SymbolFileGenerator::visit(ProcedureParameter& node)
{
    json paramJson = json::object();
    paramJson["name"] = node.name;
    paramJson["reference"] = node.isReference;
    node.type->accept(*this);
    paramJson["type"] = json_;
    json_ = paramJson;
}

void SymbolFileGenerator::visit(ConstantDeclarations& node)
{
    json cDecls = json::array();
    for (auto& c : node.constants) {
        if (!c->isExported) continue;
        c->accept(*this);
        cDecls.push_back(json_);
    }
    json_ = cDecls;
}

void SymbolFileGenerator::visit(TypeDeclarations& node)
{
    json typeDecls = json::array();
    for (auto& type : node.types) {
        if (!type->isExported) continue;
        type->accept(*this);
        typeDecls.push_back(json_);
    }
    json_ = typeDecls;
}

void SymbolFileGenerator::visit(VariableDeclarations& node)
{
    json varDecls = json::array();
    for (auto& var : node.variables) {
        if (!var->isExported) continue;
        var->accept(*this);
        varDecls.push_back(json_);
    }
    json_ = varDecls;
}

void SymbolFileGenerator::visit(DeclarationsBlock& node)
{
    json declBlockJson = json::object();
    node.constants->accept(*this);
    declBlockJson["constants"] = json_;
    node.types->accept(*this);
    declBlockJson["types"] = json_;
    node.variables->accept(*this);
    declBlockJson["variables"] = json_;
    json_ = declBlockJson;
}

void SymbolFileGenerator::visit(Import& node)
{
    json_ = node.realName;
}

void SymbolFileGenerator::visit(Module& node)
{
    json moduleJson = json::object();
    moduleJson["moduleName"] = node.name;

    moduleJson["imports"] = json::array();
    for (auto& import : node.imports) {
        import->accept(*this);
        moduleJson["imports"].push_back(json_);
    }

    node.declarations->accept(*this);
    moduleJson["declarations"] = json_;

    moduleJson["procedures"] = json::array();
    for (auto& proc : node.procedures) {
        if (!proc->isExported) continue;
        proc->accept(*this);
        moduleJson["procedures"].push_back(json_);
    }

    json_ = moduleJson;
}
}
