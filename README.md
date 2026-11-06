# Hospital Management System (HMS)

A modular, standalone hospital administration application built with
**C++17**. HMS provides console-based workflows for managing patient
records, doctor information, and outpatient appointments, with local CSV
persistence and validation-focused safeguards.

> **Project:** Software Engineering Mini-Project\
> **Institution:** PES University, Department of Computer Science and
> Engineering\
> **Application model:** Single-user, offline, console-based\
> **Language standard:** ISO C++17

------------------------------------------------------------------------

## Table of Contents

-   [Overview](#overview)
-   [Objectives](#objectives)
-   [Scope and System Boundaries](#scope-and-system-boundaries)
-   [Features](#features)
-   [Architecture](#architecture)
-   [Technology Stack](#technology-stack)
-   [Repository Layout](#repository-layout)
-   [Requirements](#requirements)
-   [Build and Run](#build-and-run)
-   [Data Storage](#data-storage)
-   [Business Rules and Validation](#business-rules-and-validation)
-   [Security and Data Integrity](#security-and-data-integrity)
-   [Testing and Verification](#testing-and-verification)
-   [Coverage Results](#coverage-results)
-   [Project Documentation](#project-documentation)
-   [Development Workflow](#development-workflow)
-   [Project Team](#project-team)
-   [Known Verification Boundaries](#known-verification-boundaries)

------------------------------------------------------------------------

## Overview

The Hospital Management System (HMS) is a standalone C++17 console
application for essential hospital administration workflows. It enables
a clinic administrator or receptionist to maintain patient and doctor
records and manage outpatient appointments through a menu-driven
interface.

The application uses local CSV files for persistence. Its design
separates domain entities, application coordination, and file operations
so that validation, business rules, and storage responsibilities remain
clearly defined.

HMS is intentionally scoped as a **single-user, offline application**.
It is not a client-server or multi-user hospital platform.

## Objectives

-   Maintain accurate patient and doctor records.
-   Support appointment booking, viewing, availability checks, and
    cancellation.
-   Enforce unique identifiers and appointment referential integrity.
-   Prevent conflicting active appointments for the same doctor and time
    slot.
-   Validate input before records are changed or persisted.
-   Preserve records across application restarts.
-   Handle malformed input and file-operation failures safely.
-   Keep patient-sensitive details out of appointment summary views.
-   Provide a maintainable, modular C++17 implementation.

## Scope and System Boundaries

### In scope

-   Patient registration, search, viewing, listing, and permitted
    updates.
-   Doctor registration, search, viewing, listing, and permitted
    updates.
-   Appointment booking, filtering, availability checks, chronological
    viewing, and cancellation.
-   Input validation and business-rule enforcement.
-   Local CSV loading and saving.
-   Handling missing, empty, malformed, duplicate, and orphaned data
    records.
-   Automated unit, integration, regression, reliability, and
    security-related tests.

### Out of scope

The approved system boundary excludes:

-   Web and mobile interfaces.
-   Client-server networking, remote APIs, and online services.
-   Multi-user concurrency and session management.
-   External SQL database integration.
-   User authentication and role-based access control.
-   Billing, insurance claims, payment processing, and external
    EHR/HL7/FHIR integration.

These are deliberate scope boundaries, not features claimed to be
implemented.

## Features

### 1. Patient Management

-   Register patient records with the required demographic and contact
    details.
-   Reject duplicate patient identifiers, including case-insensitive
    duplicates.
-   Search by patient ID or name.
-   View an individual patient record and list registered patients.
-   Update permitted patient details with field validation and retry.
-   Preserve patient identity after registration.

### 2. Doctor Management

-   Register doctors with their identifiers, names, specialization,
    department, contact details, and availability.
-   Reject duplicate doctor identifiers.
-   Search by doctor ID, name, or specialization.
-   View individual doctor details and list registered doctors.
-   Update permitted doctor information and availability with
    validation.

### 3. Appointment Management

-   Book appointments for existing patients and doctors.
-   Generate sequential appointment identifiers (`A0001` onward).
-   Validate calendar dates and 24-hour time values.
-   Reject bookings that reference unknown patients or doctors.
-   Prevent double-booking of a doctor at the same date and time.
-   Treat cancelled appointments as released time slots.
-   View and filter appointments by supported criteria.
-   Sort appointment views chronologically.
-   Cancel appointments while retaining the appointment record and its
    `Cancelled` status.
-   Confirm cancellation before applying the status change.

### 4. CSV Persistence

-   Store patient, doctor, and appointment records in separate local CSV
    files.
-   Load records when the application starts.
-   Skip malformed, duplicate, and orphaned records according to the
    defined loading rules.
-   Persist successful changes immediately.
-   Use non-destructive write handling to protect previously valid data
    when a write fails.

### 5. Console Interaction and Validation

-   Provide a main menu and module-specific submenus.
-   Provide a return-to-main-menu option in submenus.
-   Reject non-numeric and out-of-range menu selections safely.
-   Validate fields immediately and re-prompt the user for invalid
    values.
-   Display operation confirmations and actionable validation errors.
-   Exit cleanly when the exit option is selected.

## Architecture

HMS uses a modular design with separation between domain data,
application workflows, and persistence.

  -----------------------------------------------------------------------
  Component                           Responsibility
  ----------------------------------- -----------------------------------
  `Patient`                           Patient attributes, domain
                                      validation, and patient record
                                      representation.

  `Doctor`                            Doctor attributes, specialization,
                                      department, availability, and
                                      validation.

  `Appointment`                       Appointment details, date/time
                                      validation, and booking status.

  `HospitalSystem`                    Coordinates application workflows,
                                      menu operations, entity lookups,
                                      and business rules.

  `FileManager`                       Encapsulates CSV parsing, loading,
                                      validation of persisted records,
                                      and file writes.

  Console interface                   Handles menus, prompts, user input,
                                      and displayed results.
  -----------------------------------------------------------------------

### High-level interaction

``` text
User
  |
  v
Console Menu / Input
  |
  v
HospitalSystem
  |-------------------|
  v                   v
Patient / Doctor   Appointment
  |
  v
FileManager
  |
  v
Local CSV Files
```

The diagram represents the logical responsibility flow; it is not a
deployment or network architecture.

## Technology Stack

  Area                                   Technology
  -------------------------------------- ----------------------------------
  Programming language                   C++
  Language standard                      C++17
  Build configuration                    CMake
  Build generator used in verification   Ninja
  Application interface                  Command-line console
  Persistence                            Local CSV files
  Automated test runner                  CTest
  Coverage instrumentation               LLVM/Clang source-based coverage
  Version control                        Git
  Collaboration and issue tracking       GitHub

## Repository Layout

The repository is organized around application headers, implementation
files, tests, data, and project documentation. The principal areas are:

``` text
Hospital-Management-System/
├── include/
│   └── hms/                 # Public HMS headers
├── src/                     # C++ implementation and application entry point
├── tests/                   # Automated C++ tests and test drivers
├── data/                    # Local application data area
├── docs/                    # Project and deliverable documentation
├── DOCS/                    # Additional project documentation, where present
├── CMakeLists.txt           # CMake project and test configuration
└── README.md                # Project overview and usage guide
```

Generated build directories (for example, `build/` and `build-llvm/`)
are local working artifacts and are not part of the source layout. Exact
generated files may vary by build configuration.

## Requirements

### Software

-   A C++17-compatible compiler.
-   CMake 3.20 or newer.
-   Ninja or another supported CMake build tool.
-   Git (to clone and manage the repository).

The project has been built and tested on Windows using MinGW-w64 GCC.
The test plan also specifies Ubuntu Linux 22.04 as a target environment;
local Ubuntu execution remains unverified.

## Build and Run

The following commands are for **Windows PowerShell**, executed from the
repository root.

### 1. Configure

``` powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

If Ninja is not installed or is not available on `PATH`, use a generator
supported by your installed CMake toolchain.

### 2. Build

``` powershell
cmake --build build --parallel 4
```

### 3. Run automated tests

``` powershell
ctest --test-dir build --output-on-failure
```

### 4. Launch the application

The executable is produced in the configured build directory. For the
verified Windows build layout:

``` powershell
.\build\hms.exe
```

If your generator or configuration places the executable elsewhere,
locate `hms.exe` under the selected build directory and run that
executable.

## Data Storage

HMS uses local CSV files for persistent records:

  -----------------------------------------------------------------------
  File                                Stored information
  ----------------------------------- -----------------------------------
  `patients.csv`                      Patient demographic and
                                      medical-note fields required by the
                                      application.

  `doctors.csv`                       Doctor identifiers, professional
                                      details, contact information, and
                                      availability.

  `appointments.csv`                  Appointment identifiers, patient
                                      and doctor references, date, time,
                                      and status.
  -----------------------------------------------------------------------

The application is designed to load these files at startup and persist
successful record changes. Run the application from its intended
working/data directory so that its configured data-path rules are
applied consistently.

**Data handling note:** Use synthetic data for development and testing.
Do not populate the repository with real patient information or other
protected health information.

## Business Rules and Validation

The implementation and test plan define important rules, including:

1.  Patient IDs and doctor IDs must be unique; duplicate checks are
    case-insensitive.
2.  Appointment bookings must reference existing patient and doctor
    records.
3.  Appointment dates must be valid calendar dates and times must follow
    the 24-hour `HH:MM` format.
4.  A doctor cannot have two active (`Booked`) appointments in the same
    date/time slot.
5.  A cancelled appointment remains in the records and no longer blocks
    its time slot.
6.  Invalid field input must be rejected before the corresponding record
    is modified.
7.  Appointment identifiers continue sequentially based on stored
    records.
8.  Invalid or orphaned persisted rows must not be admitted as valid
    application records.
9.  Appointment summaries must not disclose patient phone numbers,
    addresses, or medical notes.
10. Input validation must reject CSV delimiter/newline and
    spreadsheet-formula-prefix vectors specified by the security
    requirements.

## Security and Data Integrity

The project includes security and reliability controls appropriate to
its local, single-user design:

-   Allow-list and domain validation for supported input fields.
-   Rejection of comma, newline, carriage-return, and formula-prefix
    input vectors where specified.
-   Safe handling of over-length console input.
-   Validation of records loaded from CSV files.
-   Non-destructive file-write handling.
-   Retention of cancelled appointments for auditability.
-   Minimization of patient information in appointment views.
-   Sanitized filesystem error messages.
-   Restrictions on data-file access to the designated data directory.
-   Offline operation without application-level remote service
    dependencies.

HMS does not implement application login, role-based permissions, or
encryption of CSV contents. Workstation and filesystem access controls
remain operating-system responsibilities under the approved project
scope.

## Testing and Verification

Automated tests are registered with CTest and cover patient, doctor,
appointment, persistence, console, non-functional, and security-related
behavior.

### Verified automated test execution

The project test plan records a Windows 11 / MinGW-w64 GCC 15.1.0
verification run with:

  Measure                  Result
  ---------------------- --------
  CTest targets passed         51
  CTest targets failed          0
  Total CTest targets          51
  Pass rate                  100%

A separate local LLVM/Clang instrumentation run also completed **51/51
CTest tests successfully**. Its production coverage results are reported
below. These are separate toolchain runs and their timings should not be
compared as if they were the same execution.

### Run the suite

``` powershell
ctest --test-dir build --output-on-failure
```

To list registered tests:

``` powershell
ctest --test-dir build -N
```

## Coverage Results

The following figures are from the **production-only LLVM/Clang coverage
report** generated from the application executable. Test-source files
were excluded from the production coverage calculation.

  Coverage metric       Result
  ------------------- --------
  Region coverage       90.20%
  Function coverage     97.92%
  Line coverage         86.67%
  Branch coverage       81.98%

Coverage describes which instrumented code regions were exercised; it
does not by itself prove that every requirement or manual
system-validation scenario has passed.

The locally retained LLVM evidence includes the merged profile
(`coverage.profdata`), raw profiles, CTest results, and production
coverage text/JSON output under `build-llvm/`. These are generated local
artifacts and should not normally be committed to the source repository.

## Project Documentation

The project documentation is maintained alongside the source repository.
The principal documents include:

  -----------------------------------------------------------------------
  Document                            Purpose
  ----------------------------------- -----------------------------------
  `HMS_SRS_Final.pdf`                 Software Requirements
                                      Specification: functional,
                                      non-functional, and security
                                      requirements.

  `SAD.pdf`                           System Architecture Document:
                                      structural and design decisions.

  `Software_Test_Plan_Final.pdf`      Master test plan, test approach,
                                      environment, traceability, and
                                      quality gates.

  Updated Test Case Document          Detailed test specifications for
                                      baseline and supplementary
                                      requirements.

  Requirements Traceability Matrix    Mapping between requirements, test
                                      cases, automated targets, and
                                      implementation.

  Test reports                        Unit test, system validation, code
                                      coverage, and branch coverage
                                      evidence.
  -----------------------------------------------------------------------

Refer to the repository's `docs/` and `DOCS/` documentation areas for
the files available in the current checkout. Report status should be
read from the corresponding report itself; planned or pending activities
are not represented as completed.

## Development Workflow

The project uses Git and GitHub for source control, issue tracking,
pull-request review, and sprint coordination.

A typical contribution workflow is:

1.  Review the relevant issue and acceptance criteria.
2.  Synchronize the local `main` branch with the remote repository.
3.  Create a focused feature or fix branch.
4.  Implement the change within the appropriate module.
5.  Build the project and run relevant tests.
6.  Run the full CTest regression suite when appropriate.
7.  Review the diff and update affected documentation.
8.  Commit with a meaningful message and submit the change for review.

Generated build output, profiling data, temporary test data, and real
patient data should not be included in source commits.

## Project Team

The following role assignments are recorded in the final Software Test
Plan (STP v2.0):

  Team member          SRN             Project role
  -------------------- --------------- ----------------
  Hrushikesh Karnam    PES1UG24CS668   QA Lead
  Mandagandla Jhansi   PES1UG24CS672   Developer Lead
  Muruli H             PES1UG24CS676   Test Engineer
  Neeta R              PES1UG24CS680   Support Lead

## Known Verification Boundaries

For accurate interpretation of the results:

-   The verified local automated test evidence is from Windows.
-   Ubuntu Linux 22.04 is specified as a target environment but is
    recorded as **not executed locally** in the final test plan.
-   Formal clinical user acceptance testing (UAT) and evaluator sign-off
    remain pending in the test plan.
-   Manual system and security validation scenarios must be reported
    only when their execution evidence is available.
-   LLVM coverage figures in this README are production-only figures;
    they exclude test-source coverage.
-   The system is a single-user local console application and should not
    be described as a networked or multi-user hospital platform.

------------------------------------------------------------------------

## Repository
**DRIVE-LINK** : [FINAL DEMO VIDEO](https://drive.google.com/file/d/13Q18OQGE-_3P8JkjGQkF-jiwejIb11ye/view?usp=drive_link)

**GitHub:**
[HOSPITAL-MANAGEMENT-SYSTEM](https://github.com/HOSPITALMANAGEMENTSYSTEM4/Hospital-Management-System)

For implementation details, requirements, design rationale, and test
evidence, consult the project documents included in the repository.
