#include "info/ASTPrintVisitor.h"


int main()
{
    // Импорты
    std::vector<std::unique_ptr<Import>> imports;
    imports.push_back(std::make_unique<Import>("Out", "Out"));
    imports.push_back(std::make_unique<Import>("Math", "Math"));

    // Константы
    std::vector<std::unique_ptr<ConstantDeclaration>> constants;
    constants.push_back(std::make_unique<ConstantDeclaration>(
        "MaxSize", false, std::make_unique<IntegerLiteral>(100)
    ));
    constants.push_back(std::make_unique<ConstantDeclaration>(
        "Pi", false, std::make_unique<RealLiteral>(3.14159)
    ));

    // Типы
    std::vector<std::unique_ptr<TypeDeclaration>> types;

    std::vector<std::unique_ptr<VariableDeclaration>> pointFields;
    pointFields.push_back(std::make_unique<VariableDeclaration>(
        "x", false, std::make_unique<IdentifierType>("", "REAL")
    ));
    pointFields.push_back(std::make_unique<VariableDeclaration>(
        "y", false, std::make_unique<IdentifierType>("", "REAL")
    ));
    types.push_back(std::make_unique<TypeDeclaration>(
        "Point",
        false,
        std::make_unique<StructType>(std::make_unique<IdentifierType>("", "object"), std::move(pointFields))
    ));

    types.push_back(std::make_unique<TypeDeclaration>(
        "Vector",
        false,
        std::make_unique<ArrayType>(
            std::make_unique<IdentifierType>("", "INTEGER"),
            std::make_unique<IntegerLiteral>(5)
        )
    ));

    // Переменные
    std::vector<std::unique_ptr<VariableDeclaration>> variables;
    variables.push_back(std::make_unique<VariableDeclaration>(
        "counter", false, std::make_unique<IdentifierType>("", "INTEGER")
    ));
    variables.push_back(std::make_unique<VariableDeclaration>(
        "done", false, std::make_unique<IdentifierType>("", "BOOLEAN")
    ));

    auto declarationsBlock = std::make_unique<DeclarationsBlock>(
        std::make_unique<ConstantDeclarations>(std::move(constants)),
        std::make_unique<TypeDeclarations>(std::move(types)),
        std::make_unique<VariableDeclarations>(std::move(variables))
    );

    // Процедура compute
    std::vector<std::unique_ptr<Statement>> computeStmts;

    auto ifCondition = std::make_unique<BinaryExpression>(
        std::make_unique<IdentifierExpression>("", "counter"),
        std::make_unique<IdentifierExpression>("", "MaxSize"),
        BinaryExpression::Op::Lt
    );

    std::vector<std::unique_ptr<Statement>> thenStmts;
    thenStmts.push_back(std::make_unique<AssignmentStatement>(
        std::make_unique<IdentifierExpression>("", "counter"),
        std::make_unique<BinaryExpression>(
            std::make_unique<IdentifierExpression>("", "counter"),
            std::make_unique<IntegerLiteral>(1),
            BinaryExpression::Op::Add
        )
    ));

    std::vector<std::unique_ptr<Statement>> elseStmts;
    elseStmts.push_back(std::make_unique<AssignmentStatement>(
        std::make_unique<IdentifierExpression>("", "done"),
        std::make_unique<BooleanLiteral>(true)
    ));

    auto ifStmt = std::make_unique<IfStatement>(
        std::move(ifCondition),
        std::make_unique<StatementsBlock>(std::move(thenStmts)),
        std::make_unique<StatementsBlock>(std::move(elseStmts))
    );
    computeStmts.push_back(std::move(ifStmt));

    auto whileCondition = std::make_unique<UnaryExpression>(
        std::make_unique<IdentifierExpression>("", "done"),
        UnaryExpression::Op::Not
    );

    std::vector<std::unique_ptr<Statement>> whileBodyStmts;

    std::vector<std::unique_ptr<Expression>> callArgs1;
    callArgs1.push_back(std::make_unique<StringLiteral>("Hello world!"));

    whileBodyStmts.push_back(std::make_unique<ProcedureCall>(
        std::make_unique<IdentifierExpression>("Out", "String"),
        std::move(callArgs1)
    ));

    whileBodyStmts.push_back(std::make_unique<AssignmentStatement>(
        std::make_unique<IdentifierExpression>("", "counter"),
        std::make_unique<BinaryExpression>(
            std::make_unique<IdentifierExpression>("", "counter"),
            std::make_unique<IntegerLiteral>(1),
            BinaryExpression::Op::Sub
        )
    ));

    whileBodyStmts.push_back(std::make_unique<AssignmentStatement>(
        std::make_unique<IdentifierExpression>("", "done"),
        std::make_unique<BinaryExpression>(
            std::make_unique<IdentifierExpression>("", "counter"),
            std::make_unique<IntegerLiteral>(0),
            BinaryExpression::Op::Eq
        )
    ));

    std::vector<std::unique_ptr<WhileBranch>> whileBranches;
    whileBranches.push_back(std::make_unique<WhileBranch>(
        std::move(whileCondition),
        std::make_unique<StatementsBlock>(std::move(whileBodyStmts))
    ));

    computeStmts.push_back(std::make_unique<WhileStatement>(std::move(whileBranches)));

    auto computeBody = std::make_unique<StatementsBlock>(std::move(computeStmts));
    auto computeParameter = std::vector<std::unique_ptr<ProcedureParameter>>();

    computeParameter.push_back(
        std::make_unique<ProcedureParameter>("state", true, std::make_unique<IdentifierType>("", "integer")));

    auto procCompute = std::make_unique<ProcedureDeclaration>(
        "compute",
        false,
        std::move(computeParameter),
        std::make_unique<IdentifierType>("", "void"),
        nullptr,
        std::move(computeBody),
        nullptr
    );

    // Процедура main
    std::vector<std::unique_ptr<Statement>> mainStmts;

    std::vector<std::unique_ptr<SwitchCase>> switchCases;

    std::vector<std::unique_ptr<CaseLabel>> labels1;
    labels1.push_back(std::make_unique<CaseLabel>(
        std::make_unique<IntegerLiteral>(0),
        nullptr
    ));

    std::vector<std::unique_ptr<Statement>> case0Stmts;
    std::vector<std::unique_ptr<Expression>> argsZero;
    argsZero.push_back(std::make_unique<StringLiteral>("Counter is zero."));

    case0Stmts.push_back(std::make_unique<ProcedureCall>(
        std::make_unique<IdentifierExpression>("Out", "String"),
        std::move(argsZero)
    ));

    switchCases.push_back(std::make_unique<SwitchCase>(
        std::move(labels1),
        std::make_unique<StatementsBlock>(std::move(case0Stmts))
    ));

    std::vector<std::unique_ptr<CaseLabel>> labels2;
    labels2.push_back(std::make_unique<CaseLabel>(
        std::make_unique<IntegerLiteral>(1),
        std::make_unique<IntegerLiteral>(5)
    ));

    std::vector<std::unique_ptr<Statement>> caseSmallStmts;
    std::vector<std::unique_ptr<Expression>> argsSmall;
    argsSmall.push_back(std::make_unique<StringLiteral>("Counter in [1;5]"));

    caseSmallStmts.push_back(std::make_unique<ProcedureCall>(
        std::make_unique<IdentifierExpression>("Out", "String"),
        std::move(argsSmall)
    ));

    switchCases.push_back(std::make_unique<SwitchCase>(
        std::move(labels2),
        std::make_unique<StatementsBlock>(std::move(caseSmallStmts))
    ));

    mainStmts.push_back(std::make_unique<SwitchStatement>(
        std::make_unique<IdentifierExpression>("", "counter"),
        std::move(switchCases)
    ));

    mainStmts.push_back(std::make_unique<ProcedureCall>(
        std::make_unique<IdentifierExpression>("Out", "Ln"),
        std::vector<std::unique_ptr<Expression>>()
    ));

    auto mainBody = std::make_unique<StatementsBlock>(std::move(mainStmts));

    auto procMain = std::make_unique<ProcedureDeclaration>(
        "main",
        true,
        std::vector<std::unique_ptr<ProcedureParameter>>(),
        std::make_unique<IdentifierType>("", "void"),
        nullptr,
        std::move(mainBody),
        nullptr
    );

    std::vector<std::unique_ptr<ProcedureDeclaration>> procedures;
    procedures.push_back(std::move(procCompute));
    procedures.push_back(std::move(procMain));

    // Модуль
    auto module = std::make_unique<Module>(
        "ASTDemo",
        std::move(imports),
        std::move(declarationsBlock),
        std::move(procedures)
    );

    // Вывод
    ASTPrintVisitor printer;
    printer.printNode(module);

    return 0;
}
