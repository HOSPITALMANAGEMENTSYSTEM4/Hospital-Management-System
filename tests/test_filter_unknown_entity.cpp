// TC-APT-12: Filter Unknown Entity in Appointment Views
// Traceability:
//   - SRS Requirement: HMS-F-022, UC-08 (exception 3a)
//   - GitHub Issue: #121
//   - Test Case: TC-APT-12_FilterUnknownEntity
//
// Acceptance Requirements:
//   1. Patient existence must be verified before filtering appointments by Patient ID.
//   2. Doctor existence must be verified before filtering appointments by Doctor ID.
//   3. Nonexistent Patient IDs must display exact message "Patient not found".
//   4. Nonexistent Doctor IDs must display exact message "Doctor not found".
//   5. Registered patients with no appointments must display exact message "No appointments found.".
//   6. Registered doctors with no appointments must display exact message "No appointments found.".
//   7. Existing appointment filtering and chronological sorting functionality must remain unaffected.
//   8. Filtering by nonexistent or valid IDs must not modify any patient, doctor, or appointment records.
//   9. Case-insensitive entity IDs and whitespace trimming must be honoured.
//  10. Interactive console menu filtering paths must behave consistently with underlying calls.
//
// Test Environment:
//   - Uses isolated test CSV files in build/test_filter_unknown_entity_dir/, never root user CSV files.

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

namespace fs = std::filesystem;

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
    if (!line.empty() &&
        line.rfind("AppointmentID", 0) != 0 && line.rfind("appointment_id", 0) != 0 &&
        line.rfind("PatientID", 0) != 0 && line.rfind("patient_id", 0) != 0 &&
        line.rfind("DoctorID", 0) != 0 && line.rfind("doctor_id", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::error_code ec;
  fs::remove(path, ec);
}

std::string captureViewAppointments(hms::HospitalSystem &system,
                                   const std::string &filterType = "ALL",
                                   const std::string &filterId = "") {
  std::ostringstream outStream;
  auto *origCout = std::cout.rdbuf(outStream.rdbuf());

  system.viewAppointments(filterType, filterId);

  std::cout.rdbuf(origCout);
  return outStream.str();
}

std::string runInteractive(hms::HospitalSystem &system, const std::string &input) {
  std::istringstream inStream(input);
  std::ostringstream outStream;

  auto *origCin = std::cin.rdbuf(inStream.rdbuf());
  auto *origCout = std::cout.rdbuf(outStream.rdbuf());

  system.run();

  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  return outStream.str();
}

} // namespace

int main() {
  std::cout << "=== Running TC-APT-12 Filter Unknown Entity in Appointment Views Tests ===\n";
  std::cout << "=== (Traceability: HMS-F-022, UC-08 exception 3a, GitHub Issue #121) ===\n\n";

  const fs::path testDir = "build/test_filter_unknown_entity_dir";
  std::error_code ec;
  fs::create_directories(testDir, ec);

  const std::string testPatientCsv = (testDir / "patients.csv").string();
  const std::string testDoctorCsv = (testDir / "doctors.csv").string();
  const std::string testApptCsv = (testDir / "appointments.csv").string();

  // Snapshot root CSV content before test to ensure zero root modification
  const std::string rootPatientBefore = readFile("patients.csv");
  const std::string rootDoctorBefore = readFile("doctors.csv");
  const std::string rootApptBefore = readFile("appointments.csv");

  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
  std::string seedErr;

  // -------------------------------------------------------------------------
  // Setup: Seed test entities
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding test records --\n";

  // Patients:
  // P001: Alice Smith (has appointments)
  // P002: Bob Jones (has appointments)
  // P003: Carol White (registered, NO appointments)
  check("Seed patient P001",
        fm.savePatient(hms::Patient("P001", "Alice Smith", 28, "F", "9876543210", "10 Main St", "None"), seedErr));
  check("Seed patient P002",
        fm.savePatient(hms::Patient("P002", "Bob Jones", 42, "M", "9123456789", "20 Oak St", "None"), seedErr));
  check("Seed patient P003 (no appointments)",
        fm.savePatient(hms::Patient("P003", "Carol White", 35, "F", "9811223344", "30 Pine St", "Asthma"), seedErr));

  // Doctors:
  // D001: Dr. John Doe (has appointments)
  // D002: Dr. Jane Roe (has appointments)
  // D003: Dr. Jack Black (registered, NO appointments)
  check("Seed doctor D001",
        fm.saveDoctor(hms::Doctor("D001", "Dr. John Doe", "Cardiology", "Cardiology Dept",
                                  "9876501234", "Mon-Fri 09:00-17:00"), seedErr));
  check("Seed doctor D002",
        fm.saveDoctor(hms::Doctor("D002", "Dr. Jane Roe", "Neurology", "Neurology Dept",
                                  "9876505678", "Mon-Fri 08:00-16:00"), seedErr));
  check("Seed doctor D003 (no appointments)",
        fm.saveDoctor(hms::Doctor("D003", "Dr. Jack Black", "Orthopedics", "Orthopedics Dept",
                                  "9876509999", "Tue-Sat 10:00-18:00"), seedErr));

  // Appointments:
  // A0001: P001, D001, 2026-11-20 14:00 (Booked)
  // A0002: P001, D001, 2026-11-10 09:00 (Booked)
  // A0003: P001, D002, 2026-11-10 11:00 (Cancelled)
  // A0004: P002, D002, 2026-11-15 10:00 (Booked)
  check("Seed appointment A0001",
        fm.saveAppointment(hms::Appointment("A0001", "P001", "D001", "2026-11-20", "14:00", hms::AppointmentStatus::Booked), seedErr));
  check("Seed appointment A0002",
        fm.saveAppointment(hms::Appointment("A0002", "P001", "D001", "2026-11-10", "09:00", hms::AppointmentStatus::Booked), seedErr));
  check("Seed appointment A0003",
        fm.saveAppointment(hms::Appointment("A0003", "P001", "D002", "2026-11-10", "11:00", hms::AppointmentStatus::Cancelled), seedErr));
  check("Seed appointment A0004",
        fm.saveAppointment(hms::Appointment("A0004", "P002", "D002", "2026-11-15", "10:00", hms::AppointmentStatus::Booked), seedErr));

  const std::string baselinePatientCsv = readFile(testPatientCsv);
  const std::string baselineDoctorCsv = readFile(testDoctorCsv);
  const std::string baselineApptCsv = readFile(testApptCsv);

  hms::HospitalSystem system(fm);

  // =========================================================================
  // Scenario 1: Existing Patient ID with appointments
  // =========================================================================
  std::cout << "\n-- Scenario 1: Existing Patient ID with Appointments --\n";
  {
    const std::string outP1 = captureViewAppointments(system, "PATIENT", "P001");
    check("S1: Contains header", outP1.find("--- View Appointments ---") != std::string::npos);
    check("S1: Contains A0001", outP1.find("A0001") != std::string::npos);
    check("S1: Contains A0002", outP1.find("A0002") != std::string::npos);
    check("S1: Contains A0003 (Cancelled)", outP1.find("A0003") != std::string::npos && outP1.find("Cancelled") != std::string::npos);
    check("S1: Contains patient name Alice Smith", outP1.find("Alice Smith") != std::string::npos);
    check("S1: Does not contain P002 appointment A0004", outP1.find("A0004") == std::string::npos);
    check("S1: Does not display 'No appointments found.'", outP1.find("No appointments found.") == std::string::npos);
    check("S1: Does not display 'Patient not found'", outP1.find("Patient not found") == std::string::npos);
  }

  // =========================================================================
  // Scenario 2: Existing Doctor ID with appointments
  // =========================================================================
  std::cout << "\n-- Scenario 2: Existing Doctor ID with Appointments --\n";
  {
    const std::string outD1 = captureViewAppointments(system, "DOCTOR", "D001");
    check("S2: Contains header", outD1.find("--- View Appointments ---") != std::string::npos);
    check("S2: Contains D001 appointment A0001", outD1.find("A0001") != std::string::npos);
    check("S2: Contains D001 appointment A0002", outD1.find("A0002") != std::string::npos);
    check("S2: Contains doctor name Dr. John Doe", outD1.find("Dr. John Doe") != std::string::npos);
    check("S2: Does not contain D002 appointment A0003", outD1.find("A0003") == std::string::npos);
    check("S2: Does not contain D002 appointment A0004", outD1.find("A0004") == std::string::npos);
    check("S2: Does not display 'No appointments found.'", outD1.find("No appointments found.") == std::string::npos);
    check("S2: Does not display 'Doctor not found'", outD1.find("Doctor not found") == std::string::npos);
  }

  // =========================================================================
  // Scenario 3: Nonexistent Patient ID displays "Patient not found"
  // =========================================================================
  std::cout << "\n-- Scenario 3: Nonexistent Patient ID --\n";
  {
    const std::string outUnknownPat = captureViewAppointments(system, "PATIENT", "P999");
    check("S3: Displays exact error 'Patient not found'",
          outUnknownPat.find("Patient not found") != std::string::npos);
    check("S3: Does NOT display 'No appointments found.'",
          outUnknownPat.find("No appointments found.") == std::string::npos);
    check("S3: Does not display any appointment records",
          outUnknownPat.find("Appointment ID:") == std::string::npos);

    // Another arbitrary unknown ID
    const std::string outUnknownPat2 = captureViewAppointments(system, "PATIENT", "PAT_NONEXISTENT");
    check("S3: Second unknown ID displays 'Patient not found'",
          outUnknownPat2.find("Patient not found") != std::string::npos);
    check("S3: Second unknown ID does NOT display 'No appointments found.'",
          outUnknownPat2.find("No appointments found.") == std::string::npos);
  }

  // =========================================================================
  // Scenario 4: Nonexistent Doctor ID displays "Doctor not found"
  // =========================================================================
  std::cout << "\n-- Scenario 4: Nonexistent Doctor ID --\n";
  {
    const std::string outUnknownDoc = captureViewAppointments(system, "DOCTOR", "D999");
    check("S4: Displays exact error 'Doctor not found'",
          outUnknownDoc.find("Doctor not found") != std::string::npos);
    check("S4: Does NOT display 'No appointments found.'",
          outUnknownDoc.find("No appointments found.") == std::string::npos);
    check("S4: Does not display any appointment records",
          outUnknownDoc.find("Appointment ID:") == std::string::npos);

    // Another arbitrary unknown ID
    const std::string outUnknownDoc2 = captureViewAppointments(system, "DOCTOR", "DOC_UNKNOWN");
    check("S4: Second unknown ID displays 'Doctor not found'",
          outUnknownDoc2.find("Doctor not found") != std::string::npos);
    check("S4: Second unknown ID does NOT display 'No appointments found.'",
          outUnknownDoc2.find("No appointments found.") == std::string::npos);
  }

  // =========================================================================
  // Scenario 5: Existing Patient ID without appointments
  // =========================================================================
  std::cout << "\n-- Scenario 5: Existing Patient ID without Appointments --\n";
  {
    const std::string outNoApptPat = captureViewAppointments(system, "PATIENT", "P003");
    check("S5: Displays exact message 'No appointments found.'",
          outNoApptPat.find("No appointments found.") != std::string::npos);
    check("S5: Does NOT display 'Patient not found'",
          outNoApptPat.find("Patient not found") == std::string::npos);
    check("S5: Does not display any appointment records",
          outNoApptPat.find("Appointment ID:") == std::string::npos);
  }

  // =========================================================================
  // Scenario 6: Existing Doctor ID without appointments
  // =========================================================================
  std::cout << "\n-- Scenario 6: Existing Doctor ID without Appointments --\n";
  {
    const std::string outNoApptDoc = captureViewAppointments(system, "DOCTOR", "D003");
    check("S6: Displays exact message 'No appointments found.'",
          outNoApptDoc.find("No appointments found.") != std::string::npos);
    check("S6: Does NOT display 'Doctor not found'",
          outNoApptDoc.find("Doctor not found") == std::string::npos);
    check("S6: Does not display any appointment records",
          outNoApptDoc.find("Appointment ID:") == std::string::npos);
  }

  // =========================================================================
  // Scenario 7: Chronological sorting preserved
  // =========================================================================
  std::cout << "\n-- Scenario 7: Chronological Sorting Preserved --\n";
  {
    const std::string outP1 = captureViewAppointments(system, "PATIENT", "P001");
    const std::size_t posA0002 = outP1.find("A0002"); // 2026-11-10 09:00
    const std::size_t posA0003 = outP1.find("A0003"); // 2026-11-10 11:00
    const std::size_t posA0001 = outP1.find("A0001"); // 2026-11-20 14:00

    check("S7: A0002 found", posA0002 != std::string::npos);
    check("S7: A0003 found", posA0003 != std::string::npos);
    check("S7: A0001 found", posA0001 != std::string::npos);
    check("S7: Earlier time A0002 (09:00) appears before A0003 (11:00)", posA0002 < posA0003);
    check("S7: Earlier date A0003 (2026-11-10) appears before A0001 (2026-11-20)", posA0003 < posA0001);
  }

  // =========================================================================
  // Scenario 8: Filtering by unknown ID does not modify records (Read-only)
  // =========================================================================
  std::cout << "\n-- Scenario 8: Read-Only Persistence Guarantee --\n";
  {
    // Execute multiple unknown ID lookups
    captureViewAppointments(system, "PATIENT", "P9999");
    captureViewAppointments(system, "DOCTOR", "D9999");
    captureViewAppointments(system, "PATIENT", "P_INVALID");
    captureViewAppointments(system, "DOCTOR", "D_INVALID");

    check("S8: patients.csv is byte-for-byte identical", readFile(testPatientCsv) == baselinePatientCsv);
    check("S8: doctors.csv is byte-for-byte identical", readFile(testDoctorCsv) == baselineDoctorCsv);
    check("S8: appointments.csv is byte-for-byte identical", readFile(testApptCsv) == baselineApptCsv);
    check("S8: Patient row count unchanged", countDataRows(testPatientCsv) == 3);
    check("S8: Doctor row count unchanged", countDataRows(testDoctorCsv) == 3);
    check("S8: Appointment row count unchanged", countDataRows(testApptCsv) == 4);

    // Verify root files in workspace root were never modified
    check("S8: Root patients.csv untouched", readFile("patients.csv") == rootPatientBefore);
    check("S8: Root doctors.csv untouched", readFile("doctors.csv") == rootDoctorBefore);
    check("S8: Root appointments.csv untouched", readFile("appointments.csv") == rootApptBefore);
  }

  // =========================================================================
  // Scenario 9: Case-insensitivity and Whitespace Trimming
  // =========================================================================
  std::cout << "\n-- Scenario 9: Case-Insensitivity & Whitespace Trimming --\n";
  {
    // Lowercase existing patient "p001"
    const std::string outLowerPat = captureViewAppointments(system, "PATIENT", "p001");
    check("S9: Lowercase 'p001' matches P001", outLowerPat.find("A0001") != std::string::npos);
    check("S9: Lowercase 'p001' does not display 'Patient not found'",
          outLowerPat.find("Patient not found") == std::string::npos);

    // Whitespace around existing patient "  P001  "
    const std::string outTrimPat = captureViewAppointments(system, "PATIENT", "  P001  ");
    check("S9: Padded '  P001  ' matches P001", outTrimPat.find("A0001") != std::string::npos);

    // Lowercase existing doctor "d001"
    const std::string outLowerDoc = captureViewAppointments(system, "DOCTOR", "d001");
    check("S9: Lowercase 'd001' matches D001", outLowerDoc.find("A0001") != std::string::npos);
    check("S9: Lowercase 'd001' does not display 'Doctor not found'",
          outLowerDoc.find("Doctor not found") == std::string::npos);

    // Whitespace around existing doctor "  D001  "
    const std::string outTrimDoc = captureViewAppointments(system, "DOCTOR", "  D001  ");
    check("S9: Padded '  D001  ' matches D001", outTrimDoc.find("A0001") != std::string::npos);

    // Lowercase nonexistent IDs still report not found
    const std::string outLowerUnknownPat = captureViewAppointments(system, "PATIENT", "p999");
    check("S9: Lowercase 'p999' displays 'Patient not found'",
          outLowerUnknownPat.find("Patient not found") != std::string::npos);

    const std::string outLowerUnknownDoc = captureViewAppointments(system, "DOCTOR", "d999");
    check("S9: Lowercase 'd999' displays 'Doctor not found'",
          outLowerUnknownDoc.find("Doctor not found") != std::string::npos);
  }

  // =========================================================================
  // Scenario 10: Interactive UI Workflow Verification
  // =========================================================================
  std::cout << "\n-- Scenario 10: Interactive UI Workflow --\n";
  {
    // Interactive sequence:
    // Main menu: 3 (Appointment Management)
    // Appointment menu: 2 (View)
    // View menu: 2 (Filter by Patient) -> "P999" (nonexistent) -> displays "Patient not found"
    // Appointment menu: 2 (View)
    // View menu: 3 (Filter by Doctor) -> "D999" (nonexistent) -> displays "Doctor not found"
    // Appointment menu: 2 (View)
    // View menu: 2 (Filter by Patient) -> "P003" (no appts) -> displays "No appointments found."
    // Appointment menu: 2 (View)
    // View menu: 3 (Filter by Doctor) -> "D003" (no appts) -> displays "No appointments found."
    // Appointment menu: 2 (View)
    // View menu: 2 (Filter by Patient) -> "P001" (with appts) -> displays appointments
    // Appointment menu: 0 (Return)
    // Main menu: 4 (Exit)
    const std::string interactiveInput =
        "3\n"           // Main menu -> Appointment
        "2\n2\nP999\n"  // View -> By Patient -> P999
        "2\n3\nD999\n"  // View -> By Doctor -> D999
        "2\n2\nP003\n"  // View -> By Patient -> P003 (no appts)
        "2\n3\nD003\n"  // View -> By Doctor -> D003 (no appts)
        "2\n2\nP001\n"  // View -> By Patient -> P001 (has appts)
        "0\n"           // Appointment -> Return to Main
        "4\n";          // Main menu -> Exit

    const std::string sessionOut = runInteractive(system, interactiveInput);

    check("S10: Interactive nonexistent patient displays 'Patient not found'",
          sessionOut.find("Patient not found") != std::string::npos);
    check("S10: Interactive nonexistent doctor displays 'Doctor not found'",
          sessionOut.find("Doctor not found") != std::string::npos);
    check("S10: Interactive patient with no appointments displays 'No appointments found.'",
          sessionOut.find("No appointments found.") != std::string::npos);
    check("S10: Interactive valid patient displays A0001",
          sessionOut.find("A0001") != std::string::npos);
    check("S10: Interactive clean exit message displayed",
          sessionOut.find("Exiting Hospital Management System.") != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Cleanup test environment
  // -------------------------------------------------------------------------
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);
  fs::remove(testDir, ec);

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=======================================================\n";
  std::cout << "TC-APT-12 Filter Unknown Entity: "
            << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
