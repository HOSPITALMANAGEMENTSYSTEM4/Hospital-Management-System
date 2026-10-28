
if(NOT DEFINED HMS_EXECUTABLE OR NOT EXISTS "${HMS_EXECUTABLE}")
    message(FATAL_ERROR "HMS executable is missing")
endif()

# Inspect runtime dependencies when a compatible objdump is available.
set(imports "")
set(inspect_result "NOT_AVAILABLE")
set(inspect_error "")

if(DEFINED OBJDUMP AND EXISTS "${OBJDUMP}")
    execute_process(
        COMMAND "${OBJDUMP}" -p "${HMS_EXECUTABLE}"
        RESULT_VARIABLE inspect_result
        OUTPUT_VARIABLE imports
        ERROR_VARIABLE inspect_error
        TIMEOUT 10
    )
endif()

if("${inspect_result}" STREQUAL "0")
    # Reject unexpected GCC runtime DLL dependencies on Windows.
    if(CMAKE_HOST_WIN32)
        string(TOLOWER "${imports}" imports_lower)

        if(imports_lower MATCHES "libwinpthread" OR
           imports_lower MATCHES "libstdc" OR
           imports_lower MATCHES "libgcc_s")
            message(FATAL_ERROR
                "Unexpected GCC runtime DLL import detected: ${imports}")
        endif()
    endif()
else()
    message(STATUS
        "DLL inspection unavailable with the configured objdump; "
        "continuing with application startup/exit checks.")
endif()

# Verify that the application starts and exits normally.
execute_process(
    COMMAND "${HMS_EXECUTABLE}"
    INPUT_FILE "${CMAKE_CURRENT_LIST_DIR}/exit_input.txt"
    OUTPUT_VARIABLE hms_output
    ERROR_VARIABLE hms_error
    RESULT_VARIABLE hms_exit_code
    TIMEOUT 10
)

if(NOT "${hms_exit_code}" STREQUAL "0")
    message(FATAL_ERROR
        "Application failed to exit normally: ${hms_exit_code}. "
        "${hms_output} ${hms_error}")
endif()

if(NOT hms_output MATCHES "Exiting Hospital Management System")
    message(FATAL_ERROR "Expected exit confirmation: ${hms_output}")
endif()

message(STATUS
    "Application startup and clean exit verified")
