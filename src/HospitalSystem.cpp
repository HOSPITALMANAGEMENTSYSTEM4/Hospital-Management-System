
#include "hms/HospitalSystem.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

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

bool parseAge(const std::string &input, int &age) {
  std::istringstream stream(input);
  char extra = '\0';

  if (!(stream >> age) || (stream >> extra)) {
    return false;
  }

  return age >= 1 && age <= 130;
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

} // namespace

void HospitalSystem::run() {
  while (true) {
    std::cout << "\n=== Hospital Management System ===\n"
              << "1. Register patient\n"
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
    } else {
      std::cout << "Invalid choice. Enter 1 or 0.\n";
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

  std::cout << "Age (1-130): ";

  if (!std::getline(std::cin, ageInput)) {
    return;
  }

  int age = 0;

  if (!parseAge(trim(ageInput), age)) {
    std::cout << "Invalid age. Enter a whole number from 1 to 130.\n";
    return;
  }

  if (!readRequiredField("Gender: ", gender) ||
      !readRequiredField("Contact number: ", contactNumber) ||
      !readRequiredField("Address: ", address)) {
    return;
  }

  std::cout << "Medical history (optional; press Enter to skip): ";

  if (!std::getline(std::cin, medicalHistory)) {
    return;
  }

  medicalHistory = trim(medicalHistory);

  const Patient patient(id, name, age, gender, contactNumber, address,
                        medicalHistory);

  if (!patient.isValid()) {
    std::cout << "Registration rejected: patient details are invalid.\n";
    return;
  }

  if (fileManager_.patientIdExists(patient.getId())) {
    std::cout << "Registration rejected: patient ID already exists.\n";
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

} // namespace hms
