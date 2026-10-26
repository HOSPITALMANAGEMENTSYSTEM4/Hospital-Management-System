// TC-APT-10: Reject Invalid Cancellation
// Traceability:
//   - SRS Requirement: HMS-F-024
//   - STP RTM Test Case: TC-APT-10
//   - SRS RTM Test Case: TC-APT-10
//   - Dependency: HMS-F-023 (Cancel Appointment)
//   - GitHub Issue: #33
//
// Acceptance Requirements:
//   1. Nonexistent appointment ID displays exact message "Appointment not found".
//   2. Already-cancelled appointment ID displays exact message "Appointment already cancelled".
//   3. Neither invalid attempt changes any appointment record or the appointments CSV data file.
//   4. Interactive console workflow handles both error cases cleanly and returns safely to menu.
//   5. Data integrity: multi-record stability, byte-for-byte file identity, persistence across restart.
//   6. Regression compatibility: valid cancellation, slot rebooking, and double-booking checks remain intact.

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <algorithm>
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
    if (!line.empty() && line.rfind("AppointmentID", 0) != 0 &&
        line.rfind("appointment_id", 0) != 0) {
      ++count;
    }
  }
  return count;
}

void removeFile(const std::string &path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
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
  std::cout << "=== Running TC-APT-10 Reject Invalid Cancellation Tests ===\n";
  std::cout << "=== (Traceability: HMS-F-024, STP: TC-APT-10, SRS: TC-APT-10, Issue #33) ===\n\n";

  // Capture baseline of root data files to guarantee they remain untouched
  const std::string rootPatientCsv = "patients.csv";
  const std::string rootDoctorCsv = "doctors.csv";
  const std::string rootApptCsv = "appointments.csv";

  const std::string rootPatientBaseline = readFile(rootPatientCsv);
  const std::string rootDoctorBaseline = readFile(rootDoctorCsv);
  const std::string rootApptBaseline = readFile(rootApptCsv);

  // Isolated test directory
  const std::string testDir = "build/test_reject_invalid_cancel_10";
  std::error_code dirEc;
  std::filesystem::create_directories(testDir, dirEc);

  const std::string testPatientCsv = testDir + "/patients.csv";
  const std::string testDoctorCsv = testDir + "/doctors.csv";
  const std::string testApptCsv = testDir + "/appointments.csv";

  // Clean stale files from prior test runs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // Setup: Seed valid patients and doctors
  std::string err;
  hms::Patient p1("P001", "Alice Smith", 30, "F", "1234567890", "123 Maple St", "None");
  hms::Patient p2("P002", "Charlie Brown", 45, "M", "9876543210", "456 Oak Ave", "Asthma");
  hms::Doctor d1("D001", "Dr. Bob Jones", "Cardiology", "Cardiology", "0987654321", "Mon-Fri");
  hms::Doctor d2("D002", "Dr. Diana Prince", "Neurology", "Neurology", "1122334455", "Tue-Sat");

  check("Seed patient P001", fm.savePatient(p1, err));
  check("Seed patient P002", fm.savePatient(p2, err));
  check("Seed doctor D001", fm.saveDoctor(d1, err));
  check("Seed doctor D002", fm.saveDoctor(d2, err));

  // =========================================================================
  // Test A — Nonexistent ID
  // =========================================================================
  std::cout << "\n-- Test A: Nonexistent appointment ID --\n";
  {
    // Seed initial valid booked appointment A0001
    hms::Appointment appt1("A0001", "P001", "D001", "2026-11-10", "10:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0001 (Booked)", fm.saveAppointment(appt1, err));

    const int initialCount = countDataRows(testApptCsv);
    const std::string baselineBytes = readFile(testApptCsv);

    hms::HospitalSystem system(fm);

    // Attempt cancellation using nonexistent ID "A9999" with confirmed = true
    const hms::OpResult res = system.cancelAppointment("A9999", true);
    check("Cancellation of nonexistent ID fails (success = false)", !res.success);
    check("Error message is exactly 'Appointment not found'",
          res.message == "Appointment not found");

    // Verify appointment count unchanged
    check("Appointment record count unchanged after nonexistent cancellation attempt",
          countDataRows(testApptCsv) == initialCount);

    // Verify appointments CSV remains byte-for-byte identical
    const std::string afterAttemptBytes = readFile(testApptCsv);
    check("Appointments CSV remains byte-for-byte identical after nonexistent ID attempt",
          afterAttemptBytes == baselineBytes);

    // Verify every existing appointment remains unchanged
    const hms::Appointment *a1 = system.findAppointment("A0001");
    check("Existing appointment A0001 still exists", a1 != nullptr);
    if (a1) {
      check("A0001 remains Booked", a1->isBooked());
      check("A0001 is not Cancelled", !a1->isCancelled());
      check("A0001 fields unchanged",
            a1->getPatientId() == "P001" &&
            a1->getDoctorId() == "D001" &&
            a1->getDate() == "2026-11-10" &&
            a1->getTime() == "10:00");
    }

    // Verify another nonexistent ID format e.g. "NONEXIST"
    const hms::OpResult res2 = system.cancelAppointment("NONEXIST", true);
    check("Cancellation of arbitrary nonexistent ID fails", !res2.success);
    check("Arbitrary nonexistent ID returns exactly 'Appointment not found'",
          res2.message == "Appointment not found");
    check("Appointments CSV still byte-for-byte identical",
          readFile(testApptCsv) == baselineBytes);
  }

  // =========================================================================
  // Test B — Already-cancelled ID
  // =========================================================================
  std::cout << "\n-- Test B: Already-cancelled appointment ID --\n";
  {
    hms::HospitalSystem system(fm);

    // Cancel A0001 successfully once
    const hms::OpResult firstCancel = system.cancelAppointment("A0001", true);
    check("First cancellation of A0001 succeeds", firstCancel.success);
    check("First cancellation message is 'Appointment cancelled successfully'",
          firstCancel.message == "Appointment cancelled successfully");

    // Capture post-cancellation baseline
    const int countAfterFirstCancel = countDataRows(testApptCsv);
    const std::string baselineAfterFirstCancel = readFile(testApptCsv);

    // Attempt to cancel A0001 a second time
    const hms::OpResult secondCancel = system.cancelAppointment("A0001", true);
    check("Second cancellation attempt fails (success = false)", !secondCancel.success);
    check("Error message is exactly 'Appointment already cancelled'",
          secondCancel.message == "Appointment already cancelled");

    // Verify appointment count unchanged
    check("Record count unchanged after duplicate cancellation attempt",
          countDataRows(testApptCsv) == countAfterFirstCancel);

    // Verify CSV contents remain byte-for-byte identical
    check("CSV file remains byte-for-byte identical to post-cancellation baseline",
          readFile(testApptCsv) == baselineAfterFirstCancel);

    // Verify original record in storage has status Cancelled and fields intact
    const hms::Appointment *a1 = system.findAppointment("A0001");
    check("A0001 found in storage", a1 != nullptr);
    if (a1) {
      check("A0001 status remains Cancelled", a1->isCancelled());
      check("A0001 status string is 'Cancelled'", a1->getStatusString() == "Cancelled");
      check("A0001 original fields intact",
            a1->getPatientId() == "P001" &&
            a1->getDoctorId() == "D001" &&
            a1->getDate() == "2026-11-10" &&
            a1->getTime() == "10:00");
    }

    // Third cancellation attempt with lowercase ID "a0001"
    const hms::OpResult thirdCancel = system.cancelAppointment("a0001", true);
    check("Case-insensitive duplicate cancellation attempt fails", !thirdCancel.success);
    check("Case-insensitive duplicate cancellation returns 'Appointment already cancelled'",
          thirdCancel.message == "Appointment already cancelled");
    check("CSV remains byte-for-byte identical after third attempt",
          readFile(testApptCsv) == baselineAfterFirstCancel);
  }

  // =========================================================================
  // Test C — Multiple-record integrity
  // =========================================================================
  std::cout << "\n-- Test C: Multiple-record integrity --\n";
  {
    // Independent directory for clean multi-record dataset
    const std::string multiDir = "build/test_reject_invalid_cancel_multi";
    std::filesystem::create_directories(multiDir, dirEc);

    const std::string multiPatCsv = multiDir + "/patients.csv";
    const std::string multiDocCsv = multiDir + "/doctors.csv";
    const std::string multiApptCsv = multiDir + "/appointments.csv";

    removeFile(multiPatCsv);
    removeFile(multiDocCsv);
    removeFile(multiApptCsv);

    hms::FileManager multiFm(multiPatCsv, multiDocCsv, multiApptCsv);
    multiFm.savePatient(p1, err);
    multiFm.savePatient(p2, err);
    multiFm.saveDoctor(d1, err);
    multiFm.saveDoctor(d2, err);

    // 5 appointments: mixture of Booked and Cancelled across various doctors/patients
    std::vector<hms::Appointment> initialList = {
      hms::Appointment("A0001", "P001", "D001", "2026-12-01", "09:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0002", "P002", "D001", "2026-12-01", "10:00", hms::AppointmentStatus::Cancelled),
      hms::Appointment("A0003", "P001", "D002", "2026-12-01", "11:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0004", "P002", "D002", "2026-12-02", "14:00", hms::AppointmentStatus::Cancelled),
      hms::Appointment("A0005", "P001", "D001", "2026-12-02", "15:00", hms::AppointmentStatus::Booked)
    };
    check("Seed 5 mixed appointments (3 Booked, 2 Cancelled)", multiFm.saveAppointments(initialList, err));

    const int baseRowCount = countDataRows(multiApptCsv);
    const std::string baseMultiBytes = readFile(multiApptCsv);
    check("Multi dataset row count is 5", baseRowCount == 5);

    hms::HospitalSystem multiSys(multiFm);

    // 1. Invalid attempt on nonexistent ID "A9999"
    const hms::OpResult invNonExist = multiSys.cancelAppointment("A9999", true);
    check("Multi: Nonexistent ID rejected", !invNonExist.success);
    check("Multi: Message is 'Appointment not found'",
          invNonExist.message == "Appointment not found");
    check("Multi: Row count unchanged after nonexistent attempt",
          countDataRows(multiApptCsv) == baseRowCount);
    check("Multi: File byte-for-byte identical after nonexistent attempt",
          readFile(multiApptCsv) == baseMultiBytes);

    // 2. Invalid attempt on already-cancelled ID "A0002"
    const hms::OpResult invCancelled2 = multiSys.cancelAppointment("A0002", true);
    check("Multi: Already-cancelled A0002 rejected", !invCancelled2.success);
    check("Multi: Message is 'Appointment already cancelled'",
          invCancelled2.message == "Appointment already cancelled");
    check("Multi: Row count unchanged after A0002 attempt",
          countDataRows(multiApptCsv) == baseRowCount);
    check("Multi: File byte-for-byte identical after A0002 attempt",
          readFile(multiApptCsv) == baseMultiBytes);

    // 3. Invalid attempt on already-cancelled ID "A0004"
    const hms::OpResult invCancelled4 = multiSys.cancelAppointment("A0004", true);
    check("Multi: Already-cancelled A0004 rejected", !invCancelled4.success);
    check("Multi: Message is 'Appointment already cancelled'",
          invCancelled4.message == "Appointment already cancelled");
    check("Multi: Row count unchanged after A0004 attempt",
          countDataRows(multiApptCsv) == baseRowCount);
    check("Multi: File byte-for-byte identical after A0004 attempt",
          readFile(multiApptCsv) == baseMultiBytes);

    // 4. Verify all 5 records and fields are completely intact
    const auto allAppts = multiSys.listAppointments("ALL");
    check("Multi: Exactly 5 appointments listed", allAppts.size() == 5);

    int bookedFound = 0;
    int cancelledFound = 0;
    for (const auto &a : allAppts) {
      if (a.isBooked()) ++bookedFound;
      if (a.isCancelled()) ++cancelledFound;
    }
    check("Multi: Exactly 3 Booked appointments remain", bookedFound == 3);
    check("Multi: Exactly 2 Cancelled appointments remain", cancelledFound == 2);

    // Detailed verification of individual records
    const hms::Appointment *m1 = multiSys.findAppointment("A0001");
    check("Multi: A0001 is Booked", m1 != nullptr && m1->isBooked() && m1->getPatientId() == "P001");
    const hms::Appointment *m2 = multiSys.findAppointment("A0002");
    check("Multi: A0002 is Cancelled", m2 != nullptr && m2->isCancelled() && m2->getPatientId() == "P002");
    const hms::Appointment *m3 = multiSys.findAppointment("A0003");
    check("Multi: A0003 is Booked", m3 != nullptr && m3->isBooked() && m3->getPatientId() == "P001");
    const hms::Appointment *m4 = multiSys.findAppointment("A0004");
    check("Multi: A0004 is Cancelled", m4 != nullptr && m4->isCancelled() && m4->getPatientId() == "P002");
    const hms::Appointment *m5 = multiSys.findAppointment("A0005");
    check("Multi: A0005 is Booked", m5 != nullptr && m5->isBooked() && m5->getPatientId() == "P001");
  }

  // =========================================================================
  // Test D — Persistent-state reload
  // =========================================================================
  std::cout << "\n-- Test D: Persistent-state reload --\n";
  {
    // Restart / fresh system loading from the multi dataset
    const std::string multiDir = "build/test_reject_invalid_cancel_multi";
    const std::string multiPatCsv = multiDir + "/patients.csv";
    const std::string multiDocCsv = multiDir + "/doctors.csv";
    const std::string multiApptCsv = multiDir + "/appointments.csv";

    const std::string baselineBeforeReload = readFile(multiApptCsv);

    hms::FileManager freshFm(multiPatCsv, multiDocCsv, multiApptCsv);
    hms::HospitalSystem freshSys(freshFm);

    // Attempt to cancel already-cancelled A0002 after restart
    const hms::OpResult reloadCancelledRes = freshSys.cancelAppointment("A0002", true);
    check("Reload: Already-cancelled appointment rejected after restart", !reloadCancelledRes.success);
    check("Reload: Exact message 'Appointment already cancelled'",
          reloadCancelledRes.message == "Appointment already cancelled");
    check("Reload: Persisted CSV data unchanged after reload cancel attempt",
          readFile(multiApptCsv) == baselineBeforeReload);

    // Attempt to cancel nonexistent ID after restart
    const hms::OpResult reloadNonExistRes = freshSys.cancelAppointment("A9999", true);
    check("Reload: Nonexistent appointment rejected after restart", !reloadNonExistRes.success);
    check("Reload: Exact message 'Appointment not found'",
          reloadNonExistRes.message == "Appointment not found");
    check("Reload: Persisted CSV data still byte-for-byte identical",
          readFile(multiApptCsv) == baselineBeforeReload);
  }

  // =========================================================================
  // Test E — Interactive console workflow
  // =========================================================================
  std::cout << "\n-- Test E: Interactive console workflow --\n";
  {
    // Interactive test on multi dataset
    const std::string multiDir = "build/test_reject_invalid_cancel_multi";
    const std::string multiPatCsv = multiDir + "/patients.csv";
    const std::string multiDocCsv = multiDir + "/doctors.csv";
    const std::string multiApptCsv = multiDir + "/appointments.csv";

    // E1: Interactive attempt with nonexistent ID "A9999"
    // Flow: Main menu 3 (Appointment) -> 3 (Cancel) -> A9999 -> Y -> 0 (Return) -> 4 (Exit)
    {
      const std::string baselineE1 = readFile(multiApptCsv);
      const int rowsE1 = countDataRows(multiApptCsv);

      hms::FileManager sysFm(multiPatCsv, multiDocCsv, multiApptCsv);
      hms::HospitalSystem sys(sysFm);

      const std::string input = "3\n3\nA9999\nY\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive E1: Displays '--- Cancel Appointment ---'",
            output.find("--- Cancel Appointment ---") != std::string::npos);
      check("Interactive E1: Prompts 'Enter appointment ID: '",
            output.find("Enter appointment ID: ") != std::string::npos);
      check("Interactive E1: Prompts 'Confirm cancellation? (Y/N): '",
            output.find("Confirm cancellation? (Y/N): ") != std::string::npos);
      check("Interactive E1: Displays exact message 'Appointment not found'",
            output.find("Appointment not found") != std::string::npos);
      check("Interactive E1: Returns safely and displays clean exit",
            output.find("Exiting Hospital Management System.") != std::string::npos);
      check("Interactive E1: Row count unchanged", countDataRows(multiApptCsv) == rowsE1);
      check("Interactive E1: CSV byte-for-byte identical", readFile(multiApptCsv) == baselineE1);
    }

    // E2: Interactive attempt with already-cancelled ID "A0002"
    // Flow: Main menu 3 (Appointment) -> 3 (Cancel) -> A0002 -> Y -> 0 (Return) -> 4 (Exit)
    {
      const std::string baselineE2 = readFile(multiApptCsv);
      const int rowsE2 = countDataRows(multiApptCsv);

      hms::FileManager sysFm(multiPatCsv, multiDocCsv, multiApptCsv);
      hms::HospitalSystem sys(sysFm);

      const std::string input = "3\n3\nA0002\nY\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive E2: Displays exact message 'Appointment already cancelled'",
            output.find("Appointment already cancelled") != std::string::npos);
      check("Interactive E2: Returns safely and displays clean exit",
            output.find("Exiting Hospital Management System.") != std::string::npos);
      check("Interactive E2: Row count unchanged", countDataRows(multiApptCsv) == rowsE2);
      check("Interactive E2: CSV byte-for-byte identical", readFile(multiApptCsv) == baselineE2);
    }

    // E3: Interactive decline vs invalid distinction
    // Confirms that declining cancellation outputs 'Cancellation cancelled by user'
    {
      const std::string baselineE3 = readFile(multiApptCsv);
      hms::FileManager sysFm(multiPatCsv, multiDocCsv, multiApptCsv);
      hms::HospitalSystem sys(sysFm);

      const std::string input = "3\n3\nA0001\nN\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive E3: Declined cancellation displays 'Cancellation cancelled by user'",
            output.find("Cancellation cancelled by user") != std::string::npos);
      check("Interactive E3: Does NOT display 'Appointment not found'",
            output.find("Appointment not found") == std::string::npos);
      check("Interactive E3: Does NOT display 'Appointment already cancelled'",
            output.find("Appointment already cancelled") == std::string::npos);
      check("Interactive E3: CSV byte-for-byte identical", readFile(multiApptCsv) == baselineE3);
    }
  }

  // =========================================================================
  // Test F — Regression compatibility
  // =========================================================================
  std::cout << "\n-- Test F: Regression compatibility --\n";
  {
    // Verify valid cancellation succeeds, record is retained as Cancelled
    const std::string multiDir = "build/test_reject_invalid_cancel_multi";
    const std::string multiPatCsv = multiDir + "/patients.csv";
    const std::string multiDocCsv = multiDir + "/doctors.csv";
    const std::string multiApptCsv = multiDir + "/appointments.csv";

    hms::FileManager sysFm(multiPatCsv, multiDocCsv, multiApptCsv);
    hms::HospitalSystem sys(sysFm);

    // Cancel Booked A0001 with confirmation
    const hms::OpResult validCancel = sys.cancelAppointment("A0001", true);
    check("Regression: Valid cancellation succeeds", validCancel.success);
    check("Regression: Message is 'Appointment cancelled successfully'",
          validCancel.message == "Appointment cancelled successfully");

    const hms::Appointment *a1 = sys.findAppointment("A0001");
    check("Regression: A0001 status is Cancelled", a1 != nullptr && a1->isCancelled());
    check("Regression: Record row count remains 5 (retained, not deleted)",
          countDataRows(multiApptCsv) == 5);

    // Now A0001 is Cancelled, slot (D001 on 2026-12-01 at 09:00) is freed (HMS-F-021)
    std::string availMsg;
    const bool available = sys.checkDoctorAvailability("D001", "2026-12-01", "09:00", availMsg);
    check("Regression: Slot freed by cancellation is available", available);

    // Rebook vacated slot
    std::string newId;
    const hms::OpResult rebookRes = sys.bookAppointment("P002", "D001", "2026-12-01", "09:00", newId);
    check("Regression: Rebooking vacated slot succeeds", rebookRes.success);

    // Attempt double-booking on now active slot
    std::string dupId;
    const hms::OpResult dupRes = sys.bookAppointment("P001", "D001", "2026-12-01", "09:00", dupId);
    check("Regression: Double-booking active slot fails", !dupRes.success);

    // Root-level CSV files preserved check
    check("Root patients.csv remains untouched",
          readFile(rootPatientCsv) == rootPatientBaseline);
    check("Root doctors.csv remains untouched",
          readFile(rootDoctorCsv) == rootDoctorBaseline);
    check("Root appointments.csv remains untouched",
          readFile(rootApptCsv) == rootApptBaseline);
  }

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=========================================\n";
  std::cout << "TC-APT-10 Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=========================================\n";

  return g_failed == 0 ? 0 : 1;
}
