
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    const std::string testCsv = "build/test_doc05_doctors.csv";
    std::remove(testCsv.c_str());

    hms::FileManager fm("data/patients.csv", testCsv);
    hms::HospitalSystem system(fm);

    // TC-DOC-05: Empty doctor list
    {
        std::istringstream input("");
        std::ostringstream output;
        auto* oldIn = std::cin.rdbuf(input.rdbuf());
        auto* oldOut = std::cout.rdbuf(output.rdbuf());

        system.listDoctors();

        std::cin.rdbuf(oldIn);
        std::cout.rdbuf(oldOut);

        if (output.str().find("No doctors registered") == std::string::npos) {
            std::cerr << "[FAIL] Empty list message\n";
            std::remove(testCsv.c_str());
            return 1;
        }
        std::cout << "[PASS] Empty list message\n";
    }

    // TC-DOC-05: Selected doctor details
    {
        std::string error;
        hms::Doctor doctor(
            "DOC05", "Dr. Test", "Cardiology", "Heart Department",
            "9876543210", "Mon-Fri 09:00-17:00");

        if (!fm.saveDoctor(doctor, error)) {
            std::cerr << "[FAIL] Could not save test doctor: " << error << '\n';
            std::remove(testCsv.c_str());
            return 1;
        }

        std::istringstream input("DOC05\n");
        std::ostringstream output;
        auto* oldIn = std::cin.rdbuf(input.rdbuf());
        auto* oldOut = std::cout.rdbuf(output.rdbuf());

        system.viewDoctorDetails();

        std::cin.rdbuf(oldIn);
        std::cout.rdbuf(oldOut);

        const std::string result = output.str();
        const std::string expected[] = {
            "Doctor ID: DOC05",
            "Name: Dr. Test",
            "Specialization: Cardiology",
            "Department: Heart Department",
            "Contact number: 9876543210",
            "Availability: Mon-Fri 09:00-17:00"
        };

        for (const auto& field : expected) {
            if (result.find(field) == std::string::npos) {
                std::cerr << "[FAIL] Missing field: " << field << '\n';
                std::remove(testCsv.c_str());
                return 1;
            }
        }
        std::cout << "[PASS] Selected doctor displays all six fields\n";
    }

    // TC-DOC-05: Multiple doctors appear exactly once
    {
        std::string error;
        hms::Doctor secondDoctor(
            "DOC06", "Dr. Second", "Neurology", "Brain Department",
            "9876543211", "Mon-Fri 10:00-18:00");

        if (!fm.saveDoctor(secondDoctor, error)) {
            std::cerr << "[FAIL] Could not save second test doctor: "
                      << error << '\n';
            std::remove(testCsv.c_str());
            return 1;
        }

        std::istringstream input("");
        std::ostringstream output;
        auto* oldIn = std::cin.rdbuf(input.rdbuf());
        auto* oldOut = std::cout.rdbuf(output.rdbuf());

        system.listDoctors();

        std::cin.rdbuf(oldIn);
        std::cout.rdbuf(oldOut);

        const std::string result = output.str();
        const std::string rows[] = {
            "DOC05 | Dr. Test | Cardiology | Heart Department",
            "DOC06 | Dr. Second | Neurology | Brain Department"
        };

        for (const auto& row : rows) {
            const auto first = result.find(row);
            if (first == std::string::npos ||
                result.find(row, first + row.size()) != std::string::npos) {
                std::cerr << "[FAIL] Missing or duplicated doctor row: "
                          << row << '\n';
                std::remove(testCsv.c_str());
                return 1;
            }
        }

        if (result.find("Doctor ID | Name | Specialization | Department")
            == std::string::npos) {
            std::cerr << "[FAIL] Required list columns are missing\n";
            std::remove(testCsv.c_str());
            return 1;
        }

        std::cout << "[PASS] Both doctors appear exactly once with required columns\n";
    }

    std::remove(testCsv.c_str());
    std::cout << "[PASS] TC-DOC-05 completed\n";
    return 0;
}
