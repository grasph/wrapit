# Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>

cmake_minimum_required (VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (CxxWrapGen)

include(FetchContent)

find_package(Git)

#Package to parse command line options
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        v3.0.0
)
FetchContent_MakeAvailable(cxxopts)

set(SEARCH_CLANG_CMAKE TRUE
    CACHE BOOL "If true use the Clang cmake configuration files found on the system.")

if(NOT CLANG_JLL)
   find_package(Clang REQUIRED CONFIG)
   include_directories(${CLANG_INCLUDE_DIRS})
else() #Using Clang_jll
   #Clang_jll package lacks the Clang Cmake conifiguration file,
   #Set the clang dependency by  hand:
   set(STATIC_LIBS clangAST clangLex clangBasic)
   foreach(static_lib IN LISTS STATIC_LIBS) 
       add_library(${static_lib} STATIC IMPORTED)
       set_property(TARGET ${static_lib} PROPERTY
                    IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/lib/lib${static_lib}${CMAKE_STATIC_LIBRARY_SUFFIX}")
   endforeach()
   
   set(SHARED_LIBS libclang LLVM)
   foreach(shared_lib IN LISTS SHARED_LIBS) 
       add_library(${shared_lib} SHARED IMPORTED)
       set(lib_path "${CMAKE_INSTALL_PREFIX}/lib/lib${shared_lib}${CMAKE_SHARED_LIBRARY_SUFFIX}")
       string(REPLACE liblib lib lib_path ${lib_path})
       set_property(TARGET ${shared_lib} PROPERTY
                    IMPORTED_LOCATION ${lib_path})
   endforeach()
   
   set_target_properties(clangBasic PROPERTIES
                         INTERFACE_LINK_LIBRARIES "LLVM")
   
   set_target_properties(clangLex PROPERTIES
                         INTERFACE_LINK_LIBRARIES "clangBasic;LLVM")
   
   set_target_properties(clangAST PROPERTIES
                         INTERFACE_LINK_LIBRARIES "clangBasic;clangLex;LLVM")

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

add_executable(wrapit
   src/TypeRcd.cpp
   src/utils.cpp
   src/libclang-ext.cpp
   src/FunctionWrapper.cpp
   src/CodeTree.cpp
   src/main.cpp
   src/toml.hpp
)

add_dependencies(wrapit version)
target_sources(wrapit PRIVATE ${CMAKE_BINARY_DIR}/version.cpp)

target_link_libraries(wrapit PRIVATE libclang clangAST cxxopts)
set_target_properties(wrapit PROPERTIES CXX_STANDARD 17 OUTPUT_NAME wrapit)
install(TARGETS wrapit DESTINATION bin)
