#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "info/ASTPrintVisitor.h"
#include "sema/SemanticAnalyzer.h"


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
    std::string inputPath = "../tests/test.obl";

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

        obould::ASTPrintVisitor printer(std::cout);
        module->accept(printer);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
