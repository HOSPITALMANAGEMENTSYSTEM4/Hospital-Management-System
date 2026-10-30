// Test Suite for Issue: Prevent Duplicate Patient and Doctor Registrations Using Identity Validation
// Requirements:
// 1. Detect potential duplicate patients using normalized name and contact number.
// 2. Detect potential duplicate doctors using normalized name and contact number, with specialization as an additional matching attribute.
// 3. Display an appropriate warning containing the existing Patient ID or Doctor ID.
// 4. Require explicit user resolution before proceeding with registration.
// 5. Do not reject individuals based on name alone.
// 6. Preserve existing duplicate ID validation and all existing functionality.
// 7. Add automated tests covering duplicate identities, different individuals with matching names, case normalization and whitespace handling.
// 8. Ensure rejected registrations do not modify CSV files.

#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void check(const std::string &description, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << description << '\n';
    ++g_passed;
  } else {
    std::cout << "  [FAIL] " << description << '\n';
    ++g_failed;
  }
}

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
  if (!std::getline(f, line)) return 0; // skip header
  while (std::getline(f, line)) {
    if (!line.empty()) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::remove(path.c_str());
}

// Resolves path to production data directory regardless of whether test runs
// from the project root or from inside the build/ directory under CTest.
std::string resolveProductionPath(const std::string &relPath) {
  if (std::filesystem::exists(relPath)) {
    return relPath;
  }
  if (std::filesystem::exists("../" + relPath)) {
    return "../" + relPath;
  }
  return relPath;
}

} // namespace

int main() {
  std::cout << "=== Running Duplicate Identity Validation Tests ===\n\n";

  // Capture baseline of actual production CSV files
  const std::string prodPatientPath = resolveProductionPath("data/patients.csv");
  const std::string prodDoctorPath = resolveProductionPath("data/doctors.csv");
  const std::string prodApptPath = resolveProductionPath("data/appointments.csv");

  const std::string prodPatientBaseline = readFile(prodPatientPath);
  const std::string prodDoctorBaseline = readFile(prodDoctorPath);
  const std::string prodApptBaseline = readFile(prodApptPath);

  // Isolated test directory to ensure real user data is never touched
  const std::string testDir = "build/test_duplicate_identity";
  std::error_code dirEc;
  std::filesystem::create_directories(testDir, dirEc);

  const std::string dummyDoctorCsv = testDir + "/dummy_doctors.csv";
  const std::string dummyApptCsv = testDir + "/dummy_appointments.csv";
  const std::string dummyPatientCsv = testDir + "/dummy_patients.csv";
  removeFile(dummyDoctorCsv);
  removeFile(dummyApptCsv);
  removeFile(dummyPatientCsv);

  // =========================================================================
  // Section 1: Normalization Utility Tests (Case and Whitespace Handling)
  // =========================================================================
  std::cout << "-- 1. Normalization Utility Tests --\n";
  {
    check("Normalizes lowercase", hms::FileManager::normalizeString("john doe") == "john doe");
    check("Normalizes uppercase to lowercase", hms::FileManager::normalizeString("JOHN DOE") == "john doe");
    check("Normalizes mixed case", hms::FileManager::normalizeString("JoHn DoE") == "john doe");
    check("Strips leading and trailing whitespace", hms::FileManager::normalizeString("   John Doe   ") == "john doe");
    check("Collapses internal multiple spaces", hms::FileManager::normalizeString("John    Doe") == "john doe");
    check("Collapses tabs and spaces", hms::FileManager::normalizeString("  John \t  Doe  ") == "john doe");
    check("Handles punctuation in names", hms::FileManager::normalizeString("Dr. John-Paul O'Connor") == "dr. john-paul o'connor");
    check("Handles contact number whitespace", hms::FileManager::normalizeString("  9876543210  ") == "9876543210");
    check("Handles specialization normalization", hms::FileManager::normalizeString("  Cardiology  ") == "cardiology");
    check("Handles multi-word specialization", hms::FileManager::normalizeString("  Cardio-Thoracic   Surgery ") == "cardio-thoracic surgery");
  }

  // =========================================================================
  // Section 2: Patient Identity Lookup via FileManager
  // =========================================================================
  std::cout << "\n-- 2. Patient Identity Lookup (FileManager) --\n";
  const std::string testPatientCsv = testDir + "/test_identity_patients.csv";
  removeFile(testPatientCsv);
  {
    hms::FileManager fm(testPatientCsv, dummyDoctorCsv, dummyApptCsv);
    std::string err;

    // Seed initial patient P001: "Alice Smith", "9876543210"
    hms::Patient p1("P001", "Alice Smith", 30, "F", "9876543210", "100 Maple St", "None");
    check("Seed initial patient P001", fm.savePatient(p1, err));

    // 2.1: Exact matching name and contact detected as duplicate
    std::string existingId;
    bool foundExact = fm.patientIdentityExists("Alice Smith", "9876543210", &existingId);
    check("Exact match detected as duplicate identity", foundExact);
    check("Existing ID correctly populated as P001", existingId == "P001");

    // 2.2: Case-insensitive match ("ALICE SMITH")
    existingId.clear();
    bool foundUpper = fm.patientIdentityExists("ALICE SMITH", "9876543210", &existingId);
    check("Case-insensitive name match detected", foundUpper);
    check("Existing ID is P001 on case variation", existingId == "P001");

    // 2.3: Whitespace variation match ("  Alice   Smith  ")
    existingId.clear();
    bool foundWhitespace = fm.patientIdentityExists("  Alice   Smith  ", "  9876543210  ", &existingId);
    check("Whitespace variation match detected", foundWhitespace);
    check("Existing ID is P001 on whitespace variation", existingId == "P001");

    // 2.4: Different individual with MATCHING NAME but DIFFERENT contact number (Req 5)
    existingId.clear();
    bool diffContact = fm.patientIdentityExists("Alice Smith", "9123456789", &existingId);
    check("Matching name with different contact is NOT detected as duplicate (Req 5)", !diffContact);
    check("Existing ID empty when not a duplicate", existingId.empty());

    // 2.5: Different name but same contact number
    existingId.clear();
    bool diffName = fm.patientIdentityExists("Bob Jones", "9876543210", &existingId);
    check("Different name with same contact is NOT detected as duplicate", !diffName);

    // 2.6: Completely different individual
    existingId.clear();
    bool diffAll = fm.patientIdentityExists("Bob Jones", "9123456789", &existingId);
    check("Different individual is NOT detected as duplicate", !diffAll);

    // 2.7: Exclusion by ID
    existingId.clear();
    bool excluded = fm.patientIdentityExists("Alice Smith", "9876543210", &existingId, "P001");
    check("Excluding P001 ignores self-match", !excluded);

    // 2.8: HospitalSystem findPatientByIdentity
    hms::HospitalSystem sys(fm);
    const hms::Patient *foundP = sys.findPatientByIdentity("alice smith", "9876543210");
    check("HospitalSystem findPatientByIdentity finds record", foundP != nullptr);
    if (foundP) {
      check("findPatientByIdentity returns correct ID", foundP->getId() == "P001");
    }
  }

  // =========================================================================
  // Section 3: Doctor Identity Lookup via FileManager
  // =========================================================================
  std::cout << "\n-- 3. Doctor Identity Lookup (FileManager) --\n";
  const std::string testDoctorCsv = testDir + "/test_identity_doctors.csv";
  removeFile(testDoctorCsv);
  {
    hms::FileManager fm(dummyPatientCsv, testDoctorCsv, dummyApptCsv);
    std::string err;

    // Seed initial doctor D001: "Dr. Gregory House", "9876543210", "Diagnostic Medicine"
    hms::Doctor d1("D001", "Dr. Gregory House", "Diagnostic Medicine", "Diagnostics",
                   "9876543210", "Mon-Fri 09:00-17:00");
    check("Seed initial doctor D001", fm.saveDoctor(d1, err));

    // 3.1: Exact match detected as duplicate
    std::string existingDocId;
    bool foundExact = fm.doctorIdentityExists("Dr. Gregory House", "9876543210",
                                             "Diagnostic Medicine", &existingDocId);
    check("Exact doctor identity match detected", foundExact);
    check("Existing Doctor ID populated as D001", existingDocId == "D001");

    // 3.2: Case and whitespace variation match
    existingDocId.clear();
    bool foundNormalized = fm.doctorIdentityExists("  dr.   gregory   house  ", " 9876543210 ",
                                                  "  diagnostic   medicine  ", &existingDocId);
    check("Normalized doctor identity match detected", foundNormalized);
    check("Existing Doctor ID is D001", existingDocId == "D001");

    // 3.3: Same name and contact, but DIFFERENT specialization (Req 2)
    existingDocId.clear();
    bool diffSpec = fm.doctorIdentityExists("Dr. Gregory House", "9876543210",
                                           "Neurology", &existingDocId);
    check("Different specialization is NOT duplicate doctor (Req 2)", !diffSpec);

    // 3.4: Same name and specialization, but DIFFERENT contact number (Req 5)
    existingDocId.clear();
    bool diffContact = fm.doctorIdentityExists("Dr. Gregory House", "9123456789",
                                              "Diagnostic Medicine", &existingDocId);
    check("Matching name with different contact is NOT duplicate doctor (Req 5)", !diffContact);

    // 3.5: Same name alone is not a duplicate (Req 5)
    existingDocId.clear();
    bool nameOnly = fm.doctorIdentityExists("Dr. Gregory House", "9111111111",
                                           "Orthopedics", &existingDocId);
    check("Same name alone is NOT rejected/duplicate (Req 5)", !nameOnly);

    // 3.6: HospitalSystem findDoctorByIdentity
    hms::HospitalSystem sys(fm);
    const hms::Doctor *foundD = sys.findDoctorByIdentity("dr. gregory house", "9876543210", "diagnostic medicine");
    check("HospitalSystem findDoctorByIdentity finds record", foundD != nullptr);
    if (foundD) {
      check("findDoctorByIdentity returns correct ID", foundD->getId() == "D001");
    }
  }

  // =========================================================================
  // Section 4: Interactive Patient Registration Console Workflow
  // =========================================================================
  std::cout << "\n-- 4. Interactive Patient Registration Workflow --\n";
  {
    const std::string sysPatientCsv = testDir + "/test_sys_identity_patients.csv";
    removeFile(sysPatientCsv);

    hms::FileManager fm(sysPatientCsv, dummyDoctorCsv, dummyApptCsv);
    std::string err;
    hms::Patient seedP("P001", "Eleanor Vance", 28, "F", "9876543210", "404 Hill House Rd", "None");
    check("Seed P001 for interactive tests", fm.savePatient(seedP, err));
    const int initialRows = countDataRows(sysPatientCsv);
    check("Initial row count is 1", initialRows == 1);

    // 4.1: Register duplicate patient identity, user enters 'Y' (uppercase confirmation)
    {
      hms::HospitalSystem sys(fm);
      std::string input =
          "P002\n"
          "  ELEANOR   VANCE  \n"
          "29\n"
          "F\n"
          "9876543210\n"
          "500 Elm St\n"
          "\n"
          "Y\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.1: Displays Warning for potential duplicate", out.find("Warning: Potential duplicate patient record") != std::string::npos);
      check("4.1: Warning contains existing Patient ID P001", out.find("P001") != std::string::npos);
      check("4.1: Prompts for resolution (Y/N)", out.find("Proceed with registration? (Y/N): ") != std::string::npos);
      check("4.1: Registration succeeds upon 'Y' resolution", out.find("Patient registered successfully.") != std::string::npos);
      check("4.1: Patient ID P002 displayed", out.find("Patient ID: P002") != std::string::npos);
      check("4.1: CSV row count increased to 2", countDataRows(sysPatientCsv) == 2);
    }

    // 4.2: Register duplicate patient identity, user enters 'y' (lowercase confirmation)
    {
      hms::HospitalSystem sys(fm);
      std::string input =
          "P003\n"
          "eleanor vance\n"
          "30\n"
          "F\n"
          "9876543210\n"
          "600 Oak St\n"
          "\n"
          "y\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.2: Registration succeeds upon lowercase 'y'", out.find("Patient registered successfully.") != std::string::npos);
      check("4.2: CSV row count increased to 3", countDataRows(sysPatientCsv) == 3);
    }

    // 4.3: Register duplicate patient identity, user enters 'N' (rejection / user decline)
    {
      const std::string beforeDeclineCsv = readFile(sysPatientCsv);
      const int beforeDeclineRows = countDataRows(sysPatientCsv);

      hms::HospitalSystem sys(fm);
      std::string input =
          "P004\n"
          "Eleanor Vance\n"
          "31\n"
          "F\n"
          "9876543210\n"
          "700 Pine St\n"
          "\n"
          "N\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.3: Displays Warning for potential duplicate", out.find("Warning: Potential duplicate patient record") != std::string::npos);
      check("4.3: Displays 'Registration cancelled.' on N", out.find("Registration cancelled.") != std::string::npos);
      check("4.3: Does not display registration success", out.find("Patient registered successfully.") == std::string::npos);
      check("4.3: CSV row count unchanged on decline (Req 8)", countDataRows(sysPatientCsv) == beforeDeclineRows);
      check("4.3: CSV byte-for-byte identical on decline (Req 8)", readFile(sysPatientCsv) == beforeDeclineCsv);
    }

    // 4.4: Register duplicate patient identity, user enters 'n' (lowercase decline)
    {
      const std::string beforeDeclineCsv = readFile(sysPatientCsv);
      const int beforeDeclineRows = countDataRows(sysPatientCsv);

      hms::HospitalSystem sys(fm);
      std::string input =
          "P005\n"
          "Eleanor Vance\n"
          "31\n"
          "F\n"
          "9876543210\n"
          "800 Birch St\n"
          "\n"
          "n\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.4: Displays 'Registration cancelled.' on lowercase 'n'", out.find("Registration cancelled.") != std::string::npos);
      check("4.4: CSV row count unchanged (Req 8)", countDataRows(sysPatientCsv) == beforeDeclineRows);
      check("4.4: CSV unchanged (Req 8)", readFile(sysPatientCsv) == beforeDeclineCsv);
    }

    // 4.5: Different individual with MATCHING NAME but DIFFERENT contact number (Req 5)
    {
      const int beforeRows = countDataRows(sysPatientCsv);

      hms::HospitalSystem sys(fm);
      // Same name "Eleanor Vance", but different contact "9123456789" -> NO WARNING, NO PROMPT
      std::string input =
          "P010\n"
          "Eleanor Vance\n"
          "45\n"
          "F\n"
          "9123456789\n"
          "999 Walnut St\n"
          "\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.5: Same name with different contact has NO duplicate warning (Req 5)",
            out.find("Warning: Potential duplicate") == std::string::npos);
      check("4.5: Same name with different contact does NOT prompt for resolution",
            out.find("Proceed with registration? (Y/N): ") == std::string::npos);
      check("4.5: Registration succeeds directly without interruption",
            out.find("Patient registered successfully.") != std::string::npos);
      check("4.5: CSV row count incremented", countDataRows(sysPatientCsv) == beforeRows + 1);
    }

    // 4.6: Existing duplicate ID validation is preserved (Req 6)
    {
      const std::string beforeCsv = readFile(sysPatientCsv);
      hms::HospitalSystem sys(fm);

      // Attempt to register existing ID P001 with completely different details
      std::string input =
          "p001\n"
          "Brand New Person\n"
          "25\n"
          "M\n"
          "9999999999\n"
          "123 Any St\n"
          "\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.registerPatient();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("4.6: Duplicate ID displays exact required message (Req 6)",
            out.find("Patient ID already exists") != std::string::npos);
      check("4.6: Duplicate ID does not prompt for identity confirmation",
            out.find("Proceed with registration? (Y/N): ") == std::string::npos);
      check("4.6: CSV unchanged on duplicate ID attempt", readFile(sysPatientCsv) == beforeCsv);
    }
  }

  // =========================================================================
  // Section 5: Interactive Doctor Registration Console Workflow
  // =========================================================================
  std::cout << "\n-- 5. Interactive Doctor Registration Workflow --\n";
  {
    const std::string sysDoctorCsv = testDir + "/test_sys_identity_doctors.csv";
    removeFile(sysDoctorCsv);

    hms::FileManager fm(dummyPatientCsv, sysDoctorCsv, dummyApptCsv);
    std::string err;
    hms::Doctor seedD("D001", "Dr. Luke Sanderson", "Orthopedics", "Surgery",
                      "9876543210", "Mon-Fri 08:00-16:00");
    check("Seed D001 for interactive doctor tests", fm.saveDoctor(seedD, err));
    const int initialRows = countDataRows(sysDoctorCsv);
    check("Initial doctor rows is 1", initialRows == 1);

    // 5.1: Duplicate doctor identity, user enters 'Y' (uppercase confirm)
    {
      hms::HospitalSystem sys(fm);
      std::string input =
          "D002\n"
          "  DR. LUKE SANDERSON  \n"
          "  orthopedics  \n"
          "Trauma\n"
          "  9876543210  \n"
          "Mon-Fri 09:00-17:00\n"
          "Y\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.addDoctor();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("5.1: Displays Warning for potential duplicate doctor",
            out.find("Warning: Potential duplicate doctor record") != std::string::npos);
      check("5.1: Warning contains existing Doctor ID D001",
            out.find("D001") != std::string::npos);
      check("5.1: Prompts for resolution (Y/N)",
            out.find("Proceed with registration? (Y/N): ") != std::string::npos);
      check("5.1: Doctor added successfully on 'Y'",
            out.find("Doctor added successfully.") != std::string::npos);
      check("5.1: CSV row count increased to 2", countDataRows(sysDoctorCsv) == 2);
    }

    // 5.2: Duplicate doctor identity, user enters 'N' (decline)
    {
      const std::string beforeDeclineCsv = readFile(sysDoctorCsv);
      const int beforeDeclineRows = countDataRows(sysDoctorCsv);

      hms::HospitalSystem sys(fm);
      std::string input =
          "D003\n"
          "Dr. Luke Sanderson\n"
          "Orthopedics\n"
          "Sports Medicine\n"
          "9876543210\n"
          "Mon-Fri 09:00-17:00\n"
          "N\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.addDoctor();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("5.2: Displays Warning for potential duplicate doctor",
            out.find("Warning: Potential duplicate doctor record") != std::string::npos);
      check("5.2: Displays 'Registration cancelled.' on N",
            out.find("Registration cancelled.") != std::string::npos);
      check("5.2: Does not display addition success",
            out.find("Doctor added successfully.") == std::string::npos);
      check("5.2: CSV row count unchanged on decline (Req 8)",
            countDataRows(sysDoctorCsv) == beforeDeclineRows);
      check("5.2: CSV byte-for-byte identical (Req 8)",
            readFile(sysDoctorCsv) == beforeDeclineCsv);
    }

    // 5.3: Same name and contact, but DIFFERENT specialization -> NO WARNING, proceeds (Req 2, 5)
    {
      const int beforeRows = countDataRows(sysDoctorCsv);

      hms::HospitalSystem sys(fm);
      // Same name "Dr. Luke Sanderson", same contact "9876543210", but DIFFERENT specialization "Cardiology"
      std::string input =
          "D004\n"
          "Dr. Luke Sanderson\n"
          "Cardiology\n"
          "Heart Clinic\n"
          "9876543210\n"
          "Mon-Fri 09:00-17:00\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.addDoctor();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("5.3: Different specialization has NO duplicate warning (Req 2)",
            out.find("Warning: Potential duplicate") == std::string::npos);
      check("5.3: Different specialization does NOT prompt for resolution",
            out.find("Proceed with registration? (Y/N): ") == std::string::npos);
      check("5.3: Doctor added successfully directly",
            out.find("Doctor added successfully.") != std::string::npos);
      check("5.3: CSV row count incremented", countDataRows(sysDoctorCsv) == beforeRows + 1);
    }

    // 5.4: Same name, but DIFFERENT contact number -> NO WARNING, proceeds (Req 5)
    {
      const int beforeRows = countDataRows(sysDoctorCsv);

      hms::HospitalSystem sys(fm);
      std::string input =
          "D005\n"
          "Dr. Luke Sanderson\n"
          "Orthopedics\n"
          "Pediatrics\n"
          "9111222333\n"
          "Mon-Fri 09:00-17:00\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.addDoctor();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("5.4: Different contact has NO duplicate warning (Req 5)",
            out.find("Warning: Potential duplicate") == std::string::npos);
      check("5.4: Doctor added successfully directly",
            out.find("Doctor added successfully.") != std::string::npos);
      check("5.4: CSV row count incremented", countDataRows(sysDoctorCsv) == beforeRows + 1);
    }

    // 5.5: Duplicate Doctor ID validation preserved (Req 6)
    {
      const std::string beforeCsv = readFile(sysDoctorCsv);
      hms::HospitalSystem sys(fm);

      std::string input =
          "d001\n"
          "Another Doctor\n"
          "Neurology\n"
          "Brain Clinic\n"
          "9998887776\n"
          "Mon-Fri 09:00-17:00\n";

      std::istringstream inStream(input);
      std::ostringstream outStream;
      auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
      auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

      sys.addDoctor();

      std::cin.rdbuf(cinBuf);
      std::cout.rdbuf(coutBuf);

      std::string out = outStream.str();
      check("5.5: Duplicate Doctor ID displays exact required message (Req 6)",
            out.find("Doctor ID already exists") != std::string::npos);
      check("5.5: Duplicate Doctor ID does not prompt for identity confirmation",
            out.find("Proceed with registration? (Y/N): ") == std::string::npos);
      check("5.5: CSV unchanged on duplicate ID attempt", readFile(sysDoctorCsv) == beforeCsv);
    }
  }

  // =========================================================================
  // Section 6: Non-regression and Root-level CSV Data Protection
  // =========================================================================
  std::cout << "\n-- 6. Root-Level CSV Data Protection --\n";
  {
    check("Root data/patients.csv exists", std::filesystem::exists(prodPatientPath));
    check("Root data/doctors.csv exists", std::filesystem::exists(prodDoctorPath));
    check("Root data/appointments.csv exists", std::filesystem::exists(prodApptPath));
    check("Root data/patients.csv remains untouched", readFile(prodPatientPath) == prodPatientBaseline);
    check("Root data/doctors.csv remains untouched", readFile(prodDoctorPath) == prodDoctorBaseline);
    check("Root data/appointments.csv remains untouched", readFile(prodApptPath) == prodApptBaseline);
  }

  std::cout << "\n=======================================================\n";
  std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return (g_failed == 0) ? 0 : 1;
}
