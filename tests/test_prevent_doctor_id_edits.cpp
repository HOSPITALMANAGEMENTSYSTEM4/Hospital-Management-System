
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Doctor.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& description, bool condition) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << description << '\n';
    } else {
        ++failed;
        std::cout << "[FAIL] " << description << '\n';
    }
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

} // namespace

int main() {
    const auto testDir =
        std::filesystem::temp_directory_path() / "hms_tc_doc_07";

    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    const auto doctorPath = (testDir / "doctors.csv").string();
    const auto patientPath = (testDir / "patients.csv").string();
    const auto appointmentPath = (testDir / "appointments.csv").string();

    {
        std::ofstream doctors(doctorPath);
        doctors << "doctor_id,name,specialization,contact\n"
                << "D001,John Smith,Cardiology,9876543210\n";
    }
    {
        std::ofstream patients(patientPath);
        patients << "patient_id,name,age,gender,contact,address,medical_history\n";
    }
    {
        std::ofstream appointments(appointmentPath);
        appointments << "appointment_id,patient_id,doctor_id,date,time,status\n";
    }

    hms::FileManager fm(patientPath, doctorPath, appointmentPath);
    hms::HospitalSystem system(fm);

    const std::string before = readFile(doctorPath);

    // A nonexistent Doctor ID must be rejected without changing the CSV.
    const hms::OpResult result = system.updateDoctor(
    "D999",
    "Unknown Doctor",
    "Cardiology",
    "General Medicine",
    "9876543210",
    "Available");

    check("TC-DOC-07: Nonexistent Doctor ID is rejected",
          !result.success);

    check("TC-DOC-07: Error message is Doctor not found",
          result.message == "Doctor not found");

    check("TC-DOC-07: Doctor CSV remains unchanged",
          readFile(doctorPath) == before);

    std::filesystem::remove_all(testDir);

    std::cout << "\nTC-DOC-07 results: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
