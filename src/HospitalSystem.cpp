#include "hms/HospitalSystem.h"

#include <algorithm>
#include <iomanip>
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

std::string toUpper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
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

void HospitalSystem::run() {
  while (true) {
    std::cout << "\n=== Hospital Management System ===\n"
              << "1. Register patient\n"
              << "2. Add doctor\n"
              << "3. Book appointment\n"
              << "4. View appointments\n"
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
      bookAppointment();
    } else if (choice == "4") {
      viewAppointments();
    } else {
      std::cout << "Invalid choice. Enter 1, 2, 3, 4, or 0.\n";
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

  // Construct the patient; constructor normalizes ID and gender to uppercase.
  const Patient patient(id, name, age, gender, contactNumber, address,
                        medicalHistory);

  // Validate with field-specific error reporting before the duplicate check.
  std::string validationError;
  if (!patient.validate(validationError)) {
    std::cout << validationError << '\n';
    return;
  }

  // Issue #2: case-insensitive duplicate detection (ID already uppercased).
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
    const std::string &filterId) const {
  const auto all = fileManager_.loadAppointments();
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

void HospitalSystem::viewAppointments() {
  std::cout << "\n--- View Appointments ---\n";
  const auto appointments = listAppointments("ALL");

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

} // namespace hms
