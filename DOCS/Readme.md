# Project Documentation

## 1. Overview

This directory contains the technical documentation, software specifications, testing reports, and sprint deliverables for the Hospital Management System (HMS).

The documents provide a structured reference for understanding the project requirements, system architecture, implementation verification, testing activities, and development progress.

The documentation is organized into the following categories:

- Software Requirements and Architecture
- Software Testing and Verification
- Sprint Deliverables and Demonstrations

## 2. Documentation Structure

```text
DOCS/
│
├── README.md
│
├── HMS_SRS_Final.pdf
├── SAD.pdf
├── Software_Test_Plan.pdf
│
├── TEST_DOCS/
│   ├── Unit_Test_Report.pdf
│   ├── System_Validation_Report.pdf
│   ├── Code_Coverage_Report.pdf
│   └── Branch_Coverage_Report.pdf
│
├── SPRINT1_DELEVERABLES/
│   ├── Sprint1_Report.pdf
│   ├── SPRINT1_SUBMISSION.md
│   └── SPRINT1_DEMO.mp4
│
└── SPRINT2_DELIVERABLES/
    ├── SPRINT2_SUBMISSION.md
    └── HMS_SPRINT2_DEMO.mp4
```

## 3. Software Requirements and Architecture

### 3.1 Software Requirements Specification

**File:** `HMS_SRS_Final.pdf`

Defines the functional, non-functional, and security requirements of the Hospital Management System.

It serves as the primary reference for:

- System objectives and scope.
- Functional requirements.
- Non-functional requirements.
- Security requirements.
- Business rules and validation constraints.
- Expected system behaviour and acceptance criteria.

### 3.2 Software Architecture Document

**File:** `SAD.pdf`

Describes the architectural design and organization of the Hospital Management System.

It provides the design reference for:

- System architecture and component organization.
- Responsibilities of the major system components.
- Class-level design and relationships.
- Data flow and interaction between components.
- Design decisions and implementation structure.

### 3.3 Software Test Plan

**File:** `Software_Test_Plan.pdf`

Defines the testing strategy used to verify the Hospital Management System against its requirements.

It covers:

- Testing objectives and scope.
- Testing levels and approach.
- Test environment and tools.
- Functional and non-functional verification.
- Security-related test coverage.
- Test execution and completion criteria.

## 4. Testing and Verification Reports

**Directory:** `TEST_DOCS/`

This directory contains the reports generated from the project's testing and verification activities.

| Document | Description |
|---|---|
| `Unit_Test_Report.pdf` | Records unit-level test execution results and the corresponding test cases. |
| `System_Validation_Report.pdf` | Documents system-level validation scenarios and their observed results. |
| `Code_Coverage_Report.pdf` | Reports code coverage measurements for the tested implementation. |
| `Branch_Coverage_Report.pdf` | Reports branch coverage measurements and the execution of decision paths. |

These reports provide supporting evidence for evaluating implementation correctness, requirement verification, and test coverage.

## 5. Sprint Deliverables

Sprint-specific reports, submission documents, and demonstration recordings are maintained separately to preserve the development history of the project.

### 5.1 Sprint 1 Deliverables

**Directory:** `SPRINT1_DELEVERABLES/`

Sprint 1 contains the initial development submission and its supporting demonstration.

| File | Description |
|---|---|
| `Sprint1_Report.pdf` | Documents the Sprint 1 work and submission details. |
| `SPRINT1_SUBMISSION.md` | Provides the Markdown submission record and deliverable references. |
| `SPRINT1_DEMO.mp4` | Contains the Sprint 1 application demonstration. |

### 5.2 Sprint 2 Deliverables

**Directory:** `SPRINT2_DELIVERABLES/`

Sprint 2 contains the subsequent development submission and its supporting demonstration.

| File | Description |
|---|---|
| `SPRINT2_SUBMISSION.md` | Provides the Sprint 2 submission record, implementation summary, and supporting verification references. |
| `HMS_SPRINT2_DEMO.mp4` | Contains the Sprint 2 application demonstration. |

## 6. Documentation Usage

The documents in this directory should be referred to throughout the project lifecycle.

| Activity | Reference Document |
|---|---|
| Understanding project requirements | `HMS_SRS_Final.pdf` |
| Understanding system design | `SAD.pdf` |
| Planning and executing tests | `Software_Test_Plan.pdf` |
| Reviewing unit test results | `TEST_DOCS/Unit_Test_Report.pdf` |
| Reviewing system validation | `TEST_DOCS/System_Validation_Report.pdf` |
| Reviewing code coverage | `TEST_DOCS/Code_Coverage_Report.pdf` |
| Reviewing branch coverage | `TEST_DOCS/Branch_Coverage_Report.pdf` |
| Reviewing Sprint 1 progress | `SPRINT1_DELEVERABLES/` |
| Reviewing Sprint 2 progress | `SPRINT2_DELIVERABLES/` |

## 7. Documentation Maintenance

The documentation is maintained alongside the source code in the project repository.

The following practices are followed:

1. Requirements and design documents serve as references during implementation.
2. Testing reports record the results of the corresponding verification activities.
3. Sprint folders preserve their respective submission documents and demonstrations.
4. Documentation is updated when relevant project changes are completed and verified.
5. Reported test results should correspond to the actual recorded test executions.

## 8. Related Project Directories

The documentation in this directory is associated with the following repository components:

- `src/` — C++ implementation files.
- `include/` — public header files and class declarations.
- `tests/` — test programs and verification scripts.
- `CMakeLists.txt` — project build and test configuration.

---
