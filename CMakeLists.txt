cmake_minimum_required(VERSION 3.14)

project(ObouldCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ==========================================

file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(obould_lib STATIC ${LIB_SOURCES})
target_include_directories(obould_lib PUBLIC src)

# ==========================================

add_executable(obould src/driver/main.cpp)
target_link_libraries(obould PRIVATE obould_lib)


find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader native)
target_link_libraries(obould PRIVATE ${llvm_libs})

# ==========================================

file(GLOB MANUAL_TESTS "tests/manual/*.cpp")

foreach(test_source ${MANUAL_TESTS})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE obould_lib)
    message(STATUS "Added manual test: ${test_name}")
endforeach()

# ==========================================

include(CTest)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE UNIT_TEST_SOURCES "tests/unit/*.cpp")

if(UNIT_TEST_SOURCES)
    add_executable(unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE obould_lib Catch2::Catch2WithMain)
    add_test(NAME AllTests COMMAND unit_tests)
endif()
