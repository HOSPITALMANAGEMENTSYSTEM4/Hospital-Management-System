// ============================================================================
// TC-SEC-06 / TC-APT-09: Retain Cancelled Appointment Records (HMS-SR-006)
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-SR-006 (Security Requirements - Retain Cancelled Records)
//   STP RTM Test Case:  TC-APT-09 (Appointment Cancellation & Retention)
//   SRS RTM Test Case:  TC-SEC-06 (Retain Cancelled Appointment Records)
//   GitHub Issue:       #52
//
// Requirement Statement (HMS-SR-006):
//   "Never physically delete appointment records through mandatory application
//   functions. When an appointment is cancelled, retain the record and update
//   its status to Cancelled."
//
// Acceptance Criteria:
//   1. Cancelling an appointment updates its status to 'Cancelled' without
//      physically deleting the record from memory or persisted storage.
//   2. The total appointment record count remains unchanged after cancellation.
//   3. The appointment's ID, patient ID, doctor ID, date, and time are preserved.
//   4. Other appointments in the system remain completely unaffected.
//   5. Cancelled appointments remain retrievable via existing access mechanisms
//      (findAppointment, listAppointments, viewAppointments).
//   6. Attempting to re-cancel an already-cancelled appointment is rejected
//      without deleting, duplicating, or altering records.
//   7. Vacated slots can be rebooked with new appointments, assigning a new
//      unique ID while retaining the cancelled record.
//   8. Root/production CSV files in the workspace remain completely untouched.
// ============================================================================

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
  std::cout << "=== Running TC-SEC-06 / TC-APT-09: Retain Cancelled Appointment Records Tests ===\n";
  std::cout << "=== (Traceability: HMS-SR-006, TC-SEC-06, TC-APT-09, Issue #52) ===\n\n";

  // Root data files baseline
  const std::string rootPatientCsv = "patients.csv";
  const std::string rootDoctorCsv = "doctors.csv";
  const std::string rootApptCsv = "appointments.csv";

  const std::string rootPatientBaseline = readFile(rootPatientCsv);
  const std::string rootDoctorBaseline = readFile(rootDoctorCsv);
  const std::string rootApptBaseline = readFile(rootApptCsv);

  // Isolated test directory
  const std::string testDir = "build/test_retain_cancelled_records";
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
  hms::Patient p2("P002", "Bob Jones", 45, "M", "9876543210", "456 Oak Ave", "Asthma");
  hms::Doctor d1("D001", "Dr. Gregory House", "Diagnostics", "Diagnostics", "1112223334", "Mon-Fri");
  hms::Doctor d2("D002", "Dr. Allison Cameron", "Immunology", "Immunology", "5556667778", "Tue-Sat");

  check("Seed patient P001", fm.savePatient(p1, err));
  check("Seed patient P002", fm.savePatient(p2, err));
  check("Seed doctor D001", fm.saveDoctor(d1, err));
  check("Seed doctor D002", fm.saveDoctor(d2, err));

  // =========================================================================
  // Test Suite 1: Single Appointment Cancellation & Record Retention (HMS-SR-006)
  // =========================================================================
  std::cout << "\n-- Test Suite 1: Single Appointment Cancellation & Record Retention --\n";
  {
    // Step 1: Create an appointment and record total appointment count
    hms::Appointment appt1("A0001", "P001", "D001", "2026-11-10", "10:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0001 (Booked)", fm.saveAppointment(appt1, err));

    hms::HospitalSystem system(fm);

    const int initialCsvCount = countDataRows(testApptCsv);
    check("Initial CSV appointment count is 1", initialCsvCount == 1);

    const auto initialList = system.listAppointments("ALL");
    const std::size_t initialMemCount = initialList.size();
    check("Initial in-memory appointment count is 1", initialMemCount == 1);

    // Step 2: Cancel appointment using actual cancellation path
    const hms::OpResult cancelRes = system.cancelAppointment("A0001", true);
    check("Cancellation operation succeeds", cancelRes.success);
    check("Cancellation returns success message",
          cancelRes.message == "Appointment cancelled successfully");

    // Step 3: Verify the appointment record still exists
    const hms::Appointment *retained = system.findAppointment("A0001");
    check("Appointment A0001 still exists after cancellation (not deleted)", retained != nullptr);

    // Step 4: Verify status is exactly Cancelled
    if (retained) {
      check("Status is Cancelled", retained->isCancelled());
      check("isBooked is false", !retained->isBooked());
      check("Status string is exactly 'Cancelled'", retained->getStatusString() == "Cancelled");
      check("Status enum is AppointmentStatus::Cancelled",
            retained->getStatus() == hms::AppointmentStatus::Cancelled);

      // Step 6: Verify appointment ID and all other details remain intact
      check("ID remains A0001", retained->getId() == "A0001");
      check("Patient ID remains P001", retained->getPatientId() == "P001");
      check("Doctor ID remains D001", retained->getDoctorId() == "D001");
      check("Date remains 2026-11-10", retained->getDate() == "2026-11-10");
      check("Time remains 10:00", retained->getTime() == "10:00");
    }

    // Step 5: Verify total appointment count is unchanged (not reduced)
    const int postCsvCount = countDataRows(testApptCsv);
    check("CSV record count remains 1 after cancellation (not reduced)", postCsvCount == initialCsvCount);

    const auto postList = system.listAppointments("ALL");
    check("In-memory record count remains 1 after cancellation (not reduced)",
          postList.size() == initialMemCount);

    // Verify CSV file contains the updated cancelled record
    const std::string csvContent = readFile(testApptCsv);
    check("CSV file contains retained record with Cancelled status",
          csvContent.find("A0001,P001,D001,2026-11-10,10:00,Cancelled") != std::string::npos);
  }

  // =========================================================================
  // Test Suite 2: Multi-Appointment Dataset Integrity & Isolation
  // =========================================================================
  std::cout << "\n-- Test Suite 2: Multi-Appointment Dataset Integrity & Isolation --\n";
  {
    // Clean and re-seed with 4 appointments
    removeFile(testApptCsv);

    std::vector<hms::Appointment> multiList = {
      hms::Appointment("A0001", "P001", "D001", "2026-12-01", "09:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0002", "P002", "D001", "2026-12-01", "10:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0003", "P001", "D002", "2026-12-01", "11:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0004", "P002", "D002", "2026-12-01", "14:00", hms::AppointmentStatus::Booked)
    };
    check("Seed 4 appointments", fm.saveAppointments(multiList, err));

    hms::HospitalSystem system(fm);

    const int initialCount = countDataRows(testApptCsv);
    check("Initial multi-dataset CSV count is 4", initialCount == 4);
    check("Initial multi-dataset in-memory count is 4", system.listAppointments("ALL").size() == 4);

    // Cancel appointment A0002
    const hms::OpResult resA2 = system.cancelAppointment("A0002", true);
    check("Cancel A0002 succeeds", resA2.success);

    // Total count must remain 4
    check("CSV count remains 4 after cancelling A0002", countDataRows(testApptCsv) == 4);
    check("In-memory count remains 4 after cancelling A0002", system.listAppointments("ALL").size() == 4);

    // Target appointment A0002 is cancelled and retained
    const hms::Appointment *a2 = system.findAppointment("A0002");
    check("A0002 still exists", a2 != nullptr);
    if (a2) {
      check("A0002 status is Cancelled", a2->isCancelled());
      check("A0002 details intact",
            a2->getId() == "A0002" && a2->getPatientId() == "P002" &&
            a2->getDoctorId() == "D001" && a2->getDate() == "2026-12-01" &&
            a2->getTime() == "10:00");
    }

    // Verify other appointments (A0001, A0003, A0004) remain completely unaffected
    const hms::Appointment *a1 = system.findAppointment("A0001");
    check("A0001 unaffected: still exists", a1 != nullptr);
    if (a1) {
      check("A0001 still Booked", a1->isBooked());
      check("A0001 details intact",
            a1->getId() == "A0001" && a1->getPatientId() == "P001" &&
            a1->getDoctorId() == "D001" && a1->getDate() == "2026-12-01" &&
            a1->getTime() == "09:00");
    }

    const hms::Appointment *a3 = system.findAppointment("A0003");
    check("A0003 unaffected: still exists", a3 != nullptr);
    if (a3) {
      check("A0003 still Booked", a3->isBooked());
      check("A0003 details intact",
            a3->getId() == "A0003" && a3->getPatientId() == "P001" &&
            a3->getDoctorId() == "D002" && a3->getDate() == "2026-12-01" &&
            a3->getTime() == "11:00");
    }

    const hms::Appointment *a4 = system.findAppointment("A0004");
    check("A0004 unaffected: still exists", a4 != nullptr);
    if (a4) {
      check("A0004 still Booked", a4->isBooked());
      check("A0004 details intact",
            a4->getId() == "A0004" && a4->getPatientId() == "P002" &&
            a4->getDoctorId() == "D002" && a4->getDate() == "2026-12-01" &&
            a4->getTime() == "14:00");
    }

    // Cancel another appointment A0004
    const hms::OpResult resA4 = system.cancelAppointment("A0004", true);
    check("Cancel A0004 succeeds", resA4.success);

    // Total count must still remain 4
    check("CSV count remains 4 after cancelling A0004", countDataRows(testApptCsv) == 4);
    check("In-memory count remains 4 after cancelling A0004", system.listAppointments("ALL").size() == 4);

    // Verify 2 Booked, 2 Cancelled
    const auto allPost = system.listAppointments("ALL");
    int bookedCount = 0;
    int cancelledCount = 0;
    for (const auto &item : allPost) {
      if (item.isBooked()) ++bookedCount;
      if (item.isCancelled()) ++cancelledCount;
    }
    check("Total Booked count is 2", bookedCount == 2);
    check("Total Cancelled count is 2", cancelledCount == 2);
  }

  // =========================================================================
  // Test Suite 3: Persistence and Durability Across System Restarts
  // =========================================================================
  std::cout << "\n-- Test Suite 3: Persistence and Durability Across System Restarts --\n";
  {
    // Re-instantiate HospitalSystem from the persisted CSV files
    hms::FileManager restartFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem restartSys(restartFm);

    const auto loadedList = restartSys.listAppointments("ALL");
    check("Restarted system loads exactly 4 appointments", loadedList.size() == 4);

    const hms::Appointment *loadedA2 = restartSys.findAppointment("A0002");
    check("Reloaded A0002 exists", loadedA2 != nullptr);
    if (loadedA2) {
      check("Reloaded A0002 is Cancelled", loadedA2->isCancelled());
      check("Reloaded A0002 status string is 'Cancelled'", loadedA2->getStatusString() == "Cancelled");
      check("Reloaded A0002 fields intact",
            loadedA2->getPatientId() == "P002" && loadedA2->getDoctorId() == "D001" &&
            loadedA2->getDate() == "2026-12-01" && loadedA2->getTime() == "10:00");
    }

    const hms::Appointment *loadedA4 = restartSys.findAppointment("A0004");
    check("Reloaded A0004 exists", loadedA4 != nullptr);
    if (loadedA4) {
      check("Reloaded A0004 is Cancelled", loadedA4->isCancelled());
      check("Reloaded A0004 fields intact",
            loadedA4->getPatientId() == "P002" && loadedA4->getDoctorId() == "D002" &&
            loadedA4->getDate() == "2026-12-01" && loadedA4->getTime() == "14:00");
    }

    const hms::Appointment *loadedA1 = restartSys.findAppointment("A0001");
    check("Reloaded A0001 exists and is Booked", loadedA1 != nullptr && loadedA1->isBooked());

    const hms::Appointment *loadedA3 = restartSys.findAppointment("A0003");
    check("Reloaded A0003 exists and is Booked", loadedA3 != nullptr && loadedA3->isBooked());
  }

  // =========================================================================
  // Test Suite 4: Record Retrievability via All Record-Access Mechanisms
  // =========================================================================
  std::cout << "\n-- Test Suite 4: Record Retrievability via Existing Access Mechanisms --\n";
  {
    hms::HospitalSystem system(fm);

    // 1. Direct lookup by ID via findAppointment
    const hms::Appointment *found = system.findAppointment("A0002");
    check("findAppointment retrieves cancelled record A0002", found != nullptr && found->isCancelled());

    // 2. Listing all appointments includes cancelled records
    const auto listAll = system.listAppointments("ALL");
    bool foundInAll = false;
    for (const auto &a : listAll) {
      if (a.getId() == "A0002" && a.isCancelled()) foundInAll = true;
    }
    check("listAppointments('ALL') contains cancelled record A0002", foundInAll);

    // 3. Filter by patient includes cancelled records
    const auto listPat = system.listAppointments("PATIENT", "P002");
    check("listAppointments('PATIENT', 'P002') returns 2 appointments", listPat.size() == 2);
    int p2Cancelled = 0;
    for (const auto &a : listPat) {
      if (a.isCancelled()) ++p2Cancelled;
    }
    check("listAppointments('PATIENT', 'P002') contains 2 cancelled records", p2Cancelled == 2);

    // 4. Filter by doctor includes cancelled records
    const auto listDoc = system.listAppointments("DOCTOR", "D001");
    check("listAppointments('DOCTOR', 'D001') returns 2 appointments", listDoc.size() == 2);
    bool docHasBooked = false;
    bool docHasCancelled = false;
    for (const auto &a : listDoc) {
      if (a.getId() == "A0001" && a.isBooked()) docHasBooked = true;
      if (a.getId() == "A0002" && a.isCancelled()) docHasCancelled = true;
    }
    check("listAppointments('DOCTOR', 'D001') includes both Booked A0001 and Cancelled A0002",
          docHasBooked && docHasCancelled);

    // 5. Console view output formats cancelled status
    const std::string viewAll = captureViewAppointments(system, "ALL");
    check("viewAppointments contains A0002", viewAll.find("A0002") != std::string::npos);
    check("viewAppointments displays 'Status: Cancelled'", viewAll.find("Status: Cancelled") != std::string::npos);
  }

  // =========================================================================
  // Test Suite 5: Duplicate Cancellation & Error Handling (No Deletion)
  // =========================================================================
  std::cout << "\n-- Test Suite 5: Duplicate Cancellation & Error Handling --\n";
  {
    hms::HospitalSystem system(fm);

    const int countBefore = countDataRows(testApptCsv);
    const std::string bytesBefore = readFile(testApptCsv);

    // Attempt to cancel an already-cancelled appointment A0002
    const hms::OpResult reCancelRes = system.cancelAppointment("A0002", true);
    check("Re-cancelling already-cancelled appointment returns false", !reCancelRes.success);
    check("Re-cancelling returns 'Appointment already cancelled'",
          reCancelRes.message == "Appointment already cancelled");
    check("Re-cancellation does NOT delete the record (count unchanged)",
          countDataRows(testApptCsv) == countBefore);
    check("Re-cancellation leaves CSV file byte-for-byte identical",
          readFile(testApptCsv) == bytesBefore);

    // Attempt to cancel a nonexistent appointment A9999
    const hms::OpResult nonExistentRes = system.cancelAppointment("A9999", true);
    check("Cancelling nonexistent appointment returns false", !nonExistentRes.success);
    check("Nonexistent cancellation returns 'Appointment not found'",
          nonExistentRes.message == "Appointment not found");
    check("Nonexistent cancellation does NOT change count",
          countDataRows(testApptCsv) == countBefore);
    check("Nonexistent cancellation leaves CSV file identical",
          readFile(testApptCsv) == bytesBefore);

    // Attempt cancellation with confirmed = false
    const hms::OpResult unconfirmedRes = system.cancelAppointment("A0001", false);
    check("Unconfirmed cancellation returns false", !unconfirmedRes.success);
    check("Unconfirmed returns 'Cancellation cancelled by user'",
          unconfirmedRes.message == "Cancellation cancelled by user");
    check("Unconfirmed leaves A0001 Booked",
          system.findAppointment("A0001") != nullptr && system.findAppointment("A0001")->isBooked());
    check("Unconfirmed cancellation leaves CSV file identical",
          readFile(testApptCsv) == bytesBefore);
  }

  // =========================================================================
  // Test Suite 6: Interactive Cancellation Flow & Confirmation
  // =========================================================================
  std::cout << "\n-- Test Suite 6: Interactive Cancellation Flow & Confirmation --\n";
  {
    // Interactive confirmation: cancel A0001
    // Menu: 3 (Appointment) -> 3 (Cancel) -> A0001 -> Y -> 0 (Return) -> 4 (Exit)
    {
      hms::HospitalSystem sys(fm);
      const int rowsBefore = countDataRows(testApptCsv);

      const std::string input = "3\n3\nA0001\nY\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive: Displays '--- Cancel Appointment ---'",
            output.find("--- Cancel Appointment ---") != std::string::npos);
      check("Interactive: Prompts for ID",
            output.find("Enter appointment ID: ") != std::string::npos);
      check("Interactive: Prompts for confirmation",
            output.find("Confirm cancellation? (Y/N): ") != std::string::npos);
      check("Interactive: Displays 'Appointment cancelled successfully'",
            output.find("Appointment cancelled successfully") != std::string::npos);

      const hms::Appointment *a1 = sys.findAppointment("A0001");
      check("Interactive: A0001 still exists", a1 != nullptr);
      check("Interactive: A0001 status is Cancelled", a1 != nullptr && a1->isCancelled());
      check("Interactive: Record count remains unchanged (retained, not deleted)",
            countDataRows(testApptCsv) == rowsBefore);
    }

    // Interactive decline: attempt to cancel A0003 with 'N'
    // Menu: 3 (Appointment) -> 3 (Cancel) -> A0003 -> N -> 0 (Return) -> 4 (Exit)
    {
      hms::HospitalSystem sys(fm);
      const int rowsBefore = countDataRows(testApptCsv);
      const std::string bytesBefore = readFile(testApptCsv);

      const std::string input = "3\n3\nA0003\nN\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive decline: Displays 'Cancellation cancelled by user'",
            output.find("Cancellation cancelled by user") != std::string::npos);

      const hms::Appointment *a3 = sys.findAppointment("A0003");
      check("Interactive decline: A0003 remains Booked", a3 != nullptr && a3->isBooked());
      check("Interactive decline: Record count unchanged",
            countDataRows(testApptCsv) == rowsBefore);
      check("Interactive decline: CSV file byte-for-byte identical",
            readFile(testApptCsv) == bytesBefore);
    }
  }

  // =========================================================================
  // Test Suite 7: Unrelated Operations Preserved (Rebooking & ID Sequence)
  // =========================================================================
  std::cout << "\n-- Test Suite 7: Unrelated Operations Preserved (Rebooking & ID Sequence) --\n";
  {
    hms::HospitalSystem system(fm);

    // A0002 was D001 on 2026-12-01 at 10:00 (now Cancelled)
    // Verify slot is now available for rebooking
    std::string availMsg;
    const bool available = system.checkDoctorAvailability("D001", "2026-12-01", "10:00", availMsg);
    check("Doctor D001 slot 2026-12-01 10:00 is available after cancellation", available);

    const int countBeforeRebook = countDataRows(testApptCsv);

    // Rebook the vacated slot
    std::string newId;
    const hms::OpResult rebookRes = system.bookAppointment("P001", "D001", "2026-12-01", "10:00", newId);
    check("Rebooking vacated slot succeeds", rebookRes.success);
    check("New appointment assigned sequential ID (A0005)", newId == "A0005");

    // Total count now increments from 4 to 5 (both cancelled A0002 and new A0005 exist)
    check("Total CSV appointment count increased to 5", countDataRows(testApptCsv) == countBeforeRebook + 1);

    // Both records coexist in the system
    const hms::Appointment *cancelledRecord = system.findAppointment("A0002");
    check("Original cancelled record A0002 still exists",
          cancelledRecord != nullptr && cancelledRecord->isCancelled());

    const hms::Appointment *newRecord = system.findAppointment(newId);
    check("New booked record A0005 exists with status Booked",
          newRecord != nullptr && newRecord->isBooked());

    // Subsequent double-booking of now-occupied slot is rejected
    std::string conflictId;
    const hms::OpResult conflictRes = system.bookAppointment("P002", "D001", "2026-12-01", "10:00", conflictId);
    check("Double-booking occupied slot is rejected", !conflictRes.success);

    // ID generation never re-uses cancelled appointment IDs
    const std::string nextId = system.nextAppointmentId();
    check("nextAppointmentId does not reuse cancelled A0001 or A0002",
          nextId != "A0001" && nextId != "A0002" && nextId != "A0004" && nextId != "A0005");
  }

  // =========================================================================
  // Test Suite 8: Root CSV Preservation
  // =========================================================================
  std::cout << "\n-- Test Suite 8: Root CSV Preservation --\n";
  {
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
  std::cout << "TC-SEC-06 / TC-APT-09 Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=========================================\n";

  return g_failed == 0 ? 0 : 1;
}

