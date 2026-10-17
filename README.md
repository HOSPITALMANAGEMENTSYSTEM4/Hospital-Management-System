# Hospital Management System (HMS)

## 1. Project Overview

The Hospital Management System (HMS) is a C++17-based, console-driven application developed to manage essential hospital information, including patient records, doctor information, and appointments. The system aims to provide reliable record management through structured operations, input validation, business-rule enforcement, and local data persistence.

The application is designed as a standalone, single-user system that stores data locally using CSV files. Its implementation follows the requirements and design defined in the Software Requirements Specification (SRS), Software Architecture Document (SAD), and Software Test Plan.

## 2. Project Objectives

The primary objectives of the Hospital Management System are to:

- Maintain accurate and consistent patient records.
- Manage doctor information and availability.
- Facilitate appointment booking, viewing, availability checking, and cancellation.
- Prevent duplicate patient and doctor identifiers.
- Prevent conflicting appointments for the same doctor and time slot.
- Validate user input before modifying records.
- Preserve confirmed data across application restarts.
- Handle invalid input and file-operation failures safely.
- Provide a structured, menu-driven console interface.

## 3. Project Scope

### 3.1 In Scope

The project includes:

- Patient registration, searching, viewing, and updating.
- Doctor registration, searching, viewing, and updating.
- Appointment booking, viewing, availability checking, and cancellation.
- Input validation and enforcement of defined business rules.
- CSV-based local data storage and retrieval.
- Handling missing, empty, malformed, or unwritable data files.
- Error handling and data-integrity protection.
- Functional, non-functional, and security-related verification.

### 3.2 Out of Scope

The project is designed as a standalone console application. The following are outside its defined scope:

- Web-based or mobile application interfaces.
- Multi-user or network-based operation.
- Database-server integration.
- Online appointment booking and external service integrations.

## 4. System Modules

### 4.1 Patient Management

The Patient Management module maintains patient information.

Its responsibilities include:

- Registering patients using the required details.
- Preventing duplicate patient IDs.
- Searching patients by ID or name.
- Displaying individual patient records and patient lists.
- Updating permitted patient information.
- Applying the specified validation rules before storing changes.

### 4.2 Doctor Management

The Doctor Management module maintains doctor information and availability.

Its responsibilities include:

- Adding doctor details, specialization, department, contact number, and availability information.
- Preventing duplicate doctor IDs.
- Searching doctors by ID, name, or specialization.
- Displaying individual doctor records and doctor lists.
- Updating permitted doctor information with appropriate validation.

### 4.3 Appointment Management

The Appointment Management module manages appointments between registered patients and doctors.

Its responsibilities include:

- Booking appointments for registered patients and doctors.
- Generating unique appointment identifiers.
- Validating appointment dates and times.
- Checking doctor availability.
- Preventing conflicting bookings for the same doctor and time slot.
- Viewing appointment records.
- Cancelling appointments while retaining the record with a `Cancelled` status.

### 4.4 Data Persistence

The Data Persistence component manages local storage and retrieval of application records.

The expected CSV data files are:

- `patients.csv` — patient records.
- `doctors.csv` — doctor records.
- `appointments.csv` — appointment records.

The system must preserve confirmed records across application restarts and handle file-loading and file-writing failures without corrupting previously valid data.

### 4.5 Console Interface and Input Validation

The console interface provides menu-driven access to the system's operations.

Its responsibilities include:

- Displaying the main menu and module submenus.
- Accepting and validating user input.
- Displaying appropriate success confirmations and error messages.
- Returning users to the relevant menu after operations.
- Handling invalid menu selections without unexpected termination.

## 5. Technology Stack

| Component | Technology |
|---|---|
| Programming language | C++ |
| Language standard | C++17 |
| User interface | Console-based |
| Data storage | Local CSV files |
| Compiler/toolchain | GCC 9+ or MinGW-w64 |
| Version control | Git |
| Collaboration and project tracking | GitHub |
| Testing reference | Software Test Plan |

The implementation is intended to use the C++ standard library and support compilation and execution on Windows 10 or later and Linux, as specified by the project requirements.

## 6. System Architecture

The system follows a modular design that separates data representation, business logic, file operations, and user interaction.

The core classes specified by the SRS are:

- **Patient:** Represents patient information.
- **Doctor:** Represents doctor information and availability.
- **Appointment:** Represents appointment details and status.
- **HospitalSystem:** Coordinates application operations and enforces business rules.
- **FileManager:** Handles CSV file input/output and data persistence.

The console interface manages menus and user interaction. Validation must be performed before invalid data can modify records. File operations are isolated within `FileManager` to support maintainability and consistency.

## 7. Data Integrity and Security

The system is designed to enforce the following rules:

1. Patient IDs and doctor IDs must be unique.
2. Appointments must reference registered patients and doctors.
3. Appointment dates and times must satisfy the defined validation rules.
4. A doctor cannot have two non-cancelled appointments in the same time slot.
5. Cancelled appointments must remain stored with their status updated to `Cancelled`.
6. Invalid input must not partially modify in-memory or stored records.
7. Confirmed records must remain available after the application restarts.
8. File-writing failures must not discard previously valid data.
9. Appointment views must not expose patient contact numbers, addresses, or medical-history notes.
10. The application must not transmit hospital records outside the local machine.
11. Data files must be accessed only through the designated data directory.

These rules support the integrity, availability, and confidentiality objectives defined in the SRS.

## 8. Project Documentation

The `DOCS` directory contains the principal project documents:

- **`DOCS/HMS_SRS_Final.pdf`** — defines functional, non-functional, and security requirements, business rules, and acceptance criteria.
- **`DOCS/SAD.pdf`** — documents the system architecture and design.
- **`DOCS/Software_Test_Plan.pdf`** — defines the testing approach and associated test cases.

These documents serve as the reference for implementation, testing, and verification of project deliverables.

## 9. Version Control and Project Management

Git and GitHub are used to maintain source code, documentation, and project history.

The team uses GitHub Issues to track requirements and development tasks, and the GitHub Project board to monitor task status and sprint assignments.

The development workflow includes:

1. Reviewing the relevant issue and acceptance criteria.
2. Implementing the assigned functionality.
3. Committing changes with meaningful commit messages.
4. Reviewing and integrating changes into the main branch.
5. Testing implemented functionality against the applicable test cases.
6. Updating documentation to reflect the verified project state.

Issue numbers identify tasks, while labels and iteration assignments are used to classify and schedule work.

## 10. Expected Deliverables

The expected project deliverables are:

- A C++17-based Hospital Management System.
- Patient and doctor record-management functionality.
- Appointment management with availability and conflict checks.
- CSV-based data persistence.
- Input validation and error handling.
- Data-integrity and security controls defined by the SRS.
- Test results demonstrating compliance with applicable requirements.
- Project documentation and a final demonstration.

A deliverable should be considered complete only after its acceptance criteria and applicable tests have been verified.

## 11. Sprint 1 — Plan of Action

**Planned Sprint Dates:** 19–23 October 2026

Sprint 1 focuses on establishing the implementation foundation and developing the initial system functionality in accordance with the approved SRS, architecture, test plan, and assigned GitHub issues.

### 11.1 Project Setup and Design Review

- Review the SRS, SAD, and Software Test Plan.
- Establish the C++17 project structure and build configuration.
- Confirm source-code organization and team responsibilities.
- Review repository conventions and the initial project documentation.

### 11.2 Core Classes and Validation

- Define the `Patient`, `Doctor`, and `Appointment` classes.
- Establish the responsibilities of `HospitalSystem` and `FileManager`.
- Implement the relevant input-validation rules.
- Enforce required fields and unique identifiers.

### 11.3 CSV Data Persistence

- Implement CSV loading and saving for the required data files.
- Handle missing, empty, and malformed data files.
- Preserve previously valid data when file-writing operations fail.
- Verify persistence across application restarts.

### 11.4 Core Workflow Implementation and Integration

- Implement the assigned patient-management operations.
- Implement the assigned doctor-management operations.
- Develop appointment-management operations as dependencies permit.
- Integrate the console menus and appropriate success/error messages.
- Verify duplicate-ID prevention and appointment conflict handling.

### 11.5 Testing and Review

- Execute applicable functional test cases.
- Test invalid input and boundary conditions.
- Verify CSV persistence and appointment cancellation behavior.
- Address identified defects within the sprint scope.
- Update documentation and record the results of verification.

### 11.6 Sprint 1 Completion Criteria

Sprint 1 deliverables will be evaluated against the team's agreed sprint scope and acceptance criteria. Completion requires:

- The assigned functionality satisfies its defined requirements.
- The project builds using the required C++17 toolchain.
- Applicable tests have been executed and results recorded.
- Critical defects affecting the agreed sprint scope have been addressed or documented.
- The repository and documentation accurately reflect the verified implementation status.