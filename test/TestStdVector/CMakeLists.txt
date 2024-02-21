cmake_minimum_required(VERSION 3.12)

project(TestStdVector)

set(WRAPPER_EXTRA_SRCS A.cxx)

# All of the real work is done in the lower level CMake file
include(../WrapitTestSetup.cmake)
