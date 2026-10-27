
#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "[PASS] " << message << '\n';
    } else {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
}

void runApplication(const std::string& dataDir) {
    hms::HospitalSystem system(dataDir);

    std::istringstream input("4\n");
    std::ostringstream output;

    auto* oldInput = std::cin.rdbuf(input.rdbuf());
    auto* oldOutput = std::cout.rdbuf(output.rdbuf());

    try {
        system.displayStartupRecords();
        system.run();
    } catch (...) {
        std::cin.rdbuf(oldInput);
        std::cout.rdbuf(oldOutput);
        throw;
    }

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    check(output.str().find("Exiting Hospital Management System.")
              != std::string::npos,
          "Application exits normally");
}

} // namespace

int main() {
    const fs::path dataDir =
        fs::path("build") / "TC-NFR-03-DataRetention";

    std::error_code ec;
    fs::remove_all(dataDir, ec);
    fs::create_directories(dataDir, ec);

    if (ec) {
        std::cerr << "[FAIL] Could not create test directory: "
                  << ec.message() << '\n';
        return 1;
    }

    const std::string directory = dataDir.string();

    hms::FileManager files(
        (dataDir / "patients.csv").string(),
        (dataDir / "doctors.csv").string(),
        (dataDir / "appointments.csv").string());

    std::string error;

    const hms::Patient patient(
        "P901", "Retention Patient", 28, "F",
        "9876543210", "Bengaluru", "None");

    const hms::Doctor doctor(
        "D901", "Dr Retention", "Cardiology",
        "Heart", "9876543211", "Mon-Fri 09:00-17:00");

    const hms::Appointment appointment(
        "A9001", "P901", "D901", "2026-10-28", "10:00",
        hms::AppointmentStatus::Booked);

    check(files.savePatient(patient, error),
          "Patient record saved before restart");

    error.clear();
    check(files.saveDoctor(doctor, error),
          "Doctor record saved before restart");

    error.clear();
    check(files.saveAppointment(appointment, error),
          "Appointment record saved before restart");

    // First application run.
    runApplication(directory);

    // Simulate a fresh application instance using the same CSV files.
    runApplication(directory);

    hms::FileManager restartedFiles(
        (dataDir / "patients.csv").string(),
        (dataDir / "doctors.csv").string(),
        (dataDir / "appointments.csv").string());

    const auto patients = restartedFiles.loadPatients();
    const auto doctors = restartedFiles.loadDoctors();
    const auto appointments = restartedFiles.loadAppointments();

    check(patients.size() == 1,
          "Patient count is retained after restart");

    check(doctors.size() == 1,
          "Doctor count is retained after restart");

    check(appointments.size() == 1,
          "Appointment count is retained after restart");

    if (patients.size() == 1) {
        check(patients[0].getId() == "P901" &&
              patients[0].getName() == "Retention Patient" &&
              patients[0].getAge() == 28 &&
              patients[0].getGender() == "F" &&
              patients[0].getContactNumber() == "9876543210" &&
              patients[0].getAddress() == "Bengaluru" &&
              patients[0].getMedicalHistory() == "None",
              "All patient field values are retained");
    }

    if (doctors.size() == 1) {
        check(doctors[0].getId() == "D901" &&
              doctors[0].getName() == "Dr Retention" &&
              doctors[0].getSpecialization() == "Cardiology" &&
              doctors[0].getDepartment() == "Heart" &&
              doctors[0].getContactNumber() == "9876543211" &&
              doctors[0].getAvailability() == "Mon-Fri 09:00-17:00",
              "All doctor field values are retained");
    }

    if (appointments.size() == 1) {
        check(appointments[0].getId() == "A9001" &&
              appointments[0].getPatientId() == "P901" &&
              appointments[0].getDoctorId() == "D901" &&
              appointments[0].getDate() == "2026-10-28" &&
              appointments[0].getTime() == "10:00" &&
              appointments[0].getStatus() ==
                  hms::AppointmentStatus::Booked,
              "All appointment field values are retained");
    }

    fs::remove_all(dataDir, ec);

    std::cout << "\nTC-NFR-03 results: "
              << (failures == 0 ? "PASS" : "FAIL") << '\n';

    return failures == 0 ? 0 : 1;
}
