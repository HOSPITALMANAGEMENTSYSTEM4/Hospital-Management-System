// ============================================================================
// TC-NFR-08: Appointment Conflict Correctness Test Suite
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-NF-008 (Non-Functional Requirement - Conflict Correctness)
//   STP RTM Test Case:  TC-NFR-08 (Software Test Plan Table 14 / Section 9)
//   SRS RTM Test Case:  TC-NFR-08 (Software Requirements Specification Table 12)
//   GitHub Issue:       #41
//
// Requirement Statement (HMS-NF-008):
//   "Appointment conflict correctness: Achieve correct outcomes for all four
//    defined doctor/date/time conflict scenarios."
//
// Four Defined Conflict Scenarios:
//   Case 1: Same doctor, same date/time slot     -> REJECTED
//   Case 2: Same doctor, different time          -> ACCEPTED
//   Case 3: Different doctor, same date/time slot -> ACCEPTED
//   Case 4: Same slot after cancellation         -> ACCEPTED
//
// Additional Regression Requirements:
//   - Case-insensitive doctor IDs behave consistently with existing conventions.
//   - An active, non-cancelled appointment blocks conflicting bookings.
//   - A cancelled appointment does not block the slot.
//   - Different doctors, dates, and times remain independent.
//   - Rejected bookings do not modify appointment count or CSV content.
//   - Conflict behavior remains correct after reloading persisted appointments
//     into a fresh application/system instance.
//   - Validation precedence: format/existence errors precede conflict checks.
//   - Root-level user CSV data files remain untouched.
// ============================================================================

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <algorithm>
#include <cstddef>
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

void check(const std::string &name, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << name << '\n';
    ++g_passed;
  } else {
    std::cerr << "  [FAIL] " << name << '\n';
    ++g_failed;
  }
}

std::string readFileContent(const fs::path &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

int countDataRows(const fs::path &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    return 0;
  }
  int count = 0;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }
    // Skip header line
    if (line.rfind("AppointmentID", 0) == 0 ||
        line.rfind("appointment_id", 0) == 0 ||
        line.rfind("patient_id", 0) == 0 ||
        line.rfind("doctor_id", 0) == 0) {
      continue;
    }
    ++count;
  }
  return count;
}

} // namespace

int main() {
  std::cout << "====================================================================\n";
  std::cout << "TC-NFR-08: Appointment Conflict Correctness Regression Tests\n";
  std::cout << "Traceability: HMS-NF-008 | STP: TC-NFR-08 | SRS: TC-NFR-08 | Issue #41\n";
  std::cout << "====================================================================\n\n";

  // Record root-level CSV state before running tests to verify preservation
  const std::string rootPatientCsvBefore = readFileContent("patients.csv");
  const std::string rootDoctorCsvBefore = readFileContent("doctors.csv");
  const std::string rootApptCsvBefore = readFileContent("appointments.csv");

  // Isolated test directory
  const fs::path testDir = fs::path("build") / "TC-NFR-08-AppointmentConflictCorrectness";
  std::error_code ec;
  fs::remove_all(testDir, ec);
  fs::create_directories(testDir, ec);

  if (ec) {
    std::cerr << "Fatal: Could not create test directory: " << ec.message() << '\n';
    return 1;
  }

  const fs::path testPatientCsv = testDir / "patients.csv";
  const fs::path testDoctorCsv = testDir / "doctors.csv";
  const fs::path testApptCsv = testDir / "appointments.csv";

  hms::FileManager fm(testPatientCsv.string(), testDoctorCsv.string(), testApptCsv.string());

  // -------------------------------------------------------------------------
  // Setup: Seed registered patients and doctors in isolated test fixture
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding registered patients and doctors --\n";
  {
    std::string seedErr;
    const hms::Patient p1("P001", "John Doe", 35, "M", "9876543210", "123 Main St", "None");
    const hms::Patient p2("P002", "Jane Smith", 28, "F", "9876543211", "456 Oak Ave", "Asthma");
    const hms::Patient p3("P003", "Bob Wilson", 45, "M", "9876543212", "789 Pine Rd", "Hypertension");

    check("Seed Patient P001", fm.savePatient(p1, seedErr));
    check("Seed Patient P002", fm.savePatient(p2, seedErr));
    check("Seed Patient P003", fm.savePatient(p3, seedErr));

    const hms::Doctor d1("D001", "Dr. Alice Adams", "Cardiology", "Cardiology",
                         "9123456780", "Mon-Fri 09:00-17:00");
    const hms::Doctor d2("D002", "Dr. Brian Baker", "Dermatology", "Dermatology",
                         "9123456781", "Mon-Fri 09:00-17:00");

    check("Seed Doctor D001", fm.saveDoctor(d1, seedErr));
    check("Seed Doctor D002", fm.saveDoctor(d2, seedErr));

    check("Patient file exists and has 3 data rows", countDataRows(testPatientCsv) == 3);
    check("Doctor file exists and has 2 data rows", countDataRows(testDoctorCsv) == 2);
    check("Appointment file initially empty or nonexistent", countDataRows(testApptCsv) == 0);
  }

  hms::HospitalSystem system(fm);

  // =========================================================================
  // Case 1: Same doctor, same date and time slot -> REJECTED
  // =========================================================================
  std::cout << "\n-- Case 1: Same doctor, same date and time slot (REJECTED) --\n";
  {
    // 1.1 Book initial valid appointment for Doctor D001 at 2026-11-10 09:00
    std::string apptId1;
    std::string err1;
    bool ok1 = system.bookAppointment("P001", "D001", "2026-11-10", "09:00", apptId1, err1);
    check("Case 1.1: Initial booking for D001 on 2026-11-10 at 09:00 succeeds", ok1);
    check("Case 1.1: Sequential appointment ID A0001 returned", apptId1 == "A0001");
    check("Case 1.1: Persistent CSV contains exactly 1 appointment row", countDataRows(testApptCsv) == 1);

    const std::string csvSnapshot1 = readFileContent(testApptCsv);
    const int rowsSnapshot1 = countDataRows(testApptCsv);

    // 1.2 Attempt second booking with SAME doctor, SAME date, SAME time
    std::string apptId2;
    std::string err2;
    bool ok2 = system.bookAppointment("P002", "D001", "2026-11-10", "09:00", apptId2, err2);
    check("Case 1.2: Second booking for same doctor, date, and time is rejected", !ok2);
    check("Case 1.2: Exact required error message returned",
          err2 == "Doctor not available at the requested date and time");
    check("Case 1.2: No new appointment ID returned (outId remains empty)", apptId2.empty());
    check("Case 1.2: Persistent CSV contents remain unchanged", readFileContent(testApptCsv) == csvSnapshot1);
    check("Case 1.2: Persistent CSV row count remains unchanged (1 row)", countDataRows(testApptCsv) == rowsSnapshot1);

    // 1.3 Verify checkDoctorAvailability reports unavailable with exact message
    std::string availMsg;
    bool avail = system.checkDoctorAvailability("D001", "2026-11-10", "09:00", availMsg);
    check("Case 1.3: checkDoctorAvailability returns false for booked slot", !avail);
    check("Case 1.3: checkDoctorAvailability message is exact error message",
          availMsg == "Doctor not available at the requested date and time");

    // 1.4 Verify OpResult overload behaves identically
    std::string opApptId;
    hms::OpResult opRes = system.bookAppointment("P003", "D001", "2026-11-10", "09:00", opApptId);
    check("Case 1.4: OpResult bookAppointment returns success == false", !opRes.success);
    check("Case 1.4: OpResult error message matches exact requirement",
          opRes.message == "Doctor not available at the requested date and time");
    check("Case 1.4: OpResult appointment ID remains empty", opApptId.empty());
    check("Case 1.4: Persistent CSV content unchanged after OpResult attempt",
          readFileContent(testApptCsv) == csvSnapshot1);

    // 1.5 Case-insensitive Doctor ID conflict check ("d001" vs "D001")
    std::string lowerApptId;
    std::string lowerErr;
    bool lowerOk = system.bookAppointment("P002", "d001", "2026-11-10", "09:00", lowerApptId, lowerErr);
    check("Case 1.5: Lowercase doctor ID 'd001' detects conflict against 'D001'", !lowerOk);
    check("Case 1.5: Case-insensitive check returns exact required error message",
          lowerErr == "Doctor not available at the requested date and time");
    check("Case 1.5: No new appointment ID returned", lowerApptId.empty());
    check("Case 1.5: CSV content unchanged after case-insensitive rejection",
          readFileContent(testApptCsv) == csvSnapshot1);
  }

  // =========================================================================
  // Case 2: Same doctor, different time -> ACCEPTED
  // =========================================================================
  std::cout << "\n-- Case 2: Same doctor, different time (ACCEPTED) --\n";
  {
    // D001 already booked on 2026-11-10 at 09:00 (A0001)
    // 2.1 Verify checkDoctorAvailability returns true for different time on same date
    std::string availMsg;
    bool avail = system.checkDoctorAvailability("D001", "2026-11-10", "10:30", availMsg);
    check("Case 2.1: checkDoctorAvailability returns true for different time 10:30", avail);
    check("Case 2.1: checkDoctorAvailability reports 'Available'", availMsg == "Available");

    // 2.2 Book appointment for same doctor D001 at different time (10:30)
    std::string apptId2;
    std::string err2;
    bool ok2 = system.bookAppointment("P002", "D001", "2026-11-10", "10:30", apptId2, err2);
    check("Case 2.2: Booking same doctor D001 at different time (10:30) succeeds", ok2);
    check("Case 2.2: Unique sequential ID A0002 returned", apptId2 == "A0002");
    check("Case 2.2: Persistent CSV now contains 2 data rows", countDataRows(testApptCsv) == 2);

    // 2.3 Book appointment for same doctor D001 at different time in afternoon (14:00)
    std::string apptId3;
    std::string err3;
    bool ok3 = system.bookAppointment("P003", "D001", "2026-11-10", "14:00", apptId3, err3);
    check("Case 2.3: Booking same doctor D001 at afternoon time (14:00) succeeds", ok3);
    check("Case 2.3: Unique sequential ID A0003 returned", apptId3 == "A0003");
    check("Case 2.3: Persistent CSV now contains 3 data rows", countDataRows(testApptCsv) == 3);

    // 2.4 Same doctor, different date (2026-11-11 at 09:00)
    std::string apptIdDate;
    std::string errDate;
    bool okDate = system.bookAppointment("P001", "D001", "2026-11-11", "09:00", apptIdDate, errDate);
    check("Case 2.4: Booking same doctor D001 on different date (2026-11-11 09:00) succeeds", okDate);
    check("Case 2.4: Unique sequential ID A0004 returned", apptIdDate == "A0004");
    check("Case 2.4: Persistent CSV now contains 4 data rows", countDataRows(testApptCsv) == 4);

    // 2.5 Verify all 4 appointments are persisted and distinct
    const auto loaded = fm.loadAppointments();
    check("Case 2.5: FileManager loaded 4 appointments", loaded.size() == 4);
    if (loaded.size() == 4) {
      check("Case 2.5: A0001 is D001 2026-11-10 09:00 Booked",
            loaded[0].getId() == "A0001" && loaded[0].getDoctorId() == "D001" &&
            loaded[0].getDate() == "2026-11-10" && loaded[0].getTime() == "09:00" &&
            loaded[0].isBooked());
      check("Case 2.5: A0002 is D001 2026-11-10 10:30 Booked",
            loaded[1].getId() == "A0002" && loaded[1].getDoctorId() == "D001" &&
            loaded[1].getDate() == "2026-11-10" && loaded[1].getTime() == "10:30" &&
            loaded[1].isBooked());
      check("Case 2.5: A0003 is D001 2026-11-10 14:00 Booked",
            loaded[2].getId() == "A0003" && loaded[2].getDoctorId() == "D001" &&
            loaded[2].getDate() == "2026-11-10" && loaded[2].getTime() == "14:00" &&
            loaded[2].isBooked());
      check("Case 2.5: A0004 is D001 2026-11-11 09:00 Booked",
            loaded[3].getId() == "A0004" && loaded[3].getDoctorId() == "D001" &&
            loaded[3].getDate() == "2026-11-11" && loaded[3].getTime() == "09:00" &&
            loaded[3].isBooked());
    }

    // 2.6 Verify newly booked slots are now blocked
    std::string rejId, rejErr;
    check("Case 2.6: D001 at 10:30 now rejected as conflict",
          !system.bookAppointment("P001", "D001", "2026-11-10", "10:30", rejId, rejErr) &&
          rejErr == "Doctor not available at the requested date and time");
    check("Case 2.6: D001 at 14:00 now rejected as conflict",
          !system.bookAppointment("P001", "D001", "2026-11-10", "14:00", rejId, rejErr) &&
          rejErr == "Doctor not available at the requested date and time");
    check("Case 2.6: D001 on 2026-11-11 at 09:00 now rejected as conflict",
          !system.bookAppointment("P002", "D001", "2026-11-11", "09:00", rejId, rejErr) &&
          rejErr == "Doctor not available at the requested date and time");
    check("Case 2.6: CSV row count still exactly 4 after rejections", countDataRows(testApptCsv) == 4);
  }

  // =========================================================================
  // Case 3: Different doctor, same date and time slot -> ACCEPTED
  // =========================================================================
  std::cout << "\n-- Case 3: Different doctor, same date and time slot (ACCEPTED) --\n";
  {
    // Doctor D001 already has appointment A0001 at 2026-11-10 09:00
    // 3.1 Check availability for Doctor D002 on 2026-11-10 at 09:00
    std::string availMsg;
    bool avail = system.checkDoctorAvailability("D002", "2026-11-10", "09:00", availMsg);
    check("Case 3.1: checkDoctorAvailability for D002 on 2026-11-10 at 09:00 returns true", avail);
    check("Case 3.1: checkDoctorAvailability reports 'Available'", availMsg == "Available");

    // 3.2 Book appointment for Doctor D002 on 2026-11-10 at 09:00 for Patient P002
    std::string apptId5;
    std::string err5;
    bool ok5 = system.bookAppointment("P002", "D002", "2026-11-10", "09:00", apptId5, err5);
    check("Case 3.2: Booking different doctor D002 at same slot (2026-11-10 09:00) succeeds", ok5);
    check("Case 3.2: Unique sequential ID A0005 returned", apptId5 == "A0005");
    check("Case 3.2: Persistent CSV now contains 5 data rows", countDataRows(testApptCsv) == 5);

    // 3.3 Verify both D001 and D002 have simultaneous appointments at 2026-11-10 09:00
    const auto loaded = fm.loadAppointments();
    check("Case 3.3: Total loaded appointments is 5", loaded.size() == 5);

    const hms::Appointment *a1 = system.findAppointment("A0001");
    const hms::Appointment *a5 = system.findAppointment("A0005");
    check("Case 3.3: A0001 found in storage", a1 != nullptr);
    check("Case 3.3: A0005 found in storage", a5 != nullptr);

    if (a1 && a5) {
      check("Case 3.3: A0001 is for Doctor D001", a1->getDoctorId() == "D001");
      check("Case 3.3: A0005 is for Doctor D002", a5->getDoctorId() == "D002");
      check("Case 3.3: Both share date 2026-11-10", a1->getDate() == "2026-11-10" && a5->getDate() == "2026-11-10");
      check("Case 3.3: Both share time 09:00", a1->getTime() == "09:00" && a5->getTime() == "09:00");
      check("Case 3.3: Both have status Booked", a1->isBooked() && a5->isBooked());
      check("Case 3.3: Unique IDs confirmed (A0001 != A0005)", a1->getId() != a5->getId());
    }

    // 3.4 Now verify D002 at 2026-11-10 09:00 is also blocked for a duplicate attempt
    std::string rejId, rejErr;
    bool rejDoc2 = system.bookAppointment("P003", "D002", "2026-11-10", "09:00", rejId, rejErr);
    check("Case 3.4: Duplicate booking for D002 at 2026-11-10 09:00 is rejected", !rejDoc2);
    check("Case 3.4: Exact error message returned for D002 conflict",
          rejErr == "Doctor not available at the requested date and time");
    check("Case 3.4: CSV row count remains exactly 5 after rejection", countDataRows(testApptCsv) == 5);
  }

  // =========================================================================
  // Case 4: Same slot after cancellation -> ACCEPTED
  // =========================================================================
  std::cout << "\n-- Case 4: Same slot after cancellation (ACCEPTED) --\n";
  {
    const int rowsBeforeCancel = countDataRows(testApptCsv);
    check("Case 4.0: Pre-cancellation CSV data row count is 5", rowsBeforeCancel == 5);

    // 4.1 Cancel appointment A0001 (Doctor D001 on 2026-11-10 at 09:00)
    hms::OpResult cancelRes = system.cancelAppointment("A0001", true);
    check("Case 4.1: Cancellation of A0001 succeeds", cancelRes.success);
    check("Case 4.1: Cancellation message is exact",
          cancelRes.message == "Appointment cancelled successfully");

    // 4.2 Verify in-memory status of A0001 is Cancelled
    const hms::Appointment *cancelledAppt = system.findAppointment("A0001");
    check("Case 4.2: A0001 found in storage after cancellation", cancelledAppt != nullptr);
    if (cancelledAppt) {
      check("Case 4.2: A0001 status is Cancelled", cancelledAppt->isCancelled());
      check("Case 4.2: A0001 getStatusString() returns 'Cancelled'",
            cancelledAppt->getStatusString() == "Cancelled");
    }

    // 4.3 Verify persistent storage retains A0001 with status Cancelled
    const std::string csvAfterCancel = readFileContent(testApptCsv);
    check("Case 4.3: CSV retains A0001 record with Cancelled status (audit trail preserved)",
          csvAfterCancel.find("A0001") != std::string::npos &&
          csvAfterCancel.find("Cancelled") != std::string::npos);
    check("Case 4.3: Persistent CSV row count unchanged after cancellation (5 rows)",
          countDataRows(testApptCsv) == rowsBeforeCancel);

    // 4.4 Doctor availability check for the cancelled slot should now return Available
    std::string availMsg;
    bool isNowAvailable = system.checkDoctorAvailability("D001", "2026-11-10", "09:00", availMsg);
    check("Case 4.4: checkDoctorAvailability reports slot Available after cancellation",
          isNowAvailable && availMsg == "Available");

    // 4.5 Re-book the same doctor D001, date 2026-11-10, time 09:00 for Patient P003
    std::string rebookId;
    std::string rebookErr;
    bool rebookOk = system.bookAppointment("P003", "D001", "2026-11-10", "09:00", rebookId, rebookErr);
    check("Case 4.5: Re-booking cancelled slot for D001 2026-11-10 09:00 succeeds", rebookOk);
    check("Case 4.5: New unique sequential ID A0006 generated", rebookId == "A0006");
    check("Case 4.5: Persistent CSV now contains 6 data rows", countDataRows(testApptCsv) == 6);

    // 4.6 Verify both A0001 (Cancelled) and A0006 (Booked) exist simultaneously
    const auto loaded = fm.loadAppointments();
    check("Case 4.6: Total loaded appointments is 6", loaded.size() == 6);

    const hms::Appointment *oldAppt = nullptr;
    const hms::Appointment *newAppt = nullptr;
    for (const auto &a : loaded) {
      if (a.getId() == "A0001") oldAppt = &a;
      if (a.getId() == "A0006") newAppt = &a;
    }

    check("Case 4.6: Old appointment A0001 present in loaded list", oldAppt != nullptr);
    check("Case 4.6: New appointment A0006 present in loaded list", newAppt != nullptr);
    if (oldAppt && newAppt) {
      check("Case 4.6: Old appointment A0001 has status Cancelled", oldAppt->isCancelled());
      check("Case 4.6: New appointment A0006 has status Booked", newAppt->isBooked());
      check("Case 4.6: Both share same doctor D001",
            oldAppt->getDoctorId() == "D001" && newAppt->getDoctorId() == "D001");
      check("Case 4.6: Both share same date 2026-11-10",
            oldAppt->getDate() == "2026-11-10" && newAppt->getDate() == "2026-11-10");
      check("Case 4.6: Both share same time 09:00",
            oldAppt->getTime() == "09:00" && newAppt->getTime() == "09:00");
    }

    // 4.7 Attempting another booking for the re-booked slot MUST again fail
    const std::string csvBeforeThird = readFileContent(testApptCsv);
    std::string thirdId, thirdErr;
    bool thirdOk = system.bookAppointment("P001", "D001", "2026-11-10", "09:00", thirdId, thirdErr);
    check("Case 4.7: Third booking attempt on active rebooked slot is rejected", !thirdOk);
    check("Case 4.7: Exact error message returned on third attempt",
          thirdErr == "Doctor not available at the requested date and time");
    check("Case 4.7: Third attempt returns empty appointment ID", thirdId.empty());
    check("Case 4.7: CSV contents completely unmodified after rejected third booking",
          readFileContent(testApptCsv) == csvBeforeThird);
    check("Case 4.7: CSV data row count still exactly 6", countDataRows(testApptCsv) == 6);
  }

  // =========================================================================
  // Section 5: Validation Precedence & Regression Checks
  // =========================================================================
  std::cout << "\n-- Section 5: Validation Precedence & Non-Conflict Error Handling --\n";
  {
    const std::string snapshotBefore = readFileContent(testApptCsv);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string outId, err;

    // 5.1 Invalid date rejected before conflict check
    bool badDate = system.bookAppointment("P001", "D001", "2026-02-30", "09:00", outId, err);
    check("Section 5.1: Invalid calendar day 2026-02-30 rejected before conflict check", !badDate);
    check("Section 5.1: Date error returned, not conflict message",
          err.find("date") != std::string::npos || err.find("Date") != std::string::npos);

    // 5.2 Malformed date format rejected before conflict check
    bool malformedDate = system.bookAppointment("P001", "D001", "10-11-2026", "09:00", outId, err);
    check("Section 5.2: Malformed date format rejected before conflict check", !malformedDate);
    check("Section 5.2: Date format error returned",
          err.find("date") != std::string::npos || err.find("Date") != std::string::npos);

    // 5.3 Invalid time format rejected before conflict check
    bool badTime = system.bookAppointment("P001", "D001", "2026-11-10", "25:00", outId, err);
    check("Section 5.3: Invalid time 25:00 rejected before conflict check", !badTime);
    check("Section 5.3: Time error returned, not conflict message",
          err.find("time") != std::string::npos || err.find("Time") != std::string::npos);

    // 5.4 Unknown patient rejected before conflict check
    bool badPat = system.bookAppointment("P999", "D001", "2026-11-10", "09:00", outId, err);
    check("Section 5.4: Unknown patient P999 rejected before conflict check", !badPat);
    check("Section 5.4: Patient error message is exactly 'Patient not found'",
          err == "Patient not found");

    // 5.5 Unknown doctor rejected before conflict check
    bool badDoc = system.bookAppointment("P001", "D999", "2026-11-10", "09:00", outId, err);
    check("Section 5.5: Unknown doctor D999 rejected before conflict check", !badDoc);
    check("Section 5.5: Doctor error message is exactly 'Doctor not found'",
          err == "Doctor not found");

    // 5.6 Case-insensitive conflict check on active slot (mixed-case doctor ID "d001" on slot A0002)
    // Slot D001 2026-11-10 10:30 is currently active (A0002)
    bool caseConflict = system.bookAppointment("P001", "d001", "2026-11-10", "10:30", outId, err);
    check("Section 5.6: Lowercase doctor ID 'd001' correctly triggers conflict check on slot A0002", !caseConflict);
    check("Section 5.6: Exact conflict error returned for lowercase doctor ID",
          err == "Doctor not available at the requested date and time");

    // Verify CSV was untouched through all validation checks
    check("Section 5.7: Persistent CSV unmodified across all validation checks",
          readFileContent(testApptCsv) == snapshotBefore);
    check("Section 5.7: Persistent CSV row count unchanged (6 rows)",
          countDataRows(testApptCsv) == rowsBefore);
  }

  // =========================================================================
  // Section 6: Persistence Durability Across Application / System Restart
  // =========================================================================
  std::cout << "\n-- Section 6: System Durability & Reload Across Restart --\n";
  {
    // Instantiate brand-new FileManager and HospitalSystem instances
    hms::FileManager reloadFm(testPatientCsv.string(), testDoctorCsv.string(), testApptCsv.string());
    hms::HospitalSystem reloadSys(reloadFm);

    // Verify all 6 appointments are loaded correctly
    const auto reloaded = reloadFm.loadAppointments();
    check("Section 6.1: Reloaded system loads exactly 6 appointments", reloaded.size() == 6);

    int bookedCount = 0;
    int cancelledCount = 0;
    for (const auto &a : reloaded) {
      if (a.isBooked()) ++bookedCount;
      if (a.isCancelled()) ++cancelledCount;
    }
    check("Section 6.2: Exactly 5 Booked appointments reloaded", bookedCount == 5);
    check("Section 6.3: Exactly 1 Cancelled appointment reloaded (A0001)", cancelledCount == 1);

    const std::string snapshotBeforeRestartRej = readFileContent(testApptCsv);

    // Active slots still reject conflicts after restart
    std::string rejId, rejErr;
    // D001 at 2026-11-10 09:00 (active via A0006)
    bool rej1 = reloadSys.bookAppointment("P001", "D001", "2026-11-10", "09:00", rejId, rejErr);
    check("Section 6.4: Conflict for D001 2026-11-10 09:00 correctly rejected after restart", !rej1);
    check("Section 6.4: Exact conflict message preserved across restart",
          rejErr == "Doctor not available at the requested date and time");

    // D001 at 2026-11-10 10:30 (active via A0002)
    bool rej2 = reloadSys.bookAppointment("P001", "D001", "2026-11-10", "10:30", rejId, rejErr);
    check("Section 6.5: Conflict for D001 2026-11-10 10:30 correctly rejected after restart", !rej2);
    check("Section 6.5: Exact conflict message preserved across restart",
          rejErr == "Doctor not available at the requested date and time");

    // D002 at 2026-11-10 09:00 (active via A0005)
    bool rej3 = reloadSys.bookAppointment("P001", "D002", "2026-11-10", "09:00", rejId, rejErr);
    check("Section 6.6: Conflict for D002 2026-11-10 09:00 correctly rejected after restart", !rej3);
    check("Section 6.6: Exact conflict message preserved across restart",
          rejErr == "Doctor not available at the requested date and time");

    check("Section 6.7: CSV content unmodified after all restart rejections",
          readFileContent(testApptCsv) == snapshotBeforeRestartRej);

    // Free slot succeeds on reloaded system
    std::string newId, newErr;
    bool okFree = reloadSys.bookAppointment("P001", "D001", "2026-11-10", "15:00", newId, newErr);
    check("Section 6.8: Booking free slot (D001 2026-11-10 15:00) on reloaded system succeeds", okFree);
    check("Section 6.8: Next sequential appointment ID A0007 generated", newId == "A0007");
    check("Section 6.8: Persistent CSV now contains 7 data rows", countDataRows(testApptCsv) == 7);
  }

  // =========================================================================
  // Section 7: Root-Level CSV Preservation
  // =========================================================================
  std::cout << "\n-- Section 7: Root-Level CSV Preservation Check --\n";
  {
    check("Section 7.1: Root patients.csv preserved untouched",
          readFileContent("patients.csv") == rootPatientCsvBefore);
    check("Section 7.2: Root doctors.csv preserved untouched",
          readFileContent("doctors.csv") == rootDoctorCsvBefore);
    check("Section 7.3: Root appointments.csv preserved untouched",
          readFileContent("appointments.csv") == rootApptCsvBefore);
  }

  // Teardown isolated test fixtures
  fs::remove_all(testDir, ec);

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n====================================================================\n";
  std::cout << "TC-NFR-08 Conflict Correctness Results: " << g_passed << " passed, "
            << g_failed << " failed.\n";
  std::cout << "====================================================================\n";

  return g_failed == 0 ? 0 : 1;
}
