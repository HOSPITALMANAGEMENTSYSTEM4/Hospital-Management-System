
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <exception>
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

void writeTestPatients(const std::string &path) {
    hms::FileManager fileManager(path, "build/test_search_doctors.csv");

    const std::vector<hms::Patient> patients = {
        {"P101", "Anita Sharma", 21, "F", "9876543210",
         "Bangalore", "None"},
        {"P102", "Anita Rao", 22, "F", "9876543211",
         "Mysore", "Asthma"},
        {"P103", "Rahul Kumar", 23, "M", "9876543212",
         "Mangalore", "None"}
    };

    for (const auto &patient : patients) {
        std::string errorMessage;

        if (!fileManager.savePatient(patient, errorMessage)) {
            throw std::runtime_error(
                "Could not save test patient: " + errorMessage);
        }
    }
}

std::string runSearch(const std::string &path,
                      const std::string &query) {
    hms::HospitalSystem system{
        hms::FileManager(path, "build/test_search_doctors.csv")};

    // Select menu option 3, enter the search query, then exit.
    std::istringstream input("3\n" + query + "\n0\n");
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

void prepareTestFile(const std::string &path) {
    std::filesystem::remove(path);
    writeTestPatients(path);
}

void testExactPatientId() {
    const std::string path = "build/test_search_exact.csv";
    prepareTestFile(path);

    const std::string output = runSearch(path, "P102");

    check(output.find("Anita Rao") != std::string::npos,
          "Exact patient ID displays the matching patient");

    check(output.find("Anita Sharma") == std::string::npos &&
          output.find("Rahul Kumar") == std::string::npos,
          "Exact patient ID displays only one patient");

    std::filesystem::remove(path);
}

void testCaseInsensitivePartialName() {
    const std::string path = "build/test_search_partial.csv";
    prepareTestFile(path);

    const std::string output = runSearch(path, "aNiTa");

    check(output.find("Anita Sharma") != std::string::npos &&
          output.find("Anita Rao") != std::string::npos,
          "Case-insensitive partial name search returns all matches");

    check(output.find("Rahul Kumar") == std::string::npos,
          "Partial name search excludes non-matching patients");

    std::filesystem::remove(path);
}

void testSearchTrimsWhitespace() {
    const std::string path = "build/test_search_whitespace.csv";

    std::filesystem::remove(path);
    writeTestPatients(path);

    const std::string idOutput = runSearch(path, "  P102  ");

    check(
        idOutput.find("Anita Rao") != std::string::npos,
        "Patient ID search trims leading and trailing spaces");

    check(
        idOutput.find("Anita Sharma") == std::string::npos &&
        idOutput.find("Rahul Kumar") == std::string::npos,
        "Whitespace-padded exact ID returns only the matching patient");

    const std::string nameOutput = runSearch(path, "  aNiTa  ");

    check(
        nameOutput.find("Anita Sharma") != std::string::npos &&
        nameOutput.find("Anita Rao") != std::string::npos,
        "Whitespace-padded partial name search returns all case-insensitive matches");

    check(
        nameOutput.find("Rahul Kumar") == std::string::npos,
        "Whitespace-padded name search excludes non-matching patients");

    std::filesystem::remove(path);
}

void testNoMatchingPatient() {
    const std::string path = "build/test_search_unknown.csv";
    prepareTestFile(path);

    const std::string idOutput = runSearch(path, "P999");

    check(idOutput.find("No matching patient found") != std::string::npos,
          "Unknown patient ID displays the no-match message");

    const std::string nameOutput = runSearch(path, "Unknown Person");

    check(nameOutput.find("No matching patient found") != std::string::npos,
          "Unknown patient name displays the no-match message");

    std::filesystem::remove(path);
}

void testWhitespaceAroundQuery() {
    const std::string path = "build/test_search_whitespace.csv";
    prepareTestFile(path);

    const std::string output = runSearch(path, "  P102  ");

    check(output.find("Anita Rao") != std::string::npos,
          "Leading and trailing spaces are trimmed from the search query");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    std::filesystem::create_directories("build");

    try {
        testExactPatientId();
        testCaseInsensitivePartialName();
        testSearchTrimsWhitespace();
        testNoMatchingPatient();
        testWhitespaceAroundQuery();
    } catch (const std::exception &error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All patient search tests passed.\n";
    return 0;
}
