## Standard pieces for setting up WrapIt tests with CMake

# Some standard options and paths
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Loaction of JLCxx - this should be injected via DCMAKE_PREFIX_PATH
# from the CxxWrap.prefix_path() in Julia
find_package(JlCxx)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

# Define target file names based on the project name for the test
# Then write the .wit file in the build area
# This then makes sure we find the correct header from the example sources
set(WRAPIT_TEST_WIT_FILE "${CMAKE_PROJECT_NAME}.wit")
set(WRAPIT_TEST_LIB_FILE "jl${CMAKE_PROJECT_NAME}")
set(WRAPIT_TEST_CXX_FILE "jl${CMAKE_PROJECT_NAME}.cxx")
set(WRAPIR_TEST_LIB_DIR "lib${CMAKE_PROJECT_NAME}")
configure_file(${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.wit.in ${CMAKE_BINARY_DIR}/${WRAPIT_TEST_WIT_FILE} @ONLY)

# Generate the wrapper
find_program(WRAPIT_EXECUTABLE wrapit)
if(WRAPIT_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${WRAPIR_TEST_LIB_DIR}/src/${WRAPIT_TEST_CXX_FILE}
        COMMAND ${WRAPIT_EXECUTABLE} --force -v 1 ${CMAKE_BINARY_DIR}/${WRAPIT_TEST_WIT_FILE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${CMAKE_BINARY_DIR}/${WRAPIT_TEST_WIT_FILE}
    )
else()
    message(FATAL_ERROR "Failed to find wrapit executable - aborting")
endif()

# Build the library
add_library(${WRAPIT_TEST_LIB_FILE} SHARED ${CMAKE_BINARY_DIR}/${WRAPIR_TEST_LIB_DIR}/src/${WRAPIT_TEST_CXX_FILE})
target_include_directories(${WRAPIT_TEST_LIB_FILE} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${WRAPIT_TEST_LIB_FILE} JlCxx::cxxwrap_julia)

# Install the library
install(TARGETS
    ${WRAPIT_TEST_LIB_FILE}
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)
