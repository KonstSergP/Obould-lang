#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "sema/SemanticAnalyzer.h"
#include "codegen/llvm/LLVMCodegen.h"
#include <llvm/Support/raw_ostream.h>


std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


int main()
{
    std::string inputPath = "../tests/comprehensive_test.obl";

    try {
        std::string source = readFile(inputPath);

        obould::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (lexer.hasErrors()) {
            std::cerr << "Lexer errors:\n";
            for (const auto& error : lexer.getErrors()) {
                std::cerr << "  " << error << "\n";
            }
            return 1;
        }

        obould::Parser parser(tokens);
        auto module = parser.parse();

        if (parser.hasErrors()) {
            std::cerr << "Parser errors:\n";
            for (const auto& error : parser.getErrors()) {
                std::cerr << "  " << error << "\n";
            }
            return 1;
        }

        if (!module) {
            std::cerr << "Failed to parse module\n";
            return 1;
        }

        obould::SemanticAnalyzer sema;
        bool semaSuccess = sema.analyze(*module);

        if (!semaSuccess) {
            std::cerr << "Semantic analysis errors:\n";
            for (const auto& error : sema.getErrors()) {
                std::cerr << "  " << error << "\n";
            }
        }

        obould::LLVMCodegenVisitor codegen;
        auto llvmModule = codegen.codegen(*module, false);

        if (!llvmModule) {
            std::cerr << "Failed to generate LLVM IR\n";
            return 1;
        }

        llvmModule->print(llvm::outs(), nullptr);
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
