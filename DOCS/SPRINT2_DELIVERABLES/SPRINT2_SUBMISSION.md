# Hospital Management System — Sprint 2 Submission

**Sprint:** 2

**Sprint dates:** 26–30 October 2026

**SPRINT-2 DEMO VIDEO**:  [DEMO VIDEO](https://drive.google.com/file/d/13Q18OQGE-_3P8JkjGQkF-jiwejIb11ye/view?usp=drive_link)

**DEMONSTRATION_VIDEO:** `HMS_SPRINT2_DEMO.mp4` (included in this deliverables folder)

---

## 1. Submission Overview

This document consolidates the Sprint 2 submission checklist and implementation evidence matrix into one Markdown file. It summarizes the functionality delivered during Sprint 2, records the related GitHub issues and pull requests, identifies the automated test targets and their reported execution evidence, and lists the supporting demonstration and documentation artifacts.

Sprint 2 extended the initial Hospital Management System implementation with additional appointment workflows, stronger input validation, improved data integrity, more robust CSV handling, and supplementary security controls. The implementation remained a standalone, single-user C++17 console application using local CSV files.

The checklist is based on the Sprint 2 implementation history and the available GitHub pull-request descriptions. Reported test results are attributed to their respective pull requests; they should not be interpreted as a fresh local test execution unless separately verified.

## 2. Sprint 2 Deliverables

- [x] **C++17 implementation:** Sprint 2 functionality is present in the repository through the recorded implementation pull requests.
- [x] **Appointment availability and conflict handling:** Doctor availability checks, conflict prevention, and cancelled-slot reuse were implemented.
- [x] **Appointment management improvements:** Appointment filtering, cancellation handling, and validation workflows were extended.
- [x] **CSV persistence and integrity:** Malformed record handling, duplicate/orphan record handling, restart retention, and non-destructive file-write behavior were strengthened.
- [x] **Security improvements:** Input validation, CSV safety, offline-operation checks, error-message sanitization, and data-directory restrictions were implemented.
- [x] **Duplicate identity validation:** Patient and doctor duplicate-identity heuristics were added with explicit user confirmation.
- [x] **Immediate field validation and retry:** Invalid field entries are rejected immediately and the user is prompted to retry the same field.
- [x] **Automated test registration:** The updated test infrastructure reports 51 registered CTest targets in the later Sprint 2 implementation history.
- [x] **Sprint 2 demonstration video:** `HMS_SPRINT2_DEMO.mp4` is included in this folder.
- [x] **Sprint 2 submission report:** This Markdown file consolidates the implementation and evidence references.

**Supporting documentation:** The repository's Deliverable Part-3 testing documentation update (PR #128) describes a comprehensive Sprint 1 and Sprint 2 Test Case Document and Requirements Traceability Matrix. PR #128 is currently closed but not merged, so its contents should be treated as a supporting review artifact, not as a merged `main`-branch deliverable.

## 3. Test Evidence Matrix

The Sprint 2 implementation history records test execution at multiple development stages. The table below identifies the principal areas, associated evidence, and reported results. Test totals are retained as reported by the corresponding pull requests; they are not combined into a single cumulative run.

| Test case / area | Area | Purpose | Evidence reference / reported status |
|---|---|---|---|
| `TC-APT-05` | Appointments | Reject conflicting bookings for the same doctor, date, and time | PR #91 — reported implementation and test evidence |
| `TC-APT-06` | Appointments | Check doctor availability without modifying records | PR #92 — reported implementation and test evidence |
| `TC-APT-07` | Appointments | Allow rebooking of slots belonging to cancelled appointments | PR #93 — reported implementation and test evidence |
| `TC-APT-08` | Appointments | List appointments in chronological order | PR #94 — reported implementation and test evidence |
| `TC-APT-09` | Appointments | Cancel appointments while retaining the cancelled record | PR #95; later confirmation workflow in PR #122 |
| `TC-APT-10` | Appointments | Reject nonexistent and already-cancelled appointment IDs | PR #96; retry handling enhanced in PR #127 |
| `TC-PER-08` | Persistence | Skip duplicate records and orphan appointments during loading | PR #97 — CTest mapping documented in the updated traceability material |
| `TC-UI-05` | User Interface | Verify clean application exit and process return code | PR #99 — implementation evidence |
| `TC-NFR-02` | Robustness | Handle missing, empty, or malformed CSV files at startup | PR #100 — implementation evidence |
| `TC-NFR-03` | Persistence | Verify data retention after application restart | PR #101 — implementation evidence |
| `TC-NFR-05` | Usability | Review prompt clarity, error feedback, and menu depth | PR #102 — implementation evidence |
| `TC-NFR-08` | Appointments | Verify appointment conflict-detection correctness | PR #103 — implementation evidence |
| `TC-NFR-06` | Architecture | Verify separation of domain, system, and file-management responsibilities | PR #104 — implementation evidence |
| `TC-NFR-09` | Offline operation | Verify standalone operation and inspect common network dependencies | PR #105 and #116 — PR #116 reports 46/46 CTest tests passed |
| `TC-NFR-04` | Performance | Benchmark operations using 1,000 synthetic records | PR #106; later CTest registration reported in PR #125 |
| `TC-NFR-07` | Portability | Check C++17 and cross-platform build configuration | PR #107 — build-time verification; no dedicated runtime CTest target identified in the traceability review |
| `TC-NFR-10` | Data format | Verify human-readable CSV formatting | PR #108 — implementation evidence |
| `TC-SEC-01` | Security | Enforce accepted input character sets | PR #109 — implementation evidence |
| `TC-SEC-02` | Security | Reject CSV delimiter, newline, and formula-injection inputs | PR #110 — implementation evidence |
| `TC-SEC-03` | Security | Exercise buffer safety with inputs exceeding 10,000 characters | PR #111 — implementation evidence |
| `TC-PER-03` | Persistence / Security | Safely skip malformed or untrusted CSV records | PR #112 — implementation evidence |
| `TC-PER-07` | Persistence / Security | Preserve existing CSV data when file writes fail | PR #113 — non-destructive atomic-write implementation |
| `TC-SEC-06` | Security | Retain cancelled appointment records for audit integrity | PR #114 — implementation evidence |
| `TC-SEC-07` | Security / Privacy | Minimize patient information exposed in appointment views | PR #115 — implementation evidence |
| `TC-NFR-09` | Offline operation | Strengthen static network API and executable dependency checks | PR #116 — reported 46/46 CTest tests passed |
| `TC-SEC-09` | Security | Prevent filesystem paths and exception details from appearing in user-facing errors | PR #117 — reported 47/47 CTest tests passed |
| `TC-SEC-10` | Security | Restrict file access to approved data files and reject unsafe paths | PR #118 — reported 48/48 CTest tests passed |
| `TC-APT-11` | Appointments | Display appointment details and obtain confirmation before cancellation | PR #122 — reported full suite 48/48 passed |
| `TC-APT-12` | Appointments | Reject filters referencing nonexistent patients or doctors | PR #123 — reported 49/49 CTest tests passed; dedicated test reports 69 assertions passed |
| `TC-ID-01` | Data integrity | Detect potential duplicate patient and doctor identities and request confirmation | PR #125 — reported 51/51 CTest tests passed |
| `TC-VAL-01` | Input validation | Retry invalid fields immediately without restarting the complete form | PR #127 — reported 51/51 CTest tests passed |

### Test Evidence Interpretation

The reported full-suite totals represent different development stages:

- PR #116 reports 46/46 tests passed.
- PR #117 reports 47/47 tests passed.
- PR #118 reports 48/48 tests passed.
- PR #123 reports 49/49 tests passed.
- PR #125 reports 51/51 tests passed.
- PR #127 reports 51/51 tests passed after its immediate-validation and retry changes.

These results are historical pull-request execution reports. They should not be represented as one combined execution or as a fresh run performed specifically for this submission.

## 4. Sprint 2 Pull-Request Traceability

The following pull requests document the primary Sprint 2 implementation work.

- **Appointment conflict prevention and availability — [#91](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/91), [#92](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/92), [#93](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/93):** prevent double booking, check doctor availability, and permit reuse of cancelled appointment slots.

- **Appointment viewing and cancellation — [#94](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/94), [#95](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/95), [#96](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/96):** chronological appointment listing, cancellation status handling, and invalid/already-cancelled appointment checks.

- **CSV data integrity and application workflow — [#97](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/97), [#98](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/98), [#99](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/99):** duplicate/orphan record handling, operation feedback, and clean application exit.

- **Robustness, persistence, usability, and architecture — [#100](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/100), [#101](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/101), [#102](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/102), [#103](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/103), [#104](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/104):** robust startup, restart data retention, usability review, appointment conflict correctness, and class responsibility verification.

- **Performance, portability, and CSV format — [#105](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/105), [#106](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/106), [#107](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/107), [#108](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/108):** offline operation, 1,000-record performance benchmarking, cross-platform build configuration, and CSV formatting.

- **Input and CSV security — [#109](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/109), [#110](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/110), [#111](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/111), [#112](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/112):** accepted-character validation, CSV/formula injection protection, long-input buffer safety, and malformed CSV validation.

- **Safe persistence and privacy — [#113](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/113), [#114](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/114), [#115](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/115):** non-destructive atomic writes, cancelled-appointment retention, and patient data minimization.

- **Offline verification and supplementary security — [#116](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/116), [#117](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/117), [#118](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/118):** offline dependency checks, sanitized error messages, and restricted data-directory access.

- **Appointment cancellation confirmation — [#122](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/122):** validates the appointment before confirmation, displays the appointment summary, and handles confirm/decline responses.

- **Appointment filter entity validation — [#123](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/123):** checks whether a patient or doctor exists before filtering appointments and distinguishes missing entities from entities with no appointments.

- **Validation, identity integrity, and test coverage — [#125](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/125):** adds duplicate identity detection, explicit confirmation, a 1,000-record benchmark target, and reports 51/51 CTest tests passed.

- **Immediate input validation and retry — [#127](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/127):** improves field-level retry handling and appointment ID/confirmation validation; reports 51/51 CTest tests passed.

### Supporting Testing Documentation

[PR #128 — Test Case Document and RTM update](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/128) contains the broader Sprint 1 and Sprint 2 implementation traceability package. It describes 60 detailed test case specifications, including 54 baseline requirements and 6 supplementary sprint capabilities. It is currently closed but not merged, so verify its review status before treating its files as approved repository deliverables.

## 5. Suggested Course/LMS Upload Set

1. `SPRINT2_SUBMISSION.md` — consolidated Sprint 2 submission checklist and implementation evidence matrix.
2. `HMS_SPRINT2_DEMO.mp4` — Sprint 2 application demonstration video.
3. Updated source code and CMake/CTest configuration available in the GitHub repository.
4. Sprint 2 issue and pull-request history linked in Section 4.
5. Updated Test Case Document and Requirements Traceability Matrix, if accepted as supporting documentation and after confirming their review/approval status.
6. Relevant test execution logs or screenshots from the latest local build, if required by the course/LMS.
7. Repository: <https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System>.

## 6. Evidence and Verification Caveat

The Sprint 2 implementation history contains multiple pull-request test summaries. The reported CTest totals increased from 46/46 in PR #116 to 51/51 in PR #125 and remained 51/51 in PR #127. These figures refer to different points in the development history and are not a single test execution.

The implementation traceability material also identifies a cross-platform verification limitation: `HMS-NF-007` is documented as build-time verification and does not have a dedicated runtime CTest target in the reviewed mapping. Likewise, static checks for offline operation do not independently prove the absence of every possible runtime network transmission.

For a high-confidence final submission, run the complete build and test suite locally from the latest `main` branch and preserve the terminal output. Confirm that the submitted demonstration video is playable and shows the intended Sprint 2 workflows. Any course/LMS-specific rubric should also be checked before final upload.