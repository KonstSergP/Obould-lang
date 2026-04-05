cmake_minimum_required(VERSION 3.14)

project(ObouldCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)


# ==========================================

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
if (LLVM_PACKAGE_VERSION VERSION_LESS "15.0")
    message(FATAL_ERROR "LLVM >= 15.0 is required. Found ${LLVM_PACKAGE_VERSION}.")
endif ()
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

link_directories(${LLVM_LIBRARY_DIRS})
llvm_map_components_to_libnames(llvm_libs
        support
        core
        irreader
        native
        passes
        analysis
        mc
        TargetParser
)

# ==========================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        bdwgc
        GIT_REPOSITORY https://github.com/bdwgc/bdwgc
        GIT_TAG v8.2.12
)
FetchContent_MakeAvailable(bdwgc)

# ==========================================

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# ==========================================

FetchContent_Declare(argparse GIT_REPOSITORY https://github.com/p-ranav/argparse.git)
FetchContent_MakeAvailable(argparse)

# ==========================================

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(obould_lib STATIC ${LIB_SOURCES})
target_include_directories(obould_lib PUBLIC src)
target_link_libraries(obould_lib PRIVATE ${llvm_libs} nlohmann_json::nlohmann_json argparse)

# ==========================================

file(GLOB STD_HEADERS CONFIGURE_DEPENDS "stdlib/*.json" "stdlib/*.h")
file(GLOB_RECURSE STD_SOURCES CONFIGURE_DEPENDS "stdlib/*.c")
add_library(obould_std STATIC ${STD_SOURCES})

add_custom_command(TARGET obould_std POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:obould_std> ${CMAKE_BINARY_DIR}/lib/$<TARGET_FILE_NAME:obould_std>
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gc> ${CMAKE_BINARY_DIR}/lib/$<TARGET_FILE_NAME:gc>
)

# ==========================================

add_executable(obould src/driver/main.cpp)
target_link_libraries(obould PRIVATE obould_lib ${llvm_libs} nlohmann_json::nlohmann_json argparse)
add_dependencies(obould gc obould_std)

add_custom_command(TARGET obould POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/system
)

foreach(HEADER_FILE IN LISTS STD_HEADERS)
    add_custom_command(TARGET obould POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${HEADER_FILE} $<TARGET_FILE_DIR:obould>/system/
    )
endforeach()

# ==========================================

include(CTest)

file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "tests/unit/*.cpp")
option(O_TESTS "Build unit tests (requires Catch2)" OFF)

if (O_TESTS)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    include(Catch)

    if (UNIT_TEST_SOURCES)
        add_executable(unit_tests ${UNIT_TEST_SOURCES})
        target_link_libraries(unit_tests PRIVATE obould_lib Catch2::Catch2WithMain ${llvm_libs} nlohmann_json::nlohmann_json)
        catch_discover_tests(unit_tests)
    endif ()

    file(GLOB_RECURSE BLACKBOX_CCODEGEN_SOURCES CONFIGURE_DEPENDS "tests/blackbox/ccodegen/*.cpp")
    if (BLACKBOX_CCODEGEN_SOURCES)
        add_executable(blackbox_ccodegen_tests ${BLACKBOX_CCODEGEN_SOURCES})
        target_compile_definitions(blackbox_ccodegen_tests PRIVATE
            OBOULD_BUILD_DIR="${CMAKE_BINARY_DIR}"
            OBOULD_STDLIB_DIR="${CMAKE_SOURCE_DIR}/stdlib"
        )
        target_link_libraries(blackbox_ccodegen_tests PRIVATE Catch2::Catch2WithMain)
        catch_discover_tests(blackbox_ccodegen_tests TEST_PREFIX "blackbox/")
    endif ()
endif ()
