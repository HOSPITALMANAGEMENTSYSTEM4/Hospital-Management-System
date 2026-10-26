
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string &description, bool condition) {
    if (condition) {
        std::cout << "[PASS] " << description << '\n';
        ++passed;
    } else {
        std::cout << "[FAIL] " << description << '\n';
        ++failed;
    }
}

std::string runWithInput(hms::HospitalSystem &system,
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

std::size_t countOccurrences(const std::string &text,
                             const std::string &needle) {
    std::size_t count = 0;
    std::size_t pos = 0;

    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }

    return count;
}

} // namespace

int main() {
    const fs::path dataDir =
        fs::path("build") / "TC-UI-04-OperationOutcomeMessages";

    std::error_code ec;
    fs::remove_all(dataDir, ec);

    if (ec) {
        std::cerr << "[FAIL] Could not clean test directory: "
                  << ec.message() << '\n';
        return 1;
    }

    fs::create_directories(dataDir, ec);

    if (ec) {
        std::cerr << "[FAIL] Could not create test directory: "
                  << ec.message() << '\n';
        return 1;
    }

    {
        hms::HospitalSystem system(dataDir.string());

        // Patient search with no matching records.
        const std::string patientSearch =
            runWithInput(system, "1\n2\nUNKNOWN\n0\n4\n");

        check("Patient search reports a reason when no match exists",
              patientSearch.find("No matching patient found")
                  != std::string::npos);

        // Doctor search with no matching records.
        const std::string doctorSearch =
            runWithInput(system, "2\n2\nUNKNOWN\n0\n4\n");

        check("Doctor search reports a reason when no match exists",
              doctorSearch.find("No matching doctor found")
                  != std::string::npos);

        // Listing operations with empty data.
        const std::string patientList =
            runWithInput(system, "1\n3\n0\n4\n");

        check("Patient listing reports an empty result",
              patientList.find("No patients registered")
                  != std::string::npos);

        const std::string doctorList =
            runWithInput(system, "2\n3\n0\n4\n");

        check("Doctor listing reports an empty result",
              doctorList.find("No doctors registered")
                  != std::string::npos);

        // A failed cancellation must produce a meaningful reason.
        const std::string cancellation =
            runWithInput(system, "3\n3\nUNKNOWN\nY\n0\n4\n");

        check("Cancellation failure reports a reason",
              cancellation.find("Appointment not found")
                  != std::string::npos);
    }

    fs::remove_all(dataDir, ec);

    if (ec) {
        std::cerr << "[FAIL] Could not clean test directory: "
                  << ec.message() << '\n';
        ++failed;
    }

    std::cout << "\nTC-UI-04 results: " << passed
              << " passed, " << failed << " failed.\n";

    return failed == 0 ? 0 : 1;
}
