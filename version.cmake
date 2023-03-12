execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                OUTPUT_VARIABLE GIT_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

configure_file(${SRC} ${DST} @ONLY)

