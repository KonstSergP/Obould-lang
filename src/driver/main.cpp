#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <system_error>
#include <filesystem>

#include <argparse/argparse.hpp>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "info/ASTPrintVisitor.h"
#include "sema/SemanticAnalyzer.h"
#include "codegen/llvm/LLVMCodegen.h"
#include "symbol/SymbolFileGenerator.h"


enum class OutputMode
{
    TOKENS,
    AST,
    LLVM_IR,
    SYMBOLS
};

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

void emitSymbolFile(obould::Module& module, const std::filesystem::path& symDir)
{
    std::error_code ec;
    std::filesystem::create_directories(symDir, ec);
    if (ec) {
        throw std::runtime_error("Cannot create symbol directory: " + ec.message());
    }
    obould::SymbolFileGenerator generator;
    module.accept(generator);
    auto symPath = symDir / (module.name + ".json");
    generator.saveToFile(symPath.string());
}

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("obould", "0.1");

    program.add_argument("input_file")
           .help("Obould source file");

    program.add_argument("--output", "-o")
           .help("Output file (defaults to stdout)")
           .default_value("");

    program.add_argument("--main", "-m")
           .help("Specify current file as main file in project")
           .flag();

    program.add_argument("--emit-tokens", "-t")
           .help("Emit lexemes/tokens")
           .flag();

    program.add_argument("--emit-ast", "-a")
           .help("Emit AST")
           .flag();

    program.add_argument("--emit-llvm", "-l")
           .help("Emit LLVM IR")
           .flag();

    program.add_argument("--emit-symbols", "-s")
           .help("Emit symbol file to .obould/<module>.json")
           .flag();

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    OutputMode mode = OutputMode::TOKENS;
    if (program["--emit-llvm"] == true) mode = OutputMode::LLVM_IR;
    else if (program["--emit-ast"] == true) mode = OutputMode::AST;
    else if (program["--emit-symbols"] == true) mode = OutputMode::SYMBOLS;
    else if (program["--emit-tokens"] == true) mode = OutputMode::TOKENS;

    auto inputPath = program.get<std::string>("input_file");
    auto outputPath = program.get<std::string>("output");

    try {
        std::string source = readFile(inputPath);
        std::filesystem::path symDir = std::filesystem::current_path() / ".obould";

        obould::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (lexer.hasErrors()) {
            std::cerr << "Lexer errors:\n";
            for (const auto& error : lexer.getErrors()) std::cerr << "  " << error << "\n";
            return 1;
        }

        if (mode == OutputMode::TOKENS) {
            if (outputPath.empty()) {
                writeTokens(tokens, std::cout);
            }
            else {
                std::ofstream outFile(outputPath);
                if (!outFile) throw std::runtime_error("Cannot open output file");
                writeTokens(tokens, outFile);
            }
            return 0;
        }

        obould::Parser parser(tokens);
        auto module = parser.parse();

        if (parser.hasErrors()) {
            std::cerr << "Parser errors:\n";
            for (const auto& error : parser.getErrors()) std::cerr << "  " << error << "\n";
            return 1;
        }

        if (mode == OutputMode::AST) {
            if (outputPath.empty()) {
                obould::ASTPrintVisitor printer(std::cout);
                module->accept(printer);
            }
            else {
                std::ofstream outFile(outputPath);
                obould::ASTPrintVisitor printer(outFile);
                module->accept(printer);
            }
        }
        else if (mode == OutputMode::SYMBOLS) {
            obould::SemanticAnalyzer sema;
            sema.setSymbolFileDir(symDir);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, symDir);
        }
        else if (mode == OutputMode::LLVM_IR) {
            obould::SemanticAnalyzer sema;
            sema.setSymbolFileDir(symDir);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, symDir);

            obould::LLVMCodegenVisitor codegen;
            auto llvmModule = codegen.codegen(*module);

            if (outputPath.empty()) {
                llvmModule->print(llvm::outs(), nullptr);
            }
            else {
                std::error_code ec;
                llvm::raw_fd_ostream outFile(outputPath, ec);
                if (ec) throw std::runtime_error("Cannot open output file: " + ec.message());
                llvmModule->print(outFile, nullptr);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Critical Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
