#pragma once
#include "ast/ASTVisitor.h"
#include <memory>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>


namespace obould
{
class LLVMCodegenVisitor : public ASTVisitor
{
public:
    std::unique_ptr<llvm::Module> codegen(Module& moduleAst, bool isMain);

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
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::Value* lastValue = nullptr;
    llvm::Function* currentFunction = nullptr;
    std::string currentModule;
    bool isMainModule = false;
    bool importedModule = false;
    bool exportedType = false;
    bool lvalue = false;

    std::unordered_map<std::string, llvm::Value*> locals;
    std::unordered_map<std::string, llvm::Function*> functions;
    std::unordered_map<TypeInfo*, llvm::StructType*> structTypes;
    std::unordered_map<TypeInfo*, llvm::GlobalVariable*> descriptors;
    std::unordered_map<TypeInfo*, llvm::Value*> lengths;

    static std::string getMangledName(const std::string& moduleName, const std::string& name);
    llvm::Type* toLLVMType(const std::shared_ptr<TypeInfo>& typeInfo);
    llvm::Value* getConstantValue(const Expression& node);
    llvm::AllocaInst* createEntryAlloca(llvm::Type* type, const std::string& name = "") const;
    llvm::Function* declareFunction(ProcedureDeclaration& node);
    llvm::FunctionType* createFunctionType(const std::shared_ptr<TypeInfo>& type);
    llvm::GlobalVariable* createStructDescriptor(const std::shared_ptr<TypeInfo>& type);
    void makeStructCastCheck(llvm::Value* left, llvm::Value* right, int64_t rDepth);
    void createModuleInitializer(Module& node);
    void createEntryPoint(Module& node);
    void visitBuiltinProcedure(ProcedureCall& node);
};
}
