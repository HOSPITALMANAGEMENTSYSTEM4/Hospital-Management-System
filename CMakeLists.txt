
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

# Patient: search patients
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

# Appointment: reject unknown patient
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

# Appointment: book appointment
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

# Appointment: check doctor availability (Issue #24, HMS-F-020, TC-APT-06)
add_executable(hms_check_doctor_availability_tests
    tests/test_check_doctor_availability.cpp
# Appointment: prevent double-booking (Issue #23, HMS-F-019, TC-APT-05, TC-NFR-08)
add_executable(hms_prevent_double_booking_tests
    tests/test_prevent_double_booking.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_check_doctor_availability_tests PRIVATE include)

add_test(
    NAME TC-APT-06_CheckDoctorAvailability
    COMMAND hms_check_doctor_availability_tests
target_include_directories(hms_prevent_double_booking_tests PRIVATE include)

add_test(
    NAME TC-APT-05_TC-NFR-08_PreventDoubleBooking
    COMMAND hms_prevent_double_booking_tests
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

add_test(
    NAME TC-PER-01_StartupCSVLoading
    COMMAND hms_startup_csv_loading_tests
)

# Missing CSV files (Issue #16)
add_executable(hms_missing_csv_files_tests
    tests/test_missing_csv_files.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_missing_csv_files_tests PRIVATE include)

add_test(
    NAME TC-PER-02_MissingCSVFiles
    COMMAND hms_missing_csv_files_tests
)


# Issue #18: Patient details, list patients, and immediate registration persistence
add_executable(hms_patient_details_tests
    tests/test_patient_details_and_list.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_patient_details_tests PRIVATE include)

add_test(
    NAME TC-PAT-05_PatientDetailsAndRegistrationPersistence
    COMMAND hms_patient_details_tests
)

# Issue #17: Skip malformed CSV records
add_executable(hms_skip_malformed_csv_tests
    tests/test_skip_malformed_csv.cpp
    src/Patient.cpp
    src/Doctor.cpp
    src/Appointment.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
)

target_include_directories(hms_skip_malformed_csv_tests PRIVATE include)

add_test(
    NAME TC-PER-03_SkipMalformedCSV
    COMMAND hms_skip_malformed_csv_tests
)

# Issue #20: Navigate menus and submenus (HMS-F-032, TC-UI-01)
add_executable(hms_menu_navigation_tests
    tests/test_menu_navigation.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_menu_navigation_tests PRIVATE include)

add_test(
    NAME TC-UI-01_MenuNavigation
    COMMAND hms_menu_navigation_tests
)

# Issue #21: Reject invalid menu options (HMS-F-033, TC-UI-02)
add_executable(hms_reject_invalid_menu_options_tests
    tests/test_reject_invalid_menu_options.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_reject_invalid_menu_options_tests PRIVATE include)

add_test(
    NAME TC-UI-02_RejectInvalidMenuOptions
    COMMAND hms_reject_invalid_menu_options_tests
)

# Issue #27: Update patient details (HMS-F-006, TC-PAT-06)
add_executable(hms_update_patient_details_tests
    tests/test_update_patient_details.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_update_patient_details_tests PRIVATE include)

add_test(
    NAME TC-PAT-06_UpdatePatientDetails
    COMMAND hms_update_patient_details_tests
)



# Issue #28: Prevent patient ID edits (TC-PAT-07)
add_executable(hms_prevent_patient_id_edits_tests
    tests/test_prevent_patient_id_edits.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_prevent_patient_id_edits_tests PRIVATE include)

add_test(
    NAME TC-PAT-07_PreventPatientIdEdits
    COMMAND hms_prevent_patient_id_edits_tests
)
# Issue #29: Update doctor details (HMS-F-013, TC-DOC-06)
add_executable(hms_update_doctor_details_tests
    tests/test_update_doctor_details.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_update_doctor_details_tests PRIVATE include)

add_test(
    NAME TC-DOC-06_UpdateDoctorDetails
    COMMAND hms_update_doctor_details_tests
)


# Issue #30: Prevent Doctor ID edits (TC-DOC-07)
add_executable(hms_prevent_doctor_id_edits_tests
    tests/test_prevent_doctor_id_edits.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_prevent_doctor_id_edits_tests PRIVATE include)

add_test(
    NAME TC-DOC-07_PreventDoctorIdEdits
    COMMAND hms_prevent_doctor_id_edits_tests
)



# Issue #31: Handle CSV write failures (TC-PER-07)
add_executable(hms_csv_write_failures_tests
    tests/test_csv_write_failures.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_csv_write_failures_tests PRIVATE include)

add_test(
    NAME TC-PER-07_HandleCSVWriteFailures
    COMMAND hms_csv_write_failures_tests
)

# Issue #37: Invalid-input crash resilience (HMS-NF-001, TC-NFR-01)
add_executable(hms_invalid_input_crash_resilience_tests
    tests/test_invalid_input_crash_resilience.cpp
    src/Appointment.cpp
    src/Doctor.cpp
    src/FileManager.cpp
    src/HospitalSystem.cpp
    src/Patient.cpp
)

target_include_directories(hms_invalid_input_crash_resilience_tests PRIVATE include)

add_test(
    NAME TC-NFR-01_InvalidInputCrashResilience
    COMMAND hms_invalid_input_crash_resilience_tests
)
