#!/usr/bin/env julia

TEST_SCRIPT="testStdString.jl"

# Generate the wrapper and build the shared library:
run(`cmake -B build`)
run(`cmake --build build`)
push!(LOAD_PATH, "build")

# Execute the test
include(TEST_SCRIPT)
