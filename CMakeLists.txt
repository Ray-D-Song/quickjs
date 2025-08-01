cmake_minimum_required(VERSION 3.10)

project(quickjs LANGUAGES C)

include(CheckCCompilerFlag)
include(GNUInstallDirs)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)

# MINGW doesn't exist in older cmake versions, newer versions don't know
# about CMAKE_COMPILER_IS_MINGW, and there is no unique CMAKE_C_COMPILER_ID
# for mingw-based compilers...
if(MINGW)
    # do nothing
elseif(CMAKE_C_COMPILER MATCHES "mingw")
    set(MINGW TRUE)
else()
    set(MINGW FALSE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Building with ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} on ${CMAKE_SYSTEM}")

macro(xcheck_add_c_compiler_flag FLAG)
    string(REPLACE "-" "" FLAG_NO_HYPHEN ${FLAG})
    check_c_compiler_flag(${FLAG} COMPILER_SUPPORTS_${FLAG_NO_HYPHEN})
    if(COMPILER_SUPPORTS_${FLAG_NO_HYPHEN})
        add_compile_options(${FLAG})
    endif()
endmacro()

xcheck_add_c_compiler_flag(-Wall)
if(NOT MSVC AND NOT IOS)
    xcheck_add_c_compiler_flag(-Werror)
    xcheck_add_c_compiler_flag(-Wextra)
endif()
xcheck_add_c_compiler_flag(-Wformat=2)
xcheck_add_c_compiler_flag(-Wno-implicit-fallthrough)
xcheck_add_c_compiler_flag(-Wno-sign-compare)
xcheck_add_c_compiler_flag(-Wno-missing-field-initializers)
xcheck_add_c_compiler_flag(-Wno-unused-parameter)
xcheck_add_c_compiler_flag(-Wno-unused-but-set-variable)
xcheck_add_c_compiler_flag(-Wno-unused-result)
xcheck_add_c_compiler_flag(-Wno-stringop-truncation)
xcheck_add_c_compiler_flag(-Wno-array-bounds)
xcheck_add_c_compiler_flag(-funsigned-char)

# ClangCL is command line compatible with MSVC, so 'MSVC' is set.
if(MSVC)
    xcheck_add_c_compiler_flag(-Wno-unsafe-buffer-usage)
    xcheck_add_c_compiler_flag(-Wno-sign-conversion)
    xcheck_add_c_compiler_flag(-Wno-nonportable-system-include-path)
    xcheck_add_c_compiler_flag(-Wno-implicit-int-conversion)
    xcheck_add_c_compiler_flag(-Wno-shorten-64-to-32)
    xcheck_add_c_compiler_flag(-Wno-reserved-macro-identifier)
    xcheck_add_c_compiler_flag(-Wno-reserved-identifier)
    xcheck_add_c_compiler_flag(-Wdeprecated-declarations)
    xcheck_add_c_compiler_flag(/experimental:c11atomics)
    xcheck_add_c_compiler_flag(/wd4018) # -Wno-sign-conversion
    xcheck_add_c_compiler_flag(/wd4061) # -Wno-implicit-fallthrough
    xcheck_add_c_compiler_flag(/wd4100) # -Wno-unused-parameter
    xcheck_add_c_compiler_flag(/wd4200) # -Wno-zero-length-array
    xcheck_add_c_compiler_flag(/wd4242) # -Wno-shorten-64-to-32
    xcheck_add_c_compiler_flag(/wd4244) # -Wno-shorten-64-to-32
    xcheck_add_c_compiler_flag(/wd4245) # -Wno-sign-compare
    xcheck_add_c_compiler_flag(/wd4267) # -Wno-shorten-64-to-32
    xcheck_add_c_compiler_flag(/wd4388) # -Wno-sign-compare
    xcheck_add_c_compiler_flag(/wd4389) # -Wno-sign-compare
    xcheck_add_c_compiler_flag(/wd4456) # Hides previous local declaration
    xcheck_add_c_compiler_flag(/wd4457) # Hides function parameter
    xcheck_add_c_compiler_flag(/wd4710) # Function not inlined
    xcheck_add_c_compiler_flag(/wd4711) # Function was inlined
    xcheck_add_c_compiler_flag(/wd4820) # Padding added after construct
    xcheck_add_c_compiler_flag(/wd4996) # -Wdeprecated-declarations
    xcheck_add_c_compiler_flag(/wd5045) # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
endif()

# Set a 8MB default stack size on Windows.
# It defaults to 1MB on MSVC, which is the same as our current JS stack size,
# so it will overflow and crash otherwise.
# On MinGW it defaults to 2MB.
if(WIN32)
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,8388608")
    endif()
endif()

# MacOS and GCC 11 or later need -Wno-maybe-uninitialized
# https://github.com/quickjs-ng/quickjs/issues/453
if(APPLE AND CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
    xcheck_add_c_compiler_flag(-Wno-maybe-uninitialized)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    add_compile_definitions(
        _WASI_EMULATED_PROCESS_CLOCKS
        _WASI_EMULATED_SIGNAL
    )
    add_link_options(
        -lwasi-emulated-process-clocks
        -lwasi-emulated-signal
    )
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    xcheck_add_c_compiler_flag(/Od)
    xcheck_add_c_compiler_flag(-O0)
    xcheck_add_c_compiler_flag(-ggdb)
    xcheck_add_c_compiler_flag(-fno-omit-frame-pointer)
endif()

macro(xoption OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow setting the option through an environment variable.
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

xoption(BUILD_SHARED_LIBS "Build a shared library" OFF)
if(BUILD_SHARED_LIBS)
    message(STATUS "Building a shared library")
endif()

# note: QJS_ENABLE_TSAN is currently incompatible with the other sanitizers but we
# don't explicitly check for that because who knows what the future will bring?
# QJS_ENABLE_MSAN only works with clang at the time of writing; also not checked
# for the same reason
xoption(QJS_BUILD_EXAMPLES "Build examples" OFF)
xoption(QJS_BUILD_CLI_STATIC "Build a static qjs executable" OFF)
xoption(QJS_BUILD_CLI_WITH_MIMALLOC "Build the qjs executable with mimalloc" OFF)
xoption(QJS_BUILD_CLI_WITH_STATIC_MIMALLOC "Build the qjs executable with mimalloc (statically linked)" OFF)
xoption(QJS_DISABLE_PARSER "Disable JS source code parser" OFF)
xoption(QJS_ENABLE_ASAN "Enable AddressSanitizer (ASan)" OFF)
xoption(QJS_ENABLE_MSAN "Enable MemorySanitizer (MSan)" OFF)
xoption(QJS_ENABLE_TSAN "Enable ThreadSanitizer (TSan)" OFF)
xoption(QJS_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (UBSan)" OFF)
xoption(QJS_ENABLE_JIT "Enable JIT compilation" ON)

if(QJS_ENABLE_ASAN)
message(STATUS "Building with ASan")
add_compile_options(
    -fsanitize=address
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
add_link_options(
    -fsanitize=address
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
endif()

if(QJS_ENABLE_MSAN)
message(STATUS "Building with MSan")
add_compile_options(
    -fsanitize=memory
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
add_link_options(
    -fsanitize=memory
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
endif()

if(QJS_ENABLE_TSAN)
message(STATUS "Building with TSan")
add_compile_options(
    -fsanitize=thread
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
add_link_options(
    -fsanitize=thread
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
endif()

if(QJS_ENABLE_UBSAN)
message(STATUS "Building with UBSan")
add_compile_options(
    -fsanitize=undefined
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
add_link_options(
    -fsanitize=undefined
    -fno-sanitize-recover=all
    -fno-omit-frame-pointer
)
endif()

if(QJS_ENABLE_JIT)
    # Create SLJIT library directly instead of using add_subdirectory
    # because SLJIT's CMakeLists.txt only creates sljit_test executable
    add_library(sljit STATIC third-party/sljit/sljit_src/sljitLir.c)
    target_include_directories(sljit PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third-party/sljit/sljit_src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/sljit>)
    # Remove SLJIT_HAVE_CONFIG_PRE to use default memory allocator
    # target_compile_definitions(sljit PRIVATE SLJIT_HAVE_CONFIG_PRE)
    find_package(Threads REQUIRED)
    target_link_libraries(sljit Threads::Threads)
    
    list(APPEND qjs_defines QJS_JIT_ENABLED)
    list(APPEND qjs_libs sljit)
endif()

# QuickJS library
#

xoption(QJS_BUILD_LIBC "Build standard library modules as part of the library" OFF)
macro(add_qjs_libc_if_needed target)
    if(NOT QJS_BUILD_LIBC)
        target_sources(${target} PRIVATE quickjs-libc.c)
    endif()
endmacro()
macro(add_static_if_needed target)
    if(QJS_BUILD_CLI_STATIC OR MINGW)
        target_link_options(${target} PRIVATE -static)
        if(MINGW)
            target_link_options(${target} PRIVATE -static-libgcc)
        endif()
    endif()
endmacro()

set(qjs_sources
    cutils.c
    libregexp.c
    libunicode.c
    quickjs.c
    xsum.c
)

if(QJS_ENABLE_JIT)
    list(APPEND qjs_sources quickjs-jit.c quickjs-jit.h)
endif()

if(QJS_BUILD_LIBC)
    list(APPEND qjs_sources quickjs-libc.c)
endif()
list(APPEND qjs_defines _GNU_SOURCE)
if(WIN32)
    # NB: Windows 7 is EOL and we are only supporting in so far as it doesn't interfere with progress.
    list(APPEND qjs_defines WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
endif()
list(APPEND qjs_libs ${CMAKE_DL_LIBS})
find_package(Threads)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    list(APPEND qjs_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

# try to find libm
find_library(M_LIBRARIES m)
if(M_LIBRARIES OR CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
    list(APPEND qjs_libs m)
endif()

add_library(qjs ${qjs_sources})
target_compile_definitions(qjs PRIVATE ${qjs_defines})
target_include_directories(qjs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(qjs PUBLIC ${qjs_libs})

if(QJS_ENABLE_JIT)
    target_include_directories(qjs PRIVATE third-party/sljit/sljit_src)
endif()

if(EMSCRIPTEN)
    add_executable(qjs_wasm ${qjs_sources})
    target_link_options(qjs_wasm PRIVATE
        # in emscripten 3.x, this will be set to 16k which is too small for quickjs. #write sth. to force github rebuild
        -sSTACK_SIZE=2097152 # let it be 2m = 2 * 1024 * 1024 = 2097152, otherwise, stack overflow may be occured at bootstrap
        -sNO_INVOKE_RUN
        -sNO_EXIT_RUNTIME
        -sMODULARIZE # do not mess the global
        -sEXPORT_ES6 # export js file to morden es module
        -sEXPORT_NAME=getQuickJs # give a name
        -sTEXTDECODER=1 # it will be 2 if we use -Oz, and that will cause js -> c string convertion fail
        -sNO_DEFAULT_TO_CXX # this project is pure c project, no need for c plus plus handle
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
    )
    target_compile_definitions(qjs_wasm PRIVATE ${qjs_defines})
    target_link_libraries(qjs_wasm m)
endif()


# QuickJS bytecode compiler
#

add_executable(qjsc
    qjsc.c
)
add_qjs_libc_if_needed(qjsc)
add_static_if_needed(qjsc)
target_compile_definitions(qjsc PRIVATE ${qjs_defines})
target_link_libraries(qjsc qjs)


# QuickJS CLI
#

add_executable(qjs_exe
    gen/repl.c
    gen/standalone.c
    qjs.c
)
add_qjs_libc_if_needed(qjs_exe)
add_static_if_needed(qjs_exe)
set_target_properties(qjs_exe PROPERTIES
    OUTPUT_NAME "qjs"
)
target_compile_definitions(qjs_exe PRIVATE ${qjs_defines})
target_link_libraries(qjs_exe qjs)
if(NOT WIN32)
    set_target_properties(qjs_exe PROPERTIES ENABLE_EXPORTS TRUE)
endif()
if(QJS_BUILD_CLI_WITH_MIMALLOC OR QJS_BUILD_CLI_WITH_STATIC_MIMALLOC)
    find_package(mimalloc REQUIRED)
    # Upstream mimalloc doesn't provide a way to know if both libraries are supported.
    if(QJS_BUILD_CLI_WITH_STATIC_MIMALLOC)
        target_link_libraries(qjs_exe mimalloc-static)
    else()
        target_link_libraries(qjs_exe mimalloc)
    endif()
endif()

# Test262 runner
#

if(NOT EMSCRIPTEN)
    add_executable(run-test262
        run-test262.c
    )
    add_qjs_libc_if_needed(run-test262)
    target_compile_definitions(run-test262 PRIVATE ${qjs_defines})
    target_link_libraries(run-test262 qjs)
endif()

# Interrupt test
#

add_executable(api-test
    api-test.c
)
target_compile_definitions(api-test PRIVATE ${qjs_defines})
target_link_libraries(api-test qjs)

# Unicode generator
#

add_executable(unicode_gen EXCLUDE_FROM_ALL
    cutils.c
    libunicode.c
    unicode_gen.c
)
target_compile_definitions(unicode_gen PRIVATE ${qjs_defines})

add_executable(function_source
    gen/function_source.c
)
add_qjs_libc_if_needed(function_source)
target_include_directories(function_source PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(function_source PRIVATE ${qjs_defines})
target_link_libraries(function_source qjs)

# Examples
#

if(QJS_BUILD_EXAMPLES)
    add_executable(hello
        gen/hello.c
    )
    add_qjs_libc_if_needed(hello)
    target_include_directories(hello PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(hello PRIVATE ${qjs_defines})
    target_link_libraries(hello qjs)

    add_executable(hello_module
        gen/hello_module.c
    )
    add_qjs_libc_if_needed(hello_module)
    target_include_directories(hello_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(hello_module PRIVATE ${qjs_defines})
    target_link_libraries(hello_module qjs)

    add_library(fib MODULE examples/fib.c)
    set_target_properties(fib PROPERTIES
        PREFIX ""
        C_VISIBILITY_PRESET default
    )
    target_compile_definitions(fib PRIVATE JS_SHARED_LIBRARY)
    if(WIN32)
        target_link_libraries(fib qjs)
    elseif(APPLE)
        target_link_options(fib PRIVATE -undefined dynamic_lookup)
    endif()

    add_library(point MODULE examples/point.c)
    set_target_properties(point PROPERTIES
        PREFIX ""
        C_VISIBILITY_PRESET default
    )
    target_compile_definitions(point PRIVATE JS_SHARED_LIBRARY)
    if(WIN32)
        target_link_libraries(point qjs)
    elseif(APPLE)
        target_link_options(point PRIVATE -undefined dynamic_lookup)
    endif()

    add_executable(test_fib
        examples/fib.c
        gen/test_fib.c
    )
    add_qjs_libc_if_needed(test_fib)
    target_include_directories(test_fib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(test_fib PRIVATE ${qjs_defines})
    target_link_libraries(test_fib qjs)
endif()

# Install target
#

if(NOT IOS)
    file(STRINGS quickjs.h quickjs_h REGEX QJS_VERSION)
    string(REGEX MATCH "QJS_VERSION_MAJOR ([0-9]*)" _ "${quickjs_h}")
    set(QJS_VERSION_MAJOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "QJS_VERSION_MINOR ([0-9]*)" _ "${quickjs_h}")
    set(QJS_VERSION_MINOR ${CMAKE_MATCH_1})
    string(REGEX MATCH "QJS_VERSION_PATCH ([0-9]*)" _ "${quickjs_h}")
    set(QJS_VERSION_PATCH ${CMAKE_MATCH_1})
    set_target_properties(qjs PROPERTIES
        VERSION ${QJS_VERSION_MAJOR}.${QJS_VERSION_MINOR}.${QJS_VERSION_PATCH}
        SOVERSION ${QJS_VERSION_MAJOR}
    )
    install(FILES quickjs.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    if(QJS_BUILD_LIBC)
        install(FILES quickjs-libc.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()
    install(TARGETS qjs_exe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS qjsc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS qjs sljit EXPORT qjsConfig
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(EXPORT qjsConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/quickjs)
    install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
    install(DIRECTORY examples DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
