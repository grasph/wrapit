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
FetchContent_MakeAvailable(cxxopts)

# CLANG_JLL is used by Binary Builder
# For building the package stand-alone it is not used
if (NOT CLANG_JLL)

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
   # due to inconsitency between the lists expected and installed files

   file(GLOB CLANG_RESOURCE_DIR_ "${CMAKE_INSTALL_PREFIX}/lib/clang/*/include/stddef.h")
   list(GET CLANG_RESOURCE_DIR_ 0 CLANG_RESOURCE_DIR)
   message("Searching in " "${CMAKE_INSTALL_PREFIX}/lib/clang")

   cmake_path(REMOVE_FILENAME CLANG_RESOURCE_DIR)
   cmake_path(RELATIVE_PATH CLANG_RESOURCE_DIR
              BASE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/lib")

   set(CLANG_RESOURCE_DIR ${CLANG_RESOURCE_DIR} CACHE STRING
       "Clang resource directory as returned by clang -print-resource-dir")

   set(SHARED_LIBS libclang clang-cpp LLVM)
   foreach(shared_lib IN LISTS SHARED_LIBS) 
       add_library(${shared_lib} SHARED IMPORTED)
       set(lib_path "${CMAKE_INSTALL_PREFIX}/lib/lib${shared_lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
       string(REPLACE liblib lib lib_path ${lib_path})
       set_property(TARGET ${shared_lib} PROPERTY
                    IMPORTED_LOCATION ${lib_path})
   endforeach()

   #To prevent 'undefined symbol: _ZN4llvm23EnableABIBreakingChecksE' error:
   add_compile_definitions(LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING)

   if(NOT ($ENV{target} MATCHES "darwin"))
      execute_process(COMMAND /bin/sh -c "nm -C \$prefix/lib/libclang.so | grep -q abi:cxx11" RESULT_VARIABLE rc)
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

target_link_libraries(wrapit PRIVATE libclang clang-cpp LLVM cxxopts dl
                      OpenSSL::Crypto)
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
