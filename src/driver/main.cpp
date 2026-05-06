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
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/Verifier.h>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "info/ASTPrintVisitor.h"
#include "sema/SemanticAnalyzer.h"
#include "codegen/llvm/LLVMCodegen.h"
#include "codegen/c/CCodegenVisitor.h"
#include "symbol/SymbolFileGenerator.h"


namespace fs = std::filesystem;

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

std::string linkerGcSectionsFlag()
{
#if defined(__APPLE__)
    return "-Wl,-dead_strip";
#else
    return "-Wl,--gc-sections";
#endif
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
    auto rm = std::optional(llvm::Reloc::PIC_);
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

void optimizeModule(llvm::Module& module, llvm::OptimizationLevel level)
{
    if (level == llvm::OptimizationLevel::O0) {
        return;
    }

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(level);

    MPM.run(module, MAM);
}

llvm::OptimizationLevel getOptLevel(const argparse::ArgumentParser& program)
{
    auto opt_level = llvm::OptimizationLevel::O1;
    if (program.is_used("-O0")) {
        opt_level = llvm::OptimizationLevel::O0;
    }
    else if (program.is_used("-O1")) {
        opt_level = llvm::OptimizationLevel::O1;
    }
    else if (program.is_used("-O2")) {
        opt_level = llvm::OptimizationLevel::O2;
    }
    else if (program.is_used("-O3")) {
        opt_level = llvm::OptimizationLevel::O3;
    }
    else if (program.is_used("-Os")) {
        opt_level = llvm::OptimizationLevel::Os;
    }
    else if (program.is_used("-Oz")) {
        opt_level = llvm::OptimizationLevel::Oz;
    }
    return opt_level;
}


int main(int argc, char** argv)
{
    argparse::ArgumentParser program("obould", "0.1");

    program.add_argument("input_files")
           .help("Obould source file and/or object files")
           .nargs(argparse::nargs_pattern::any);

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

    program.add_argument("--link")
           .help("Link with standard libraries")
           .flag();

    auto& opt_group = program.add_mutually_exclusive_group();
    opt_group.add_argument("-O0").flag().help("No optimization");
    opt_group.add_argument("-O1").flag().help("Fast optimization (default)");
    opt_group.add_argument("-O2").flag().help("Execution speed optimization");
    opt_group.add_argument("-O3").flag().help("Aggressive execution speed optimization");
    opt_group.add_argument("-Os").flag().help("Binary size optimization");
    opt_group.add_argument("-Oz").flag().help("Aggressive binary size optimization");

    program.add_argument("--sympath", "-S")
           .help("Directories to search for imported symbol files (can be used multiple times)")
           .append();

    program.add_argument("--obj")
           .help("Object files to link (.o)")
           .append();

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::error_code ec;
    fs::path compilerPath = fs::canonical(fs::path(argv[0]), ec);
    if (ec) {
        compilerPath = fs::absolute(argv[0]);
    }
    fs::path systemDir = compilerPath.parent_path() / "system";
    fs::path defaultLocalDir = fs::current_path() / ".obould";

    OutputMode mode = OutputMode::OBJ;
    if (program["emit-llvm"] == true) mode = OutputMode::LLVM_IR;
    else if (program["emit-c"] == true) mode = OutputMode::C_CODE;
    else if (program["emit-ast"] == true) mode = OutputMode::AST;
    else if (program["emit-symbols"] == true) mode = OutputMode::SYMBOLS;
    else if (program["emit-tokens"] == true) mode = OutputMode::TOKENS;

    auto optLevel = getOptLevel(program);

    auto inputFiles = program.get<std::vector<std::string>>("input_files");
    auto objFilesFromFlag = program.get<std::vector<std::string>>("obj");
    if (inputFiles.empty()) {
        std::cerr << "Error: input file is required." << "\n";
        return 1;
    }

    auto outputPath = program.get<std::string>("output");
    auto symPathsStrings = program.get<std::vector<std::string>>("sympath");
    bool doLink = program.get<bool>("link");

    if (doLink && mode != OutputMode::OBJ) {
        std::cerr << "Error: --link is only supported when producing object code (no --emit-* flags)." << "\n";
        return 1;
    }

    std::vector<std::string> oblFiles;
    std::vector objFiles(objFilesFromFlag.begin(), objFilesFromFlag.end());
    for (const auto& file : inputFiles) {
        auto ext = fs::path(file).extension().string();
        if (ext == ".obl") {
            oblFiles.push_back(file);
        }
        else if (ext == ".o") {
            objFiles.push_back(file);
        }
        else {
            std::cerr << "Error: unsupported input file '" << file << "'." << "\n";
            return 1;
        }
    }

    if (oblFiles.size() > 1) {
        std::cerr << "Error: only one .obl input file is supported." << "\n";
        return 1;
    }

    bool linkOnly = doLink && oblFiles.empty();
    if (!doLink && !objFiles.empty()) {
        std::cerr << "Error: object files are only supported with --link." << "\n";
        return 1;
    }
    if (linkOnly && objFiles.empty()) {
        std::cerr << "Error: --link requires at least one .o file when no .obl input is provided." << "\n";
        return 1;
    }

    auto inputPath = oblFiles.empty() ? std::string{} : oblFiles.front();

    std::vector<fs::path> searchDirs;
    if (fs::exists(systemDir)) {
        searchDirs.push_back(systemDir);
    }
    if (symPathsStrings.empty()) {
        searchDirs.push_back(defaultLocalDir);
    }
    for (const auto& path : symPathsStrings) {
        searchDirs.push_back(fs::absolute(path));
    }
    fs::path outDir;
    if (!outputPath.empty()) {
        outDir = fs::absolute(outputPath);
        if (mode != OutputMode::C_CODE) outDir = outDir.parent_path();
    }
    else if (!symPathsStrings.empty()) {
        outDir = fs::absolute(symPathsStrings[0]);
    }
    else {
        outDir = fs::absolute(defaultLocalDir);
    }

    if (linkOnly) {
        fs::path libDir = compilerPath.parent_path() / "lib";
        fs::path stdlibPath = libDir / "libobould_std.a";
        fs::path gcPath = libDir / "libgc.a";

        if (!fs::exists(stdlibPath)) {
            throw std::runtime_error("Cannot find standard library: " + stdlibPath.string());
        }
        if (!fs::exists(gcPath)) {
            throw std::runtime_error("Cannot find GC library: " + gcPath.string());
        }

        fs::path binPath = outputPath.empty() ? fs::path("a.out") : fs::path(outputPath);

        auto shellQuote = [](const std::string& s)
        {
            std::string out = "\"";
            for (char ch : s) {
                if (ch == '\"' || ch == '\\') out.push_back('\\');
                out.push_back(ch);
            }
            out.push_back('\"');
            return out;
        };

        std::ostringstream cmd;
        cmd << "cc " << linkerGcSectionsFlag() << " -o " << shellQuote(binPath.string());
        for (const auto& obj : objFiles) {
            cmd << " " << shellQuote(obj);
        }
        cmd << " " << shellQuote(stdlibPath.string());
        cmd << " " << shellQuote(gcPath.string());
        cmd << " -lm";

        int rc = std::system(cmd.str().c_str());
        if (rc != 0) {
            std::cerr << "Linking failed." << "\n";
            return 1;
        }
        return 0;
    }

    try {
        obould::Lexer lexer(readFile(inputPath));
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
            obould::SemanticAnalyzer sema;
            sema.setSymbolSearchDirs(searchDirs);
            if (!sema.analyze(*module)) {
                std::cerr << "Semantic analysis failed:\n";
                for (const auto& error : sema.getErrors()) std::cerr << "  " << error << "\n";
                return 1;
            }

            bool isMain = program.get<bool>("main");

            if (outputPath.empty()) {
                obould::CCodegenVisitor hgen(std::cout, obould::COutputMode::HEADER);
                module->accept(hgen);
                std::cout << "\n/* ========== SOURCE FILE ========== */\n\n";
                obould::CCodegenVisitor cgen(std::cout, obould::COutputMode::SOURCE);
                cgen.setIsMain(isMain);
                module->accept(cgen);
            }
            else {
                fs::path outDir(outputPath);
                ensureDirExists(outDir);

                fs::path headerPath = outDir / (module->name + ".h");
                fs::path sourcePath = outDir / (module->name + ".c");

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
            sema.setSymbolSearchDirs(searchDirs);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, outDir);
        }
        else if (mode == OutputMode::OBJ) {
            obould::SemanticAnalyzer sema;
            sema.setSymbolSearchDirs(searchDirs);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, outDir);

            obould::LLVMCodegenVisitor codegen;
            auto llvmModule = codegen.codegen(*module, program.get<bool>("main"));

            if (llvm::verifyModule(*llvmModule, &llvm::errs())) {
                llvm::errs() << "LLVM IR verification failed!\n";
                llvmModule->print(llvm::errs(), nullptr);
                exit(1);
            }

            optimizeModule(*llvmModule, optLevel);

            fs::path objPath;
            fs::path binPath;
            if (!outputPath.empty()) {
                fs::path outAbs = fs::absolute(outputPath);
                if (doLink && outAbs.extension() == ".o") {
                    objPath = outAbs;
                    binPath = outAbs;
                    binPath.replace_extension("");
                }
                else if (doLink) {
                    binPath = outAbs;
                    objPath = outAbs;
                    objPath += ".o";
                }
                else {
                    objPath = outAbs;
                }
            }
            else {
                auto stem = fs::path(inputPath).stem().string();
                objPath = outDir / (stem + ".o");
                if (doLink) {
                    binPath = outDir / stem;
                }
            }

            auto objDir = objPath.parent_path();
            if (!objDir.empty()) {
                ensureDirExists(objDir);
            }
            emitObjectFile(*llvmModule, objPath);

            if (doLink) {
                fs::path libDir = compilerPath.parent_path() / "lib";
                fs::path stdlibPath = libDir / "libobould_std.a";
                fs::path gcPath = libDir / "libgc.a";

                if (!fs::exists(stdlibPath)) {
                    throw std::runtime_error("Cannot find standard library: " + stdlibPath.string());
                }
                if (!fs::exists(gcPath)) {
                    throw std::runtime_error("Cannot find GC library: " + gcPath.string());
                }

                auto shellQuote = [](const std::string& s)
                {
                    std::string out = "\"";
                    for (char ch : s) {
                        if (ch == '\"' || ch == '\\') out.push_back('\\');
                        out.push_back(ch);
                    }
                    out.push_back('\"');
                    return out;
                };

                std::ostringstream cmd;
                cmd << "cc " << linkerGcSectionsFlag() << " -o " << shellQuote(binPath.string());
                cmd << " " << shellQuote(objPath.string());
                for (const auto& obj : objFiles) {
                    cmd << " " << shellQuote(obj);
                }
                cmd << " " << shellQuote(stdlibPath.string());
                cmd << " " << shellQuote(gcPath.string());
                cmd << " -lm";

                int rc = std::system(cmd.str().c_str());
                if (rc != 0) {
                    throw std::runtime_error("Linking failed.");
                }
            }
        }
        else if (mode == OutputMode::LLVM_IR) {
            obould::SemanticAnalyzer sema;
            sema.setSymbolSearchDirs(searchDirs);
            if (!sema.analyze(*module)) {
                for (const auto& error : sema.getErrors()) std::cerr << error << "\n";
                return 1;
            }

            emitSymbolFile(*module, outDir);

            obould::LLVMCodegenVisitor codegen;
            auto llvmModule = codegen.codegen(*module, program.get<bool>("main"));

            if (llvm::verifyModule(*llvmModule, &llvm::errs())) {
                llvm::errs() << "LLVM IR verification failed!\n";
                llvmModule->print(llvm::errs(), nullptr);
                exit(1);
            }

            optimizeModule(*llvmModule, optLevel);

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
