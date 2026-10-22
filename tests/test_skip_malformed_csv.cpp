
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

void testSkipMalformedPatientRecords() {
    const std::string path = "build/test_malformed_patients.csv";
    const std::string doctorPath = "build/test_malformed_doctors.csv";

    std::filesystem::create_directories("build");

    {
        std::ofstream csv(path);
        csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";

                // Ten valid patient records.
        const std::vector<std::string> names = {
            "Anita", "Rahul", "Priya", "Arun", "Meera",
            "Kiran", "Divya", "Ravi", "Neha", "Suman"
        };

        for (int i = 1; i <= 10; ++i) {
            csv << "P" << i << "," << names[i - 1]
                << ",25,M,9876543210,City,None\n";
        }

        // Three malformed records:
        // 1. Unterminated quoted field.
        csv << "P11,\"Broken name,25,M,9876543211,City,None\n";

        // 2. Wrong number of columns.
        csv << "P12,Too few columns,25,M\n";

        // 3. Invalid age.
        csv << "P13,Invalid age,not-a-number,M,9876543213,City,None\n";
    }

    hms::FileManager fileManager(path, doctorPath);
    std::size_t malformedCount = 0;
    const auto patients = fileManager.loadPatients(&malformedCount);

    check(patients.size() == 10,
          "Ten valid patient records are loaded");

    check(malformedCount == 3,
          "Three malformed patient records are counted");

    // Select List all patients (menu option 7), then Exit (option 0).
    hms::HospitalSystem system(fileManager);
    std::istringstream input("7\n0\n");
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

    const std::string displayed = output.str();

    check(displayed.find("3 malformed record(s) skipped")
              != std::string::npos,
          "The exact malformed-record warning is displayed");

    check(displayed.find("P10 | Suman") != std::string::npos,
      "The last valid patient is displayed");
    check(displayed.find("Exiting Hospital Management System.")
              != std::string::npos,
          "The application continues and exits normally after listing");

    std::filesystem::remove(path);
    std::filesystem::remove(doctorPath);
}

} // namespace

int main() {
    try {
        testSkipMalformedPatientRecords();
    } catch (const std::exception &error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All malformed CSV tests passed.\n";
    return 0;
}
