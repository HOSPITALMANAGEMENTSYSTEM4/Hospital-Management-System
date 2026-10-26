
#include "hms/HospitalSystem.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
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
  std::istringstream stream(input);
  char extra = '\0';

  if (!(stream >> age) || (stream >> extra)) {
    return false;
  }

  return age >= 0 && age <= 120;
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

// HMS-F-032, SRS §3.1 - Patient submenu
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

// HMS-F-032, SRS §3.1 - Doctor submenu
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

// HMS-F-032, SRS §3.1 - Appointment submenu
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

  if (!readRequiredField("Patient ID: ", id) ||
      !readRequiredField("Full name: ", name)) {
    return;
  }

  std::cout << "Age (0-120): ";

  if (!std::getline(std::cin, ageInput)) {
    return;
  }

  int age = 0;

  if (!parseAge(trim(ageInput), age)) {
    std::cout << "Age: must be a whole number from 0 to 120.\n";
    return;
  }

  if (!readRequiredField("Gender (M/F/O): ", gender) ||
      !readRequiredField("Contact number: ", contactNumber) ||
      !readRequiredField("Address: ", address)) {
    return;
  }

  std::cout << "Medical history (optional; press Enter to skip): ";

  if (!std::getline(std::cin, medicalHistory)) {
    return;
  }

  medicalHistory = trim(medicalHistory);

  // The Patient constructor normalizes the ID and gender to uppercase.
  const Patient patient(id, name, age, gender, contactNumber, address,
                        medicalHistory);

  std::string validationError;

  if (!patient.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

  // Case-insensitive duplicate detection.
  if (fileManager_.patientIdExists(patient.getId())) {
    std::cout << "Patient ID already exists\n";
    return;
  }

  std::string errorMessage;

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

  if (!readRequiredField("Enter patient ID or name: ", query)) {
    return;
  }

  const std::vector<Patient> patients = fileManager_.loadPatients();
  const std::string normalizedQuery = toLower(query);

  // An exact patient ID match takes priority over name matches.
  for (const Patient &patient : patients) {
    if (toLower(patient.getId()) == normalizedQuery) {
      displayPatientDetails(patient);
      return;
    }
  }

  // Otherwise, display every patient whose name contains the query.
  bool found = false;

  for (const Patient &patient : patients) {
    if (toLower(patient.getName()).find(normalizedQuery) !=
        std::string::npos) {
      displayPatientDetails(patient);
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

  if (!readRequiredField("Enter patient ID: ", patientId)) {
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
  if (!readRequiredField("Enter patient ID: ", id)) {
    return;
  }

  const std::string searchId = toUpper(trim(id));
  const std::vector<Patient> patients = fileManager_.loadPatients();
  const Patient *targetPatient = nullptr;

  for (const Patient &patient : patients) {
    if (toUpper(patient.getId()) == searchId) {
      targetPatient = &patient;
      break;
    }
  }

  if (!targetPatient) {
    std::cout << "Patient not found\n";
    return;
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

  std::string newName = targetPatient->getName();
  int newAge = targetPatient->getAge();
  std::string newGender = targetPatient->getGender();
  std::string newContact = targetPatient->getContactNumber();
  std::string newAddress = targetPatient->getAddress();
  std::string newMed = targetPatient->getMedicalHistory();

  if (choice == "1") {
    if (!readRequiredField("Enter new name: ", newName)) {
      return;
    }
  } else if (choice == "2") {
    std::cout << "Enter new age (0-120): ";
    std::string ageInput;
    if (!std::getline(std::cin, ageInput)) {
      return;
    }
    if (!parseAge(trim(ageInput), newAge)) {
      std::cout << "Age: must be a whole number from 0 to 120.\n";
      return;
    }
  } else if (choice == "3") {
    if (!readRequiredField("Enter new gender (M/F/O): ", newGender)) {
      return;
    }
  } else if (choice == "4") {
    if (!readRequiredField("Enter new contact number: ", newContact)) {
      return;
    }
  } else if (choice == "5") {
    if (!readRequiredField("Enter new address: ", newAddress)) {
      return;
    }
  } else if (choice == "6") {
    std::cout << "Enter new medical history (optional; press Enter to skip): ";
    if (!std::getline(std::cin, newMed)) {
      return;
    }
    newMed = trim(newMed);
  } else if (choice == "7") {
    if (!readRequiredField("Full name: ", newName)) {
      return;
    }
    std::cout << "Age (0-120): ";
    std::string ageInput;
    if (!std::getline(std::cin, ageInput)) {
      return;
    }
    if (!parseAge(trim(ageInput), newAge)) {
      std::cout << "Age: must be a whole number from 0 to 120.\n";
      return;
    }
    if (!readRequiredField("Gender (M/F/O): ", newGender) ||
        !readRequiredField("Contact number: ", newContact) ||
        !readRequiredField("Address: ", newAddress)) {
      return;
    }
    std::cout << "Medical history (optional; press Enter to skip): ";
    if (!std::getline(std::cin, newMed)) {
      return;
    }
    newMed = trim(newMed);
  } else {
    std::cout << "Invalid choice. Enter 0 to 7.\n";
    return;
  }

  const OpResult result = updatePatient(targetPatient->getId(), newName, newAge,
                                       newGender, newContact, newAddress, newMed);
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

void HospitalSystem::addDoctor() {
  std::cout << "\n--- Add Doctor ---\n";

  std::string id;
  std::string name;
  std::string specialization;
  std::string department;
  std::string contactNumber;
  std::string availability;

  if (!readRequiredField("Doctor ID: ", id) ||
      !readRequiredField("Full name: ", name) ||
      !readRequiredField("Specialization: ", specialization) ||
      !readRequiredField("Department: ", department) ||
      !readRequiredField("Contact number: ", contactNumber) ||
      !readRequiredField("Availability: ", availability)) {
    return;
  }

  const Doctor doctor(id, name, specialization, department, contactNumber,
                      availability);

  std::string validationError;

  if (!doctor.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

  if (fileManager_.doctorIdExists(doctor.getId())) {
    std::cout << "Doctor ID already exists\n";
    return;
  }

  std::string errorMessage;

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

  if (!readRequiredField("Enter doctor ID: ", doctorId)) {
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
          "Enter doctor ID, name, or specialization: ", query)) {
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
  if (!readRequiredField("Enter doctor ID: ", id)) {
    return;
  }

  const std::string searchId = toUpper(trim(id));
  const std::vector<Doctor> doctors = fileManager_.loadDoctors();
  const Doctor *targetDoctor = nullptr;

  for (const Doctor &doctor : doctors) {
    if (toUpper(doctor.getId()) == searchId) {
      targetDoctor = &doctor;
      break;
    }
  }

  if (!targetDoctor) {
    std::cout << "Doctor not found\n";
    return;
  }

  displayDoctorDetails(*targetDoctor);

  std::cout << "\nSelect field to update:\n"
            << "1. Name\n"
            << "2. Specialization\n"
            << "3. Department\n"
            << "4. Contact number\n"
            << "5. Availability\n"
            << "6. All fields\n"
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

  std::string newName = targetDoctor->getName();
  std::string newSpec = targetDoctor->getSpecialization();
  std::string newDept = targetDoctor->getDepartment();
  std::string newContact = targetDoctor->getContactNumber();
  std::string newAvail = targetDoctor->getAvailability();

  if (choice == "1") {
    if (!readRequiredField("Enter new name: ", newName)) {
      return;
    }
  } else if (choice == "2") {
    if (!readRequiredField("Enter new specialization: ", newSpec)) {
      return;
    }
  } else if (choice == "3") {
    if (!readRequiredField("Enter new department: ", newDept)) {
      return;
    }
  } else if (choice == "4") {
    if (!readRequiredField("Enter new contact number: ", newContact)) {
      return;
    }
  } else if (choice == "5") {
    if (!readRequiredField("Enter new availability: ", newAvail)) {
      return;
    }
  } else if (choice == "6") {
    if (!readRequiredField("Full name: ", newName) ||
        !readRequiredField("Specialization: ", newSpec) ||
        !readRequiredField("Department: ", newDept) ||
        !readRequiredField("Contact number: ", newContact) ||
        !readRequiredField("Availability: ", newAvail)) {
      return;
    }
  } else {
    std::cout << "Invalid choice. Enter 0 to 6.\n";
    return;
  }

  const OpResult result = updateDoctor(targetDoctor->getId(), newName, newSpec,
                                       newDept, newContact, newAvail);
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

const Doctor* HospitalSystem::findDoctor(const std::string &doctorId) const {
  doctorsCache_ = fileManager_.loadDoctors();
  const std::string targetId = toUpper(trim(doctorId));

  for (const auto &d : doctorsCache_) {
    if (toUpper(d.getId()) == targetId) {
      return &d;
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
        int num = std::stoi(id.substr(1));
        if (num > maxNum) {
          maxNum = num;
        }
      } catch (...) {
        // Skip malformed numeric parts
      }
    }
  }

  int nextNum = maxNum + 1;
  std::ostringstream oss;
  oss << 'A' << std::setw(4) << std::setfill('0') << nextNum;
  std::string candidate = oss.str();

  while (fileManager_.appointmentIdExists(candidate)) {
    ++nextNum;
    oss.str("");
    oss.clear();
    oss << 'A' << std::setw(4) << std::setfill('0') << nextNum;
    candidate = oss.str();
  }

  return candidate;
}

bool HospitalSystem::checkDoctorAvailability(const std::string &doctorId,
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

  if (!readRequiredField("Patient ID: ", patientId) ||
      !readRequiredField("Doctor ID: ", doctorId) ||
      !readRequiredField("Date (YYYY-MM-DD): ", date) ||
      !readRequiredField("Time (HH:MM): ", time)) {
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

  const std::string filterUpper = toUpper(trim(filterType));
  const std::string targetId = toUpper(trim(filterId));

  for (const auto &appt : all) {
    if (filterUpper == "ALL" || filterUpper.empty()) {
      result.push_back(appt);
    } else if (filterUpper == "PATIENT") {
      if (toUpper(appt.getPatientId()) == targetId) {
        result.push_back(appt);
      }
    } else if (filterUpper == "DOCTOR") {
      if (toUpper(appt.getDoctorId()) == targetId) {
        result.push_back(appt);
      }
    }
  }

  // Sort chronologically by Date, then Time, then ID (HMS-F-022, SAD 4.3)
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
  std::size_t malformedCount = 0;
  const auto appointments = listAppointments(filterType, filterId, &malformedCount);

  if (malformedCount > 0) {
    std::cout << malformedCount << " malformed record(s) skipped\n";
  }

  if (appointments.empty()) {
    std::cout << "No appointments found.\n";
    return;
  }

  for (const auto &a : appointments) {
    std::cout << "Appointment ID: " << a.getId()
              << " | Patient ID: " << a.getPatientId()
              << " | Doctor ID: " << a.getDoctorId()
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
    if (!readRequiredField("Enter patient ID: ", patientId)) {
      return;
    }
    viewAppointments("PATIENT", patientId);
  } else if (choice == "3") {
    std::string doctorId;
    if (!readRequiredField("Enter doctor ID: ", doctorId)) {
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

  if (!readRequiredField("Enter appointment ID: ", appointmentId)) {
    return;
  }

  std::cout << "Confirm cancellation? (Y/N): ";
  std::string confirm;

  if (!std::getline(std::cin, confirm)) {
    return;
  }

  confirm = trim(confirm);
  const bool confirmed = (confirm == "Y" || confirm == "y");
  const OpResult result = cancelAppointment(appointmentId, confirmed);
  std::cout << result.message << '\n';
}

void HospitalSystem::checkDoctorAvailabilityInteractive() {
  std::cout << "\n--- Check Doctor Availability ---\n";

  std::string doctorId;
  std::string date;
  std::string time;

  if (!readRequiredField("Enter doctor ID: ", doctorId) ||
      !readRequiredField("Date (YYYY-MM-DD): ", date) ||
      !readRequiredField("Time (HH:MM): ", time)) {
    return;
  }

  std::string message;
  const bool available = checkDoctorAvailability(doctorId, date, time, message);
  std::cout << (available ? "Available" : "Not available") << ": "
            << message << '\n';
}

} // namespace hms
