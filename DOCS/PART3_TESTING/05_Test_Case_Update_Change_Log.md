# Hospital Management System (HMS)
## Test Case Update Change Log
### Comprehensive Record of Test Case Modifications, Enhancements, and Classification Across Sprint 1 & Sprint 2

**Document Identifier:** CL-TCD-HMS-2026-V1.0
**Document Date:** November 2026
**Baseline Reference:** Software Test Plan (STP-HMS-2026-V1.0), Table 7 (Baseline Requirements Traceability Matrix)
**Updated Document Reference:** Updated Test Case Document (TCD-HMS-2026-V2.0)

---

## 1. Executive Summary

This Change Log records every change made to the original Software Test Plan (STP v1.0) test specifications during the evolution of the Hospital Management System across Sprint 1 and Sprint 2.

*Baseline Date Note:* On the title page of the baseline Software Test Plan PDF (`DOCS/Software_Test_Plan.pdf`), the document date is printed as "Date: 20 November 2026". However, its internal document control table, revision history, and all subsequent Sprint 1 and Sprint 2 reports consistently establish its creation date as 20 October 2026. This updated document maintains traceability to that October 2026 baseline.

The original STP defined **54 baseline requirements** (36 Functional, 10 Non-Functional, 8 Security) mapped to 54 test case entries in Table 7, with 10 representative detailed test specifications. During implementation, several original test cases were enhanced to validate advanced edge cases (e.g., case-insensitive matching, whitespace trimming, immediate field-level retry, non-destructive writes), and **6 supplementary test suites** were created to validate capabilities outside the frozen baseline.

This document classifies every requirement and test case into one of five rigorous QA categories:
- **Category A:** Existing STP functionality, unchanged.
- **Category B:** Existing STP functionality, enhanced during sprints.
- **Category C:** New functional capability beyond original STP baseline.
- **Category D:** Additional non-functional or security improvements.
- **Category E:** Implementation present but testing gap exists.

---

## 2. Test Case Classification Summary

| Classification Category | Description | Count | Percentage |
|:---|:---|:---:|:---:|
| **Category A** | Existing STP functionality, unchanged | 36 | 60.0% |
| **Category B** | Existing STP functionality, enhanced during sprints | 17 | 28.3% |
| **Category C** | New functionality beyond original STP baseline | 3 | 5.0% |
| **Category D** | Additional non-functional or security improvements | 3 | 5.0% |
| **Category E** | Implementation present but testing gap exists | 1 | 1.7% |
| **Total Evaluated Scope** | Baseline (54) + Supplementary Scope (6) | **60** | **100.0%** |

---

## 3. Granular Change Log by Functional Area

### 3.1 Patient Management (`HMS-F-001` to `HMS-F-007`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-F-001` | `TC-PAT-01` | `TC-PAT-01` | **Category B** | **Enhanced:** Added immediate CSV persistence verification (`patients.csv` contains row immediately without waiting for exit) and integrated immediate field retry. In CTest, exercised via `TC-PAT-05_PatientDetailsAndRegistrationPersistence`. | PR #61, #80 (Issue #1, #18) |
| `HMS-F-002` | `TC-PAT-02` | `TC-PAT-02` | **Category B** | **Enhanced:** Explicitly enforced case-insensitive duplicate ID checks (`p001` vs `P001`). Rejects duplicate with exact message `"Patient ID already exists"`. | PR #62 (Issue #2) |
| `HMS-F-003` | `TC-PAT-03` | `TC-PAT-03` | **Category B** | **Enhanced:** Expanded test matrix to cover VR-01 through VR-07 individually (contact exactly 10 digits, address 1–100 chars, gender M/F/O) and text-safety VR-14 (formula prefixes `=,+,-,@` and delimiters). Added immediate retry on field errors. | PR #63, #127 (Issue #3, #119) |
| `HMS-F-004` | `TC-PAT-04` | `TC-PAT-04` | **Category B** | **Enhanced:** Added test steps verifying that leading and trailing whitespace are trimmed from search queries (`  P001  ` resolves to `P001`). Case-insensitive partial name search preserved. | PR #67, #70 (Issue #4) |
| `HMS-F-005` | `TC-PAT-05` | `TC-PAT-05` | **Category A** | **Unchanged:** Verified exact display of all 7 fields for selected patient and full patient listing. Returns `"No patients registered"` on empty dataset. | PR #68, #71 (Issue #5) |
| `HMS-F-006` | `TC-PAT-06` | `TC-PAT-06` | **Category B** | **Enhanced:** Updated execution steps to reflect interactive update submenu (selective field editing vs update all fields). Validates updated values with same-field retry and commits immediately to CSV. | PR #85, #127 (Issue #27, #119) |
| `HMS-F-007` | `TC-PAT-07` | `TC-PAT-07` | **Category A** | **Unchanged:** Verifies that Patient ID field cannot be modified during update. Rejects update for nonexistent Patient ID with `"Patient not found"`. | PR #86 (Issue #28) |

---

### 3.2 Doctor Management (`HMS-F-008` to `HMS-F-014`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-F-008` | `TC-DOC-01` | `TC-DOC-01` | **Category A** | **Unchanged:** Verifies doctor addition across all 6 fields (ID, name, specialization, department, contact, availability) and immediate write to `doctors.csv`. | PR #64 (Issue #6) |
| `HMS-F-009` | `TC-DOC-02` | `TC-DOC-02` | **Category A** | **Unchanged:** Verifies rejection of duplicate Doctor ID with exact message `"Doctor ID already exists"`. Preserves stored records. | PR #65 (Issue #7) |
| `HMS-F-010` | `TC-DOC-03` | `TC-DOC-03` | **Category B** | **Enhanced:** Expanded test matrix to cover specialization and department limits (1–40 chars), contact (10 digits), availability text (1–60 chars), and VR-14 formula safety. | PR #66, #127 (Issue #8, #119) |
| `HMS-F-011` | `TC-DOC-04` | `TC-DOC-04` | **Category A** | **Unchanged:** Verifies doctor search across exact ID, partial name, and partial specialization. Returns `"No matching doctor found"` when unrepresented. | PR #72 (Issue #9) |
| `HMS-F-012` | `TC-DOC-05` | `TC-DOC-05` | **Category A** | **Unchanged:** Verifies complete doctor details view (all 6 fields) and full doctor listing. Displays `"No doctors registered"` when empty. | PR #73 (Issue #10) |
| `HMS-F-013` | `TC-DOC-06` | `TC-DOC-06` | **Category B** | **Enhanced:** Updated execution steps to reflect interactive update submenu. Verifies field-level validation and same-field retry during doctor updates. | PR #87, #127 (Issue #29, #119) |
| `HMS-F-014` | `TC-DOC-07` | `TC-DOC-07` | **Category A** | **Unchanged:** Verifies Doctor ID cannot be modified during update. Rejects update for nonexistent Doctor ID with `"Doctor not found"`. | PR #88 (Issue #30) |

---

### 3.3 Appointment Management (`HMS-F-015` to `HMS-F-024`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-F-015` | `TC-APT-01` | `TC-APT-01` | **Category B** | **Enhanced:** Verifies system-generated ID `A0001`+ with sequential numbering, status `Booked`, and immediate persistence to `appointments.csv`. | PR #69 (Issue #11) |
| `HMS-F-016` | `TC-APT-02` | `TC-APT-02` | **Category A** | **Unchanged:** Verifies foreign key check; rejects booking with unregistered Patient ID displaying `"Patient not found"`. | PR #74 (Issue #12) |
| `HMS-F-017` | `TC-APT-03` | `TC-APT-03` | **Category A** | **Unchanged:** Verifies foreign key check; rejects booking with unregistered Doctor ID displaying `"Doctor not found"`. | PR #75 (Issue #13) |
| `HMS-F-018` | `TC-APT-04` | `TC-APT-04` | **Category B** | **Enhanced:** Expanded date/time test matrix: calendar validity (leap year `2024-02-29` accepted, `2026-02-29` rejected), month range (1–12), day range (1–31), time format `HH:MM` 24-hr (`00:00`–`23:59`, rejects `24:00`). | PR #76, #127 (Issue #14, #119) |
| `HMS-F-019` | `TC-APT-05` | `TC-APT-05` | **Category A** | **Unchanged:** Verifies double-booking conflict rejection: second booking for same doctor at same date and time displays `"Doctor not available at the requested date and time"`. In CTest: `TC-APT-05_TC-NFR-08_PreventDoubleBooking`. | PR #91 (Issue #23) |
| `HMS-F-020` | `TC-APT-06` | `TC-APT-06` | **Category A** | **Unchanged:** Verifies standalone doctor availability query. Reports `"Available"` or `"Not Available"` without modifying records. | PR #92 (Issue #24) |
| `HMS-F-021` | `TC-APT-05` | `TC-APT-07` | **Category A** | **Unchanged:** Verifies that cancelled appointments are ignored during conflict checks, allowing a freed slot to be rebooked. In CTest: `TC-APT-07_IgnoreCancelledAppointmentsInConflicts`. | PR #93 (Issue #25) |
| `HMS-F-022` | `TC-APT-08` | `TC-APT-08` | **Category B** | **Enhanced:** Verifies chronological sorting (ascending date, then time) across all 3 view modes (All, by Patient ID, by Doctor ID). Note: Entity validation on filtering is covered by supplementary case `TC-APT-12`. | PR #94 (Issue #26) |
| `HMS-F-023` | `TC-APT-09` | `TC-APT-09` | **Category B** | **Enhanced:** Verifies cancellation updates status to `Cancelled` and retains record in `appointments.csv`. Updated to incorporate cancellation confirmation workflow (PR #122). | PR #95, #122 (Issue #32, #120) |
| `HMS-F-024` | `TC-APT-10` | `TC-APT-10` | **Category B** | **Enhanced:** Verifies rejection of non-existent Appointment ID (`"Appointment not found"`) and already cancelled ID (`"Appointment already cancelled"`). Updated to reflect interactive retry loop. | PR #96, #127 (Issue #33, #119) |

---

### 3.4 Persistence and File Handling (`HMS-F-025` to `HMS-F-031`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-F-025` | `TC-PER-01` | `TC-PER-01` | **Category A** | **Unchanged:** Verifies pre-existing valid records in `patients.csv`, `doctors.csv`, and `appointments.csv` load into memory on startup. | PR #77 (Issue #15) |
| `HMS-F-026` | `TC-PER-02` | `TC-PER-02` | **Category A** | **Unchanged:** Verifies startup when CSV files are missing; system initializes empty datasets without crashing and creates files on first save. | PR #78 (Issue #16) |
| `HMS-F-027` | `TC-PER-03` | `TC-PER-03` | **Category B** | **Enhanced:** Verifies malformed CSV rows (corrupt columns, out-of-range fields) are skipped safely. Reports skipped count (`"X malformed record(s) skipped"`). Hardened via PR #112 to cover doctor and appointment CSVs. | PR #79, #112 (Issue #17, #50) |
| `HMS-F-028` | `TC-PER-04` | `TC-PER-04` | **Category B** | **Enhanced (Discrepancy Note):** In STP Table 7 named `TC-PER-04`. In CTest registered as `TC-PER-08_SkipDuplicateAndOrphanRecords`. Verifies skipping duplicate entity IDs and orphan appointments during startup loading. | PR #97 (Issue #34) |
| `HMS-F-029` | `TC-PER-05` | `TC-PER-05` | **Category B** | **Enhanced (Discrepancy Note):** In STP Table 7 named `TC-PER-05`. In CTest verified via `TC-PAT-05_PatientDetailsAndRegistrationPersistence`. Verifies immediate disk flush after every modifying action. | PR #80 (Issue #18) |
| `HMS-F-030` | `TC-PER-06` | `TC-PER-06` | **Category B** | **Enhanced (Discrepancy Note):** In STP Table 7 named `TC-PER-06`. In CTest verified via `TC-NFR-03_DataRetention`. Verifies 100% record retention after closing and restarting application. | PR #81, #101 (Issue #19, #39) |
| `HMS-F-031` | `TC-PER-07` | `TC-PER-07` | **Category B** | **Enhanced:** Verifies error handling when CSV write fails (read-only filesystem). Hardened in PR #113 with non-destructive atomic write guarantees (previous file content preserved intact). | PR #89, #113 (Issue #31, #51) |

---

### 3.5 Console UI and Input Handling (`HMS-F-032` to `HMS-F-036`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-F-032` | `TC-UI-01` | `TC-UI-01` | **Category A** | **Unchanged:** Verifies main menu and 3 submenus (Patient, Doctor, Appointment) with option 0 returning to previous menu. | PR #82 (Issue #20) |
| `HMS-F-033` | `TC-UI-02` | `TC-UI-02` | **Category A** | **Unchanged:** Rejects non-numeric, alphabetic, negative, and out-of-range menu options without crashing, redisplaying menu. | PR #83 (Issue #21) |
| `HMS-F-034` | `TC-UI-03` | `TC-UI-03` | **Category B** | **Enhanced (Discrepancy Note):** STP Table 7 lists `TC-UI-03`. Exercised across unit/system tests (`test_reject_invalid_menu_options.cpp`, `test_csv_write_failures.cpp`). Verifies memory and disk state unchanged on failed operations. | PR #84 (Issue #22) |
| `HMS-F-035` | `TC-UI-04` | `TC-UI-04` | **Category A** | **Unchanged:** Verifies that every single operation terminates with either a clear success confirmation or an informative error message. | PR #98 (Issue #35) |
| `HMS-F-036` | `TC-UI-05` | `TC-UI-05` | **Category A** | **Unchanged:** Verifies Option 4 on main menu terminates application cleanly with process return code 0. In CTest verified via `TC-UI-05_ExitCleanly` and `TC-UI-05_ExitProcessCode`. | PR #99 (Issue #36) |

---

### 3.6 Non-Functional Quality Requirements (`HMS-NF-001` to `HMS-NF-010`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-NF-001` | `TC-NFR-01` | `TC-NFR-01` | **Category A** | **Unchanged:** Stress tests console input across 20 distinct invalid input vectors (empty, symbols, long strings, format specifiers); verifies 0 crashes. | PR #90 (Issue #37) |
| `HMS-NF-002` | `TC-NFR-02` | `TC-NFR-02` | **Category A** | **Unchanged:** Verifies startup across all combinations of missing, empty, and malformed data files. | PR #100 (Issue #38) |
| `HMS-NF-003` | `TC-NFR-03` | `TC-NFR-03` | **Category A** | **Unchanged:** Verifies 100% record retention after normal exit and restart. | PR #101 (Issue #39) |
| `HMS-NF-004` | `TC-NFR-04` | `TC-NFR-04` | **Category A** | **Unchanged:** Performance benchmark with 1,000 synthetic records each. Verifies startup $\le 3.0\text{s}$ and operations $\le 2.0\text{s}$. Merged to CTest via PR #125 as `TC-NFR-04_Performance1000Records`. | PR #106, #125 (Issue #44) |
| `HMS-NF-005` | `TC-NFR-05` | `TC-NFR-05` | **Category A** | **Unchanged:** Usability review verifying prompts state format, error messages state violated rule, and menu depth $\le 3$. | PR #102 (Issue #40) |
| `HMS-NF-006` | `TC-NFR-06` | `TC-NFR-06` | **Category A** | **Unchanged:** Architectural class isolation: verifies 5 classes (`Patient`, `Doctor`, `Appointment`, `HospitalSystem`, `FileManager`) and confirms file I/O is restricted to `FileManager`. | PR #104 (Issue #42) |
| `HMS-NF-007` | `TC-NFR-07` | `TC-NFR-07` | **Category E** | **Testing Gap / Build-Time Verification:** Cross-platform compilation on Windows and Linux with `-std=c++17 -Wall`. Verified via build configuration; lacks dedicated runtime CTest target executable. | PR #107 (Issue #45) |
| `HMS-NF-008` | `TC-NFR-08` | `TC-NFR-08` | **Category A** | **Unchanged:** Verifies 100% accuracy of appointment conflict detection across all 4 matrix cases. In CTest: `TC-NFR-08_AppointmentConflictCorrectness`. | PR #103 (Issue #41) |
| `HMS-NF-009` | `TC-NFR-09` | `TC-NFR-09` | **Category A** | **Unchanged:** Verifies standalone execution without network sockets or external database servers. In CTest: `TC-NFR-09_StandaloneOffline`. | PR #105 (Issue #43) |
| `HMS-NF-010` | `TC-NFR-10` | `TC-NFR-10` | **Category A** | **Unchanged:** Human-readable CSV formatting: verifies valid headers, delimiter correctness, and text editor readability. | PR #108 (Issue #46) |

---

### 3.7 Security Requirements (`HMS-SR-001` to `HMS-SR-008`)

| Req ID | STP Test Case ID | Updated Test Case ID | Category | Change Description & Rationale | Merged PR / Issue |
|:---:|:---:|:---:|:---:|:---|:---:|
| `HMS-SR-001` | `TC-SEC-01` | `TC-SEC-01` | **Category B** | **Enhanced:** Enforces accept-list character set validation across all domain inputs before modifying state. | PR #109 (Issue #47) |
| `HMS-SR-002` | `TC-SEC-02` | `TC-SEC-02` | **Category A** | **Unchanged:** Rejects delimiter and formula injection characters (`,`, `\r`, `\n`, `=`, `+`, `-`, `@`). | PR #110 (Issue #48) |
| `HMS-SR-003` | `TC-SEC-03` | `TC-SEC-03` | **Category A** | **Unchanged:** Buffer safety: tests 10,000+ character inputs to guarantee zero memory corruption or crashes. | PR #111 (Issue #49) |
| `HMS-SR-004` | `TC-SEC-04` | `TC-SEC-04` | **Category B** | **Enhanced (Discrepancy Note):** Untrusted CSV data validation. Exercised via `TC-PER-03_SkipMalformedCSV` and `TC-NFR-02_RobustStartup`. | PR #112 (Issue #50) |
| `HMS-SR-005` | `TC-PER-07` | `TC-PER-07` | **Category B** | **Enhanced:** Non-destructive atomic file writes: official STP test ID is `TC-PER-07` (alias `TC-SEC-05` in sprint notes). Atomic non-destructive write logic verified via `TC-PER-07_HandleCSVWriteFailures` (PR #113). | PR #113 (Issue #51) |
| `HMS-SR-006` | `TC-APT-09` | `TC-APT-09` | **Category A** | **Unchanged:** Audit non-deletion: cancelled appointments retained in CSV with status `Cancelled`. Official STP test ID is `TC-APT-09` (alias `TC-SEC-06`). Dedicated security CTest target `TC-SEC-06_RetainCancelledAppointmentRecords` added in PR #114. | PR #114 (Issue #52) |
| `HMS-SR-007` | `TC-SEC-07` | `TC-SEC-07` | **Category A** | **Unchanged:** Patient privacy minimization: conceals address, phone, and medical history from appointment views. | PR #115 (Issue #53) |
| `HMS-SR-008` | `TC-NFR-09` | `TC-NFR-09` | **Category A** | **Unchanged:** Air-gapped offline operation: official STP test ID is `TC-NFR-09` (alias `TC-SEC-08`). Verified via objdump network symbol inspection in `test_standalone_offline.cmake` (`TC-NFR-09_StandaloneOffline`). Hardened via PR #116. | PR #105, #116 (Issue #43, #54) |

---

### 3.8 Supplementary Sprint Scope (Additions Beyond Baseline 54)

| Scope ID | Associated Test ID | Category | Implementation & Testing Details | Merged PR / Issue |
|:---|:---:|:---:|:---|:---:|
| **HMS-SR-009** | `TC-SEC-09` | **Category D** | **Supplementary Project Scope ID (Not in baseline STP): Sanitize Error Messages:** Error messages omit raw filesystem paths, exception traces, or raw file lines. In CTest: `TC-SEC-09_SanitizeErrorMessages`. | PR #117 (Issue #55) |
| **HMS-SR-010** | `TC-SEC-10` | **Category D** | **Supplementary Project Scope ID (Not in baseline STP): Restrict Data Directory:** Restricts file I/O strictly to designated data directory, preventing directory traversal. In CTest: `TC-SEC-10_RestrictDataDirectory`. | PR #118 (Issue #56) |
| **HMS-F-023-EXT** | `TC-APT-11` | **Category C** | **Supplementary Project Scope ID (Not in baseline STP): Cancellation Confirmation Workflow:** Displays appointment summary before asking for confirmation (`Y/N`). Retries invalid confirmation. | PR #122 (Issue #120) |
| **HMS-F-022-EXT** | `TC-APT-12` | **Category C** | **Supplementary Project Scope ID (Not in baseline STP): Appointment Filtering Entity Existence:** Validates patient/doctor existence when filtering. Nonexistent entity displays `"Patient not found"` / `"Doctor not found"`. In CTest: `TC-APT-12_FilterUnknownEntity`. | PR #123 (Issue #121) |
| **HMS-F-001/008-EXT**| `TC-ID-01` | **Category C** | **Supplementary Project Scope ID (Not in baseline STP): Duplicate Identity Heuristics:** Detects duplicate individuals (patient: name+contact; doctor: name+contact+specialization) and requests confirmation before saving. In CTest: `TC-ID-01_DuplicateIdentityValidation`. | PR #125 (Issue #124) |
| **HMS-F-003/010/018-EXT**| `TC-VAL-01` | **Category B / C** | **Supplementary Project Scope ID (Not in baseline STP): Immediate Field Validation & Retry:** Prompts for immediate retry of invalid field without aborting the entire form. | PR #127 (Issue #119) |

---

## 4. Discrepancies and Mapping Rationales

1. **`TC-PAT-01` Registration Mapping:** STP Table 7 designated `TC-PAT-01` for patient registration. In the implementation, registration is exercised as a foundational prerequisite across multiple tests and specifically verified for immediate CSV writing in `tests/test_patient_details_and_list.cpp` under CTest target `TC-PAT-05_PatientDetailsAndRegistrationPersistence`. `TC-PAT-01` is maintained as the logical test case ID.
2. **`TC-PER-04` vs `TC-PER-08`:** STP Table 7 numbered duplicate/orphan record skipping as `TC-PER-04`. In CTest and CMakeLists.txt, this was registered as `TC-PER-08_SkipDuplicateAndOrphanRecords` (PR #97). The updated RTM cross-references both IDs.
3. **`TC-PER-06` vs `TC-NFR-03`:** STP Table 7 numbered restart data retention as `TC-PER-06`. The automated test executable is `hms_data_retention_tests` registered as `TC-NFR-03_DataRetention`. Both requirement IDs (`HMS-F-030` and `HMS-NF-003`) map to this test suite.
4. **`TC-UI-03` Transactional State Protection:** STP Table 7 lists `TC-UI-03`. The implementation asserts state protection across several focused unit tests (e.g. `test_reject_invalid_menu_options.cpp` and `test_csv_write_failures.cpp`) without a dedicated standalone executable.
5. **`TC-NFR-07` Cross-Platform Portability:** Verified at compile time via CMake configuration flags (`-std=c++17 -Wall`) and multi-compiler support, but has no runtime CTest target executable. Formally flagged as a minor test infrastructure gap (Category E).
6. **`TC-SEC-04` Untrusted CSV Validation:** Tested through `TC-PER-03_SkipMalformedCSV` and `TC-NFR-02_RobustStartup` which exhaustively feed corrupted doctor and appointment CSV rows to verify safe line skipping.
