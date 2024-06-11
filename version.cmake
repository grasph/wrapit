execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
                WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                OUTPUT_VARIABLE GIT_REF
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

# If we're not on a tag, use the commit hash
if(NOT GIT_REF)
  execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --dirty
                  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                  OUTPUT_VARIABLE GIT_REF
                  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()

configure_file(${SRC} ${DST} @ONLY)
