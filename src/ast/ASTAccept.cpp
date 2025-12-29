#include "ASTVisitor.h"
#include "ASTExpressions.h"
#include "ASTStatements.h"
#include "ASTTypes.h"
#include "ASTDeclarations.h"


// Expressions
void IntegerLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void RealLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void StringLiteral::accept(ASTVisitor& v) { v.visit(*this); }
void Nil::accept(ASTVisitor& v) { v.visit(*this); }
void BinaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void UnaryExpression::accept(ASTVisitor& v) { v.visit(*this); }
void IdentifierExpression::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayAccessExpression::accept(ASTVisitor& v) { v.visit(*this); }
void MemberAccessExpression::accept(ASTVisitor& v) { v.visit(*this); }
void DereferenceExpression::accept(ASTVisitor& v) { v.visit(*this); }

// Statements
void ProcedureCall::accept(ASTVisitor& v) { v.visit(*this); }
void AssignmentStatement::accept(ASTVisitor& v) { v.visit(*this); }
void IfStatement::accept(ASTVisitor& v) { v.visit(*this); }
void WhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void WhileBranch::accept(ASTVisitor& v) { v.visit(*this); }
void DoWhileStatement::accept(ASTVisitor& v) { v.visit(*this); }
void ForStatement::accept(ASTVisitor& v) { v.visit(*this); }
void SwitchStatement::accept(ASTVisitor& v) { v.visit(*this); }
void SwitchCase::accept(ASTVisitor& v) { v.visit(*this); }
void CaseLabel::accept(ASTVisitor& v) { v.visit(*this); }
void StatementsBlock::accept(ASTVisitor& v) { v.visit(*this); }

// Types
void IdentifierType::accept(ASTVisitor& v) { v.visit(*this); }
void ArrayType::accept(ASTVisitor& v) { v.visit(*this); }
void OpenArrayType::accept(ASTVisitor& v) { v.visit(*this); }
void StructType::accept(ASTVisitor& v) { v.visit(*this); }
void PointerType::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureType::accept(ASTVisitor& v) { v.visit(*this); }

// Declarations
void ConstantDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void TypeDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void VariableDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureDeclaration::accept(ASTVisitor& v) { v.visit(*this); }
void ProcedureParameter::accept(ASTVisitor& v) { v.visit(*this); }
void ConstantDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void TypeDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void VariableDeclarations::accept(ASTVisitor& v) { v.visit(*this); }
void DeclarationsBlock::accept(ASTVisitor& v) { v.visit(*this); }
void Import::accept(ASTVisitor& v) { v.visit(*this); }
void Module::accept(ASTVisitor& v) { v.visit(*this); }
