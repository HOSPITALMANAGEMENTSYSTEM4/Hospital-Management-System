// Issue #34: Skip duplicate and orphan records
//
// Test suite covering:
//   1. Skip duplicate Patient IDs in CSV loading (isolated, 0 malformed records).
//   2. Skip duplicate Doctor IDs in CSV loading (isolated, 0 malformed records).
//   3. Skip duplicate Appointment IDs in CSV loading (isolated, 0 malformed records, 0 orphans).
//   4. Skip orphan appointments referencing nonexistent Patient ID.
//   5. Skip orphan appointments referencing nonexistent Doctor ID.
//   6. Skip orphan appointments referencing nonexistent Patient and Doctor IDs.
//   7. Verify cumulative skipped-record counts tallying malformed, duplicate, and orphan records.
//   8. Verify clean valid records result in 0 skipped records and no console warnings.
//   9. Verify deterministic interactive console menu navigation and clean exit.

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
  if (condition) {
    std::cout << "  [PASS] " << message << '\n';
  } else {
    std::cerr << "  [FAIL] " << message << '\n';
    ++failures;
  }
}

void cleanDir(const fs::path &dir) {
  std::error_code ec;
  fs::remove_all(dir, ec);
  fs::create_directories(dir, ec);
}

// ---------------------------------------------------------------------------
// Test 1: Duplicate Patient IDs (Isolated from malformed records)
// ---------------------------------------------------------------------------
void testDuplicatePatientIdsOnly() {
  std::cout << "\n-- Test 1: Duplicate Patient IDs (Isolated) --\n";

  const fs::path dir = fs::path("build") / "test_issue34_patients_dup_only";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    // Valid records
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P002,Bob Jones,40,M,9876543211,456 Oak St,Asthma\n";
    // Duplicate of P001 (exact case)
    csv << "P001,Alice Duplicate,31,F,9876543213,999 Other St,None\n";
    // Duplicate of P002 (case-insensitive: p002)
    csv << "p002,Bob Lowercase,41,M,9876543214,888 Different St,None\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);
  std::size_t skippedCount = 0;
  const auto patients = fm.loadPatients(&skippedCount);

  check(patients.size() == 2, "Exactly 2 unique valid patients loaded");
  check(skippedCount == 2, "Skipped count is exactly 2 (pure duplicate patient records)");

  if (patients.size() == 2) {
    check(patients[0].getId() == "P001" && patients[0].getName() == "Alice Smith",
          "First occurrence P001 retained with original data");
    check(patients[1].getId() == "P002" && patients[1].getName() == "Bob Jones",
          "First occurrence P002 retained with original data");
  }

  // Verify console listing reports skipped records
  hms::HospitalSystem sys(fm);
  std::istringstream in("1\n3\n0\n4\n");
  std::ostringstream out;
  auto *oldCin = std::cin.rdbuf(in.rdbuf());
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.run();
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);

  const std::string displayed = out.str();
  check(displayed.find("2 malformed record(s) skipped") != std::string::npos,
        "listPatients displays warning for 2 skipped duplicate records");
  check(displayed.find("Exiting Hospital Management System.") != std::string::npos,
        "Application cleanly exits after patient menu operation");
}

// ---------------------------------------------------------------------------
// Test 2: Duplicate Doctor IDs (Isolated from malformed records)
// ---------------------------------------------------------------------------
void testDuplicateDoctorIdsOnly() {
  std::cout << "\n-- Test 2: Duplicate Doctor IDs (Isolated) --\n";

  const fs::path dir = fs::path("build") / "test_issue34_doctors_dup_only";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    // Valid records
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "D002,Dr. Wilson,Oncology,Cancer,9876543211,Mon-Fri 10:00-18:00\n";
    // Duplicate of D001 (exact case)
    csv << "D001,Dr. House Duplicate,Surgery,Medicine,9876543213,Mon-Fri 09:00-17:00\n";
    // Duplicate of D002 (case-insensitive: d002)
    csv << "d002,Dr. Wilson Lowercase,Oncology,Cancer,9876543214,Mon-Fri 10:00-18:00\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);
  std::size_t skippedCount = 0;
  const auto doctors = fm.loadDoctors(&skippedCount);

  check(doctors.size() == 2, "Exactly 2 unique valid doctors loaded");
  check(skippedCount == 2, "Skipped count is exactly 2 (pure duplicate doctor records)");

  if (doctors.size() == 2) {
    check(doctors[0].getId() == "D001" && doctors[0].getName() == "Dr. House",
          "First occurrence D001 retained with original data");
    check(doctors[1].getId() == "D002" && doctors[1].getName() == "Dr. Wilson",
          "First occurrence D002 retained with original data");
  }

  // Verify console listing reports skipped records
  hms::HospitalSystem sys(fm);
  std::istringstream in("2\n3\n0\n4\n");
  std::ostringstream out;
  auto *oldCin = std::cin.rdbuf(in.rdbuf());
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.run();
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);

  const std::string displayed = out.str();
  check(displayed.find("2 malformed record(s) skipped") != std::string::npos,
        "listDoctors displays warning for 2 skipped duplicate records");
  check(displayed.find("Exiting Hospital Management System.") != std::string::npos,
        "Application cleanly exits after doctor menu operation");
}

// ---------------------------------------------------------------------------
// Test 3: Duplicate Appointment IDs (Isolated from orphan and malformed records)
// ---------------------------------------------------------------------------
void testDuplicateAppointmentIdsOnly() {
  std::cout << "\n-- Test 3: Duplicate Appointment IDs (Isolated) --\n";

  const fs::path dir = fs::path("build") / "test_issue34_appts_dup_only";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  // Registered patients and doctors
  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P002,Bob Jones,40,M,9876543211,456 Oak St,None\n";
  }
  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "D002,Dr. Wilson,Oncology,Cancer,9876543211,Mon-Fri 10:00-18:00\n";
  }

  // Appointments with duplicates only:
  // 1. A0001 (valid)
  // 2. A0002 (valid)
  // 3. A0001 (duplicate of A0001)
  // 4. a0002 (case-insensitive duplicate of A0002)
  {
    std::ofstream csv(apptPath);
    csv << "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";
    csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
    csv << "A0002,P002,D002,2026-11-16,11:00,Booked\n";
    csv << "A0001,P001,D001,2026-11-17,12:00,Booked\n";
    csv << "a0002,P002,D002,2026-11-18,13:00,Booked\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);
  std::size_t skippedCount = 0;
  const auto appointments = fm.loadAppointments(&skippedCount);

  check(appointments.size() == 2, "Exactly 2 unique valid appointments loaded");
  check(skippedCount == 2, "Skipped count is exactly 2 (pure duplicate appointment records)");

  if (appointments.size() == 2) {
    check(appointments[0].getId() == "A0001" && appointments[0].getDate() == "2026-11-15",
          "First occurrence A0001 retained with original date");
    check(appointments[1].getId() == "A0002" && appointments[1].getDate() == "2026-11-16",
          "First occurrence A0002 retained with original date");
  }

  // Verify console viewAppointments displays warning
  hms::HospitalSystem sys(fm);
  std::ostringstream out;
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.viewAppointments("ALL");
  std::cout.rdbuf(oldCout);

  check(out.str().find("2 malformed record(s) skipped") != std::string::npos,
        "viewAppointments displays warning for 2 skipped duplicate appointment records");
}

// ---------------------------------------------------------------------------
// Test 4: Orphan Appointments (Isolated from duplicates and malformed records)
// ---------------------------------------------------------------------------
void testOrphanAppointmentsOnly() {
  std::cout << "\n-- Test 4: Orphan Appointments (Isolated) --\n";

  const fs::path dir = fs::path("build") / "test_issue34_orphan_only";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  // Registered patients: P001, P002
  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P002,Bob Jones,40,M,9876543211,456 Oak St,Asthma\n";
  }

  // Registered doctors: D001, D002
  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "D002,Dr. Wilson,Oncology,Cancer,9876543211,Mon-Fri 10:00-18:00\n";
  }

  // Appointments:
  // 1. A0001: P001, D001 (valid)
  // 2. A0002: p002, d002 (valid case-insensitive references)
  // 3. A0003: P999, D001 (orphan: nonexistent patient P999)
  // 4. A0004: P001, D999 (orphan: nonexistent doctor D999)
  // 5. A0005: P888, D888 (orphan: nonexistent patient P888 and doctor D888)
  {
    std::ofstream csv(apptPath);
    csv << "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";
    csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
    csv << "A0002,p002,d002,2026-11-16,11:00,Cancelled\n";
    csv << "A0003,P999,D001,2026-11-17,12:00,Booked\n";
    csv << "A0004,P001,D999,2026-11-18,13:00,Booked\n";
    csv << "A0005,P888,D888,2026-11-19,14:00,Booked\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);
  std::size_t skippedCount = 0;
  const auto appointments = fm.loadAppointments(&skippedCount);

  check(appointments.size() == 2, "Exactly 2 valid appointments loaded (A0001, A0002)");
  check(skippedCount == 3, "Skipped count is exactly 3 (all 3 orphan records)");

  if (appointments.size() == 2) {
    check(appointments[0].getId() == "A0001" && appointments[0].getPatientId() == "P001" &&
          appointments[0].getDoctorId() == "D001", "A0001 correctly loaded");
    check(appointments[1].getId() == "A0002" && appointments[1].getPatientId() == "P002" &&
          appointments[1].getDoctorId() == "D002", "A0002 correctly loaded");
  }

  // Verify console viewAppointments displays warning for orphans
  hms::HospitalSystem sys(fm);
  std::ostringstream out;
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.viewAppointments("ALL");
  std::cout.rdbuf(oldCout);

  const std::string displayed = out.str();
  check(displayed.find("3 malformed record(s) skipped") != std::string::npos,
        "viewAppointments displays warning for 3 skipped orphan appointments");
  check(displayed.find("Appointment ID: A0001") != std::string::npos,
        "viewAppointments displays A0001");
  check(displayed.find("Appointment ID: A0002") != std::string::npos,
        "viewAppointments displays A0002");
  check(displayed.find("A0003") == std::string::npos,
        "Orphan appointment A0003 is omitted from output");
  check(displayed.find("A0004") == std::string::npos,
        "Orphan appointment A0004 is omitted from output");
  check(displayed.find("A0005") == std::string::npos,
        "Orphan appointment A0005 is omitted from output");
}

// ---------------------------------------------------------------------------
// Test 5: Cumulative Skipped Count (Malformed + Duplicate + Orphan)
// ---------------------------------------------------------------------------
void testCumulativeSkippedCount() {
  std::cout << "\n-- Test 5: Cumulative Skipped Count --\n";

  const fs::path dir = fs::path("build") / "test_issue34_cumulative";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  // 1 valid patient (P001), 1 duplicate (P001), 1 malformed patient (P002 NaN)
  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P001,Alice Duplicate,30,F,9876543210,123 Main St,None\n";
    csv << "P002,Bob Invalid,NaN,M,9876543211,456 Oak St,None\n";
  }

  // 1 valid doctor (D001), 1 duplicate (D001), 1 malformed doctor (D002 short)
  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "d001,Dr. House Lowercase,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "D002,Dr. Broken\n";
  }

  // Appointments:
  // 1. A0001: P001, D001 (valid)
  // 2. A0001: P001, D001 (duplicate ID)
  // 3. A0002: P002, D001 (orphan: P002 was malformed and not loaded)
  // 4. A0003: P001, D002 (orphan: D002 was malformed and not loaded)
  // 5. A0004: P999, D999 (orphan: unknown patient and doctor)
  // 6. A0005: P001, D001, 2026-02-30 (malformed date)
  // 7. A0006: Broken,line (malformed columns)
  {
    std::ofstream csv(apptPath);
    csv << "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";
    csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
    csv << "A0001,P001,D001,2026-11-16,11:00,Booked\n";
    csv << "A0002,P002,D001,2026-11-17,12:00,Booked\n";
    csv << "A0003,P001,D002,2026-11-18,13:00,Booked\n";
    csv << "A0004,P999,D999,2026-11-19,14:00,Booked\n";
    csv << "A0005,P001,D001,2026-02-30,10:00,Booked\n";
    csv << "A0006,Broken,line\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);

  std::size_t patSkipped = 0;
  const auto pats = fm.loadPatients(&patSkipped);
  check(pats.size() == 1, "Cumulative: 1 valid patient loaded");
  check(patSkipped == 2, "Cumulative: 2 patients skipped (1 duplicate + 1 malformed)");

  std::size_t docSkipped = 0;
  const auto docs = fm.loadDoctors(&docSkipped);
  check(docs.size() == 1, "Cumulative: 1 valid doctor loaded");
  check(docSkipped == 2, "Cumulative: 2 doctors skipped (1 duplicate + 1 malformed)");

  std::size_t apptSkipped = 0;
  const auto appts = fm.loadAppointments(&apptSkipped);
  check(appts.size() == 1, "Cumulative: 1 valid appointment loaded (A0001)");
  check(apptSkipped == 6, "Cumulative: 6 appointments skipped (1 duplicate + 3 orphans + 2 malformed)");
}

// ---------------------------------------------------------------------------
// Test 6: Clean Valid Records (Zero Skipped)
// ---------------------------------------------------------------------------
void testCleanValidRecords() {
  std::cout << "\n-- Test 6: Clean Valid Records (Zero Skipped) --\n";

  const fs::path dir = fs::path("build") / "test_issue34_clean_records";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P002,Bob Jones,40,M,9876543211,456 Oak St,Asthma\n";
  }

  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "D002,Dr. Wilson,Oncology,Cancer,9876543211,Mon-Fri 10:00-18:00\n";
  }

  {
    std::ofstream csv(apptPath);
    csv << "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";
    csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
    csv << "A0002,P002,D002,2026-11-16,11:00,Booked\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);

  std::size_t patSkipped = 0;
  const auto pats = fm.loadPatients(&patSkipped);
  check(pats.size() == 2 && patSkipped == 0, "All 2 patients valid, 0 skipped");

  std::size_t docSkipped = 0;
  const auto docs = fm.loadDoctors(&docSkipped);
  check(docs.size() == 2 && docSkipped == 0, "All 2 doctors valid, 0 skipped");

  std::size_t apptSkipped = 0;
  const auto appts = fm.loadAppointments(&apptSkipped);
  check(appts.size() == 2 && apptSkipped == 0, "All 2 appointments valid, 0 skipped");

  // Verify console output contains NO skipped warning
  hms::HospitalSystem sys(fm);
  std::ostringstream out;
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.displayStartupRecords();
  std::cout.rdbuf(oldCout);

  check(out.str().find("skipped") == std::string::npos,
        "Clean dataset displayStartupRecords shows no skipped-record warnings");
}

// ---------------------------------------------------------------------------
// Test 7: Full Interactive Menu Flow Verification & Clean Exit
// ---------------------------------------------------------------------------
void testInteractiveMenuFlowAndCleanExit() {
  std::cout << "\n-- Test 7: Deterministic Interactive Menu Navigation --\n";

  const fs::path dir = fs::path("build") / "test_issue34_interactive";
  cleanDir(dir);
  const std::string patientPath = (dir / "patients.csv").string();
  const std::string doctorPath = (dir / "doctors.csv").string();
  const std::string apptPath = (dir / "appointments.csv").string();

  // Seed patients with 1 duplicate
  {
    std::ofstream csv(patientPath);
    csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    csv << "P001,Alice Smith,30,F,9876543210,123 Main St,None\n";
    csv << "P001,Alice Duplicate,30,F,9876543210,123 Main St,None\n";
  }

  // Seed doctors with 1 duplicate
  {
    std::ofstream csv(doctorPath);
    csv << "doctor_id,name,specialization,department,contact_number,availability\n";
    csv << "D001,Dr. House,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
    csv << "d001,Dr. House Lowercase,Diagnostics,Medicine,9876543210,Mon-Fri 09:00-17:00\n";
  }

  // Seed appointments with 1 orphan
  {
    std::ofstream csv(apptPath);
    csv << "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";
    csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
    csv << "A0002,P999,D001,2026-11-16,11:00,Booked\n";
  }

  hms::FileManager fm(patientPath, doctorPath, apptPath);
  hms::HospitalSystem sys(fm);

  // Scripted menu input:
  // 1 -> Patient Management
  //   3 -> View All (triggers listPatients, outputs 1 skipped warning)
  //   0 -> Return to main menu
  // 2 -> Doctor Management
  //   3 -> View All (triggers listDoctors, outputs 1 skipped warning)
  //   0 -> Return to main menu
  // 3 -> Appointment Management
  //   2 -> View Appointments
  //     1 -> All Appointments (triggers viewAppointments, outputs 1 skipped warning)
  //     0 -> Return to Appointment submenu
  //   0 -> Return to main menu
  // 4 -> Exit Hospital Management System
  std::istringstream in(
      "1\n3\n0\n"
      "2\n3\n0\n"
      "3\n2\n1\n0\n0\n"
      "4\n"
  );
  std::ostringstream out;

  auto *oldCin = std::cin.rdbuf(in.rdbuf());
  auto *oldCout = std::cout.rdbuf(out.rdbuf());
  sys.run();
  std::cin.rdbuf(oldCin);
  std::cout.rdbuf(oldCout);

  const std::string sessionOutput = out.str();

  check(sessionOutput.find("--- Patient List ---") != std::string::npos,
        "Interactive menu reached Patient List");
  check(sessionOutput.find("--- Doctor List ---") != std::string::npos,
        "Interactive menu reached Doctor List");
  check(sessionOutput.find("--- View Appointments ---") != std::string::npos,
        "Interactive menu reached View Appointments");
  check(sessionOutput.find("1 malformed record(s) skipped") != std::string::npos,
        "Interactive menu reported skipped record warnings");
  check(sessionOutput.find("Exiting Hospital Management System.") != std::string::npos,
        "Interactive menu terminated cleanly with exit message");
}

} // namespace

int main() {
  std::cout << "=== Running Issue #34 Skip Duplicate and Orphan Records Tests ===\n";

  try {
    testDuplicatePatientIdsOnly();
    testDuplicateDoctorIdsOnly();
    testDuplicateAppointmentIdsOnly();
    testOrphanAppointmentsOnly();
    testCumulativeSkippedCount();
    testCleanValidRecords();
    testInteractiveMenuFlowAndCleanExit();
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] Exception thrown: " << e.what() << '\n';
    return 1;
  }

  std::cout << "\n=======================================================\n";
  std::cout << "Issue #34 Tests Result: " << (failures == 0 ? "ALL PASSED" : "FAILED")
            << " (" << failures << " failures)\n";
  std::cout << "=======================================================\n";

  return failures == 0 ? 0 : 1;
}
