
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

bool runStartup(const fs::path& dataDir, std::string& outputText) {
    hms::HospitalSystem system(dataDir.string());

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

    outputText = output.str();

    return outputText.find("=== Hospital Management System ===")
               != std::string::npos &&
           outputText.find("Exiting Hospital Management System.")
               != std::string::npos;
}

void testAllFilesMissing(const fs::path& root) {
    const fs::path dir = root / "missing";
    fs::create_directories(dir);

    std::string output;
    check(runStartup(dir, output),
          "Startup succeeds when all CSV files are missing");
}

void testAllFilesEmpty(const fs::path& root) {
    const fs::path dir = root / "empty";
    fs::create_directories(dir);

    for (const char* name :
         {"patients.csv", "doctors.csv", "appointments.csv"}) {
        std::ofstream file(dir / name, std::ios::binary);
    }

    std::string output;
    check(runStartup(dir, output),
          "Startup succeeds when all CSV files are empty");
}

void testFiveMalformedLinesPerFile(const fs::path& root) {
    const fs::path dir = root / "malformed";
    fs::create_directories(dir);

    const char* names[] = {
        "patients.csv", "doctors.csv", "appointments.csv"
    };

    for (const char* name : names) {
        std::ofstream file(dir / name, std::ios::binary);

        // Five malformed lines in each CSV file.
        for (int i = 0; i < 5; ++i) {
            file << "malformed,line\n";
        }
    }

    std::string output;
    check(runStartup(dir, output),
          "Startup succeeds with five malformed lines per CSV file");
}

} // namespace

int main() {
    const fs::path root =
        fs::path("build") / "TC-NFR-02-RobustStartup";

    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);

    if (ec) {
        std::cerr << "[FAIL] Could not create test directory: "
                  << ec.message() << '\n';
        return 1;
    }

    try {
        testAllFilesMissing(root);
        testAllFilesEmpty(root);
        testFiveMalformedLinesPerFile(root);
    } catch (const std::exception& error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        fs::remove_all(root, ec);
        return 1;
    }

    fs::remove_all(root, ec);

    std::cout << "\nTC-NFR-02 results: "
              << (failures == 0 ? "PASS" : "FAIL") << '\n';

    return failures == 0 ? 0 : 1;
}
