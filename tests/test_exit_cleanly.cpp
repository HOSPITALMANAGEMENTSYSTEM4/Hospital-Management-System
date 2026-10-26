#include <exception>
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
    if (condition) {
        std::cout << "[PASS] " << message << '\n';
    } else {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
}

void testExitReturnsNormally() {
    const std::filesystem::path directory =
        "build/test_exit_cleanly_data";

    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    const std::string patients = (directory / "patients.csv").string();
    const std::string doctors = (directory / "doctors.csv").string();
    const std::string appointments =
        (directory / "appointments.csv").string();

    hms::FileManager fileManager(patients, doctors, appointments);

    std::string error;
    const hms::Patient patient(
        "P901", "Exit Test Patient", 25, "F",
        "9876543210", "Bangalore", "None");

    check(fileManager.savePatient(patient, error),
          "A confirmed patient record is saved before exit");

    hms::HospitalSystem system(fileManager);
    std::istringstream input("4\n");
    std::ostringstream output;

    auto *oldInput = std::cin.rdbuf(input.rdbuf());
    auto *oldOutput = std::cout.rdbuf(output.rdbuf());

    try {
        system.run();
    } catch (...) {
        std::cin.rdbuf(oldInput);
        std::cout.rdbuf(oldOutput);
        throw;
    }

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    check(output.str().find("Exiting Hospital Management System.") !=
              std::string::npos,
          "Selecting Exit displays the exit confirmation");

    const auto savedPatients = fileManager.loadPatients();
    check(savedPatients.size() == 1 &&
              savedPatients[0].getId() == "P901",
          "The confirmed patient record remains saved after exit");

    hms::FileManager restartedFileManager(patients, doctors, appointments);
    const auto reloadedPatients = restartedFileManager.loadPatients();

    check(reloadedPatients.size() == 1 &&
              reloadedPatients[0].getId() == "P901",
          "The confirmed record remains available after reloading the data file");

    std::filesystem::remove_all(directory);
}

} // namespace

int main() {
    try {
        testExitReturnsNormally();
    } catch (const std::exception &error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All exit and persistence tests passed.\n";
    return 0;
}
