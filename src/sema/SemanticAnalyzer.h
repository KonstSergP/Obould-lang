#pragma once
#include <set>
#include <filesystem>
#include "SymbolTable.h"
#include "TypeInfo.h"
#include "ast/ASTVisitor.h"
#include "parser/SymbolFileParser.h"


namespace obould
{
class SemanticAnalyzer : public ASTVisitor
{
public:
    SemanticAnalyzer();

    bool analyze(Module& module);
    void setSymbolSearchDirs(const std::vector<std::filesystem::path>& dirs);
    const std::vector<std::string>& getErrors() const;
    void addError(const std::string& error);

    // Expressions
    void visit(IntegerLiteral& node) override;
    void visit(RealLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(SetLiteral& node) override;
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
    void visit(IncompleteType& node) override;

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
    std::shared_ptr<TypeInfo> createNewType(TypeKind kind);
    void createBuiltinTypes();
    void addBuiltinTypes(SymbolTable& symTable) const;
    void addBuiltinProcedures(SymbolTable& symTable);
    std::shared_ptr<TypeInfo> getBuiltinType(TypeKind kind);
    void declareProcedure(ProcedureDeclaration& node);
    void visitBuiltinProcedure(ProcedureCall& node);
    std::shared_ptr<TypeInfo> getPolymorphicBase(Expression* expr);
    void validateType(const std::shared_ptr<TypeInfo>& type);
    void validateTypeInternal(const std::shared_ptr<TypeInfo>& type, std::set<uint32_t>& visiting);


    enum class AnalyzeStages { Default, CreateType, FillType, ResolveAliases, ValidateType };

    AnalyzeStages analyzeStage = AnalyzeStages::Default;
    std::shared_ptr<TypeInfo> switchSelectorType;
    bool importedModule = false;
    std::vector<std::string> errors;
    std::unordered_map<std::string, std::shared_ptr<TypeInfo>> builtinTypes;
    std::unordered_map<std::string, SymbolTable> symbolTables;
    std::unordered_map<std::string, std::string> moduleRealNames;
    std::string currentModuleName;
    SymbolFileParser symbolFileParser;
    Module* rootModule = nullptr;
    uint32_t nextTypeId = 1;
};
}
