// TC-DOC-01  Add Doctor (HMS-F-008)
//
// Verifies:
//   - Doctor entity field validation (VR-01, VR-02, VR-05, VR-08, VR-09, VR-14)
//   - Successful doctor registration and persistence to doctors.csv
//   - Exactly one record added per registration
//   - Duplicate doctor ID rejection (case-insensitive, BR-02, HMS-F-009)
//   - No partial or corrupted writes on invalid inputs
//   - Graceful handling of missing files, malformed records, and write errors
//   - HospitalSystem interactive add-doctor workflow and console messages
//
// Self-contained test binary following the project's CTest harness conventions.

#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void check(const char *name, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << name << '\n';
    ++g_passed;
  } else {
    std::cout << "  [FAIL] " << name << '\n';
    ++g_failed;
  }
}

// ---------------------------------------------------------------------------
// File helpers for isolated test CSVs
// ---------------------------------------------------------------------------

std::string readFile(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return "";
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

int countDataRows(const std::string &path) {
  std::ifstream f(path);
  if (!f) return 0;
  int count = 0;
  std::string line;
  while (std::getline(f, line)) {
    if (!line.empty() && line.rfind("doctor_id", 0) != 0 && line.rfind("DoctorID", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::remove(path.c_str());
}

// ---------------------------------------------------------------------------
// Doctor factory for baseline valid doctor
// ---------------------------------------------------------------------------

hms::Doctor makeDoctor(const std::string &id = "DOC1001",
                       const std::string &name = "Dr. Gregory House",
                       const std::string &specialization = "Diagnostic Medicine",
                       const std::string &department = "Infectious Disease",
                       const std::string &contact = "9876543210",
                       const std::string &availability = "Mon-Fri 09:00-17:00") {
  return hms::Doctor(id, name, specialization, department, contact, availability);
}

bool isValidDoc(const hms::Doctor &d) {
  std::string err;
  return d.validate(err);
}

bool errorContains(const hms::Doctor &d, const std::string &expectedSubstring) {
  std::string err;
  if (d.validate(err)) return false;
  return err.find(expectedSubstring) != std::string::npos;
}

} // namespace

int main() {
  std::cout << "=== Running TC-DOC-01 / HMS-F-008 Add Doctor Tests ===\n\n";

  // =========================================================================
  // Section 1: Doctor Field Validation Rules (VR-01, VR-02, VR-05, VR-08, VR-09, VR-14)
  // =========================================================================
  std::cout << "-- 1. Unit Validation Rules --\n";

  // Baseline valid doctor
  {
    hms::Doctor d = makeDoctor();
    check("Baseline doctor is valid", isValidDoc(d));
    check("Doctor ID getter", d.getId() == "DOC1001");
    check("Name getter", d.getName() == "Dr. Gregory House");
    check("Specialization getter", d.getSpecialization() == "Diagnostic Medicine");
    check("Department getter", d.getDepartment() == "Infectious Disease");
    check("Contact number getter", d.getContactNumber() == "9876543210");
    check("Availability getter", d.getAvailability() == "Mon-Fri 09:00-17:00");
  }

  // VR-01: Doctor ID boundaries & rules
  {
    check("VR-01: Min length 1 char (D)", isValidDoc(makeDoctor("D")));
    check("VR-01: Max length 10 chars (D123456789)", isValidDoc(makeDoctor("D123456789")));
    check("VR-01: Normalised to uppercase", makeDoctor("doc001").getId() == "DOC001");
    check("VR-01: Below min (empty) rejected", !isValidDoc(makeDoctor("")));
    check("VR-01: Empty error message", errorContains(makeDoctor(""), "Doctor ID: must contain 1-10"));
    check("VR-01: Above max (11 chars) rejected", !isValidDoc(makeDoctor("DOC12345678")));
    check("VR-01: Non-alphanumeric hyphen rejected", !isValidDoc(makeDoctor("DOC-01")));
    check("VR-01: Non-alphanumeric space rejected", !isValidDoc(makeDoctor("DOC 01")));
    check("VR-01: Non-alphanumeric symbol rejected", !isValidDoc(makeDoctor("DOC@01")));
    check("VR-14: Comma in ID rejected", !isValidDoc(makeDoctor("DOC,01")));
    check("VR-14: Newline in ID rejected", !isValidDoc(makeDoctor("DOC\n01")));
    check("VR-14: Leading = in ID rejected", !isValidDoc(makeDoctor("=DOC1")));
    check("VR-14: Leading + in ID rejected", !isValidDoc(makeDoctor("+DOC1")));
    check("VR-14: Leading - in ID rejected", !isValidDoc(makeDoctor("-DOC1")));
    check("VR-14: Leading @ in ID rejected", !isValidDoc(makeDoctor("@DOC1")));
  }

  // VR-02: Name boundaries & rules
  {
    check("VR-02: Min length 1 char", isValidDoc(makeDoctor("DOC1", "A")));
    check("VR-02: Max length 50 chars", isValidDoc(makeDoctor("DOC1", std::string(50, 'A'))));
    check("VR-02: Below min (empty) rejected", !isValidDoc(makeDoctor("DOC1", "")));
    check("VR-02: Above max (51 chars) rejected", !isValidDoc(makeDoctor("DOC1", std::string(51, 'A'))));
    check("VR-02: Name with period valid (Dr. Smith)", isValidDoc(makeDoctor("DOC1", "Dr. Smith")));
    check("VR-02: Name with hyphen valid (Mary-Jane)", isValidDoc(makeDoctor("DOC1", "Mary-Jane")));
    check("VR-02: Name with apostrophe valid (O'Connor)", isValidDoc(makeDoctor("DOC1", "O'Connor")));
    check("VR-02: Digits in name rejected", !isValidDoc(makeDoctor("DOC1", "John 2nd")));
    check("VR-02: Symbols in name rejected", !isValidDoc(makeDoctor("DOC1", "Dr. John!")));
    check("VR-14: Comma in name rejected", !isValidDoc(makeDoctor("DOC1", "Smith, John")));
    check("VR-14: Newline in name rejected", !isValidDoc(makeDoctor("DOC1", "John\nSmith")));
    check("VR-14: Leading = in name rejected", !isValidDoc(makeDoctor("DOC1", "=Smith")));
    check("VR-14: Leading + in name rejected", !isValidDoc(makeDoctor("DOC1", "+Smith")));
    check("VR-14: Leading - in name rejected", !isValidDoc(makeDoctor("DOC1", "-Smith")));
    check("VR-14: Leading @ in name rejected", !isValidDoc(makeDoctor("DOC1", "@Smith")));
  }

  // VR-08: Specialization boundaries & rules
  {
    check("VR-08: Spec min length 1 char", isValidDoc(makeDoctor("DOC1", "Dr. A", "C")));
    check("VR-08: Spec max length 40 chars", isValidDoc(makeDoctor("DOC1", "Dr. A", std::string(40, 'S'))));
    check("VR-08: Spec empty rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "")));
    check("VR-08: Spec above max (41 chars) rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", std::string(41, 'S'))));
    check("VR-14: Comma in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology, Surgery")));
    check("VR-14: Newline in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology\nDept")));
    check("VR-14: Leading = in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "=Cardiology")));
    check("VR-14: Leading + in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "+Cardiology")));
    check("VR-14: Leading - in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "-Cardiology")));
    check("VR-14: Leading @ in specialization rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "@Cardiology")));
  }

  // VR-08: Department boundaries & rules
  {
    check("VR-08: Dept min length 1 char", isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "D")));
    check("VR-08: Dept max length 40 chars", isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", std::string(40, 'D'))));
    check("VR-08: Dept empty rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "")));
    check("VR-08: Dept above max (41 chars) rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", std::string(41, 'D'))));
    check("VR-14: Comma in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Ward A, B")));
    check("VR-14: Newline in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Ward\nB")));
    check("VR-14: Leading = in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "=Ward")));
    check("VR-14: Leading + in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "+Ward")));
    check("VR-14: Leading - in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "-Ward")));
    check("VR-14: Leading @ in department rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "@Ward")));
  }

  // VR-05: Contact number boundaries & rules
  {
    check("VR-05: Exactly 10 digits valid", isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "9876543210")));
    check("VR-05: 9 digits rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "987654321")));
    check("VR-05: 11 digits rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "98765432100")));
    check("VR-05: Non-digits rejected (letters)", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "987654321A")));
    check("VR-05: Non-digits rejected (spaces)", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "987 654321")));
    check("VR-05: Non-digits rejected (hyphens)", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "987-654321")));
    check("VR-14: Leading + rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Cardiology", "Dept", "+987654321")));
  }

  // VR-09: Availability boundaries & rules
  {
    check("VR-09: Min length 1 char", isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "M")));
    check("VR-09: Max length 60 chars", isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", std::string(60, 'M'))));
    check("VR-09: Empty availability rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "")));
    check("VR-09: Above max (61 chars) rejected", !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", std::string(61, 'M'))));
    check("VR-09: Standard text format valid (Mon-Fri 09:00-17:00)",
          isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "Mon-Fri 09:00-17:00")));
    check("VR-09: Supported delimiters valid (colons, hyphens, slashes, semicolons)",
          isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "Mon/Wed 08:00-12:00; Sat 10:00-13:00")));
    check("VR-14: Comma in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "Mon, Wed, Fri 09:00-17:00")));
    check("VR-14: Newline in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "Mon-Fri\n09:00-17:00")));
    check("VR-14: Leading = in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "=AllDay")));
    check("VR-14: Leading + in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "+Daily")));
    check("VR-14: Leading - in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "-Mon-Fri")));
    check("VR-14: Leading @ in availability rejected",
          !isValidDoc(makeDoctor("DOC1", "Dr. A", "Spec", "Dept", "9876543210", "@Clinic")));
  }

  // =========================================================================
  // Section 2: Persistence & FileManager (HMS-F-008, HMS-F-009, HMS-F-026, HMS-F-029)
  // =========================================================================
  std::cout << "\n-- 2. Persistence & FileManager Integration --\n";

  const std::string testDoctorCsv = "build/test_isolated_doctors.csv";
  removeFile(testDoctorCsv);

  hms::FileManager fm("data/patients.csv", testDoctorCsv);

  // Missing file handling (BR-09 / HMS-F-026)
  {
    check("Missing CSV loads as empty vector", fm.loadDoctors().empty());
    check("doctorIdExists on missing file returns false", !fm.doctorIdExists("DOC1001"));
  }

  // Save first valid doctor
  {
    std::string err;
    hms::Doctor doc1 = makeDoctor("DOC1001", "Dr. Alice", "Cardiology", "Cardiology Dept", "9876543210", "Mon-Fri 09:00-17:00");
    bool saved = fm.saveDoctor(doc1, err);
    check("First doctor saved successfully", saved);
    check("Error message empty on success", err.empty());
    check("Exactly 1 data row in CSV", countDataRows(testDoctorCsv) == 1);
    check("doctorIdExists returns true for DOC1001", fm.doctorIdExists("DOC1001"));
    check("doctorIdExists is case-insensitive (doc1001)", fm.doctorIdExists("doc1001"));

    auto loaded = fm.loadDoctors();
    check("loadDoctors count is 1", loaded.size() == 1);
    if (!loaded.empty()) {
      check("Loaded doctor ID matches", loaded[0].getId() == "DOC1001");
      check("Loaded doctor name matches", loaded[0].getName() == "Dr. Alice");
      check("Loaded specialization matches", loaded[0].getSpecialization() == "Cardiology");
      check("Loaded department matches", loaded[0].getDepartment() == "Cardiology Dept");
      check("Loaded contact matches", loaded[0].getContactNumber() == "9876543210");
      check("Loaded availability matches", loaded[0].getAvailability() == "Mon-Fri 09:00-17:00");
    }
  }

  // Save second distinct doctor — preserves existing record (no truncation)
  {
    std::string err;
    hms::Doctor doc2 = makeDoctor("DOC1002", "Dr. Bob", "Neurology", "Neuro Dept", "9123456789", "Tue-Sat 10:00-18:00");
    bool saved = fm.saveDoctor(doc2, err);
    check("Second doctor saved successfully", saved);
    check("Exactly 2 data rows in CSV", countDataRows(testDoctorCsv) == 2);
    auto loaded = fm.loadDoctors();
    check("loadDoctors count is 2", loaded.size() == 2);
    if (loaded.size() >= 2) {
      check("First doctor preserved", loaded[0].getId() == "DOC1001");
      check("Second doctor present", loaded[1].getId() == "DOC1002");
    }
  }

  // Duplicate ID rejection (HMS-F-009 / BR-02)
  {
    const std::string before = readFile(testDoctorCsv);
    std::string err;
    // Attempt exact duplicate ID
    hms::Doctor dup1 = makeDoctor("DOC1001", "Dr. Duplicate", "Pediatrics", "Pediatrics Dept", "9998887776", "Mon-Fri 09:00-17:00");
    bool saved = fm.saveDoctor(dup1, err);
    check("Duplicate exact ID rejected", !saved);
    check("Duplicate error message contains duplicate notice", err.find("already exists") != std::string::npos);
    check("CSV file unchanged after duplicate attempt", readFile(testDoctorCsv) == before);
    check("Data row count still 2", countDataRows(testDoctorCsv) == 2);

    // Attempt case-insensitive duplicate ID
    hms::Doctor dup2 = makeDoctor("doc1002", "Dr. Case Dup", "Pediatrics", "Pediatrics Dept", "9998887776", "Mon-Fri 09:00-17:00");
    saved = fm.saveDoctor(dup2, err);
    check("Case-insensitive duplicate ID rejected", !saved);
    check("CSV file unchanged after case-insensitive duplicate", readFile(testDoctorCsv) == before);
    check("Data row count still 2", countDataRows(testDoctorCsv) == 2);
  }

  // Invalid data must not write or partially write to CSV
  {
    const std::string before = readFile(testDoctorCsv);
    std::string err;
    hms::Doctor invalidDoc = makeDoctor("DOC9999", "Dr. Bad", "Spec", "Dept", "123", "Mon-Fri"); // invalid phone
    bool saved = fm.saveDoctor(invalidDoc, err);
    check("Invalid doctor rejected by saveDoctor", !saved);
    check("Error message indicates invalid contact", err.find("Contact number") != std::string::npos);
    check("CSV unchanged after invalid doctor save", readFile(testDoctorCsv) == before);
    check("Data row count still 2", countDataRows(testDoctorCsv) == 2);
  }

  // Supported special characters in CSV
  {
    std::string err;
    hms::Doctor docSpecial = makeDoctor("DOC1003", "Dr. Mary-Jane O'Connor",
                                        "Ear-Nose-Throat & Surgery", "Surgical Unit #1",
                                        "9871112223", "Mon/Wed 08:00-12:00; Sat 10:00-13:00");
    bool saved = fm.saveDoctor(docSpecial, err);
    check("Doctor with allowed special characters saved", saved);
    check("Data row count now 3", countDataRows(testDoctorCsv) == 3);

    auto loaded = fm.loadDoctors();
    bool foundSpecial = false;
    for (const auto &d : loaded) {
      if (d.getId() == "DOC1003") {
        foundSpecial = (d.getName() == "Dr. Mary-Jane O'Connor" &&
                        d.getSpecialization() == "Ear-Nose-Throat & Surgery" &&
                        d.getDepartment() == "Surgical Unit #1" &&
                        d.getAvailability() == "Mon/Wed 08:00-12:00; Sat 10:00-13:00");
      }
    }
    check("Special characters correctly preserved and reloaded", foundSpecial);
  }

  // Malformed line skipping during load (HMS-F-027)
  {
    const std::string malformedCsv = "build/test_malformed_doctors.csv";
    removeFile(malformedCsv);
    {
      std::ofstream out(malformedCsv);
      out << "doctor_id,name,specialization,department,contact_number,availability\n";
      out << "DOC01,Dr. Valid One,Cardiology,Dept One,9876543210,Mon-Fri 09:00-17:00\n";
      out << "DOC02,Too,Few,Columns\n"; // malformed row (4 columns instead of 6)
      out << "DOC03,Dr. Bad Contact,Cardiology,Dept,12345,Mon-Fri\n"; // invalid contact (5 digits)
      out << "DOC04,Dr. Valid Two,Neurology,Dept Two,9123456789,Tue-Sat 10:00-18:00\n";
    }
    hms::FileManager malformedFm("data/patients.csv", malformedCsv);
    auto loaded = malformedFm.loadDoctors();
    check("Malformed rows skipped safely without crash", loaded.size() == 2);
    if (loaded.size() == 2) {
      check("First valid loaded", loaded[0].getId() == "DOC01");
      check("Second valid loaded", loaded[1].getId() == "DOC04");
    }
    removeFile(malformedCsv);
  }

      // Write failure / unwritable path handling (HMS-F-031)
  {
    const std::string blockerPath = "build/test_write_blocker_doctor";
    removeFile(blockerPath);

    {
      std::ofstream blocker(blockerPath);
      blocker << "blocker";
    }

    hms::FileManager badFm(
        "data/patients.csv", blockerPath + "/doctors.csv");

    std::string err;
    const bool saved = badFm.saveDoctor(makeDoctor("DOC999"), err);

    check("Save to unwritable path fails gracefully", !saved);
    check("Unwritable path error message populated", !err.empty());

    removeFile(blockerPath);
  }

  // =========================================================================
  // Section 3: HospitalSystem End-to-End Console Workflow
  // =========================================================================
  std::cout << "\n-- 3. HospitalSystem Interactive Workflow --\n";

  const std::string systemDoctorCsv = "build/test_system_doctors.csv";
  removeFile(systemDoctorCsv);

  // Positive registration workflow
  {
    hms::FileManager sysFm("data/patients.csv", systemDoctorCsv);
    hms::HospitalSystem sys(sysFm);

    // Provide simulated inputs: ID, Name, Specialization, Department, Contact, Availability
    std::string inputData =
        "DOC2001\n"
        "Dr. Leonard McCoy\n"
        "General Medicine\n"
        "Diagnostics\n"
        "9876543210\n"
        "Mon-Fri 08:00-16:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.addDoctor();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays Doctor added successfully", output.find("Doctor added successfully.") != std::string::npos);
    check("Console displays Doctor ID", output.find("Doctor ID: DOC2001") != std::string::npos);
    check("System persisted 1 record in CSV", countDataRows(systemDoctorCsv) == 1);
  }

  // Duplicate rejection in console workflow
  {
    hms::FileManager sysFm("data/patients.csv", systemDoctorCsv);
    hms::HospitalSystem sys(sysFm);

    // Provide input with existing ID in lowercase: "doc2001"
    std::string inputData =
        "doc2001\n"
        "Dr. Another McCoy\n"
        "Pediatrics\n"
        "Children Clinic\n"
        "9998887776\n"
        "Mon-Fri 09:00-17:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.addDoctor();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays Doctor ID already exists", output.find("Doctor ID already exists") != std::string::npos);
    check("CSV row count unchanged after duplicate attempt", countDataRows(systemDoctorCsv) == 1);
  }

  // Field validation rejection in console workflow
  {
    hms::FileManager sysFm("data/patients.csv", systemDoctorCsv);
    hms::HospitalSystem sys(sysFm);

    // Provide invalid contact (9 digits)
    std::string inputData =
        "DOC2002\n"
        "Dr. Sarah Connor\n"
        "Orthopedics\n"
        "Surgery\n"
        "987654321\n" // 9 digits
        "Mon-Fri 09:00-17:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.addDoctor();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays Contact number validation error",
          output.find("Contact number: must contain exactly 10 digits.") != std::string::npos);
    check("CSV row count unchanged after invalid contact attempt", countDataRows(systemDoctorCsv) == 1);
  }

    // Immediate validation and retry for an empty required field
  {
    hms::FileManager sysFm("data/patients.csv", systemDoctorCsv);
    hms::HospitalSystem sys(sysFm);

    // Empty Doctor ID, followed by a valid ID and valid remaining fields.
    std::string inputData =
        "\n"
        "DOC2003\n"
        "Dr. Sarah Connor\n"
        "Orthopedics\n"
        "Surgery\n"
        "9876543210\n"
        "Mon-Fri 09:00-17:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.addDoctor();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();

    const std::string idPrompt =
        "Doctor ID (1-10 alphanumeric characters, e.g., D001): ";

    const size_t firstPrompt = output.find(idPrompt);
    const size_t validationError =
        output.find("Doctor ID: must contain 1-10 alphanumeric characters.");
    const size_t retryPrompt =
        output.find(idPrompt, firstPrompt + idPrompt.size());

    check("Empty Doctor ID displays specific validation error",
          validationError != std::string::npos);

    check("Doctor ID is prompted again after validation error",
          firstPrompt != std::string::npos &&
          retryPrompt != std::string::npos &&
          validationError < retryPrompt);

    check("Valid Doctor ID is accepted after retry",
          output.find("Doctor added successfully.") != std::string::npos);

    check("CSV contains original and successfully retried doctor",
          countDataRows(systemDoctorCsv) == 2);

    check("Retried doctor is saved",
          sysFm.doctorIdExists("DOC2003"));
  }
  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=======================================================\n";
  std::cout << "Tests passed: " << g_passed << ", Tests failed: " << g_failed << '\n';
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
