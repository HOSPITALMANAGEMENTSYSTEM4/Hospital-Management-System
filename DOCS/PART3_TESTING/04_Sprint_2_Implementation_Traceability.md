# Hospital Management System (HMS)
## Sprint 2 Implementation Traceability Report
### Detailed Audit of Features, Non-Functional Hardening, Security Safeguards, and Supplementary Test Suites

**Document Identifier:** TR-S2-HMS-2026-V1.0
**Sprint Dates:** 26 October 2026 – 30 October 2026
**Document Date:** November 2026
**Repository:** [HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System)
**Sprint Boundary:** Merged Pull Requests #91 through #118, #122, #123, #125, #127 (Commits: PR #91 merge to latest `main` commit `542b90b`)

---

## 1. Executive Summary

Sprint 2 represented a major expansion in system capability, data integrity, non-functional quality, and security hardening. While Sprint 1 focused on entity primitives, single-table persistence, and basic menu routing, Sprint 2 delivered:
1. **Full Appointment Lifecycle & Conflict Checking:** Enforced doctor double-booking prevention (`BR-05`), informational availability reporting, exclusion of cancelled slots from active conflict sets (`BR-06`), chronological sorting of appointment views, and non-destructive appointment cancellation.
2. **Quality Attribute (NFR) Hardening:** Verified startup resilience across corrupt/empty files, data retention rate, menu depth ($\le 3$ levels), 1,000-record performance benchmarks ($\le 3\text{s}$ startup, $\le 2\text{s}$ operations), strict modular class separation, standalone offline operation, and cross-platform compilation.
3. **Local Security Safeguards:** Implemented accept-list input validation, CSV and formula injection guards, 10,000-character over-length safety, patient data privacy minimization in appointment listings, and atomic file write safety.
4. **Supplementary Production Improvements:** Merged four advanced PRs (#122, #123, #125, #127) implementing appointment cancellation confirmation summaries, entity-existence validation during appointment filtering, duplicate identity heuristics for patients/doctors, and universal immediate field-level validation with interactive retry handling.

By the end of Sprint 2, the automated test suite expanded from **24 to 51 CTest test targets**, achieving 100% test pass rates across all functional, non-functional, and security specifications.

---

## 2. Sprint 2 Boundary & Evidence Verification

The boundaries and merged status of Sprint 2 were verified using Git commit logs, `gh pr list --state merged`, and GitHub issue closure records:
- **Sprint Start:** 26 October 2026 06:19 UTC (PR #91 merged).
- **Sprint End:** 30 October 2026 21:13 UTC (PR #127 merged).
- **Final Test Target Count in Sprint 2:** 51 CTest test targets.
- **Excluded Unmerged Work:** PR #126 (closed without merge; its 1,000-record CTest registration was properly integrated and merged via PR #125).

---

## 3. Comprehensive Sprint 2 Implementation Inventory

The table below catalogs every confirmed, merged pull request in Sprint 2 along with its source code modifications, automated test coverage, requirement mappings, and change classification.

| PR # | Issue # | Merge Date | Feature / Implementation Summary | Modified Source Files | Test Files / CTest Target | STP / SRS Mapping | Classification |
|:---:|:---:|:---:|:---|:---|:---|:---|:---:|
| **#91** | #23 | 2026-10-26 | Rejection of doctor double-booking at identical date and time slot | `src/HospitalSystem.cpp` | `tests/test_prevent_double_booking.cpp` (`TC-APT-05_TC-NFR-08_PreventDoubleBooking`) | `HMS-F-019`, `BR-05` | **Category A** |
| **#92** | #24 | 2026-10-26 | Doctor availability query checking (reports Available/Not Available) | `src/HospitalSystem.cpp` | `tests/test_check_doctor_availability.cpp` (`TC-APT-06_CheckDoctorAvailability`) | `HMS-F-020`, `UC-10` | **Category A** |
| **#93** | #25 | 2026-10-26 | Ignore cancelled appointments in conflict checks, allowing rebooking of freed slots | `src/HospitalSystem.cpp` | `tests/test_ignore_cancelled_appointments.cpp` (`TC-APT-07_IgnoreCancelledAppointmentsInConflicts`) | `HMS-F-021`, `BR-06` | **Category A** |
| **#94** | #26 | 2026-10-26 | Chronological appointment view sorting (by ascending date, then time) | `src/HospitalSystem.cpp` | `tests/test_view_appointments_date_time_order.cpp` (`TC-APT-08_ViewAppointmentsDateTimeOrder`) | `HMS-F-022`, `UC-08` | **Category B** |
| **#95** | #32 | 2026-10-26 | Appointment cancellation updating status to Cancelled without row deletion | `src/HospitalSystem.cpp`, `src/Appointment.cpp` | `tests/test_cancel_appointment.cpp` (`TC-APT-09_CancelAppointment`) | `HMS-F-023`, `BR-06` | **Category B** |
| **#96** | #33 | 2026-10-26 | Rejection of invalid appointment cancellations (nonexistent or already cancelled) | `src/HospitalSystem.cpp` | `tests/test_reject_invalid_cancellation.cpp` (`TC-APT-10_RejectInvalidCancellation`) | `HMS-F-024` | **Category B** |
| **#97** | #34 | 2026-10-26 | Skipping duplicate and orphan records during startup CSV loading | `src/FileManager.cpp` | `tests/test_skip_duplicate_and_orphan_records.cpp` (`TC-PER-08_SkipDuplicateAndOrphanRecords`) | `HMS-F-028`, `BR-01`–`BR-04` | **Category B** |
| **#98** | #35 | 2026-10-26 | Displaying standard outcome messages (clear success or failure) for all operations | `src/HospitalSystem.cpp` | `tests/test_operation_outcome_messages.cpp` (`TC-UI-04_OperationOutcomeMessages`) | `HMS-F-035` | **Category A** |
| **#99** | #36 | 2026-10-26 | Clean application exit via Option 4 and process exit code 0 | `src/HospitalSystem.cpp`, `src/main.cpp` | `tests/test_exit_cleanly.cpp` (`TC-UI-05_ExitCleanly`), `test_exit_process.cmake` (`TC-UI-05_ExitProcessCode`) | `HMS-F-036` | **Category A** |
| **#100** | #38 | 2026-10-27 | Robust startup verification across missing, empty, and corrupt CSV datasets | `src/FileManager.cpp` | `tests/test_robust_startup.cpp` (`TC-NFR-02_RobustStartup`) | `HMS-NF-002`, `HMS-SO-02` | **Category A** |
| **#101** | #39 | 2026-10-27 | Verification of 100% data retention rate after normal close and restart | `src/FileManager.cpp` | `tests/test_data_retention.cpp` (`TC-NFR-03_DataRetention`) | `HMS-NF-003`, `HMS-F-030` | **Category A** |
| **#102** | #40 | 2026-10-27 | Usability formatting and enforcement of maximum menu depth $\le 3$ levels | `src/HospitalSystem.cpp` | `tests/test_usability_menu_depth.cpp` (`TC-NFR-05_UsabilityMenuDepth`) | `HMS-NF-005` | **Category A** |
| **#103** | #41 | 2026-10-27 | 100% Conflict Matrix verification across all 4 combination cases | `src/HospitalSystem.cpp` | `tests/test_appointment_conflict_correctness.cpp` (`TC-NFR-08_AppointmentConflictCorrectness`) | `HMS-NF-008` | **Category A** |
| **#104** | #42 | 2026-10-27 | Verification of 5 OOP classes and isolation of file I/O exclusively to FileManager | `src/*`, `include/hms/*` | `tests/test_architectural_class_isolation.cpp` (`TC-NFR-06_ArchitecturalClassIsolation`) | `HMS-NF-006` | **Category A** |
| **#105** | #43 | 2026-10-27 | Verification of standalone execution with zero networking sockets | `CMakeLists.txt` | `tests/test_standalone_offline.cmake` (`TC-NFR-09_StandaloneOffline`) | `HMS-NF-009`, `HMS-SR-008` | **Category A** |
| **#106** | #44 | 2026-10-28 | Performance benchmarking with 1,000 records each (startup $\le 3\text{s}$, ops $\le 2\text{s}$) | `src/*` | `tests/test_performance_1000_records.cpp` (registered via PR #125 as `TC-NFR-04_Performance1000Records`) | `HMS-NF-004` | **Category A** |
| **#107** | #45 | 2026-10-28 | Cross-platform C++17 compilation and testing clean build flags | `CMakeLists.txt` | Build-time CMake configuration and compiler flags | `HMS-NF-007` | **Category A** |
| **#108** | #46 | 2026-10-28 | Human-readable CSV formatting with valid header rows and text editor compatibility | `src/FileManager.cpp` | `tests/test_human_readable_csv.cpp` (`TC-NFR-10_HumanReadableCSV`) | `HMS-NF-010` | **Category A** |
| **#109** | #47 | 2026-10-28 | Strict accept-list input validation against Table 4-1 character domains | `src/Patient.cpp`, `src/Doctor.cpp`, `src/Appointment.cpp` | `tests/test_accept_list_input_validation.cpp` (`TC-SEC-01_AcceptListInputValidation`) | `HMS-SR-001`, `HMS-SO-01` | **Category B** |
| **#110** | #48 | 2026-10-29 | CSV delimiter and formula injection guard (rejecting commas, newlines, `=`, `+`, `-`, `@`) | `src/*.cpp` | `tests/test_csv_formula_injection_guard.cpp` (`TC-SEC-02_CSVFormulaInjectionGuard`) | `HMS-SR-002`, `VR-14` | **Category A** |
| **#111** | #49 | 2026-10-29 | Over-length input buffer safety (handling 10,000+ character strings without corruption) | `src/*.cpp` | `tests/test_over_length_input_safety.cpp` (`TC-SEC-03_OverLengthInputSafety`) | `HMS-SR-003`, `HMS-SO-02` | **Category A** |
| **#112** | #50 | 2026-10-29 | Extended validation for untrusted CSV data across doctor and appointment records | `src/FileManager.cpp` | `tests/test_skip_malformed_csv.cpp` (`TC-PER-03_SkipMalformedCSV`) | `HMS-SR-004` | **Category B** |
| **#113** | #51 | 2026-10-29 | Non-destructive atomic file writes preserving previous file data upon write failures | `src/FileManager.cpp` | `tests/test_csv_write_failures.cpp` (`TC-PER-07_HandleCSVWriteFailures`) | `HMS-SR-005` | **Category B** |
| **#114** | #52 | 2026-10-29 | Audit non-deletion: cancelled appointments retained in `appointments.csv` | `src/FileManager.cpp` | `tests/test_retain_cancelled_appointment_records.cpp` (`TC-SEC-06_RetainCancelledAppointmentRecords`) | `HMS-SR-006` | **Category A** |
| **#115** | #53 | 2026-10-29 | Patient privacy minimization: omitting address, phone, and medical notes from views | `src/HospitalSystem.cpp` | `tests/test_minimize_patient_data_in_appointment_views.cpp` (`TC-SEC-07_MinimizePatientDataInAppointmentViews`) | `HMS-SR-007`, `HMS-SO-03` | **Category A** |
| **#116** | #54 | 2026-10-29 | Air-gapped isolation: binary objdump inspection verifying zero network symbols | `CMakeLists.txt` | `tests/test_standalone_offline.cmake` (`TC-NFR-09_StandaloneOffline`) | `HMS-SR-008` | **Category A** |
| **#117** | #55 | 2026-10-29 | Sanitizing error messages: omitting raw system paths, exceptions, or raw file lines | `src/FileManager.cpp`, `src/HospitalSystem.cpp` | `tests/test_sanitize_error_messages.cpp` (`TC-SEC-09_SanitizeErrorMessages`) | `HMS-SR-009` (Supplementary Scope) | **Category D** |
| **#118** | #56 | 2026-10-29 | Restricting data directory access: preventing directory traversal or arbitrary paths | `src/FileManager.cpp` | `tests/test_restrict_data_directory.cpp` (`TC-SEC-10_RestrictDataDirectory`) | `HMS-SR-010` (Supplementary Scope) | **Category D** |
| **#122** | #120 | 2026-10-30 | Appointment cancellation confirmation workflow: summary display and Y/N confirmation | `src/HospitalSystem.cpp` | `tests/test_cancel_appointment.cpp`, `tests/test_reject_invalid_cancellation.cpp` | `HMS-F-023`, `UC-09` (Supplementary Scope) | **Category C** |
| **#123** | #121 | 2026-10-30 | Validating patient and doctor existence before retrieving appointment filter views | `src/HospitalSystem.cpp` | `tests/test_filter_unknown_entity.cpp` (`TC-APT-12_FilterUnknownEntity`) | `HMS-F-022`, `UC-08` (Supplementary Scope) | **Category C** |
| **#125** | #124 | 2026-10-30 | Duplicate identity detection for patients/doctors and CTest registration for performance test | `src/FileManager.cpp`, `src/HospitalSystem.cpp`, `CMakeLists.txt` | `tests/test_duplicate_identity_validation.cpp` (`TC-ID-01_DuplicateIdentityValidation`), `TC-NFR-04_Performance1000Records` | `HMS-F-001/008` Ext, `HMS-NF-004` | **Category C / D** |
| **#127** | #119 | 2026-10-30 | Universal immediate field-level validation and same-field interactive retry handling | `src/HospitalSystem.cpp` | Integrated across `test_book_appointment.cpp`, `test_cancel_appointment.cpp`, etc. | `HMS-F-003/010/018` Ext | **Category B / C** |

---

## 4. Supplementary Sprint Implementations (Scope Beyond STP Baseline)

Sprint 2 introduced six significant functional and security capabilities that extended beyond the original 54 baseline requirements. These have been formally integrated into the supplementary traceability scope:

### 1. Sensitive Filesystem Error Sanitization (`HMS-SR-009` / `TC-SEC-09`)
- **Originating PR:** PR #117 (Issue #55).
- **Rationale:** The STP v1.0 baseline covered input validation and injection guard, but did not address information leakage via error messages. PR #117 sanitized all error outputs in `FileManager` and `HospitalSystem` to ensure that full local file paths (e.g. `C:\Users\...\patients.csv`), operating system exception traces, or raw malformed rows are never leaked to the console user.
- **Automated Target:** `TC-SEC-09_SanitizeErrorMessages` in `tests/test_sanitize_error_messages.cpp`.

### 2. Strict Data Directory Restriction (`HMS-SR-010` / `TC-SEC-10`)
- **Originating PR:** PR #118 (Issue #56).
- **Rationale:** Prevents path traversal vulnerabilities and unauthorized filesystem access. The application strictly forbids taking file paths from user inputs or environment variables in production, restricting all reads/writes to the designated application data folder.
- **Automated Target:** `TC-SEC-10_RestrictDataDirectory` in `tests/test_restrict_data_directory.cpp`.

### 3. Appointment Cancellation Confirmation Workflow (`HMS-F-023-EXT` / `TC-APT-11`)
- **Originating PR:** PR #122 (Issue #120).
- **Rationale:** Previously, cancellation took an ID and immediately cancelled the record without user confirmation. PR #122 enhanced `cancelAppointmentInteractive()` to display an appointment summary (Patient ID/Name, Doctor ID/Name, Date, Time, Status) and require explicit `Y/N` confirmation, preventing accidental cancellations. Invalid confirmations or entering `N` abort the operation cleanly.

### 4. Appointment Filtering Entity Existence Validation (`HMS-F-022-EXT` / `TC-APT-12`)
- **Originating PR:** PR #123 (Issue #121).
- **Rationale:** When filtering appointments by Patient ID or Doctor ID, querying a nonexistent ID previously displayed `"No appointments found."`, indistinguishable from a registered entity with zero appointments. PR #123 added entity validation via `patientIdExists()` and `doctorIdExists()`, outputting `"Patient not found"` or `"Doctor not found"` immediately on nonexistent entities while preserving `"No appointments found."` for registered entities with zero bookings.
- **Automated Target:** `TC-APT-12_FilterUnknownEntity` in `tests/test_filter_unknown_entity.cpp` (69 assertions).

### 5. Duplicate Patient and Doctor Identity Validation (`HMS-F-001/008-EXT` / `TC-ID-01`)
- **Originating PR:** PR #125 (Issue #124).
- **Rationale:** Duplicate ID rejection prevents identical IDs, but did not prevent registering the same human twice under different IDs (e.g., `P001` John Doe `9876543210` and `P002` John Doe `9876543210`). PR #125 introduced identity heuristic checks on normalized name and contact number. If a potential duplicate is detected, a warning displays the existing ID and asks for explicit confirmation (`Y/N`) before proceeding.
- **Automated Target:** `TC-ID-01_DuplicateIdentityValidation` in `tests/test_duplicate_identity_validation.cpp`.

### 6. Universal Immediate Field-Level Validation & Retry Handling (`HMS-F-003/010/018-EXT` / `TC-VAL-01`)
- **Originating PR:** PR #127 (Issue #119).
- **Rationale:** Form entry originally parsed all fields and aborted if any field failed, forcing the user to re-type all fields from the start. PR #127 refactored console input to use `readValidatedField()`. Each field is validated immediately upon entry; if invalid, an error message is displayed and the system re-prompts for the same field immediately, preserving previously entered valid fields.
