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
    }

    // Transpile: obould <file> --emit-c -o <dir> --main
    std::string transpileCmd = getObould().string() +
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
    std::string compileCmd = "cc -o " + binPath.string() +
        " " + cPath.string();
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
    Out.WriteInt(100 / 10);
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
var sum: i64;
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
fn init() -> void {
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
var arr: i64[5];
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
