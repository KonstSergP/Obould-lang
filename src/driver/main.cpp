#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <system_error>
#include <filesystem>
#include <optional>

#include <argparse/argparse.hpp>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "info/ASTPrintVisitor.h"
#include "sema/SemanticAnalyzer.h"
#include "codegen/llvm/LLVMCodegen.h"
#include "codegen/c/CCodegenVisitor.h"
#include "symbol/SymbolFileGenerator.h"


enum class OutputMode
{
    TOKENS,
    AST,
    LLVM_IR,
    C_CODE,
    SYMBOLS,
    OBJ
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

void ensureDirExists(const std::filesystem::path& dir)
{
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        throw std::runtime_error("Cannot create output directory: " + ec.message());
    }
}

void emitSymbolFile(obould::Module& module, const std::filesystem::path& symDir)
{
    ensureDirExists(symDir);
    obould::SymbolFileGenerator generator;
    module.accept(generator);
    auto symPath = symDir / (module.name + ".json");
    generator.saveToFile(symPath.string());
}

void emitObjectFile(llvm::Module& module, const std::filesystem::path& outPath)
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto targetTriple = llvm::Triple(llvm::sys::getDefaultTargetTriple());
    module.setTargetTriple(targetTriple);

    std::string error;
    const auto* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        throw std::runtime_error("Target lookup failed: " + error);
    }

    llvm::TargetOptions opt;
    auto rm = std::optional<llvm::Reloc::Model>();
    auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(targetTriple, "generic", "", opt, rm));

    module.setDataLayout(targetMachine->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(outPath.string(), ec);
    if (ec) {
        throw std::runtime_error("Cannot open output file: " + ec.message());
    }

    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit object file");
    }

    pass.run(module);
    dest.flush();
}


int main(int argc, char** argv)
{
    argparse::ArgumentParser program("obould", "0.1");

    program.add_argument("input_file")
           .help("Obould source file");

    program.add_argument("--output", "-o")
           .help("Output path (for --emit-c: directory path, files named by module)")
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

    program.add_argument("--emit-c", "-c")
           .help("Emit C code")
           .flag();

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    OutputMode mode = OutputMode::OBJ;
    if (program["emit-llvm"] == true) mode = OutputMode::LLVM_IR;
    else if (program["emit-c"] == true) mode = OutputMode::C_CODE;
    else if (program["emit-ast"] == true) mode = OutputMode::AST;
    else if (program["emit-symbols"] == true) mode = OutputMode::SYMBOLS;
    else if (program["emit-tokens"] == true) mode = OutputMode::TOKENS;

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
        else if (mode == OutputMode::C_CODE) {
            // Semantic analysis is required for C code generation
            // obould::SemanticAnalyzer sema;
            // sema.setSymbolFileDir(symDir);
            // if (!sema.analyze(*module)) {
            //     std::cerr << "Semantic analysis failed:\n";
            //     for (const auto& error : sema.getErrors()) std::cerr << "  " << error << "\n";
            //     return 1;
            // }

            bool isMain = program.get<bool>("main");

            if (outputPath.empty()) {
                // Output both header and source to stdout (header first)
                obould::CCodegenVisitor hgen(std::cout, obould::COutputMode::HEADER);
                module->accept(hgen);
                std::cout << "\n/* ========== SOURCE FILE ========== */\n\n";
                obould::CCodegenVisitor cgen(std::cout, obould::COutputMode::SOURCE);
                cgen.setIsMain(isMain);
                module->accept(cgen);
            }
            else {
                std::filesystem::path outDir(outputPath);
                ensureDirExists(outDir);

                std::filesystem::path headerPath = outDir / (module->name + ".h");
                std::filesystem::path sourcePath = outDir / (module->name + ".c");

                // Generate header file
                {
                    std::ofstream headerFile(headerPath);
                    if (!headerFile) throw std::runtime_error("Cannot open header file: " + headerPath.string());
                    obould::CCodegenVisitor hgen(headerFile, obould::COutputMode::HEADER);
                    module->accept(hgen);
                }

                // Generate source file
                {
                    std::ofstream sourceFile(sourcePath);
                    if (!sourceFile) throw std::runtime_error("Cannot open source file: " + sourcePath.string());
                    obould::CCodegenVisitor cgen(sourceFile, obould::COutputMode::SOURCE);
                    cgen.setIsMain(isMain);
                    module->accept(cgen);
                }

                std::cout << "Generated: " << headerPath.string() << "\n";
                std::cout << "Generated: " << sourcePath.string() << "\n";
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
        else if (mode == OutputMode::OBJ) {
            obould::SemanticAnalyzer sema;
            sema.setSymbolFileDir(symDir);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, symDir);

            obould::LLVMCodegenVisitor codegen;
            auto llvmModule = codegen.codegen(*module, program.get<bool>("main"));

            std::filesystem::path outPath;
            if (!outputPath.empty()) {
                outPath = outputPath;
            }
            else {
                auto stem = std::filesystem::path(inputPath).stem().string();
                outPath = symDir / (stem + ".o");
            }

            auto outDir = outPath.parent_path();
            if (!outDir.empty()) {
                ensureDirExists(outDir);
            }
            emitObjectFile(*llvmModule, outPath);
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
            auto llvmModule = codegen.codegen(*module, program.get<bool>("main"));

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
