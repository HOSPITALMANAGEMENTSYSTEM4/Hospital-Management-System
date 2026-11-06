# Source Directory (`src`)

## 1. Overview

The `src` directory contains the C++ implementation files of the Hospital Management System (HMS). It implements the application entry point, core entity operations, hospital-management workflows, and CSV-based data persistence.

The implementation follows a modular structure in which each source file provides the definitions for the corresponding declarations in the `include/hms/` directory.

The project uses **C++17** and is built using the root-level `CMakeLists.txt`.

## 2. Source Files

| File | Responsibility |
|---|---|
| `main.cpp` | Application entry point. Creates the `hms::HospitalSystem` object, displays startup records, and starts the main application loop. |
| `Patient.cpp` | Implements the `Patient` class, including patient-field validation, security checks, and accessors for patient information. |
| `Doctor.cpp` | Implements the `Doctor` class, including doctor-field validation, security checks, and accessors for doctor information. |
| `Appointment.cpp` | Implements appointment details, appointment-status conversion, date and time validation, conflict checking, cancellation, and CSV serialization/deserialization. |
| `HospitalSystem.cpp` | Implements the main application workflows, including patient and doctor management, appointment booking, viewing, filtering, availability checking, cancellation, and menu navigation. |
| `FileManager.cpp` | Implements CSV-based persistence for patient, doctor, and appointment records. Handles loading, saving, updating, record validation, and data-file access restrictions. |

## 3. Responsibilities of the Main Components

### 3.1 Application Entry Point

**File:** `main.cpp`

The application starts by creating an instance of `hms::HospitalSystem`.

The entry point performs two main operations:

1. Calls `displayStartupRecords()` to display the records loaded at startup.
2. Calls `run()` to enter the main menu-driven application.

The `main()` function returns `0` after the application exits normally.

### 3.2 Patient Management

**File:** `Patient.cpp`

This file contains the implementation of the `Patient` class.

Its responsibilities include:

- Maintaining patient information such as ID, name, age, gender, contact number, address, and medical history.
- Validating patient identifiers and individual fields.
- Applying the defined input-security checks.
- Exposing patient information through getter functions.
- Ensuring that invalid patient data is rejected before persistence.

### 3.3 Doctor Management

**File:** `Doctor.cpp`

This file contains the implementation of the `Doctor` class.

Its responsibilities include:

- Maintaining doctor ID, name, specialization, department, contact number, and availability.
- Validating doctor information.
- Applying the defined input-security checks.
- Providing getter functions for doctor information.
- Ensuring that invalid doctor data is rejected before persistence.

### 3.4 Appointment Management

**File:** `Appointment.cpp`

This file contains the implementation of the `Appointment` class and its status-related operations.

Its responsibilities include:

- Maintaining appointment ID, patient ID, doctor ID, date, time, and status.
- Validating appointment identifiers, dates, times, and status values.
- Representing appointment states as `Booked` or `Cancelled`.
- Checking whether an appointment conflicts with a requested doctor, date, and time.
- Supporting appointment cancellation.
- Converting appointment records to and from CSV format.

### 3.5 Hospital Management Workflows

**File:** `HospitalSystem.cpp`

This file implements the `HospitalSystem` class, which coordinates the main application workflows.

Its responsibilities include:

- Managing the main menu and module submenus.
- Registering, searching, viewing, and updating patient records.
- Adding, searching, viewing, listing, and updating doctor records.
- Booking appointments for registered patients and doctors.
- Checking doctor availability.
- Listing and filtering appointments by patient or doctor.
- Cancelling appointments with confirmation.
- Generating sequential appointment IDs.
- Coordinating with `FileManager` for record retrieval and persistence.
- Returning operation results and appropriate messages.

### 3.6 Data Persistence

**File:** `FileManager.cpp`

This file implements the `FileManager` class, which isolates CSV file operations from the hospital-management workflows.

Its responsibilities include:

- Loading patient, doctor, and appointment records.
- Saving new records and collections of records.
- Updating existing patient and doctor records.
- Checking whether patient, doctor, and appointment IDs already exist.
- Checking duplicate identities using normalized values.
- Handling malformed records during CSV loading.
- Validating records before saving.
- Applying safe data-path restrictions.
- Handling file-operation failures while protecting existing valid data.

## 4. Relationship with Header Files

Each implementation file corresponds to a public header declaration under `include/hms/`.

| Implementation File | Corresponding Header |
|---|---|
| `Patient.cpp` | `hms/Patient.h` |
| `Doctor.cpp` | `hms/Doctor.h` |
| `Appointment.cpp` | `hms/Appointment.h` |
| `HospitalSystem.cpp` | `hms/HospitalSystem.h` |
| `FileManager.cpp` | `hms/FileManager.h` |
| `main.cpp` | Uses `hms/HospitalSystem.h` |

The header files declare classes, functions, and interfaces, while the source files contain their implementations.

## 5. Build Integration

The source files are included in the main executable through the root `CMakeLists.txt`.

The executable target is named `hms`.

The project uses:

- CMake 3.20 or later.
- C++17.
- The `include` directory as the header search path.

From the repository root, the application can be configured and built using:

```bash
cmake -S . -B build
cmake --build build
```

The generated executable is located in the build output directory.

## 6. Design Considerations

The source directory follows these implementation principles:

- **Modularity:** Each class has a defined responsibility.
- **Separation of concerns:** Business workflows are coordinated by `HospitalSystem`, while CSV operations are handled by `FileManager`.
- **Validation:** Data is validated before it is accepted or persisted.
- **Data integrity:** Record updates and appointment operations must follow the defined business rules.
- **Maintainability:** Public declarations are separated from implementation details.
- **Testability:** The source files are reused by the test targets configured in `CMakeLists.txt`.

## 7. Related Directories

- `include/hms/` — public class declarations and interfaces.
- `tests/` — test programs and verification scripts.
- `DOCS/` — project specifications, architecture, test documentation, and deliverables.
- `CMakeLists.txt` — executable and test build configuration.

