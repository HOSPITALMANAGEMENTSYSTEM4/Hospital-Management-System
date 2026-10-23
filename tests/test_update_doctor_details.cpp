// TC-DOC-06  Update Doctor Details (HMS-F-013, HMS-F-014)
//
// Acceptance Criteria:
//   - A valid update to an existing doctor is saved to doctors.csv.
//   - The updated values appear when the doctor is viewed, searched, or listed again.
//   - Invalid field values are rejected according to doctor validation rules (VR-01, VR-02, VR-05, VR-08, VR-09, VR-14).
//   - A failed or invalid update leaves the stored doctor record unchanged.
//   - Existing doctor records remain intact when one doctor's details are updated.
//   - Doctor ID remains immutable; no option is provided to change the ID.
//   - Non-existent Doctor ID is rejected with "Doctor not found" and changes nothing.
//   - Update survives application restart (data reload from CSV).
//   - File write failures do not report false success or cause partial data loss.
//   - Interactive console workflow (Doctor Management -> 4) works as specified.
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
    if (!line.empty() && line.rfind("doctor_id", 0) != 0 &&
        line.rfind("DoctorID", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
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
  std::cout << "=== Running TC-DOC-06 / HMS-F-013 Update Doctor Details Tests ===\n\n";

  const std::string testDir = "build/test_tc_doc_06_data";
  std::error_code ec;
  std::filesystem::remove_all(testDir, ec);
  std::filesystem::create_directories(testDir, ec);

  const std::string testPatientCsv = testDir + "/patients.csv";
  const std::string testDoctorCsv = testDir + "/doctors.csv";
  const std::string testApptCsv = testDir + "/appointments.csv";

  // =========================================================================
  // Section 1: Valid Update Modifies Intended Doctor Fields (Programmatic API)
  // =========================================================================
  std::cout << "-- Section 1: Valid update modifies intended doctor fields --\n";
  {
    removeFile(testDoctorCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;
    const hms::Doctor d1("DOC01", "Dr. Gregory House", "Diagnostic Medicine",
                         "Infectious Disease", "9876543210", "Mon-Fri 09:00-17:00");
    check("Seed initial doctor DOC01", fm.saveDoctor(d1, err));

    hms::HospitalSystem system(fm);
    const hms::OpResult res = system.updateDoctor(
        "DOC01", "Dr. Allison Cameron", "Immunology", "Internal Medicine",
        "9876543211", "Tue-Sat 10:00-18:00");

    check("updateDoctor returns success == true", res.success);
    check("updateDoctor returns success message", res.message == "Doctor updated successfully");

    const hms::Doctor *updated = system.findDoctor("DOC01");
    check("Doctor DOC01 found in system", updated != nullptr);
    if (updated) {
      check("Doctor ID remains DOC01", updated->getId() == "DOC01");
      check("Name updated to Dr. Allison Cameron", updated->getName() == "Dr. Allison Cameron");
      check("Specialization updated to Immunology", updated->getSpecialization() == "Immunology");
      check("Department updated to Internal Medicine", updated->getDepartment() == "Internal Medicine");
      check("Contact updated to 9876543211", updated->getContactNumber() == "9876543211");
      check("Availability updated to Tue-Sat 10:00-18:00", updated->getAvailability() == "Tue-Sat 10:00-18:00");
    }
  }

  // =========================================================================
  // Section 2: Doctor ID Remains Unchanged (Case Insensitivity & Immutability)
  // =========================================================================
  std::cout << "\n-- Section 2: Doctor ID remains unchanged --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    // Call update using lowercase "doc01"
    const hms::OpResult res = system.updateDoctor(
        "doc01", "Dr. James Wilson", "Oncology", "Oncology Dept",
        "9876543212", "Mon-Fri 08:00-16:00");

    check("Update using lowercase 'doc01' succeeds", res.success);

    const hms::Doctor *d = system.findDoctor("DOC01");
    check("Doctor ID is still normalized uppercase 'DOC01'", d != nullptr && d->getId() == "DOC01");
    check("Doctor Name updated", d != nullptr && d->getName() == "Dr. James Wilson");

    // Overload taking Doctor object with different casing ID
    const hms::Doctor replacement("doc01", "Dr. Lisa Cuddy", "Endocrinology", "Administration",
                                  "9876543213", "Mon-Fri 09:00-17:00");
    const hms::OpResult res2 = system.updateDoctor(replacement);
    check("updateDoctor(Doctor) succeeds", res2.success);

    const hms::Doctor *d2 = system.findDoctor("DOC01");
    check("Doctor ID remains DOC01 after object update", d2 != nullptr && d2->getId() == "DOC01");
    check("Name updated to Dr. Lisa Cuddy", d2 != nullptr && d2->getName() == "Dr. Lisa Cuddy");
  }

  // =========================================================================
  // Section 3: Updated Values Saved to doctors.csv Immediately
  // =========================================================================
  std::cout << "\n-- Section 3: Updated values saved to CSV immediately --\n";
  {
    const std::string csvContent = readFile(testDoctorCsv);
    check("CSV file contains updated name", csvContent.find("Dr. Lisa Cuddy") != std::string::npos);
    check("CSV file contains updated specialization", csvContent.find("Endocrinology") != std::string::npos);
    check("CSV file contains updated department", csvContent.find("Administration") != std::string::npos);
    check("CSV file does not contain old name 'Dr. Gregory House'", csvContent.find("Dr. Gregory House") == std::string::npos);
    check("CSV file does not contain old name 'Dr. Allison Cameron'", csvContent.find("Dr. Allison Cameron") == std::string::npos);
    check("CSV contains exactly 1 data row (no duplicates appended)", countDataRows(testDoctorCsv) == 1);
  }

  // =========================================================================
  // Section 4: Searching and Viewing Doctor Displays Updated Values
  // =========================================================================
  std::cout << "\n-- Section 4: Searching and viewing doctor displays updated values --\n";
  {
    hms::HospitalSystem system(testDir);

    // Search by updated name "Cuddy"
    const std::string searchNameOut = runInteractive(system, "2\n2\nCuddy\n0\n4\n");
    check("Search by updated name displays Doctor ID: DOC01", searchNameOut.find("Doctor ID: DOC01") != std::string::npos);
    check("Search by updated name displays Name: Dr. Lisa Cuddy", searchNameOut.find("Name: Dr. Lisa Cuddy") != std::string::npos);
    check("Search by updated name displays Specialization: Endocrinology", searchNameOut.find("Specialization: Endocrinology") != std::string::npos);

    // Search by updated specialization "Endocrinology"
    const std::string searchSpecOut = runInteractive(system, "2\n2\nEndocrinology\n0\n4\n");
    check("Search by updated spec displays Doctor ID: DOC01", searchSpecOut.find("Doctor ID: DOC01") != std::string::npos);

    // Search by old name "House" returns "No matching doctor found"
    const std::string oldSearchOut = runInteractive(system, "2\n2\nHouse\n0\n4\n");
    check("Search by old name does not find doctor", oldSearchOut.find("No matching doctor found") != std::string::npos);

    // View All Doctors (Option 3 in Doctor menu)
    const std::string listOut = runInteractive(system, "2\n3\n0\n4\n");
    check("Doctor list shows updated name", listOut.find("Dr. Lisa Cuddy") != std::string::npos);
    check("Doctor list shows updated specialization", listOut.find("Endocrinology") != std::string::npos);
  }

  // =========================================================================
  // Section 5: Update Survives Application Restart (Reloading CSV)
  // =========================================================================
  std::cout << "\n-- Section 5: Update survives application restart --\n";
  {
    // Simulate restart by creating completely new HospitalSystem instance
    hms::FileManager newFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem restartedSystem(newFm);

    const hms::Doctor *reloaded = restartedSystem.findDoctor("DOC01");
    check("Reloaded doctor found after restart", reloaded != nullptr);
    if (reloaded) {
      check("Reloaded ID is DOC01", reloaded->getId() == "DOC01");
      check("Reloaded name is Dr. Lisa Cuddy", reloaded->getName() == "Dr. Lisa Cuddy");
      check("Reloaded specialization is Endocrinology", reloaded->getSpecialization() == "Endocrinology");
      check("Reloaded department is Administration", reloaded->getDepartment() == "Administration");
      check("Reloaded contact is 9876543213", reloaded->getContactNumber() == "9876543213");
      check("Reloaded availability is Mon-Fri 09:00-17:00", reloaded->getAvailability() == "Mon-Fri 09:00-17:00");
    }
  }

  // =========================================================================
  // Section 6: Reject Unknown Doctor ID Without Modifying Data
  // =========================================================================
  std::cout << "\n-- Section 6: Reject unknown doctor ID --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    const std::string beforeCsv = readFile(testDoctorCsv);
    const int beforeCount = countDataRows(testDoctorCsv);

    std::string errMsg;
    const bool ok = system.updateDoctor(
        "DOC999", "Ghost Doctor", "Shadow", "Nowhere", "9876543210", "Never", errMsg);

    check("Updating non-existent ID DOC999 returns false", !ok);
    check("Error message is 'Doctor not found'", errMsg == "Doctor not found");
    check("CSV file unchanged after unknown ID attempt", readFile(testDoctorCsv) == beforeCsv);
    check("Doctor count unchanged", countDataRows(testDoctorCsv) == beforeCount);
  }

  // =========================================================================
  // Section 7: Reject Invalid Field Values (VR-01, VR-02, VR-05, VR-08, VR-09, VR-14)
  // =========================================================================
  std::cout << "\n-- Section 7: Reject invalid field values --\n";
  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem system(fm);

    const std::string baselineCsv = readFile(testDoctorCsv);

    auto testInvalidField = [&](const char *label,
                                const std::string &name,
                                const std::string &spec,
                                const std::string &dept,
                                const std::string &contact,
                                const std::string &avail) {
      std::string err;
      const bool ok = system.updateDoctor("DOC01", name, spec, dept, contact, avail, err);
      check(label, !ok && !err.empty());
      check("State protected: CSV unchanged", readFile(testDoctorCsv) == baselineCsv);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("State protected: stored record unchanged", d != nullptr && d->getName() == "Dr. Lisa Cuddy");
    };

    // VR-02: Name validation
    testInvalidField("VR-02: Reject empty name",
                     "", "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-02: Reject name > 50 characters",
                     std::string(51, 'A'), "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-02: Reject disallowed characters in name",
                     "Dr. Cuddy@123", "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject formula prefix in name (=)",
                     "=Dr. Cuddy", "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject comma in name",
                     "Cuddy, Lisa", "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject newline in name",
                     "Dr. Cuddy\nLisa", "Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");

    // VR-08: Specialization validation
    testInvalidField("VR-08: Reject empty specialization",
                     "Dr. Lisa Cuddy", "", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-08: Reject specialization > 40 characters",
                     "Dr. Lisa Cuddy", std::string(41, 'S'), "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject formula prefix in specialization",
                     "Dr. Lisa Cuddy", "=Endocrinology", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject comma in specialization",
                     "Dr. Lisa Cuddy", "Cardio, Neuro", "Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject newline in specialization",
                     "Dr. Lisa Cuddy", "Cardio\nNeuro", "Administration", "9876543213", "Mon-Fri 09:00-17:00");

    // VR-08: Department validation
    testInvalidField("VR-08: Reject empty department",
                     "Dr. Lisa Cuddy", "Endocrinology", "", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-08: Reject department > 40 characters",
                     "Dr. Lisa Cuddy", "Endocrinology", std::string(41, 'D'), "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject formula prefix in department",
                     "Dr. Lisa Cuddy", "Endocrinology", "=Administration", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject comma in department",
                     "Dr. Lisa Cuddy", "Endocrinology", "Dept A, Dept B", "9876543213", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject newline in department",
                     "Dr. Lisa Cuddy", "Endocrinology", "Dept A\nDept B", "9876543213", "Mon-Fri 09:00-17:00");

    // VR-05: Contact number validation
    testInvalidField("VR-05: Reject contact < 10 digits",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "987654321", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-05: Reject contact > 10 digits",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "98765432101", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-05: Reject non-digit in contact",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "987654321A", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject formula prefix in contact",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "=987654321", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject comma in contact",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "98765,4321", "Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject newline in contact",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "98765\n4321", "Mon-Fri 09:00-17:00");

    // VR-09: Availability validation
    testInvalidField("VR-09: Reject empty availability",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "9876543213", "");
    testInvalidField("VR-09: Reject availability > 60 characters",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "9876543213", std::string(61, 'X'));
    testInvalidField("VR-14: Reject formula prefix in availability",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "9876543213", "=Mon-Fri 09:00-17:00");
    testInvalidField("VR-14: Reject comma in availability",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "9876543213", "Mon, Fri 09:00-17:00");
    testInvalidField("VR-14: Reject newline in availability",
                     "Dr. Lisa Cuddy", "Endocrinology", "Administration", "9876543213", "Mon-Fri\n09:00-17:00");
  }

  // =========================================================================
  // Section 8: Updating One Doctor Does Not Modify Other Doctor Records
  // =========================================================================
  std::cout << "\n-- Section 8: Updating one doctor does not modify other records --\n";
  {
    removeFile(testDoctorCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;

    const hms::Doctor d1("DOC01", "Dr. One", "Cardiology", "Cardio Dept", "9876543210", "Mon-Fri 09:00-17:00");
    const hms::Doctor d2("DOC02", "Dr. Two", "Neurology", "Neuro Dept", "9876543220", "Tue-Sat 10:00-18:00");
    const hms::Doctor d3("DOC03", "Dr. Three", "Pediatrics", "Pedia Dept", "9876543230", "Wed-Sun 08:00-16:00");

    check("Seed doctor DOC01", fm.saveDoctor(d1, err));
    check("Seed doctor DOC02", fm.saveDoctor(d2, err));
    check("Seed doctor DOC03", fm.saveDoctor(d3, err));

    hms::HospitalSystem system(fm);

    // Update DOC02 only
    const hms::OpResult res = system.updateDoctor(
        "DOC02", "Dr. Two Updated", "Neurosurgery", "Surgery Dept", "9876543229", "Mon-Thu 09:00-15:00");
    check("Update DOC02 succeeded", res.success);

    // Verify DOC01 is completely untouched
    const hms::Doctor *checkD1 = system.findDoctor("DOC01");
    check("DOC01 still exists", checkD1 != nullptr);
    if (checkD1) {
      check("DOC01 name unchanged", checkD1->getName() == "Dr. One");
      check("DOC01 specialization unchanged", checkD1->getSpecialization() == "Cardiology");
      check("DOC01 contact unchanged", checkD1->getContactNumber() == "9876543210");
      check("DOC01 availability unchanged", checkD1->getAvailability() == "Mon-Fri 09:00-17:00");
    }

    // Verify DOC02 was updated
    const hms::Doctor *checkD2 = system.findDoctor("DOC02");
    check("DOC02 updated name", checkD2 != nullptr && checkD2->getName() == "Dr. Two Updated");
    check("DOC02 updated specialization", checkD2 != nullptr && checkD2->getSpecialization() == "Neurosurgery");
    check("DOC02 updated contact", checkD2 != nullptr && checkD2->getContactNumber() == "9876543229");

    // Verify DOC03 is completely untouched
    const hms::Doctor *checkD3 = system.findDoctor("DOC03");
    check("DOC03 still exists", checkD3 != nullptr);
    if (checkD3) {
      check("DOC03 name unchanged", checkD3->getName() == "Dr. Three");
      check("DOC03 department unchanged", checkD3->getDepartment() == "Pedia Dept");
      check("DOC03 availability unchanged", checkD3->getAvailability() == "Wed-Sun 08:00-16:00");
    }

    // Verify total count in CSV is still exactly 3
    check("Total record count remains exactly 3", countDataRows(testDoctorCsv) == 3);
  }

  // =========================================================================
  // Section 9: File Write Failure Safety (Non-destructive atomicity)
  // =========================================================================
  std::cout << "\n-- Section 9: File write failure safety --\n";
  {
    const std::string invalidPath = testDir + "/blocked_dir/deep/nested/doctors.csv";
    const std::string blocker = testDir + "/blocked_dir";
    {
      std::ofstream b(blocker);
      b << "blocking file";
    }

    hms::FileManager failingFm(testPatientCsv, invalidPath, testApptCsv);
    const hms::Doctor validDoctor("DOC01", "Dr. Jane Doe", "Cardiology", "Cardio", "9876543210", "Mon-Fri 09:00-17:00");

    std::string err;
    const bool saveFailed = failingFm.updateDoctor(validDoctor, err);
    check("Update doctor on invalid path returns false", !saveFailed);
    check("Error message indicates save failure", !err.empty());

    removeFile(blocker);
  }

  // =========================================================================
  // Section 10: Interactive Console Workflow (Doctor Management -> 4: Update)
  // =========================================================================
  std::cout << "\n-- Section 10: Interactive Console Workflow --\n";
  {
    removeFile(testDoctorCsv);
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;
    const hms::Doctor d1("DOC01", "Dr. Gregory House", "Diagnostic Medicine",
                         "Infectious Disease", "9876543210", "Mon-Fri 09:00-17:00");
    check("Seed interactive doctor DOC01", fm.saveDoctor(d1, err));

    hms::HospitalSystem system(testDir);

    // 10A: Unknown Doctor ID entered at Update Doctor prompt
    {
      // Inputs: 2 (Doctor Menu) -> 4 (Update) -> DOC999 (ID) -> 0 (Return) -> 4 (Exit)
      const std::string out = runInteractive(system, "2\n4\nDOC999\n0\n4\n");
      check("Interactive: displays 'Doctor not found'", out.find("Doctor not found") != std::string::npos);
    }

    // 10B: Update individual field: choice 1 (Name)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 1 -> Dr. Eric Foreman -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n1\nDr. Eric Foreman\n0\n4\n");
      check("Interactive: confirms successful update", out.find("Doctor updated successfully") != std::string::npos);

      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Name changed to Dr. Eric Foreman", d != nullptr && d->getName() == "Dr. Eric Foreman");
      check("Interactive: Specialization kept as Diagnostic Medicine", d != nullptr && d->getSpecialization() == "Diagnostic Medicine");
      check("Interactive: Department kept as Infectious Disease", d != nullptr && d->getDepartment() == "Infectious Disease");
    }

    // 10C: Update individual field: choice 2 (Specialization)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 2 -> Neurology -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n2\nNeurology\n0\n4\n");
      check("Interactive: Specialization update succeeds", out.find("Doctor updated successfully") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Specialization changed to Neurology", d != nullptr && d->getSpecialization() == "Neurology");
    }

    // 10D: Update individual field: choice 3 (Department)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 3 -> Neuro Dept -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n3\nNeuro Dept\n0\n4\n");
      check("Interactive: Department update succeeds", out.find("Doctor updated successfully") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Department changed to Neuro Dept", d != nullptr && d->getDepartment() == "Neuro Dept");
    }

    // 10E: Update individual field: choice 4 (Contact number)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 4 -> 9123456789 -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n4\n9123456789\n0\n4\n");
      check("Interactive: Contact update succeeds", out.find("Doctor updated successfully") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Contact changed to 9123456789", d != nullptr && d->getContactNumber() == "9123456789");
    }

    // 10F: Update individual field: choice 5 (Availability)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 5 -> Mon-Thu 10:00-16:00 -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n5\nMon-Thu 10:00-16:00\n0\n4\n");
      check("Interactive: Availability update succeeds", out.find("Doctor updated successfully") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Availability changed to Mon-Thu 10:00-16:00", d != nullptr && d->getAvailability() == "Mon-Thu 10:00-16:00");
    }

    // 10G: Update all fields: choice 6 (All fields)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 6 -> Dr. Robert Chase -> Surgery -> Surgical Dept -> 9998887776 -> Mon-Fri 07:00-15:00 -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n6\nDr. Robert Chase\nSurgery\nSurgical Dept\n9998887776\nMon-Fri 07:00-15:00\n0\n4\n");
      check("Interactive: All fields update succeeds", out.find("Doctor updated successfully") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: Name is Dr. Robert Chase", d != nullptr && d->getName() == "Dr. Robert Chase");
      check("Interactive: Specialization is Surgery", d != nullptr && d->getSpecialization() == "Surgery");
      check("Interactive: Department is Surgical Dept", d != nullptr && d->getDepartment() == "Surgical Dept");
      check("Interactive: Contact is 9998887776", d != nullptr && d->getContactNumber() == "9998887776");
      check("Interactive: Availability is Mon-Fri 07:00-15:00", d != nullptr && d->getAvailability() == "Mon-Fri 07:00-15:00");
    }

    // 10H: Update menu offers no Doctor ID edit and offers Return (0)
    {
      // Inputs: 2 -> 4 -> DOC01 -> 0 -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n0\n0\n4\n");
      check("Interactive: Menu contains 'Select field to update'", out.find("Select field to update") != std::string::npos);
      check("Interactive: Menu contains 1. Name", out.find("1. Name") != std::string::npos);
      check("Interactive: Menu does NOT offer Doctor ID option", out.find("Doctor ID edit") == std::string::npos &&
                                                                  out.find("0. Return") != std::string::npos);
    }

    // 10I: Invalid input at update field prompt rejected
    {
      // Inputs: 2 -> 4 -> DOC01 -> 1 -> Chase, Robert (invalid comma in name) -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n1\nChase, Robert\n0\n4\n");
      check("Interactive: Reject invalid name with error message",
            out.find("must contain 1-50 permitted characters") != std::string::npos);
      const hms::Doctor *d = system.findDoctor("DOC01");
      check("Interactive: State protected, Name still Dr. Robert Chase", d != nullptr && d->getName() == "Dr. Robert Chase");

      // Inputs: 2 -> 4 -> DOC01 -> 2 -> Cardio, Neuro (invalid comma in specialization) -> 0 -> 4
      const std::string outSpec = runInteractive(system, "2\n4\nDOC01\n2\nCardio, Neuro\n0\n4\n");
      check("Interactive: Reject invalid specialization comma with error message",
            outSpec.find("commas and line breaks are not allowed") != std::string::npos);
      const hms::Doctor *dSpec = system.findDoctor("DOC01");
      check("Interactive: State protected, Specialization still Surgery", dSpec != nullptr && dSpec->getSpecialization() == "Surgery");
    }

    // 10J: Invalid menu choice in update menu
    {
      // Inputs: 2 -> 4 -> DOC01 -> 9 -> 0 -> 4
      const std::string out = runInteractive(system, "2\n4\nDOC01\n9\n0\n4\n");
      check("Interactive: Rejects invalid choice 9", out.find("Invalid choice. Enter 0 to 6.") != std::string::npos);
    }
  }

  // Cleanup test directory
  std::filesystem::remove_all(testDir, ec);

  std::cout << "\n=======================================================\n";
  std::cout << "Tests Passed: " << g_passed << ", Failed: " << g_failed << '\n';
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
