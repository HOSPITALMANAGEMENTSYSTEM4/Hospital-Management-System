
if(NOT DEFINED HMS_EXECUTABLE)
    message(FATAL_ERROR "HMS_EXECUTABLE was not provided")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
            "PATH=$ENV{PATH}"
            "${HMS_EXECUTABLE}"
    INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/exit_input.txt"
    OUTPUT_VARIABLE hms_output
    ERROR_VARIABLE hms_error
    RESULT_VARIABLE hms_exit_code
    TIMEOUT 10
)

if(NOT "${hms_exit_code}" STREQUAL "0")
    message(FATAL_ERROR
        "Expected exit code 0, got '${hms_exit_code}'. "
        "Output: ${hms_output} ${hms_error}")
endif()

if(NOT hms_output MATCHES "Exiting Hospital Management System")
    message(FATAL_ERROR
        "Expected exit confirmation. Output: ${hms_output}")
endif()

message(STATUS "Application exited normally with exit code 0")
