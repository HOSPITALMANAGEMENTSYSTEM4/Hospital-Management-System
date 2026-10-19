cmake_minimum_required(VERSION 3.20)

project(HospitalManagementSystem
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hms
    src/main.cpp
    src/Patient.cpp
    src/Doctor.cpp
    src/Appointment.cpp
    src/HospitalSystem.cpp
    src/FileManager.cpp
)

target_include_directories(hms PRIVATE include)

# ---- Tests ---------------------------------------------------------------
enable_testing()

add_executable(hms_tests
    tests/test_duplicate_patient_id.cpp
    src/Patient.cpp
    src/FileManager.cpp
    src/Doctor.cpp
    src/Appointment.cpp
)

target_include_directories(hms_tests PRIVATE include)

add_test(NAME TC-PAT-02_DuplicatePatientId COMMAND hms_tests)
