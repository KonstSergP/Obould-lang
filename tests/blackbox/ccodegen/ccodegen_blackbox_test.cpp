#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static fs::path obould_bin;
static fs::path stdlib_dir;
static fs::path gc_include_dir;
static fs::path gc_lib_dir;

static fs::path getObould()
{
    if (!obould_bin.empty()) return obould_bin;
    auto build = fs::path(OBOULD_BUILD_DIR);
    obould_bin = build / "obould";
    REQUIRE(fs::exists(obould_bin));
    return obould_bin;
}

static fs::path getStdlibDir()
{
    if (!stdlib_dir.empty()) return stdlib_dir;
    stdlib_dir = fs::path(OBOULD_STDLIB_DIR);
    REQUIRE(fs::exists(stdlib_dir / "Out.c"));
    return stdlib_dir;
}

static fs::path getStdlibArtifact(const std::string& lib, const std::string& ext)
{
    auto sourcePath = getStdlibDir() / (lib + ext);
    if (fs::exists(sourcePath)) return sourcePath;

    auto buildPath = fs::path(OBOULD_BUILD_DIR) / "system" / (lib + ext);
    REQUIRE(fs::exists(buildPath));
    return buildPath;
}

static fs::path getGcIncludeDir()
{
    if (!gc_include_dir.empty()) return gc_include_dir;
    gc_include_dir = fs::path(OBOULD_GC_INCLUDE_DIR);
    return gc_include_dir;
}

static fs::path getGcLibDir()
{
    if (!gc_lib_dir.empty()) return gc_lib_dir;
    gc_lib_dir = fs::path(OBOULD_GC_LIB_DIR);
    return gc_lib_dir;
}

static std::string transpileCompileRun(const std::string& moduleName,
                                         const std::string& source,
                                         const std::vector<std::string>& stdlibs,
                                         const std::string& stdinData = "")
{
    auto tmpDir = fs::temp_directory_path() / ("obould_bb_" + moduleName + "_" +
        std::to_string(std::hash<std::string>{}(source)));
    fs::create_directories(tmpDir);

    auto oblPath = tmpDir / (moduleName + ".obl");
    {
        std::ofstream f(oblPath);
        REQUIRE(f.is_open());
        f << source;
    }

    auto symDir = tmpDir / ".obould";
    if (!stdlibs.empty()) {
        fs::create_directories(symDir);
    }
    for (auto& lib : stdlibs) {
        fs::copy_file(getStdlibArtifact(lib, ".h"), tmpDir / (lib + ".h"),
                       fs::copy_options::overwrite_existing);
        fs::copy_file(getStdlibArtifact(lib, ".c"), tmpDir / (lib + ".c"),
                       fs::copy_options::overwrite_existing);
        fs::copy_file(getStdlibArtifact(lib, ".json"), symDir / (lib + ".json"),
                       fs::copy_options::overwrite_existing);
    }

    std::string transpileCmd = "cd " + tmpDir.string() + " && " +
        getObould().string() +
        " " + oblPath.string() +
        " --emit-c -o " + tmpDir.string() +
        " --main 2>&1";
    REQUIRE(std::system(transpileCmd.c_str()) == 0);

    auto hPath = tmpDir / (moduleName + ".h");
    auto cPath = tmpDir / (moduleName + ".c");
    REQUIRE(fs::exists(hPath));
    REQUIRE(fs::exists(cPath));

    auto binPath = tmpDir / moduleName;
    std::string compileCmd = "cc -Wno-incompatible-pointer-types"
        " -I" + getGcIncludeDir().string() +
        " -o " + binPath.string() + " " + cPath.string();
    for (auto& lib : stdlibs) {
        compileCmd += " " + (tmpDir / (lib + ".c")).string();
    }
    compileCmd += " " + (getGcLibDir() / "libgc.a").string();
    compileCmd += " -lm";
    compileCmd += " 2>&1";
    REQUIRE(std::system(compileCmd.c_str()) == 0);

    auto outPath = tmpDir / "stdout.txt";
    std::string runCmd;
    if (!stdinData.empty()) {
        auto inPath = tmpDir / "stdin.txt";
        { std::ofstream inf(inPath); inf << stdinData; }
        runCmd = binPath.string() + " < " + inPath.string() + " > " + outPath.string() + " 2>&1";
    } else {
        runCmd = binPath.string() + " > " + outPath.string() + " 2>&1";
    }
    int rc = std::system(runCmd.c_str());
    REQUIRE(rc == 0);

    std::ifstream f(outPath);
    std::stringstream ss;
    ss << f.rdbuf();

    fs::remove_all(tmpDir);

    return ss.str();
}

struct ModuleSource {
    std::string name;
    std::string source;
};

static std::string multiModuleRun(const std::vector<ModuleSource>& libs,
                                  const ModuleSource& main)
{
    auto tmpDir = fs::temp_directory_path() / ("obould_mm_" + main.name + "_" +
        std::to_string(std::hash<std::string>{}(main.source)));
    fs::create_directories(tmpDir);

    auto symDir = tmpDir / ".obould";
    fs::create_directories(symDir);

    fs::copy_file(getStdlibDir() / "Out.h", tmpDir / "Out.h",
                   fs::copy_options::overwrite_existing);
    fs::copy_file(getStdlibDir() / "Out.c", tmpDir / "Out.c",
                   fs::copy_options::overwrite_existing);
    fs::copy_file(getStdlibDir() / "Out.json", symDir / "Out.json",
                   fs::copy_options::overwrite_existing);

    std::vector<fs::path> cFiles = { tmpDir / "Out.c" };

    for (auto& lib : libs) {
        auto oblPath = tmpDir / (lib.name + ".obl");
        { std::ofstream f(oblPath); f << lib.source; }

        std::string symCmd = "cd " + tmpDir.string() + " && " +
            getObould().string() + " " + oblPath.string() +
            " --emit-symbols 2>&1";
        REQUIRE(std::system(symCmd.c_str()) == 0);

        std::string cCmd = "cd " + tmpDir.string() + " && " +
            getObould().string() + " " + oblPath.string() +
            " --emit-c -o " + tmpDir.string() + " 2>&1";
        REQUIRE(std::system(cCmd.c_str()) == 0);

        cFiles.push_back(tmpDir / (lib.name + ".c"));
    }

    auto mainObl = tmpDir / (main.name + ".obl");
    { std::ofstream f(mainObl); f << main.source; }

    std::string mainCmd = "cd " + tmpDir.string() + " && " +
        getObould().string() + " " + mainObl.string() +
        " --emit-c -o " + tmpDir.string() + " --main 2>&1";
    REQUIRE(std::system(mainCmd.c_str()) == 0);
    cFiles.push_back(tmpDir / (main.name + ".c"));

    auto binPath = tmpDir / main.name;
    std::string compileCmd = "cc -Wno-incompatible-pointer-types"
        " -I" + getGcIncludeDir().string() +
        " -o " + binPath.string();
    for (auto& cf : cFiles) {
        compileCmd += " " + cf.string();
    }
    compileCmd += " " + (getGcLibDir() / "libgc.a").string();
    compileCmd += " -lm";
    compileCmd += " 2>&1";
    REQUIRE(std::system(compileCmd.c_str()) == 0);

    auto outPath = tmpDir / "stdout.txt";
    std::string runCmd = binPath.string() + " > " + outPath.string() + " 2>&1";
    REQUIRE(std::system(runCmd.c_str()) == 0);

    std::ifstream f(outPath);
    std::stringstream ss;
    ss << f.rdbuf();

    fs::remove_all(tmpDir);
    return ss.str();
}

// ============================================================================
// Basic tests
// ============================================================================

TEST_CASE("Hello world — WriteInt and WriteLn", "[ccodegen][basic]")
{
    std::string src = R"(module Hello
import Out;
fn init() -> void {
    Out.Int(42);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Hello", src, {"Out"}) == "42\n");
}

// ============================================================================
// Expression tests
// ============================================================================

TEST_CASE("Arithmetic expressions", "[ccodegen][expr]")
{
    std::string src = R"(module Arith
import Out;
fn init() -> void {
    Out.Int(2 + 3 * 4);
    Out.Ln();
    Out.Int(100 div 10);
    Out.Ln();
    Out.Int(17 mod 5);
    Out.Ln();
    Out.Int(10 - 3);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Arith", src, {"Out"}) == "14\n10\n2\n7\n");
}

TEST_CASE("Unary operators", "[ccodegen][expr]")
{
    std::string src = R"(module Unary
import Out;
fn init() -> void
var x: i64;
{
    x = 5;
    Out.Int(-x);
    Out.Ln();
    Out.Int(+x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Unary", src, {"Out"}) == "-5\n5\n");
}

// ============================================================================
// Variable and constant tests
// ============================================================================

TEST_CASE("Local variables and assignment", "[ccodegen][var]")
{
    std::string src = R"(module Vars
import Out;
fn init() -> void
var {
    a: i64;
    b: i64;
    c: i64;
}
{
    a = 10;
    b = 20;
    c = a + b;
    Out.Int(c);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Vars", src, {"Out"}) == "30\n");
}

TEST_CASE("Constants", "[ccodegen][const]")
{
    std::string src = R"(module Consts
import Out;
const {
    X = 100;
    Y = 200;
}
fn init() -> void {
    Out.Int(X + Y);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Consts", src, {"Out"}) == "300\n");
}

TEST_CASE("Global variables", "[ccodegen][var]")
{
    std::string src = R"(module Globals
import Out;
var g: i64;
fn init() -> void {
    g = 99;
    Out.Int(g);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Globals", src, {"Out"}) == "99\n");
}

// ============================================================================
// Control flow tests
// ============================================================================

TEST_CASE("If-else", "[ccodegen][control]")
{
    std::string src = R"(module IfElse
import Out;
fn printSign(x: i64) -> void {
    if x > 0 {
        Out.Int(1);
    } else if x == 0 {
        Out.Int(0);
    } else {
        Out.Int(-1);
    }
    Out.Ln();
}
fn init() -> void {
    printSign(5);
    printSign(0);
    printSign(-3);
}
)";
    REQUIRE(transpileCompileRun("IfElse", src, {"Out"}) == "1\n0\n-1\n");
}

TEST_CASE("While loop", "[ccodegen][control]")
{
    std::string src = R"(module WhileLoop
import Out;
fn init() -> void
var i: i64;
{
    i = 0;
    while i < 5 {
        Out.Int(i);
        Out.Ln();
        i = i + 1;
    }
}
)";
    REQUIRE(transpileCompileRun("WhileLoop", src, {"Out"}) == "0\n1\n2\n3\n4\n");
}

TEST_CASE("Do-while loop", "[ccodegen][control]")
{
    std::string src = R"(module DoWhile
import Out;
fn init() -> void
var i: i64;
{
    i = 1;
    do {
        Out.Int(i);
        Out.Ln();
        i = i * 2;
    } while i <= 16;
}
)";
    REQUIRE(transpileCompileRun("DoWhile", src, {"Out"}) == "1\n2\n4\n8\n16\n");
}

TEST_CASE("For loop", "[ccodegen][control]")
{
    std::string src = R"(module ForLoop
import Out;
fn init() -> void
var {
    sum: i64;
    i: i64;
}
{
    sum = 0;
    for (i = 1, 10, 1) {
        sum = sum + i;
    }
    Out.Int(sum);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ForLoop", src, {"Out"}) == "55\n");
}

TEST_CASE("For loop with step", "[ccodegen][control]")
{
    std::string src = R"(module ForStep
import Out;
fn init() -> void
var i: i64;
{
    for (i = 0, 20, 5) {
        Out.Int(i);
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("ForStep", src, {"Out"}) == "0\n5\n10\n15\n20\n");
}

TEST_CASE("Switch statement", "[ccodegen][control]")
{
    std::string src = R"(module Switch
import Out;
fn classify(x: i64) -> void {
    switch x {
        case 1:
            Out.Int(10);
        case 2:
            Out.Int(20);
        case 3:
            Out.Int(30);
    }
    Out.Ln();
}
fn init() -> void {
    classify(1);
    classify(2);
    classify(3);
}
)";
    REQUIRE(transpileCompileRun("Switch", src, {"Out"}) == "10\n20\n30\n");
}

TEST_CASE("Switch with range", "[ccodegen][control]")
{
    std::string src = R"(module SwitchRange
import Out;
fn init() -> void
var x: i64;
{
    x = 7;
    switch x {
        case 1..5:
            Out.Int(1);
        case 6..10:
            Out.Int(2);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SwitchRange", src, {"Out"}) == "2\n");
}

// ============================================================================
// Procedure and function tests
// ============================================================================

TEST_CASE("Function call and return value", "[ccodegen][proc]")
{
    std::string src = R"(module FuncCall
import Out;
fn add(a, b: i64) -> i64 {
    return a + b;
}
fn init() -> void {
    Out.Int(add(3, 7));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FuncCall", src, {"Out"}) == "10\n");
}

TEST_CASE("Multiple functions with local vars", "[ccodegen][proc]")
{
    std::string src = R"(module MultiFn
import Out;
fn square(x: i64) -> i64
var result: i64;
{
    result = x * x;
    return result;
}
fn cube(x: i64) -> i64 {
    return x * square(x);
}
fn init() -> void {
    Out.Int(square(5));
    Out.Ln();
    Out.Int(cube(3));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("MultiFn", src, {"Out"}) == "25\n27\n");
}

TEST_CASE("Recursion — factorial", "[ccodegen][proc]")
{
    std::string src = R"(module Fact
import Out;
fn factorial(n: i64) -> i64
var result: i64;
{
    if n <= 1 {
        result = 1;
    } else {
        result = n * factorial(n - 1);
    }
    return result;
}
fn init() -> void {
    Out.Int(factorial(10));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Fact", src, {"Out"}) == "3628800\n");
}

TEST_CASE("Recursion — fibonacci", "[ccodegen][proc]")
{
    std::string src = R"(module Fib
import Out;
fn fib(n: i64) -> i64
var result: i64;
{
    if n <= 1 {
        result = n;
    } else {
        result = fib(n - 1) + fib(n - 2);
    }
    return result;
}
fn init() -> void {
    Out.Int(fib(10));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Fib", src, {"Out"}) == "55\n");
}

// ============================================================================
// Reference parameter tests
// ============================================================================

TEST_CASE("Reference parameters", "[ccodegen][ref]")
{
    std::string src = R"(module RefParam
import Out;
fn swap(a, b: &i64) -> void
var tmp: i64;
{
    tmp = a;
    a = b;
    b = tmp;
}
fn init() -> void
var {
    x: i64;
    y: i64;
}
{
    x = 10;
    y = 20;
    swap(x, y);
    Out.Int(x);
    Out.Ln();
    Out.Int(y);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("RefParam", src, {"Out"}) == "20\n10\n");
}

TEST_CASE("Reference parameter pass-through", "[ccodegen][ref]")
{
    std::string src = R"(module RefPass
import Out;
fn increment(x: &i64) -> void {
    x = x + 1;
}
fn doubleIncrement(x: &i64) -> void {
    increment(x);
    increment(x);
}
fn init() -> void
var n: i64;
{
    n = 0;
    doubleIncrement(n);
    Out.Int(n);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("RefPass", src, {"Out"}) == "2\n");
}

// ============================================================================
// Array tests
// ============================================================================

TEST_CASE("Array — basic indexing", "[ccodegen][array]")
{
    std::string src = R"(module ArrBasic
import Out;
fn init() -> void
var {
    arr: i64[5];
    i: i64;
}
{
    for (i = 0, 4, 1) {
        arr[i] = i * i;
    }
    for (i = 0, 4, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("ArrBasic", src, {"Out"}) == "0\n1\n4\n9\n16\n");
}

TEST_CASE("Array — sum elements", "[ccodegen][array]")
{
    std::string src = R"(module ArrSum
import Out;
fn init() -> void
var {
    arr: i64[5];
    sum: i64;
    i: i64;
}
{
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;
    sum = 0;
    for (i = 0, 4, 1) {
        sum = sum + arr[i];
    }
    Out.Int(sum);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ArrSum", src, {"Out"}) == "15\n");
}

TEST_CASE("Boolean expressions", "[ccodegen][expr]")
{
    std::string src = R"(module BoolExpr
import Out;
fn init() -> void
var {
    a: i64;
    b: i64;
    p: bool;
}
{
    a = 5;
    b = 10;
    p = (a < b) && (b > 0);
    if p {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
    p = (a > b) || (a == 5);
    if p {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
    if !(a == b) {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("BoolExpr", src, {"Out"}) == "1\n1\n1\n");
}

TEST_CASE("Nested control flow", "[ccodegen][control]")
{
    std::string src = R"(module Nested
import Out;
fn init() -> void
var {
    i: i64;
    j: i64;
    count: i64;
}
{
    count = 0;
    for (i = 1, 5, 1) {
        for (j = 1, 5, 1) {
            if i == j {
                count = count + 1;
            }
        }
    }
    Out.Int(count);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("Nested", src, {"Out"}) == "5\n");
}

TEST_CASE("Global variable modified by function", "[ccodegen][var]")
{
    std::string src = R"(module GlobFn
import Out;
var counter: i64;
fn bump() -> void {
    counter = counter + 1;
}
fn init() -> void {
    counter = 0;
    bump();
    bump();
    bump();
    Out.Int(counter);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("GlobFn", src, {"Out"}) == "3\n");
}

TEST_CASE("Multiple constants and globals together", "[ccodegen][var]")
{
    std::string src = R"(module ConstGlob
import Out;
const {
    OFFSET = 100;
    SCALE = 3;
}
var total: i64;
fn init() -> void {
    total = OFFSET + SCALE * 10;
    Out.Int(total);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ConstGlob", src, {"Out"}) == "130\n");
}

TEST_CASE("Integer division (div)", "[ccodegen][expr]")
{
    std::string src = R"(module IntDiv
import Out;
fn init() -> void {
    Out.Int(17 div 5);
    Out.Ln();
    Out.Int(100 div 3);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("IntDiv", src, {"Out"}) == "3\n33\n");
}

TEST_CASE("Complex expression with parentheses", "[ccodegen][expr]")
{
    std::string src = R"(module ComplexExpr
import Out;
fn init() -> void {
    Out.Int((2 + 3) * (4 + 1));
    Out.Ln();
    Out.Int((100 - 50) div (5 + 5));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ComplexExpr", src, {"Out"}) == "25\n5\n");
}

TEST_CASE("Exported function", "[ccodegen][proc]")
{
    std::string src = R"(module ExportFn
import Out;
fn export compute(x: i64) -> i64 {
    return x * x + 1;
}
fn init() -> void {
    Out.Int(compute(7));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ExportFn", src, {"Out"}) == "50\n");
}

TEST_CASE("Abs via if-else (no early return)", "[ccodegen][proc]")
{
    std::string src = R"(module AbsFn
import Out;
fn abs(x: i64) -> i64
var result: i64;
{
    if x < 0 {
        result = -x;
    } else {
        result = x;
    }
    return result;
}
fn init() -> void {
    Out.Int(abs(-42));
    Out.Ln();
    Out.Int(abs(17));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("AbsFn", src, {"Out"}) == "42\n17\n");
}

TEST_CASE("While-elif", "[ccodegen][control]")
{
    std::string src = R"(module WhileElif
import Out;
fn init() -> void
var {
    a: i64;
    b: i64;
}
{
    a = 0;
    b = 0;
    while a < 3 {
        a = a + 1;
    } elif b < 3 {
        b = b + 1;
    }
    Out.Int(a);
    Out.Ln();
    Out.Int(b);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("WhileElif", src, {"Out"}) == "3\n3\n");
}

// ============================================================================
// Integration tests
// ============================================================================

TEST_CASE("Bubble sort with arrays and refs", "[ccodegen][integration]")
{
    std::string src = R"(module BubbleSort
import Out;
fn swap(a, b: &i64) -> void
var tmp: i64;
{
    tmp = a;
    a = b;
    b = tmp;
}
fn init() -> void
var {
    arr: i64[5];
    n: i64;
    i: i64;
    j: i64;
}
{
    arr[0] = 5;
    arr[1] = 3;
    arr[2] = 1;
    arr[3] = 4;
    arr[4] = 2;
    n = 5;
    for (i = 0, 3, 1) {
        for (j = 0, 3 - i, 1) {
            if arr[j] > arr[j + 1] {
                swap(arr[j], arr[j + 1]);
            }
        }
    }
    for (i = 0, 4, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("BubbleSort", src, {"Out"}) == "1\n2\n3\n4\n5\n");
}

TEST_CASE("GCD — Euclidean algorithm", "[ccodegen][integration]")
{
    std::string src = R"(module GCD
import Out;
fn gcd(a, b: i64) -> i64 {
    while b != 0 {
        a = a mod b;
        swap(a, b);
    }
    return a;
}
fn swap(a, b: &i64) -> void
var t: i64;
{
    t = a;
    a = b;
    b = t;
}
fn init() -> void {
    Out.Int(gcd(48, 18));
    Out.Ln();
    Out.Int(gcd(100, 75));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("GCD", src, {"Out"}) == "6\n25\n");
}

TEST_CASE("For counter visible after loop", "[ccodegen][control]")
{
    std::string src = R"(module ForAfter
import Out;
fn init() -> void
var {
    sum: i64;
    i: i64;
}
{
    sum = 0;
    for (i = 1, 10, 1) {
        sum = sum + i;
    }
    i = i + 2;
    sum = sum + i;
    Out.Int(sum);
    Out.Ln();
    Out.Int(i);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ForAfter", src, {"Out"}) == "68\n13\n");
}

// ============================================================================
// Multi-module tests
// ============================================================================

TEST_CASE("Multi-module — import custom library", "[ccodegen][multimodule]")
{
    ModuleSource mathLib = {"MathLib", R"(module MathLib

fn export Square(x: i64) -> i64 {
    return x * x;
}

fn export Cube(x: i64) -> i64 {
    return x * Square(x);
}

fn export Add(a, b: i64) -> i64 {
    return a + b;
}

fn init() -> void {
}
)"};

    ModuleSource main = {"Main", R"(module Main

import MathLib, Out;

fn init() -> void {
    Out.Int(MathLib.Square(5));
    Out.Ln();
    Out.Int(MathLib.Cube(3));
    Out.Ln();
    Out.Int(MathLib.Add(10, 20));
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({mathLib}, main) == "25\n27\n30\n");
}

TEST_CASE("Multi-module — library with global state", "[ccodegen][multimodule]")
{
    ModuleSource counter = {"Counter", R"(module Counter

var count: i64;

fn export Reset() -> void {
    count = 0;
}

fn export Increment() -> void {
    count = count + 1;
}

fn export Get() -> i64 {
    return count;
}

fn init() -> void {
    count = 0;
}
)"};

    ModuleSource main = {"UseCounter", R"(module UseCounter

import Counter, Out;

fn init() -> void {
    Counter.Reset();
    Counter.Increment();
    Counter.Increment();
    Counter.Increment();
    Out.Int(Counter.Get());
    Out.Ln();
    Counter.Increment();
    Counter.Increment();
    Out.Int(Counter.Get());
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({counter}, main) == "3\n5\n");
}

TEST_CASE("Multi-module — two libraries", "[ccodegen][multimodule]")
{
    ModuleSource mathLib = {"MyMath", R"(module MyMath

fn export Double(x: i64) -> i64 {
    return x + x;
}

fn init() -> void {
}
)"};

    ModuleSource strLib = {"Fmt", R"(module Fmt

import Out;

fn export PrintLabeled(label: i64, value: i64) -> void {
    Out.Int(label);
    Out.Int(value);
    Out.Ln();
}

fn init() -> void {
}
)"};

    ModuleSource main = {"App", R"(module App

import MyMath, Fmt, Out;

fn init() -> void
var x: i64;
{
    x = MyMath.Double(21);
    Fmt.PrintLabeled(0, x);
    x = MyMath.Double(x);
    Fmt.PrintLabeled(1, x);
}
)"};

    REQUIRE(multiModuleRun({mathLib, strLib}, main) == "042\n184\n");
}

// ============================================================================
// RTTI tests
// ============================================================================

TEST_CASE("RTTI — struct fields and inheritance", "[ccodegen][rtti]")
{
    std::string src = R"(module StructFields
import Out;

type {
    Point: struct {
        x, y: i64;
    };
    Point3D: struct (Point) {
        z: i64;
    };
}

fn init() -> void
var {
    p: Point;
    p3: Point3D;
}
{
    p.x = 10;
    p.y = 20;
    Out.Int(p.x + p.y);
    Out.Ln();
    p3.x = 1;
    p3.y = 2;
    p3.z = 3;
    Out.Int(p3.x + p3.y + p3.z);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("StructFields", src, {"Out"}) == "30\n6\n");
}

TEST_CASE("RTTI — is operator", "[ccodegen][rtti]")
{
    std::string src = R"(module IsOp
import Out;

type {
    Shape: struct {
        id: i64;
    };
    Circle: struct (Shape) {
        radius: i64;
    };
    Rect: struct (Shape) {
        w, h: i64;
    };
}

fn Identify(s: &Shape) -> i64
var result: i64;
{
    result = 0;
    if s is Rect {
        result = 2;
    }
    if s is Circle {
        result = 1;
    }
    return result;
}

fn init() -> void
var {
    c: Circle;
    r: Rect;
    s: Shape;
}
{
    c.id = 0;
    c.radius = 5;
    Out.Int(Identify(c));
    Out.Ln();
    r.id = 0;
    r.w = 3;
    r.h = 4;
    Out.Int(Identify(r));
    Out.Ln();
    s.id = 0;
    Out.Int(Identify(s));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("IsOp", src, {"Out"}) == "1\n2\n0\n");
}

TEST_CASE("RTTI — type guard with field access", "[ccodegen][rtti]")
{
    std::string src = R"(module TypeGuardAccess
import Out;

type {
    Animal: struct {
        legs: i64;
    };
    Dog: struct (Animal) {
        barkVolume: i64;
    };
    Cat: struct (Animal) {
        purring: i64;
    };
}

fn Describe(a: &Animal) -> void {
    Out.Int(a.legs);
    if a is Dog {
        Out.Int(a(Dog).barkVolume);
    }
    if a is Cat {
        Out.Int(a(Cat).purring);
    }
    Out.Ln();
}

fn init() -> void
var {
    d: Dog;
    c: Cat;
}
{
    d.legs = 4;
    d.barkVolume = 90;
    Describe(d);
    c.legs = 4;
    c.purring = 1;
    Describe(c);
}
)";
    REQUIRE(transpileCompileRun("TypeGuardAccess", src, {"Out"}) == "490\n41\n");
}

TEST_CASE("RTTI — three-level inheritance chain", "[ccodegen][rtti]")
{
    std::string src = R"(module ThreeLevel
import Out;

type {
    Base: struct {
        a: i64;
    };
    Mid: struct (Base) {
        b: i64;
    };
    Leaf: struct (Mid) {
        c: i64;
    };
}

fn Level(x: &Base) -> i64
var result: i64;
{
    if x is Leaf {
        result = 3;
    } else if x is Mid {
        result = 2;
    } else {
        result = 1;
    }
    return result;
}

fn init() -> void
var {
    b: Base;
    m: Mid;
    l: Leaf;
}
{
    b.a = 0;
    m.a = 0;
    m.b = 0;
    l.a = 0;
    l.b = 0;
    l.c = 0;
    Out.Int(Level(b));
    Out.Ln();
    Out.Int(Level(m));
    Out.Ln();
    Out.Int(Level(l));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ThreeLevel", src, {"Out"}) == "1\n2\n3\n");
}

TEST_CASE("RTTI — type guard reading inherited fields", "[ccodegen][rtti]")
{
    std::string src = R"(module InheritedFields
import Out;

type {
    Vehicle: struct {
        speed: i64;
    };
    Car: struct (Vehicle) {
        doors: i64;
    };
}

fn PrintSpeed(v: &Vehicle) -> void {
    Out.Int(v.speed);
    if v is Car {
        Out.Int(v(Car).doors);
    }
    Out.Ln();
}

fn init() -> void
var {
    c: Car;
    v: Vehicle;
}
{
    c.speed = 120;
    c.doors = 4;
    PrintSpeed(c);
    v.speed = 60;
    PrintSpeed(v);
}
)";
    REQUIRE(transpileCompileRun("InheritedFields", src, {"Out"}) == "1204\n60\n");
}

TEST_CASE("RTTI — descriptors initialized in arrays and nested structs", "[ccodegen][rtti][array]")
{
    std::string src = R"(module DescriptorArrays
import Out;

type {
    Base: struct {
        value: i64;
    };
    Derived: struct (Base) {
        extra: i64;
    };
    Other: struct (Base) {
        left, right: i64;
    };
    Wrapper: struct (Base) {
        item: Base;
        width: i64;
    };
    ArrayWrapper: struct (Wrapper) {
        items: Base[4];
        count: i64;
    };

    DeepWrapper: struct (ArrayWrapper) {
        index: i64;
    };
    DerivedBox: struct {
        single: Derived;
        many: Derived[2];
    };
}

fn Identify(value: &Base) -> i64
var result: i64;
{
    result = 0;
    if value is Other {
        result = 2;
    }
    if value is Derived {
        result = 1;
    }
    return result;
}

fn init() -> void
var {
    derived: Derived;
    other: Other;
    base: Base;
    wrapper: Wrapper;
    arrayWrapper: ArrayWrapper;
    deepWrapper: DeepWrapper;
    bases: Base[3];
    derivedItems: Derived[2];
    box: DerivedBox;
}
{
    derived.value = 0;
    derived.extra = 5;
    Out.Int(Identify(derived));
    Out.Ln();
    other.value = 0;
    other.left = 3;
    other.right = 4;
    Out.Int(Identify(other));
    Out.Ln();
    base.value = 0;
    Out.Int(Identify(base));
    Out.Ln();

    Out.Int(Identify(bases[0]));
    Out.Ln();
    Out.Int(Identify(derivedItems[0]));
    Out.Ln();
    Out.Int(Identify(wrapper.item));
    Out.Ln();
    Out.Int(Identify(arrayWrapper.items[0]));
    Out.Ln();
    Out.Int(Identify(deepWrapper.items[0]));
    Out.Ln();
    Out.Int(Identify(box.single));
    Out.Ln();
    Out.Int(Identify(box.many[1]));
    Out.Ln();

    wrapper.item = base;
    arrayWrapper.items[1] = base;
    bases[2] = base;

    Out.Int(Identify(bases[1]));
    Out.Ln();
    Out.Int(Identify(bases[2]));
    Out.Ln();
    Out.Int(Identify(arrayWrapper.items[1]));
    Out.Ln();
    Out.Int(Identify(deepWrapper.items[2]));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("DescriptorArrays", src, {"Out"}) ==
        "1\n2\n0\n0\n1\n0\n0\n0\n1\n1\n0\n0\n0\n0\n");
}

// ============================================================================
// Output tests
// ============================================================================

TEST_CASE("Output string type support", "[ccodegen][output]")
{
    std::string src = R"(module OutputString
import Out;
fn init() -> void 
var {
    s: char[14];
}
{
    s = "Hello, World!";
    Out.String(s);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OutputString", src, {"Out"}) == "Hello, World!\n");
}

TEST_CASE("Output string literal passes hidden length", "[ccodegen][output]")
{
    std::string src = R"(module OutputStringLiteral
import Out;
fn init() -> void {
    Out.String("abc123");
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OutputStringLiteral", src, {"Out"}) == "abc123\n");
}

TEST_CASE("Char array comparison uses strcmp", "[ccodegen][output]")
{
    std::string src = R"(module CharArrayCompare
import Out;
fn IsText(s: char[]) -> bool {
    return s == "text";
}
fn init() -> void
var {
    s: char[8];
}
{
    s = "text";
    if IsText(s) {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("CharArrayCompare", src, {"Out"}) == "1\n");
}

// ============================================================================
// Open array tests
// ============================================================================

TEST_CASE("Open array — sum with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OABasic
import Out;
fn sumArray(arr: i64[]) -> i64
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i];
    }
    return s;
}
fn init() -> void
var {
    a: i64[5];
}
{
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    a[4] = 5;
    Out.Int(sumArray(a));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OABasic", src, {"Out"}) == "15\n");
}

TEST_CASE("Open array — print all elements via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OARead
import Out;
fn printArray(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}
fn init() -> void
var {
    a: i64[3];
}
{
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    printArray(a);
}
)";
    REQUIRE(transpileCompileRun("OARead", src, {"Out"}) == "10\n20\n30\n");
}

TEST_CASE("Open array — multiple open array params with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OAMulti
import Out;
fn sumTwo(a: i64[], b: i64[]) -> i64
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(a) - 1, 1) {
        s = s + a[i];
    }
    for (i = 0, len(b) - 1, 1) {
        s = s + b[i];
    }
    return s;
}
fn init() -> void
var {
    x: i64[3];
    y: i64[2];
}
{
    x[0] = 1;
    x[1] = 2;
    x[2] = 3;
    y[0] = 10;
    y[1] = 20;
    Out.Int(sumTwo(x, y));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OAMulti", src, {"Out"}) == "36\n");
}

TEST_CASE("Open array — open array with value param and len", "[ccodegen][openarray]")
{
    std::string src = R"(module OAMixed
import Out;
fn scaleAndSum(arr: i64[], factor: i64) -> i64
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i] * factor;
    }
    return s;
}
fn init() -> void
var {
    a: i64[3];
}
{
    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    Out.Int(scaleAndSum(a, 10));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OAMixed", src, {"Out"}) == "60\n");
}

TEST_CASE("Open array — exported function with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OAExport
import Out;
fn export maxArr(arr: i64[]) -> i64
var {
    m: i64;
    i: i64;
}
{
    m = arr[0];
    for (i = 1, len(arr) - 1, 1) {
        if arr[i] > m {
            m = arr[i];
        }
    }
    return m;
}
fn init() -> void
var {
    a: i64[4];
}
{
    a[0] = 3;
    a[1] = 7;
    a[2] = 2;
    a[3] = 9;
    Out.Int(maxArr(a));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OAExport", src, {"Out"}) == "9\n");
}

TEST_CASE("Open array — global array with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OAGlobal
import Out;
var g: i64[4];
fn printSum(arr: i64[]) -> void
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i];
    }
    Out.Int(s);
    Out.Ln();
}
fn init() -> void {
    g[0] = 100;
    g[1] = 200;
    g[2] = 300;
    g[3] = 400;
    printSum(g);
}
)";
    REQUIRE(transpileCompileRun("OAGlobal", src, {"Out"}) == "1000\n");
}

TEST_CASE("Open array — single element array", "[ccodegen][openarray]")
{
    std::string src = R"(module OASingle
import Out;
fn getFirst(arr: i64[]) -> i64 {
    return arr[0];
}
fn init() -> void
var {
    a: i64[1];
}
{
    a[0] = 42;
    Out.Int(getFirst(a));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OASingle", src, {"Out"}) == "42\n");
}

TEST_CASE("Open array — different sizes to same function with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OASizes
import Out;
fn sum(arr: i64[]) -> i64
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i];
    }
    return s;
}
fn init() -> void
var {
    small: i64[2];
    medium: i64[4];
    big: i64[6];
}
{
    small[0] = 1;
    small[1] = 2;
    medium[0] = 10;
    medium[1] = 20;
    medium[2] = 30;
    medium[3] = 40;
    big[0] = 1;
    big[1] = 1;
    big[2] = 1;
    big[3] = 1;
    big[4] = 1;
    big[5] = 1;
    Out.Int(sum(small));
    Out.Ln();
    Out.Int(sum(medium));
    Out.Ln();
    Out.Int(sum(big));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OASizes", src, {"Out"}) == "3\n100\n6\n");
}

TEST_CASE("Open array — search with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OASearch
import Out;
fn linearSearch(arr: i64[], target: i64) -> i64
var {
    i: i64;
    result: i64;
}
{
    result = -1;
    for (i = 0, len(arr) - 1, 1) {
        if arr[i] == target {
            result = i;
        }
    }
    return result;
}
fn init() -> void
var {
    a: i64[5];
}
{
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;
    a[4] = 50;
    Out.Int(linearSearch(a, 30));
    Out.Ln();
    Out.Int(linearSearch(a, 99));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OASearch", src, {"Out"}) == "2\n-1\n");
}

TEST_CASE("Open array — pass-through to another open array function", "[ccodegen][openarray]")
{
    std::string src = R"(module OAPassThru
import Out;
fn sumArr(arr: i64[]) -> i64
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i];
    }
    return s;
}
fn doubleSum(arr: i64[]) -> i64 {
    return sumArr(arr) * 2;
}
fn init() -> void
var {
    a: i64[3];
}
{
    a[0] = 5;
    a[1] = 10;
    a[2] = 15;
    Out.Int(doubleSum(a));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("OAPassThru", src, {"Out"}) == "60\n");
}

TEST_CASE("Open array — 2D all dimensions via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA2D
import Out;
fn printDims(m: i64[][]) -> void {
    Out.Int(len(m));
    Out.Ln();
    Out.Int(len(m[0]));
    Out.Ln();
}
fn init() -> void
var {
    m: i64[3][4];
}
{
    printDims(m);
}
)";
    REQUIRE(transpileCompileRun("OA2D", src, {"Out"}) == "3\n4\n");
}

TEST_CASE("Open array — 3D all dimensions via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA3D
import Out;
fn printDims(m: i64[][][]) -> void {
    Out.Int(len(m));
    Out.Ln();
    Out.Int(len(m[0]));
    Out.Ln();
    Out.Int(len(m[0][0]));
    Out.Ln();
}
fn init() -> void
var {
    m: i64[3][4][5];
}
{
    printDims(m);
}
)";
    REQUIRE(transpileCompileRun("OA3D", src, {"Out"}) == "3\n4\n5\n");
}

TEST_CASE("Open array — 2D pass-through with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA2DPass
import Out;
fn innerLen(m: i64[][]) -> void {
    Out.Int(len(m));
    Out.Ln();
    Out.Int(len(m[0]));
    Out.Ln();
}
fn wrapper(m: i64[][]) -> void {
    innerLen(m);
}
fn init() -> void
var {
    m: i64[5][3];
}
{
    wrapper(m);
}
)";
    REQUIRE(transpileCompileRun("OA2DPass", src, {"Out"}) == "5\n3\n");
}

TEST_CASE("Open array — multi-module with len", "[ccodegen][openarray][multimodule]")
{
    ModuleSource arrLib = {"ArrLib", R"(module ArrLib

import Out;

fn export PrintArray(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}

fn export PrintSum(arr: i64[]) -> void
var {
    s: i64;
    i: i64;
}
{
    s = 0;
    for (i = 0, len(arr) - 1, 1) {
        s = s + arr[i];
    }
    Out.Int(s);
    Out.Ln();
}

fn init() -> void {
}
)"};

    ModuleSource main = {"OAMain", R"(module OAMain

import ArrLib, Out;

fn init() -> void
var {
    a: i64[5];
}
{
    a[0] = 2;
    a[1] = 4;
    a[2] = 6;
    a[3] = 8;
    a[4] = 10;
    ArrLib.PrintArray(a);
    ArrLib.PrintSum(a);
}
)"};

    REQUIRE(multiModuleRun({arrLib}, main) == "2\n4\n6\n8\n10\n30\n");
}

// ============================================================================
// Builtin function tests
// ============================================================================

TEST_CASE("Builtin len — fixed array", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenFixed
import Out;
fn init() -> void
var {
    a: i64[5];
    b: i64[10];
}
{
    Out.Int(len(a));
    Out.Ln();
    Out.Int(len(b));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("LenFixed", src, {"Out"}) == "5\n10\n");
}

TEST_CASE("Builtin len — open array parameter", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenOpen
import Out;
fn printLen(arr: i64[]) -> void {
    Out.Int(len(arr));
    Out.Ln();
}
fn init() -> void
var {
    a: i64[7];
    b: i64[3];
}
{
    printLen(a);
    printLen(b);
}
)";
    REQUIRE(transpileCompileRun("LenOpen", src, {"Out"}) == "7\n3\n");
}

TEST_CASE("Builtin len — iterate open array with len", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenIter
import Out;
fn printAll(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}
fn init() -> void
var {
    a: i64[4];
}
{
    a[0] = 10;
    a[1] = 20;
    a[2] = 30;
    a[3] = 40;
    printAll(a);
}
)";
    REQUIRE(transpileCompileRun("LenIter", src, {"Out"}) == "10\n20\n30\n40\n");
}

TEST_CASE("Builtin len — char array (string)", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenStr
import Out;
fn init() -> void
var {
    s: char[6];
}
{
    s = "Hello";
    Out.Int(len(s));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("LenStr", src, {"Out"}) == "6\n");
}

TEST_CASE("Builtin new — allocate and use struct pointer", "[ccodegen][builtin][new]")
{
    std::string src = R"(module NewBasic
import Out;
type {
    Node: struct {
        value: i64;
    };
}
fn init() -> void
var {
    p: *Node;
}
{
    new(p);
    p.value = 42;
    Out.Int(p.value);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("NewBasic", src, {"Out"}) == "42\n");
}

TEST_CASE("Builtin new — multiple allocations", "[ccodegen][builtin][new]")
{
    std::string src = R"(module NewMulti
import Out;
type {
    Pair: struct {
        x, y: i64;
    };
}
fn init() -> void
var {
    a: *Pair;
    b: *Pair;
}
{
    new(a);
    new(b);
    a.x = 1;
    a.y = 2;
    b.x = 10;
    b.y = 20;
    Out.Int(a.x + a.y);
    Out.Ln();
    Out.Int(b.x + b.y);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("NewMulti", src, {"Out"}) == "3\n30\n");
}

TEST_CASE("Builtin new — pointer to nil check", "[ccodegen][builtin][new]")
{
    std::string src = R"(module NewNil
import Out;
type {
    Item: struct {
        val: i64;
    };
}
fn init() -> void
var {
    p: *Item;
}
{
    new(p);
    p.val = 99;
    Out.Int(p.val);
    Out.Ln();
    p = nil;
    if p == nil {
        Out.Int(0);
    } else {
        Out.Int(1);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("NewNil", src, {"Out"}) == "99\n0\n");
}

TEST_CASE("Builtin new — calloc zeroes fields", "[ccodegen][builtin][new]")
{
    std::string src = R"(module NewZero
import Out;
type {
    Data: struct {
        a, b, c: i64;
    };
}
fn init() -> void
var {
    p: *Data;
}
{
    new(p);
    Out.Int(p.a);
    Out.Ln();
    Out.Int(p.b);
    Out.Ln();
    Out.Int(p.c);
    Out.Ln();
    p.a = 100;
    Out.Int(p.a);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("NewZero", src, {"Out"}) == "0\n0\n0\n100\n");
}

TEST_CASE("Builtin assert — passing assertion", "[ccodegen][builtin][assert]")
{
    std::string src = R"(module AssertPass
import Out;
fn init() -> void
var x: i64;
{
    x = 42;
    assert(x == 42);
    assert(x > 0);
    assert(x < 100);
    Out.Int(x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("AssertPass", src, {"Out"}) == "42\n");
}

TEST_CASE("Builtin assert — with boolean expression", "[ccodegen][builtin][assert]")
{
    std::string src = R"(module AssertBool
import Out;
fn init() -> void
var {
    a: i64;
    b: i64;
}
{
    a = 10;
    b = 20;
    assert(a + b == 30);
    assert(a != b);
    assert((a < b) && (b > 0));
    Out.Int(a + b);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("AssertBool", src, {"Out"}) == "30\n");
}

TEST_CASE("Builtin assert — with string message", "[ccodegen][builtin][assert]")
{
    std::string src = R"(module AssertMsg
import Out;
fn init() -> void
var {
    x: i64;
}
{
    x = 10;
    assert(x > 0, "x must be positive");
    assert(x == 10, "x must equal 10");
    Out.Int(x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("AssertMsg", src, {"Out"}) == "10\n");
}

TEST_CASE("Builtin int — float to integer conversion", "[ccodegen][builtin][int]")
{
    std::string src = R"(module BuiltinInt
import Out;
fn init() -> void
var {
    a: i64;
    b: f64;
}
{
    b = 3.7;
    a = int(b);
    Out.Int(a);
    Out.Ln();
    Out.Int(int(-2.9));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("BuiltinInt", src, {"Out"}) == "3\n-2\n");
}

TEST_CASE("Builtin float — integer to float conversion", "[ccodegen][builtin][float]")
{
    std::string src = R"(module BuiltinFloat
import Out;
fn init() -> void
var {
    x: i64;
    y: f64;
}
{
    x = 42;
    y = float(x);
    Out.Real(y, 1);
    Out.Ln();
    Out.Real(float(5) / float(2), 1);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("BuiltinFloat", src, {"Out"}) == "42.0\n2.5\n");
}

TEST_CASE("Builtin ord — bool char and set to integer", "[ccodegen][builtin][ord]")
{
    std::string src = R"(module BuiltinOrd
import Out;
fn init() -> void
var {
    s: set;
}
{
    s = {0, 2, 5};
    Out.Int(ord(true));
    Out.Ln();
    Out.Int(ord("A"));
    Out.Ln();
    Out.Int(ord(s));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("BuiltinOrd", src, {"Out"}) == "1\n65\n37\n");
}

TEST_CASE("Builtin chr — integer to char conversion", "[ccodegen][builtin][chr]")
{
    std::string src = R"(module BuiltinChr
import Out;
fn init() -> void
var c: char;
{
    c = chr(65);
    Out.Char(c);
    Out.Ln();
    c = chr(97);
    Out.Char(c);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("BuiltinChr", src, {"Out"}) == "A\na\n");
}

// ============================================================================
// Forward declaration tests
// ============================================================================

TEST_CASE("Forward decl — type alias before struct definition", "[ccodegen][fwddecl]")
{
    std::string src = R"(module FwdAlias
import Out;
type {
    BaseAlias: Base;
    Base: struct {
        x: i64;
    };
}
fn init() -> void
var {
    b: Base;
}
{
    b.x = 42;
    Out.Int(b.x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FwdAlias", src, {"Out"}) == "42\n");
}

TEST_CASE("Forward decl — pointer to later-declared struct", "[ccodegen][fwddecl]")
{
    std::string src = R"(module FwdPtr
import Out;
type {
    Container: struct {
        item: *Item;
    };
    Item: struct {
        value: i64;
    };
}
fn init() -> void
var {
    c: Container;
    it: Item;
}
{
    it.value = 77;
    c.item = nil;
    Out.Int(it.value);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FwdPtr", src, {"Out"}) == "77\n");
}

TEST_CASE("Forward decl — struct with inheritance using alias", "[ccodegen][fwddecl]")
{
    std::string src = R"(module FwdInherit
import Out;
type {
    Animal: struct {
        legs: i64;
    };
    Dog: struct (Animal) {
        bark: i64;
    };
}
fn init() -> void
var {
    d: Dog;
}
{
    d.legs = 4;
    d.bark = 1;
    Out.Int(d.legs);
    Out.Ln();
    Out.Int(d.bark);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FwdInherit", src, {"Out"}) == "4\n1\n");
}

// ============================================================================
// Linked list tests
// ============================================================================

TEST_CASE("Linked list — build and traverse", "[ccodegen][linkedlist]")
{
    std::string src = R"(module LList
import Out;
type {
    Node: struct {
        value: i64;
        next: *Node;
    };
}
fn init() -> void
var {
    head: *Node;
    cur: *Node;
    i: i64;
    sum: i64;
}
{
    head = nil;
    for (i = 1, 5, 1) {
        new(cur);
        cur.value = i * 10;
        cur.next = head;
        head = cur;
    }
    sum = 0;
    cur = head;
    while cur != nil {
        sum = sum + cur.value;
        cur = cur.next;
    }
    Out.Int(sum);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("LList", src, {"Out"}) == "150\n");
}

TEST_CASE("Linked list — count and last element", "[ccodegen][linkedlist]")
{
    std::string src = R"(module LListOps
import Out;
type {
    Node: struct {
        value: i64;
        next: *Node;
    };
}
fn count(head: *Node) -> i64
var {
    n: i64;
    cur: *Node;
}
{
    n = 0;
    cur = head;
    while cur != nil {
        n = n + 1;
        cur = cur.next;
    }
    return n;
}
fn last(head: *Node) -> i64
var {
    cur: *Node;
    result: i64;
}
{
    cur = head;
    result = 0;
    while cur != nil {
        result = cur.value;
        cur = cur.next;
    }
    return result;
}
fn init() -> void
var {
    head: *Node;
    cur: *Node;
    i: i64;
}
{
    head = nil;
    for (i = 1, 4, 1) {
        new(cur);
        cur.value = i;
        cur.next = head;
        head = cur;
    }
    Out.Int(count(head));
    Out.Ln();
    Out.Int(last(head));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("LListOps", src, {"Out"}) == "4\n1\n");
}

// ============================================================================
// Function type tests
// ============================================================================

TEST_CASE("Function type — basic call through variable", "[ccodegen][functype]")
{
    std::string src = R"(module FnBasic
import Out;
type {
    IntFn: (x: i64) -> i64;
}
fn double(x: i64) -> i64 {
    return x * 2;
}
fn init() -> void
var {
    f: IntFn;
}
{
    f = double;
    Out.Int(f(5));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FnBasic", src, {"Out"}) == "10\n");
}

TEST_CASE("Function type — swap function at runtime", "[ccodegen][functype]")
{
    std::string src = R"(module FnSwap
import Out;
type {
    Op: (a, b: i64) -> i64;
}
fn add(a, b: i64) -> i64 {
    return a + b;
}
fn mul(a, b: i64) -> i64 {
    return a * b;
}
fn apply(op: Op, x, y: i64) -> i64 {
    return op(x, y);
}
fn init() -> void
var {
    f: Op;
}
{
    f = add;
    Out.Int(apply(f, 3, 4));
    Out.Ln();
    f = mul;
    Out.Int(apply(f, 3, 4));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FnSwap", src, {"Out"}) == "7\n12\n");
}

TEST_CASE("Function type — function returning function", "[ccodegen][functype]")
{
    std::string src = R"(module FnRet
import Out;
type {
    IntToInt: (x: i64) -> i64;
    Factory: () -> IntToInt;
}
fn double(x: i64) -> i64 {
    return x * 2;
}
fn triple(x: i64) -> i64 {
    return x * 3;
}
fn getDoubler() -> IntToInt {
    return double;
}
fn getTripler() -> IntToInt {
    return triple;
}
fn init() -> void
var {
    maker: Factory;
    f: IntToInt;
}
{
    maker = getDoubler;
    f = maker();
    Out.Int(f(5));
    Out.Ln();
    maker = getTripler;
    f = maker();
    Out.Int(f(5));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FnRet", src, {"Out"}) == "10\n15\n");
}

TEST_CASE("Function type — three levels of nesting", "[ccodegen][functype]")
{
    std::string src = R"(module FnDeep
import Out;
type {
    Fn0: () -> i64;
    Fn1: () -> Fn0;
    Fn2: () -> Fn1;
}
fn getValue() -> i64 {
    return 42;
}
fn makeLevel1() -> Fn0 {
    return getValue;
}
fn makeLevel2() -> Fn1 {
    return makeLevel1;
}
fn init() -> void
var {
    l2: Fn2;
    l1: Fn1;
    l0: Fn0;
}
{
    l2 = makeLevel2;
    l1 = l2();
    l0 = l1();
    Out.Int(l0());
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FnDeep", src, {"Out"}) == "42\n");
}

TEST_CASE("Function type — parameterized factory", "[ccodegen][functype]")
{
    std::string src = R"(module FnFactory
import Out;
type {
    IntToInt: (x: i64) -> i64;
}
fn add10(x: i64) -> i64 {
    return x + 10;
}
fn mul5(x: i64) -> i64 {
    return x * 5;
}
fn pick(which: i64) -> IntToInt
var result: IntToInt;
{
    if which == 0 {
        result = add10;
    } else {
        result = mul5;
    }
    return result;
}
fn init() -> void
var {
    f: IntToInt;
}
{
    f = pick(0);
    Out.Int(f(3));
    Out.Ln();
    f = pick(1);
    Out.Int(f(3));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FnFactory", src, {"Out"}) == "13\n15\n");
}

TEST_CASE("Function type — array of function pointers", "[ccodegen][functype]")
{
    std::string src = R"(module FnArray
import Out;
type {
    UnaryOp: (x: i64) -> i64;
}
fn negate(x: i64) -> i64 {
    return -x;
}
fn double(x: i64) -> i64 {
    return x * 2;
}
fn square(x: i64) -> i64 {
    return x * x;
}
fn init() -> void
var {
    ops: UnaryOp[3];
    i: i64;
}
{
    ops[0] = negate;
    ops[1] = double;
    ops[2] = square;
    for (i = 0, 2, 1) {
        Out.Int(ops[i](5));
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("FnArray", src, {"Out"}) == "-5\n10\n25\n");
}

// ============================================================================
// Random module tests
// ============================================================================

TEST_CASE("Random — Seed and NextInt deterministic", "[ccodegen][stdlib][random]")
{
    std::string src = R"(module RndTest
import Random, Out;
fn init() -> void
var i: i64;
{
    Random.Seed(42);
    for (i = 0, 4, 1) {
        Out.Int(Random.IntN(100));
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("RndTest", src, {"Out", "Random"}) == "6\n45\n29\n82\n25\n");
}

TEST_CASE("Random — NextBool deterministic", "[ccodegen][stdlib][random]")
{
    std::string src = R"(module RndBool
import Random, Out;
fn init() -> void
var i: i64;
{
    Random.Seed(42);
    for (i = 0, 3, 1) {
        if Random.Bool() {
            Out.Int(1);
        } else {
            Out.Int(0);
        }
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("RndBool", src, {"Out", "Random"}) == "0\n0\n1\n1\n");
}

TEST_CASE("Random — same seed reproduces sequence", "[ccodegen][stdlib][random]")
{
    std::string src = R"(module RndRepro
import Random, Out;
fn init() -> void
var {
    a: i64;
    b: i64;
}
{
    Random.Seed(123);
    a = Random.IntN(1000);
    Random.Seed(123);
    b = Random.IntN(1000);
    if a == b {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("RndRepro", src, {"Out", "Random"}) == "1\n");
}

// ============================================================================
// Strings module tests
// ============================================================================

TEST_CASE("Strings — length prefix suffix and equality", "[ccodegen][stdlib][strings]")
{
    std::string src = R"(module StringsPredicates
import Strings, Out;
fn init() -> void
{
    Out.Int(Strings.Length("obould"));
    Out.Ln();
    Out.Bool(Strings.StartsWith("obould", "obo"));
    Out.Ln();
    Out.Bool(Strings.EndsWith("obould", "uld"));
    Out.Ln();
    Out.Bool(Strings.Equals("obould", "oberon"));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("StringsPredicates", src, {"Out", "Strings"}) == "6\nTrue\nTrue\nFalse\n");
}

TEST_CASE("Strings — find from offset and missing character", "[ccodegen][stdlib][strings]")
{
    std::string src = R"(module StringsFind
import Strings, Out;
fn init() -> void
{
    Out.Int(Strings.Find("banana", chr(97), 2));
    Out.Ln();
    Out.Int(Strings.Find("banana", chr(98), -3));
    Out.Ln();
    Out.Int(Strings.Find("banana", chr(122), 0));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("StringsFind", src, {"Out", "Strings"}) == "3\n0\n-1\n");
}

// ============================================================================
// Parse module tests
// ============================================================================

TEST_CASE("Parse — integers and reals", "[ccodegen][stdlib][parse]")
{
    std::string src = R"(module ParseValues
import Parse, Out;
fn init() -> void
var {
    i: i64;
    r: f64;
}
{
    if Parse.Int("-42", i) {
        Out.Int(i);
    } else {
        Out.Int(0);
    }
    Out.Ln();

    if Parse.Real("3.25", r) {
        Out.Real(r, 2);
    } else {
        Out.Real(0.0, 2);
    }
    Out.Ln();

    Out.Bool(Parse.Int("12x", i));
    Out.Ln();
    Out.Bool(Parse.Real("abc", r));
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ParseValues", src, {"Out", "Parse"}) == "-42\n3.25\nFalse\nFalse\n");
}

// ============================================================================
// In module tests
// ============================================================================

TEST_CASE("In — ReadInt from stdin", "[ccodegen][stdlib][in]")
{
    std::string src = R"(module InInt
import In, Out;
fn init() -> void
var {
    a: i64;
    b: i64;
}
{
    a = In.Int();
    b = In.Int();
    Out.Int(a + b);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("InInt", src, {"Out", "In"}, "10 20\n") == "30\n");
}

TEST_CASE("In — ReadLine from stdin", "[ccodegen][stdlib][in]")
{
    std::string src = R"(module InLine
import In, Out;
fn init() -> void
var {
    buf: char[64];
}
{
    In.Line(buf);
    Out.String(buf);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("InLine", src, {"Out", "In"}, "Hello World\n") == "Hello World\n");
}

TEST_CASE("In — ReadChar from stdin", "[ccodegen][stdlib][in]")
{
    std::string src = R"(module InChar
import In, Out;
fn init() -> void
var {
    c: char;
}
{
    c = In.Char();
    Out.Char(c);
    c = In.Char();
    Out.Char(c);
    c = In.Char();
    Out.Char(c);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("InChar", src, {"Out", "In"}, "ABC") == "ABC\n");
}

// ============================================================================
// Files module tests
// ============================================================================

TEST_CASE("Files — write and read back integers", "[ccodegen][stdlib][files]")
{
    std::string src = R"(module FilesInt
import Files, Out;
fn init() -> void
var {
    f: Files.File;
    r: Files.FileRider;
    val: i64;
}
{
    f = Files.Open("_test_int.bin", "wb");
    Files.Set(r, f, 0);
    Files.WriteInt64(r, 42);
    Files.WriteInt64(r, 100);
    Files.Close(f);

    f = Files.Open("_test_int.bin", "rb");
    Files.Set(r, f, 0);
    Files.ReadInt64(r, val);
    Out.Int(val);
    Out.Ln();
    Files.ReadInt64(r, val);
    Out.Int(val);
    Out.Ln();
    Files.Close(f);
}
)";
    REQUIRE(transpileCompileRun("FilesInt", src, {"Out", "Files"}) == "42\n100\n");
}

TEST_CASE("Files — write and read back bytes", "[ccodegen][stdlib][files]")
{
    std::string src = R"(module FilesByte
import Files, Out;
fn init() -> void
var {
    f: Files.File;
    r: Files.FileRider;
    b: byte;
}
{
    f = Files.Open("_test_byte.bin", "wb");
    Files.Set(r, f, 0);
    Files.WriteByte(r, 65);
    Files.WriteByte(r, 66);
    Files.WriteByte(r, 67);
    Files.Close(f);

    f = Files.Open("_test_byte.bin", "rb");
    Files.Set(r, f, 0);
    Files.ReadByte(r, b);
    Out.Int(b);
    Out.Ln();
    Files.ReadByte(r, b);
    Out.Int(b);
    Out.Ln();
    Files.ReadByte(r, b);
    Out.Int(b);
    Out.Ln();
    Files.Close(f);
}
)";
    REQUIRE(transpileCompileRun("FilesByte", src, {"Out", "Files"}) == "65\n66\n67\n");
}

TEST_CASE("Files — write and read back string", "[ccodegen][stdlib][files]")
{
    std::string src = R"(module FilesStr
import Files, Out;
fn init() -> void
var {
    f: Files.File;
    r: Files.FileRider;
    buf: char[64];
}
{
    f = Files.Open("_test_str.bin", "wb");
    Files.Set(r, f, 0);
    Files.WriteString(r, "Hello");
    Files.Close(f);

    f = Files.Open("_test_str.bin", "rb");
    Files.Set(r, f, 0);
    Files.ReadString(r, buf);
    Out.String(buf);
    Out.Ln();
    Files.Close(f);
}
)";
    REQUIRE(transpileCompileRun("FilesStr", src, {"Out", "Files"}) == "Hello\n");
}

// ============================================================================
// Set tests
// ============================================================================

TEST_CASE("Set — literal and in operator", "[ccodegen][set]")
{
    std::string src = R"(module SetIn
import Out;
fn init() -> void
var x: set;
{
    x = {0, 3, 5..11};
    if 7 in x { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
    if 2 in x { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
    if 0 in x { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetIn", src, {"Out"}) == "1\n0\n1\n");
}

TEST_CASE("Set — union (+)", "[ccodegen][set]")
{
    std::string src = R"(module SetUnion
import Out;
fn init() -> void
var {
    a: set;
    b: set;
    c: set;
}
{
    a = {1, 2, 3};
    b = {3, 4, 5};
    c = a + b;
    if 1 in c { Out.Int(1); } else { Out.Int(0); }
    if 4 in c { Out.Int(1); } else { Out.Int(0); }
    if 6 in c { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetUnion", src, {"Out"}) == "110\n");
}

TEST_CASE("Set — difference (-)", "[ccodegen][set]")
{
    std::string src = R"(module SetDiff
import Out;
fn init() -> void
var {
    a: set;
    b: set;
    c: set;
}
{
    a = {1, 2, 3, 4, 5};
    b = {3, 4};
    c = a - b;
    if 1 in c { Out.Int(1); } else { Out.Int(0); }
    if 3 in c { Out.Int(1); } else { Out.Int(0); }
    if 5 in c { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetDiff", src, {"Out"}) == "101\n");
}

TEST_CASE("Set — intersection (*)", "[ccodegen][set]")
{
    std::string src = R"(module SetInter
import Out;
fn init() -> void
var {
    a: set;
    b: set;
    c: set;
}
{
    a = {1, 2, 3, 4};
    b = {3, 4, 5, 6};
    c = a * b;
    if 2 in c { Out.Int(1); } else { Out.Int(0); }
    if 3 in c { Out.Int(1); } else { Out.Int(0); }
    if 5 in c { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetInter", src, {"Out"}) == "010\n");
}

TEST_CASE("Set — symmetric difference (/)", "[ccodegen][set]")
{
    std::string src = R"(module SetSymDiff
import Out;
fn init() -> void
var {
    a: set;
    b: set;
    c: set;
}
{
    a = {1, 2, 3};
    b = {2, 3, 4};
    c = a / b;
    if 1 in c { Out.Int(1); } else { Out.Int(0); }
    if 2 in c { Out.Int(1); } else { Out.Int(0); }
    if 4 in c { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetSymDiff", src, {"Out"}) == "101\n");
}

TEST_CASE("Set — complement (unary minus)", "[ccodegen][set]")
{
    std::string src = R"(module SetCompl
import Out;
fn init() -> void
var {
    a: set;
    b: set;
}
{
    a = {0, 1, 2};
    b = -a;
    if 0 in b { Out.Int(1); } else { Out.Int(0); }
    if 3 in b { Out.Int(1); } else { Out.Int(0); }
    if 63 in b { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetCompl", src, {"Out"}) == "011\n");
}

TEST_CASE("Set — equality and inequality", "[ccodegen][set]")
{
    std::string src = R"(module SetEq
import Out;
fn init() -> void
var {
    a: set;
    b: set;
}
{
    a = {1, 2, 3};
    b = {1, 2, 3};
    if a == b { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
    b = {1, 2};
    if a != b { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetEq", src, {"Out"}) == "1\n1\n");
}

TEST_CASE("Set — empty set", "[ccodegen][set]")
{
    std::string src = R"(module SetEmpty
import Out;
fn init() -> void
var x: set;
{
    x = {};
    if 0 in x { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
    x = x + {5};
    if 5 in x { Out.Int(1); } else { Out.Int(0); }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetEmpty", src, {"Out"}) == "0\n1\n");
}

TEST_CASE("Set — combined operations", "[ccodegen][set]")
{
    std::string src = R"(module SetCombined
import Out;
fn init() -> void
var {
    evens: set;
    odds: set;
    small: set;
    result: set;
    i: i64;
}
{
    evens = {0, 2, 4, 6, 8};
    odds = {1, 3, 5, 7, 9};
    small = {0..4};
    result = (evens + odds) * small;
    for (i = 0, 9, 1) {
        if i in result {
            Out.Int(i);
        }
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetCombined", src, {"Out"}) == "01234\n");
}

TEST_CASE("Set — range expressions with constants", "[ccodegen][set]")
{
    std::string src = R"(module SetConst
import Out;
const {
    LO = 3;
    HI = 7;
}
fn init() -> void
var {
    x: set;
    i: i64;
}
{
    x = {LO..HI};
    for (i = 0, 9, 1) {
        if i in x {
            Out.Int(i);
        }
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("SetConst", src, {"Out"}) == "34567\n");
}

// ============================================================================
// Threads module tests
// ============================================================================

TEST_CASE("Threads — start and join single thread", "[ccodegen][stdlib][threads]")
{
    std::string src = R"(module ThrSingle
import Threads, Out;
var done: i64;
fn task() -> void {
    done = 1;
}
fn init() -> void
var t: Threads.Thread;
{
    done = 0;
    t = Threads.Start(task);
    Threads.Join(t);
    Out.Int(done);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ThrSingle", src, {"Out", "Threads"}) == "1\n");
}

TEST_CASE("Threads — two threads accumulate into global", "[ccodegen][stdlib][threads]")
{
    std::string src = R"(module ThrTwo
import Threads, Out;
var counter: i64;
fn addTen() -> void {
    counter = counter + 10;
}
fn addTwenty() -> void {
    counter = counter + 20;
}
fn init() -> void
var {
    t1: Threads.Thread;
    t2: Threads.Thread;
}
{
    counter = 0;
    t1 = Threads.Start(addTen);
    Threads.Join(t1);
    t2 = Threads.Start(addTwenty);
    Threads.Join(t2);
    Out.Int(counter);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("ThrTwo", src, {"Out", "Threads"}) == "30\n");
}

TEST_CASE("Threads — thread modifies global array", "[ccodegen][stdlib][threads]")
{
    std::string src = R"(module ThrArr
import Threads, Out;
var arr: i64[3];
fn fillArr() -> void {
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
}
fn init() -> void
var {
    t: Threads.Thread;
    i: i64;
}
{
    t = Threads.Start(fillArr);
    Threads.Join(t);
    for (i = 0, 2, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
}
)";
    REQUIRE(transpileCompileRun("ThrArr", src, {"Out", "Threads"}) == "10\n20\n30\n");
}

// ============================================================================
// Module init ordering tests
// ============================================================================

TEST_CASE("Init — single module init called via wrapper", "[ccodegen][init]")
{
    std::string src = R"(module InitSingle
import Out;
fn init() -> void {
    Out.Int(1);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("InitSingle", src, {"Out"}) == "1\n");
}

TEST_CASE("Init — imported module init called before main", "[ccodegen][init][multimodule]")
{
    ModuleSource lib = {"InitLib", R"(module InitLib
import Out;
fn init() -> void {
    Out.Int(1);
    Out.Ln();
}
)"};

    ModuleSource main = {"InitOrder", R"(module InitOrder
import InitLib, Out;
fn init() -> void {
    Out.Int(2);
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({lib}, main) == "1\n2\n");
}

TEST_CASE("Init — diamond import, init called once", "[ccodegen][init][multimodule]")
{
    ModuleSource shared = {"Shared", R"(module Shared
import Out;
fn init() -> void {
    Out.Int(99);
    Out.Ln();
}
)"};

    ModuleSource libA = {"LibA", R"(module LibA
import Shared, Out;
fn init() -> void {
    Out.Int(10);
    Out.Ln();
}
)"};

    ModuleSource libB = {"LibB", R"(module LibB
import Shared, Out;
fn init() -> void {
    Out.Int(20);
    Out.Ln();
}
)"};

    ModuleSource main = {"Diamond", R"(module Diamond
import LibA, LibB, Out;
fn init() -> void {
    Out.Int(30);
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({shared, libA, libB}, main) == "99\n10\n20\n30\n");
}

TEST_CASE("Init — module without init procedure", "[ccodegen][init][multimodule]")
{
    ModuleSource lib = {"NoInit", R"(module NoInit
fn export Add(a, b: i64) -> i64 {
    return a + b;
}
)"};

    ModuleSource main = {"UseNoInit", R"(module UseNoInit
import NoInit, Out;
fn init() -> void {
    Out.Int(NoInit.Add(3, 4));
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({lib}, main) == "7\n");
}

TEST_CASE("Init — chain of imports A->B->C", "[ccodegen][init][multimodule]")
{
    ModuleSource modC = {"ModC", R"(module ModC
import Out;
fn init() -> void {
    Out.Int(1);
    Out.Ln();
}
)"};

    ModuleSource modB = {"ModB", R"(module ModB
import ModC, Out;
fn init() -> void {
    Out.Int(2);
    Out.Ln();
}
)"};

    ModuleSource main = {"ModA", R"(module ModA
import ModB, ModC, Out;
fn init() -> void {
    Out.Int(3);
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({modC, modB}, main) == "1\n2\n3\n");
}

TEST_CASE("Init — chain of imports A->B+C", "[ccodegen][init][multimodule]")
{
    ModuleSource modC = {"ModC", R"(module ModC
import Out;
fn init() -> void {
    Out.Int(1);
    Out.Ln();
}
)"};

    ModuleSource modB = {"ModB", R"(module ModB
import Out;
fn init() -> void {
    Out.Int(2);
    Out.Ln();
}
)"};

    ModuleSource main = {"ModA", R"(module ModA
import ModB, ModC, Out;
fn init() -> void {
    Out.Int(3);
    Out.Ln();
}
)"};

    REQUIRE(multiModuleRun({modC, modB}, main) == "2\n1\n3\n");
}

// ============================================================================
// Hex and float-E literal tests
// ============================================================================

TEST_CASE("Literals — hex integers", "[ccodegen][literals]")
{
    std::string src = R"(module HexLit
import Out;
fn init() -> void {
    Out.Int(0xFF);
    Out.Ln();
    Out.Int(0x0);
    Out.Ln();
    Out.Int(0x1A);
    Out.Ln();
    Out.Int(0x10);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexLit", src, {"Out"}) == "255\n0\n26\n16\n");
}

TEST_CASE("Literals — hex arithmetic", "[ccodegen][literals]")
{
    std::string src = R"(module HexArith
import Out;
fn init() -> void
var x: i64;
{
    x = 0xA + 0xB;
    Out.Int(x);
    Out.Ln();
    x = 0xFF - 0xF;
    Out.Int(x);
    Out.Ln();
    x = 0x10 * 2;
    Out.Int(x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexArith", src, {"Out"}) == "21\n240\n32\n");
}

TEST_CASE("Literals — float with E notation", "[ccodegen][literals]")
{
    std::string src = R"(module FloatE
import Out;
fn init() -> void
var f: f64;
{
    f = 1.5E2;
    Out.Real(f, 1);
    Out.Ln();
    f = 2.0E-1;
    Out.Real(f, 1);
    Out.Ln();
    f = 3.14E0;
    Out.Real(f, 2);
    Out.Ln();
    f = 1.0E3;
    Out.Real(f, 1);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FloatE", src, {"Out"}) == "150.0\n0.2\n3.14\n1000.0\n");
}

TEST_CASE("Literals — float E with positive exponent", "[ccodegen][literals]")
{
    std::string src = R"(module FloatEPos
import Out;
fn init() -> void
var f: f64;
{
    f = 5.0E+2;
    Out.Real(f, 1);
    Out.Ln();
    f = 1.23E+1;
    Out.Real(f, 1);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FloatEPos", src, {"Out"}) == "500.0\n12.3\n");
}

TEST_CASE("Literals — float E arithmetic", "[ccodegen][literals]")
{
    std::string src = R"(module FloatECalc
import Out;
fn init() -> void
var {
    a: f64;
    b: f64;
}
{
    a = 1.0E2;
    b = 2.5E1;
    Out.Real(a + b, 1);
    Out.Ln();
    Out.Real(a / b, 1);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("FloatECalc", src, {"Out"}) == "125.0\n4.0\n");
}

TEST_CASE("Literals — hex in conditions and arrays", "[ccodegen][literals]")
{
    std::string src = R"(module HexUsage
import Out;
fn init() -> void
var {
    arr: i64[3];
    i: i64;
}
{
    arr[0] = 0xA;
    arr[1] = 0xB;
    arr[2] = 0xC;
    for (i = 0, 2, 1) {
        Out.Int(arr[i]);
        Out.Ln();
    }
    if 0xA == 10 {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexUsage", src, {"Out"}) == "10\n11\n12\n1\n");
}

TEST_CASE("Literals — H-suffix hex integers", "[ccodegen][literals]")
{
    std::string src = R"(module HexHLit
import Out;
fn init() -> void {
    Out.Int(0FFH);
    Out.Ln();
    Out.Int(1AH);
    Out.Ln();
    Out.Int(10H);
    Out.Ln();
    Out.Int(0H);
    Out.Ln();
    Out.Int(7FH);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexHLit", src, {"Out"}) == "255\n26\n16\n0\n127\n");
}

TEST_CASE("Literals — H-suffix hex arithmetic", "[ccodegen][literals]")
{
    std::string src = R"(module HexHArith
import Out;
fn init() -> void
var x: i64;
{
    x = 0AH + 0BH;
    Out.Int(x);
    Out.Ln();
    x = 0FFH - 0FH;
    Out.Int(x);
    Out.Ln();
    x = 10H * 2;
    Out.Int(x);
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexHArith", src, {"Out"}) == "21\n240\n32\n");
}

TEST_CASE("Literals — H-suffix and 0x hex mixed", "[ccodegen][literals]")
{
    std::string src = R"(module HexMixed
import Out;
fn init() -> void {
    if 0FFH == 0xFF {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
    if 10H == 0x10 {
        Out.Int(1);
    } else {
        Out.Int(0);
    }
    Out.Ln();
}
)";
    REQUIRE(transpileCompileRun("HexMixed", src, {"Out"}) == "1\n1\n");
}
