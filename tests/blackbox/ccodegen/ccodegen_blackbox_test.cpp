#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static fs::path obould_bin;
static fs::path stdlib_dir;

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

static std::string transpileCompileRun(const std::string& moduleName,
                                       const std::string& source,
                                       bool needsOut = false)
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

    if (needsOut) {
        fs::copy_file(getStdlibDir() / "Out.h", tmpDir / "Out.h",
                       fs::copy_options::overwrite_existing);
        fs::copy_file(getStdlibDir() / "Out.c", tmpDir / "Out.c",
                       fs::copy_options::overwrite_existing);
        auto symDir = tmpDir / ".obould";
        fs::create_directories(symDir);
        fs::copy_file(getStdlibDir() / "Out.json", symDir / "Out.json",
                       fs::copy_options::overwrite_existing);
    }

    // Transpile: obould <file> --emit-c -o <dir> --main
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

    // Compile
    auto binPath = tmpDir / moduleName;
    std::string compileCmd = "cc -Wno-incompatible-pointer-types -o " +
        binPath.string() + " " + cPath.string();
    if (needsOut) {
        compileCmd += " " + (tmpDir / "Out.c").string();
    }
    compileCmd += " 2>&1";
    REQUIRE(std::system(compileCmd.c_str()) == 0);

    auto outPath = tmpDir / "stdout.txt";
    std::string runCmd = binPath.string() + " > " + outPath.string() + " 2>&1";
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
    std::string compileCmd = "cc -Wno-incompatible-pointer-types -o " + binPath.string();
    for (auto& cf : cFiles) {
        compileCmd += " " + cf.string();
    }
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

TEST_CASE("Hello world — WriteInt and WriteLn", "[ccodegen][basic]")
{
    std::string src = R"(module Hello
import Out;
fn init() -> void {
    Out.WriteInt(42);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Hello", src, true) == "42\n");
}

TEST_CASE("Arithmetic expressions", "[ccodegen][expr]")
{
    std::string src = R"(module Arith
import Out;
fn init() -> void {
    Out.WriteInt(2 + 3 * 4);
    Out.WriteLn();
    Out.WriteInt(100 div 10);
    Out.WriteLn();
    Out.WriteInt(17 mod 5);
    Out.WriteLn();
    Out.WriteInt(10 - 3);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Arith", src, true) == "14\n10\n2\n7\n");
}

TEST_CASE("Unary operators", "[ccodegen][expr]")
{
    std::string src = R"(module Unary
import Out;
fn init() -> void
var x: i64;
{
    x = 5;
    Out.WriteInt(-x);
    Out.WriteLn();
    Out.WriteInt(+x);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Unary", src, true) == "-5\n5\n");
}

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
    Out.WriteInt(c);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Vars", src, true) == "30\n");
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
    Out.WriteInt(X + Y);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Consts", src, true) == "300\n");
}

TEST_CASE("Global variables", "[ccodegen][var]")
{
    std::string src = R"(module Globals
import Out;
var g: i64;
fn init() -> void {
    g = 99;
    Out.WriteInt(g);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Globals", src, true) == "99\n");
}

TEST_CASE("If-else", "[ccodegen][control]")
{
    std::string src = R"(module IfElse
import Out;
fn printSign(x: i64) -> void {
    if x > 0 {
        Out.WriteInt(1);
    } else if x == 0 {
        Out.WriteInt(0);
    } else {
        Out.WriteInt(-1);
    }
    Out.WriteLn();
}
fn init() -> void {
    printSign(5);
    printSign(0);
    printSign(-3);
}
)";
    REQUIRE(transpileCompileRun("IfElse", src, true) == "1\n0\n-1\n");
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
        Out.WriteInt(i);
        Out.WriteLn();
        i = i + 1;
    }
}
)";
    REQUIRE(transpileCompileRun("WhileLoop", src, true) == "0\n1\n2\n3\n4\n");
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
        Out.WriteInt(i);
        Out.WriteLn();
        i = i * 2;
    } while i <= 16;
}
)";
    REQUIRE(transpileCompileRun("DoWhile", src, true) == "1\n2\n4\n8\n16\n");
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
    Out.WriteInt(sum);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ForLoop", src, true) == "55\n");
}

TEST_CASE("For loop with step", "[ccodegen][control]")
{
    std::string src = R"(module ForStep
import Out;
fn init() -> void
var i: i64;
{
    for (i = 0, 20, 5) {
        Out.WriteInt(i);
        Out.WriteLn();
    }
}
)";
    REQUIRE(transpileCompileRun("ForStep", src, true) == "0\n5\n10\n15\n20\n");
}

TEST_CASE("Switch statement", "[ccodegen][control]")
{
    std::string src = R"(module Switch
import Out;
fn classify(x: i64) -> void {
    switch x {
        case 1:
            Out.WriteInt(10);
        case 2:
            Out.WriteInt(20);
        case 3:
            Out.WriteInt(30);
    }
    Out.WriteLn();
}
fn init() -> void {
    classify(1);
    classify(2);
    classify(3);
}
)";
    REQUIRE(transpileCompileRun("Switch", src, true) == "10\n20\n30\n");
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
            Out.WriteInt(1);
        case 6..10:
            Out.WriteInt(2);
    }
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("SwitchRange", src, true) == "2\n");
}

TEST_CASE("Function call and return value", "[ccodegen][proc]")
{
    std::string src = R"(module FuncCall
import Out;
fn add(a, b: i64) -> i64 {
    return a + b;
}
fn init() -> void {
    Out.WriteInt(add(3, 7));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("FuncCall", src, true) == "10\n");
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
    Out.WriteInt(square(5));
    Out.WriteLn();
    Out.WriteInt(cube(3));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("MultiFn", src, true) == "25\n27\n");
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
    Out.WriteInt(factorial(10));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Fact", src, true) == "3628800\n");
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
    Out.WriteInt(fib(10));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Fib", src, true) == "55\n");
}

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
    Out.WriteInt(x);
    Out.WriteLn();
    Out.WriteInt(y);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("RefParam", src, true) == "20\n10\n");
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
    Out.WriteInt(n);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("RefPass", src, true) == "2\n");
}

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
        Out.WriteInt(arr[i]);
        Out.WriteLn();
    }
}
)";
    REQUIRE(transpileCompileRun("ArrBasic", src, true) == "0\n1\n4\n9\n16\n");
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
    Out.WriteInt(sum);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ArrSum", src, true) == "15\n");
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
        Out.WriteInt(1);
    } else {
        Out.WriteInt(0);
    }
    Out.WriteLn();
    p = (a > b) || (a == 5);
    if p {
        Out.WriteInt(1);
    } else {
        Out.WriteInt(0);
    }
    Out.WriteLn();
    if !(a == b) {
        Out.WriteInt(1);
    } else {
        Out.WriteInt(0);
    }
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("BoolExpr", src, true) == "1\n1\n1\n");
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
    Out.WriteInt(count);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("Nested", src, true) == "5\n");
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
    Out.WriteInt(counter);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("GlobFn", src, true) == "3\n");
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
    Out.WriteInt(total);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ConstGlob", src, true) == "130\n");
}

TEST_CASE("Integer division (div)", "[ccodegen][expr]")
{
    std::string src = R"(module IntDiv
import Out;
fn init() -> void {
    Out.WriteInt(17 div 5);
    Out.WriteLn();
    Out.WriteInt(100 div 3);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("IntDiv", src, true) == "3\n33\n");
}

TEST_CASE("Complex expression with parentheses", "[ccodegen][expr]")
{
    std::string src = R"(module ComplexExpr
import Out;
fn init() -> void {
    Out.WriteInt((2 + 3) * (4 + 1));
    Out.WriteLn();
    Out.WriteInt((100 - 50) div (5 + 5));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ComplexExpr", src, true) == "25\n5\n");
}

TEST_CASE("Exported function", "[ccodegen][proc]")
{
    std::string src = R"(module ExportFn
import Out;
fn export compute(x: i64) -> i64 {
    return x * x + 1;
}
fn init() -> void {
    Out.WriteInt(compute(7));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ExportFn", src, true) == "50\n");
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
    Out.WriteInt(abs(-42));
    Out.WriteLn();
    Out.WriteInt(abs(17));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("AbsFn", src, true) == "42\n17\n");
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
    Out.WriteInt(a);
    Out.WriteLn();
    Out.WriteInt(b);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("WhileElif", src, true) == "3\n3\n");
}

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
        Out.WriteInt(arr[i]);
        Out.WriteLn();
    }
}
)";
    REQUIRE(transpileCompileRun("BubbleSort", src, true) == "1\n2\n3\n4\n5\n");
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
    Out.WriteInt(gcd(48, 18));
    Out.WriteLn();
    Out.WriteInt(gcd(100, 75));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("GCD", src, true) == "6\n25\n");
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
    Out.WriteInt(sum);
    Out.WriteLn();
    Out.WriteInt(i);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ForAfter", src, true) == "68\n13\n");
}

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
    Out.WriteInt(MathLib.Square(5));
    Out.WriteLn();
    Out.WriteInt(MathLib.Cube(3));
    Out.WriteLn();
    Out.WriteInt(MathLib.Add(10, 20));
    Out.WriteLn();
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
    Out.WriteInt(Counter.Get());
    Out.WriteLn();
    Counter.Increment();
    Counter.Increment();
    Out.WriteInt(Counter.Get());
    Out.WriteLn();
}
)"};

    REQUIRE(multiModuleRun({counter}, main) == "3\n5\n");
}

TEST_CASE("Multi-module — two libraries", "[ccodegen][multimodule]")
{
    ModuleSource mathLib = {"Math", R"(module Math

fn export Double(x: i64) -> i64 {
    return x + x;
}

fn init() -> void {
}
)"};

    ModuleSource strLib = {"Fmt", R"(module Fmt

import Out;

fn export PrintLabeled(label: i64, value: i64) -> void {
    Out.WriteInt(label);
    Out.WriteInt(value);
    Out.WriteLn();
}

fn init() -> void {
}
)"};

    ModuleSource main = {"App", R"(module App

import Math, Fmt, Out;

fn init() -> void
var x: i64;
{
    x = Math.Double(21);
    Fmt.PrintLabeled(0, x);
    x = Math.Double(x);
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
    Out.WriteInt(p.x + p.y);
    Out.WriteLn();
    p3.x = 1;
    p3.y = 2;
    p3.z = 3;
    Out.WriteInt(p3.x + p3.y + p3.z);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("StructFields", src, true) == "30\n6\n");
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
    Out.WriteInt(Identify(c));
    Out.WriteLn();
    r.id = 0;
    r.w = 3;
    r.h = 4;
    Out.WriteInt(Identify(r));
    Out.WriteLn();
    s.id = 0;
    Out.WriteInt(Identify(s));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("IsOp", src, true) == "1\n2\n0\n");
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
    Out.WriteInt(a.legs);
    if a is Dog {
        Out.WriteInt(a(Dog).barkVolume);
    }
    if a is Cat {
        Out.WriteInt(a(Cat).purring);
    }
    Out.WriteLn();
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
    REQUIRE(transpileCompileRun("TypeGuardAccess", src, true) == "490\n41\n");
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
    Out.WriteInt(Level(b));
    Out.WriteLn();
    Out.WriteInt(Level(m));
    Out.WriteLn();
    Out.WriteInt(Level(l));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("ThreeLevel", src, true) == "1\n2\n3\n");
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
    Out.WriteInt(v.speed);
    if v is Car {
        Out.WriteInt(v(Car).doors);
    }
    Out.WriteLn();
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
    REQUIRE(transpileCompileRun("InheritedFields", src, true) == "1204\n60\n");
}

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
    Out.WriteString(s);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OutputString", src, true) == "Hello, World!\n");
}

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
    Out.WriteInt(sumArray(a));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OABasic", src, true) == "15\n");
}

TEST_CASE("Open array — print all elements via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OARead
import Out;
fn printArray(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.WriteInt(arr[i]);
        Out.WriteLn();
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
    REQUIRE(transpileCompileRun("OARead", src, true) == "10\n20\n30\n");
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
    Out.WriteInt(sumTwo(x, y));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OAMulti", src, true) == "36\n");
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
    Out.WriteInt(scaleAndSum(a, 10));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OAMixed", src, true) == "60\n");
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
    Out.WriteInt(maxArr(a));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OAExport", src, true) == "9\n");
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
    Out.WriteInt(s);
    Out.WriteLn();
}
fn init() -> void {
    g[0] = 100;
    g[1] = 200;
    g[2] = 300;
    g[3] = 400;
    printSum(g);
}
)";
    REQUIRE(transpileCompileRun("OAGlobal", src, true) == "1000\n");
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
    Out.WriteInt(getFirst(a));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OASingle", src, true) == "42\n");
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
    Out.WriteInt(sum(small));
    Out.WriteLn();
    Out.WriteInt(sum(medium));
    Out.WriteLn();
    Out.WriteInt(sum(big));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OASizes", src, true) == "3\n100\n6\n");
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
    Out.WriteInt(linearSearch(a, 30));
    Out.WriteLn();
    Out.WriteInt(linearSearch(a, 99));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OASearch", src, true) == "2\n-1\n");
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
    Out.WriteInt(doubleSum(a));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("OAPassThru", src, true) == "60\n");
}

TEST_CASE("Open array — 2D all dimensions via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA2D
import Out;
fn printDims(m: i64[][]) -> void {
    Out.WriteInt(len(m));
    Out.WriteLn();
    Out.WriteInt(len(m[0]));
    Out.WriteLn();
}
fn init() -> void
var {
    m: i64[3][4];
}
{
    printDims(m);
}
)";
    REQUIRE(transpileCompileRun("OA2D", src, true) == "3\n4\n");
}

TEST_CASE("Open array — 3D all dimensions via len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA3D
import Out;
fn printDims(m: i64[][][]) -> void {
    Out.WriteInt(len(m));
    Out.WriteLn();
    Out.WriteInt(len(m[0]));
    Out.WriteLn();
    Out.WriteInt(len(m[0][0]));
    Out.WriteLn();
}
fn init() -> void
var {
    m: i64[3][4][5];
}
{
    printDims(m);
}
)";
    REQUIRE(transpileCompileRun("OA3D", src, true) == "3\n4\n5\n");
}

TEST_CASE("Open array — 2D pass-through with len", "[ccodegen][openarray]")
{
    std::string src = R"(module OA2DPass
import Out;
fn innerLen(m: i64[][]) -> void {
    Out.WriteInt(len(m));
    Out.WriteLn();
    Out.WriteInt(len(m[0]));
    Out.WriteLn();
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
    REQUIRE(transpileCompileRun("OA2DPass", src, true) == "5\n3\n");
}

TEST_CASE("Open array — multi-module with len", "[ccodegen][openarray][multimodule]")
{
    ModuleSource arrLib = {"ArrLib", R"(module ArrLib

import Out;

fn export PrintArray(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.WriteInt(arr[i]);
        Out.WriteLn();
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
    Out.WriteInt(s);
    Out.WriteLn();
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
    Out.WriteInt(len(a));
    Out.WriteLn();
    Out.WriteInt(len(b));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("LenFixed", src, true) == "5\n10\n");
}

TEST_CASE("Builtin len — open array parameter", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenOpen
import Out;
fn printLen(arr: i64[]) -> void {
    Out.WriteInt(len(arr));
    Out.WriteLn();
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
    REQUIRE(transpileCompileRun("LenOpen", src, true) == "7\n3\n");
}

TEST_CASE("Builtin len — iterate open array with len", "[ccodegen][builtin][len]")
{
    std::string src = R"(module LenIter
import Out;
fn printAll(arr: i64[]) -> void
var i: i64;
{
    for (i = 0, len(arr) - 1, 1) {
        Out.WriteInt(arr[i]);
        Out.WriteLn();
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
    REQUIRE(transpileCompileRun("LenIter", src, true) == "10\n20\n30\n40\n");
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
    Out.WriteInt(len(s));
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("LenStr", src, true) == "6\n");
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
    Out.WriteInt(p.value);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("NewBasic", src, true) == "42\n");
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
    Out.WriteInt(a.x + a.y);
    Out.WriteLn();
    Out.WriteInt(b.x + b.y);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("NewMulti", src, true) == "3\n30\n");
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
    Out.WriteInt(p.val);
    Out.WriteLn();
    p = nil;
    if p == nil {
        Out.WriteInt(0);
    } else {
        Out.WriteInt(1);
    }
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("NewNil", src, true) == "99\n0\n");
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
    Out.WriteInt(p.a);
    Out.WriteLn();
    Out.WriteInt(p.b);
    Out.WriteLn();
    Out.WriteInt(p.c);
    Out.WriteLn();
    p.a = 100;
    Out.WriteInt(p.a);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("NewZero", src, true) == "0\n0\n0\n100\n");
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
    Out.WriteInt(x);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("AssertPass", src, true) == "42\n");
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
    Out.WriteInt(a + b);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("AssertBool", src, true) == "30\n");
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
    Out.WriteInt(x);
    Out.WriteLn();
}
)";
    REQUIRE(transpileCompileRun("AssertMsg", src, true) == "10\n");
}