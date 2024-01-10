cmake_minimum_required(VERSION 3.12)

project(TestStdSring)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

find_package(JlCxx)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

message(STATUS "Found JlCxx at ${JlCxx_location}")

# Write the .wit file in the build area
# This then makes sure we find the correct header from the example sources
configure_file(${CMAKE_SOURCE_DIR}/TestStdString.wit.in ${CMAKE_BINARY_DIR}/TestStdString.wit @ONLY)

# And similar for the setup script wrapper
# configure_file(${CMAKE_SOURCE_DIR}/setup.sh.in ${CMAKE_BINARY_DIR}/setup.sh @ONLY)

# Generate the wrapper
find_program(WRAPIT_EXECUTABLE wrapit)
if(WRAPIT_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/libTestStdString/src/jlTestStdString.cxx
        COMMAND ${WRAPIT_EXECUTABLE} --force -v 1 ${CMAKE_BINARY_DIR}/TestStdString.wit
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${CMAKE_SOURCE_DIR}/TestStdString.wit
    )
else()
    message(FATAL_ERROR "Failed to find wrapit executable - aborting")
endif()

# Build the library
add_library(jlTestStdString SHARED ${CMAKE_BINARY_DIR}/libTestStdString/src/jlTestStdString.cxx)
target_include_directories(jlTestStdString PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(jlTestStdString JlCxx::cxxwrap_julia)

install(TARGETS
    jlTestStdString
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib
RUNTIME DESTINATION lib)
