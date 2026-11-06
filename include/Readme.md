# Include Directory (`include`)

## 1. Overview

The `include` directory contains the header files of the Hospital Management System (HMS).

These headers define the public interfaces, classes, structures, enumerations, and function declarations used throughout the application.

The headers are organized under the `hms/` namespace directory and are included by the implementation files in `src/`.

The project uses **C++17**.

## 2. Header Directory Structure

```text
include/
├── README.md
└── hms/
    ├── Patient.h
    ├── Doctor.h
    ├── Appointment.h
    ├── HospitalSystem.h
    └── FileManager.h
```

## 3. Header Files

| Header File | Responsibility |
|---|---|
| `hms/Patient.h` | Declares the `Patient` class, patient information fields, validation functions, security checks, and getter functions. |
| `hms/Doctor.h` | Declares the `Doctor` class, doctor information fields, validation functions, security checks, and getter functions. |
| `hms/Appointment.h` | Declares the `Appointment` class, appointment status enumeration, validation functions, conflict checking, cancellation, and CSV conversion interfaces. |
| `hms/HospitalSystem.h` | Declares the `HospitalSystem` class and `OpResult` structure. Provides interfaces for application workflows, menus, record operations, and appointment management. |
| `hms/FileManager.h` | Declares the `FileManager` class and its CSV loading, saving, updating, duplicate-checking, and data-path validation interfaces. |

## 4. Responsibilities of the Header Files

### 4.1 Patient Header

**File:** `hms/Patient.h`

Declares the `Patient` class, which represents an individual patient in the system.

The class maintains:

- Patient ID.
- Name.
- Age.
- Gender.
- Contact number.
- Address.
- Medical history.

It exposes interfaces for:

- Checking whether a patient record is valid.
- Validating individual patient fields.
- Applying security checks to input values.
- Retrieving patient information through getter functions.

The actual function definitions are provided in `src/Patient.cpp`.

### 4.2 Doctor Header

**File:** `hms/Doctor.h`

Declares the `Doctor` class, which represents a doctor registered in the hospital system.

The class maintains:

- Doctor ID.
- Name.
- Specialization.
- Department.
- Contact number.
- Availability.

It exposes interfaces for:

- Checking whether a doctor record is valid.
- Validating doctor information.
- Applying security checks to input values.
- Retrieving doctor information through getter functions.

The actual function definitions are provided in `src/Doctor.cpp`.

### 4.3 Appointment Header

**File:** `hms/Appointment.h`

Declares the `Appointment` class and the appointment-status enumeration.

The appointment status is represented by:

```cpp
enum class AppointmentStatus {
    Booked,
    Cancelled
};
```

The header provides interfaces for:

- Maintaining appointment ID and associated patient and doctor IDs.
- Storing appointment date and time.
- Validating appointment fields.
- Checking appointment conflicts.
- Cancelling appointments.
- Checking whether an appointment is booked or cancelled.
- Converting appointment status to and from its string representation.
- Converting appointment records to and from CSV format.

The actual function definitions are provided in `src/Appointment.cpp`.

### 4.4 Hospital System Header

**File:** `hms/HospitalSystem.h`

Declares the `HospitalSystem` class, which coordinates the application's main operations.

It also declares the `OpResult` structure:

```cpp
struct OpResult {
    bool success{false};
    std::string message;
};
```

The class exposes interfaces for:

- Starting and running the application.
- Displaying records at startup.
- Registering and updating patients.
- Adding, listing, viewing, and updating doctors.
- Searching for patients and doctors.
- Booking appointments.
- Checking doctor availability.
- Viewing and filtering appointments.
- Cancelling appointments.
- Finding records by their identifiers.
- Generating the next appointment ID.

The class coordinates the entity classes and `FileManager` to perform the system's business workflows.

The actual function definitions are provided in `src/HospitalSystem.cpp`.

### 4.5 File Manager Header

**File:** `hms/FileManager.h`

Declares the `FileManager` class, which provides the interface for local CSV data persistence.

Its interfaces cover:

- Loading patient records.
- Loading doctor records.
- Loading appointment records.
- Saving individual records and record collections.
- Updating patient and doctor records.
- Checking the existence of patient, doctor, and appointment IDs.
- Checking duplicate patient and doctor identities.
- Normalizing strings for identity comparison.
- Retrieving configured CSV file paths.
- Checking whether configured data paths are allowed.

The class uses the entity declarations from `Patient.h`, `Doctor.h`, and `Appointment.h`.

The actual function definitions are provided in `src/FileManager.cpp`.

## 5. Header Dependencies

The main header relationships are:

```text
HospitalSystem.h
    ├── Appointment.h
    ├── Doctor.h
    ├── FileManager.h
    └── Patient.h

FileManager.h
    ├── Appointment.h
    ├── Doctor.h
    └── Patient.h

Patient.h
    └── Standard Library: <string>

Doctor.h
    └── Standard Library: <string>

Appointment.h
    └── Standard Library: <string>
```

The headers use the `hms` namespace to organize the application's classes and avoid unnecessary global namespace declarations.

## 6. Including Headers in Source Files

The project uses the `include` directory as its header search path.

Source files include headers using the `hms/` prefix:

```cpp
#include "hms/Patient.h"
#include "hms/Doctor.h"
#include "hms/Appointment.h"
#include "hms/HospitalSystem.h"
#include "hms/FileManager.h"
```

For example, the application entry point includes:

```cpp
#include "hms/HospitalSystem.h"
```

This include style is consistent with the directory structure and the include path configured in the root `CMakeLists.txt`.

## 7. Design Considerations

The header directory follows these principles:

- **Interface separation:** Class declarations are maintained separately from implementation files.
- **Encapsulation:** Internal data members are declared within their respective classes.
- **Namespace organization:** Application components are grouped under the `hms` namespace.
- **Reusability:** Header declarations can be included by the application and relevant test targets.
- **Maintainability:** Each header represents a specific component or responsibility.
- **Consistency:** Header paths and naming conventions match the implementation files.

## 8. Related Directories

- `src/` — contains the implementation of the declared classes and application entry point.
- `tests/` — contains test programs that use the public interfaces.
- `DOCS/` — contains project requirements, architecture, test documentation, and deliverables.
- `CMakeLists.txt` — configures the header search path and build targets.

