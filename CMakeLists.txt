# Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>

cmake_minimum_required (VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (CxxWrapGen)

include(FetchContent)

#Package to parse command line options
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.0.0
)
FetchContent_MakeAvailable(cxxopts)

find_package(Clang REQUIRED CONFIG)
include_directories(${CLANG_INCLUDE_DIRS})

add_executable(wrapit src/TypeRcd.cpp src/utils.cpp  src/libclang-ext.cpp src/FunctionWrapper.cpp src/CodeTree.cpp src/main.cpp src/toml.hpp)
target_link_libraries(wrapit PRIVATE libclang clangAST cxxopts)
set_target_properties(wrapit PROPERTIES CXX_STANDARD 17 OUTPUT_NAME wrapit)
install(TARGETS wrapit DESTINATION bin)
