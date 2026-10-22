
#include "hms/HospitalSystem.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

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

void testMissingFiles(const std::string &scenario,
                      bool omitPatients,
                      bool omitDoctors,
                      bool omitAppointments) {
    const fs::path dataDir =
        fs::path("build") / ("TC-PER-02-" + scenario);

    std::error_code ec;
    fs::remove_all(dataDir, ec);
    ec.clear();
    fs::create_directories(dataDir, ec);

    if (ec) {
        check(false, scenario + ": temporary directory created");
        return;
    }

    // Files are deliberately not created for the selected missing cases.
    // Create empty files for the other cases so only the requested files
    // are missing.
    auto createFile = [](const fs::path &path) {
        std::ofstream file(path);
        return file.good();
    };

    bool setupOk = true;

    if (!omitPatients) {
        setupOk = createFile(dataDir / "patients.csv") && setupOk;
    }
    if (!omitDoctors) {
        setupOk = createFile(dataDir / "doctors.csv") && setupOk;
    }
    if (!omitAppointments) {
        setupOk = createFile(dataDir / "appointments.csv") && setupOk;
    }

    if (!setupOk) {
        check(false, scenario + ": test files prepared");
        fs::remove_all(dataDir, ec);
        return;
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

    check(displayed.find("=== Hospital Management System ===")
              != std::string::npos,
          scenario + ": main menu appears");

    check(displayed.find("Enter your choice:")
              != std::string::npos,
          scenario + ": menu accepts input");

    check(displayed.find("Exiting Hospital Management System.")
              != std::string::npos,
          scenario + ": application exits normally after choice 0");

    check(displayed.find("Records Loaded at Startup")
              != std::string::npos,
          scenario + ": startup records section appears");

    fs::remove_all(dataDir, ec);
}

} // namespace

int main() {
    testMissingFiles("AllMissing", true, true, true);
    testMissingFiles("PatientsMissing", true, false, false);
    testMissingFiles("DoctorsMissing", false, true, false);
    testMissingFiles("AppointmentsMissing", false, false, true);

    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All Issue #16 startup scenarios passed.\n";
    return 0;
}
