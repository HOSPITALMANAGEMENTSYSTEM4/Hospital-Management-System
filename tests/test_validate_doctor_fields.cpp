#include "hms/Doctor.h"
#include "hms/FileManager.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int passed = 0;
int failed = 0;

void check(const char *name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << '\n';
        ++passed;
    } else {
        std::cout << "  [FAIL] " << name << '\n';
        ++failed;
    }
}

hms::Doctor makeDoctor(
    const std::string &id = "DOC1001",
    const std::string &name = "Dr. Alice",
    const std::string &specialization = "Cardiology",
    const std::string &department = "Cardiology Dept",
    const std::string &contact = "9876543210",
    const std::string &availability = "Mon-Fri 09:00-17:00") {
    return hms::Doctor(id, name, specialization, department,
                       contact, availability);
}

bool valid(const hms::Doctor &doctor) {
    std::string error;
    return doctor.validate(error);
}

bool errorContains(const hms::Doctor &doctor,
                   const std::string &expected) {
    std::string error;
    return !doctor.validate(error) &&
           error.find(expected) != std::string::npos;
}

std::string readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

int countDataRows(const std::string &path) {
    std::ifstream file(path);
    std::string line;
    int count = 0;

    while (std::getline(file, line)) {
        if (!line.empty() && line.rfind("doctor_id", 0) != 0) {
            ++count;
        }
    }

    return count;
}

} // namespace

int main() {
    const std::string csvPath = "build/test_tc_doc_03_doctors.csv";
    std::remove(csvPath.c_str());

    std::cout << "=== TC-DOC-03: Validate Doctor Fields ===\n";

    // Valid values at the documented length boundaries.
    std::cout << "\n[Valid boundaries]\n";
    check("Doctor ID length 1 accepted", valid(makeDoctor("D")));
    check("Doctor ID length 10 accepted",
          valid(makeDoctor("D123456789")));
    check("Name length 1 accepted",
          valid(makeDoctor("DOC1", "A")));
    check("Name length 50 accepted",
          valid(makeDoctor("DOC1", std::string(50, 'A'))));
    check("Specialization length 1 accepted",
          valid(makeDoctor("DOC1", "A", "S")));
    check("Specialization length 40 accepted",
          valid(makeDoctor("DOC1", "A", std::string(40, 'S'))));
    check("Department length 1 accepted",
          valid(makeDoctor("DOC1", "A", "S", "D")));
    check("Department length 40 accepted",
          valid(makeDoctor("DOC1", "A", "S", std::string(40, 'D'))));
    check("Contact with exactly 10 digits accepted", valid(makeDoctor()));
    check("Availability length 1 accepted",
          valid(makeDoctor("DOC1", "A", "S", "D", "9876543210", "M")));
    check("Availability length 60 accepted",
          valid(makeDoctor("DOC1", "A", "S", "D", "9876543210",
                           std::string(60, 'M'))));

    // Invalid values must identify the field that failed validation.
    std::cout << "\n[Invalid fields and field-specific errors]\n";
    check("Empty doctor ID rejected",
          errorContains(makeDoctor(""), "Doctor ID"));
    check("Doctor ID over 10 characters rejected",
          errorContains(makeDoctor("DOC12345678"), "Doctor ID"));
    check("Empty name rejected",
          errorContains(makeDoctor("DOC1", ""), "Name"));
    check("Name over 50 characters rejected",
          errorContains(makeDoctor("DOC1", std::string(51, 'A')), "Name"));
    check("Name containing a digit rejected",
          errorContains(makeDoctor("DOC1", "Alice2"), "Name"));
    check("Empty specialization rejected",
          errorContains(makeDoctor("DOC1", "Alice", ""), "Specialization"));
    check("Specialization over 40 characters rejected",
          errorContains(makeDoctor("DOC1", "Alice", std::string(41, 'S')),
                        "Specialization"));
    check("Empty department rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", ""), "Department"));
    check("Department over 40 characters rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", std::string(41, 'D')),
                        "Department"));
    check("Nine-digit contact rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", "D", "123456789"),
                        "Contact number"));
    check("Eleven-digit contact rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", "D", "12345678901"),
                        "Contact number"));
    check("Contact containing a letter rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", "D", "123456789A"),
                        "Contact number"));
    check("Empty availability rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", "D",
                                   "9876543210", ""),
                        "Availability"));
    check("Availability over 60 characters rejected",
          errorContains(makeDoctor("DOC1", "Alice", "S", "D",
                                   "9876543210", std::string(61, 'M')),
                        "Availability"));

    // CSV safety rules.
    std::cout << "\n[Security rules]\n";
    check("Comma in name rejected",
          !valid(makeDoctor("DOC1", "Alice,Smith")));
    check("Newline in specialization rejected",
          !valid(makeDoctor("DOC1", "Alice", "Cardiology\nWard")));
    check("Carriage return in department rejected",
          !valid(makeDoctor("DOC1", "Alice", "S", "Ward\rB")));
    check("Formula prefix in availability rejected",
          !valid(makeDoctor("DOC1", "Alice", "S", "D",
                            "9876543210", "=formula")));

    // Invalid data must not be persisted, and an existing file must be
    // unchanged when saveDoctor rejects an invalid record.
    std::cout << "\n[Persistence protection]\n";
    hms::FileManager fileManager("data/patients.csv", csvPath);
    std::string error;

    const bool saved = fileManager.saveDoctor(makeDoctor(), error);
    check("Valid doctor can be saved", saved);
    check("Successful save leaves no error", saved && error.empty());

    const std::string before = readFile(csvPath);
    const int rowsBefore = countDataRows(csvPath);

    error.clear();
    const bool invalidSaved =
        fileManager.saveDoctor(makeDoctor("DOC2", "Invalid2"), error);

    check("Invalid doctor is not saved", !invalidSaved);
    check("Invalid save returns an error", !error.empty());
    check("CSV content unchanged after invalid save",
          readFile(csvPath) == before);
    check("CSV row count unchanged after invalid save",
          countDataRows(csvPath) == rowsBefore);

    std::remove(csvPath.c_str());

    std::cout << "\nResults: " << passed << " passed, "
              << failed << " failed.\n";

    return failed == 0 ? 0 : 1;
}
