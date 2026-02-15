#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <system_error>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "info/ASTPrintVisitor.h"
#include "sema/SemanticAnalyzer.h"
#include "codegen/llvm/LLVMCodegen.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

enum class OutputMode
{
    TOKENS,
    AST,
    LLVM_IR
};

void printUsage(const char* programName)
{
    std::cerr << "Obould Compiler\n\n";
    std::cerr << "Usage: " << programName << " [options] <input_file> [output_file]\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  --tokens, -t    Output lexemes/tokens (default)\n";
    std::cerr << "  --ast, -a       Parse and output AST\n";
    std::cerr << "  --llvm-ir, -l   Generate and output LLVM IR\n";
    std::cerr << "  --help, -h      Show this help message\n\n";
    std::cerr << "Arguments:\n";
    std::cerr << "  input_file      Obould source file (.obl)\n";
    std::cerr << "  output_file     Output file (optional, defaults to stdout)\n";
}

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

void writeTokens(const std::vector<obould::Token>& tokens, std::ostream& out)
{
    for (const auto& token : tokens) {
        out << token << "\n";
    }
}

int main(int argc, char* argv[])
{
    OutputMode mode = OutputMode::TOKENS;
    std::string inputPath;
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--tokens") == 0 || strcmp(argv[i], "-t") == 0) {
            mode = OutputMode::TOKENS;
        }
        else if (strcmp(argv[i], "--ast") == 0 || strcmp(argv[i], "-a") == 0) {
            mode = OutputMode::AST;
        }
        else if (strcmp(argv[i], "--llvm-ir") == 0 || strcmp(argv[i], "-l") == 0) {
            mode = OutputMode::LLVM_IR;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        else if (argv[i][0] == '-') {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        }
        else if (inputPath.empty()) {
            inputPath = argv[i];
        }
        else if (outputPath.empty()) {
            outputPath = argv[i];
        }
        else {
            std::cerr << "Too many arguments\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (inputPath.empty()) {
        printUsage(argv[0]);
        return 1;
    }

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

        if (mode == OutputMode::TOKENS) {
            if (outputPath.empty()) {
                writeTokens(tokens, std::cout);
            }
            else {
                std::ofstream outFile(outputPath);
                if (!outFile) {
                    std::cerr << "Cannot open output file: " << outputPath << "\n";
                    return 1;
                }
                writeTokens(tokens, outFile);
                std::cout << "Lexemes written to: " << outputPath << "\n";
            }
        }
        else if (mode == OutputMode::AST) {
            // Parse and output AST
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

            if (outputPath.empty()) {
                obould::ASTPrintVisitor printer(std::cout);
                module->accept(printer);
            }
            else {
                std::ofstream outFile(outputPath);
                if (!outFile) {
                    std::cerr << "Cannot open output file: " << outputPath << "\n";
                    return 1;
                }
                obould::ASTPrintVisitor printer(outFile);
                module->accept(printer);
                std::cout << "AST written to: " << outputPath << "\n";
            }
        }
        else {
            // Generate and output LLVM IR
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
                return 1;
            }

            obould::LLVMCodegenVisitor codegen;
            auto llvmModule = codegen.codegen(*module);

            if (!llvmModule) {
                std::cerr << "Failed to generate LLVM IR\n";
                return 1;
            }

            if (outputPath.empty()) {
                llvmModule->print(llvm::outs(), nullptr);
            }
            else {
                std::error_code ec;
                llvm::raw_fd_ostream outFile(outputPath, ec);
                if (ec) {
                    std::cerr << "Cannot open output file: " << outputPath << "\n";
                    return 1;
                }
                llvmModule->print(outFile, nullptr);
                std::cout << "LLVM IR written to: " << outputPath << "\n";
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
