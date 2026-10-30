#include "hms/FileManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
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

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path testDir =
        fs::temp_directory_path() / "hms_tc_sec_09";

    std::error_code cleanupError;
    fs::remove_all(testDir, cleanupError);
    fs::create_directories(testDir);

    const fs::path blockedParent = testDir / "not_a_directory";

    {
        std::ofstream blocker(blockedParent);
        blocker << "block";
    }

    const fs::path patientPath = blockedParent / "patients.csv";
    const fs::path doctorPath = testDir / "doctors.csv";
    const fs::path appointmentPath = testDir / "appointments.csv";

    hms::FileManager fileManager(
        patientPath.string(),
        doctorPath.string(),
        appointmentPath.string());

    std::string errorMessage;
    const bool saved = fileManager.savePatients({}, errorMessage);

    check("TC-SEC-09: The forced filesystem operation fails",
          !saved);

    check("TC-SEC-09: A useful error message is provided",
          !errorMessage.empty());

    check("TC-SEC-09: The error message is generic",
          errorMessage ==
              "Could not save data due to a filesystem error.");

    check("TC-SEC-09: The full filesystem path is not exposed",
          errorMessage.find(patientPath.string()) ==
              std::string::npos &&
          errorMessage.find(testDir.string()) ==
              std::string::npos);

    check("TC-SEC-09: The blocked directory name is not exposed",
          errorMessage.find("not_a_directory") ==
              std::string::npos);

    check("TC-SEC-09: Raw blocker contents are not exposed",
          errorMessage.find("block") ==
              std::string::npos);

    fs::remove_all(testDir, cleanupError);

    std::cout << "\nTC-SEC-09 results: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
