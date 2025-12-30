#include "ASTPrintVisitor.h"

#include <algorithm>


ASTPrintVisitor::ASTPrintVisitor(std::ostream& os) : os(os) {}

void ASTPrintVisitor::printNodeName(const std::string& name, const std::string& extraInfo) const
{
    os << BLUE << name << RESET;
    if (!extraInfo.empty()) {
        os << ": " << extraInfo;
    }
    os << std::endl;
}

static std::string binOpToString(const BinaryExpression::Op op)
{
    switch (op) {
    case BinaryExpression::Op::Add: return "+";
    case BinaryExpression::Op::Sub: return "-";
    case BinaryExpression::Op::Mul: return "*";
    case BinaryExpression::Op::FDiv: return "/";
    case BinaryExpression::Op::IDiv: return "div";
    case BinaryExpression::Op::Mod: return "mod";
    case BinaryExpression::Op::And: return "&&";
    case BinaryExpression::Op::Or: return "||";
    case BinaryExpression::Op::Eq: return "==";
    case BinaryExpression::Op::Neq: return "!=";
    case BinaryExpression::Op::Lt: return "<";
    case BinaryExpression::Op::Lte: return "<=";
    case BinaryExpression::Op::Gt: return ">";
    case BinaryExpression::Op::Gte: return ">=";
    case BinaryExpression::Op::Is: return "is";
    default: throw std::runtime_error("Unknown binary expression type");
    }
}

static std::string unOpToString(const UnaryExpression::Op op)
{
    switch (op) {
    case UnaryExpression::Op::Negate: return "-";
    case UnaryExpression::Op::Not: return "!";
    case UnaryExpression::Op::Plus: return "+";
    default: throw std::runtime_error("Unknown unary expression type");
    }
}


// Expressions

void ASTPrintVisitor::visit(IntegerLiteral& node)
{
    os << GREEN << node.value << RESET << " integer" << std::endl;
}

void ASTPrintVisitor::visit(RealLiteral& node)
{
    os << GREEN << node.value << RESET << " real" << std::endl;
}

void ASTPrintVisitor::visit(StringLiteral& node)
{
    os << GREEN << "\"" << node.value << "\"" << RESET << std::endl;
}

void ASTPrintVisitor::visit(BooleanLiteral& node)
{
    os << GREEN << (node.value ? "true" : "false") << RESET << std::endl;
}

void ASTPrintVisitor::visit(Nil& node)
{
    os << "nil" << std::endl;
}

void ASTPrintVisitor::visit(BinaryExpression& node)
{
    printNodeName("BinaryExpr", binOpToString(node.op));
    printChild(node.left, false);
    printChild(node.right, true);
}

void ASTPrintVisitor::visit(UnaryExpression& node)
{
    printNodeName("UnaryExpr", unOpToString(node.op));
    printChild(node.operand, true);
}

void ASTPrintVisitor::visit(IdentifierExpression& node)
{
    std::string full = node.moduleName.empty() ? node.name : node.moduleName + "." + node.name;
    os << YELLOW << full << RESET << std::endl;
}

void ASTPrintVisitor::visit(ArrayAccessExpression& node)
{
    printNodeName("ArrayAccess");
    printChild(node.array, false);
    printChild(node.index, true);
}

void ASTPrintVisitor::visit(MemberAccessExpression& node)
{
    printNodeName("FieldAccess", node.memberName);
    printChild(node.object, true);
}

void ASTPrintVisitor::visit(DereferenceExpression& node)
{
    printNodeName("Dereference");
    printChild(node.ptr, true);
}


// Statements

void ASTPrintVisitor::visit(StatementsBlock& node)
{
    if (node.statements.empty()) {
        os << GRAY << "(empty block)" << RESET << std::endl;
        return;
    }

    os << "Block" << std::endl;
    for (size_t i = 0; i < node.statements.size(); ++i) {
        printChild(node.statements[i], i == node.statements.size() - 1);
    }
}

void ASTPrintVisitor::visit(AssignmentStatement& node)
{
    printNodeName("Assign");
    printChild(node.target, false);
    printChild(node.value, true);
}

void ASTPrintVisitor::visit(IfStatement& node)
{
    printNodeName("If");
    printChild(node.condition, false);
    bool hasElse = (node.elseBranch != nullptr);

    os << indentPrefix << (hasElse ? "├── " : "└── ") << CYAN << "Then" << RESET << std::endl;

    std::string oldIndent = indentPrefix;
    indentPrefix += (hasElse ? "│   " : "    ");
    printChild(node.thenBranch, true);
    indentPrefix = oldIndent;

    if (hasElse) {
        os << indentPrefix << "└── " << CYAN << "Else" << RESET << std::endl;
        indentPrefix += "    ";
        printChild(node.elseBranch, true);
        indentPrefix = oldIndent;
    }
}

void ASTPrintVisitor::visit(WhileStatement& node)
{
    printNodeName("While");
    for (size_t i = 0; i < node.branches.size(); ++i) {
        printChild(node.branches[i], i == node.branches.size() - 1);
    }
}

void ASTPrintVisitor::visit(WhileBranch& node)
{
    os << "Branch" << std::endl;
    printChild(node.condition, false);
    printChild(node.body, true);
}

void ASTPrintVisitor::visit(DoWhileStatement& node)
{
    printNodeName("DoWhile");
    printChild(node.body, false);
    printChild(node.condition, true);
}

void ASTPrintVisitor::visit(ForStatement& node)
{
    printNodeName("For");
    printChild(node.rangeStart, false);
    printChild(node.rangeEnd, false);
    if (node.step) {
        printChild(node.step, false);
    }
    printChild(node.body, true);
}

void ASTPrintVisitor::visit(SwitchStatement& node)
{
    printNodeName("SwitchCase");
    printChild(node.selector, false);
    for (size_t i = 0; i < node.cases.size(); ++i) {
        printChild(node.cases[i], i == node.cases.size() - 1);
    }
}

void ASTPrintVisitor::visit(SwitchCase& node)
{
    printNodeName("Case Group");
    for (const auto& label : node.labels) {
        printChild(label, false);
    }
    printChild(node.body, true);
}

void ASTPrintVisitor::visit(CaseLabel& node)
{
    if (node.endValue) {
        os << CYAN << "Range" << RESET << std::endl;
        printChild(node.value, false);
        printChild(node.endValue, true);
    }
    else {
        os << CYAN << "Label" << RESET << std::endl;
        printChild(node.value, true);
    }
}

void ASTPrintVisitor::visit(ProcedureCall& node)
{
    printNodeName("Call");
    bool hasArgs = !node.args.empty();
    printChild(node.procedureName, !hasArgs);

    for (size_t i = 0; i < node.args.size(); ++i) {
        printChild(node.args[i], i == node.args.size() - 1);
    }
}


// Types

void ASTPrintVisitor::visit(IdentifierType& node)
{
    os << CYAN << (node.moduleName.empty() ? "" : node.moduleName + ".") << node.name << RESET << std::endl;
}

void ASTPrintVisitor::visit(ArrayType& node)
{
    printNodeName("Array");
    printChild(node.elementType, false);
    printChild(node.length, true);
}

void ASTPrintVisitor::visit(OpenArrayType& node)
{
    printNodeName("OpenArray");
    printChild(node.elementType, true);
}

void ASTPrintVisitor::visit(StructType& node)
{
    printNodeName("struct");
    if (node.baseType) {
        bool hasFields = !node.fields.empty();
        os << indentPrefix << (hasFields ? "├── " : "└── ") << CYAN << "Extends" << RESET << std::endl;
        std::string oldIndent = indentPrefix;
        indentPrefix += (hasFields ? "│   " : "    ");
        printChild(node.baseType, true);
        indentPrefix = oldIndent;
    }
    for (size_t i = 0; i < node.fields.size(); ++i) {
        printChild(node.fields[i], i == node.fields.size() - 1);
    }
}

void ASTPrintVisitor::visit(PointerType& node)
{
    printNodeName("Pointer to");
    printChild(node.type, true);
}

void ASTPrintVisitor::visit(ProcedureType& node)
{
    printNodeName("Procedure type");

    bool hasReturn = (node.returnType != nullptr);
    size_t count = node.parameters.size();

    for (size_t i = 0; i < count; ++i) {
        bool isLast = (i == count - 1) && !hasReturn;
        printChild(node.parameters[i], isLast);
    }

    if (hasReturn) {
        os << indentPrefix << "└── " << CYAN << "Returns" << RESET << std::endl;
        std::string oldIndent = indentPrefix;
        indentPrefix += "    ";
        printChild(node.returnType, true);
        indentPrefix = oldIndent;
    }
}


// Declarations

void ASTPrintVisitor::visit(ConstantDeclaration& node)
{
    std::string constant = (node.isExported ? "export " : "") + std::string("constant");
    printNodeName(constant, node.name);
    printChild(node.value, true);
}

void ASTPrintVisitor::visit(TypeDeclaration& node)
{
    std::string type = (node.isExported ? "export " : "") + std::string("type");
    printNodeName(type, node.name);
    printChild(node.type, true);
}

void ASTPrintVisitor::visit(VariableDeclaration& node)
{
    std::string variable = (node.isExported ? "export " : "") + std::string("variable");
    printNodeName(variable, node.name);
    printChild(node.type, true);
}

void ASTPrintVisitor::visit(ProcedureDeclaration& node)
{
    std::string proc = (node.isExported ? "export " : "") + std::string("fn");
    printNodeName(proc, node.name);

    if (!node.parameters.empty()) {
        os << indentPrefix << "├── " << GRAY << "Params" << RESET << std::endl;
        std::string saved = indentPrefix;
        indentPrefix += "│   ";
        for (int i = 0; i < node.parameters.size(); ++i) {
            printChild(node.parameters[i], i == node.parameters.size() - 1);
        }
        indentPrefix = saved;
    }

    if (node.returnType) {
        os << indentPrefix << "├── " << GRAY << "Returns" << RESET << std::endl;
        std::string saved = indentPrefix;
        indentPrefix += "│   ";
        printChild(node.returnType, true);
        indentPrefix = saved;
    }

    if (node.declarations) {
        printChild(node.declarations, false);
    }
    printChild(node.body, true);
}

void ASTPrintVisitor::visit(ProcedureParameter& node)
{
    os << (node.isReference ? "var " : " : ") << YELLOW << node.name << RESET << std::endl;
    printChild(node.type, true);
}

void ASTPrintVisitor::visit(ConstantDeclarations& node)
{
    for (int i = 0; i < node.constants.size(); ++i) {
        printChild(node.constants[i], i == node.constants.size() - 1);
    }
}

void ASTPrintVisitor::visit(TypeDeclarations& node)
{
    for (int i = 0; i < node.types.size(); ++i) {
        printChild(node.types[i], i == node.types.size() - 1);
    }
}

void ASTPrintVisitor::visit(VariableDeclarations& node)
{
    for (int i = 0; i < node.variables.size(); ++i) {
        printChild(node.variables[i], i == node.variables.size() - 1);
    }
}

void ASTPrintVisitor::visit(DeclarationsBlock& node)
{
    bool hasConstants = (node.constants != nullptr);
    bool hasTypes = (node.types != nullptr);
    bool hasVars = (node.variables != nullptr);

    auto printSection = [&](const std::string& title, auto& containerPtr, bool isLastSection)
    {
        os << indentPrefix << (isLastSection ? "└── " : "├── ") << CYAN << title << RESET << std::endl;

        std::string oldIndent = indentPrefix;
        indentPrefix += (isLastSection ? "    " : "│   ");
        if (containerPtr) {
            containerPtr->accept(*this);
        }
        indentPrefix = oldIndent;
    };

    if (hasConstants) {
        printSection("Constants", node.constants, !hasTypes && !hasVars);
    }
    if (hasTypes) {
        printSection("Types", node.types, !hasVars);
    }
    if (hasVars) {
        printSection("Variables", node.variables, true);
    }
}

void ASTPrintVisitor::visit(Import& node)
{
    os << YELLOW << node.realName << RESET;
    if (node.realName != node.localName) {
        os << " as " << node.localName;
    }
    os << std::endl;
}

void ASTPrintVisitor::visit(Module& node)
{
    os << indentPrefix << BLUE << "module " << node.name << RESET << std::endl;

    if (!node.imports.empty()) {
        os << indentPrefix << "├── " << GRAY << "Imports" << RESET << std::endl;
        std::string old = indentPrefix;
        indentPrefix += "│   ";
        for (size_t i = 0; i < node.imports.size(); ++i) {
            printChild(node.imports[i], i == node.imports.size() - 1);
        }
        indentPrefix = old;
    }

    if (node.declarations) {
        os << indentPrefix << "├── " << GRAY << "Declarations" << RESET << std::endl;
        std::string old = indentPrefix;
        indentPrefix += "│   ";
        node.declarations->accept(*this);
        indentPrefix = old;
    }

    if (!node.procedures.empty()) {
        os << "└── " << GRAY << "Procedures" << RESET << std::endl;
        indentPrefix += "    ";
        for (size_t i = 0; i < node.procedures.size(); ++i) {
            printChild(node.procedures[i], i == node.procedures.size() - 1);
        }
    }
}
