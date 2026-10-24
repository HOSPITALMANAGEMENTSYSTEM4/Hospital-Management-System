# Hospital Management System — Sprint 1 Submission

**Sprint:** 1
**Sprint dates:** 19–23 October 2026
**Prepared:** 24 October 2026
**Repository:** <https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System>

**MANUAL_TESTING_VIDEO:** <https://drive.google.com/file/d/1dZwBXsWHjWfXe4_V5-LoogPxTr0uLehA/view?usp=drive_link>

---

## 1. Submission Overview

This document consolidates the Sprint 1 submission checklist and test evidence matrix into one Markdown file. It summarizes the delivered implementation and documentation, records the automated test targets and their available evidence references, and identifies any evidence that must still be captured locally before submission to the course/LMS.

The checklist is based on the Sprint 1 plan in the project README and evidence visible in the GitHub repository. If the course/LMS provides a separate submission rubric, compare it with this document before uploading.

## 2. Sprint 1 Deliverables

- [x] **C++17 implementation:** source code, CMake configuration, and CTest test targets are present in the repository.
- [x] **Software Requirements Specification (SRS):** [`DOCS/HMS_SRS_Final.pdf`](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/blob/main/DOCS/HMS_SRS_Final.pdf).
- [x] **System Architecture Document (SAD):** [`DOCS/SAD.pdf`](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/blob/main/DOCS/SAD.pdf).
- [x] **Software Test Plan:** [`DOCS/Software_Test_Plan.pdf`](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/blob/main/DOCS/Software_Test_Plan.pdf).
- [x] **Project README/build overview:** [`README.md`](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/blob/main/README.md).
- [x] **Issue and pull-request traceability:** relevant merged pull requests are linked in Section 4.
- [x] **Automated test registration:** the current `main` CMake configuration lists 24 CTest targets.
- [x] **Sprint 1 completion report:** prepared separately in PDF and DOCX formats.
- [x] **Demonstration runbook:** prepared separately as a Markdown artifact.

## 3. Test Evidence Matrix

The current `main` CMake configuration lists 24 test registrations. PR #90 reports that the full CTest suite passed 24/24. A local terminal log after Issue #28 integration records 21/21 passed before the later PR #88–#90 increments. The table below distinguishes test registration and referenced evidence from a fresh local run.

| Test case | Area | Purpose | Evidence reference/status |
|---|---|---|---|
| `TC-PAT-02_DuplicatePatientId` | Patient | Reject duplicate patient IDs | Existing test target |
| `TC-PAT-03_ValidatePatientFields` | Patient | Validate patient fields | Existing test target |
| `TC-DOC-01_AddDoctor` | Doctor | Register a doctor | Existing test target |
| `TC-DOC-02_RejectDuplicateDoctorId` | Doctor | Reject duplicate doctor IDs | Existing test target |
| `TC-DOC-03_ValidateDoctorFields` | Doctor | Validate doctor fields | Existing test target |
| `TC-DOC-04_SearchDoctors` | Doctor | Search doctors | Existing test target |
| `TC-PAT-04_SearchPatients` | Patient | Search patients | Existing test target |
| `TC-APT-02_RejectUnknownPatient` | Appointments | Reject booking for an unknown patient | Existing test target |
| `TC-APT-01_BookAppointment` | Appointments | Book an appointment | Existing test target |
| `TC-APT-04_ValidateAppointmentDateTime` | Appointments | Validate appointment date and time | Existing test target |
| `TC-APT-03_RejectUnknownDoctor` | Appointments | Reject booking for an unknown doctor | Existing test target |
| `TC-DOC-05_ViewAndListDoctors` | Doctor | View and list doctors | Existing test target |
| `TC-PER-01_StartupCSVLoading` | Persistence | Load CSV records at startup | PR [#77](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/77) |
| `TC-PER-02_MissingCSVFiles` | Persistence | Handle missing CSV files | PR [#78](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/78) |
| `TC-PAT-05_PatientDetailsAndRegistrationPersistence` | Patient/Persistence | Patient details/listing and immediate registration persistence | PR [#80](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/80) |
| `TC-PER-03_SkipMalformedCSV` | Persistence | Skip malformed CSV records | PR [#79](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/79) |
| `TC-UI-01_MenuNavigation` | UI | Navigate the main menu and submenus | PR [#82](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/82) |
| `TC-UI-02_RejectInvalidMenuOptions` | UI | Reject invalid menu options | PR [#83](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/83) |
| `TC-PAT-06_UpdatePatientDetails` | Patient | Update patient details | PR [#85](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/85); prior local 159-assertion/check log reported |
| `TC-PAT-07_PreventPatientIdEdits` | Patient | Prevent patient ID edits | PR [#86](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/86); focused test passed in the prior 21-test local run |
| `TC-DOC-06_UpdateDoctorDetails` | Doctor | Update doctor details | PR [#87](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/87); prior local focused test and 168 assertions reported passed |
| `TC-DOC-07_PreventDoctorIdEdits` | Doctor | Prevent doctor ID edits | PR [#88](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/88) |
| `TC-PER-07_HandleCSVWriteFailures` | Persistence | Handle CSV write failures safely | PR [#89](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/89); focused test reported passed |
| `TC-NFR-01_InvalidInputCrashResilience` | Robustness/NFR | Exercise resilience across 20 invalid-input vectors | PR [#90](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/90) reports 20/20 vectors and full suite 24/24 passed |

## 4. Merged Pull-Request Traceability

- **Patient registration and validation — [#61](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/61), [#63](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/63):** patient registration and field validation.
- **Patient search, details, and listing — [#67](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/67), [#68](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/68), [#70](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/70), [#71](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/71):** patient search, details, and listing coverage.
- **Doctor registration, validation, search, and listing — [#64](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/64), [#65](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/65), [#66](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/66), [#72](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/72), [#73](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/73):** doctor registration, validation, details, search, and listing.
- **Appointment booking and input validation — [#69](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/69), [#74](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/74), [#75](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/75), [#76](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/76):** booking, unknown patient/doctor rejection, and date/time validation.
- **CSV loading, malformed/missing files, and persistence — [#77](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/77), [#78](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/78), [#79](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/79), [#80](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/80), [#81](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/81):** startup loading, missing/malformed file handling, immediate save, and restart persistence.
- **Menu navigation and invalid menu handling — [#82](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/82), [#83](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/83):** menu/submenu navigation and invalid option handling.
- **State protection and patient update rules — [#84](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/84), [#85](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/85), [#86](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/86):** failed-operation state protection, patient updates, and immutable patient IDs.
- **Doctor update and immutable doctor IDs — [#87](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/87), [#88](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/88):** doctor detail updates, validation, and immutable doctor IDs.
- **CSV write-failure safety — [#89](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/89):** safe handling of CSV write failures.
- **Invalid-input crash resilience — [#90](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System/pull/90):** 20 invalid-input vectors; PR reports 24/24 CTest tests passing.

## 5. Suggested Course/LMS Upload Set

1. `01_HMS_Sprint1_Completion_Report.pdf` — primary report.
2. `01_HMS_Sprint1_Completion_Report.docx` — editable report, if accepted.
3. This file, `SPRINT1_SUBMISSION.md` — consolidated checklist and test evidence matrix.
4. Actual test log from the current `main`, if requested.
5. Genuine application screenshots/video and sprint-board evidence, if requested.
6. SRS, SAD, and Software Test Plan PDFs if the LMS requires copies rather than repository links.
7. Repository: <https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System>.

## 6. Evidence and Verification Caveat

A local verification log after incorporating Issue #28 reports **21/21 tests passed**. The later merged PR #90 reports **24/24 tests passed**, and the current `main` CMake configuration lists 24 test registrations. These are different points in the repository history. This document does **not** represent the earlier 21-test local run as a fresh local run of the latest 24-test `main`.

For high-confidence submission, run the build and full test suite locally from freshly updated `main` and save the terminal output if the instructor expects run evidence. A test target being registered is not, by itself, proof that it was executed successfully in a fresh local run.
