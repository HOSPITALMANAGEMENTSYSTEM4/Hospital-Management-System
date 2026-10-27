
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
    const fs::path root = fs::current_path();
    const fs::path includeDir = root / "include" / "hms";
    const fs::path sourceDir = root / "src";

    const std::vector<std::string> headers = {
        "Patient.h",
        "Doctor.h",
        "Appointment.h",
        "HospitalSystem.h",
        "FileManager.h"
    };

    int failures = 0;

    for (const auto& header : headers) {
        if (!fs::exists(includeDir / header)) {
            std::cerr << "FAIL: Missing header " << header << '\n';
            ++failures;
        }
    }

    const std::vector<std::string> sourceFiles = {
        "Patient.cpp",
        "Doctor.cpp",
        "Appointment.cpp",
        "HospitalSystem.cpp"
    };

    for (const auto& file : sourceFiles) {
        std::ifstream input(sourceDir / file);

        if (!input) {
            std::cerr << "FAIL: Cannot read " << file << '\n';
            ++failures;
            continue;
        }

        std::string line;
        int lineNumber = 0;

        while (std::getline(input, line)) {
            ++lineNumber;

            if (line.find("ifstream") != std::string::npos ||
                line.find("ofstream") != std::string::npos ||
                line.find("fstream") != std::string::npos) {
                std::cerr << "FAIL: File-stream reference in "
                          << file << ':' << lineNumber << '\n';
                ++failures;
            }
        }
    }

    if (failures == 0) {
        std::cout << "PASS: Five headers exist; no file-stream "
                     "references found in the four other source files.\n";
        return 0;
    }

    std::cerr << failures << " architecture check(s) failed.\n";
    return 1;
}
