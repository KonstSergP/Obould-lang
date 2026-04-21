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

add_executable(obould src/driver/main.cpp)
target_link_libraries(obould PRIVATE obould_lib ${llvm_libs} nlohmann_json::nlohmann_json argparse)

# ==========================================

set(SYS_DIR "${CMAKE_BINARY_DIR}/system")
set(LIB_DIR "${CMAKE_BINARY_DIR}/lib")
set(ARTIFACTS_DIR "${CMAKE_BINARY_DIR}/artifacts")
set(LIB_ARTIFACTS "")


set(GC_LIB "${LIB_DIR}/libgc.a")
add_custom_command(
        OUTPUT "${GC_LIB}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:gc>" "${GC_LIB}"
        DEPENDS gc
)
list(APPEND LIB_ARTIFACTS "${GC_LIB}")


file(GLOB STD_HEADERS CONFIGURE_DEPENDS "stdlib/*.json" "stdlib/*.h")
foreach (HEADER IN LISTS STD_HEADERS)
    get_filename_component(FNAME "${HEADER}" NAME)
    add_custom_command(
            OUTPUT "${SYS_DIR}/${FNAME}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SYS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HEADER}" "${SYS_DIR}/${FNAME}"
            DEPENDS "${HEADER}"
    )
    list(APPEND LIB_ARTIFACTS "${SYS_DIR}/${FNAME}")
endforeach ()


file(GLOB_RECURSE STD_C_SOURCES CONFIGURE_DEPENDS "stdlib/*.c")

set(STD_OBL_OBJS "")

file(GLOB_RECURSE STD_OBL_SOURCES CONFIGURE_DEPENDS "stdlib/*.obl")
foreach (OBL_FILE IN LISTS STD_OBL_SOURCES)
    get_filename_component(FNAME_WE "${OBL_FILE}" NAME_WE)
    set(OUT_OBJ "${ARTIFACTS_DIR}/${FNAME_WE}.o")
    set(OUT_JSON "${SYS_DIR}/${FNAME_WE}.json")

    add_custom_command(
            OUTPUT "${OUT_OBJ}" "${OUT_JSON}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ARTIFACTS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SYS_DIR}"

            COMMAND $<TARGET_FILE:obould> "${OBL_FILE}" -o "${OUT_OBJ}"

            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARTIFACTS_DIR}/${FNAME_WE}.json" "${OUT_JSON}"

            DEPENDS obould "${OBL_FILE}"
            COMMENT "Compiling Obould module: ${FNAME_WE}.obl"
    )
    list(APPEND LIB_ARTIFACTS "${OUT_JSON}")
    list(APPEND STD_OBL_OBJS "${OUT_OBJ}")
endforeach ()

add_library(obould_std STATIC ${STD_C_SOURCES} ${STD_OBL_OBJS})
target_compile_options(obould_std PRIVATE
        -ffunction-sections
        -fdata-sections
)
set(OB_STD_LIB "${LIB_DIR}/libobould_std.a")
add_custom_command(
        OUTPUT "${LIB_DIR}/libobould_std.a"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LIB_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:obould_std>" "${OB_STD_LIB}"
        DEPENDS obould_std
)
list(APPEND LIB_ARTIFACTS "${OB_STD_LIB}")

add_custom_target(libs ALL DEPENDS ${LIB_ARTIFACTS})

# ==========================================

option(O_TESTS "Build tests" OFF)

if (O_TESTS)
    include(CTest)

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    include(Catch)

    file(GLOB_RECURSE BLACKBOX_CCODEGEN_SOURCES CONFIGURE_DEPENDS "tests/blackbox/ccodegen/*.cpp")
    if (BLACKBOX_CCODEGEN_SOURCES)
        add_executable(blackbox_ccodegen_tests ${BLACKBOX_CCODEGEN_SOURCES})
        target_compile_definitions(blackbox_ccodegen_tests PRIVATE
                OBOULD_BUILD_DIR="${CMAKE_BINARY_DIR}"
                OBOULD_STDLIB_DIR="${CMAKE_SOURCE_DIR}/stdlib"
                OBOULD_GC_INCLUDE_DIR="${bdwgc_SOURCE_DIR}/include"
                OBOULD_GC_LIB_DIR="${CMAKE_BINARY_DIR}/lib"
        )
        target_link_libraries(blackbox_ccodegen_tests PRIVATE Catch2::Catch2WithMain)
        catch_discover_tests(blackbox_ccodegen_tests TEST_PREFIX "blackbox/")
    endif ()


    file(GLOB_RECURSE CASES_SOURCES CONFIGURE_DEPENDS "tests/cases/*.obl")
    if (CASES_SOURCES)
        foreach (TEST_FILE IN LISTS CASES_SOURCES)
            get_filename_component(TEST_NAME "${TEST_FILE}" NAME_WE)
            add_test(
                    NAME "${TEST_NAME}"
                    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/run_case.sh"
                    "$<TARGET_FILE:obould>"
                    "${TEST_FILE}"
            )
            set_tests_properties(${TEST_NAME} PROPERTIES DEPENDS obould_std)
        endforeach ()
    endif ()
endif ()
