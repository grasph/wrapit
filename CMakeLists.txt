# Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>

cmake_minimum_required (VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (CxxWrapGen)

include(FetchContent)

find_package(Git REQUIRED)

find_package(OpenSSL REQUIRED)

# Package to parse command line options
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.0.0
)

if(BUILD_LLVM) #in this mode llvm and clang codes are downloaded
    FetchContent_Declare(llvm
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL https://github.com/llvm/llvm-project/releases/download/llvmorg-13.0.1/llvm-13.0.1.src.tar.xz
    )
    FetchContent_GetProperties(llvm)
    if(NOT llvm_POPULATED)
        FetchContent_Populate(llvm)
    endif()

    FetchContent_Declare(clang
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm/tools/clang
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL https://github.com/llvm/llvm-project/releases/download/llvmorg-13.0.1/clang-13.0.1.src.tar.xz
    )      
    FetchContent_GetProperties(clang)
    if(NOT clang_POPULATED)
        FetchContent_Populate(clang)
    endif()
endif()

FetchContent_MakeAvailable(cxxopts)

project(MyProject)

# Option to enable LLVM/Clang download and build
option(BUILD_LLVM "Download and build LLVM/Clang" OFF)

if(BUILD_LLVM)

    function(add_llvm)
        set(CMAKE_BUILD_TYPE Release) #applies only for LLVM as defined in the function
        set(LLVM_BUILD_LLVM_DYLIB ON)
        set(LLVM_BUILD_TOOLS OFF)

       set(LLVM_TARGETS_TO_BUILD host)

        # Define a couple of LLVM cache variables of LLVM before adding the subdirectory 
        # in order to choose the default values.
        set(LLVM_TARGETS_TO_BUILD ${LLVM_TARGETS_TO_BUILD_} CACHE STRING "Semicolon-separated list of experimental targets to build.")

        set(CLANG_RESOURCE_DIR ${CMAKE_INSTALL_PREFIX}/lib/clang/13.0.1 CACHE STRING
            "Clang resource directory as returned by clang -print-resource-dir")

        add_subdirectory(${llvm_SOURCE_DIR} ${llvm_BINARY_DIR})
    endfunction()

    add_llvm()

#    include_directories(${llvm_SOURCE_DIR}/include
#                        ${clang_SOURCE_DIR}/include
#                        ${llvm_BINARY_DIR}/tools/clang/include
#                        ${llvm_BINARY_DIR}/include)

elseif (NOT CLANG_JLL)

   find_package(Clang REQUIRED CONFIG)
   find_package(LLVM REQUIRED CONFIG)
   include_directories(${CLANG_INCLUDE_DIRS})

   execute_process(COMMAND "${LLVM_TOOLS_BINARY_DIR}/clang" -print-resource-dir
                   OUTPUT_VARIABLE CLANG_RESOURCE_DIR_DISCOVERED_PATH
                   OUTPUT_STRIP_TRAILING_WHITESPACE)

   set(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR_DISCOVERED_PATH}" CACHE
      FILEPATH "Clang resource directory as returned by clang -print-resource-dir")

   if("${CLANG_RESOURCE_DIR}" STREQUAL "")
       message(FATAL_ERROR "CLANG_RESOURCE_DIR needs to be set to the Clang resource directory (see clang -print-resource-directory). This path is used at runtime by the wrapit executable.")
   else()
       if(NOT IS_DIRECTORY "${CLANG_RESOURCE_DIR}")
           message(WARNING "CLANG_RESOURCE_DIR value, " "${CLANG_RESOURCE_DIR}" ",  does not point to an existing directory.")
       endif()
   endif()

else() # Using Clang_jll
    # find_package(Clang) leads to errors with Clang_jll
    # due to inconsitency in the lists of expected and installed files

    set(CLANG_PREFIX "" CACHE FILEPATH "Root path of libclang that contains lib/libclang.so.")
    if("${CLANG_PREFIX}" STREQUAL "")
        find_library(LIBCLANG_PATH_ clang REQUIRED NOCACHE)
        get_filename_component(CLANG_PREFIX "${LIBCLANG_PATH_}" DIRECTORY)
        get_filename_component(CLANG_PREFIX "${CLANG_PREFIX}" DIRECTORY)
        unset(LIBCLANG_PATH_)
        message(STATUS "CLANG_PREFIX: ${CLANG_PREFIX}")
    endif()
    include_directories("${CLANG_PREFIX}/include")

    set(CLANG_RESOURCE_DIR "" CACHE STRING
        "Clang resource directory as returned by clang -print-resource-dir")
    if("${CLANG_RESOURCE_DIR}" STREQUAL "")
        message("Searching clang resource directory in " "${CLANG_PREFIX}/lib/clang")
        file(GLOB CLANG_RESOURCE_DIR_ "${CLANG_PREFIX}/lib/clang/*/include/stddef.h")
        if("${CLANG_RESOURCE_DIR_}" STREQUAL "")
            message(FATAL_ERROR "Failed to find Clang resource directory")
        else()
            list(GET CLANG_RESOURCE_DIR_ 0 CLANG_RESOURCE_DIR)
            unset(CLANG_RESOURCE_DIR_)
            get_filename_component(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR}" DIRECTORY)
            get_filename_component(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR}" DIRECTORY)
            message(STATUS "CLANG_RESOURCE_DIR: ${CLANG_RESOURCE_DIR}")
        endif()
    endif()

    set(SHARED_LIBS libclang clang-cpp LLVM)
    foreach(shared_lib IN LISTS SHARED_LIBS)
        set(lib_path "${CLANG_PREFIX}/lib/lib${shared_lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        string(REPLACE liblib lib lib_path ${lib_path})
        if(EXISTS "${lib_path}")
            add_library(${shared_lib} SHARED IMPORTED)
        else()
            set(lib_path "${CLANG_PREFIX}/lib/lib${shared_lib}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            string(REPLACE liblib lib lib_path ${lib_path})
            if(EXISTS "${lib_path}")
                add_library(${shared_lib} STATIC IMPORTED)
            else()
                message(FATAL_ERROR "${shared_lib} library was not found in ${CLANG_PREFIX}/lib")
            endif()
        endif()
        set_property(TARGET ${shared_lib} PROPERTY
                     IMPORTED_LOCATION ${lib_path})
    endforeach()

    #To prevent 'undefined symbol: _ZN4llvm23EnableABIBreakingChecksE' error:
    add_compile_definitions(LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING)

    if(NOT ($ENV{target} MATCHES "darwin"))
        execute_process(COMMAND /bin/sh -c "nm -D -C ${CLANG_PREFIX}/lib/libclang.so | grep -q abi:cxx11" RESULT_VARIABLE rc)
        if(NOT (rc EQUAL 0)) #libclang.so compiled with cxx03 ABI
            add_compile_options(-D_GLIBCXX_USE_CXX11_ABI=0)
        endif()
    endif()
endif()

add_custom_target(version
    ${CMAKE_COMMAND}
    -D SRC=${CMAKE_SOURCE_DIR}/src/version.cpp.in
    -D DST=${CMAKE_BINARY_DIR}/version.cpp
    -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
    -P ${CMAKE_SOURCE_DIR}/version.cmake
    DEPENDS src/version.cpp.in
   )

# The target 'versions' is used to produce version.cpp
# This dummy custom command is defined to allow
# inclusion of version.cpp dependency to the
# wrapit executable.
add_custom_command(OUTPUT version.cpp
   COMMAND true
   )

add_executable(wrapit
    src/TypeRcd.cpp
    src/TypeMapper.cpp
    src/utils.cpp
    src/cxxwrap_version.cpp
    src/uuid_utils.cpp
    src/libclang-ext.cpp
    src/FunctionWrapper.cpp
    src/CodeTree.cpp
    src/main.cpp
    src/toml.hpp
    src/md5sum.cpp
    src/FileTimeRestorer.cpp
    src/Graph.cpp
    version.cpp
)

add_dependencies(wrapit version)


if(BUILD_LLVM)
    if(APPLE)
       set_target_properties(wrapit PROPERTIES INSTALL_RPATH @loader_path/../lib)
    else()
       set_target_properties(wrapit PROPERTIES INSTALL_RPATH $ORIGIN/../lib)
    endif()

    target_include_directories(wrapit PRIVATE
        ${llvm_SOURCE_DIR}/include
        ${clang_SOURCE_DIR}/include
        ${llvm_BINARY_DIR}/tools/clang/include
        ${llvm_BINARY_DIR}/include)
endif()

target_link_libraries(wrapit PRIVATE libclang clang-cpp LLVM cxxopts dl
                      OpenSSL::Crypto
                      #- gcc < 9.0 needs std++fs for the std::filesystem support
                      $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
)
set_target_properties(wrapit PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME wrapit
    POSITION_INDEPENDENT_CODE ON #required for dladdrr on Linux (*)
    )

configure_file(src/config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR}) #contains the generated config.h header file

target_compile_definitions(wrapit PRIVATE)

install(TARGETS wrapit DESTINATION bin)

#(*) dladdr man page says "On some architectures (notably i386 and x86-64),
#       dli_fname and dli_fbase may end up pointing back at the object from
#       which you called dladdr(), even if the function used as an argument
#       should come from a dynamically linked library." and prescribe to
#       compile the code as position independent to work around this bug.
# See also https://github.com/grasph/wrapit/issues/30

# Enable unit testing capabilities
include(CTest)
add_test(NAME ex001-HelloWorld WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/ex001-HelloWorld COMMAND julia --project=. runtests.jl)
add_test(NAME unittests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test COMMAND julia --project=. runtests.jl)

add_subdirectory(test)
add_subdirectory(examples)
