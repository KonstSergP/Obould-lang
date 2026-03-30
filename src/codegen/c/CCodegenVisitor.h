#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include "ast/ASTVisitor.h"

namespace obould
{

enum class COutputMode
{
    HEADER,  // Generate .h file (declarations only)
    SOURCE   // Generate .c file (implementations)
};

class CCodegenVisitor : public ASTVisitor
{
public:
    explicit CCodegenVisitor(std::ostream& os, COutputMode mode = COutputMode::HEADER);

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(RealLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(Nil& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(IdentifierExpression& node) override;
    void visit(ArrayAccessExpression& node) override;
    void visit(MemberAccessExpression& node) override;
    void visit(DereferenceExpression& node) override;
    void visit(QualifiedNameNode& node) override;

    // Statements
    void visit(ProcedureCall& node) override;
    void visit(StatementsBlock& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(WhileBranch& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(SwitchStatement& node) override;
    void visit(SwitchCase& node) override;
    void visit(CaseLabel& node) override;

    // Types
    void visit(IdentifierType& node) override;
    void visit(ArrayType& node) override;
    void visit(OpenArrayType& node) override;
    void visit(StructType& node) override;
    void visit(PointerType& node) override;
    void visit(ProcedureType& node) override;

    // Declarations
    void visit(ConstantDeclaration& node) override;
    void visit(TypeDeclaration& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(ProcedureDeclaration& node) override;
    void visit(ProcedureParameter& node) override;
    void visit(ConstantDeclarations& node) override;
    void visit(TypeDeclarations& node) override;
    void visit(VariableDeclarations& node) override;
    void visit(DeclarationsBlock& node) override;
    void visit(Import& node) override;
    void visit(Module& node) override;

private:
    std::ostream& os_;
    COutputMode mode_;
    std::string indent_;
    std::string moduleName_;

    // For collecting type string (used when we need to build type before name)
    std::stringstream typeBuffer_;
    bool collectingType_ = false;

    // For array dimensions
    std::stringstream arrayDims_;

    // Current typedef name (for struct/procedure type declarations)
    std::string currentTypedefName_;

    // Reference parameter tracking
    std::set<std::string> referenceParams_;  // Names of reference parameters in current function
    // Map: procedure full name -> vector of bools indicating which params are references
    std::map<std::string, std::vector<bool>> procedureRefParams_;

    // RTTI: struct name -> parent struct name (empty if no parent)
    std::vector<std::pair<std::string, std::string>> structTypes_;

    // Struct field tracking for inheritance flattening
    // Maps struct full name -> list of (field name, field type as string)
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> structFields_;

    // Global variable and constant tracking (names that need module prefix)
    std::set<std::string> globalVariables_;
    std::set<std::string> globalConstants_;

    // When true, reference params are emitted as raw pointers (no dereference).
    // Used for RTTI: `is` operator and type guards need the pointer, not the value.
    bool suppressDeref_ = false;

    // Flag indicating this is the main module (should generate main() calling init())
    bool isMain_ = false;

    // Track if init() procedure exists
    bool hasInitProcedure_ = false;

public:
    void setIsMain(bool isMain) { isMain_ = isMain; }

private:
    void increaseIndent();
    void decreaseIndent();
    void emitIndent();

    // Helper to map Obould types to C types
    std::string mapTypeName(const std::string& obouldType);

    // Helper to generate include guard name
    std::string generateGuardName(const std::string& moduleName);

    // Helper to emit type with variable name (handles arrays)
    void emitTypeWithName(Type& type, const std::string& name);

    // Helper to get type as string
    std::string getTypeString(Type& type);

    // Helper for expressions - returns the expression as string
    std::string getExpressionString(Expression& expr);

    // Helper to generate mangled prefix: "ob_{len}{moduleName}_"
    static std::string makePrefix(const std::string& moduleName);
};
}
