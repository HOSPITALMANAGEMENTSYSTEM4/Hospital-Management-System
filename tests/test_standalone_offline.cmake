if(NOT DEFINED HMS_EXECUTABLE OR NOT EXISTS "${HMS_EXECUTABLE}")
    message(FATAL_ERROR "HMS executable is missing")
endif()

if(NOT DEFINED OBJDUMP OR NOT EXISTS "${OBJDUMP}")
    message(FATAL_ERROR "objdump is required for runtime dependency verification")
endif()

execute_process(
    COMMAND "${OBJDUMP}" -p "${HMS_EXECUTABLE}"
    RESULT_VARIABLE inspect_result
    OUTPUT_VARIABLE imports
    ERROR_VARIABLE inspect_error
    TIMEOUT 10
)

if(NOT "${inspect_result}" STREQUAL "0")
    message(FATAL_ERROR "DLL inspection failed: ${inspect_error}")
endif()

# Windows static-runtime check: these are Windows DLL imports.
# Linux ELF executables legitimately report shared-library dependencies.
if(CMAKE_HOST_WIN32)
    string(TOLOWER "${imports}" imports_lower)

    if(imports_lower MATCHES "libwinpthread" OR
       imports_lower MATCHES "libstdc" OR
       imports_lower MATCHES "libgcc_s")
        message(FATAL_ERROR
            "Unexpected GCC runtime DLL import detected: ${imports}")
    endif()
endif()

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
    "Standalone runtime imports and application startup/exit verified")
