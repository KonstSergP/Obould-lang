#include "CCodegenVisitor.h"
#include "sema/TypeInfo.h"
#include <algorithm>
#include <cctype>
#include <set>

namespace obould
{

CCodegenVisitor::CCodegenVisitor(std::ostream& os, COutputMode mode)
    : os_(os), mode_(mode) {}

void CCodegenVisitor::increaseIndent()
{
    indent_ += "    ";
}

void CCodegenVisitor::decreaseIndent()
{
    if (indent_.size() >= 4) {
        indent_.resize(indent_.size() - 4);
    }
}

void CCodegenVisitor::emitIndent()
{
    os_ << indent_;
}

std::string CCodegenVisitor::mapTypeName(const std::string& obouldType)
{
    if (obouldType == "i64") return "int64_t";
    if (obouldType == "f64") return "double";
    if (obouldType == "bool") return "bool";
    if (obouldType == "char") return "char";
    if (obouldType == "byte") return "uint8_t";
    if (obouldType == "void") return "void";
    return obouldType;
}

std::string CCodegenVisitor::generateGuardName(const std::string& moduleName)
{
    std::string guard = moduleName;
    std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);
    return guard + "_H";
}

std::string CCodegenVisitor::getTypeString(Type& type)
{
    std::stringstream ss;
    CCodegenVisitor tempVisitor(ss, mode_);
    tempVisitor.moduleName_ = moduleName_;
    tempVisitor.procedureRefParams_ = procedureRefParams_;
    type.accept(tempVisitor);
    return ss.str();
}

std::string CCodegenVisitor::getExpressionString(Expression& expr)
{
    std::stringstream ss;
    CCodegenVisitor tempVisitor(ss, mode_);
    tempVisitor.moduleName_ = moduleName_;
    tempVisitor.referenceParams_ = referenceParams_;
    tempVisitor.procedureRefParams_ = procedureRefParams_;
    expr.accept(tempVisitor);
    return ss.str();
}

void CCodegenVisitor::emitTypeWithName(Type& type, const std::string& name)
{
    // Handle array types specially
    if (auto* arrType = dynamic_cast<ArrayType*>(&type)) {
        emitTypeWithName(*arrType->elementType, name);
        os_ << "[";
        arrType->length->accept(*this);
        os_ << "]";
    } else if (auto* openArr = dynamic_cast<OpenArrayType*>(&type)) {
        emitTypeWithName(*openArr->elementType, name);
        os_ << "[]";
    } else {
        type.accept(*this);
        os_ << " " << name;
    }
}

// ============================================================================
// Expressions
// ============================================================================

void CCodegenVisitor::visit(IntegerLiteral& node)
{
    os_ << node.value;
}

void CCodegenVisitor::visit(RealLiteral& node)
{
    os_ << node.value;
}

void CCodegenVisitor::visit(BooleanLiteral& node)
{
    os_ << (node.value ? "true" : "false");
}

void CCodegenVisitor::visit(StringLiteral& node)
{
    os_ << "\"";
    for (char c : node.value) {
        switch (c) {
            case '\n': os_ << "\\n"; break;
            case '\t': os_ << "\\t"; break;
            case '\\': os_ << "\\\\"; break;
            case '"': os_ << "\\\""; break;
            default: os_ << c; break;
        }
    }
    os_ << "\"";
}

void CCodegenVisitor::visit(Nil& node)
{
    os_ << "NULL";
}

void CCodegenVisitor::visit(BinaryExpression& node)
{
    os_ << "(";
    node.left->accept(*this);

    switch (node.op) {
        case BinaryExpression::Op::Add: os_ << " + "; break;
        case BinaryExpression::Op::Sub: os_ << " - "; break;
        case BinaryExpression::Op::Mul: os_ << " * "; break;
        case BinaryExpression::Op::FDiv: os_ << " / "; break;
        case BinaryExpression::Op::IDiv: os_ << " / "; break;
        case BinaryExpression::Op::Mod: os_ << " % "; break;
        case BinaryExpression::Op::And: os_ << " && "; break;
        case BinaryExpression::Op::Or: os_ << " || "; break;
        case BinaryExpression::Op::Eq: os_ << " == "; break;
        case BinaryExpression::Op::Neq: os_ << " != "; break;
        case BinaryExpression::Op::Lt: os_ << " < "; break;
        case BinaryExpression::Op::Lte: os_ << " <= "; break;
        case BinaryExpression::Op::Gt: os_ << " > "; break;
        case BinaryExpression::Op::Gte: os_ << " >= "; break;
        case BinaryExpression::Op::Is:
            // Type check - needs runtime support, emit placeholder
            os_ << " /* is */ ";
            break;
    }

    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<Expression>>) {
            arg->accept(*this);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<Type>>) {
            // For 'is' operator - just emit type name as comment
            os_ << "/* ";
            arg->accept(*this);
            os_ << " */0";
        }
    }, node.right);

    os_ << ")";
}

void CCodegenVisitor::visit(UnaryExpression& node)
{
    switch (node.op) {
        case UnaryExpression::Op::Negate: os_ << "(-"; break;
        case UnaryExpression::Op::Not: os_ << "(!"; break;
        case UnaryExpression::Op::Plus: os_ << "(+"; break;
    }
    node.operand->accept(*this);
    os_ << ")";
}

void CCodegenVisitor::visit(IdentifierExpression& node)
{
    bool isRefParam = referenceParams_.count(node.name) > 0;

    if (isRefParam) {
        os_ << "(*";
    }

    if (!node.moduleName.empty()) {
        os_ << node.moduleName << "_";
    }
    os_ << node.name;

    if (isRefParam) {
        os_ << ")";
    }
}

void CCodegenVisitor::visit(ArrayAccessExpression& node)
{
    node.array->accept(*this);
    os_ << "[";
    node.index->accept(*this);
    os_ << "]";
}

void CCodegenVisitor::visit(MemberAccessExpression& node)
{
    node.object->accept(*this);
    os_ << "." << node.memberName;
}

void CCodegenVisitor::visit(DereferenceExpression& node)
{
    os_ << "(*";
    node.ptr->accept(*this);
    os_ << ")";
}

void CCodegenVisitor::visit(QualifiedNameNode& node)
{
    // Check if it's a module.identifier or object.field
    if (std::holds_alternative<std::unique_ptr<MemberAccessExpression>>(node.realisation)) {
        auto& member = std::get<std::unique_ptr<MemberAccessExpression>>(node.realisation);
        member->accept(*this);
    } else if (std::holds_alternative<std::unique_ptr<IdentifierExpression>>(node.realisation)) {
        auto& ident = std::get<std::unique_ptr<IdentifierExpression>>(node.realisation);
        ident->accept(*this);
    } else {
        // Not resolved - use heuristic:
        // Module names start with uppercase, variable names with lowercase
        if (!node.first.empty() && std::isupper(node.first[0])) {
            // Looks like Module.identifier
            os_ << node.first << "_" << node.second;
        } else {
            // Looks like object.field
            os_ << node.first << "." << node.second;
        }
    }
}

// ============================================================================
// Statements
// ============================================================================

void CCodegenVisitor::visit(ProcedureCall& node)
{
    // Determine procedure name for looking up reference parameters
    std::string procName;
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node.procedureName.get())) {
        if (ident->moduleName.empty()) {
            // Local function call - add current module prefix
            os_ << moduleName_ << "_" << ident->name;
            procName = ident->name;
        } else {
            // External function call
            os_ << ident->moduleName << "_" << ident->name;
            procName = ident->moduleName + "_" + ident->name;
        }
    } else if (auto* qn = dynamic_cast<QualifiedNameNode*>(node.procedureName.get())) {
        // Module.function call
        os_ << qn->first << "_" << qn->second;
        procName = qn->first + "_" + qn->second;
    } else {
        node.procedureName->accept(*this);
    }

    os_ << "(";

    // Look up reference parameter info for this procedure
    std::vector<bool> refParams;
    auto it = procedureRefParams_.find(procName);
    if (it != procedureRefParams_.end()) {
        refParams = it->second;
    }

    for (size_t i = 0; i < node.args.size(); ++i) {
        if (i > 0) os_ << ", ";

        // Check if this argument position is a reference parameter
        bool isRefArg = (i < refParams.size()) && refParams[i];

        if (isRefArg) {
            // Check if argument is a reference parameter in current scope
            // (if so, it's already a pointer - just pass it directly)
            if (auto* argIdent = dynamic_cast<IdentifierExpression*>(node.args[i].get())) {
                if (referenceParams_.count(argIdent->name) > 0) {
                    // Already a pointer, pass directly without dereferencing
                    if (!argIdent->moduleName.empty()) {
                        os_ << argIdent->moduleName << "_";
                    }
                    os_ << argIdent->name;
                    continue;
                }
            }
            // Not a reference parameter - take address
            os_ << "&(";
            node.args[i]->accept(*this);
            os_ << ")";
        } else {
            node.args[i]->accept(*this);
        }
    }
    os_ << ")";
}

void CCodegenVisitor::visit(StatementsBlock& node)
{
    for (auto& stmt : node.statements) {
        emitIndent();
        stmt->accept(*this);
        os_ << ";\n";
    }
}

void CCodegenVisitor::visit(AssignmentStatement& node)
{
    // Check for special char/string assignments
    auto* strLit = dynamic_cast<StringLiteral*>(node.value.get());
    if (strLit && node.target->resolvedType) {
        auto& targetType = node.target->resolvedType;

        // Single char assignment: ch = "a" -> ch = 'a'
        if (targetType->kind == TypeKind::Char && strLit->value.length() == 1) {
            node.target->accept(*this);
            os_ << " = '";
            char c = strLit->value[0];
            switch (c) {
                case '\n': os_ << "\\n"; break;
                case '\t': os_ << "\\t"; break;
                case '\\': os_ << "\\\\"; break;
                case '\'': os_ << "\\'"; break;
                default: os_ << c; break;
            }
            os_ << "'";
            return;
        }

        // Char array assignment: buf = "hello" -> strncpy(buf, "hello", sizeof(buf))
        if (targetType->kind == TypeKind::Array &&
            targetType->baseType &&
            targetType->baseType->kind == TypeKind::Char) {
            os_ << "strncpy(";
            node.target->accept(*this);
            os_ << ", ";
            node.value->accept(*this);
            os_ << ", sizeof(";
            node.target->accept(*this);
            os_ << "))";
            return;
        }
    }

    // Default assignment
    node.target->accept(*this);
    os_ << " = ";
    node.value->accept(*this);
}

void CCodegenVisitor::visit(IfStatement& node)
{
    os_ << "if (";
    node.condition->accept(*this);
    os_ << ") {\n";

    increaseIndent();
    if (node.thenBranch) {
        node.thenBranch->accept(*this);
    }
    decreaseIndent();

    emitIndent();
    os_ << "}";

    if (node.elseBranch) {
        // Check if else branch is another if statement
        if (auto* elseIf = dynamic_cast<IfStatement*>(node.elseBranch.get())) {
            os_ << " else ";
            elseIf->accept(*this);
        } else {
            os_ << " else {\n";
            increaseIndent();
            node.elseBranch->accept(*this);
            decreaseIndent();
            emitIndent();
            os_ << "}";
        }
    }
}

void CCodegenVisitor::visit(WhileStatement& node)
{
    // Obould while with elif branches - generate as nested if-else
    if (node.branches.size() == 1) {
        os_ << "while (";
        node.branches[0]->condition->accept(*this);
        os_ << ") {\n";
        increaseIndent();
        node.branches[0]->body->accept(*this);
        decreaseIndent();
        emitIndent();
        os_ << "}";
    } else {
        // Multiple branches - generate loop with if-else chain
        os_ << "while (1) {\n";
        increaseIndent();

        for (size_t i = 0; i < node.branches.size(); ++i) {
            emitIndent();
            if (i == 0) {
                os_ << "if (";
            } else {
                os_ << "} else if (";
            }
            node.branches[i]->condition->accept(*this);
            os_ << ") {\n";
            increaseIndent();
            node.branches[i]->body->accept(*this);
            decreaseIndent();
        }
        emitIndent();
        os_ << "} else {\n";
        increaseIndent();
        emitIndent();
        os_ << "break;\n";
        decreaseIndent();
        emitIndent();
        os_ << "}\n";

        decreaseIndent();
        emitIndent();
        os_ << "}";
    }
}

void CCodegenVisitor::visit(WhileBranch& node)
{
    // Handled by WhileStatement
}

void CCodegenVisitor::visit(DoWhileStatement& node)
{
    os_ << "do {\n";
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    emitIndent();
    os_ << "} while (";
    node.condition->accept(*this);
    os_ << ")";
}

void CCodegenVisitor::visit(ForStatement& node)
{
    os_ << "for (int64_t " << node.counterName << " = ";
    node.rangeStart->accept(*this);
    os_ << "; " << node.counterName << " <= ";
    node.rangeEnd->accept(*this);
    os_ << "; " << node.counterName << " += ";
    if (node.step) {
        node.step->accept(*this);
    } else {
        os_ << "1";
    }
    os_ << ") {\n";

    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();

    emitIndent();
    os_ << "}";
}

void CCodegenVisitor::visit(SwitchStatement& node)
{
    os_ << "switch (";
    node.selector->accept(*this);
    os_ << ") {\n";

    for (auto& caseNode : node.cases) {
        caseNode->accept(*this);
    }

    emitIndent();
    os_ << "}";
}

void CCodegenVisitor::visit(SwitchCase& node)
{
    for (auto& label : node.labels) {
        label->accept(*this);
    }
    increaseIndent();
    node.body->accept(*this);
    emitIndent();
    os_ << "break;\n";
    decreaseIndent();
}

void CCodegenVisitor::visit(CaseLabel& node)
{
    if (node.endValue) {
        // Range label - generate multiple case statements
        auto* startInt = dynamic_cast<IntegerLiteral*>(node.value.get());
        auto* endInt = dynamic_cast<IntegerLiteral*>(node.endValue.get());
        if (startInt && endInt) {
            for (int64_t i = startInt->value; i <= endInt->value; ++i) {
                emitIndent();
                os_ << "case " << i << ":\n";
            }
        }
    } else {
        emitIndent();
        os_ << "case ";
        node.value->accept(*this);
        os_ << ":\n";
    }
}

// ============================================================================
// Types
// ============================================================================

void CCodegenVisitor::visit(IdentifierType& node)
{
    // Check if it's a primitive type
    static const std::set<std::string> primitiveTypes = {
        "i64", "f64", "bool", "char", "byte", "void"
    };

    bool isPrimitive = primitiveTypes.count(node.name) > 0;
    std::string typeName = mapTypeName(node.name);

    if (!node.moduleName.empty()) {
        // Explicit module prefix
        os_ << node.moduleName << "_" << typeName;
    } else if (!isPrimitive && !moduleName_.empty()) {
        // User-defined type from current module - add module prefix
        os_ << moduleName_ << "_" << typeName;
    } else {
        // Primitive type or no module context
        os_ << typeName;
    }
}

void CCodegenVisitor::visit(ArrayType& node)
{
    // Array types are handled specially in emitTypeWithName
    node.elementType->accept(*this);
}

void CCodegenVisitor::visit(OpenArrayType& node)
{
    node.elementType->accept(*this);
    os_ << "*";
}

void CCodegenVisitor::visit(StructType& node)
{
    os_ << "struct {\n";
    increaseIndent();

    if (node.baseType) {
        emitIndent();
        node.baseType->accept(*this);
        os_ << " _base;\n";
    }

    for (auto& field : node.fields) {
        emitIndent();
        field->accept(*this);
    }

    decreaseIndent();
    emitIndent();
    os_ << "}";
}

void CCodegenVisitor::visit(PointerType& node)
{
    node.type->accept(*this);
    os_ << "*";
}

void CCodegenVisitor::visit(ProcedureType& node)
{
    node.returnType->accept(*this);
    os_ << " (*)(";
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        if (i > 0) os_ << ", ";
        node.parameters[i]->type->accept(*this);
        if (node.parameters[i]->isReference) {
            os_ << "*";
        }
    }
    os_ << ")";
}

// ============================================================================
// Declarations
// ============================================================================

void CCodegenVisitor::visit(ConstantDeclaration& node)
{
    os_ << "#define " << moduleName_ << "_" << node.name << " (";
    node.value->accept(*this);
    os_ << ")\n";
}

void CCodegenVisitor::visit(TypeDeclaration& node)
{
    std::string fullName = moduleName_ + "_" + node.name;
    currentTypedefName_ = fullName;

    os_ << "typedef ";

    if (auto* structType = dynamic_cast<StructType*>(node.type.get())) {
        // Output struct with tag name for self-referential types
        os_ << "struct " << fullName << " {\n";
        increaseIndent();

        if (structType->baseType) {
            emitIndent();
            structType->baseType->accept(*this);
            os_ << " _base;\n";
        }

        for (auto& field : structType->fields) {
            emitIndent();
            field->accept(*this);
        }

        decreaseIndent();
        emitIndent();
        os_ << "} " << fullName << ";\n";
    } else if (auto* procType = dynamic_cast<ProcedureType*>(node.type.get())) {
        // Procedure type: typedef ReturnType (*TypeName)(params);
        procType->returnType->accept(*this);
        os_ << " (*" << fullName << ")(";
        for (size_t i = 0; i < procType->parameters.size(); ++i) {
            if (i > 0) os_ << ", ";
            procType->parameters[i]->type->accept(*this);
            if (procType->parameters[i]->isReference) {
                os_ << "*";
            }
        }
        os_ << ");\n";
    } else {
        node.type->accept(*this);
        os_ << " " << fullName << ";\n";
    }

    currentTypedefName_.clear();
}

void CCodegenVisitor::visit(VariableDeclaration& node)
{
    // Check if it's an array type
    if (auto* arrType = dynamic_cast<ArrayType*>(node.type.get())) {
        arrType->elementType->accept(*this);
        os_ << " " << node.name;
        os_ << "[";
        arrType->length->accept(*this);
        os_ << "];\n";
    } else if (auto* openArr = dynamic_cast<OpenArrayType*>(node.type.get())) {
        openArr->elementType->accept(*this);
        os_ << "* " << node.name << ";\n";
    } else {
        node.type->accept(*this);
        os_ << " " << node.name << ";\n";
    }
}

void CCodegenVisitor::visit(ProcedureDeclaration& node)
{
    // Clear and populate reference parameters for this function
    referenceParams_.clear();
    for (auto& param : node.parameters) {
        if (param->isReference) {
            referenceParams_.insert(param->name);
        }
    }

    // Add static for non-exported functions in source mode
    if (mode_ == COutputMode::SOURCE && !node.isExported) {
        os_ << "static ";
    }

    // Return type
    if (node.returnType) {
        node.returnType->accept(*this);
    } else {
        os_ << "void";
    }

    os_ << " " << moduleName_ << "_" << node.name << "(";

    // Parameters
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        if (i > 0) os_ << ", ";
        node.parameters[i]->accept(*this);
    }

    os_ << ") {\n";
    increaseIndent();

    // Local declarations
    if (node.declarations) {
        if (node.declarations->variables) {
            for (auto& var : node.declarations->variables->variables) {
                emitIndent();
                var->accept(*this);
            }
        }
        if (node.declarations->constants) {
            for (auto& constant : node.declarations->constants->constants) {
                emitIndent();
                // Local constants as const variables
                os_ << "const ";
                constant->value->accept(*this);
                os_ << ";\n";
            }
        }
    }

    // Body
    if (node.body) {
        node.body->accept(*this);
    }

    // Return statement
    if (node.returnExpression) {
        emitIndent();
        os_ << "return ";
        node.returnExpression->accept(*this);
        os_ << ";\n";
    }

    decreaseIndent();
    os_ << "}\n\n";

    // Clear reference parameters after function body
    referenceParams_.clear();
}

void CCodegenVisitor::visit(ProcedureParameter& node)
{
    node.type->accept(*this);
    if (node.isReference) {
        os_ << "*";
    }
    os_ << " " << node.name;
}

void CCodegenVisitor::visit(ConstantDeclarations& node)
{
    for (auto& constant : node.constants) {
        constant->accept(*this);
    }
}

void CCodegenVisitor::visit(TypeDeclarations& node)
{
    for (auto& type : node.types) {
        type->accept(*this);
    }
}

void CCodegenVisitor::visit(VariableDeclarations& node)
{
    for (auto& var : node.variables) {
        var->accept(*this);
    }
}

void CCodegenVisitor::visit(DeclarationsBlock& node)
{
    if (node.constants) {
        node.constants->accept(*this);
    }
    if (node.types) {
        node.types->accept(*this);
    }
    if (node.variables) {
        node.variables->accept(*this);
    }
}

void CCodegenVisitor::visit(Import& node)
{
    os_ << "#include \"" << node.realName << ".h\"\n";
}

void CCodegenVisitor::visit(Module& node)
{
    moduleName_ = node.name;
    std::string guardName = generateGuardName(node.name);

    // Collect procedure reference parameter info for call site handling
    for (auto& proc : node.procedures) {
        std::string fullName = moduleName_ + "_" + proc->name;
        std::vector<bool> refParams;
        for (auto& param : proc->parameters) {
            refParams.push_back(param->isReference);
        }
        procedureRefParams_[fullName] = refParams;
        // Also store without prefix for local calls before prefix resolution
        procedureRefParams_[proc->name] = refParams;
    }

    if (mode_ == COutputMode::HEADER) {
        // =====================================================================
        // HEADER FILE (.h) - declarations only
        // =====================================================================

        // Include guard
        os_ << "#ifndef " << guardName << "\n";
        os_ << "#define " << guardName << "\n\n";

        // Standard includes
        os_ << "#include <stdint.h>\n";
        os_ << "#include <stdbool.h>\n";
        os_ << "#include <stddef.h>\n\n";

        // Module imports
        for (auto& import : node.imports) {
            import->accept(*this);
        }
        if (!node.imports.empty()) {
            os_ << "\n";
        }

        // Declarations
        if (node.declarations) {
            // Constants (macros go in header)
            if (node.declarations->constants) {
                os_ << "/* Constants */\n";
                node.declarations->constants->accept(*this);
                os_ << "\n";
            }

            // Types (all type definitions go in header)
            if (node.declarations->types) {
                os_ << "/* Types */\n";
                node.declarations->types->accept(*this);
                os_ << "\n";
            }

            // Global variables - extern declarations for exported, nothing for static
            if (node.declarations->variables) {
                bool hasExported = false;
                for (auto& var : node.declarations->variables->variables) {
                    if (var->isExported) {
                        hasExported = true;
                        break;
                    }
                }
                if (hasExported) {
                    os_ << "/* Exported global variables */\n";
                    for (auto& var : node.declarations->variables->variables) {
                        if (var->isExported) {
                            os_ << "extern ";
                            std::string varName = moduleName_ + "_" + var->name;
                            if (auto* arrType = dynamic_cast<ArrayType*>(var->type.get())) {
                                arrType->elementType->accept(*this);
                                os_ << " " << varName << "[";
                                arrType->length->accept(*this);
                                os_ << "];\n";
                            } else if (auto* openArr = dynamic_cast<OpenArrayType*>(var->type.get())) {
                                openArr->elementType->accept(*this);
                                os_ << "* " << varName << ";\n";
                            } else {
                                var->type->accept(*this);
                                os_ << " " << varName << ";\n";
                            }
                        }
                    }
                    os_ << "\n";
                }
            }
        }

        // Function prototypes (only exported functions in header)
        bool hasExportedProcs = false;
        for (auto& proc : node.procedures) {
            if (proc->isExported) {
                hasExportedProcs = true;
                break;
            }
        }
        if (hasExportedProcs) {
            os_ << "/* Function prototypes */\n";
            for (auto& proc : node.procedures) {
                if (proc->isExported) {
                    if (proc->returnType) {
                        proc->returnType->accept(*this);
                    } else {
                        os_ << "void";
                    }
                    os_ << " " << moduleName_ << "_" << proc->name << "(";
                    for (size_t i = 0; i < proc->parameters.size(); ++i) {
                        if (i > 0) os_ << ", ";
                        proc->parameters[i]->accept(*this);
                    }
                    os_ << ");\n";
                }
            }
            os_ << "\n";
        }

        // End include guard
        os_ << "#endif /* " << guardName << " */\n";

    } else {
        // =====================================================================
        // SOURCE FILE (.c) - implementations
        // =====================================================================

        // Include own header and string.h for strncpy
        os_ << "#include \"" << node.name << ".h\"\n";
        os_ << "#include <string.h>\n\n";

        // Global variable definitions
        if (node.declarations && node.declarations->variables) {
            os_ << "/* Global variables */\n";
            for (auto& var : node.declarations->variables->variables) {
                // static for non-exported, no storage class for exported (defined here)
                if (!var->isExported) {
                    os_ << "static ";
                }
                std::string varName = moduleName_ + "_" + var->name;
                if (auto* arrType = dynamic_cast<ArrayType*>(var->type.get())) {
                    arrType->elementType->accept(*this);
                    os_ << " " << varName << "[";
                    arrType->length->accept(*this);
                    os_ << "];\n";
                } else if (auto* openArr = dynamic_cast<OpenArrayType*>(var->type.get())) {
                    openArr->elementType->accept(*this);
                    os_ << "* " << varName << ";\n";
                } else {
                    var->type->accept(*this);
                    os_ << " " << varName << ";\n";
                }
            }
            os_ << "\n";
        }

        // Static function prototypes (forward declarations for non-exported)
        bool hasStaticProcs = false;
        for (auto& proc : node.procedures) {
            if (!proc->isExported) {
                hasStaticProcs = true;
                break;
            }
        }
        if (hasStaticProcs) {
            os_ << "/* Static function prototypes */\n";
            for (auto& proc : node.procedures) {
                if (!proc->isExported) {
                    os_ << "static ";
                    if (proc->returnType) {
                        proc->returnType->accept(*this);
                    } else {
                        os_ << "void";
                    }
                    os_ << " " << moduleName_ << "_" << proc->name << "(";
                    for (size_t i = 0; i < proc->parameters.size(); ++i) {
                        if (i > 0) os_ << ", ";
                        proc->parameters[i]->accept(*this);
                    }
                    os_ << ");\n";
                }
            }
            os_ << "\n";
        }

        // Function implementations
        os_ << "/* Function implementations */\n";
        for (auto& proc : node.procedures) {
            proc->accept(*this);
        }
    }
}

} // namespace obould
