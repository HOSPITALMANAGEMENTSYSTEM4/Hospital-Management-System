// TC-PAT-06  Update Patient Details (HMS-F-006, HMS-F-007)
//
// Acceptance Criteria:
//   - A valid update to an existing patient is saved to patients.csv.
//   - The updated values appear when the patient is viewed or searched again.
//   - Invalid field values are rejected according to registration validation rules (VR-01 through VR-07, VR-14).
//   - A failed or invalid update leaves the stored patient record unchanged.
//   - Existing patient records remain intact when one patient's details are updated.
//   - Patient ID remains unchanged; no option is provided to change the ID.
//   - Non-existent Patient ID is rejected with "Patient not found" and changes nothing.
//   - Update survives application restart (data reload from CSV).
//   - File write failures do not report false success or cause partial data loss.
//   - Interactive console workflow (Patient Management -> 4) works as specified.
//
// Self-contained test binary following the project's CTest harness conventions.

#include "hms/Appointment.h"
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

void check(const char *name, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << name << '\n';
    ++g_passed;
  } else {
    std::cout << "  [FAIL] " << name << '\n';
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
  while (std::getline(f, line)) {
    if (!line.empty() && line.rfind("patient_id", 0) != 0 &&
        line.rfind("PatientID", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

std::size_t countOccurrences(const std::string &haystack,
                             const std::string &needle) {
  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

std::string runInteractive(hms::HospitalSystem &sys, const std::string &input) {
  std::istringstream in(input);
  std::ostringstream out;
  auto *oldIn = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  sys.run();

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);
  return out.str();
}

} // namespace

int main() {
  std::cout << "=== Running TC-PAT-06 / HMS-F-006 Update Patient Details Tests ===\n\n";

  const std::string testDir = "build/test_tc_pat_06_data";
  std::error_code ec;
  std::filesystem::remove_all(testDir, ec);
  std::filesystem::create_directories(testDir, ec);

  const std::string testPatientCsv = testDir + "/patients.csv";
  const std::string testDoctorCsv = testDir + "/doctors.csv";
  const std::string testApptCsv = testDir + "/appointments.csv";

  // =========================================================================
  // Section 1: Valid Update Modifies Intended Patient Fields (Programmatic API)
  // =========================================================================
  std::cout << "-- Section 1: Valid update modifies intended patient fields --\n";
  {
    removeFile(testPatientCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;
    const hms::Patient p1("P001", "John Doe", 45, "M", "9876543210", "123 Main St", "Hypertension");
    check("Seed initial patient P001", fm.savePatient(p1, err));

    hms::HospitalSystem system(fm);
    const hms::OpResult res = system.updatePatient(
        "P001", "Jane Doe", 46, "F", "9876543211", "456 Oak St", "Asthma");

    check("updatePatient returns success == true", res.success);
    check("updatePatient returns success message", res.message == "Patient updated successfully");

    const hms::Patient *updated = system.findPatient("P001");
    check("Patient P001 found in system", updated != nullptr);
    if (updated) {
      check("Patient ID remains P001", updated->getId() == "P001");
      check("Name updated to Jane Doe", updated->getName() == "Jane Doe");
      check("Age updated to 46", updated->getAge() == 46);
      check("Gender updated to F", updated->getGender() == "F");
      check("Contact updated to 9876543211", updated->getContactNumber() == "9876543211");
      check("Address updated to 456 Oak St", updated->getAddress() == "456 Oak St");
      check("Medical history updated to Asthma", updated->getMedicalHistory() == "Asthma");
    }
  }

  // =========================================================================
  // Section 2: Patient ID Remains Unchanged (Case Insensitivity & Immutability)
  // =========================================================================
  std::cout << "\n-- Section 2: Patient ID remains unchanged --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    // Call update using lowercase "p001"
    const hms::OpResult res = system.updatePatient(
        "p001", "Jane Smith", 47, "F", "9876543211", "789 Pine St", "None");

    check("Update using lowercase 'p001' succeeds", res.success);

    const hms::Patient *p = system.findPatient("P001");
    check("Patient ID is still normalized uppercase 'P001'", p != nullptr && p->getId() == "P001");
    check("Patient Name updated", p != nullptr && p->getName() == "Jane Smith");

    // Overload taking Patient object with different casing ID
    const hms::Patient replacement("p001", "Jane Williams", 48, "F", "9876543211", "101 Maple St", "None");
    const hms::OpResult res2 = system.updatePatient(replacement);
    check("updatePatient(Patient) succeeds", res2.success);

    const hms::Patient *p2 = system.findPatient("P001");
    check("Patient ID remains P001 after object update", p2 != nullptr && p2->getId() == "P001");
    check("Name updated to Jane Williams", p2 != nullptr && p2->getName() == "Jane Williams");
  }

  // =========================================================================
  // Section 3: Updated Values Saved to patients.csv Immediately
  // =========================================================================
  std::cout << "\n-- Section 3: Updated values saved to CSV immediately --\n";
  {
    const std::string csvContent = readFile(testPatientCsv);
    check("CSV file contains updated name", csvContent.find("Jane Williams") != std::string::npos);
    check("CSV file contains updated age 48", csvContent.find(",48,") != std::string::npos);
    check("CSV file contains updated address", csvContent.find("101 Maple St") != std::string::npos);
    check("CSV file does not contain old name 'John Doe'", csvContent.find("John Doe") == std::string::npos);
    check("CSV file does not contain old address '123 Main St'", csvContent.find("123 Main St") == std::string::npos);
    check("CSV contains exactly 1 data row (no duplicates appended)", countDataRows(testPatientCsv) == 1);
  }

  // =========================================================================
  // Section 4: Searching and Viewing Patient Displays Updated Values
  // =========================================================================
  std::cout << "\n-- Section 4: Searching and viewing patient displays updated values --\n";
  {
    hms::HospitalSystem system(testDir);

    // Search by new name "Williams"
    const std::string searchOut = runInteractive(system, "1\n2\nWilliams\n0\n4\n");
    check("Search by updated name displays Patient ID: P001", searchOut.find("Patient ID: P001") != std::string::npos);
    check("Search by updated name displays Name: Jane Williams", searchOut.find("Name: Jane Williams") != std::string::npos);
    check("Search by updated name displays Age: 48", searchOut.find("Age: 48") != std::string::npos);

    // Search by old name "John" returns "No matching patient found"
    const std::string oldSearchOut = runInteractive(system, "1\n2\nJohn\n0\n4\n");
    check("Search by old name does not find patient", oldSearchOut.find("No matching patient found") != std::string::npos);

    // View All Patients
    const std::string listOut = runInteractive(system, "1\n3\n0\n4\n");
    check("Patient list shows updated name", listOut.find("Jane Williams") != std::string::npos);
    check("Patient list shows updated age 48", listOut.find("48") != std::string::npos);
  }

  // =========================================================================
  // Section 5: Update Survives Application Restart (Reloading CSV)
  // =========================================================================
  std::cout << "\n-- Section 5: Update survives application restart --\n";
  {
    // Simulate restart by creating completely new HospitalSystem instance
    hms::FileManager newFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem restartedSystem(newFm);

    const hms::Patient *reloaded = restartedSystem.findPatient("P001");
    check("Reloaded patient found after restart", reloaded != nullptr);
    if (reloaded) {
      check("Reloaded ID is P001", reloaded->getId() == "P001");
      check("Reloaded name is Jane Williams", reloaded->getName() == "Jane Williams");
      check("Reloaded age is 48", reloaded->getAge() == 48);
      check("Reloaded gender is F", reloaded->getGender() == "F");
      check("Reloaded contact is 9876543211", reloaded->getContactNumber() == "9876543211");
      check("Reloaded address is 101 Maple St", reloaded->getAddress() == "101 Maple St");
      check("Reloaded medical history is None", reloaded->getMedicalHistory() == "None");
    }
  }

  // =========================================================================
  // Section 6: Reject Unknown Patient ID Without Changing Data
  // =========================================================================
  std::cout << "\n-- Section 6: Reject unknown patient ID --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    const std::string beforeCsv = readFile(testPatientCsv);
    const int beforeCount = countDataRows(testPatientCsv);

    std::string errMsg;
    const bool ok = system.updatePatient(
        "P999", "Ghost Patient", 30, "M", "9876543210", "Nowhere St", "None", errMsg);

    check("Updating non-existent ID P999 returns false", !ok);
    check("Error message is 'Patient not found'", errMsg == "Patient not found");
    check("CSV file unchanged after unknown ID attempt", readFile(testPatientCsv) == beforeCsv);
    check("Patient count unchanged", countDataRows(testPatientCsv) == beforeCount);
  }

  // =========================================================================
  // Section 7: Reject Invalid Field Values (VR-01 through VR-07, VR-14)
  // =========================================================================
  std::cout << "\n-- Section 7: Reject invalid field values --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    const std::string baselineCsv = readFile(testPatientCsv);

    // Helper lambda to test invalid updates
    auto testInvalidField = [&](const char *label,
                                const std::string &name, int age,
                                const std::string &gender,
                                const std::string &contact,
                                const std::string &address,
                                const std::string &med) {
      std::string err;
      const bool ok = system.updatePatient("P001", name, age, gender, contact, address, med, err);
      check(label, !ok && !err.empty());
      check("State protected: CSV unchanged", readFile(testPatientCsv) == baselineCsv);
      const hms::Patient *p = system.findPatient("P001");
      check("State protected: stored record unchanged", p != nullptr && p->getName() == "Jane Williams");
    };

    // VR-02: Name validation
    testInvalidField("VR-02: Reject empty name",
                     "", 48, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-02: Reject name > 50 characters",
                     std::string(51, 'A'), 48, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-02: Reject disallowed characters in name",
                     "Jane@Doe", 48, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-14: Reject formula prefix in name (=)",
                     "=Jane", 48, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-14: Reject comma in name",
                     "Doe, Jane", 48, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-14: Reject newline in name",
                     "Jane\nDoe", 48, "F", "9876543211", "101 Maple St", "None");

    // VR-03: Age validation
    testInvalidField("VR-03: Reject negative age (-1)",
                     "Jane Williams", -1, "F", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-03: Reject age > 120 (121)",
                     "Jane Williams", 121, "F", "9876543211", "101 Maple St", "None");

    // VR-04: Gender validation
    testInvalidField("VR-04: Reject invalid gender 'X'",
                     "Jane Williams", 48, "X", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-04: Reject multi-char gender 'Female'",
                     "Jane Williams", 48, "Female", "9876543211", "101 Maple St", "None");
    testInvalidField("VR-04: Reject empty gender",
                     "Jane Williams", 48, "", "9876543211", "101 Maple St", "None");

    // VR-05: Contact number validation
    testInvalidField("VR-05: Reject contact < 10 digits",
                     "Jane Williams", 48, "F", "987654321", "101 Maple St", "None");
    testInvalidField("VR-05: Reject contact > 10 digits",
                     "Jane Williams", 48, "F", "98765432101", "101 Maple St", "None");
    testInvalidField("VR-05: Reject non-digit in contact",
                     "Jane Williams", 48, "F", "987654321A", "101 Maple St", "None");
    testInvalidField("VR-14: Reject formula prefix in contact",
                     "Jane Williams", 48, "F", "=987654321", "101 Maple St", "None");
    testInvalidField("VR-14: Reject comma in contact",
                     "Jane Williams", 48, "F", "98765,4321", "101 Maple St", "None");

    // VR-06: Address validation
    testInvalidField("VR-06: Reject empty address",
                     "Jane Williams", 48, "F", "9876543211", "", "None");
    testInvalidField("VR-06: Reject address > 100 characters",
                     "Jane Williams", 48, "F", "9876543211", std::string(101, 'X'), "None");
    testInvalidField("VR-14: Reject formula prefix in address",
                     "Jane Williams", 48, "F", "9876543211", "=101 Maple St", "None");
    testInvalidField("VR-14: Reject comma in address",
                     "Jane Williams", 48, "F", "9876543211", "101, Maple St", "None");

    // VR-07: Medical history validation
    testInvalidField("VR-07: Reject medical history > 200 characters",
                     "Jane Williams", 48, "F", "9876543211", "101 Maple St", std::string(201, 'M'));
    testInvalidField("VR-14: Reject formula prefix in medical history",
                     "Jane Williams", 48, "F", "9876543211", "101 Maple St", "=Asthma");
    testInvalidField("VR-14: Reject comma in medical history",
                     "Jane Williams", 48, "F", "9876543211", "101 Maple St", "Asthma, Allergy");
  }

  // =========================================================================
  // Section 8: Updating One Patient Does Not Modify Other Patient Records
  // =========================================================================
  std::cout << "\n-- Section 8: Updating one patient does not modify other records --\n";
  {
    removeFile(testPatientCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;

    const hms::Patient p1("P001", "Alice One", 25, "F", "9876543210", "111 First Ave", "None");
    const hms::Patient p2("P002", "Bob Two", 35, "M", "9876543220", "222 Second Ave", "Diabetes");
    const hms::Patient p3("P003", "Charlie Three", 45, "M", "9876543230", "333 Third Ave", "Hypertension");

    check("Seed patient P001", fm.savePatient(p1, err));
    check("Seed patient P002", fm.savePatient(p2, err));
    check("Seed patient P003", fm.savePatient(p3, err));

    hms::HospitalSystem system(fm);

    // Update P002 only
    const hms::OpResult res = system.updatePatient(
        "P002", "Bob Updated", 36, "M", "9876543229", "222 New Ave", "Controlled Diabetes");
    check("Update P002 succeeded", res.success);

    // Verify P001 is completely untouched
    const hms::Patient *checkP1 = system.findPatient("P001");
    check("P001 still exists", checkP1 != nullptr);
    if (checkP1) {
      check("P001 name unchanged", checkP1->getName() == "Alice One");
      check("P001 age unchanged", checkP1->getAge() == 25);
      check("P001 contact unchanged", checkP1->getContactNumber() == "9876543210");
      check("P001 address unchanged", checkP1->getAddress() == "111 First Ave");
    }

    // Verify P002 was updated
    const hms::Patient *checkP2 = system.findPatient("P002");
    check("P002 updated name", checkP2 != nullptr && checkP2->getName() == "Bob Updated");
    check("P002 updated age", checkP2 != nullptr && checkP2->getAge() == 36);

    // Verify P003 is completely untouched
    const hms::Patient *checkP3 = system.findPatient("P003");
    check("P003 still exists", checkP3 != nullptr);
    if (checkP3) {
      check("P003 name unchanged", checkP3->getName() == "Charlie Three");
      check("P003 age unchanged", checkP3->getAge() == 45);
      check("P003 address unchanged", checkP3->getAddress() == "333 Third Ave");
      check("P003 medical history unchanged", checkP3->getMedicalHistory() == "Hypertension");
    }

    // Verify total count in CSV is still exactly 3
    check("Total record count remains exactly 3", countDataRows(testPatientCsv) == 3);
  }

  // =========================================================================
  // Section 9: File Write Failure Safety (Non-destructive atomicity)
  // =========================================================================
  std::cout << "\n-- Section 9: File write failure safety --\n";
  {
    // Try to save to an impossible / invalid file path
    const std::string invalidPath = testDir + "/non_existent_subdir/deep/nested/patients.csv";
    // Create a file blocking directory creation to trigger failure
    const std::string blocker = testDir + "/non_existent_subdir";
    {
      std::ofstream b(blocker);
      b << "blocking file";
    }

    hms::FileManager failingFm(invalidPath, testDoctorCsv, testApptCsv);
    const hms::Patient validPatient("P001", "Jane Doe", 30, "F", "9876543210", "123 Main St", "None");

    std::string err;
    const bool saveFailed = failingFm.updatePatient(validPatient, err);
    check("Update patient on invalid path returns false", !saveFailed);
    check("Error message indicates save failure", !err.empty());

    removeFile(blocker);
  }

  // =========================================================================
  // Section 10: Interactive Console Workflow (Patient Management -> 4: Update)
  // =========================================================================
  std::cout << "\n-- Section 10: Interactive Console Workflow --\n";
  {
    removeFile(testPatientCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;
    const hms::Patient p1("P001", "David Ross", 50, "M", "9876543210", "500 High St", "Hypertension");
    check("Seed interactive patient P001", fm.savePatient(p1, err));

    hms::HospitalSystem system(testDir);

    // 10A: Unknown Patient ID entered at Update Patient prompt
    {
      // Inputs: 1 (Patient Menu) -> 4 (Update) -> P999 (ID) -> 0 (Return) -> 4 (Exit)
      const std::string out = runInteractive(system, "1\n4\nP999\n0\n4\n");
      check("Interactive: displays 'Patient not found'", out.find("Patient not found") != std::string::npos);
    }

    // 10B: Update individual field: choice 1 (Name)
    {
      // Inputs: 1 (Patient Menu) -> 4 (Update) -> P001 -> 1 (Name) -> David Wallace -> 0 (Return) -> 4 (Exit)
      const std::string out = runInteractive(system, "1\n4\nP001\n1\nDavid Wallace\n0\n4\n");
      check("Interactive: confirms successful update", out.find("Patient updated successfully") != std::string::npos);

      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Name changed to David Wallace", p != nullptr && p->getName() == "David Wallace");
      check("Interactive: Age kept as 50", p != nullptr && p->getAge() == 50);
      check("Interactive: Gender kept as M", p != nullptr && p->getGender() == "M");
    }

    // 10C: Update individual field: choice 2 (Age)
    {
      // Inputs: 1 -> 4 -> P001 -> 2 -> 52 -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n2\n52\n0\n4\n");
      check("Interactive: Age update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Age changed to 52", p != nullptr && p->getAge() == 52);
    }

    // 10D: Update individual field: choice 3 (Gender)
    {
      // Inputs: 1 -> 4 -> P001 -> 3 -> O -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n3\nO\n0\n4\n");
      check("Interactive: Gender update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Gender changed to O", p != nullptr && p->getGender() == "O");
    }

    // 10E: Update individual field: choice 4 (Contact number)
    {
      // Inputs: 1 -> 4 -> P001 -> 4 -> 9123456789 -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n4\n9123456789\n0\n4\n");
      check("Interactive: Contact update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Contact changed to 9123456789", p != nullptr && p->getContactNumber() == "9123456789");
    }

    // 10F: Update individual field: choice 5 (Address)
    {
      // Inputs: 1 -> 4 -> P001 -> 5 -> 600 Grand Ave -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n5\n600 Grand Ave\n0\n4\n");
      check("Interactive: Address update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Address changed to 600 Grand Ave", p != nullptr && p->getAddress() == "600 Grand Ave");
    }

    // 10G: Update individual field: choice 6 (Medical history)
    {
      // Inputs: 1 -> 4 -> P001 -> 6 -> None -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n6\nNone\n0\n4\n");
      check("Interactive: Medical history update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Medical history changed to None", p != nullptr && p->getMedicalHistory() == "None");
    }

    // 10H: Update all fields: choice 7 (All fields)
    {
      // Inputs: 1 -> 4 -> P001 -> 7 -> Sarah Connor -> 35 -> F -> 9998887776 -> 700 Cyber St -> None -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n7\nSarah Connor\n35\nF\n9998887776\n700 Cyber St\nNone\n0\n4\n");
      check("Interactive: All fields update succeeds", out.find("Patient updated successfully") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: Name is Sarah Connor", p != nullptr && p->getName() == "Sarah Connor");
      check("Interactive: Age is 35", p != nullptr && p->getAge() == 35);
      check("Interactive: Gender is F", p != nullptr && p->getGender() == "F");
      check("Interactive: Contact is 9998887776", p != nullptr && p->getContactNumber() == "9998887776");
      check("Interactive: Address is 700 Cyber St", p != nullptr && p->getAddress() == "700 Cyber St");
    }

    // 10I: Update menu offers no Patient ID edit
    {
      // Inputs: 1 -> 4 -> P001 -> 0 -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n0\n0\n4\n");
      check("Interactive: Menu contains 'Select field to update'", out.find("Select field to update") != std::string::npos);
      check("Interactive: Menu contains 1. Name", out.find("1. Name") != std::string::npos);
      check("Interactive: Menu does NOT offer Patient ID option", out.find("Patient ID edit") == std::string::npos &&
                                                                   out.find("0. Return") != std::string::npos);
    }

    // 10J: Invalid input at update field prompt rejected
    {
      // Inputs: 1 -> 4 -> P001 -> 1 -> Doe, John (invalid comma in name) -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n1\nDoe, John\n0\n4\n");
      check("Interactive: Reject invalid name with error message",
            out.find("must contain 1-50 permitted characters") != std::string::npos);
      const hms::Patient *p = system.findPatient("P001");
      check("Interactive: State protected, Name still Sarah Connor", p != nullptr && p->getName() == "Sarah Connor");

      // Inputs: 1 -> 4 -> P001 -> 5 -> 700, Cyber St (invalid comma in address) -> 0 -> 4
      const std::string outAddr = runInteractive(system, "1\n4\nP001\n5\n700, Cyber St\n0\n4\n");
      check("Interactive: Reject invalid address comma with error message",
            outAddr.find("commas and line breaks are not allowed") != std::string::npos);
      const hms::Patient *pAddr = system.findPatient("P001");
      check("Interactive: State protected, Address still 700 Cyber St", pAddr != nullptr && pAddr->getAddress() == "700 Cyber St");
    }

    // 10K: Invalid menu choice in update menu
    {
      // Inputs: 1 -> 4 -> P001 -> 9 -> 0 -> 4
      const std::string out = runInteractive(system, "1\n4\nP001\n9\n0\n4\n");
      check("Interactive: Rejects invalid choice 9", out.find("Invalid choice. Enter 0 to 7.") != std::string::npos);
    }
  }

  // Cleanup test directory
  std::filesystem::remove_all(testDir, ec);

  std::cout << "\n=======================================================\n";
  std::cout << "Tests Passed: " << g_passed << ", Failed: " << g_failed << '\n';
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
