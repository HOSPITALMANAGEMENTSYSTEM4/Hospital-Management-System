
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

# Patient: duplicate ID
add_executable(hms_tests
    tests/test_duplicate_patient_id.cpp
    src/Patient.cpp
    src/FileManager.cpp
    src/Doctor.cpp
    src/Appointment.cpp
)

target_include_directories(hms_tests PRIVATE include)

add_test(
    NAME TC-PAT-02_DuplicatePatientId
    COMMAND hms_tests
)

# Patient: field validation
add_executable(hms_validate_tests
    tests/test_validate_patient_fields.cpp
    src/Patient.cpp
    src/FileManager.cpp
    src/Doctor.cpp
    src/Appointment.cpp
)

target_include_directories(hms_validate_tests PRIVATE include)

add_test(
    NAME TC-PAT-03_ValidatePatientFields
    COMMAND hms_validate_tests
)

# Doctor: add doctor
add_executable(hms_add_doctor_tests
    tests/test_add_doctor.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
    src/Appointment.cpp
)

target_include_directories(hms_add_doctor_tests PRIVATE include)

add_test(
    NAME TC-DOC-01_AddDoctor
    COMMAND hms_add_doctor_tests
)

# Doctor: reject duplicate ID
add_executable(hms_duplicate_doctor_tests
    tests/test_duplicate_doctor_id.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
    src/Appointment.cpp
)

target_include_directories(hms_duplicate_doctor_tests PRIVATE include)

add_test(
    NAME TC-DOC-02_RejectDuplicateDoctorId
    COMMAND hms_duplicate_doctor_tests
)

# Doctor: field validation
add_executable(hms_validate_doctor_tests
    tests/test_validate_doctor_fields.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/Patient.cpp
    src/Appointment.cpp
)

target_include_directories(hms_validate_doctor_tests PRIVATE include)

add_test(
    NAME TC-DOC-03_ValidateDoctorFields
    COMMAND hms_validate_doctor_tests
)

# Patient: search
add_executable(hms_search_patients_tests
    tests/test_search_patients.cpp
    src/Patient.cpp
    src/Doctor.cpp
    src/Appointment.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
)

target_include_directories(hms_search_patients_tests PRIVATE include)

add_test(
    NAME TC-PAT-04_SearchPatients
    COMMAND hms_search_patients_tests
)
# Patient: view details and list patients
add_executable(hms_patient_details_list_tests
    tests/test_patient_details_and_list.cpp
    src/Patient.cpp
    src/Doctor.cpp
    src/Appointment.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
)

target_include_directories(hms_patient_details_list_tests PRIVATE include)

add_test(
    NAME TC-PAT-05_ViewDetailsAndList
    COMMAND hms_patient_details_list_tests
)