// TC-APT-08: View Appointments in Date/Time Order (HMS-F-022)
//
// Traceability:
//   - SRS Requirement: HMS-F-022
//   - STP RTM Test Case: TC-APT-08
//   - SRS RTM Test Case: TC-APT-08
//   - Dependency: HMS-F-015 (Book Appointment)
//
// Acceptance Requirements:
//   1. All / by-patient / by-doctor views list expected records in date-then-time order.
//   2. Cancelled appointment records are included in all views.
//   3. When the selected view has no matching appointments, display exactly "No appointments found.".
//   4. Tie-breaking for equal date/time uses Appointment ID in ascending order.
//   5. Interactive UI allows selection of All, By Patient, and By Doctor.
//   6. Empty required IDs and invalid filter choices are handled safely.
//   7. Viewing appointments is strictly non-modifying (CSV remains byte-for-byte identical).
//
// Test Environment:
//   - Uses isolated test CSV files in build/, never root-level user data files.

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

std::string captureViewAppointments(hms::HospitalSystem &system,
                                   const std::string &filterType = "ALL",
                                   const std::string &filterId = "") {
  std::ostringstream outStream;
  auto *origCout = std::cout.rdbuf(outStream.rdbuf());

  system.viewAppointments(filterType, filterId);

  std::cout.rdbuf(origCout);
  return outStream.str();
}

} // namespace

int main() {
  std::cout << "=== Running TC-APT-08 View Appointments in Date/Time Order Tests ===\n";
  std::cout << "=== (Traceability: HMS-F-022, STP: TC-APT-08, SRS: TC-APT-08, Issue #26) ===\n\n";

  const std::string testPatientCsv = "build/test_view_appts_patients.csv";
  const std::string testDoctorCsv = "build/test_view_appts_doctors.csv";
  const std::string testApptCsv = "build/test_view_appts_appointments.csv";

  // Clean up any stale files from previous runs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // -------------------------------------------------------------------------
  // Setup: Seed patients and doctors
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding registered patients and doctors --\n";

  std::string seedErr;
  bool p1 = fm.savePatient(
      hms::Patient("P001", "Alice Smith", 30, "F", "9876543210", "10 Main St", "None"),
      seedErr);
  check("Seed patient P001", p1);

  bool p2 = fm.savePatient(
      hms::Patient("P002", "Bob Jones", 45, "M", "9123456789", "20 Oak St", "Hypertension"),
      seedErr);
  check("Seed patient P002", p2);

  bool p3 = fm.savePatient(
      hms::Patient("P003", "Carol White", 28, "F", "9811223344", "30 Pine St", "Asthma"),
      seedErr);
  check("Seed patient P003", p3);

  bool d1 = fm.saveDoctor(
      hms::Doctor("D001", "Dr. John Doe", "Cardiology", "Cardiology Dept",
                  "9876501234", "Mon-Fri 09:00-17:00"),
      seedErr);
  check("Seed doctor D001", d1);

  bool d2 = fm.saveDoctor(
      hms::Doctor("D002", "Dr. Jane Roe", "Neurology", "Neurology Dept",
                  "9876505678", "Mon-Fri 08:00-16:00"),
      seedErr);
  check("Seed doctor D002", d2);

  // -------------------------------------------------------------------------
  // Seed deliberately unsorted appointments into appointments.csv:
  // Expected sorted chronological order:
  //   1. A0005: 2026-10-15 16:00 (P002, D002, Cancelled)
  //   2. A0002: 2026-10-20 09:30 (P002, D001, Booked)    [Tie on date/time: A0002 < A0004]
  //   3. A0004: 2026-10-20 09:30 (P003, D002, Cancelled) [Tie on date/time: A0002 < A0004]
  //   4. A0003: 2026-10-20 11:00 (P001, D002, Booked)
  //   5. A0001: 2026-11-15 14:00 (P001, D001, Booked)
  // -------------------------------------------------------------------------
  std::vector<hms::Appointment> seedAppts = {
      hms::Appointment("A0001", "P001", "D001", "2026-11-15", "14:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0002", "P002", "D001", "2026-10-20", "09:30", hms::AppointmentStatus::Booked),
      hms::Appointment("A0003", "P001", "D002", "2026-10-20", "11:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0004", "P003", "D002", "2026-10-20", "09:30", hms::AppointmentStatus::Cancelled),
      hms::Appointment("A0005", "P002", "D002", "2026-10-15", "16:00", hms::AppointmentStatus::Cancelled)
  };
  bool apptsSaved = fm.saveAppointments(seedAppts, seedErr);
  check("Seed 5 deliberately unsorted appointments (Booked & Cancelled)", apptsSaved);

  const std::string baselineCsvContent = readFile(testApptCsv);
  const int baselineRowCount = countDataRows(testApptCsv);
  check("CSV has 5 rows initially", baselineRowCount == 5);

  hms::HospitalSystem system(fm);

  // =========================================================================
  // Test A — All Appointments View & Chronological Sorting
  // =========================================================================
  std::cout << "\n-- Test A: All Appointments View & Chronological Sorting --\n";
  {
    const auto all = system.listAppointments("ALL");
    check("All appointments returns exactly 5 records", all.size() == 5);

    if (all.size() == 5) {
      check("1st sorted record is A0005 (2026-10-15 16:00)",
            all[0].getId() == "A0005" && all[0].getDate() == "2026-10-15" && all[0].getTime() == "16:00");
      check("1st sorted record is Cancelled", all[0].isCancelled());

      check("2nd sorted record is A0002 (2026-10-20 09:30)",
            all[1].getId() == "A0002" && all[1].getDate() == "2026-10-20" && all[1].getTime() == "09:30");
      check("2nd sorted record is Booked", all[1].isBooked());

      check("3rd sorted record is A0004 (2026-10-20 09:30 tie-breaker ID)",
            all[2].getId() == "A0004" && all[2].getDate() == "2026-10-20" && all[2].getTime() == "09:30");
      check("3rd sorted record is Cancelled", all[2].isCancelled());
      check("Equal date/time tie-breaker: A0002 precedes A0004",
            all[1].getId() < all[2].getId());

      check("4th sorted record is A0003 (2026-10-20 11:00)",
            all[3].getId() == "A0003" && all[3].getDate() == "2026-10-20" && all[3].getTime() == "11:00");
      check("4th sorted record is Booked", all[3].isBooked());

      check("5th sorted record is A0001 (2026-11-15 14:00)",
            all[4].getId() == "A0001" && all[4].getDate() == "2026-11-15" && all[4].getTime() == "14:00");
      check("5th sorted record is Booked", all[4].isBooked());
    }

    // Verify console output of viewAppointments("ALL")
    const std::string out = captureViewAppointments(system, "ALL");
    check("viewAppointments('ALL') contains header", out.find("--- View Appointments ---") != std::string::npos);
    check("viewAppointments('ALL') displays A0005 Cancelled",
          out.find("Appointment ID: A0005 | Patient ID: P002 | Patient Name: Bob Jones | Doctor ID: D002 | Doctor Name: Dr. Jane Roe | Date: 2026-10-15 | Time: 16:00 | Status: Cancelled") != std::string::npos);
    check("viewAppointments('ALL') displays A0002 Booked",
          out.find("Appointment ID: A0002 | Patient ID: P002 | Patient Name: Bob Jones | Doctor ID: D001 | Doctor Name: Dr. John Doe | Date: 2026-10-20 | Time: 09:30 | Status: Booked") != std::string::npos);
    check("viewAppointments('ALL') displays A0004 Cancelled",
          out.find("Appointment ID: A0004 | Patient ID: P003 | Patient Name: Carol White | Doctor ID: D002 | Doctor Name: Dr. Jane Roe | Date: 2026-10-20 | Time: 09:30 | Status: Cancelled") != std::string::npos);
    check("viewAppointments('ALL') displays A0003 Booked",
          out.find("Appointment ID: A0003 | Patient ID: P001 | Patient Name: Alice Smith | Doctor ID: D002 | Doctor Name: Dr. Jane Roe | Date: 2026-10-20 | Time: 11:00 | Status: Booked") != std::string::npos);
    check("viewAppointments('ALL') displays A0001 Booked",
          out.find("Appointment ID: A0001 | Patient ID: P001 | Patient Name: Alice Smith | Doctor ID: D001 | Doctor Name: Dr. John Doe | Date: 2026-11-15 | Time: 14:00 | Status: Booked") != std::string::npos);

    // Verify relative display order in console output
    const std::size_t posA0005 = out.find("A0005");
    const std::size_t posA0002 = out.find("A0002");
    const std::size_t posA0004 = out.find("A0004");
    const std::size_t posA0003 = out.find("A0003");
    const std::size_t posA0001 = out.find("A0001");
    check("Console order: A0005 appears before A0002", posA0005 < posA0002);
    check("Console order: A0002 appears before A0004", posA0002 < posA0004);
    check("Console order: A0004 appears before A0003", posA0004 < posA0003);
    check("Console order: A0003 appears before A0001", posA0003 < posA0001);
  }

  // =========================================================================
  // Test B — Patient Filter
  // =========================================================================
  std::cout << "\n-- Test B: Filter by Patient --\n";
  {
    // Patient P001 has A0003 (2026-10-20 11:00) and A0001 (2026-11-15 14:00)
    const auto p1Appts = system.listAppointments("PATIENT", "P001");
    check("P001 has exactly 2 appointments", p1Appts.size() == 2);
    if (p1Appts.size() == 2) {
      check("P001 1st record is A0003 (2026-10-20 11:00)", p1Appts[0].getId() == "A0003");
      check("P001 2nd record is A0001 (2026-11-15 14:00)", p1Appts[1].getId() == "A0001");
    }

    // Patient P002 has A0005 (2026-10-15 16:00 Cancelled) and A0002 (2026-10-20 09:30 Booked)
    const auto p2Appts = system.listAppointments("PATIENT", "P002");
    check("P002 has exactly 2 appointments", p2Appts.size() == 2);
    if (p2Appts.size() == 2) {
      check("P002 1st record is A0005 Cancelled",
            p2Appts[0].getId() == "A0005" && p2Appts[0].isCancelled());
      check("P002 2nd record is A0002 Booked",
            p2Appts[1].getId() == "A0002" && p2Appts[1].isBooked());
    }

    // Patient P003 has A0004 (2026-10-20 09:30 Cancelled)
    const auto p3Appts = system.listAppointments("PATIENT", "P003");
    check("P003 has exactly 1 appointment", p3Appts.size() == 1);
    if (!p3Appts.empty()) {
      check("P003 record is A0004 Cancelled",
            p3Appts[0].getId() == "A0004" && p3Appts[0].isCancelled());
    }

    // Case-insensitivity and whitespace trimming: "  p001  "
    const auto p1Lower = system.listAppointments("PATIENT", "  p001  ");
    check("Patient filter is case-insensitive and trims whitespace", p1Lower.size() == 2);

    // Verify console output for Patient P002 includes Cancelled and Booked
    const std::string outP2 = captureViewAppointments(system, "PATIENT", "P002");
    check("P002 console output shows A0005 Cancelled", outP2.find("A0005") != std::string::npos && outP2.find("Cancelled") != std::string::npos);
    check("P002 console output shows A0002 Booked", outP2.find("A0002") != std::string::npos && outP2.find("Booked") != std::string::npos);
    check("P002 console output does NOT show P001's A0001", outP2.find("A0001") == std::string::npos);
    check("P002 console output does NOT show P003's A0004", outP2.find("A0004") == std::string::npos);
  }

  // =========================================================================
  // Test C — Doctor Filter
  // =========================================================================
  std::cout << "\n-- Test C: Filter by Doctor --\n";
  {
    // Doctor D001 has A0002 (2026-10-20 09:30 Booked) and A0001 (2026-11-15 14:00 Booked)
    const auto d1Appts = system.listAppointments("DOCTOR", "D001");
    check("D001 has exactly 2 appointments", d1Appts.size() == 2);
    if (d1Appts.size() == 2) {
      check("D001 1st record is A0002 (2026-10-20 09:30)", d1Appts[0].getId() == "A0002");
      check("D001 2nd record is A0001 (2026-11-15 14:00)", d1Appts[1].getId() == "A0001");
    }

    // Doctor D002 has A0005 (Cancelled), A0004 (Cancelled), A0003 (Booked)
    const auto d2Appts = system.listAppointments("DOCTOR", "D002");
    check("D002 has exactly 3 appointments", d2Appts.size() == 3);
    if (d2Appts.size() == 3) {
      check("D002 1st record is A0005 (2026-10-15 16:00 Cancelled)",
            d2Appts[0].getId() == "A0005" && d2Appts[0].isCancelled());
      check("D002 2nd record is A0004 (2026-10-20 09:30 Cancelled)",
            d2Appts[1].getId() == "A0004" && d2Appts[1].isCancelled());
      check("D002 3rd record is A0003 (2026-10-20 11:00 Booked)",
            d2Appts[2].getId() == "A0003" && d2Appts[2].isBooked());
    }

    // Case-insensitivity: "d001"
    const auto d1Lower = system.listAppointments("DOCTOR", "d001");
    check("Doctor filter is case-insensitive", d1Lower.size() == 2);

    // Verify console output for Doctor D001
    const std::string outD1 = captureViewAppointments(system, "DOCTOR", "D001");
    check("D001 console output shows A0002", outD1.find("A0002") != std::string::npos);
    check("D001 console output shows A0001", outD1.find("A0001") != std::string::npos);
    check("D001 console output does NOT show D002's A0003", outD1.find("A0003") == std::string::npos);
    check("D001 console output does NOT show D002's A0005", outD1.find("A0005") == std::string::npos);
  }

  // =========================================================================
  // Test D — Empty Data and Empty Filtered Results
  // =========================================================================
  std::cout << "\n-- Test D: Empty Data and Empty Filtered Results --\n";
  {
    // 1. Patient with no appointments (P999)
    const std::string outNonExistentPat = captureViewAppointments(system, "PATIENT", "P999");
    check("Patient with no appointments displays 'No appointments found.'",
          outNonExistentPat.find("No appointments found.") != std::string::npos);

    // 2. Doctor with no appointments (D999)
    const std::string outNonExistentDoc = captureViewAppointments(system, "DOCTOR", "D999");
    check("Doctor with no appointments displays 'No appointments found.'",
          outNonExistentDoc.find("No appointments found.") != std::string::npos);

    // 3. Empty filter IDs ("")
    const std::string outEmptyPat = captureViewAppointments(system, "PATIENT", "");
    check("Empty patient ID displays 'No appointments found.'",
          outEmptyPat.find("No appointments found.") != std::string::npos);

    const std::string outEmptyDoc = captureViewAppointments(system, "DOCTOR", "");
    check("Empty doctor ID displays 'No appointments found.'",
          outEmptyDoc.find("No appointments found.") != std::string::npos);

    // 4. Completely empty appointments dataset
    const std::string emptyApptCsv = "build/test_view_appts_empty.csv";
    removeFile(emptyApptCsv);
    hms::FileManager emptyFm(testPatientCsv, testDoctorCsv, emptyApptCsv);
    hms::HospitalSystem emptySys(emptyFm);

    const std::string outAllEmpty = captureViewAppointments(emptySys, "ALL");
    check("Completely empty appointments dataset displays 'No appointments found.'",
          outAllEmpty.find("No appointments found.") != std::string::npos);

    removeFile(emptyApptCsv);
  }

  // =========================================================================
  // Test E — Interactive UI Workflow
  // =========================================================================
  std::cout << "\n-- Test E: Interactive UI Workflow --\n";
  {
    // Sequence:
    // Main menu: 3 (Appointment Management)
    // Appointment menu: 2 (View)
    // View menu: 1 (All Appointments)
    // Appointment menu: 2 (View)
    // View menu: 2 (Filter by Patient) -> "P001"
    // Appointment menu: 2 (View)
    // View menu: 3 (Filter by Doctor) -> "D001"
    // Appointment menu: 2 (View)
    // View menu: 9 (Invalid choice)
    // Appointment menu: 2 (View)
    // View menu: 2 (Filter by Patient) -> "" (Empty ID)
    // Appointment menu: 2 (View)
    // View menu: 0 (Return)
    // Appointment menu: 0 (Return)
    // Main menu: 4 (Exit)
    const std::string interactiveInput =
        "3\n"           // Main menu -> Appointment
        "2\n1\n"        // View -> All
        "2\n2\nP001\n"  // View -> By Patient P001
        "2\n3\nD001\n"  // View -> By Doctor D001
        "2\n9\n"        // View -> Invalid choice 9
        "2\n2\n\n"      // View -> By Patient -> empty ID
        "2\n0\n"        // View -> Return
        "0\n"           // Appointment -> Return
        "4\n";          // Main menu -> Exit

    const std::string sessionOut = runInteractive(system, interactiveInput);

    check("Interactive: '=== View Appointments ===' header displayed",
          sessionOut.find("=== View Appointments ===") != std::string::npos);
    check("Interactive: '1. All Appointments' option displayed",
          sessionOut.find("1. All Appointments") != std::string::npos);
    check("Interactive: '2. Filter by Patient' option displayed",
          sessionOut.find("2. Filter by Patient") != std::string::npos);
    check("Interactive: '3. Filter by Doctor' option displayed",
          sessionOut.find("3. Filter by Doctor") != std::string::npos);
    check("Interactive: '0. Return' option displayed",
          sessionOut.find("0. Return") != std::string::npos);

    // Check All view results in interactive session
    check("Interactive All: includes A0005 Cancelled", sessionOut.find("A0005") != std::string::npos);
    check("Interactive All: includes A0001 Booked", sessionOut.find("A0001") != std::string::npos);

    // Check Patient filter prompt and results
    check("Interactive: prompted for patient ID", sessionOut.find("Enter patient ID:") != std::string::npos);

    // Check Doctor filter prompt and results
    check("Interactive: prompted for doctor ID", sessionOut.find("Enter doctor ID:") != std::string::npos);

    // Check invalid choice handling
    check("Interactive: invalid view choice displays 'Invalid choice'",
          sessionOut.find("Invalid choice. Enter 0 to 3.") != std::string::npos ||
          sessionOut.find("Invalid choice.") != std::string::npos);

    // Check empty ID handling
    check("Interactive: empty ID displays 'This field is required.'",
          sessionOut.find("This field is required.") != std::string::npos);

    // Check clean exit
    check("Interactive: clean exit message displayed",
          sessionOut.find("Exiting Hospital Management System.") != std::string::npos);
  }

  // =========================================================================
  // Test F — Persistence Integrity
  // =========================================================================
  std::cout << "\n-- Test F: Persistence Integrity (Read-Only Guarantee) --\n";
  {
    const std::string finalCsvContent = readFile(testApptCsv);
    const int finalRowCount = countDataRows(testApptCsv);

    check("CSV row count completely unchanged after all operations", finalRowCount == baselineRowCount);
    check("CSV content is byte-for-byte identical to baseline (read-only)",
          finalCsvContent == baselineCsvContent);

    // Verify all original statuses are preserved on disk
    const auto loaded = fm.loadAppointments();
    check("FileManager loads exactly 5 persisted records", loaded.size() == 5);
    int bookedCount = 0;
    int cancelledCount = 0;
    for (const auto &a : loaded) {
      if (a.isBooked()) ++bookedCount;
      if (a.isCancelled()) ++cancelledCount;
    }
    check("Persisted records still have exactly 3 Booked", bookedCount == 3);
    check("Persisted records still have exactly 2 Cancelled", cancelledCount == 2);
  }

  // -------------------------------------------------------------------------
  // Cleanup isolated test CSV files
  // -------------------------------------------------------------------------
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=======================================================\n";
  std::cout << "TC-APT-08 View Appointments in Date/Time Order: "
            << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
