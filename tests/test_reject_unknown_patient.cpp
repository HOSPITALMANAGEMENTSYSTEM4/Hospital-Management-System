// TC-APT-02  Reject Unknown Patient (HMS-F-016)
//
// Acceptance Criteria:
//   - When a booking uses an unregistered Patient ID:
//       1. The returned error message must be exactly "Patient not found".
//       2. The appointment count must remain unchanged.
//       3. The complete contents of appointments.csv must remain byte-for-byte identical.
//       4. No appointment ID may be generated or returned (remains empty).
//       5. No appointment record may be created.
//   - Test uses isolated test CSV files, never root-level user files.
//   - Seeds a valid registered doctor, at least one registered patient, and an existing appointment.
//   - Verifies programmatic APIs and interactive console booking workflow.

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

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
    if (!line.empty() && line.rfind("AppointmentID", 0) != 0 &&
        line.rfind("appointment_id", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::remove(path.c_str());
}

} // namespace

int main() {
  std::cout << "=== Running TC-APT-02 / HMS-F-016 Reject Unknown Patient Tests ===\n\n";

  const std::string testPatientCsv = "build/test_reject_patient_patients.csv";
  const std::string testDoctorCsv = "build/test_reject_patient_doctors.csv";
  const std::string testApptCsv = "build/test_reject_patient_appointments.csv";

  // Clean up any stale files from previous runs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // -------------------------------------------------------------------------
  // Setup / Arrange: Seed registered doctor, registered patient, and initial appointment
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding registered doctor, registered patient, and existing appointment --\n";

  std::string seedErr;
  bool patSaved = fm.savePatient(hms::Patient("P001", "Alice Doe", 32, "F", "9876543210", "123 Main St", "None"), seedErr);
  check("Seed registered patient P001", patSaved);

  bool pat2Saved = fm.savePatient(hms::Patient("P002", "Bob Smith", 48, "M", "9123456789", "456 Oak St", "Hypertension"), seedErr);
  check("Seed registered patient P002", pat2Saved);

  bool docSaved = fm.saveDoctor(hms::Doctor("DOC101", "Dr. Gregory House", "Diagnostics", "Medicine", "9876543210", "Mon-Fri 09:00-17:00"), seedErr);
  check("Seed registered doctor DOC101", docSaved);

  bool doc2Saved = fm.saveDoctor(hms::Doctor("DOC102", "Dr. James Wilson", "Oncology", "Oncology Dept", "9123456789", "Mon-Fri 10:00-18:00"), seedErr);
  check("Seed registered doctor DOC102", doc2Saved);

  // Seed baseline appointment record (A0001)
  hms::HospitalSystem system(fm);
  std::string initialApptId;
  std::string initialError;
  bool initialBooked = system.bookAppointment("P001", "DOC101", "2026-11-01", "09:30", initialApptId, initialError);
  check("Seed existing appointment A0001", initialBooked && initialApptId == "A0001");

  // Read baseline state of appointments.csv
  const std::string baselineApptContent = readFile(testApptCsv);
  const int baselineApptCount = countDataRows(testApptCsv);
  const std::string baselinePatContent = readFile(testPatientCsv);
  const std::string baselineDocContent = readFile(testDoctorCsv);

  // Guard: Fail immediately if baseline file content comparison would be trivial or meaningless
  check("Guard: appointments.csv exists and is not empty before rejection tests", !baselineApptContent.empty());
  check("Guard: appointments.csv has at least 1 existing appointment record", baselineApptCount >= 1);
  check("Guard: patient P001 exists in repository", fm.patientIdExists("P001"));
  check("Guard: patient P999 does not exist in repository", !fm.patientIdExists("P999"));
  check("Guard: patient P000 does not exist in repository", !fm.patientIdExists("P000"));

  // =========================================================================
  // Section 1: Programmatic API with errorMessage (TC-APT-02 Core Requirement)
  // =========================================================================
  std::cout << "\n-- 1. Programmatic API: Reject Unknown Patient ID (P999) --\n";
  {
    std::string outApptId = "";
    std::string errorMsg = "";

    bool success = system.bookAppointment("P999", "DOC101", "2026-11-02", "10:00", outApptId, errorMsg);

    check("Booking with unknown patient P999 returns false", !success);
    check("Exact error message is 'Patient not found'", errorMsg == "Patient not found");
    check("Output appointment ID remains empty (no ID generated)", outApptId.empty());
    check("Appointment count remains unchanged (still baseline count)", countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv is byte-for-byte identical to baseline", readFile(testApptCsv) == baselineApptContent);
    check("patients.csv is byte-for-byte untouched", readFile(testPatientCsv) == baselinePatContent);
    check("doctors.csv is byte-for-byte untouched", readFile(testDoctorCsv) == baselineDocContent);

    // Verify system state via appointment lookup
    const auto appointments = system.listAppointments("ALL");
    check("Appointment listing count matches baseline", static_cast<int>(appointments.size()) == baselineApptCount);
    for (const auto &appt : appointments) {
      check("No appointment record created with unknown patient P999", appt.getPatientId() != "P999");
    }
  }

  // =========================================================================
  // Section 2: Programmatic API with OpResult
  // =========================================================================
  std::cout << "\n-- 2. OpResult API: Reject Unknown Patient ID (P000) --\n";
  {
    std::string outApptId = "";
    hms::OpResult result = system.bookAppointment("P000", "DOC101", "2026-11-03", "11:00", outApptId);

    check("OpResult success is false for unknown patient P000", !result.success);
    check("OpResult message is exactly 'Patient not found'", result.message == "Patient not found");
    check("OpResult output appointment ID remains empty", outApptId.empty());
    check("Appointment count remains unchanged after OpResult call", countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv remains byte-for-byte identical after OpResult call", readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 3: Various unknown patient formats
  // =========================================================================
  std::cout << "\n-- 3. Additional Unknown Patient ID Patterns --\n";
  {
    const std::vector<std::string> unknownIds = {
      "UNKNOWN_PATIENT",
      "P9999",
      "NONEXISTENT",
      "p001_not_found"
    };

    for (const auto &badId : unknownIds) {
      std::string outId = "";
      std::string err = "";
      bool res = system.bookAppointment(badId, "DOC101", "2026-11-04", "14:00", outId, err);

      check(("Rejection of " + badId + " returns false").c_str(), !res);
      check(("Rejection of " + badId + " has error 'Patient not found'").c_str(), err == "Patient not found");
      check(("Rejection of " + badId + " leaves output ID empty").c_str(), outId.empty());
    }

    check("Appointment count still unchanged after multiple rejections", countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv still byte-for-byte identical after multiple rejections", readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 4: Interactive Console Workflow Rejection
  // =========================================================================
  std::cout << "\n-- 4. Interactive Console Booking Workflow with Unknown Patient --\n";
  {
    // Provide inputs: Patient ID (unknown P999), Doctor ID (valid DOC101), Date (valid), Time (valid)
    std::istringstream input("P999\nDOC101\n2026-11-05\n15:00\n");
    std::ostringstream output;

    auto *origCin = std::cin.rdbuf(input.rdbuf());
    auto *origCout = std::cout.rdbuf(output.rdbuf());

    system.bookAppointment();

    std::cin.rdbuf(origCin);
    std::cout.rdbuf(origCout);

    const std::string consoleOutput = output.str();

    check("Console displays exact 'Patient not found' rejection message",
          consoleOutput.find("Patient not found") != std::string::npos);
    check("Console does NOT display success message",
          consoleOutput.find("Appointment booked successfully") == std::string::npos);
    check("Console does NOT display generated Appointment ID",
          consoleOutput.find("Appointment ID:") == std::string::npos);
    check("Appointment count remains unchanged after interactive rejection",
          countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv remains byte-for-byte identical after interactive rejection",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 5: Verify System Remains Fully Functional For Valid Booking
  // =========================================================================
  std::cout << "\n-- 5. System Integrity: Valid Booking Succeeds After Rejections --\n";
  {
    std::string outApptId = "";
    std::string errorMsg = "";

    // Book appointment for registered patient P002 with doctor DOC102
    bool ok = system.bookAppointment("P002", "DOC102", "2026-11-06", "16:00", outApptId, errorMsg);

    check("Subsequent booking with valid patient P002 succeeds", ok);
    check("Sequential appointment ID A0002 generated", outApptId == "A0002");
    check("appointments.csv now has exactly baseline + 1 data rows", countDataRows(testApptCsv) == baselineApptCount + 1);

    const auto loaded = fm.loadAppointments();
    check("Loaded appointments count is 2", loaded.size() == 2);
    if (loaded.size() >= 2) {
      check("Initial appointment preserved intact", loaded[0].getId() == "A0001" && loaded[0].getPatientId() == "P001");
      check("New appointment created correctly", loaded[1].getId() == "A0002" && loaded[1].getPatientId() == "P002");
    }
  }

  // Clean up isolated test files
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  std::cout << "\n=======================================================\n";
  std::cout << "TC-APT-02 Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
