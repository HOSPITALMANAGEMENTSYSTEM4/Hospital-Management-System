
#include "hms/HospitalSystem.h"

#include <algorithm>
#include <cctype>
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

std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) {
                   return static_cast<char>(std::tolower(ch));
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

} // namespace

HospitalSystem::HospitalSystem(FileManager fileManager)
    : fileManager_(std::move(fileManager)) {}

void HospitalSystem::run() {
  while (true) {
    std::cout << "\n=== Hospital Management System ===\n"
              << "1. Register patient\n"
              << "2. Add doctor\n"
              << "3. Search patients\n"
              << "4. View patient details\n"
              << "5. List all patients\n"
              << "0. Exit\n"
              << "Enter your choice: ";

    std::string choice;

    if (!std::getline(std::cin, choice)) {
      std::cout << "\nExiting Hospital Management System.\n";
      return;
    }

    choice = trim(choice);

    if (choice == "0") {
      std::cout << "Exiting Hospital Management System.\n";
      return;
    }

    if (choice == "1") {
      registerPatient();
    } else if (choice == "2") {
      addDoctor();
    } else if (choice == "3") {
      searchPatients();
    } else if (choice == "4") {
      viewPatientDetails();
    } else if (choice == "5") {
      listPatients();
    } else {
      std::cout << "Invalid choice. Enter 1, 2, 3, 4, 5, or 0.\n";
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

  // The Patient constructor normalizes the ID and gender.
  const Patient patient(id, name, age, gender, contactNumber, address,
                        medicalHistory);

  std::string validationError;
  if (!patient.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

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
  const std::string normalizedId = toLower(patientId);

  for (const Patient &patient : patients) {
    if (toLower(patient.getId()) == normalizedId) {
      displayPatientDetails(patient);
      return;
    }
  }

  std::cout << "No matching patient found\n";
}

void HospitalSystem::listPatients() {
  std::cout << "\n--- Patient List ---\n";

  const std::vector<Patient> patients = fileManager_.loadPatients();

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

} // namespace hms
