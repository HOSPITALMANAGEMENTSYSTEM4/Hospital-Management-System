#include "hms/Doctor.h"
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

void writeTestDoctors(const std::string &doctorPath) {
  hms::FileManager fileManager("build/test_search_doc_patients.csv", doctorPath);

  const std::vector<hms::Doctor> doctors = {
      {"DOC101", "Dr. Alice Smith", "Cardiology", "Cardiology Dept",
       "9876543210", "Mon-Fri 09:00-17:00"},
      {"DOC102", "Dr. Bob Jones", "Neurology", "Neurology Dept",
       "9876543211", "Tue-Thu 10:00-16:00"},
      {"DOC103", "Dr. Charlie Brown", "Pediatrics", "Pediatrics Dept",
       "9876543212", "Mon-Wed 08:00-14:00"}};

  for (const auto &doctor : doctors) {
    std::string errorMessage;
    if (!fileManager.saveDoctor(doctor, errorMessage)) {
      throw std::runtime_error("Could not save test doctor: " + errorMessage);
    }
  }
}

std::string runSearch(const std::string &doctorPath, const std::string &query) {
  hms::HospitalSystem system{
      hms::FileManager("build/test_search_doc_patients.csv", doctorPath)};

  // Select menu option 8 ("Search doctors"), enter query, then exit (0).
  std::istringstream input("8\n" + query + "\n0\n");
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
  writeTestDoctors(path);
}

void testExactDoctorId() {
  const std::string path = "build/test_search_doc_exact.csv";
  prepareTestFile(path);

  // Exact ID matching case-sensitive and case-insensitive
  const std::string outputUpper = runSearch(path, "DOC102");
  check(outputUpper.find("Dr. Bob Jones") != std::string::npos,
        "Exact doctor ID (DOC102) displays matching doctor");
  check(outputUpper.find("Dr. Alice Smith") == std::string::npos &&
            outputUpper.find("Dr. Charlie Brown") == std::string::npos,
        "Exact doctor ID displays only the matching doctor");

  const std::string outputLower = runSearch(path, "doc101");
  check(outputLower.find("Dr. Alice Smith") != std::string::npos,
        "Case-insensitive exact doctor ID (doc101) displays matching doctor");
  check(outputLower.find("Dr. Bob Jones") == std::string::npos &&
            outputLower.find("Dr. Charlie Brown") == std::string::npos,
        "Case-insensitive exact doctor ID excludes non-matching doctors");

  std::filesystem::remove(path);
}

void testPartialName() {
  const std::string path = "build/test_search_doc_name.csv";
  prepareTestFile(path);

  // Partial name, case-insensitive
  const std::string output = runSearch(path, "cHaRlIe");
  check(output.find("Dr. Charlie Brown") != std::string::npos,
        "Case-insensitive partial name search returns matching doctor");
  check(output.find("Dr. Alice Smith") == std::string::npos &&
            output.find("Dr. Bob Jones") == std::string::npos,
        "Partial name search excludes non-matching doctors");

  // Query matching multiple doctors
  const std::string multiOutput = runSearch(path, "Dr.");
  check(multiOutput.find("Dr. Alice Smith") != std::string::npos &&
            multiOutput.find("Dr. Bob Jones") != std::string::npos &&
            multiOutput.find("Dr. Charlie Brown") != std::string::npos,
        "Partial name matching multiple doctors returns all matches");

  std::filesystem::remove(path);
}

void testPartialSpecialization() {
  const std::string path = "build/test_search_doc_spec.csv";
  prepareTestFile(path);

  // Partial specialization: "neuro"
  const std::string neuroOutput = runSearch(path, "neuro");
  check(neuroOutput.find("Dr. Bob Jones") != std::string::npos,
        "Case-insensitive partial specialization (neuro) matches doctor");
  check(neuroOutput.find("Neurology") != std::string::npos,
        "Specialization field is displayed");
  check(neuroOutput.find("Dr. Alice Smith") == std::string::npos &&
            neuroOutput.find("Dr. Charlie Brown") == std::string::npos,
        "Partial specialization excludes non-matching doctors");

  // Partial specialization: "CARDIO"
  const std::string cardioOutput = runSearch(path, "CARDIO");
  check(cardioOutput.find("Dr. Alice Smith") != std::string::npos,
        "Uppercase partial specialization (CARDIO) matches doctor");
  check(cardioOutput.find("Cardiology") != std::string::npos,
        "Cardiology displayed in result");
  check(cardioOutput.find("Dr. Bob Jones") == std::string::npos,
        "Cardiology search excludes other doctors");

  std::filesystem::remove(path);
}

void testExactIdPriority() {
  const std::string path = "build/test_search_doc_priority.csv";
  std::filesystem::remove(path);

  hms::FileManager fileManager("build/test_search_doc_patients.csv", path);
  std::string err;

  // Doctor 1 has ID DOC201
  fileManager.saveDoctor(
      {"DOC201", "Dr. Primary", "General", "General Dept", "9876543210",
       "Mon 09:00-17:00"},
      err);
  // Doctor 2 has name containing DOC201
  fileManager.saveDoctor(
      {"DOC202", "Dr. DOC201 Secondary", "Pediatrics", "Pediatrics Dept",
       "9876543211", "Tue 09:00-17:00"},
      err);

  const std::string output = runSearch(path, "DOC201");
  check(output.find("Dr. Primary") != std::string::npos,
        "Exact ID match displays doctor with exact ID");
  check(output.find("Dr. DOC201 Secondary") == std::string::npos,
        "Exact ID match takes priority over partial name match");

  std::filesystem::remove(path);
}

void testNoMatchingDoctor() {
  const std::string path = "build/test_search_doc_none.csv";
  prepareTestFile(path);

  const std::string unknownIdOutput = runSearch(path, "DOC999");
  check(unknownIdOutput.find("No matching doctor found") != std::string::npos,
        "Unknown doctor ID displays 'No matching doctor found'");
  check(unknownIdOutput.find("Dr.") == std::string::npos,
        "Unknown doctor ID displays no doctor records");

  const std::string unknownNameOutput = runSearch(path, "Nonexistent Name");
  check(unknownNameOutput.find("No matching doctor found") != std::string::npos,
        "Unknown doctor name displays 'No matching doctor found'");

  const std::string unknownSpecOutput = runSearch(path, "Oncology");
  check(unknownSpecOutput.find("No matching doctor found") != std::string::npos,
        "Unknown specialization displays 'No matching doctor found'");

  std::filesystem::remove(path);
}

void testSearchTrimsWhitespace() {
  const std::string path = "build/test_search_doc_trim.csv";
  prepareTestFile(path);

  const std::string paddedIdOutput = runSearch(path, "   DOC102   ");
  check(paddedIdOutput.find("Dr. Bob Jones") != std::string::npos,
        "Whitespace-padded exact ID search trims whitespace and finds doctor");

  const std::string paddedSpecOutput = runSearch(path, "   pediatric   ");
  check(paddedSpecOutput.find("Dr. Charlie Brown") != std::string::npos,
        "Whitespace-padded specialization search trims whitespace and finds doctor");

  std::filesystem::remove(path);
}

void testDoctorDisplayFields() {
  const std::string path = "build/test_search_doc_fields.csv";
  prepareTestFile(path);

  const std::string output = runSearch(path, "DOC101");
  check(output.find("Doctor ID: DOC101") != std::string::npos,
        "Displays Doctor ID");
  check(output.find("Name: Dr. Alice Smith") != std::string::npos,
        "Displays Name");
  check(output.find("Specialization: Cardiology") != std::string::npos,
        "Displays Specialization");
  check(output.find("Department: Cardiology Dept") != std::string::npos,
        "Displays Department");
  check(output.find("Contact number: 9876543210") != std::string::npos,
        "Displays Contact number");
  check(output.find("Availability: Mon-Fri 09:00-17:00") != std::string::npos,
        "Displays Availability");

  std::filesystem::remove(path);
}

} // namespace

int main() {
  std::filesystem::create_directories("build");

  std::cout << "=== Running TC-DOC-04_SearchDoctors Tests ===\n\n";

  try {
    testExactDoctorId();
    testPartialName();
    testPartialSpecialization();
    testExactIdPriority();
    testNoMatchingDoctor();
    testSearchTrimsWhitespace();
    testDoctorDisplayFields();
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] Exception: " << e.what() << '\n';
    return 1;
  }

  if (failures != 0) {
    std::cerr << failures << " test(s) failed.\n";
    return 1;
  }

  std::cout << "\nAll doctor search tests passed successfully.\n";
  return 0;
}
