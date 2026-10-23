
#include "hms/FileManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path testDir =
        fs::temp_directory_path() / "hms_tc_per_07";
    fs::remove_all(testDir);
    fs::create_directories(testDir);

    const fs::path patientPath = testDir / "patients.csv";
    const fs::path doctorPath = testDir / "doctors.csv";
    const fs::path appointmentPath = testDir / "appointments.csv";

    const std::string original =
        "patient_id,name,age,gender,contact,address,medical_history\n";

    {
        std::ofstream file(patientPath, std::ios::binary);
        file << original;
    }

    // Force a predictable failure: the parent path is a regular file,
    // so the FileManager cannot create a directory beneath it.
    const fs::path blockedParent = testDir / "not_a_directory";
    {
        std::ofstream file(blockedParent);
        file << "block";
    }

    const fs::path blockedPatientPath =
        blockedParent / "patients.csv";

    hms::FileManager fm(
        blockedPatientPath.string(),
        doctorPath.string(),
        appointmentPath.string());

    std::string errorMessage;
    const bool saved = fm.savePatients({}, errorMessage);

    check("TC-PER-07: Write failure is reported", !saved);
    check("TC-PER-07: Error message is provided",
          !errorMessage.empty());

    check("TC-PER-07: Existing patient CSV remains unchanged",
          readFile(patientPath) == original);

    fs::remove_all(testDir);

    std::cout << "\nTC-PER-07 results: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
