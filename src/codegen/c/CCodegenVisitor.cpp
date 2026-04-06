#include "CCodegenVisitor.h"
#include "sema/TypeInfo.h"
#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

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

std::string CCodegenVisitor::makePrefix(const std::string& moduleName)
{
    return "ob_" + std::to_string(moduleName.size()) + moduleName + "_";
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
    if (node.op == BinaryExpression::Op::Is) {
        os_ << "isInstanceOf(((StructDescriptor*)";
        suppressDeref_ = true;
        node.left->accept(*this);
        suppressDeref_ = false;
        os_ << "->_desc), &";

        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Type>>) {
                if (auto* identType = dynamic_cast<IdentifierType*>(arg.get())) {
                    if (!identType->moduleName.empty()) {
                        os_ << makePrefix(identType->moduleName) << identType->name;
                    } else if (!moduleName_.empty()) {
                        os_ << makePrefix(moduleName_) << identType->name;
                    } else {
                        os_ << identType->name;
                    }
                    os_ << "_desc";
                }
            }
        }, node.right);

        os_ << ")";
        return;
    }

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
            throw std::runtime_error("CCodegenVisitor: BinaryExpression::Op::Is reached in switch (should be handled above)");
    }

    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<Expression>>) {
            arg->accept(*this);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<Type>>) {
            throw std::runtime_error("CCodegenVisitor: non-Is binary operator with Type right-hand side");
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
    bool isGlobalVar = globalVariables_.count(node.name) > 0;
    bool isGlobalConst = globalConstants_.count(node.name) > 0;

    bool deref = isRefParam && !suppressDeref_;

    if (deref) {
        os_ << "(*";
    }

    if (!node.moduleName.empty()) {
        os_ << makePrefix(node.moduleName);
    } else if (isGlobalVar || isGlobalConst) {
        os_ << makePrefix(moduleName_);
    }
    os_ << node.name;

    if (deref) {
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

    // Use -> for pointer types (implicit dereference) and type guards
    bool useArrow = false;
    if (auto* call = dynamic_cast<ProcedureCall*>(node.object.get())) {
        if (call->isTypeGuard) {
            useArrow = true;
        }
    }
    if (node.object->resolvedType && node.object->resolvedType->kind == TypeKind::Pointer) {
        useArrow = true;
    }

    os_ << (useArrow ? "->" : ".") << node.memberName;
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
        throw std::runtime_error("CCodegenVisitor: unresolved QualifiedNameNode '" + node.first + "." + node.second + "'");
    }
}

// ============================================================================
// Statements
// ============================================================================

void CCodegenVisitor::visit(ProcedureCall& node)
{
    // Handle type guard: p(Point3D) -> ((RTTITest_Point3D*)p)
    if (node.isTypeGuard && !node.args.empty()) {
        os_ << "((";
        // Get the target type name
        if (auto* typeIdent = dynamic_cast<IdentifierExpression*>(node.args[0].get())) {
            if (!typeIdent->moduleName.empty()) {
                os_ << makePrefix(typeIdent->moduleName) << typeIdent->name;
            } else {
                os_ << makePrefix(moduleName_) << typeIdent->name;
            }
        } else {
            node.args[0]->accept(*this);
        }
        os_ << "*)";
        suppressDeref_ = true;
        node.procedureName->accept(*this);
        suppressDeref_ = false;
        os_ << ")";
        return;
    }

    if (node.procedureName->resolvedType &&
        node.procedureName->resolvedType->builtin != BuiltinKind::None) {
        auto builtin = node.procedureName->resolvedType->builtin;
        switch (builtin) {
        case BuiltinKind::LEN:
        {
            if (node.args.empty()) break;
            auto& arg = node.args[0];
            auto argType = arg->resolvedType;
            
            if (argType && argType->isOpenArray) {
                Expression* cur = arg.get();
                int depth = 0;
                while (auto* acc = dynamic_cast<ArrayAccessExpression*>(cur)) {
                    depth++;
                    cur = acc->array.get();
                }
                if (auto* ident = dynamic_cast<IdentifierExpression*>(cur)) {
                    auto oaIt = openArrayParams_.find(ident->name);
                    if (oaIt != openArrayParams_.end() && depth < (int)oaIt->second.size()) {
                        os_ << oaIt->second[depth];
                        return;
                    }
                }
            }
            
            if (argType && (argType->kind == TypeKind::Array || argType->kind == TypeKind::String)) {
                if (argType->length > 0) {
                    os_ << argType->length;
                } else {
                    std::string argStr = getExpressionString(*arg);
                    os_ << "(sizeof(" << argStr << ") / sizeof(" << argStr << "[0]))";
                }
                return;
            }
            
            // os_ << "0";
            throw std::runtime_error("CCodegenVisitor: BuiltinKind::LEN reached in switch (should be handled above)");
            return;
        }
        case BuiltinKind::NEW:
        {
            if (node.args.empty()) break;

            auto& arg = node.args[0];
            auto argType = arg->resolvedType;
            if (argType && argType->kind == TypeKind::Pointer && argType->baseType) {
                std::string argStr = getExpressionString(*arg);
                auto& baseType = argType->baseType;
                std::string typeName;
                if (baseType->kind == TypeKind::Struct) {
                    std::string owner = baseType->ownerModule.empty() ? moduleName_ : baseType->ownerModule;
                    typeName = makePrefix(owner) + baseType->name;
                } else {
                    typeName = "void";
                }
                os_ << argStr << " = (" << typeName << "*)calloc(1, sizeof(" << typeName << "))";
                return;
            }
            break;
        }
        case BuiltinKind::ASSERT:
        {
            if (node.args.empty()) break;
            if (node.args.size() == 2) {
                os_ << "assert((";
                node.args[0]->accept(*this);
                os_ << ") && ";
                node.args[1]->accept(*this);
                os_ << ")";
            } else {
                os_ << "assert(";
                node.args[0]->accept(*this);
                os_ << ")";
            }
            return;
        }
        default:
            break;
        }
    }

    // Determine procedure name for looking up reference parameters
    std::string procName;
    if (auto* ident = dynamic_cast<IdentifierExpression*>(node.procedureName.get())) {
        if (ident->moduleName.empty()) {
            // Local function call - add current module prefix
            os_ << makePrefix(moduleName_) << ident->name;
            procName = ident->name;
        } else {
            // External function call
            os_ << makePrefix(ident->moduleName) << ident->name;
            procName = makePrefix(ident->moduleName) + ident->name;
        }
    } else if (auto* qn = dynamic_cast<QualifiedNameNode*>(node.procedureName.get())) {
        // Module.function call
        os_ << makePrefix(qn->first) << qn->second;
        procName = makePrefix(qn->first) + qn->second;
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

        bool isRefArg = (i < refParams.size()) && refParams[i];

        if (isRefArg) {
            // Check if argument is a reference parameter in current scope
            if (auto* argIdent = dynamic_cast<IdentifierExpression*>(node.args[i].get())) {
                if (referenceParams_.count(argIdent->name) > 0) {
                    // Already a pointer, pass directly without dereferencing
                    if (!argIdent->moduleName.empty()) {
                        os_ << makePrefix(argIdent->moduleName);
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

    // Append hidden length args for open array parameters
    auto oaCallIt = procedureOpenArrayParams_.find(procName);
    if (oaCallIt != procedureOpenArrayParams_.end()) {
        std::map<int, int> dimCount;
        for (auto& [idx, name] : oaCallIt->second) {
            os_ << ", ";
            int dim = dimCount[idx]++;
            if (idx < (int)node.args.size()) {
                auto* argIdent = dynamic_cast<IdentifierExpression*>(node.args[idx].get());
                auto oaIt2 = argIdent ? openArrayParams_.find(argIdent->name) : openArrayParams_.end();

                if (oaIt2 != openArrayParams_.end() && dim < (int)oaIt2->second.size()) {
                    os_ << oaIt2->second[dim];
                } else {
                    std::string argStr = getExpressionString(*node.args[idx]);
                    std::string subscripts;

                    for (int d = 0; d < dim; d++) subscripts += "[0]";

                    os_ << "(sizeof(" << argStr << subscripts
                        << ") / sizeof(" << argStr << subscripts << "[0]))";
                }
            } else {
                os_ << "0";
            }
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
    auto* strLit = dynamic_cast<StringLiteral*>(node.value.get());
    if (strLit && node.target->resolvedType) {
        auto& targetType = node.target->resolvedType;

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
    throw std::runtime_error("CCodegenVisitor: WhileBranch::visit called directly (should be handled by WhileStatement)");
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
    os_ << "for (" << node.counterName << " = ";
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
    static const std::set<std::string> primitiveTypes = {
        "i64", "f64", "bool", "char", "byte", "void"
    };

    bool isPrimitive = primitiveTypes.count(node.name) > 0;
    std::string typeName = mapTypeName(node.name);

    if (!node.moduleName.empty()) {
        // Explicit module prefix
        os_ << makePrefix(node.moduleName) << typeName;
    } else if (!isPrimitive && !moduleName_.empty()) {
        // User-defined type from current module - add module prefix
        os_ << makePrefix(moduleName_) << typeName;
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
    os_ << "#define " << makePrefix(moduleName_) << node.name << " (";
    node.value->accept(*this);
    os_ << ")\n";
}

void CCodegenVisitor::visit(TypeDeclaration& node)
{
    std::string fullName = makePrefix(moduleName_) + node.name;
    currentTypedefName_ = fullName;

    os_ << "typedef ";

    if (auto* structType = dynamic_cast<StructType*>(node.type.get())) {
        // Track struct for RTTI
        std::string parentName;
        if (structType->baseType) {
            if (!structType->baseType->moduleName.empty()) {
                parentName = makePrefix(structType->baseType->moduleName) + structType->baseType->name;
            } else {
                parentName = makePrefix(moduleName_) + structType->baseType->name;
            }
        }
        structTypes_.push_back({fullName, parentName});

        std::vector<std::pair<std::string, std::string>> allFields;

        os_ << "struct " << fullName << " {\n";
        increaseIndent();

        emitIndent();
        os_ << "StructDescriptor* _desc;\n";

        // Copy inherited fields from parent
        if (!parentName.empty()) {
            auto it = structFields_.find(parentName);
            if (it != structFields_.end()) {
                for (const auto& field : it->second) {
                    emitIndent();
                    os_ << field.second << " " << field.first << ";\n";
                    allFields.push_back(field);
                }
            }
        }

        // Add own fields
        for (auto& field : structType->fields) {
            std::string fieldType = getTypeString(*field->type);
            allFields.push_back({field->name, fieldType});

            emitIndent();
            if (auto* arrType = dynamic_cast<ArrayType*>(field->type.get())) {
                std::vector<Expression*> dimensions;
                Type* currentType = field->type.get();

                while (auto* arr = dynamic_cast<ArrayType*>(currentType)) {
                    dimensions.push_back(arr->length.get());
                    currentType = arr->elementType.get();
                }

                currentType->accept(*this);
                os_ << " " << field->name;
                for (auto* dim : dimensions) {
                    os_ << "[";
                    dim->accept(*this);
                    os_ << "]";
                }
                os_ << ";\n";
            } else {
                field->type->accept(*this);
                os_ << " " << field->name << ";\n";
            }
        }

        // Store all fields for future children
        structFields_[fullName] = allFields;

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
    if (auto* arrType = dynamic_cast<ArrayType*>(node.type.get())) {
        std::vector<Expression*> dimensions;
        Type* currentType = node.type.get();

        while (auto* arr = dynamic_cast<ArrayType*>(currentType)) {
            dimensions.push_back(arr->length.get());
            currentType = arr->elementType.get();
        }

        currentType->accept(*this);
        os_ << " " << node.name;

        for (auto* dim : dimensions) {
            os_ << "[";
            dim->accept(*this);
            os_ << "]";
        }
        os_ << ";\n";
    } else if (auto* openArr = dynamic_cast<OpenArrayType*>(node.type.get())) {
        openArr->elementType->accept(*this);
        os_ << "* " << node.name << ";\n";
    } else {
        node.type->accept(*this);
        os_ << " " << node.name << ";\n";

        // Initialize _desc for struct variables (RTTI support)
        if (auto* identType = dynamic_cast<IdentifierType*>(node.type.get())) {
            std::string fullTypeName;
            if (!identType->moduleName.empty()) {
                fullTypeName = makePrefix(identType->moduleName) + identType->name;
            } else if (!moduleName_.empty()) {
                fullTypeName = makePrefix(moduleName_) + identType->name;
            }
            for (const auto& st : structTypes_) {
                if (st.first == fullTypeName) {
                    emitIndent();
                    os_ << node.name << "._desc = &" << fullTypeName << "_desc;\n";
                    break;
                }
            }
        }
    }
}

void CCodegenVisitor::visit(ProcedureDeclaration& node)
{
    // Clear and populate reference parameters for this function
    referenceParams_.clear();
    openArrayParams_.clear();
    for (auto& param : node.parameters) {
        if (param->isReference) {
            referenceParams_.insert(param->name);
        }
        auto* oaType = dynamic_cast<OpenArrayType*>(param->type.get());
        if (oaType) {
            std::vector<std::string> lenNames;
            lenNames.push_back(param->name + "_len");
            int dim = 1;
            auto* inner = dynamic_cast<OpenArrayType*>(oaType->elementType.get());
            while (inner) {
                dim++;
                lenNames.push_back(param->name + "_len" + std::to_string(dim));
                inner = dynamic_cast<OpenArrayType*>(inner->elementType.get());
            }
            openArrayParams_[param->name] = lenNames;
        }
    }

    if (mode_ == COutputMode::SOURCE && !node.isExported) {
        os_ << "static ";
    }

    // Return type
    if (node.returnType) {
        node.returnType->accept(*this);
    } else {
        os_ << "void";
    }

    std::string procFullName = makePrefix(moduleName_) + node.name;
    os_ << " " << procFullName << "(";

    // Parameters
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        if (i > 0) os_ << ", ";
        node.parameters[i]->accept(*this);
    }
    // Hidden length params for open arrays (appended at end)
    auto oaIt = procedureOpenArrayParams_.find(procFullName);
    if (oaIt != procedureOpenArrayParams_.end()) {
        for (auto& [idx, name] : oaIt->second) {
            os_ << ", int64_t " << name;
        }
    }

    os_ << ") {\n";
    increaseIndent();

    if (mode_ == COutputMode::SOURCE && !structTypes_.empty()) {
        emitIndent();
        os_ << "_init_descriptors();\n";
    }

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

    // Collect global variable and constant names for prefix handling
    globalVariables_.clear();
    globalConstants_.clear();
    if (node.declarations) {
        if (node.declarations->variables) {
            for (auto& var : node.declarations->variables->variables) {
                globalVariables_.insert(var->name);
            }
        }
        if (node.declarations->constants) {
            for (auto& constant : node.declarations->constants->constants) {
                globalConstants_.insert(constant->name);
            }
        }
    }

    // Collect procedure parameter info for call site handling
    auto collectProcParams = [this](const std::string& prefix,
                                     const std::vector<std::unique_ptr<ProcedureDeclaration>>& procs,
                                     bool storeShortName) {
        for (auto& proc : procs) {
            std::string fullName = prefix + proc->name;
            std::vector<bool> refParams;
            std::vector<std::pair<int, std::string>> openArrayParams;
            for (size_t i = 0; i < proc->parameters.size(); ++i) {
                refParams.push_back(proc->parameters[i]->isReference);
                
                auto* oaType = dynamic_cast<OpenArrayType*>(proc->parameters[i]->type.get());
                if (oaType) {
                    std::string baseName = proc->parameters[i]->name;
                    int dim = 1;
                    openArrayParams.push_back({(int)i, baseName + "_len"});
                    auto* inner = dynamic_cast<OpenArrayType*>(oaType->elementType.get());
                    while (inner) {
                        dim++;
                        openArrayParams.push_back({(int)i, baseName + "_len" + std::to_string(dim)});
                        inner = dynamic_cast<OpenArrayType*>(inner->elementType.get());
                    }
                }
            }
            procedureRefParams_[fullName] = refParams;
            if (!openArrayParams.empty()) {
                procedureOpenArrayParams_[fullName] = openArrayParams;
            }
            if (storeShortName) {
                procedureRefParams_[proc->name] = refParams;
                if (!openArrayParams.empty()) {
                    procedureOpenArrayParams_[proc->name] = openArrayParams;
                }
            }
        }
    };

    // Local procedures
    collectProcParams(makePrefix(moduleName_), node.procedures, true);

    // Imported modules
    for (auto& import : node.imports) {
        if (import->module) {
            std::string importPrefix = makePrefix(import->module->name);
            collectProcParams(importPrefix, import->module->procedures, false);
        }
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

        // RTTI support - StructDescriptor
        os_ << "/* RTTI Support */\n";
        os_ << "#ifndef STRUCT_DESCRIPTOR_DEFINED\n";
        os_ << "#define STRUCT_DESCRIPTOR_DEFINED\n";
        os_ << "typedef struct StructDescriptor {\n";
        os_ << "    const char* name;\n";
        os_ << "    int level;\n";
        os_ << "    struct StructDescriptor** ancestors;\n";
        os_ << "} StructDescriptor;\n";
        os_ << "#endif\n\n";

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
            // Clear struct tracking before visiting types
            structTypes_.clear();
            structFields_.clear();
            if (node.declarations->types) {
                os_ << "/* Types */\n";
                node.declarations->types->accept(*this);
                os_ << "\n";
            }

            // Struct descriptor extern declarations
            if (!structTypes_.empty()) {
                os_ << "/* Struct descriptors */\n";
                for (const auto& st : structTypes_) {
                    os_ << "extern StructDescriptor " << st.first << "_desc;\n";
                }
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
                            std::string varName = makePrefix(moduleName_) + var->name;
                            if (auto* arrType = dynamic_cast<ArrayType*>(var->type.get())) {
                                std::vector<Expression*> dimensions;
                                Type* currentType = var->type.get();
                                while (auto* arr = dynamic_cast<ArrayType*>(currentType)) {
                                    dimensions.push_back(arr->length.get());
                                    currentType = arr->elementType.get();
                                }
                                currentType->accept(*this);
                                os_ << " " << varName;
                                for (auto* dim : dimensions) {
                                    os_ << "[";
                                    dim->accept(*this);
                                    os_ << "]";
                                }
                                os_ << ";\n";
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
                    std::string pFullName = makePrefix(moduleName_) + proc->name;
                    os_ << " " << pFullName << "(";
                    for (size_t i = 0; i < proc->parameters.size(); ++i) {
                        if (i > 0) os_ << ", ";
                        proc->parameters[i]->accept(*this);
                    }
                    auto oaIt2 = procedureOpenArrayParams_.find(pFullName);
                    if (oaIt2 != procedureOpenArrayParams_.end()) {
                        for (auto& [idx, name] : oaIt2->second) {
                            os_ << ", int64_t " << name;
                        }
                    }
                    os_ << ");\n";
                }
            }
            os_ << "\n";
        }

        os_ << "#endif /* " << guardName << " */\n";

    } else {
        // =====================================================================
        // SOURCE FILE (.c) - implementations
        // =====================================================================

        // Collect struct types for RTTI
        structTypes_.clear();
        structFields_.clear();
        if (node.declarations && node.declarations->types) {
            for (auto& typeDecl : node.declarations->types->types) {
                if (auto* structType = dynamic_cast<StructType*>(typeDecl->type.get())) {
                    std::string fullName = makePrefix(moduleName_) + typeDecl->name;
                    std::string parentName;
                    if (structType->baseType) {
                        if (!structType->baseType->moduleName.empty()) {
                            parentName = makePrefix(structType->baseType->moduleName) + structType->baseType->name;
                        } else {
                            parentName = makePrefix(moduleName_) + structType->baseType->name;
                        }
                    }
                    structTypes_.push_back({fullName, parentName});
                }
            }
        }

        // Include own header and required libraries
        os_ << "#include \"" << node.name << ".h\"\n";
        os_ << "#include <string.h>\n";
        os_ << "#include <stdlib.h>\n";
        os_ << "#include <assert.h>\n\n";

        // RTTI helper functions
        if (!structTypes_.empty()) {
            os_ << "/* RTTI helper functions */\n";
            os_ << "#ifndef RTTI_HELPERS_DEFINED\n";
            os_ << "#define RTTI_HELPERS_DEFINED\n";
            os_ << "static StructDescriptor* createStructDescriptor(\n";
            os_ << "    const char* name,\n";
            os_ << "    StructDescriptor* parent\n";
            os_ << ") {\n";
            os_ << "    StructDescriptor* desc = (StructDescriptor*)malloc(sizeof(StructDescriptor));\n";
            os_ << "    desc->name = name;\n";
            os_ << "    if (parent == NULL) {\n";
            os_ << "        desc->level = 0;\n";
            os_ << "        desc->ancestors = NULL;\n";
            os_ << "    } else {\n";
            os_ << "        desc->level = parent->level + 1;\n";
            os_ << "        desc->ancestors = (StructDescriptor**)malloc(sizeof(StructDescriptor*) * desc->level);\n";
            os_ << "        for (int i = 0; i < parent->level; i++) {\n";
            os_ << "            desc->ancestors[i] = parent->ancestors[i];\n";
            os_ << "        }\n";
            os_ << "        desc->ancestors[desc->level - 1] = parent;\n";
            os_ << "    }\n";
            os_ << "    return desc;\n";
            os_ << "}\n\n";
            os_ << "static int isInstanceOf(StructDescriptor* actual, StructDescriptor* expected) {\n";
            os_ << "    if (actual == expected) return 1;\n";
            os_ << "    if (actual->level <= expected->level) return 0;\n";
            os_ << "    return actual->ancestors[expected->level] == expected;\n";
            os_ << "}\n";
            os_ << "#endif\n\n";

            // Struct descriptor definitions
            os_ << "/* Struct descriptors */\n";
            for (const auto& st : structTypes_) {
                os_ << "StructDescriptor " << st.first << "_desc;\n";
            }
            os_ << "\n";

            // Initialization function
            os_ << "static void _init_descriptors(void) {\n";
            os_ << "    static int initialized = 0;\n";
            os_ << "    if (initialized) return;\n";
            os_ << "    initialized = 1;\n";
            for (const auto& st : structTypes_) {
                os_ << "    {\n";
                os_ << "        StructDescriptor* d = createStructDescriptor(\"" << st.first << "\", ";
                if (st.second.empty()) {
                    os_ << "NULL";
                } else {
                    os_ << "&" << st.second << "_desc";
                }
                os_ << ");\n";
                os_ << "        " << st.first << "_desc = *d;\n";
                os_ << "        free(d);\n";
                os_ << "    }\n";
            }
            os_ << "}\n\n";
        }

        // Global variable definitions
        if (node.declarations && node.declarations->variables) {
            os_ << "/* Global variables */\n";
            for (auto& var : node.declarations->variables->variables) {
                if (!var->isExported) {
                    os_ << "static ";
                }
                std::string varName = makePrefix(moduleName_) + var->name;
                if (auto* arrType = dynamic_cast<ArrayType*>(var->type.get())) {
                    std::vector<Expression*> dimensions;
                    Type* currentType = var->type.get();
                    while (auto* arr = dynamic_cast<ArrayType*>(currentType)) {
                        dimensions.push_back(arr->length.get());
                        currentType = arr->elementType.get();
                    }
                    currentType->accept(*this);
                    os_ << " " << varName;
                    for (auto* dim : dimensions) {
                        os_ << "[";
                        dim->accept(*this);
                        os_ << "]";
                    }
                    os_ << ";\n";
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
                    std::string spFullName = makePrefix(moduleName_) + proc->name;
                    os_ << " " << spFullName << "(";
                    for (size_t i = 0; i < proc->parameters.size(); ++i) {
                        if (i > 0) os_ << ", ";
                        proc->parameters[i]->accept(*this);
                    }
                    auto oaIt3 = procedureOpenArrayParams_.find(spFullName);
                    if (oaIt3 != procedureOpenArrayParams_.end()) {
                        for (auto& [idx, name] : oaIt3->second) {
                            os_ << ", int64_t " << name;
                        }
                    }
                    os_ << ");\n";
                }
            }
            os_ << "\n";
        }

        os_ << "/* Function implementations */\n";
        for (auto& proc : node.procedures) {
            proc->accept(*this);
            if (proc->name == "init") {
                hasInitProcedure_ = true;
            }
        }

        if (isMain_) {
            if (!hasInitProcedure_) {
                throw std::runtime_error(
                    "Error: --main flag specified but no 'init' procedure found in module '" +
                    moduleName_ + "'. The entry point must be 'fn init() -> void'.");
            }

            os_ << "/* Entry point */\n";
            os_ << "int main(int argc, char** argv) {\n";
            os_ << "    " << makePrefix(moduleName_) << "init();\n";
            os_ << "    return 0;\n";
            os_ << "}\n";
        }
    }
}

} // namespace obould
