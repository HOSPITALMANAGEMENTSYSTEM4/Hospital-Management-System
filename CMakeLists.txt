
cmake_minimum_required(VERSION 3.20)

project(HospitalManagementSystem
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MINGW OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND WIN32))
    add_link_options(-static-libgcc -static-libstdc++)
endif()

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

# Doctor: search doctors
add_executable(hms_search_doctors_tests
    tests/test_search_doctors.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
    src/Appointment.cpp
)

target_include_directories(hms_search_doctors_tests PRIVATE include)

add_test(
    NAME TC-DOC-04_SearchDoctors
    COMMAND hms_search_doctors_tests
)

add_executable(hms_search_patients_tests
    tests/test_search_patients.cpp
    src/Patient.cpp
    src/Doctor.cpp
    src/Appointment.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
)

target_include_directories(hms_search_patients_tests PRIVATE include)

add_test(NAME TC-PAT-04_SearchPatients COMMAND hms_search_patients_tests)

add_executable(hms_reject_unknown_patient_tests
    tests/test_reject_unknown_patient.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_reject_unknown_patient_tests PRIVATE include)

add_test(
    NAME TC-APT-02_RejectUnknownPatient
    COMMAND hms_reject_unknown_patient_tests
)

add_executable(hms_book_appointment_tests
    tests/test_book_appointment.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_book_appointment_tests PRIVATE include)

add_test(
    NAME TC-APT-01_BookAppointment
    COMMAND hms_book_appointment_tests
)

# Appointment: validate date and time (Issue #14, HMS-F-018, TC-APT-04)
add_executable(hms_validate_appointment_datetime_tests
    tests/test_validate_appointment_date_time.cpp
    src/Appointment.cpp
    src/FileManager.cpp
    src/Patient.cpp
    src/Doctor.cpp
)

target_include_directories(hms_validate_appointment_datetime_tests PRIVATE include)

add_test(
    NAME TC-APT-04_ValidateAppointmentDateTime
    COMMAND hms_validate_appointment_datetime_tests
)

# Appointment: reject unknown doctor (Issue #13)
add_executable(hms_reject_unknown_doctor_tests
    tests/test_reject_unknown_doctor.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_reject_unknown_doctor_tests PRIVATE include)

add_test(
    NAME TC-APT-03_RejectUnknownDoctor
    COMMAND hms_reject_unknown_doctor_tests
)

# Doctor: view details and list doctors (TC-DOC-05)
add_executable(hms_view_list_doctors_tests
    tests/test_view_list_doctors.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_view_list_doctors_tests PRIVATE include)

add_test(
    NAME TC-DOC-05_ViewAndListDoctors
    COMMAND hms_view_list_doctors_tests
)

# Startup CSV loading acceptance test (Issue #15 / TC-PER-01)
add_executable(hms_startup_csv_loading_tests
    tests/test_startup_csv_loading.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)
target_include_directories(hms_startup_csv_loading_tests PRIVATE include)
add_test(NAME TC-PER-01_StartupCSVLoading COMMAND hms_startup_csv_loading_tests)
