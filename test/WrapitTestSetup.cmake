# Standard pieces for setting up WrapIt tests with CMake

# This files needs:
#  - wrapit executable, which will be looked for in the standard program path.
# Its location can be specified with -DWRAPIT=...
#
# Assumptions:
#
# - the test script is named run<PROJECT_NAME>.jl, with <PROJECT_NAME> the cmake projet name
# - the wit file is named <PROJECT_NAME>.wit
# - the module_name defined in the wit file is the same as <PROJECT_NAME>
# - the produced wrapper shared library must be named libjl<PROJECT_NAME>, placed in the
#   <PROJECT_NAME>/deps folder and the lib_basename wit parameter is set to
#   "$(@__DIR__)/../deps/libjl<PROJECT_NAME>
#
# Note @PROJECT_NAME@ can be usd in the wit.in place of the explicit project and module name.
# It will be replaced by the value define by the project() CMakeLists.txt statement when generating
# the actual .wit file from the .wit.in

# julia is used to retrieve the CxxWrap library paths
find_program(JULIA julia REQUIRED)

# Some standard options and paths
set(CMAKE_MACOSX_RPATH 1)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}/deps")

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS OFF)

# Wrapit verbosity
set(WRAPIT_VERBOSITY 1 CACHE STRING "Define verbosity level of the wrapit command. An integer, 0 for a quiet mode, 1 for a normal mode, higher numbers for a verbosity that increases with the number.")

# Find wrapit command (https://github.com/grasph/wrapit), used to generate
# the wrapper code.
find_program(WRAPIT wrapit DOC "wrapit command path")
#FIXME: use REQUIRE option of find_program when migrating to cmake >= 3.18
if(WRAPIT STREQUAL WRAPIT-NOTFOUND)
  message(FATAL_ERROR "Failed to find wrapit executable - aborting")
else()
  message("Found wrapit executable: ${WRAPIT}")
endif()

# Define target file names based on the project name for the test
# Then write the .wit file in the build area
# This then makes sure we find the correct header from the example sources
set(WRAPIT_WIT_FILE "${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.wit")
set(WRAPPER_LIB "jl${CMAKE_PROJECT_NAME}")
set(WRAPPER_JULIA_PACKAGE_DIR "${CMAKE_PROJECT_NAME}")
set(WRAPPER_JULIA_PACKAGE_FILE "${CMAKE_PROJECT_NAME}.jl")

# julia is used to retrieve the CxxWrap library paths
find_program(JULIA julia REQUIRED)

message(STATUS "Wrapit configuration file: ${WRAPIT_WIT_FILE}")

execute_process(
  COMMAND ${JULIA} "--project=${CMAKE_BINARY_DIR}" -e "import TOML; print(get(TOML.parse(open(\"${WRAPIT_WIT_FILE}\")), \"cxxwrap_version\", \"\"));"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE CXXWRAP_REQUESTED_VERSION
  RESULT_VARIABLE result
  )

if(NOT result EQUAL 0)
  message(FATAL_ERROR "Failed to parse ${WRAPIT_WIT_FILE}")
endif()

if("${CXXWRAP_REQUESTED_VERSION}" STREQUAL "")
  execute_process(
    COMMAND ${JULIA} --project=${CMAKE_BINARY_DIR} -e "import Pkg; Pkg.add(\"CxxWrap\"); Pkg.resolve(); import CxxWrap; print(pkgversion(CxxWrap));"
    OUTPUT_VARIABLE CXXWRAP_INSTALLED_VERSION
    RESULT_VARIABLE result
  )
  #if no CxxWrap version requirement was specified in the .wit file,
  #we align it to the version that was installed
  set(WRAPIT_OPT --add-cfg "cxxwrap_version=\"${CXXWRAP_INSTALLED_VERSION}\"")
  message(STATUS ${WRAPIT})
else()
  execute_process(
    COMMAND ${JULIA} --project=${CMAKE_BINARY_DIR} -e "import Pkg; Pkg.add(name=\"CxxWrap\", version=\"${CXXWRAP_REQUESTED_VERSION}\"); Pkg.resolve(); import CxxWrap; print(pkgversion(CxxWrap));"
    OUTPUT_VARIABLE CXXWRAP_INSTALLED_VERSION
    RESULT_VARIABLE result)
endif()

if(NOT result EQUAL 0)
  message(FATAL_ERROR "Failed to install CxxWrap")
elseif("${CXXWRAP_REQUESTED_VERSION}" STREQUAL "")
  message(STATUS "CxxWrap version requested to be compatible with any version, using v${CXXWRAP_INSTALLED_VERSION}")
else()
  message(STATUS "CxxWrap version requested to be compatible with ${CXXWRAP_REQUESTED_VERSION}, using version: ${CXXWRAP_INSTALLED_VERSION}")
endif()

execute_process(
  COMMAND "${JULIA}" --project=${CMAKE_BINARY_DIR} -e "import CxxWrap; print(CxxWrap.prefix_path())"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE CXXWRAP_PREFIX
  RESULT_VARIABLE result)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "Failed to retrieve CxxWrap library path")
else()
  message(STATUS "CxxWrap library path prefix: ${CXXWRAP_PREFIX}")
endif()

#This find_package(JlCxx...) command modifies CMAKE_CXX_STANDARD.
#So we save our value and restore it.
set(SAVED_CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD})
find_package(JlCxx PATHS ${CXXWRAP_PREFIX})
set(CMAKE_CXX_STANDARD ${SAVED_CMAKE_CXX_STANDARD})

get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")


#configure_file(${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.wit.in ${CMAKE_BINARY_DIR}/${WRAPIT_WIT_FILE} @ONLY)

# Generate the wrapper code. This is done at configure time.
execute_process(
  COMMAND "${WRAPIT}" ${WRAPIT_OPT} -v "${WRAPIT_VERBOSITY}" --force --update --cmake --output-prefix "${CMAKE_BINARY_DIR}" "${WRAPIT_WIT_FILE}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMAND_ECHO STDERR
  RESULT_VARIABLE result)

if(NOT result EQUAL 0)
  message(FATAL_ERROR "Execution of wrapit failed")
endif()

# File generated by wrapit that defines two variables, WRAPIT_PRODUCTS and
# WRAPIT_INPUT, with respectively the list of produced c++ code file and
# the list of files their contents on.
include("${CMAKE_BINARY_DIR}/wrapit.cmake")

# Require reconfiguration if one of the dependency of the contents produced
# by wrapit (itself executed at configure step) changed:
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${WRAPIT_DEPENDS}" "${WRAPIT_WIT_FILE}")

# Build the library.
add_library(${WRAPPER_LIB} SHARED ${WRAPIT_PRODUCTS} ${WRAPPER_EXTRA_SRCS})
set_target_properties(${WRAPPER_LIB}
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${WRAPPER_JULIA_PACKAGE_DIR}/deps)
target_include_directories(${WRAPPER_LIB} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${WRAPPER_LIB} JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl) 

# Installation paths:
set(WRAPPER_INSTALL_DIR "share/wrapit" CACHE FILEPATH "Installation path for the test modules")
install(FILES ${CMAKE_BINARY_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/src/${WRAPPER_JULIA_PACKAGE_FILE}
  DESTINATION ${WRAPPER_INSTALL_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/src)
install(TARGETS ${WRAPPER_LIB}
  LIBRARY DESTINATION  ${WRAPPER_INSTALL_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/deps
  ARCHIVE DESTINATION ${WRAPPER_INSTALL_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/deps 
  RUNTIME DESTINATION ${WRAPPER_INSTALL_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/deps)

# Help function to lower the case of the initial letter
# of a character string.
# input: the character string to transform
# output_var: the name of the variable to store the result
function(uncapitalize input output_var)
    string(SUBSTRING ${input} 0 1 FIRST_LETTER)
    string(SUBSTRING ${input} 1 -1 REMAINING_STRING)
    string(TOLOWER ${FIRST_LETTER} FIRST_LETTER_LOWER)
    set(${output_var} "${FIRST_LETTER_LOWER}${REMAINING_STRING}" PARENT_SCOPE)
endfunction()

# Setup the CTest
enable_testing()
uncapitalize(${PROJECT_NAME} TEST_SCRIPT)
set(TEST_SCRIPT "${CMAKE_SOURCE_DIR}/${TEST_SCRIPT}.jl")
set(TEST_NAME ${CMAKE_PROJECT_NAME})
add_test(NAME ${TEST_NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND env JULIA_LOAD_PATH=:${CMAKE_BINARY_DIR}/${WRAPPER_JULIA_PACKAGE_DIR}/src julia --project=${CMAKE_BINARY_DIR} "${TEST_SCRIPT}")
