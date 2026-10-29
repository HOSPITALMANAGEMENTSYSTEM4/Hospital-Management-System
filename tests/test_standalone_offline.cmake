
if(NOT DEFINED HMS_EXECUTABLE OR NOT EXISTS "${HMS_EXECUTABLE}")
    message(FATAL_ERROR "HMS executable is missing")
endif()

# Issue #54: HMS must not use network APIs or transmit HMS data.
# Static source inspection complements the executable dependency check.
set(source_root "${CMAKE_CURRENT_LIST_DIR}/../src")
set(include_root "${CMAKE_CURRENT_LIST_DIR}/../include")

file(GLOB_RECURSE hms_source_files
    "${source_root}/*.cpp"
    "${source_root}/*.h"
    "${source_root}/*.hpp"
    "${include_root}/*.cpp"
    "${include_root}/*.h"
    "${include_root}/*.hpp"
)

set(network_api_pattern
    "wsastartup|wsacleanup|socket[ \t]*\\(|connect[ \t]*\\(|send[ \t]*\\(|recv[ \t]*\\(|winhttp|wininet|internetopen|internetconnect|urldownloadtofile|curl_easy_|libcurl|boost::asio|qnetworkaccessmanager|ws2_32|wsock32|http://|https://|ftp://"
)

foreach(source_file IN LISTS hms_source_files)
    file(READ "${source_file}" source_contents)
    string(TOLOWER "${source_contents}" source_lower)

    if(source_lower MATCHES "${network_api_pattern}")
        message(FATAL_ERROR
            "Potential network API or endpoint found in ${source_file}")
    endif()
endforeach()

message(STATUS "Source network API audit passed")

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
    string(TOLOWER "${imports}" imports_lower)

    if(imports_lower MATCHES
       "ws2_32\\.dll|wsock32\\.dll|winhttp\\.dll|wininet\\.dll|urlmon\\.dll|libcurl")
        message(FATAL_ERROR
            "Potential network library dependency detected: ${imports}")
    endif()

    if(CMAKE_HOST_WIN32)
        if(imports_lower MATCHES "libwinpthread|libstdc|libgcc_s")
            message(FATAL_ERROR
                "Unexpected GCC runtime DLL import detected: ${imports}")
        endif()
    endif()

    message(STATUS "Executable dependency audit passed")
else()
    message(STATUS
        "Dependency inspection unavailable; source audit and startup/exit "
        "checks will still run.")
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
    message(FATAL_ERROR
        "Expected exit confirmation: ${hms_output}")
endif()

message(STATUS
    "Offline source/dependency checks and clean exit verified")
