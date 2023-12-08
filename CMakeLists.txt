# Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>

cmake_minimum_required (VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (CxxWrapGen)

include(FetchContent)

find_package(Git)

# Package to parse command line options
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.0.0
)
FetchContent_MakeAvailable(cxxopts)

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(llvm_libs core)
include_directories(${CLANG_INCLUDE_DIRS})
if(DEFINED CLANG_RESOURCE_DIR AND NOT CLANG_RESOURCE_DIR STREQUAL "")
   set(CLANG_RESOURCE_DIR_PATH "${LLVM_TOOLS_BINARY_DIR}/${CLANG_RESOURCE_DIR}")
else()
   set(CLANG_RESOURCE_DIR_PATH "${LLVM_LIBRARY_DIR}/clang/${CLANG_VERSION_MAJOR}")
endif()

execute_process(COMMAND "${LLVM_TOOLS_BINARY_DIR}/clang" -print-resource-dir
               OUTPUT_VARIABLE CLANG_RESOURCE_DIR_DISCOVERED_PATH
               OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CLANG_RESOURCE_DIR_FULLPATH "${CLANG_RESOURCE_DIR_DISCOVERED_PATH}" CACHE
    FILEPATH
    "Clang resource directory as returned by clang -print-resource-dir")

if("${CLANG_RESOURCE_DIR_FULLPATH}" STREQUAL "")
  message(FATAL_ERROR "CLANG_RESOURCE_DIR_FULLPATH needs to be set to the Clang resource directory (see clang -resource-directory). This path is used at runtime by the wrapit executable.")
else()
  if(NOT IS_DIRECTORY "${CLANG_RESOURCE_DIR_FULLPATH}")
    message(WARNING "CLANG_RESOURCE_DIR_FULLPATH value, " "${CLANG_RESOURCE_DIR_FULLPATH}" ",  does not point to an existing directory.")
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
   src/libclang-ext.cpp
   src/FunctionWrapper.cpp
   src/CodeTree.cpp
   src/main.cpp
   src/toml.hpp
   version.cpp
)

add_dependencies(wrapit version)

target_link_libraries(wrapit PRIVATE libclang clang-cpp ${llvm_libs} cxxopts dl)
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
