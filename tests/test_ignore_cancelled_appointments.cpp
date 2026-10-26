// TC-APT-07 / TC-APT-05: Ignore Cancelled Appointments in Conflicts
// Traceability:
//   - SRS Requirement: HMS-F-021
//   - STP RTM Test Case: TC-APT-05
//   - SRS RTM Test Case: TC-APT-07
//   - Dependency: HMS-F-023 (Cancel Appointment)
//   - Status: Mismatch documented in Issue #25 (STP vs SRS)
//
// Acceptance Requirements:
//   1. Test A: Valid initial appointment is booked and persisted with status Booked.
//   2. Test B: Active appointment blocks duplicate booking for same doctor/date/time.
//      - Rejection message: "Doctor not available at the requested date and time".
//      - Appointment count and CSV content remain unchanged.
//   3. Test C: Cancel original appointment and rebook same slot.
//      - Cancellation succeeds and original record remains stored with status Cancelled.
//      - Rebooking the same doctor/date/time succeeds with a different patient.
//      - New appointment is persisted with status Booked and a new unique ID.
//      - Both original cancelled record and new booked record remain present.
//   4. Test D: New active booking blocks another duplicate attempt.
//      - Subsequent attempt to book the same slot is rejected because the new booking is active.
//      - Stored appointment count and CSV content remain unchanged after rejection.
//   5. Test E: Regression and persistence durability across system reload.
//      - Unrelated doctors, dates, and times remain independent slots.
//      - System reload from persisted CSVs preserves cancelled status and active status.
//      - No duplicate appointment IDs occur.
//      - Active appointment still blocks conflicting booking after restart.
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

} // namespace

int main() {
  std::cout << "=== Running TC-APT-07 Ignore Cancelled Appointments in Conflicts Tests ===\n";
  std::cout << "=== (Traceability: HMS-F-021, STP: TC-APT-05, SRS: TC-APT-07, Issue #25) ===\n\n";

  const std::string testPatientCsv = "build/test_ignore_cancelled_patients.csv";
  const std::string testDoctorCsv = "build/test_ignore_cancelled_doctors.csv";
  const std::string testApptCsv = "build/test_ignore_cancelled_appointments.csv";

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

  hms::HospitalSystem system(fm);

  // =========================================================================
  // Test A — Initial appointment
  // =========================================================================
  std::cout << "\n-- Test A: Initial Appointment Booking (D001, 2026-10-15 10:00) --\n";
  std::string apptIdA;
  std::string errA;
  bool bookedA = system.bookAppointment("P001", "D001", "2026-10-15", "10:00", apptIdA, errA);
  check("Initial booking succeeds", bookedA);
  check("Initial appointment ID is A0001", apptIdA == "A0001");
  check("Initial booking error message is empty", errA.empty());

  const hms::Appointment *apptA = system.findAppointment("A0001");
  check("Initial appointment A0001 exists in system", apptA != nullptr);
  if (apptA) {
    check("A0001 status is Booked", apptA->isBooked());
    check("A0001 status string is 'Booked'", apptA->getStatusString() == "Booked");
    check("A0001 doctor is D001", apptA->getDoctorId() == "D001");
    check("A0001 patient is P001", apptA->getPatientId() == "P001");
    check("A0001 date is 2026-10-15", apptA->getDate() == "2026-10-15");
    check("A0001 time is 10:00", apptA->getTime() == "10:00");
  }

  const int rowsAfterA = countDataRows(testApptCsv);
  const std::string contentAfterA = readFile(testApptCsv);
  check("appointments.csv contains exactly 1 row after initial booking", rowsAfterA == 1);
  check("appointments.csv contains 'A0001,P001,D001,2026-10-15,10:00,Booked'",
        contentAfterA.find("A0001,P001,D001,2026-10-15,10:00,Booked") != std::string::npos);

  // =========================================================================
  // Test B — Active appointment blocks a duplicate
  // =========================================================================
  std::cout << "\n-- Test B: Active Appointment Blocks Duplicate Booking --\n";
  {
    std::string dupId;
    std::string dupErr;
    bool dupBooked = system.bookAppointment("P002", "D001", "2026-10-15", "10:00", dupId, dupErr);
    check("Duplicate booking for active slot is rejected", !dupBooked);
    check("Exact error message is 'Doctor not available at the requested date and time'",
          dupErr == "Doctor not available at the requested date and time");
    check("Output appointment ID remains empty on rejection", dupId.empty());
    check("Appointment count remains unchanged after rejection", countDataRows(testApptCsv) == rowsAfterA);
    check("appointments.csv is completely unmodified after rejection", readFile(testApptCsv) == contentAfterA);

    // Also verify via OpResult API
    std::string opId;
    hms::OpResult opRes = system.bookAppointment("P002", "D001", "2026-10-15", "10:00", opId);
    check("OpResult booking returns false for active conflict", !opRes.success);
    check("OpResult message matches required conflict message",
          opRes.message == "Doctor not available at the requested date and time");
    check("OpResult output ID remains empty", opId.empty());

    // Also verify checkDoctorAvailability
    std::string availMsg;
    bool avail = system.checkDoctorAvailability("D001", "2026-10-15", "10:00", availMsg);
    check("checkDoctorAvailability returns false for active slot", !avail);
    check("checkDoctorAvailability message is 'Doctor not available at the requested date and time'",
          availMsg == "Doctor not available at the requested date and time");
  }

  // =========================================================================
  // Test C — Cancel and rebook the same slot
  // =========================================================================
  std::cout << "\n-- Test C: Cancel and Rebook Same Slot (HMS-F-021 / HMS-F-023) --\n";
  {
    // 1. Cancel the original appointment A0001
    hms::OpResult cancelRes = system.cancelAppointment("A0001", true);
    check("Cancel appointment A0001 succeeds", cancelRes.success);
    check("Cancel message is 'Appointment cancelled successfully'",
          cancelRes.message == "Appointment cancelled successfully");

    // Verify original record still exists and its status is Cancelled
    const hms::Appointment *cancelledA = system.findAppointment("A0001");
    check("Original appointment A0001 still exists after cancellation", cancelledA != nullptr);
    if (cancelledA) {
      check("Original appointment status is Cancelled", cancelledA->isCancelled());
      check("Original appointment status string is 'Cancelled'", cancelledA->getStatusString() == "Cancelled");
    }

    // Verify CSV persistent storage retains A0001 with Cancelled
    const std::string csvAfterCancel = readFile(testApptCsv);
    check("Persistent CSV contains A0001 with Cancelled status",
          csvAfterCancel.find("A0001,P001,D001,2026-10-15,10:00,Cancelled") != std::string::npos);
    check("Row count remains 1 after cancellation (audit trail retained)", countDataRows(testApptCsv) == 1);

    // Verify slot is now reported as Available
    std::string availMsg;
    bool isAvail = system.checkDoctorAvailability("D001", "2026-10-15", "10:00", availMsg);
    check("Slot D001 at 2026-10-15 10:00 is now Available after cancellation", isAvail);
    check("Availability message is 'Available'", availMsg == "Available");

    // 2. Rebook the same doctor/date/time with a different valid patient (P002)
    std::string rebookId;
    std::string rebookErr;
    bool rebooked = system.bookAppointment("P002", "D001", "2026-10-15", "10:00", rebookId, rebookErr);
    check("Rebooking the same doctor, date, and time succeeds", rebooked);
    check("Rebooked appointment receives a new unique ID (A0002)", rebookId == "A0002");
    check("Rebooking error message is empty", rebookErr.empty());

    // Verify new appointment exists in system with status Booked
    const hms::Appointment *newAppt = system.findAppointment("A0002");
    check("New appointment A0002 exists in system", newAppt != nullptr);
    if (newAppt) {
      check("New appointment status is Booked", newAppt->isBooked());
      check("New appointment doctor is D001", newAppt->getDoctorId() == "D001");
      check("New appointment patient is P002", newAppt->getPatientId() == "P002");
      check("New appointment date is 2026-10-15", newAppt->getDate() == "2026-10-15");
      check("New appointment time is 10:00", newAppt->getTime() == "10:00");
    }

    // 3. Confirm that BOTH original cancelled record and new booked record remain present
    const std::string csvAfterRebook = readFile(testApptCsv);
    check("CSV row count is now exactly 2", countDataRows(testApptCsv) == 2);
    check("CSV contains original cancelled record A0001",
          csvAfterRebook.find("A0001,P001,D001,2026-10-15,10:00,Cancelled") != std::string::npos);
    check("CSV contains new booked record A0002",
          csvAfterRebook.find("A0002,P002,D001,2026-10-15,10:00,Booked") != std::string::npos);

    const auto allAppts = system.listAppointments("ALL");
    check("In-memory list contains exactly 2 appointments", allAppts.size() == 2);
  }

  // =========================================================================
  // Test D — New active booking blocks another duplicate
  // =========================================================================
  std::cout << "\n-- Test D: New Active Booking Blocks Another Duplicate --\n";
  {
    const int rowsBeforeD = countDataRows(testApptCsv);
    const std::string contentBeforeD = readFile(testApptCsv);

    std::string thirdId;
    std::string thirdErr;
    bool thirdBooked = system.bookAppointment("P003", "D001", "2026-10-15", "10:00", thirdId, thirdErr);
    check("Subsequent booking attempt for rebooked slot is rejected", !thirdBooked);
    check("Exact error message on subsequent rejection is 'Doctor not available at the requested date and time'",
          thirdErr == "Doctor not available at the requested date and time");
    check("Third booking attempt output ID remains empty", thirdId.empty());
    check("Stored appointment row count unchanged after rejection", countDataRows(testApptCsv) == rowsBeforeD);
    check("appointments.csv remains byte-for-byte identical after rejection", readFile(testApptCsv) == contentBeforeD);

    // Verify checkDoctorAvailability reports not available
    std::string availMsg;
    bool availAfterRebook = system.checkDoctorAvailability("D001", "2026-10-15", "10:00", availMsg);
    check("checkDoctorAvailability reports not available because A0002 is active", !availAfterRebook);
    check("Availability check message is 'Doctor not available at the requested date and time'",
          availMsg == "Doctor not available at the requested date and time");
  }

  // =========================================================================
  // Test E — Regression, Multi-Slot Independence, and Persistence Reload
  // =========================================================================
  std::cout << "\n-- Test E: Regression, Multi-Slot Independence, and Persistence Reload --\n";
  {
    // 1. Independent slots: Different doctor, same date & time (D002, 2026-10-15 10:00)
    std::string doc2Id, doc2Err;
    bool doc2Booked = system.bookAppointment("P001", "D002", "2026-10-15", "10:00", doc2Id, doc2Err);
    check("Different doctor D002 at same date and time succeeds", doc2Booked);
    check("Doctor D002 appointment ID is A0003", doc2Id == "A0003");

    // 2. Independent slots: Same doctor, different time (D001, 2026-10-15 11:00)
    std::string time2Id, time2Err;
    bool time2Booked = system.bookAppointment("P003", "D001", "2026-10-15", "11:00", time2Id, time2Err);
    check("Same doctor D001 at different time succeeds", time2Booked);
    check("Same doctor different time appointment ID is A0004", time2Id == "A0004");

    // 3. Independent slots: Same doctor, different date (D001, 2026-10-16 10:00)
    std::string date2Id, date2Err;
    bool date2Booked = system.bookAppointment("P001", "D001", "2026-10-16", "10:00", date2Id, date2Err);
    check("Same doctor D001 on different date succeeds", date2Booked);
    check("Same doctor different date appointment ID is A0005", date2Id == "A0005");

    check("CSV now contains 5 rows total", countDataRows(testApptCsv) == 5);

    // 4. Durability and Behavior Across Restart / Reload
    std::cout << "-- Simulating system restart / reload from persisted CSV files --\n";
    hms::FileManager reloadFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem reloadSys(reloadFm);

    // Verify loaded records
    const auto reloadedAppts = reloadSys.listAppointments("ALL");
    check("Reloaded system has 5 appointments", reloadedAppts.size() == 5);

    // Verify cancelled appointment A0001 is still cancelled after reload
    const hms::Appointment *reloadedA0001 = reloadSys.findAppointment("A0001");
    check("A0001 exists after reload", reloadedA0001 != nullptr);
    if (reloadedA0001) {
      check("A0001 is still Cancelled after reload", reloadedA0001->isCancelled());
      check("A0001 status string is 'Cancelled'", reloadedA0001->getStatusString() == "Cancelled");
    }

    // Verify active appointment A0002 is still Booked after reload
    const hms::Appointment *reloadedA0002 = reloadSys.findAppointment("A0002");
    check("A0002 exists after reload", reloadedA0002 != nullptr);
    if (reloadedA0002) {
      check("A0002 is still Booked after reload", reloadedA0002->isBooked());
      check("A0002 status string is 'Booked'", reloadedA0002->getStatusString() == "Booked");
    }

    // Verify that new active appointment still blocks conflict after reload
    std::string reloadBlockId, reloadBlockErr;
    bool reloadBlocked = reloadSys.bookAppointment("P003", "D001", "2026-10-15", "10:00",
                                                   reloadBlockId, reloadBlockErr);
    check("Conflicting booking still blocked after reload", !reloadBlocked);
    check("Blocked message after reload is 'Doctor not available at the requested date and time'",
          reloadBlockErr == "Doctor not available at the requested date and time");
    check("Output ID remains empty after reload blocked attempt", reloadBlockId.empty());

    // 5. Verify subsequent cancellation of A0002 frees slot again (lifecycle repetition)
    hms::OpResult cancel2Res = reloadSys.cancelAppointment("A0002", true);
    check("Second active appointment A0002 cancelled successfully", cancel2Res.success);

    // Slot should now be available again even though TWO cancelled appointments exist for D001 at 2026-10-15 10:00!
    std::string reAvailMsg;
    bool isNowReAvail = reloadSys.checkDoctorAvailability("D001", "2026-10-15", "10:00", reAvailMsg);
    check("Slot D001 at 2026-10-15 10:00 is Available with TWO cancelled records in history",
          isNowReAvail && reAvailMsg == "Available");

    // Rebook third time into same slot
    std::string thirdBookingId, thirdBookingErr;
    bool thirdBookingOk = reloadSys.bookAppointment("P003", "D001", "2026-10-15", "10:00",
                                                    thirdBookingId, thirdBookingErr);
    check("Rebooking third time into slot with multiple cancelled predecessors succeeds", thirdBookingOk);
    check("Third booking gets unique sequential ID A0006", thirdBookingId == "A0006");

    // Verify both A0001 (Cancelled), A0002 (Cancelled), and A0006 (Booked) coexist in storage
    const std::string finalCsv = readFile(testApptCsv);
    check("CSV contains A0001 Cancelled", finalCsv.find("A0001,P001,D001,2026-10-15,10:00,Cancelled") != std::string::npos);
    check("CSV contains A0002 Cancelled", finalCsv.find("A0002,P002,D001,2026-10-15,10:00,Cancelled") != std::string::npos);
    check("CSV contains A0006 Booked", finalCsv.find("A0006,P003,D001,2026-10-15,10:00,Booked") != std::string::npos);
    check("Final CSV has 6 data rows total", countDataRows(testApptCsv) == 6);

    // Re-verify that now the third booking blocks new attempts
    std::string finalBlockId, finalBlockErr;
    bool finalBlocked = reloadSys.bookAppointment("P001", "D001", "2026-10-15", "10:00",
                                                  finalBlockId, finalBlockErr);
    check("Attempt to book slot occupied by A0006 is blocked", !finalBlocked);
    check("Final block error message matches",
          finalBlockErr == "Doctor not available at the requested date and time");
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
  std::cout << "TC-APT-07 Ignore Cancelled Appointments in Conflicts: "
            << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
