# Hospital Management System (HMS)
## Sprint 1 Implementation Traceability Report
### Detailed Audit of Features, Source Changes, Requirements Mapping, and Test Case Evolution

**Document Identifier:** TR-S1-HMS-2026-V1.0
**Sprint Dates:** 19 October 2026 – 23 October 2026
**Document Date:** November 2026
**Repository:** [HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System)
**Sprint Boundary:** Merged Pull Requests #60 through #90 (Commits: initial setup to PR #90 merge)

---

## 1. Executive Summary

Sprint 1 established the core foundations of the Hospital Management System (HMS). The development objectives focused on standing up the C++17 modular architecture, implementing essential entity domain classes (`Patient`, `Doctor`, `Appointment`), creating basic console interactive menus, delivering foundational CSV file persistence in `FileManager`, and hardening the system against common invalid input crashes.

By the conclusion of Sprint 1 on 23 October 2026, a total of **31 pull requests (PRs #60–#90)** were merged into `main`, delivering **24 registered CTest test targets**.

This document details every merged PR and closed issue during Sprint 1, identifies the precise source and test files affected, maps them to the baseline Software Requirements Specification (SRS v1.0) and Software Test Plan (STP v1.0), and explains how each implementation impacted the test case specifications.

---

## 2. Sprint 1 Boundary & Evidence Verification

The boundaries of Sprint 1 were established using repository commit history, pull request merge timestamps, and the official project submission document [`DOCS/SPRINT1_DELEVERABLES/SPRINT1_SUBMISSION.md`](file:///C:/Users/hp/Desktop/SE_grp/DOCS/SPRINT1_DELEVERABLES/SPRINT1_SUBMISSION.md):
- **Sprint Start:** 19 October 2026 07:34 UTC (PR #60 merged).
- **Sprint End:** 23 October 2026 14:08 UTC (PR #90 merged).
- **Final Test Target Count in Sprint 1:** 24 CTest test targets (confirmed in PR #90 and `SPRINT1_SUBMISSION.md`).
- **Issues Resolved:** Issues #1 through #22, #27 through #31, #37, #57, #59.

---

## 3. Comprehensive Sprint 1 Implementation Inventory

The table below catalogs every confirmed, merged pull request in Sprint 1 along with its source code modifications, automated test coverage, requirement mappings, and change classification.

| PR # | Issue # | Merge Date | Feature / Implementation Summary | Modified Source Files | Test Files / CTest Target | STP Mapping | Classification |
|:---:|:---:|:---:|:---|:---|:---|:---|:---:|
| **#60** | #59 | 2026-10-19 | Initial C++17 project structure, CMake build configuration, and modular skeleton | `CMakeLists.txt`, `src/main.cpp`, `include/hms/*.h` | Initial test harness setup | Architectural Foundation | **Category A** |
| **#61** | #1 | 2026-10-19 | Patient registration implementation (ID, name, age, gender, contact, address, note) | `src/Patient.cpp`, `src/HospitalSystem.cpp` | `tests/test_patient_details_and_list.cpp` | `HMS-F-001`, `UC-01` | **Category B** |
| **#62** | #2 | 2026-10-19 | Case-insensitive duplicate Patient ID validation and error reporting | `src/Patient.cpp`, `src/FileManager.cpp` | `tests/test_duplicate_patient_id.cpp` (`TC-PAT-02_DuplicatePatientId`) | `HMS-F-002`, `BR-01` | **Category B** |
| **#63** | #3 | 2026-10-19 | Comprehensive patient field validation (rules VR-01 to VR-07, VR-14) | `src/Patient.cpp` | `tests/test_validate_patient_fields.cpp` (`TC-PAT-03_ValidatePatientFields`) | `HMS-F-003`, `VR-01`–`VR-07` | **Category B** |
| **#64** | #6 | 2026-10-19 | Doctor registration implementation (ID, name, specialization, department, contact, availability) | `src/Doctor.cpp`, `src/HospitalSystem.cpp` | `tests/test_add_doctor.cpp` (`TC-DOC-01_AddDoctor`) | `HMS-F-008`, `UC-04` | **Category A** |
| **#65** | #7 | 2026-10-19 | Duplicate Doctor ID rejection and error reporting | `src/Doctor.cpp`, `src/FileManager.cpp` | `tests/test_duplicate_doctor_id.cpp` (`TC-DOC-02_RejectDuplicateDoctorId`) | `HMS-F-009`, `BR-02` | **Category A** |
| **#66** | #8 | 2026-10-19 | Doctor field validation (rules VR-01, VR-02, VR-05, VR-08, VR-09, VR-14) | `src/Doctor.cpp` | `tests/test_validate_doctor_fields.cpp` (`TC-DOC-03_ValidateDoctorFields`) | `HMS-F-010`, `VR-08`–`VR-09` | **Category B** |
| **#67** | #4 | 2026-10-19 | Patient search by exact ID and case-insensitive partial name match | `src/HospitalSystem.cpp` | `tests/test_search_patients.cpp` (`TC-PAT-04_SearchPatients`) | `HMS-F-004`, `UC-02` | **Category B** |
| **#68** | #5 | 2026-10-19 | Patient details view and listing of all registered patients | `src/HospitalSystem.cpp` | `tests/test_patient_details_and_list.cpp` | `HMS-F-005`, `UC-02` | **Category A** |
| **#69** | #11 | 2026-10-19 | Initial appointment booking implementation with auto-generated ID (`A0001`) | `src/Appointment.cpp`, `src/HospitalSystem.cpp` | `tests/test_book_appointment.cpp` (`TC-APT-01_BookAppointment`) | `HMS-F-015`, `VR-12` | **Category B** |
| **#70** | #4 | 2026-10-19 | Patient search whitespace trimming and sanitization coverage | `src/HospitalSystem.cpp` | `tests/test_search_patients.cpp` (`TC-PAT-04_SearchPatients`) | `HMS-F-004` | **Category B** |
| **#71** | #5 | 2026-10-19 | Refactored patient details and listing display format | `src/HospitalSystem.cpp` | `tests/test_patient_details_and_list.cpp` | `HMS-F-005` | **Category A** |
| **#72** | #9 | 2026-10-19 | Doctor search by exact ID, partial name, and partial specialization | `src/HospitalSystem.cpp` | `tests/test_search_doctors.cpp` (`TC-DOC-04_SearchDoctors`) | `HMS-F-011`, `UC-05` | **Category A** |
| **#73** | #10 | 2026-10-19 | Doctor details display and full doctor listing | `src/HospitalSystem.cpp` | `tests/test_view_list_doctors.cpp` (`TC-DOC-05_ViewAndListDoctors`) | `HMS-F-012`, `UC-05` | **Category A** |
| **#74** | #12 | 2026-10-20 | Rejection of unknown Patient ID during appointment booking | `src/HospitalSystem.cpp` | `tests/test_reject_unknown_patient.cpp` (`TC-APT-02_RejectUnknownPatient`) | `HMS-F-016`, `BR-03` | **Category A** |
| **#75** | #13 | 2026-10-20 | Rejection of unknown Doctor ID during appointment booking | `src/HospitalSystem.cpp` | `tests/test_reject_unknown_doctor.cpp` (`TC-APT-03_RejectUnknownDoctor`) | `HMS-F-017`, `BR-04` | **Category A** |
| **#76** | #14 | 2026-10-20 | Appointment date and time validation (rules VR-10 and VR-11) | `src/Appointment.cpp` | `tests/test_validate_appointment_date_time.cpp` (`TC-APT-04_ValidateAppointmentDateTime`) | `HMS-F-018`, `VR-10`–`VR-11` | **Category B** |
| **#77** | #15 | 2026-10-20 | Loading CSV records at application startup into memory | `src/FileManager.cpp` | `tests/test_startup_csv_loading.cpp` (`TC-PER-01_StartupCSVLoading`) | `HMS-F-025`, `BR-09` | **Category A** |
| **#78** | #16 | 2026-10-20 | Graceful handling of missing CSV files (treated as empty datasets) | `src/FileManager.cpp` | `tests/test_missing_csv_files.cpp` (`TC-PER-02_MissingCSVFiles`) | `HMS-F-026`, `HMS-NF-002` | **Category A** |
| **#79** | #17 | 2026-10-22 | Skipping malformed CSV lines and reporting skipped record count | `src/FileManager.cpp` | `tests/test_skip_malformed_csv.cpp` (`TC-PER-03_SkipMalformedCSV`) | `HMS-F-027`, `BR-10` | **Category B** |
| **#80** | #18 | 2026-10-22 | Immediate CSV persistence writing after each modifying operation | `src/FileManager.cpp`, `src/HospitalSystem.cpp` | `tests/test_patient_details_and_list.cpp` (`TC-PAT-05_PatientDetailsAndRegistrationPersistence`) | `HMS-F-029` | **Category B** |
| **#81** | #19 | 2026-10-22 | Data retention across process restart verification | `src/FileManager.cpp` | `tests/test_patient_details_and_list.cpp` | `HMS-F-030`, `HMS-NF-003` | **Category A** |
| **#82** | #20 | 2026-10-22 | Console main menu and submenu navigation hierarchy | `src/HospitalSystem.cpp` | `tests/test_menu_navigation.cpp` (`TC-UI-01_MenuNavigation`) | `HMS-F-032`, `UC-01`–`UC-10` | **Category A** |
| **#83** | #21 | 2026-10-22 | Rejection of non-numeric, empty, and out-of-range menu selections | `src/HospitalSystem.cpp` | `tests/test_reject_invalid_menu_options.cpp` (`TC-UI-02_RejectInvalidMenuOptions`) | `HMS-F-033`, `BR-08` | **Category A** |
| **#84** | #22 | 2026-10-22 | Transactional state protection: memory and CSV untouched on failed operations | `src/HospitalSystem.cpp`, `src/FileManager.cpp` | Verified in unit test harnesses (`test_reject_invalid_menu_options.cpp`) | `HMS-F-034`, `BR-08` | **Category B** |
| **#85** | #27 | 2026-10-23 | Patient detail update functionality with field-level validation | `src/HospitalSystem.cpp`, `src/Patient.cpp` | `tests/test_update_patient_details.cpp` (`TC-PAT-06_UpdatePatientDetails`) | `HMS-F-006`, `UC-03` | **Category B** |
| **#86** | #28 | 2026-10-23 | Prevention of Patient ID editing during record updates | `src/HospitalSystem.cpp` | `tests/test_prevent_patient_id_edits.cpp` (`TC-PAT-07_PreventPatientIdEdits`) | `HMS-F-007`, `BR-01` | **Category A** |
| **#87** | #29 | 2026-10-23 | Doctor detail update functionality with field-level validation | `src/HospitalSystem.cpp`, `src/Doctor.cpp` | `tests/test_update_doctor_details.cpp` (`TC-DOC-06_UpdateDoctorDetails`) | `HMS-F-013`, `UC-06` | **Category B** |
| **#88** | #30 | 2026-10-23 | Prevention of Doctor ID editing during record updates | `src/HospitalSystem.cpp` | `tests/test_prevent_doctor_id_edits.cpp` (`TC-DOC-07_PreventDoctorIdEdits`) | `HMS-F-014`, `BR-02` | **Category A** |
| **#89** | #31 | 2026-10-23 | Handling of CSV file write failures (read-only filesystem simulation) | `src/FileManager.cpp` | `tests/test_csv_write_failures.cpp` (`TC-PER-07_HandleCSVWriteFailures`) | `HMS-F-031`, `HMS-SR-005` | **Category B** |
| **#90** | #37 | 2026-10-23 | Console input crash resilience across 20 distinct invalid input vectors | `src/HospitalSystem.cpp` | `tests/test_invalid_input_crash_resilience.cpp` (`TC-NFR-01_InvalidInputCrashResilience`) | `HMS-NF-001` | **Category A** |

---

## 4. Analysis of Test Case Evolution in Sprint 1

### A. Enhancements to Existing Baseline Test Cases (Category B)
1. **Patient Registration & Duplicate Checking (`TC-PAT-01`, `TC-PAT-02`):**
   - *STP Baseline:* Verified that a patient could be saved and duplicate IDs were rejected.
   - *Sprint 1 Enhancement:* PR #62 added case-insensitive matching (`p001` vs `P001`). PR #80 added explicit verification that records are committed immediately to `patients.csv` without waiting for application exit.
   - *Test Case Update:* `TC-PAT-02` was updated to explicitly assert case-insensitivity; `TC-PAT-01` was updated to check immediate CSV file contents.
2. **Patient Field Validation (`TC-PAT-03`):**
   - *STP Baseline:* Tested boundaries for age (0–120) and gender (M/F/O).
   - *Sprint 1 Enhancement:* PR #63 implemented strict validation across all 7 fields (VR-01 through VR-07) and enforced VR-14 (text safety rejecting commas, newlines, and formula symbols).
   - *Test Case Update:* `TC-PAT-03` was expanded into a multi-attribute matrix covering contact numbers (exactly 10 digits), address boundaries (1–100 chars), and formula prefixes.
3. **Patient Search Whitespace Handling (`TC-PAT-04`):**
   - *STP Baseline:* Verified exact ID search and partial name search.
   - *Sprint 1 Enhancement:* PR #70 added whitespace trimming on input search queries (`  P001  ` -> `P001`), preventing search misses caused by leading/trailing spaces.
   - *Test Case Update:* `TC-PAT-04` was updated to include padded string test data.
4. **Patient and Doctor Updates (`TC-PAT-06`, `TC-DOC-06`):**
   - *STP Baseline:* Verified that fields could be updated.
   - *Sprint 1 Enhancement:* PRs #85 and #87 implemented interactive field menus, allowing selective updating of individual fields or updating all fields sequentially, while validating each modified field against domain rules.
   - *Test Case Update:* Test execution steps in `TC-PAT-06` and `TC-DOC-06` were updated to reflect the submenu update prompts.

### B. Traceability Discrepancies Resolved
- **Discrepancy 1: `TC-PAT-01` vs CTest Registration:** In STP Table 7, patient registration is named `TC-PAT-01`. In Sprint 1, PR #80 registered the combined target `TC-PAT-05_PatientDetailsAndRegistrationPersistence` (`tests/test_patient_details_and_list.cpp`) which covers both registration persistence (`HMS-F-001`, `HMS-F-029`) and details viewing (`HMS-F-005`). `TC-PAT-01` remains the official baseline test specification ID, while the CTest executable target is documented accurately.
- **Discrepancy 2: `TC-UI-03` State Protection:** In STP Table 7, transactional state protection is designated `TC-UI-03`. In Sprint 1, PR #84 embedded state protection assertions directly into `tests/test_reject_invalid_menu_options.cpp` and `tests/test_csv_write_failures.cpp` rather than creating a standalone `hms_state_protection_tests` executable. This is documented as Category B with shared test file mapping.
