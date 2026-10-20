
#include "hms/HospitalSystem.h"
#include <algorithm>
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

} // namespace

HospitalSystem::HospitalSystem(FileManager fileManager)
    : fileManager_(std::move(fileManager)) {}

void HospitalSystem::loadRecordsAtStartup() {
  patients_ = fileManager_.loadPatients();
  doctors_ = fileManager_.loadDoctors();
  appointments_ = fileManager_.loadAppointments();

  // Reject appointments referring to patients or doctors that were not loaded.
  appointments_.erase(
      std::remove_if(
          appointments_.begin(), appointments_.end(),
          [this](const Appointment &appointment) {
            const bool patientExists =
                std::any_of(patients_.begin(), patients_.end(),
                            [&appointment](const Patient &patient) {
                              return patient.getId() ==
                                     appointment.getPatientId();
                            });

            const bool doctorExists =
                std::any_of(doctors_.begin(), doctors_.end(),
                            [&appointment](const Doctor &doctor) {
                              return doctor.getId() ==
                                     appointment.getDoctorId();
                            });

            return !patientExists || !doctorExists;
          }),
      appointments_.end());
}

void HospitalSystem::displayRecords() const {
  std::cout << "\n=== Records Loaded at Startup ===\n";

  std::cout << "\nPatients (" << patients_.size() << "):\n";
  if (patients_.empty()) {
    std::cout << "No patient records found.\n";
  } else {
    for (const auto &patient : patients_) {
      std::cout << "ID: " << patient.getId()
                << " | Name: " << patient.getName()
                << " | Age: " << patient.getAge()
                << " | Gender: " << patient.getGender()
                << " | Contact: " << patient.getContactNumber()
                << " | Address: " << patient.getAddress()
                << " | Medical History: " << patient.getMedicalHistory()
                << '\n';
    }
  }

  std::cout << "\nDoctors (" << doctors_.size() << "):\n";
  if (doctors_.empty()) {
    std::cout << "No doctor records found.\n";
  } else {
    for (const auto &doctor : doctors_) {
      std::cout << "ID: " << doctor.getId()
                << " | Name: " << doctor.getName()
                << " | Specialization: " << doctor.getSpecialization()
                << " | Department: " << doctor.getDepartment()
                << " | Contact: " << doctor.getContactNumber()
                << " | Availability: " << doctor.getAvailability()
                << '\n';
    }
  }

  std::cout << "\nAppointments (" << appointments_.size() << "):\n";
  if (appointments_.empty()) {
    std::cout << "No appointment records found.\n";
  } else {
    for (const auto &appointment : appointments_) {
      std::cout << "ID: " << appointment.getId()
                << " | Patient ID: " << appointment.getPatientId()
                << " | Doctor ID: " << appointment.getDoctorId()
                << " | Date: " << appointment.getDate()
                << " | Time: " << appointment.getTime()
                << " | Status: " << appointment.getStatus() << '\n';
    }
  }

  std::cout << "\n==================================\n";
}

void HospitalSystem::run() {
  loadRecordsAtStartup();
  displayRecords();

  while (true) {
    std::cout << "\n=== Hospital Management System ===\n"
              << "1. Register patient\n"
              << "2. Add doctor\n"
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
    } else {
      std::cout << "Invalid choice. Enter 1, 2, or 0.\n";
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

  patients_ = fileManager_.loadPatients();

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

  doctors_ = fileManager_.loadDoctors();

  std::cout << "\nDoctor added successfully.\n"
            << "Doctor ID: " << doctor.getId() << '\n';
}

} // namespace hms
