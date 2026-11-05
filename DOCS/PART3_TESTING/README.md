# Hospital Management System (HMS)
## Deliverable Part-3: Testing Documentation & Requirements Traceability
### Comprehensive Sprint 1 + Sprint 2 Implementation Traceability

**Academic Program:** Software Engineering Mini-Project
**Department:** Department of Computer Science and Engineering
**Institution:** PES University, Bengaluru
**Course Code:** Software Engineering Laboratory / Mini-Project
**Document Version:** 2.0 (Post-Sprint 1 & Sprint 2 Baseline Update)
**Date:** November 2026
**Repository:** [HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System)
**Target Branch:** `main` (Commit: `542b90be8adaddf13f7ce900340973eb4c9f5a56`)

---

## 1. Project Team & Roles

| Name | Student Registration Number (SRN) | Project Role | Primary Testing Responsibility |
|:---|:---|:---|:---|
| **Hrushikesh Karnam** | PES1UG24CS668 | QA Lead | Test plan creation, test strategy definition, RTM maintenance, security validation |
| **Muruli H** | PES1UG24CS676 | Test Engineer | Test case design, appointment conflict matrix testing, CLI menu validation |
| **Mandagandla Jhansi** | PES1UG24CS672 | Developer Lead | Unit and integration testing, CSV file handling verification, defect fixing |
| **Neeta R** | PES1UG24CS680 | Support Lead | Test data generation, performance timing, cross-platform build testing |

---

## 2. Executive Purpose & Scope

The primary objective of **Deliverable Part-3 (Activity 1)** is to evolve the initial **Software Test Plan (STP-HMS-2026-V1.0)** into a comprehensive, production-aligned **Test Case Document** and **Requirements Traceability Matrix (RTM)** that rigorously reflect the **actual implementations delivered across Sprint 1 and Sprint 2**.

During Sprint 1 (19–23 October 2026) and Sprint 2 (26–30 October 2026), the Hospital Management System transitioned from initial structural prototypes to an enterprise-grade C++17 console application backed by resilient CSV persistence. Along this development lifecycle:
1. **Existing Baseline Features Were Enhanced:** Core operations (patient registration, doctor management, appointment booking, conflict checking, cancellation, and file persistence) received advanced input sanitization, immediate field-level validation with interactive retry, transactional state protection, and non-destructive atomic file writing.
2. **Supplementary Scope Was Introduced:** Additional security mechanisms (sensitive filesystem error sanitization, restricted data directory enforcement), advanced business logic (appointment cancellation confirmation workflows, entity-existence validation during appointment filtering), and data integrity guards (patient/doctor duplicate identity heuristics) were implemented via pull requests.

This documentation package provides 100% bidirectional traceability between:
- **Requirements:** Software Requirements Specification (SRS v1.0) baseline requirements (`HMS-F-001` to `HMS-F-036`, `HMS-NF-001` to `HMS-NF-010`, `HMS-SR-001` to `HMS-SR-008`) and supplementary sprint requirements.
- **Implementations:** Merged GitHub Issues and Pull Requests across Sprint 1 (PRs #60–#90) and Sprint 2 (PRs #91–#118, #122, #123, #125, #127).
- **Automated Verification:** The 51 automated CTest targets registered in CMake and implemented across the test suite in `tests/`.

---

## 3. Documentation Package Structure

This directory (`DOCS/PART3_TESTING/`) contains the complete academic deliverables for Part-3:

```text
DOCS/
└── PART3_TESTING/
    │
    ├── README.md                                       <- Package overview, indexing, and project metadata (this document)
    │
    ├── 01_Updated_Test_Case_Document.md               <- Comprehensive Test Case Document (all 60 detailed test specifications)
    ├── 01_Updated_Test_Case_Document.pdf              <- Formatted publication PDF for 01_Updated_Test_Case_Document
    │
    ├── 02_Updated_Requirements_Traceability_Matrix.md  <- Complete RTM (Part A: 54 Baseline + Part B: Supplementary Scope)
    ├── 02_Updated_Requirements_Traceability_Matrix.pdf <- Formatted publication PDF for 02_Updated_Requirements_Traceability_Matrix
    │
    ├── 03_Sprint_1_Implementation_Traceability.md     <- Sprint 1 deep-dive: PRs #60–#90, issues, modified code, and test impact
    ├── 04_Sprint_2_Implementation_Traceability.md     <- Sprint 2 deep-dive: PRs #91–#127, issues, modified code, and test impact
    └── 05_Test_Case_Update_Change_Log.md              <- Granular change log classifying all modifications into Categories A–E
```

---

## 4. Key Metrics & Traceability Summary

### A. Baseline Requirements (STP v1.0 Frozen Denominator = 54)
| Requirement Category | Requirement IDs | Total Baseline | Designed Test Coverage | Coverage % |
|:---|:---|:---:|:---:|:---:|
| **Functional Requirements (FR)** | `HMS-F-001` to `HMS-F-036` | 36 | 36 | **100.0%** |
| **Non-Functional Requirements (NFR)** | `HMS-NF-001` to `HMS-NF-010` | 10 | 10 | **100.0%** |
| **Security Requirements (SR)** | `HMS-SR-001` to `HMS-SR-008` | 8 | 8 | **100.0%** |
| **Total Baseline Scope** | — | **54** | **54** | **100.0%** |

*Note: The official 54-requirement denominator from STP v1.0 is strictly preserved. No new requirements are silently injected into the baseline.*

### B. Supplementary Sprint Scope (Sprint 1 & Sprint 2 Additions)
| Supplementary Requirement ID | Title / Scope | Originating Issue / PR | Associated Test Case ID | Current CTest Target |
|:---|:---|:---|:---|:---|
| **HMS-SR-009** | Filesystem Error Message Sanitization | Issue #55 / PR #117 | `TC-SEC-09` | `TC-SEC-09_SanitizeErrorMessages` |
| **HMS-SR-010** | Strict Data Directory Access Restriction | Issue #56 / PR #118 | `TC-SEC-10` | `TC-SEC-10_RestrictDataDirectory` |
| **HMS-F-022-EXT** | Entity Existence Check in Appointment Views | Issue #121 / PR #123 | `TC-APT-12` | `TC-APT-12_FilterUnknownEntity` |
| **HMS-F-001/008-EXT**| Duplicate Identity Detection (Patients/Doctors) | Issue #124 / PR #125 | `TC-ID-01` | `TC-ID-01_DuplicateIdentityValidation` |
| **HMS-F-023-EXT** | Appointment Cancellation Confirmation Workflow | Issue #120 / PR #122 | `TC-APT-11` | `TC-APT-09_CancelAppointment` / `TC-APT-10` |
| **HMS-F-003/010/018-EXT**| Immediate Field Validation & Retry Handling | Issue #119 / PR #127 | `TC-VAL-01` | Integrated across `TC-PAT-03`, `TC-DOC-03`, `TC-APT-04` |

### C. Automated Test Infrastructure
- **Total Registered CTest Targets:** **51**
- **Test Executable Targets:** 49 compiled test executables + 2 custom CMake test drivers (`test_exit_process.cmake`, `test_standalone_offline.cmake`).
- **Build System:** CMake 3.20+ with MinGW-w64 GCC 9+ / MSVC C++17 support.
- **Safety Guarantee:** Tests utilize isolated temporary runtime directories (`build/test_*`); root user data files (`patients.csv`, `doctors.csv`, `appointments.csv`) are strictly protected from modification.

---

## 5. Document Revision History

| Version | Date | Author(s) | Description of Changes |
|:---|:---|:---|:---|
| **1.0** | 20-Oct-2026 | QA Team | Initial Software Test Plan (STP-HMS-2026-V1.0) baseline with 54 requirements and 10 representative test cases. *(Note: PDF cover page printed as '20 November 2026'; internal revision records cite 20-Oct-2026).* |
| **2.0** | 02-Nov-2026 | QA Team & Requirements Analysts | Comprehensive Deliverable Part-3 update. Expanded test specifications from 10 representative cases to 60 exhaustive test specifications (54 baseline + 6 supplementary sprint cases). Mapped all Sprint 1 and Sprint 2 merged PRs (#60–#127) to requirements, CTest targets, and source files. Updated RTM with Part A (Baseline) and Part B (Sprint Extensions). Documented all traceability discrepancies and testing gaps. Status: Pending Course Evaluator Review. |
