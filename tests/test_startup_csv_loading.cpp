#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace {
int failures = 0;

void check(bool condition, const std::string &message) {
  if (condition) {
    std::cout << "[PASS] " << message << '\n';
  } else {
    std::cout << "[FAIL] " << message << '\n';
    ++failures;
  }
}

bool seedData(const std::filesystem::path &dataDir) {
  hms::FileManager files((dataDir / "patients.csv").string(),
                         (dataDir / "doctors.csv").string(),
                         (dataDir / "appointments.csv").string());
  std::string error;
  const hms::Patient patients[] = {
      {"P001", "Anita Sharma", 30, "F", "9876543210", "Bengaluru", "None"},
      {"P002", "Rahul Kumar", 35, "M", "9876543211", "Mysuru", "None"}};
  for (const auto &patient : patients) {
    if (!files.savePatient(patient, error)) {
      std::cerr << "Could not seed patient: " << error << '\n';
      return false;
    }
  }

  const hms::Doctor doctors[] = {
      {"D001", "Dr. Meera Rao", "Cardiology", "Heart", "9876543212", "Mon-Fri 09:00-17:00"},
      {"D002", "Dr. Arun Das", "Neurology", "Neuro", "9876543213", "Mon-Fri 09:00-17:00"}};
  for (const auto &doctor : doctors) {
    if (!files.saveDoctor(doctor, error)) {
      std::cerr << "Could not seed doctor: " << error << '\n';
      return false;
    }
  }

  const hms::Appointment appointments[] = {
      {"A0001", "P001", "D001", "2026-10-23", "10:00", hms::AppointmentStatus::Booked},
      {"A0002", "P002", "D002", "2026-10-23", "11:00", hms::AppointmentStatus::Booked}};
  for (const auto &appointment : appointments) {
    if (!files.saveAppointment(appointment, error)) {
      std::cerr << "Could not seed appointment: " << error << '\n';
      return false;
    }
  }
  return true;
}
} // namespace

int main() {
  // Isolated test data: never reads or changes the project's real data/ files.
  const std::filesystem::path dataDir =
      std::filesystem::path("build") / "TC-PER-01-startup-data";
  std::error_code ec;
  std::filesystem::remove_all(dataDir, ec);
  std::filesystem::create_directories(dataDir, ec);
  if (ec) {
    std::cerr << "Could not create isolated test directory: " << ec.message() << '\n';
    return 1;
  }

  if (!seedData(dataDir)) {
    std::filesystem::remove_all(dataDir, ec);
    return 1;
  }

  hms::HospitalSystem system(dataDir.string());
  std::istringstream input("0\n");
  std::ostringstream output;
  auto *oldInput = std::cin.rdbuf(input.rdbuf());
  auto *oldOutput = std::cout.rdbuf(output.rdbuf());
  system.displayStartupRecords();
  system.run();
  std::cin.rdbuf(oldInput);
  std::cout.rdbuf(oldOutput);

  const std::string displayed = output.str();
  check(displayed.find("=== Records Loaded at Startup ===") != std::string::npos,
        "startup records section appears before the interactive menu");
  check(displayed.find("P001 | Anita Sharma") != std::string::npos &&
            displayed.find("P002 | Rahul Kumar") != std::string::npos,
        "both patient records are displayed at startup");
  check(displayed.find("D001 | Dr. Meera Rao") != std::string::npos &&
            displayed.find("D002 | Dr. Arun Das") != std::string::npos,
        "both doctor records are displayed at startup");
  check(displayed.find("Appointment ID: A0001") != std::string::npos &&
            displayed.find("Appointment ID: A0002") != std::string::npos,
        "both appointment records are displayed at startup");
  check(displayed.find("Enter your choice:") != std::string::npos,
        "interactive menu appears after startup records");

  std::cout << "\n--- TC-PER-01 captured startup output ---\n" << displayed;
  std::filesystem::remove_all(dataDir, ec);
  std::cout << "\nTC-PER-01: " << (failures == 0 ? "PASS" : "FAIL") << '\n';
  return failures == 0 ? 0 : 1;
}
