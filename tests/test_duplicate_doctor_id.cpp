
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void check(const std::string &description, bool passed) {
  std::cout << (passed ? "[PASS] " : "[FAIL] ") << description << '\n';

  if (!passed) {
    ++failures;
  }
}

std::string readFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}

std::size_t countDataRows(const std::string &path) {
  std::ifstream file(path);
  std::string line;
  std::size_t count = 0;

  // Skip the CSV header.
  if (!std::getline(file, line)) {
    return 0;
  }

  while (std::getline(file, line)) {
    if (!line.empty()) {
      ++count;
    }
  }

  return count;
}

hms::Doctor makeDoctor(const std::string &id, const std::string &name) {
  return hms::Doctor(id, name, "Cardiology", "Cardiology Dept", "9876543210",
                     "Mon-Fri 09:00-17:00");
}

} // namespace

int main() {
  const std::string csvPath = "build/test_duplicate_doctor_id.csv";

  std::remove(csvPath.c_str());

  hms::FileManager fileManager("data/patients.csv", csvPath);

  std::string error;

  // Arrange: save an existing doctor.
  const hms::Doctor existing = makeDoctor("DOC7001", "Dr. Existing");

  check("Initial doctor can be saved", fileManager.saveDoctor(existing, error));

  const std::string originalContents = readFile(csvPath);
  const std::size_t originalCount = countDataRows(csvPath);

  // Act: attempt to register the same ID through the
  // application workflow.
  hms::HospitalSystem system(fileManager);

  std::istringstream input("DOC7001\n"
                           "Dr. Duplicate\n"
                           "Cardiology\n"
                           "Cardiology Dept\n"
                           "9876543210\n"
                           "Mon-Fri 09:00-17:00\n");

  std::ostringstream output;

  auto *originalInput = std::cin.rdbuf(input.rdbuf());
  auto *originalOutput = std::cout.rdbuf(output.rdbuf());

  system.addDoctor();

  std::cin.rdbuf(originalInput);
  std::cout.rdbuf(originalOutput);

  // Assert: required message, no extra record, unchanged file.
  const std::string expectedMessage = "Doctor ID already exists";
  const std::string actualOutput = output.str();

  check("Duplicate registration displays exact required message",
        actualOutput.find(expectedMessage) != std::string::npos);

  check("Doctor count remains unchanged",
        countDataRows(csvPath) == originalCount);

  check("CSV contents remain byte-for-byte unchanged",
        readFile(csvPath) == originalContents);

  // Also verify case-insensitive duplicate rejection at the
  // persistence layer.
  error.clear();

  const hms::Doctor caseVariant = makeDoctor("doc7001", "Dr. Case Variant");

  check("Case-insensitive duplicate is rejected by FileManager",
        !fileManager.saveDoctor(caseVariant, error));

  check("Case-insensitive duplicate does not modify the CSV",
        readFile(csvPath) == originalContents);

  std::remove(csvPath.c_str());

  std::cout << "\nFailures: " << failures << '\n';

  return failures == 0 ? 0 : 1;
}
