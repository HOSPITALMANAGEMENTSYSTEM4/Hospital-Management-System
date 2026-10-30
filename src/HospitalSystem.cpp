
#include "hms/HospitalSystem.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hms {
namespace {

std::string trim(const std::string &value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t first = value.find_first_not_of(whitespace);

  if (first == std::string::npos) {
    return "";
  }

  const std::size_t last = value.find_last_not_of(whitespace);
  return value.substr(first, last - first + 1);
}

std::string toUpper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return value;
}

// VR-03: age 0-120 inclusive.
bool parseAge(const std::string &input, int &age) {
  std::string unusedErr;
  return Patient::validateAge(input, age, unusedErr);
}

bool readRequiredField(const std::string &prompt, std::string &value) {
  std::cout << prompt;

  if (!std::getline(std::cin, value)) {
    return false;
  }

  value = trim(value);

  if (value.empty()) {
    std::cout << "This field is required.\n";
    return false;
  }

  return true;
}
template <typename Validator>
bool readValidatedField(const std::string &prompt,
                        std::string &value,
                        Validator validator) {
  while (true) {
    std::cout << prompt;

    if (!std::getline(std::cin, value)) {
      return false;
    }

    value = trim(value);

    std::string validationError;

    if (validator(value, validationError)) {
      return true;
    }

    std::cout << validationError << '\n';
  }
}
void displayPatientDetails(const Patient &patient) {
  std::cout << "\n--- Patient Details ---\n"
            << "Patient ID: " << patient.getId() << '\n'
            << "Name: " << patient.getName() << '\n'
            << "Age: " << patient.getAge() << '\n'
            << "Gender: " << patient.getGender() << '\n'
            << "Contact number: " << patient.getContactNumber() << '\n'
            << "Address: " << patient.getAddress() << '\n'
            << "Medical history: " << patient.getMedicalHistory() << '\n';
}

void displayDoctorDetails(const Doctor &doctor) {
  std::cout << "\n--- Doctor Details ---\n"
            << "Doctor ID: " << doctor.getId() << '\n'
            << "Name: " << doctor.getName() << '\n'
            << "Specialization: " << doctor.getSpecialization() << '\n'
            << "Department: " << doctor.getDepartment() << '\n'
            << "Contact number: " << doctor.getContactNumber() << '\n'
            << "Availability: " << doctor.getAvailability() << '\n';
}

} // namespace

HospitalSystem::HospitalSystem(FileManager fileManager)
    : fileManager_(std::move(fileManager)) {}

HospitalSystem::HospitalSystem(const std::string &dataDir) {
  std::string prefix = dataDir;
  if (!prefix.empty() && prefix.back() != '/' && prefix.back() != '\\') {
    prefix += '/';
  }
  fileManager_ = FileManager(prefix + "patients.csv",
                             prefix + "doctors.csv",
                             prefix + "appointments.csv");
}

const FileManager &HospitalSystem::getFileManager() const {
  return fileManager_;
}

FileManager &HospitalSystem::getFileManager() {
  return fileManager_;
}

void HospitalSystem::displayStartupRecords() {
  // The listing methods use FileManager's CSV loaders and handle empty or
  // missing files without creating or overwriting data files.
  std::cout << "\n=== Records Loaded at Startup ===\n";
  listPatients();
  listDoctors();
  viewAppointments();
}

void HospitalSystem::run() {
  while (true) {
    std::cout << "\n=== Hospital Management System ===\n"
              << "1. Patient Management\n"
              << "2. Doctor Management\n"
              << "3. Appointment Management\n"
              << "4. Exit\n"
              << "Enter your choice: ";

    std::string choice;

    if (!std::getline(std::cin, choice)) {
      std::cout << "\nExiting Hospital Management System.\n";
      return;
    }

    choice = trim(choice);

    if (choice == "4") {
      std::cout << "Exiting Hospital Management System.\n";
      return;
    } else if (choice == "1") {
      patientMenu();
    } else if (choice == "2") {
      doctorMenu();
    } else if (choice == "3") {
      appointmentMenu();
    } else {
      // HMS-F-033: Reject invalid menu options (non-numeric, out-of-range, empty)
      std::cout << "Invalid choice. Enter 1, 2, 3, or 4.\n";
    }
  }
}

// HMS-F-032, SRS Â§3.1 - Patient submenu
void HospitalSystem::patientMenu() {
  while (true) {
    std::cout << "\n=== Patient Management ===\n"
              << "1. Register\n"
              << "2. Search\n"
              << "3. View All\n"
              << "4. Update\n"
              << "0. Return\n"
              << "Enter your choice: ";

    std::string choice;

    if (!std::getline(std::cin, choice)) {
      return;
    }

    choice = trim(choice);

    if (choice == "0") {
      return;
    } else if (choice == "1") {
      registerPatient();
    } else if (choice == "2") {
      searchPatients();
    } else if (choice == "3") {
      listPatients();
    } else if (choice == "4") {
      updatePatient();
    } else {
      // HMS-F-033: Reject invalid menu options (non-numeric, out-of-range, empty)
      std::cout << "Invalid choice. Enter 0 to 4.\n";
    }
  }
}

// HMS-F-032, SRS Â§3.1 - Doctor submenu
void HospitalSystem::doctorMenu() {
  while (true) {
    std::cout << "\n=== Doctor Management ===\n"
              << "1. Add\n"
              << "2. Search\n"
              << "3. View All\n"
              << "4. Update\n"
              << "0. Return\n"
              << "Enter your choice: ";

    std::string choice;

    if (!std::getline(std::cin, choice)) {
      return;
    }

    choice = trim(choice);

    if (choice == "0") {
      return;
    } else if (choice == "1") {
      addDoctor();
    } else if (choice == "2") {
      searchDoctors();
    } else if (choice == "3") {
      listDoctors();
    } else if (choice == "4") {
      updateDoctor();
    } else {
      // HMS-F-033: Reject invalid menu options (non-numeric, out-of-range, empty)
      std::cout << "Invalid choice. Enter 0 to 4.\n";
    }
  }
}

// HMS-F-032, SRS Â§3.1 - Appointment submenu
void HospitalSystem::appointmentMenu() {
  while (true) {
    std::cout << "\n=== Appointment Management ===\n"
              << "1. Book\n"
              << "2. View\n"
              << "3. Cancel\n"
              << "4. Check Doctor Availability\n"
              << "0. Return\n"
              << "Enter your choice: ";

    std::string choice;

    if (!std::getline(std::cin, choice)) {
      return;
    }

    choice = trim(choice);

    if (choice == "0") {
      return;
    } else if (choice == "1") {
      bookAppointment();
    } else if (choice == "2") {
      viewAppointmentsInteractive();
    } else if (choice == "3") {
      cancelAppointmentInteractive();
    } else if (choice == "4") {
      checkDoctorAvailabilityInteractive();
    } else {
      // HMS-F-033: Reject invalid menu options (non-numeric, out-of-range, empty)
      std::cout << "Invalid choice. Enter 0 to 4.\n";
    }
  }
}

void HospitalSystem::registerPatient() {
  std::cout << "\n--- Register Patient ---\n";

  std::string id;
  std::string name;
  std::string ageInput;
  std::string gender;
  std::string contactNumber;
  std::string address;
  std::string medicalHistory;

  // Patient ID: validate format and check duplicates before continuing.
  while (true) {
    if (!readValidatedField(
            "Patient ID (1-10 alphanumeric characters, e.g., P001): ",
            id,
            [](const std::string &value, std::string &error) {
              return Patient::validateId(value, error);
            })) {
      return;
    }

    if (!fileManager_.patientIdExists(toUpper(id))) {
      break;
    }

    std::cout << "Patient ID already exists\n";
  }

  // Full name.
  if (!readValidatedField(
          "Full name (1-50 characters): ",
          name,
          [](const std::string &value, std::string &error) {
            return Patient::validateName(value, error);
          })) {
    return;
  }

  // Age.
  int age = 0;

  if (!readValidatedField(
          "Age (0-120): ",
          ageInput,
          [&age](const std::string &value, std::string &error) {
            return Patient::validateAge(value, age, error);
          })) {
    return;
  }

  // Gender.
  if (!readValidatedField(
          "Gender (M/F/O): ",
          gender,
          [](const std::string &value, std::string &error) {
            return Patient::validateGender(value, error);
          })) {
    return;
  }

  // Contact number.
  if (!readValidatedField(
          "Contact number (10 digits): ",
          contactNumber,
          [](const std::string &value, std::string &error) {
            return Patient::validateContactNumber(value, error);
          })) {
    return;
  }

  // Address.
  if (!readValidatedField(
          "Address (1-100 characters): ",
          address,
          [](const std::string &value, std::string &error) {
            return Patient::validateAddress(value, error);
          })) {
    return;
  }

  // Medical history is optional, but must not exceed 200 characters.
  if (!readValidatedField(
          "Medical history (optional, up to 200 characters; press Enter to skip): ",
          medicalHistory,
          [](const std::string &value, std::string &error) {
            return Patient::validateMedicalHistory(value, error);
          })) {
    return;
  }

  // The constructor normalizes the ID and gender to uppercase.
  const Patient patient(id, name, age, gender, contactNumber, address,
                        medicalHistory);

  // Final validation remains as a safeguard before persistence.
  std::string validationError;

  if (!patient.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

  // Check for duplicates again before saving.
  if (fileManager_.patientIdExists(patient.getId())) {
    std::cout << "Patient ID already exists\n";
    return;
  }

  std::string errorMessage;

  std::string existingPatientId;
  if (fileManager_.patientIdentityExists(patient.getName(),
                                         patient.getContactNumber(),
                                         &existingPatientId)) {
    std::cout << "\nWarning: Potential duplicate patient record found with Patient ID: "
              << existingPatientId << '\n';
    std::cout << "Proceed with registration? (Y/N): ";
    std::string response;
    if (!std::getline(std::cin, response)) {
      return;
    }
    response = toUpper(trim(response));
    if (response != "Y") {
      std::cout << "Registration cancelled.\n";
      return;
    }
  }

  if (!fileManager_.savePatient(patient, errorMessage)) {
    std::cout << "Registration failed: " << errorMessage << '\n';
    return;
  }

  std::cout << "\nPatient registered successfully.\n"
            << "Patient ID: " << patient.getId() << '\n';
}

void HospitalSystem::searchPatients() {
  std::cout << "\n--- Search Patients ---\n";

  std::string query;

  if (!readRequiredField("Enter patient ID or name (e.g., P001 or John): ", query)) {
    return;
  }

  const std::vector<Patient> patients = fileManager_.loadPatients();
  const std::string normalizedQuery = toLower(query);

  // An exact patient ID match takes priority over name matches.
  for (const Patient &patient : patients) {
    if (toLower(patient.getId()) == normalizedQuery) {
  displayPatientDetails(patient);
  std::cout << "Patient search completed successfully.\n";
  return;
}
  }

  // Otherwise, display every patient whose name contains the query.
  bool found = false;

  for (const Patient &patient : patients) {
    if (toLower(patient.getName()).find(normalizedQuery) !=
        std::string::npos) {
      displayPatientDetails(patient);
std::cout << "Patient search completed successfully.\n";
      found = true;
    }
  }

  if (!found) {
    std::cout << "No matching patient found\n";
  }
}

void HospitalSystem::viewPatientDetails() {
  std::cout << "\n--- View Patient Details ---\n";

  std::string patientId;

  if (!readRequiredField("Enter patient ID: (1-10 alphanumeric characters, e.g., P001): ", patientId)) {
    return;
  }

  const std::vector<Patient> patients = fileManager_.loadPatients();
  const std::string searchId = toUpper(patientId);

  for (const Patient &patient : patients) {
    if (toUpper(patient.getId()) == searchId) {
      displayPatientDetails(patient);
      return;
    }
  }

  std::cout << "No matching patient found\n";
}


void HospitalSystem::listPatients() {
  std::cout << "\n--- Patient List ---\n";

  std::size_t malformedCount = 0;
  const std::vector<Patient> patients =
      fileManager_.loadPatients(&malformedCount);

  if (malformedCount > 0) {
    std::cout << malformedCount << " malformed record(s) skipped\n";
  }

  if (patients.empty()) {
    std::cout << "No patients registered\n";
    return;
  }

  std::cout << "Patient ID | Name | Age | Gender | Contact number\n"
            << "---------------------------------------------------\n";

  for (const Patient &patient : patients) {
    std::cout << patient.getId() << " | "
              << patient.getName() << " | "
              << patient.getAge() << " | "
              << patient.getGender() << " | "
              << patient.getContactNumber() << '\n';
  }
}

void HospitalSystem::updatePatient() {
  std::cout << "\n--- Update Patient ---\n";

  std::string id;
  std::vector<Patient> patients;
  const Patient *targetPatient = nullptr;

  // Validate Patient ID and retry if it is invalid or not found.
  while (true) {
    if (!readValidatedField(
            "Enter patient ID (1-10 alphanumeric characters, e.g., P001): ",
            id,
            [](const std::string &value, std::string &error) {
              return Patient::validateId(value, error);
            })) {
      return;
    }

    const std::string searchId = toUpper(trim(id));
    patients = fileManager_.loadPatients();
    targetPatient = nullptr;

    for (const Patient &patient : patients) {
      if (toUpper(patient.getId()) == searchId) {
        targetPatient = &patient;
        break;
      }
    }

    if (targetPatient != nullptr) {
      break;
    }

    std::cout << "Patient not found\n";
  }

  displayPatientDetails(*targetPatient);

  std::cout << "\nSelect field to update:\n"
            << "1. Name\n"
            << "2. Age\n"
            << "3. Gender\n"
            << "4. Contact number\n"
            << "5. Address\n"
            << "6. Medical history\n"
            << "7. All fields\n"
            << "0. Return\n"
            << "Enter choice: ";

  std::string choice;
  if (!std::getline(std::cin, choice)) {
    return;
  }

  choice = trim(choice);

  if (choice == "0") {
    return;
  }

  // Preserve existing values for fields not selected for updating.
  std::string newName = targetPatient->getName();
  int newAge = targetPatient->getAge();
  std::string newGender = targetPatient->getGender();
  std::string newContact = targetPatient->getContactNumber();
  std::string newAddress = targetPatient->getAddress();
  std::string newMed = targetPatient->getMedicalHistory();

  if (choice == "1") {
    if (!readValidatedField(
            "Enter new name (1-50 characters): ",
            newName,
            [](const std::string &value, std::string &error) {
              return Patient::validateName(value, error);
            })) {
      return;
    }

  } else if (choice == "2") {
    std::string ageInput;

    if (!readValidatedField(
            "Enter new age (0-120): ",
            ageInput,
            [&newAge](const std::string &value, std::string &error) {
              return Patient::validateAge(value, newAge, error);
            })) {
      return;
    }

  } else if (choice == "3") {
    if (!readValidatedField(
            "Enter new gender (M/F/O): ",
            newGender,
            [](const std::string &value, std::string &error) {
              return Patient::validateGender(value, error);
            })) {
      return;
    }

  } else if (choice == "4") {
    if (!readValidatedField(
            "Enter new contact number (10 digits): ",
            newContact,
            [](const std::string &value, std::string &error) {
              return Patient::validateContactNumber(value, error);
            })) {
      return;
    }

  } else if (choice == "5") {
    if (!readValidatedField(
            "Enter new address (1-100 characters): ",
            newAddress,
            [](const std::string &value, std::string &error) {
              return Patient::validateAddress(value, error);
            })) {
      return;
    }

  } else if (choice == "6") {
    if (!readValidatedField(
            "Enter new medical history (optional, up to 200 characters; press Enter to skip): ",
            newMed,
            [](const std::string &value, std::string &error) {
              return Patient::validateMedicalHistory(value, error);
            })) {
      return;
    }

  } else if (choice == "7") {

    if (!readValidatedField(
            "Full name (1-50 characters): ",
            newName,
            [](const std::string &value, std::string &error) {
              return Patient::validateName(value, error);
            })) {
      return;
    }

    std::string ageInput;

    if (!readValidatedField(
            "Age (0-120): ",
            ageInput,
            [&newAge](const std::string &value, std::string &error) {
              return Patient::validateAge(value, newAge, error);
            })) {
      return;
    }

    if (!readValidatedField(
            "Gender (M/F/O): ",
            newGender,
            [](const std::string &value, std::string &error) {
              return Patient::validateGender(value, error);
            })) {
      return;
    }

    if (!readValidatedField(
            "Contact number (10 digits): ",
            newContact,
            [](const std::string &value, std::string &error) {
              return Patient::validateContactNumber(value, error);
            })) {
      return;
    }

    if (!readValidatedField(
            "Address (1-100 characters): ",
            newAddress,
            [](const std::string &value, std::string &error) {
              return Patient::validateAddress(value, error);
            })) {
      return;
    }

    if (!readValidatedField(
            "Medical history (optional, up to 200 characters; press Enter to skip): ",
            newMed,
            [](const std::string &value, std::string &error) {
              return Patient::validateMedicalHistory(value, error);
            })) {
      return;
    }

  } else {
    std::cout << "Invalid choice. Enter 0 to 7.\n";
    return;
  }

  // Persist only after every selected field has passed validation.
  const OpResult result = updatePatient(
      targetPatient->getId(),
      newName,
      newAge,
      newGender,
      newContact,
      newAddress,
      newMed);

  std::cout << result.message << '\n';
}

OpResult HospitalSystem::updatePatient(const Patient &patient) {
  const std::string targetId = toUpper(trim(patient.getId()));
  auto patients = fileManager_.loadPatients();
  bool found = false;

  for (auto &existing : patients) {
    if (toUpper(existing.getId()) == targetId) {
      found = true;
      break;
    }
  }

  if (!found) {
    return { false, "Patient not found" };
  }

  std::string validationError;
  if (!patient.validate(validationError)) {
    return { false, validationError };
  }

  std::string saveError;
  if (!fileManager_.updatePatient(patient, saveError)) {
    return { false, saveError };
  }

  return { true, "Patient updated successfully" };
}

OpResult HospitalSystem::updatePatient(const std::string &patientId,
                                      const std::string &name,
                                      int age,
                                      const std::string &gender,
                                      const std::string &contactNumber,
                                      const std::string &address,
                                      const std::string &medicalHistory) {
  const std::string targetId = toUpper(trim(patientId));
  auto patients = fileManager_.loadPatients();
  std::string canonicalId;
  bool found = false;

  for (const auto &existing : patients) {
    if (toUpper(existing.getId()) == targetId) {
      canonicalId = existing.getId();
      found = true;
      break;
    }
  }

  if (!found) {
    return { false, "Patient not found" };
  }

  Patient updated(canonicalId, name, age, gender, contactNumber, address, medicalHistory);

  std::string validationError;
  if (!updated.validate(validationError)) {
    return { false, validationError };
  }

  std::string saveError;
  if (!fileManager_.updatePatient(updated, saveError)) {
    return { false, saveError };
  }

  return { true, "Patient updated successfully" };
}

bool HospitalSystem::updatePatient(const std::string &patientId,
                                  const std::string &name,
                                  int age,
                                  const std::string &gender,
                                  const std::string &contactNumber,
                                  const std::string &address,
                                  const std::string &medicalHistory,
                                  std::string &errorMessage) {
  OpResult result = updatePatient(patientId, name, age, gender, contactNumber, address, medicalHistory);
  if (!result.success) {
    errorMessage = result.message;
    return false;
  }
  return true;
}

const Patient* HospitalSystem::findPatient(const std::string &patientId) const {
  patientsCache_ = fileManager_.loadPatients();
  const std::string targetId = toUpper(trim(patientId));

  for (const auto &p : patientsCache_) {
    if (toUpper(p.getId()) == targetId) {
      return &p;
    }
  }

  return nullptr;
}

const Patient* HospitalSystem::findPatientByIdentity(
    const std::string &name,
    const std::string &contactNumber,
    const std::string &excludePatientId) const {
  patientsCache_ = fileManager_.loadPatients();
  const std::string normName = FileManager::normalizeString(name);
  const std::string normContact = FileManager::normalizeString(contactNumber);
  const std::string normExclude = toLower(trim(excludePatientId));

  for (const auto &p : patientsCache_) {
    if (!normExclude.empty() && toLower(trim(p.getId())) == normExclude) {
      continue;
    }
    if (FileManager::normalizeString(p.getName()) == normName &&
        FileManager::normalizeString(p.getContactNumber()) == normContact) {
      return &p;
    }
  }

  return nullptr;
}

void HospitalSystem::addDoctor() {
  std::cout << "\n--- Add Doctor ---\n";

  std::string id;
  std::string name;
  std::string specialization;
  std::string department;
  std::string contactNumber;
  std::string availability;

  // Validate Doctor ID and retry duplicate IDs before continuing.
  while (true) {
    if (!readValidatedField(
            "Doctor ID (1-10 alphanumeric characters, e.g., D001): ",
            id,
            [](const std::string &value, std::string &error) {
              return Doctor::validateId(value, error);
            })) {
      return;
    }

    if (!fileManager_.doctorIdExists(toUpper(id))) {
      break;
    }

    std::cout << "Doctor ID already exists\n";
  }

  // Validate each field immediately after input.
  if (!readValidatedField(
          "Full name (1-50 characters): ",
          name,
          [](const std::string &value, std::string &error) {
            return Doctor::validateName(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Specialization (1-40 characters): ",
          specialization,
          [](const std::string &value, std::string &error) {
            return Doctor::validateSpecialization(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Department (1-40 characters): ",
          department,
          [](const std::string &value, std::string &error) {
            return Doctor::validateDepartment(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Contact number (10 digits): ",
          contactNumber,
          [](const std::string &value, std::string &error) {
            return Doctor::validateContactNumber(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Availability (1-60 characters, e.g., Mon-Fri 09:00-17:00): ",
          availability,
          [](const std::string &value, std::string &error) {
            return Doctor::validateAvailability(value, error);
          })) {
    return;
  }

  const Doctor doctor(id, name, specialization, department, contactNumber,
                      availability);

  // Final validation safeguard before saving.
  std::string validationError;

  if (!doctor.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

  // Check duplicates again immediately before persistence.
  if (fileManager_.doctorIdExists(doctor.getId())) {
    std::cout << "Doctor ID already exists\n";
    return;
  }

  std::string errorMessage;

  std::string existingDoctorId;
  if (fileManager_.doctorIdentityExists(doctor.getName(),
                                        doctor.getContactNumber(),
                                        doctor.getSpecialization(),
                                        &existingDoctorId)) {
    std::cout << "\nWarning: Potential duplicate doctor record found with Doctor ID: "
              << existingDoctorId << '\n';
    std::cout << "Proceed with registration? (Y/N): ";
    std::string response;
    if (!std::getline(std::cin, response)) {
      return;
    }
    response = toUpper(trim(response));
    if (response != "Y") {
      std::cout << "Registration cancelled.\n";
      return;
    }
  }

  if (!fileManager_.saveDoctor(doctor, errorMessage)) {
    std::cout << "Addition failed: " << errorMessage << '\n';
    return;
  }

  std::cout << "\nDoctor added successfully.\n"
            << "Doctor ID: " << doctor.getId() << '\n';
}


void HospitalSystem::viewDoctorDetails() {
  std::cout << "\n--- View Doctor Details ---\n";

  std::string doctorId;

  if (!readRequiredField("Enter doctor ID: (1-10 alphanumeric characters, e.g., D001): ", doctorId)) {
    return;
  }

  const std::vector<Doctor> doctors = fileManager_.loadDoctors();
  const std::string searchId = toUpper(doctorId);

  for (const Doctor &doctor : doctors) {
    if (toUpper(doctor.getId()) == searchId) {
      displayDoctorDetails(doctor);
      return;
    }
  }

  std::cout << "No matching doctor found\n";
}


void HospitalSystem::listDoctors() {
  std::cout << "\n--- Doctor List ---\n";

  std::size_t malformedCount = 0;
  const std::vector<Doctor> doctors =
      fileManager_.loadDoctors(&malformedCount);

  if (malformedCount > 0) {
    std::cout << malformedCount << " malformed record(s) skipped\n";
  }

  if (doctors.empty()) {
    std::cout << "No doctors registered\n";
    return;
  }

  std::cout << "Doctor ID | Name | Specialization | Department\n"
            << "---------------------------------------------\n";

  for (const Doctor &doctor : doctors) {
    std::cout << doctor.getId() << " | "
              << doctor.getName() << " | "
              << doctor.getSpecialization() << " | "
              << doctor.getDepartment() << '\n';
  }
}

void HospitalSystem::searchDoctors() {
  std::cout << "\n--- Search Doctors ---\n";

  std::string query;

  if (!readRequiredField(
          "Enter doctor ID, name, or specialization (e.g., D001, John, Cardiology): ", query)) {
    return;
  }

  const std::vector<Doctor> doctors = fileManager_.loadDoctors();
  const std::string normalizedQuery = toLower(query);

  // Exact doctor ID matches take priority.
  for (const Doctor &doctor : doctors) {
    if (toLower(doctor.getId()) == normalizedQuery) {
      displayDoctorDetails(doctor);
      return;
    }
  }

  // Otherwise, find every doctor matching the partial name or specialization.
  bool found = false;

  for (const Doctor &doctor : doctors) {
    if (toLower(doctor.getName()).find(normalizedQuery) !=
            std::string::npos ||
        toLower(doctor.getSpecialization()).find(normalizedQuery) !=
            std::string::npos) {
      displayDoctorDetails(doctor);
      found = true;
    }
  }

  if (!found) {
    std::cout << "No matching doctor found\n";
  }
}

void HospitalSystem::updateDoctor() {
  std::cout << "\n--- Update Doctor ---\n";

  std::string id;
  const Doctor *targetDoctor = nullptr;
  std::vector<Doctor> doctors;

  // Validate the Doctor ID and retry if it is invalid or not found.
  while (true) {
    if (!readValidatedField(
            "Enter doctor ID: (1-10 alphanumeric characters, e.g., D001): ",
            id,
            [](const std::string &value, std::string &error) {
              return Doctor::validateId(value, error);
            })) {
      return;
    }

    const std::string searchId = toUpper(trim(id));
    doctors = fileManager_.loadDoctors();
    targetDoctor = nullptr;

    for (const Doctor &doctor : doctors) {
      if (toUpper(doctor.getId()) == searchId) {
        targetDoctor = &doctor;
        break;
      }
    }

    if (targetDoctor != nullptr) {
      break;
    }

    std::cout << "Doctor not found. Please try again.\n";
  }

  displayDoctorDetails(*targetDoctor);

  std::string choice;

  // Keep asking until the user enters a valid menu choice.
  while (true) {
    std::cout << "\nSelect field to update:\n"
              << "1. Name\n"
              << "2. Specialization\n"
              << "3. Department\n"
              << "4. Contact number\n"
              << "5. Availability\n"
              << "6. All fields\n"
              << "0. Return\n"
              << "Enter choice: ";

    if (!std::getline(std::cin, choice)) {
      return;
    }

    choice = trim(choice);

    if (choice == "0") {
      return;
    }

    if (choice >= "1" && choice <= "6" && choice.size() == 1) {
      break;
    }

    std::cout << "Invalid choice. Enter 0 to 6.\n";
  }

  // Preserve existing values for fields not selected for update.
  std::string newName = targetDoctor->getName();
  std::string newSpec = targetDoctor->getSpecialization();
  std::string newDept = targetDoctor->getDepartment();
  std::string newContact = targetDoctor->getContactNumber();
  std::string newAvail = targetDoctor->getAvailability();

  if (choice == "1" || choice == "6") {
    if (!readValidatedField(
            choice == "6" ? "Full name (1-50 characters): "
                          : "Enter new name (1-50 characters): ",
            newName,
            [](const std::string &value, std::string &error) {
              return Doctor::validateName(value, error);
            })) {
      return;
    }
  }

  if (choice == "2" || choice == "6") {
    if (!readValidatedField(
            "Enter new specialization (1-40 characters): ",
            newSpec,
            [](const std::string &value, std::string &error) {
              return Doctor::validateSpecialization(value, error);
            })) {
      return;
    }
  }

  if (choice == "3" || choice == "6") {
    if (!readValidatedField(
            "Enter new department (1-40 characters): ",
            newDept,
            [](const std::string &value, std::string &error) {
              return Doctor::validateDepartment(value, error);
            })) {
      return;
    }
  }

  if (choice == "4" || choice == "6") {
    if (!readValidatedField(
            "Enter new contact number (10 digits): ",
            newContact,
            [](const std::string &value, std::string &error) {
              return Doctor::validateContactNumber(value, error);
            })) {
      return;
    }
  }

  if (choice == "5" || choice == "6") {
    if (!readValidatedField(
            "Enter new availability (1-60 characters, e.g., Mon-Fri 09:00-17:00): ",
            newAvail,
            [](const std::string &value, std::string &error) {
              return Doctor::validateAvailability(value, error);
            })) {
      return;
    }
  }

  // Save only after every selected field has passed validation.
  const OpResult result = updateDoctor(
      targetDoctor->getId(), newName, newSpec, newDept, newContact, newAvail);

  std::cout << result.message << '\n';
}

OpResult HospitalSystem::updateDoctor(const Doctor &doctor) {
  const std::string targetId = toUpper(trim(doctor.getId()));
  auto doctors = fileManager_.loadDoctors();
  bool found = false;

  for (auto &existing : doctors) {
    if (toUpper(existing.getId()) == targetId) {
      found = true;
      break;
    }
  }

  if (!found) {
    return { false, "Doctor not found" };
  }

  std::string validationError;
  if (!doctor.validate(validationError)) {
    return { false, validationError };
  }

  std::string saveError;
  if (!fileManager_.updateDoctor(doctor, saveError)) {
    return { false, saveError };
  }

  return { true, "Doctor updated successfully" };
}

OpResult HospitalSystem::updateDoctor(const std::string &doctorId,
                                      const std::string &name,
                                      const std::string &specialization,
                                      const std::string &department,
                                      const std::string &contactNumber,
                                      const std::string &availability) {
  const std::string targetId = toUpper(trim(doctorId));
  auto doctors = fileManager_.loadDoctors();
  std::string canonicalId;
  bool found = false;

  for (const auto &existing : doctors) {
    if (toUpper(existing.getId()) == targetId) {
      canonicalId = existing.getId();
      found = true;
      break;
    }
  }

  if (!found) {
    return { false, "Doctor not found" };
  }

  Doctor updated(canonicalId, name, specialization, department, contactNumber, availability);

  std::string validationError;
  if (!updated.validate(validationError)) {
    return { false, validationError };
  }

  std::string saveError;
  if (!fileManager_.updateDoctor(updated, saveError)) {
    return { false, saveError };
  }

  return { true, "Doctor updated successfully" };
}

bool HospitalSystem::updateDoctor(const std::string &doctorId,
                                  const std::string &name,
                                  const std::string &specialization,
                                  const std::string &department,
                                  const std::string &contactNumber,
                                  const std::string &availability,
                                  std::string &errorMessage) {
  OpResult result = updateDoctor(doctorId, name, specialization, department, contactNumber, availability);
  if (!result.success) {
    errorMessage = result.message;
    return false;
  }
  return true;
}


const Doctor* HospitalSystem::findDoctor(
    const std::string &doctorId) const {
  doctorsCache_ = fileManager_.loadDoctors();
  const std::string targetId = toUpper(trim(doctorId));

  for (const auto &doctor : doctorsCache_) {
    if (toUpper(doctor.getId()) == targetId) {
      return &doctor;
    }
  }

  return nullptr;
}


const Doctor* HospitalSystem::findDoctorByIdentity(
    const std::string &name,
    const std::string &contactNumber,
    const std::string &specialization,
    const std::string &excludeDoctorId) const {
  doctorsCache_ = fileManager_.loadDoctors();
  const std::string normName = FileManager::normalizeString(name);
  const std::string normContact = FileManager::normalizeString(contactNumber);
  const std::string normSpec = FileManager::normalizeString(specialization);
  const std::string normExclude = toLower(trim(excludeDoctorId));

  for (const auto &doctor : doctorsCache_) {
    if (!normExclude.empty() && toLower(trim(doctor.getId())) == normExclude) {
      continue;
    }
    if (FileManager::normalizeString(doctor.getName()) == normName &&
        FileManager::normalizeString(doctor.getContactNumber()) == normContact &&
        FileManager::normalizeString(doctor.getSpecialization()) == normSpec) {
      return &doctor;
    }
  }

  return nullptr;
}

std::string HospitalSystem::nextAppointmentId() const {
  int maxNum = 0;
  const auto appointments = fileManager_.loadAppointments();

  for (const auto &appt : appointments) {
    const std::string &id = appt.getId();

    if (id.size() >= 2 && (id[0] == 'A' || id[0] == 'a')) {
      try {
        const int num = std::stoi(id.substr(1));
        if (num > maxNum) {
          maxNum = num;
        }
      } catch (...) {
        // Skip malformed numeric parts.
      }
    }
  }

  int nextNum = maxNum + 1;

  while (true) {
    std::ostringstream oss;
    oss << 'A' << std::setw(4) << std::setfill('0') << nextNum;
    const std::string candidate = oss.str();

    if (!fileManager_.appointmentIdExists(candidate)) {
      return candidate;
    }

    ++nextNum;
  }
}

bool HospitalSystem::checkDoctorAvailability(
    const std::string &doctorId,
    const std::string &date,
    const std::string &time,
    std::string &message) const {

  std::string err;

  if (!Appointment::validateDate(date, err)) {
    message = err;
    return false;
  }

  if (!Appointment::validateTime(time, err)) {
    message = err;
    return false;
  }

  if (!fileManager_.doctorIdExists(doctorId)) {
    message = "Doctor not found";
    return false;
  }

  const auto appointments = fileManager_.loadAppointments();

  for (const auto &appt : appointments) {
    if (appt.conflictsWith(doctorId, date, time)) {
      message = "Doctor not available at the requested date and time";
      return false;
    }
  }

  message = "Available";
  return true;
}

bool HospitalSystem::bookAppointment(const std::string &patientId,
                                    const std::string &doctorId,
                                    const std::string &date,
                                    const std::string &time,
                                    std::string &outAppointmentId,
                                    std::string &errorMessage) {
  // Step 1: Validate Date format and calendar validity (VR-10, HMS-F-018)
  if (!Appointment::validateDate(date, errorMessage)) {
    return false;
  }

  // Step 2: Validate Time format (VR-11, HMS-F-018)
  if (!Appointment::validateTime(time, errorMessage)) {
    return false;
  }

  // Step 3: Verify patient exists (BR-03, HMS-F-016)
  if (!fileManager_.patientIdExists(patientId)) {
    errorMessage = "Patient not found";
    return false;
  }

  // Step 4: Verify doctor exists (BR-04, HMS-F-017)
  if (!fileManager_.doctorIdExists(doctorId)) {
    errorMessage = "Doctor not found";
    return false;
  }

  // Step 5: Check doctor availability / conflict (BR-05, HMS-F-019)
  std::string availMsg;
  if (!checkDoctorAvailability(doctorId, date, time, availMsg)) {
    errorMessage = availMsg;
    return false;
  }

  // Step 6: Generate unique sequential Appointment ID (VR-12, HMS-F-015)
  const std::string newId = nextAppointmentId();

  // Step 7: Create Appointment record with status Booked (VR-13)
  const Appointment newAppt(newId, patientId, doctorId, date, time,
                            AppointmentStatus::Booked);

  // Step 8: Transactional persistence to appointments.csv (HMS-F-029, HMS-F-034)
  if (!fileManager_.saveAppointment(newAppt, errorMessage)) {
    return false;
  }

  // Step 9: Return generated ID only after successful persistence
  outAppointmentId = newId;
  return true;
}

OpResult HospitalSystem::bookAppointment(const std::string &patientId,
                                        const std::string &doctorId,
                                        const std::string &date,
                                        const std::string &time,
                                        std::string &outAppointmentId) {
  std::string errorMessage;
  bool ok = bookAppointment(patientId, doctorId, date, time, outAppointmentId,
                            errorMessage);
  if (ok) {
    return { true, "Appointment booked successfully with ID: " + outAppointmentId };
  }
  return { false, errorMessage };
}

void HospitalSystem::bookAppointment() {
  std::cout << "\n--- Book Appointment ---\n";

  std::string patientId;
  std::string doctorId;
  std::string date;
  std::string time;

  // Validate the patient ID and confirm that the patient exists before
  // prompting for the next field.
  while (true) {
    if (!readValidatedField(
            "Patient ID (e.g., P001): ", patientId,
            [](const std::string &value, std::string &error) {
              return Patient::validateId(value, error);
            })) {
      return;
    }

    if (fileManager_.patientIdExists(toUpper(patientId))) {
      break;
    }

    std::cout << "Patient not found. Please try again.\n";
  }

  // Validate the doctor ID and confirm that the doctor exists immediately.
  while (true) {
    if (!readValidatedField(
            "Doctor ID (e.g., D001): ", doctorId,
            [](const std::string &value, std::string &error) {
              return Doctor::validateId(value, error);
            })) {
      return;
    }

    if (fileManager_.doctorIdExists(toUpper(doctorId))) {
      break;
    }

    std::cout << "Doctor not found. Please try again.\n";
  }

  if (!readValidatedField(
          "Date (YYYY-MM-DD): ", date,
          [](const std::string &value, std::string &error) {
            return Appointment::validateDate(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Time (HH:MM, 24-hour format): ", time,
          [](const std::string &value, std::string &error) {
            return Appointment::validateTime(value, error);
          })) {
    return;
  }

  std::string appointmentId;
  std::string errorMessage;

  if (!bookAppointment(patientId, doctorId, date, time, appointmentId,
                       errorMessage)) {
    std::cout << errorMessage << '\n';
    return;
  }

  std::cout << "\nAppointment booked successfully.\n"
            << "Appointment ID: " << appointmentId << '\n';
}


std::vector<Appointment> HospitalSystem::listAppointments(
    const std::string &filterType,
    const std::string &filterId,
    std::size_t *malformedCount) const {

  const auto all = fileManager_.loadAppointments(malformedCount);
  std::vector<Appointment> result;
  result.reserve(all.size());

  const std::string filterUpper = toUpper(trim(filterType));
  const std::string targetId = toUpper(trim(filterId));

  const bool showAll =
      filterUpper == "ALL" || filterUpper.empty();
  const bool filterByPatient = filterUpper == "PATIENT";
  const bool filterByDoctor = filterUpper == "DOCTOR";

  for (const auto &appt : all) {
    bool matches = showAll;

    if (filterByPatient) {
      matches = toUpper(appt.getPatientId()) == targetId;
    } else if (filterByDoctor) {
      matches = toUpper(appt.getDoctorId()) == targetId;
    }

    if (matches) {
      result.push_back(appt);
    }
  }

  // Preserve the existing chronological ordering.
  std::sort(result.begin(), result.end(),
            [](const Appointment &a, const Appointment &b) {
              if (a.getDate() != b.getDate()) {
                return a.getDate() < b.getDate();
              }
              if (a.getTime() != b.getTime()) {
                return a.getTime() < b.getTime();
              }
              return a.getId() < b.getId();
            });

  return result;
}


void HospitalSystem::viewAppointments(const std::string &filterType,
                                      const std::string &filterId) {
  std::cout << "\n--- View Appointments ---\n";

  const std::string filterUpper = toUpper(trim(filterType));
  const std::string targetId = trim(filterId);

  if (filterUpper == "PATIENT") {
    if (!targetId.empty() && !fileManager_.patientIdExists(targetId)) {
      std::cout << "Patient not found\n";
      return;
    }
  } else if (filterUpper == "DOCTOR") {
    if (!targetId.empty() && !fileManager_.doctorIdExists(targetId)) {
      std::cout << "Doctor not found\n";
      return;
    }
  }

  std::size_t malformedCount = 0;
  const auto appointments = listAppointments(filterType, filterId, &malformedCount);

  if (malformedCount > 0) {
    std::cout << malformedCount << " malformed record(s) skipped\n";
  }

  if (appointments.empty()) {
    std::cout << "No appointments found.\n";
    return;
  }

  const auto patients = fileManager_.loadPatients();
  const auto doctors = fileManager_.loadDoctors();

  std::unordered_map<std::string, std::string> patientNames;
  patientNames.reserve(patients.size());
  for (const auto &p : patients) {
    patientNames[toUpper(trim(p.getId()))] = p.getName();
  }

  std::unordered_map<std::string, std::string> doctorNames;
  doctorNames.reserve(doctors.size());
  for (const auto &d : doctors) {
    doctorNames[toUpper(trim(d.getId()))] = d.getName();
  }

  for (const auto &a : appointments) {
    std::string patName = "N/A";
    const auto itP = patientNames.find(toUpper(trim(a.getPatientId())));
    if (itP != patientNames.end()) {
      patName = itP->second;
    }

    std::string docName = "N/A";
    const auto itD = doctorNames.find(toUpper(trim(a.getDoctorId())));
    if (itD != doctorNames.end()) {
      docName = itD->second;
    }
    std::cout << "Appointment ID: " << a.getId()
              << " | Patient ID: " << a.getPatientId()
              << " | Patient Name: " << patName
              << " | Doctor ID: " << a.getDoctorId()
              << " | Doctor Name: " << docName
              << " | Date: " << a.getDate()
              << " | Time: " << a.getTime()
              << " | Status: " << a.getStatusString() << '\n';
  }
}

void HospitalSystem::viewAppointmentsInteractive() {
  std::cout << "\n=== View Appointments ===\n"
            << "1. All Appointments\n"
            << "2. Filter by Patient\n"
            << "3. Filter by Doctor\n"
            << "0. Return\n"
            << "Enter your choice: ";

  std::string choice;
  if (!std::getline(std::cin, choice)) {
    return;
  }
  choice = trim(choice);

  if (choice == "0") {
    return;
  } else if (choice == "1") {
    viewAppointments("ALL");
  } else if (choice == "2") {
    std::string patientId;
    if (!readRequiredField("Enter patient ID: (e.g., P001): ", patientId)) {
      return;
    }
    viewAppointments("PATIENT", patientId);
  } else if (choice == "3") {
    std::string doctorId;
    if (!readRequiredField("Enter doctor ID: (e.g., D001): ", doctorId)) {
      return;
    }
    viewAppointments("DOCTOR", doctorId);
  } else {
    std::cout << "Invalid choice. Enter 0 to 3.\n";
  }
}

OpResult HospitalSystem::cancelAppointment(const std::string &appointmentId,
                                          bool confirmed) {
  if (!confirmed) {
    return { false, "Cancellation cancelled by user" };
  }

  auto appointments = fileManager_.loadAppointments();
  const std::string targetId = toUpper(trim(appointmentId));
  bool found = false;

  for (auto &a : appointments) {
    if (toUpper(a.getId()) == targetId) {
      found = true;
      if (a.isCancelled()) {
        return { false, "Appointment already cancelled" };
      }
      a.cancel();
      break;
    }
  }

  if (!found) {
    return { false, "Appointment not found" };
  }

  std::string err;
  if (!fileManager_.saveAppointments(appointments, err)) {
    return { false, err };
  }

  appointmentsCache_ = appointments;
  return { true, "Appointment cancelled successfully" };
}

const Appointment* HospitalSystem::findAppointment(
    const std::string &appointmentId) const {
  appointmentsCache_ = fileManager_.loadAppointments();
  const std::string targetId = toUpper(trim(appointmentId));

  for (const auto &appt : appointmentsCache_) {
    if (toUpper(appt.getId()) == targetId) {
      return &appt;
    }
  }

  return nullptr;
}


void HospitalSystem::cancelAppointmentInteractive() {
  std::cout << "\n--- Cancel Appointment ---\n";

  std::string appointmentId;

  // Retry invalid, unknown, or already-cancelled appointment IDs.
  while (true) {
    if (!readValidatedField(
            "Enter appointment ID: (e.g., A0001): ", appointmentId,
            [](const std::string &value, std::string &error) {
              return Appointment::validateId(value, error);
            })) {
      return;
    }

    const auto appointments = fileManager_.loadAppointments();
    const std::string targetId = toUpper(appointmentId);

    const auto found = std::find_if(
        appointments.begin(), appointments.end(),
        [&targetId](const Appointment &appointment) {
          return toUpper(appointment.getId()) == targetId;
        });

    if (found == appointments.end()) {
      std::cout << "Appointment not found. Please try again.\n";
      continue;
    }

    if (found->isCancelled()) {
      std::cout
          << "Appointment already cancelled. Please try another ID.\n";
      continue;
    }

    break;
  }

  // Retrieve appointment and related details.
  const Appointment *appointment = findAppointment(appointmentId);

  if (appointment == nullptr) {
    std::cout << "Appointment not found\n";
    return;
  }

  const Patient *patient = findPatient(appointment->getPatientId());
  const Doctor *doctor = findDoctor(appointment->getDoctorId());

  // Display appointment summary.
  std::cout << "\n--- Appointment Summary ---\n";
  std::cout << "Appointment ID : " << appointment->getId() << '\n';
  std::cout << "Patient Name   : "
            << (patient ? patient->getName() : appointment->getPatientId())
            << '\n';
  std::cout << "Doctor Name    : "
            << (doctor ? doctor->getName() : appointment->getDoctorId())
            << '\n';
  std::cout << "Date           : " << appointment->getDate() << '\n';
  std::cout << "Time           : " << appointment->getTime() << '\n';
  std::cout << "Status         : " << appointment->getStatusString() << '\n';

  // Validate confirmation.
  std::string confirm;

  if (!readValidatedField(
          "Confirm cancellation? (Y/N): ", confirm,
          [](const std::string &value, std::string &error) {
            const std::string normalized = toUpper(value);

            if (normalized == "Y" || normalized == "N") {
              return true;
            }

            error = "Invalid confirmation. Enter Y or N.";
            return false;
          })) {
    return;
  }

  const bool confirmed = (toUpper(confirm) == "Y");
  const OpResult result = cancelAppointment(appointmentId, confirmed);

  std::cout << result.message << '\n';
}

void HospitalSystem::checkDoctorAvailabilityInteractive() {
  std::cout << "\n--- Check Doctor Availability ---\n";

  std::string doctorId;
  std::string date;
  std::string time;

  // Validate the doctor ID and verify existence before asking for a date.
  while (true) {
    if (!readValidatedField(
            "Enter doctor ID (e.g., D001): ", doctorId,
            [](const std::string &value, std::string &error) {
              return Doctor::validateId(value, error);
            })) {
      return;
    }

    if (fileManager_.doctorIdExists(toUpper(doctorId))) {
      break;
    }

    std::cout << "Doctor not found. Please try again.\n";
  }

  if (!readValidatedField(
          "Date (YYYY-MM-DD): ", date,
          [](const std::string &value, std::string &error) {
            return Appointment::validateDate(value, error);
          })) {
    return;
  }

  if (!readValidatedField(
          "Time (HH:MM, 24-hour format): ", time,
          [](const std::string &value, std::string &error) {
            return Appointment::validateTime(value, error);
          })) {
    return;
  }

  std::string message;
  const bool available =
      checkDoctorAvailability(doctorId, date, time, message);
  std::cout << (available ? "Available" : "Not available") << ": "
            << message << '\n';
}

} // namespace hms
