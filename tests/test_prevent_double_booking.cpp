// TC-APT-05 / TC-NFR-08: Prevent Double-Booking (HMS-F-019, HMS-NF-008, BR-05, BR-06)
//
// Acceptance Requirements:
//   1. A valid first appointment is saved successfully.
//   2. A second appointment with the same Doctor ID, date, and time is rejected.
//   3. The required error message is displayed exactly:
//      "Doctor not available at the requested date and time"
//   4. The appointment count remains unchanged after the rejected booking.
//   5. No rejected appointment is written to persistent storage.
//   6. A cancelled appointment does not block the slot.
//   7. Rebooking a slot after cancellation is allowed if all other validations pass.
//   8. Appointments for different doctors, dates, or times are not incorrectly rejected.
//   9. Existing patient, doctor, date, time, and persistence validation remains intact.
//  10. 100% accuracy across conflict test matrix (HMS-NF-008):
//      - Same doctor / same slot (rejected)
//      - Same doctor / different time (accepted)
//      - Same doctor / different date (accepted)
//      - Different doctor / same slot (accepted)
//      - Same doctor / same slot after cancellation (accepted)
//
// Test Environment:
//   - Uses isolated test CSV files in build/, never root-level user data files.
//   - Verifies programmatic APIs (with errorMessage and OpResult) and interactive console workflow.

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
  std::cout << "=== Running TC-APT-05 / TC-NFR-08 Prevent Double-Booking Tests ===\n\n";

  const std::string testPatientCsv = "build/test_prevent_db_patients.csv";
  const std::string testDoctorCsv = "build/test_prevent_db_doctors.csv";
  const std::string testApptCsv = "build/test_prevent_db_appointments.csv";

  // Clean up any stale files from previous runs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // -------------------------------------------------------------------------
  // Setup / Arrange: Seed registered patients and registered doctors
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding registered patients, doctors, and initial appointment --\n";

  std::string seedErr;
  bool pat1Saved = fm.savePatient(
      hms::Patient("P001", "Alice Smith", 30, "F", "9876543210", "10 Main St", "None"),
      seedErr);
  check("Seed registered patient P001", pat1Saved);

  bool pat2Saved = fm.savePatient(
      hms::Patient("P002", "Bob Jones", 45, "M", "9123456789", "20 Oak St", "Hypertension"),
      seedErr);
  check("Seed registered patient P002", pat2Saved);

  bool pat3Saved = fm.savePatient(
      hms::Patient("P003", "Carol White", 28, "F", "9811223344", "30 Pine St", "Asthma"),
      seedErr);
  check("Seed registered patient P003", pat3Saved);

  bool doc1Saved = fm.saveDoctor(
      hms::Doctor("D001", "Dr. John Doe", "Cardiology", "Cardiology Dept",
                  "9876501234", "Mon-Fri 09:00-17:00"),
      seedErr);
  check("Seed registered doctor D001", doc1Saved);

  bool doc2Saved = fm.saveDoctor(
      hms::Doctor("D002", "Dr. Jane Roe", "Neurology", "Neurology Dept",
                  "9876505678", "Mon-Fri 08:00-16:00"),
      seedErr);
  check("Seed registered doctor D002", doc2Saved);

  hms::HospitalSystem system(fm);

  // Seed baseline appointment: D001 with P001 on 2026-10-15 at 10:00
  std::string initialApptId;
  std::string initialError;
  bool initialBooked = system.bookAppointment("P001", "D001", "2026-10-15", "10:00",
                                              initialApptId, initialError);
  check("Seed initial valid appointment A0001 (D001, 2026-10-15 10:00)",
        initialBooked && initialApptId == "A0001");

  const std::string baselineApptContent = readFile(testApptCsv);
  const int baselineApptCount = countDataRows(testApptCsv);
  const std::string baselinePatContent = readFile(testPatientCsv);
  const std::string baselineDocContent = readFile(testDoctorCsv);

  check("Guard: appointments.csv exists and is non-empty", !baselineApptContent.empty());
  check("Guard: appointments.csv has exactly 1 initial record", baselineApptCount == 1);
  check("Guard: doctor D001 exists in repository", fm.doctorIdExists("D001"));
  check("Guard: doctor D002 exists in repository", fm.doctorIdExists("D002"));

  // =========================================================================
  // Section 1: Programmatic API with errorMessage (TC-APT-05 Core Requirement)
  // =========================================================================
  std::cout << "\n-- 1. Programmatic API: Reject Duplicate Doctor/Date/Time Booking --\n";
  {
    std::string outApptId = "";
    std::string errorMsg = "";

    // Attempt second booking for D001 at the same date and time (2026-10-15 10:00) with P002
    bool success = system.bookAppointment("P002", "D001", "2026-10-15", "10:00",
                                          outApptId, errorMsg);

    check("Second booking for same doctor, date, and time is rejected (returns false)", !success);
    check("Exact error message is 'Doctor not available at the requested date and time'",
          errorMsg == "Doctor not available at the requested date and time");
    check("Output appointment ID remains empty (no ID generated/returned)", outApptId.empty());
    check("Appointment row count unchanged in CSV after conflict rejection",
          countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv is byte-for-byte identical to baseline after rejection",
          readFile(testApptCsv) == baselineApptContent);
    check("patients.csv remains byte-for-byte untouched",
          readFile(testPatientCsv) == baselinePatContent);
    check("doctors.csv remains byte-for-byte untouched",
          readFile(testDoctorCsv) == baselineDocContent);

    // Verify in-memory appointment list remains untouched
    const auto appts = system.listAppointments("ALL");
    check("In-memory appointment list count matches baseline (1)",
          static_cast<int>(appts.size()) == baselineApptCount);
    if (!appts.empty()) {
      check("Only baseline appointment A0001 exists", appts[0].getId() == "A0001");
    }
  }

  // =========================================================================
  // Section 2: Case-Insensitivity & Whitespace Tolerance in Conflict Detection
  // =========================================================================
  std::cout << "\n-- 2. Conflict Detection: Doctor ID Case-Insensitivity & Formatting --\n";
  {
    // Mixed/Lowercase doctor ID "d001"
    {
      std::string outId = "";
      std::string err = "";
      bool res = system.bookAppointment("p002", "d001", "2026-10-15", "10:00", outId, err);
      check("Booking with lowercase doctor ID 'd001' detects conflict and is rejected", !res);
      check("Exact error message matches for lowercase 'd001'",
            err == "Doctor not available at the requested date and time");
      check("Output ID remains empty for lowercase 'd001'", outId.empty());
    }

    // Padded doctor ID in interactive booking (trimmed by input handler)
    {
      std::istringstream paddedInput("P002\n  D001  \n2026-10-15\n10:00\n");
      std::ostringstream paddedOutput;

      auto *origCin = std::cin.rdbuf(paddedInput.rdbuf());
      auto *origCout = std::cout.rdbuf(paddedOutput.rdbuf());

      system.bookAppointment();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      check("Interactive booking with padded doctor ID detects conflict after trim",
            paddedOutput.str().find("Doctor not available at the requested date and time") != std::string::npos);
    }

    // Same patient (P001) attempting duplicate slot with same doctor
    {
      std::string outId = "";
      std::string err = "";
      bool res = system.bookAppointment("P001", "D001", "2026-10-15", "10:00", outId, err);
      check("Same patient attempting duplicate booking for same doctor/slot is rejected", !res);
      check("Exact error message matches for same patient duplicate attempt",
            err == "Doctor not available at the requested date and time");
      check("Output ID remains empty for same patient duplicate attempt", outId.empty());
    }

    check("Appointment row count still unchanged after multiple conflict rejections",
          countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv still byte-for-byte identical after multiple conflict rejections",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 3: Programmatic API with OpResult
  // =========================================================================
  std::cout << "\n-- 3. Programmatic API: OpResult Rejection Verification --\n";
  {
    std::string outApptId = "";
    hms::OpResult result = system.bookAppointment("P002", "D001", "2026-10-15", "10:00", outApptId);

    check("OpResult success is false for conflicting booking", !result.success);
    check("OpResult message is exactly 'Doctor not available at the requested date and time'",
          result.message == "Doctor not available at the requested date and time");
    check("OpResult output appointment ID remains empty", outApptId.empty());
    check("Appointment count remains unchanged after OpResult rejection",
          countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv remains byte-for-byte identical after OpResult rejection",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 4: Interactive Console Booking Workflow
  // =========================================================================
  std::cout << "\n-- 4. Interactive Console Booking Workflow: Conflict Handling --\n";
  {
    // Inputs: Patient ID (P002), Doctor ID (D001), Date (2026-10-15), Time (10:00)
    std::istringstream input("P002\nD001\n2026-10-15\n10:00\n");
    std::ostringstream output;

    auto *origCin = std::cin.rdbuf(input.rdbuf());
    auto *origCout = std::cout.rdbuf(output.rdbuf());

    system.bookAppointment();

    std::cin.rdbuf(origCin);
    std::cout.rdbuf(origCout);

    const std::string consoleOutput = output.str();

    check("Console displays exact 'Doctor not available at the requested date and time' message",
          consoleOutput.find("Doctor not available at the requested date and time") != std::string::npos);
    check("Console does NOT display success message",
          consoleOutput.find("Appointment booked successfully") == std::string::npos);
    check("Console does NOT display generated Appointment ID",
          consoleOutput.find("Appointment ID:") == std::string::npos);
    check("Appointment count remains unchanged after interactive conflict rejection",
          countDataRows(testApptCsv) == baselineApptCount);
    check("appointments.csv remains byte-for-byte identical after interactive conflict rejection",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section 5: Doctor Availability Check API & Interactive Workflow
  // =========================================================================
  std::cout << "\n-- 5. Doctor Availability Standalone Method & Interactive Check --\n";
  {
    std::string availMsg;
    bool isAvailConflicting = system.checkDoctorAvailability("D001", "2026-10-15", "10:00", availMsg);
    check("checkDoctorAvailability returns false for conflicting slot", !isAvailConflicting);
    check("checkDoctorAvailability message is exactly 'Doctor not available at the requested date and time'",
          availMsg == "Doctor not available at the requested date and time");

    bool isAvailFree = system.checkDoctorAvailability("D001", "2026-10-15", "11:00", availMsg);
    check("checkDoctorAvailability returns true for free slot", isAvailFree);
    check("checkDoctorAvailability message is 'Available' for free slot", availMsg == "Available");

    std::string diffDocMsg;
    bool isDiffDocAvail = system.checkDoctorAvailability("D002", "2026-10-15", "10:00", diffDocMsg);
    check("checkDoctorAvailability returns true for different doctor at same date and time", isDiffDocAvail);
    check("checkDoctorAvailability message is 'Available' for different doctor", diffDocMsg == "Available");
  }

  // =========================================================================
  // Section 6: Full Conflict Matrix Verification (HMS-NF-008 / TC-NFR-08)
  // =========================================================================
  std::cout << "\n-- 6. HMS-NF-008 Conflict Detection Matrix Verification --\n";
  {
    // Matrix Case 1: Same Doctor, Same Slot -> REJECTED (Verified in Sections 1-5)

    // Matrix Case 2: Same Doctor, Different Time (D001, 2026-10-15, 11:00) -> ACCEPTED
    std::string apptIdTime;
    std::string errTime;
    bool okTime = system.bookAppointment("P002", "D001", "2026-10-15", "11:00",
                                         apptIdTime, errTime);
    check("Matrix Case 2: Same doctor, different time is accepted", okTime);
    check("Matrix Case 2: Sequential ID A0002 generated", apptIdTime == "A0002");
    check("Matrix Case 2: CSV now contains 2 data rows", countDataRows(testApptCsv) == 2);

    // Matrix Case 3: Same Doctor, Different Date (D001, 2026-10-16, 10:00) -> ACCEPTED
    std::string apptIdDate;
    std::string errDate;
    bool okDate = system.bookAppointment("P003", "D001", "2026-10-16", "10:00",
                                         apptIdDate, errDate);
    check("Matrix Case 3: Same doctor, different date is accepted", okDate);
    check("Matrix Case 3: Sequential ID A0003 generated", apptIdDate == "A0003");
    check("Matrix Case 3: CSV now contains 3 data rows", countDataRows(testApptCsv) == 3);

    // Matrix Case 4: Different Doctor, Same Date & Time (D002, 2026-10-15, 10:00) -> ACCEPTED
    std::string apptIdDoc;
    std::string errDoc;
    bool okDoc = system.bookAppointment("P001", "D002", "2026-10-15", "10:00",
                                        apptIdDoc, errDoc);
    check("Matrix Case 4: Different doctor, same date and time is accepted", okDoc);
    check("Matrix Case 4: Sequential ID A0004 generated", apptIdDoc == "A0004");
    check("Matrix Case 4: CSV now contains 4 data rows", countDataRows(testApptCsv) == 4);

    // Verify all 4 appointments are persisted with status Booked
    const auto loaded = fm.loadAppointments();
    check("FileManager loads 4 persisted appointments", loaded.size() == 4);
    if (loaded.size() == 4) {
      check("A0001: D001 2026-10-15 10:00 Booked",
            loaded[0].getId() == "A0001" && loaded[0].getDoctorId() == "D001" &&
            loaded[0].getDate() == "2026-10-15" && loaded[0].getTime() == "10:00" &&
            loaded[0].isBooked());
      check("A0002: D001 2026-10-15 11:00 Booked",
            loaded[1].getId() == "A0002" && loaded[1].getDoctorId() == "D001" &&
            loaded[1].getDate() == "2026-10-15" && loaded[1].getTime() == "11:00" &&
            loaded[1].isBooked());
      check("A0003: D001 2026-10-16 10:00 Booked",
            loaded[2].getId() == "A0003" && loaded[2].getDoctorId() == "D001" &&
            loaded[2].getDate() == "2026-10-16" && loaded[2].getTime() == "10:00" &&
            loaded[2].isBooked());
      check("A0004: D002 2026-10-15 10:00 Booked",
            loaded[3].getId() == "A0004" && loaded[3].getDoctorId() == "D002" &&
            loaded[3].getDate() == "2026-10-15" && loaded[3].getTime() == "10:00" &&
            loaded[3].isBooked());
    }

    // Now re-verify conflicts for all currently booked slots:
    // D001 at 2026-10-15 10:00 still rejected
    std::string cId, cErr;
    check("D001 at 2026-10-15 10:00 is rejected",
          !system.bookAppointment("P002", "D001", "2026-10-15", "10:00", cId, cErr) &&
          cErr == "Doctor not available at the requested date and time");
    // D001 at 2026-10-15 11:00 still rejected
    check("D001 at 2026-10-15 11:00 is rejected",
          !system.bookAppointment("P003", "D001", "2026-10-15", "11:00", cId, cErr) &&
          cErr == "Doctor not available at the requested date and time");
    // D001 at 2026-10-16 10:00 still rejected
    check("D001 at 2026-10-16 10:00 is rejected",
          !system.bookAppointment("P002", "D001", "2026-10-16", "10:00", cId, cErr) &&
          cErr == "Doctor not available at the requested date and time");
    // D002 at 2026-10-15 10:00 still rejected
    check("D002 at 2026-10-15 10:00 is rejected",
          !system.bookAppointment("P003", "D002", "2026-10-15", "10:00", cId, cErr) &&
          cErr == "Doctor not available at the requested date and time");

    check("CSV data rows still exactly 4 after all rejection attempts", countDataRows(testApptCsv) == 4);
  }

  // =========================================================================
  // Section 7: Cancellation and Slot Re-booking (HMS-F-021, BR-06, Matrix Case 5)
  // =========================================================================
  std::cout << "\n-- 7. Cancellation & Slot Re-booking (Matrix Case 5) --\n";
  {
    const int rowsBeforeCancel = countDataRows(testApptCsv);

    // Cancel appointment A0001 (Doctor D001 on 2026-10-15 at 10:00)
    hms::OpResult cancelRes = system.cancelAppointment("A0001", true);
    check("Cancellation of A0001 succeeds", cancelRes.success);
    check("Cancellation message confirmed", cancelRes.message == "Appointment cancelled successfully");

    // Verify in-memory status
    const hms::Appointment *cancelledAppt = system.findAppointment("A0001");
    check("A0001 found after cancellation", cancelledAppt != nullptr);
    if (cancelledAppt) {
      check("A0001 status is Cancelled", cancelledAppt->isCancelled());
    }

    // Verify CSV contains A0001 with Cancelled status
    const std::string csvAfterCancel = readFile(testApptCsv);
    check("CSV retains A0001 record with Cancelled status (HMS-SR-006)",
          csvAfterCancel.find("A0001") != std::string::npos &&
          csvAfterCancel.find("Cancelled") != std::string::npos);
    check("CSV data row count remains unchanged after cancellation (4 rows)",
          countDataRows(testApptCsv) == rowsBeforeCancel);

    // Doctor availability check for the cancelled slot should now return true
    std::string availMsg;
    bool isNowAvailable = system.checkDoctorAvailability("D001", "2026-10-15", "10:00", availMsg);
    check("checkDoctorAvailability reports slot Available after cancellation",
          isNowAvailable && availMsg == "Available");

    // Re-book Doctor D001 at 2026-10-15 10:00 for Patient P002
    std::string rebookId;
    std::string rebookErr;
    bool rebookOk = system.bookAppointment("P002", "D001", "2026-10-15", "10:00",
                                           rebookId, rebookErr);
    check("Matrix Case 5: Re-booking previously cancelled slot succeeds", rebookOk);
    check("Matrix Case 5: Re-booked appointment gets new sequential ID A0005", rebookId == "A0005");
    check("CSV now contains 5 data rows (4 original + 1 re-booked)", countDataRows(testApptCsv) == 5);

    // Verify both A0001 (Cancelled) and A0005 (Booked) exist simultaneously
    const std::string csvAfterRebook = readFile(testApptCsv);
    check("CSV retains both A0001 (Cancelled) and A0005 (Booked)",
          csvAfterRebook.find("A0001") != std::string::npos &&
          csvAfterRebook.find("A0005") != std::string::npos);

    // Attempting a third booking for the re-booked slot should again fail!
    std::string thirdId, thirdErr;
    bool thirdOk = system.bookAppointment("P003", "D001", "2026-10-15", "10:00",
                                          thirdId, thirdErr);
    check("Third booking attempt after re-booking is rejected", !thirdOk);
    check("Exact error message on third booking attempt",
          thirdErr == "Doctor not available at the requested date and time");
    check("CSV row count still exactly 5 after third booking rejection", countDataRows(testApptCsv) == 5);
  }

  // =========================================================================
  // Section 8: Validation Precedence & Regression
  // =========================================================================
  std::cout << "\n-- 8. Validation Precedence & System Durability Across Restart --\n";
  {
    const std::string currentCsvContent = readFile(testApptCsv);
    const int currentCsvCount = countDataRows(testApptCsv);

    std::string outId, err;

    // 1. Invalid date rejected before conflict check
    bool badDate = system.bookAppointment("P001", "D001", "2026-02-30", "10:00", outId, err);
    check("Invalid calendar day 2026-02-30 rejected before conflict check", !badDate);
    check("Date error message is returned, not conflict message",
          err.find("date") != std::string::npos || err.find("Date") != std::string::npos);

    // 2. Invalid time rejected before conflict check
    bool badTime = system.bookAppointment("P001", "D001", "2026-10-15", "24:15", outId, err);
    check("Invalid time 24:15 rejected before conflict check", !badTime);
    check("Time error message is returned, not conflict message",
          err.find("time") != std::string::npos || err.find("Time") != std::string::npos);

    // 3. Unknown patient rejected before conflict check
    bool badPat = system.bookAppointment("P999", "D001", "2026-10-15", "10:00", outId, err);
    check("Unknown patient P999 rejected before conflict check", !badPat);
    check("Patient error message is exactly 'Patient not found'", err == "Patient not found");

    // 4. Unknown doctor rejected before conflict check
    bool badDoc = system.bookAppointment("P001", "D999", "2026-10-15", "10:00", outId, err);
    check("Unknown doctor D999 rejected before conflict check", !badDoc);
    check("Doctor error message is exactly 'Doctor not found'", err == "Doctor not found");

    check("CSV content completely unmodified across validation attempts",
          readFile(testApptCsv) == currentCsvContent);
    check("CSV count completely unmodified across validation attempts",
          countDataRows(testApptCsv) == currentCsvCount);

    // 5. Durability across simulated application restart
    hms::FileManager restartFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem restartSys(restartFm);

    std::string restartId, restartErr;
    bool restartConflict = restartSys.bookAppointment("P003", "D001", "2026-10-15", "10:00",
                                                      restartId, restartErr);
    check("Double-booking conflict detected after system restart", !restartConflict);
    check("Exact error message preserved across restart",
          restartErr == "Doctor not available at the requested date and time");
    check("Restart output ID remains empty", restartId.empty());
    check("CSV content unmodified after restart rejection",
          readFile(testApptCsv) == currentCsvContent);
  }

  // -------------------------------------------------------------------------
  // Cleanup isolated test CSVs
  // -------------------------------------------------------------------------
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=======================================================\n";
  std::cout << "TC-APT-05 / TC-NFR-08 Results: " << g_passed << " passed, "
            << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
