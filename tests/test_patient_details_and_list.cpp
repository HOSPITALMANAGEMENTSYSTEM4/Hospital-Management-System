
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
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

void writePatients(const std::string &path) {
    hms::FileManager fileManager(path, "build/test_details_doctors.csv");

    const std::vector<hms::Patient> patients = {
        {"P101", "Anita Sharma", 21, "F", "9876543210",
         "Bangalore", "None"},
        {"P102", "Anita Rao", 22, "F", "9876543211",
         "Mysore", "Asthma"},
        {"P103", "Rahul Kumar", 23, "M", "9876543212",
         "Mangalore", "None"}
    };

    for (const auto &patient : patients) {
        std::string error;
        if (!fileManager.savePatient(patient, error)) {
            throw std::runtime_error("Could not save test patient: " + error);
        }
    }
}

std::string runMenu(const std::string &path,
                    const std::string &menuInput) {
    hms::HospitalSystem system{
        hms::FileManager(path, "build/test_details_doctors.csv")};

    std::istringstream input(menuInput);
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

    return output.str();
}

void testViewPatientDetails() {
    const std::string path = "build/test_view_details.csv";
    std::filesystem::remove(path);
    writePatients(path);

    const std::string output = runMenu(path, "4\nP102\n0\n");

    check(output.find("Patient ID: P102") != std::string::npos,
          "Option 4 displays the requested patient ID");
    check(output.find("Name: Anita Rao") != std::string::npos,
          "Option 4 displays the patient's name");
    check(output.find("Age: 22") != std::string::npos,
          "Option 4 displays the patient's age");
    check(output.find("Address: Mysore") != std::string::npos,
          "Option 4 displays the patient's address");
    check(output.find("Medical history: Asthma") != std::string::npos,
          "Option 4 displays the patient's medical history");
    check(output.find("Name: Anita Sharma") == std::string::npos,
          "Option 4 displays only the requested patient's details");

    std::filesystem::remove(path);
}

void testUnknownPatientDetails() {
    const std::string path = "build/test_view_unknown.csv";
    std::filesystem::remove(path);
    writePatients(path);

    const std::string output = runMenu(path, "4\nP999\n0\n");

    check(output.find("No matching patient found") != std::string::npos,
          "Option 4 reports an unknown patient ID");

    std::filesystem::remove(path);
}

void testListAllPatients() {
    const std::string path = "build/test_list_patients.csv";
    std::filesystem::remove(path);
    writePatients(path);

    const std::string output = runMenu(path, "5\n0\n");

    check(output.find("P101 | Anita Sharma") != std::string::npos,
          "Option 5 lists the first patient");
    check(output.find("P102 | Anita Rao") != std::string::npos,
          "Option 5 lists the second patient");
    check(output.find("P103 | Rahul Kumar") != std::string::npos,
          "Option 5 lists the third patient");

    std::filesystem::remove(path);
}

void testEmptyPatientList() {
    const std::string path = "build/test_list_empty.csv";
    std::filesystem::remove(path);

    const std::string output = runMenu(path, "5\n0\n");

    check(output.find("No patients registered") != std::string::npos,
          "Option 5 handles an empty patient list");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    std::filesystem::create_directories("build");

    try {
        testViewPatientDetails();
        testUnknownPatientDetails();
        testListAllPatients();
        testEmptyPatientList();
    } catch (const std::exception &error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All patient details and listing tests passed.\n";
    return 0;
}
