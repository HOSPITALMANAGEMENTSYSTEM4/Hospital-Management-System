
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int passed = 0;
int failed = 0;

void check(const std::string &description, bool condition) {
  if (condition) {
    ++passed;
    std::cout << "[PASS] " << description << '\n';
  } else {
    ++failed;
    std::cout << "[FAIL] " << description << '\n';
  }
}

std::string readFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}

std::string runInteractive(hms::HospitalSystem &system,
                           const std::string &input) {
  std::istringstream in(input);
  std::ostringstream out;
  auto *oldIn = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  system.run();

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);
  return out.str();
}

} // namespace

int main() {
  const auto testDir =
      std::filesystem::temp_directory_path() / "hms_tc_pat_07";
  std::filesystem::remove_all(testDir);
  std::filesystem::create_directories(testDir);

  const auto patientPath = (testDir / "patients.csv").string();
  const auto doctorPath = (testDir / "doctors.csv").string();
  const auto appointmentPath = (testDir / "appointments.csv").string();

  {
    std::ofstream patients(patientPath);
    patients << "patient_id,name,age,gender,contact,address,medical_history\n"
             << "P001,John Doe,40,M,9876543210,Main Street,None\n";
  }
  {
    std::ofstream doctors(doctorPath);
    doctors << "doctor_id,name,specialization,contact\n";
  }
  {
    std::ofstream appointments(appointmentPath);
    appointments << "appointment_id,patient_id,doctor_id,date,time,status\n";
  }

  hms::FileManager fm(patientPath, doctorPath, appointmentPath);
  hms::HospitalSystem system(fm);

  // TC-PAT-07: A nonexistent patient ID must not modify persisted data.
  const std::string before = readFile(patientPath);
  const hms::OpResult result = system.updatePatient(
      "P999", "Ghost Patient", 30, "M", "9876543210",
      "Nowhere Street", "None");

  check("TC-PAT-07: Nonexistent Patient ID is rejected", !result.success);
  check("TC-PAT-07: Error message is Patient not found",
        result.message == "Patient not found");
  check("TC-PAT-07: Patient CSV remains unchanged",
        readFile(patientPath) == before);

  // The interactive update menu must not offer an ID-edit option.
  const std::string output =
      runInteractive(system, "1\n4\nP001\n0\n4\n");

  check("TC-PAT-07: Update menu does not offer Patient ID editing",
        output.find("Patient ID edit") == std::string::npos &&
        output.find("Change Patient ID") == std::string::npos);

  std::filesystem::remove_all(testDir);

  std::cout << "\nTC-PAT-07 results: " << passed << " passed, "
            << failed << " failed\n";
  return failed == 0 ? 0 : 1;
}
