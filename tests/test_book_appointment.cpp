// TC-APT-01  Book Appointment (HMS-F-015)
//
// Verifies:
//   - Appointment entity validation (VR-01, VR-10, VR-11, VR-12, VR-13, VR-14)
//   - Successful appointment booking and persistence in appointments.csv
//   - System-generated unique sequential Appointment ID (A0001, A0002, ...)
//   - Correct initial status "Booked"
//   - Validation of Patient ID (rejection of unknown patient with "Patient not found", HMS-F-016, TC-APT-02)
//   - Validation of Doctor ID (rejection of unknown doctor with "Doctor not found", HMS-F-017, TC-APT-03)
//   - Validation of date and time (rejection of invalid date/time formats and non-calendar days, HMS-F-018, TC-APT-04)
//   - Double-booking conflict rejection (BR-05, HMS-F-019, TC-APT-05)
//   - Slot re-booking after cancellation (BR-06, HMS-F-021)
//   - Preservation of existing records during incremental bookings
//   - Application restart retention and ID continuity
//   - Appointment listing and chronological sorting (HMS-F-022)
//   - Safe error handling on file write failure (HMS-F-031)
//   - Interactive HospitalSystem console booking workflow
//
// Self-contained test binary adhering to project CTest harness conventions.

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

// ---------------------------------------------------------------------------
// File helpers for isolated test CSVs
// ---------------------------------------------------------------------------

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
  std::cout << "=== Running TC-APT-01 / HMS-F-015 Book Appointment Tests ===\n\n";

  // =========================================================================
  // Section 1: Entity & Field Validation Rules (VR-01, VR-10, VR-11, VR-12, VR-13, VR-14)
  // =========================================================================
  std::cout << "-- 1. Appointment Entity Field Validation Rules --\n";

  // Baseline valid appointment
  {
    hms::Appointment a("A0001", "P001", "DOC1001", "2026-10-15", "10:00",
                       hms::AppointmentStatus::Booked);
    std::string err;
    check("Baseline appointment is valid", a.validate(err));
    check("Error string empty on valid", err.empty());
    check("Getter ID", a.getId() == "A0001");
    check("Getter PatientID", a.getPatientId() == "P001");
    check("Getter DoctorID", a.getDoctorId() == "DOC1001");
    check("Getter Date", a.getDate() == "2026-10-15");
    check("Getter Time", a.getTime() == "10:00");
    check("Getter Status", a.getStatus() == hms::AppointmentStatus::Booked);
    check("Getter StatusString", a.getStatusString() == "Booked");
    check("isBooked returns true", a.isBooked());
    check("isCancelled returns false", !a.isCancelled());
  }

  // VR-12: Appointment ID rules
  {
    std::string err;
    check("VR-12: Valid A0001", hms::Appointment::validateId("A0001", err));
    check("VR-12: Valid A9999", hms::Appointment::validateId("A9999", err));
    check("VR-12: Valid lowercase a0001", hms::Appointment::validateId("a0001", err));
    check("VR-12: Too short (A001) rejected", !hms::Appointment::validateId("A001", err));
    check("VR-12: Missing prefix A (10001) rejected", !hms::Appointment::validateId("10001", err));
    check("VR-12: Non-digits in suffix (A00A1) rejected", !hms::Appointment::validateId("A00A1", err));
    check("VR-14: Leading = in ID rejected", !hms::Appointment::validateId("=A0001", err));
    check("VR-14: Comma in ID rejected", !hms::Appointment::validateId("A0,01", err));
    check("VR-14: Newline in ID rejected", !hms::Appointment::validateId("A0\n01", err));
  }

  // VR-10: Appointment Date rules
  {
    std::string err;
    check("VR-10: Valid date 2026-10-15", hms::Appointment::validateDate("2026-10-15", err));
    check("VR-10: Leap year 2024-02-29 valid", hms::Appointment::validateDate("2024-02-29", err));
    check("VR-10: Century leap year 2000-02-29 valid", hms::Appointment::validateDate("2000-02-29", err));
    check("VR-10: Non-leap year 2026-02-29 rejected", !hms::Appointment::validateDate("2026-02-29", err));
    check("VR-10: Non-existent calendar day 2026-02-30 rejected", !hms::Appointment::validateDate("2026-02-30", err));
    check("VR-10: Month 13 (2026-13-01) rejected", !hms::Appointment::validateDate("2026-13-01", err));
    check("VR-10: Month 00 (2026-00-10) rejected", !hms::Appointment::validateDate("2026-00-10", err));
    check("VR-10: Day 00 (2026-10-00) rejected", !hms::Appointment::validateDate("2026-10-00", err));
    check("VR-10: Day 32 (2026-10-32) rejected", !hms::Appointment::validateDate("2026-10-32", err));
    check("VR-10: Malformed date (15-10-2026) rejected", !hms::Appointment::validateDate("15-10-2026", err));
    check("VR-10: Empty date rejected", !hms::Appointment::validateDate("", err));
    check("VR-14: Leading = in date rejected", !hms::Appointment::validateDate("=2026-10-15", err));
  }

  // VR-11: Appointment Time rules
  {
    std::string err;
    check("VR-11: Valid time 10:00", hms::Appointment::validateTime("10:00", err));
    check("VR-11: Valid boundary 00:00", hms::Appointment::validateTime("00:00", err));
    check("VR-11: Valid boundary 23:59", hms::Appointment::validateTime("23:59", err));
    check("VR-11: 24:00 rejected", !hms::Appointment::validateTime("24:00", err));
    check("VR-11: 24:15 rejected", !hms::Appointment::validateTime("24:15", err));
    check("VR-11: Single digit hour 9:00 rejected", !hms::Appointment::validateTime("9:00", err));
    check("VR-11: Minute out of range 12:60 rejected", !hms::Appointment::validateTime("12:60", err));
    check("VR-11: Empty time rejected", !hms::Appointment::validateTime("", err));
    check("VR-14: Leading formula prefix in time rejected", !hms::Appointment::validateTime("+10:00", err));
  }

  // VR-13: Status & Conflict logic
  {
    hms::AppointmentStatus st = hms::AppointmentStatus::Booked;
    std::string err;
    check("VR-13: Status Booked parsed", hms::Appointment::validateStatus("Booked", st, err) && st == hms::AppointmentStatus::Booked);
    check("VR-13: Status Cancelled parsed", hms::Appointment::validateStatus("Cancelled", st, err) && st == hms::AppointmentStatus::Cancelled);
    check("VR-13: Invalid status rejected", !hms::Appointment::validateStatus("Pending", st, err));

    hms::Appointment appt("A0001", "P001", "D001", "2026-10-15", "10:00", hms::AppointmentStatus::Booked);
    check("conflictsWith returns true for same doctor, date, time",
          appt.conflictsWith("D001", "2026-10-15", "10:00"));
    check("conflictsWith is case-insensitive for doctor ID (d001)",
          appt.conflictsWith("d001", "2026-10-15", "10:00"));
    check("conflictsWith returns false for different doctor",
          !appt.conflictsWith("D002", "2026-10-15", "10:00"));
    check("conflictsWith returns false for different date",
          !appt.conflictsWith("D001", "2026-10-16", "10:00"));
    check("conflictsWith returns false for different time",
          !appt.conflictsWith("D001", "2026-10-15", "11:00"));

    appt.cancel();
    check("isCancelled returns true after cancel()", appt.isCancelled());
    check("Cancelled appointment does NOT conflict (BR-06 / HMS-F-021)",
          !appt.conflictsWith("D001", "2026-10-15", "10:00"));
  }

  // =========================================================================
  // Section 2: Isolated Persistence & FileManager Integration
  // =========================================================================
  std::cout << "\n-- 2. Persistence & FileManager Integration --\n";

  const std::string testPatientCsv = "build/test_apt_patients.csv";
  const std::string testDoctorCsv = "build/test_apt_doctors.csv";
  const std::string testApptCsv = "build/test_apt_appointments.csv";

  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // Missing file handling (BR-09 / HMS-F-026)
  {
    check("Missing appointments CSV loads as empty vector", fm.loadAppointments().empty());
    check("appointmentIdExists on missing file returns false", !fm.appointmentIdExists("A0001"));
  }

  // Save first valid appointment
  {
    std::string err;
    hms::Appointment appt1("A0001", "P001", "D001", "2026-10-15", "10:00", hms::AppointmentStatus::Booked);
    bool saved = fm.saveAppointment(appt1, err);
    check("First appointment saved successfully", saved);
    check("Error empty on success", err.empty());
    check("appointments.csv now has exactly 1 data row", countDataRows(testApptCsv) == 1);
    check("appointmentIdExists returns true for A0001", fm.appointmentIdExists("A0001"));
    check("appointmentIdExists is case-insensitive (a0001)", fm.appointmentIdExists("a0001"));

    auto loaded = fm.loadAppointments();
    check("loadAppointments count is 1", loaded.size() == 1);
    if (!loaded.empty()) {
      check("Loaded ID matches", loaded[0].getId() == "A0001");
      check("Loaded PatientID matches", loaded[0].getPatientId() == "P001");
      check("Loaded DoctorID matches", loaded[0].getDoctorId() == "D001");
      check("Loaded Date matches", loaded[0].getDate() == "2026-10-15");
      check("Loaded Time matches", loaded[0].getTime() == "10:00");
      check("Loaded Status is Booked", loaded[0].isBooked());
    }
  }

  // Save second appointment — verifies preservation of existing records
  {
    std::string err;
    hms::Appointment appt2("A0002", "P002", "D002", "2026-10-16", "14:00", hms::AppointmentStatus::Booked);
    bool saved = fm.saveAppointment(appt2, err);
    check("Second appointment saved successfully", saved);
    check("appointments.csv now has exactly 2 data rows", countDataRows(testApptCsv) == 2);
    auto loaded = fm.loadAppointments();
    check("loadAppointments count is 2", loaded.size() == 2);
    if (loaded.size() >= 2) {
      check("First appointment preserved", loaded[0].getId() == "A0001");
      check("Second appointment present", loaded[1].getId() == "A0002");
    }
  }

  // Duplicate Appointment ID rejection
  {
    const std::string before = readFile(testApptCsv);
    std::string err;
    hms::Appointment dupAppt("A0001", "P003", "D003", "2026-10-20", "11:00", hms::AppointmentStatus::Booked);
    bool saved = fm.saveAppointment(dupAppt, err);
    check("Duplicate Appointment ID rejected by saveAppointment", !saved);
    check("Error message indicates already exists", err.find("already exists") != std::string::npos);
    check("CSV file unchanged after duplicate rejection", readFile(testApptCsv) == before);
    check("Row count still 2", countDataRows(testApptCsv) == 2);
  }

  // Write failure handling (HMS-F-031)
  {
    hms::FileManager badFm(testPatientCsv, testDoctorCsv, "Z:/non_existent_folder_xyz/appointments.csv");
    std::string err;
    hms::Appointment appt("A0099", "P001", "D001", "2026-10-15", "10:00");
    bool saved = badFm.saveAppointment(appt, err);
    check("Save to unwritable path fails gracefully", !saved);
    check("Unwritable path error populated", !err.empty());
  }

  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  // =========================================================================
  // Section 3: HospitalSystem Business Logic & Acceptance Criteria (TC-APT-01)
  // =========================================================================
  std::cout << "\n-- 3. HospitalSystem Booking Workflows (TC-APT-01 to TC-APT-05) --\n";

  // Seed patient and doctor for testing
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;
    sysFm.savePatient(hms::Patient("P001", "John Doe", 45, "M", "9876543210", "123 Main St", "None"), err);
    sysFm.savePatient(hms::Patient("P002", "Jane Smith", 30, "F", "9123456789", "456 Oak St", "Asthma"), err);
    sysFm.saveDoctor(hms::Doctor("D001", "Dr. Alice", "Cardiology", "Cardiology Dept", "9876543210", "Mon-Fri 09:00-17:00"), err);
    sysFm.saveDoctor(hms::Doctor("D002", "Dr. Bob", "Neurology", "Neurology Dept", "9123456789", "Tue-Sat 10:00-18:00"), err);
  }

  // Test Case Specification 3: TC-APT-01 Baseline Valid Booking
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    std::string apptId;
    std::string error;
    bool ok = sys.bookAppointment("P001", "D001", "2026-10-15", "10:00", apptId, error);
    check("TC-APT-01: Valid booking succeeds", ok);
    check("TC-APT-01: System generates ID A0001", apptId == "A0001");
    check("TC-APT-01: appointments.csv contains 1 data row", countDataRows(testApptCsv) == 1);

    auto list = sys.listAppointments("ALL");
    check("TC-APT-01: Appears in appointment list", list.size() == 1);
    if (!list.empty()) {
      check("TC-APT-01: Status is Booked", list[0].isBooked());
      check("TC-APT-01: Patient matches P001", list[0].getPatientId() == "P001");
      check("TC-APT-01: Doctor matches D001", list[0].getDoctorId() == "D001");
      check("TC-APT-01: Date matches 2026-10-15", list[0].getDate() == "2026-10-15");
      check("TC-APT-01: Time matches 10:00", list[0].getTime() == "10:00");
    }
  }

  // Unknown Patient Rejection (HMS-F-016, TC-APT-02)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const std::string contentBefore = readFile(testApptCsv);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string apptId;
    std::string error;
    bool ok = sys.bookAppointment("P999", "D001", "2026-10-16", "11:00", apptId, error);
    check("TC-APT-02: Unknown patient rejected", !ok);
    check("TC-APT-02: Error message is 'Patient not found'", error == "Patient not found");
    check("TC-APT-02: CSV row count unchanged", countDataRows(testApptCsv) == rowsBefore);
    check("TC-APT-02: CSV file contents unchanged", readFile(testApptCsv) == contentBefore);
    check("TC-APT-02: No appointment ID generated", apptId.empty());
  }

  // Unknown Doctor Rejection (HMS-F-017, TC-APT-03)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const std::string contentBefore = readFile(testApptCsv);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string apptId;
    std::string error;
    bool ok = sys.bookAppointment("P001", "D999", "2026-10-16", "11:00", apptId, error);
    check("TC-APT-03: Unknown doctor rejected", !ok);
    check("TC-APT-03: Error message is 'Doctor not found'", error == "Doctor not found");
    check("TC-APT-03: CSV row count unchanged", countDataRows(testApptCsv) == rowsBefore);
    check("TC-APT-03: CSV file contents unchanged", readFile(testApptCsv) == contentBefore);
    check("TC-APT-03: No appointment ID generated", apptId.empty());
  }

  // Invalid Date / Time Rejection (HMS-F-018, TC-APT-04)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string apptId;
    std::string error;

    // Case 1: Non-existent calendar day 2026-02-30
    bool ok1 = sys.bookAppointment("P001", "D001", "2026-02-30", "10:00", apptId, error);
    check("TC-APT-04: Non-existent calendar day 2026-02-30 rejected", !ok1);
    check("TC-APT-04: Date error mentions date", error.find("date") != std::string::npos || error.find("Date") != std::string::npos);

    // Case 2: Invalid 24-hour time 24:15
    bool ok2 = sys.bookAppointment("P001", "D001", "2026-10-15", "24:15", apptId, error);
    check("TC-APT-04: Invalid 24-hour time 24:15 rejected", !ok2);
    check("TC-APT-04: Time error mentions time", error.find("time") != std::string::npos || error.find("Time") != std::string::npos);

    // Case 3: Empty date
    bool ok3 = sys.bookAppointment("P001", "D001", "", "10:00", apptId, error);
    check("TC-APT-04: Empty date rejected", !ok3);

    // Case 4: Formula injection in time
    bool ok4 = sys.bookAppointment("P001", "D001", "2026-10-15", "=cmd'calc'", apptId, error);
    check("TC-APT-04: Formula injection rejected", !ok4);

    check("TC-APT-04: CSV row count untouched across all invalid attempts", countDataRows(testApptCsv) == rowsBefore);
  }

  // Sequential ID Generation & Multi-booking
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    std::string apptId2;
    std::string error;
    bool ok = sys.bookAppointment("P002", "D002", "2026-10-16", "14:00", apptId2, error);
    check("Second booking succeeds", ok);
    check("Second appointment ID is sequential A0002", apptId2 == "A0002");
    check("CSV now contains 2 data rows (A0001, A0002)", countDataRows(testApptCsv) == 2);
  }

  // Doctor Double-Booking Conflict Rejection (BR-05, HMS-F-019, TC-APT-05)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const int rowsBefore = countDataRows(testApptCsv);

    // Doctor D001 is already booked on 2026-10-15 at 10:00 (A0001)
    std::string conflictId;
    std::string error;
    bool ok = sys.bookAppointment("P002", "D001", "2026-10-15", "10:00", conflictId, error);
    check("TC-APT-05: Double-booking same doctor at same date and time rejected", !ok);
    check("TC-APT-05: Conflict error matches specification",
          error == "Doctor not available at the requested date and time");
    check("TC-APT-05: CSV row count unchanged after conflict rejection", countDataRows(testApptCsv) == rowsBefore);
  }

  // Cancellation and Slot Re-booking (BR-06, HMS-F-021)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    // Cancel appointment A0001
    hms::OpResult cancelRes = sys.cancelAppointment("A0001", true);
    check("Cancellation of A0001 succeeds", cancelRes.success);

    const hms::Appointment *cancelled = sys.findAppointment("A0001");
    check("A0001 found after cancellation", cancelled != nullptr);
    if (cancelled) {
      check("A0001 status changed to Cancelled", cancelled->isCancelled());
    }

    // Now re-book Doctor D001 at 2026-10-15 10:00 for Patient P002
    std::string apptId3;
    std::string error;
    bool rebookOk = sys.bookAppointment("P002", "D001", "2026-10-15", "10:00", apptId3, error);
    check("Re-booking previously cancelled slot succeeds", rebookOk);
    check("Re-booked appointment gets new sequential ID A0003 (never reuses A0001)", apptId3 == "A0003");

    // Cancelled record A0001 remains in appointments.csv alongside A0002 and A0003 (3 total)
    check("appointments.csv retains cancelled appointment (3 data rows total)", countDataRows(testApptCsv) == 3);
  }

  // Application Restart Retention and Sequential ID Continuity (HMS-F-030)
  {
    // Re-instantiate HospitalSystem with same CSV files
    hms::FileManager restartFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem restartedSys(restartFm);

    auto list = restartedSys.listAppointments("ALL");
    check("Restart: 3 appointments loaded from disk", list.size() == 3);

    const hms::Appointment *a1 = restartedSys.findAppointment("A0001");
    check("Restart: A0001 preserved with status Cancelled", a1 && a1->isCancelled());

    const hms::Appointment *a2 = restartedSys.findAppointment("A0002");
    check("Restart: A0002 preserved with status Booked", a2 && a2->isBooked());

    const hms::Appointment *a3 = restartedSys.findAppointment("A0003");
    check("Restart: A0003 preserved with status Booked", a3 && a3->isBooked());

    // Next booking should automatically get A0004
    std::string apptId4;
    std::string error;
    bool ok = restartedSys.bookAppointment("P001", "D002", "2026-10-18", "09:00", apptId4, error);
    check("Restart: New booking gets sequential ID A0004", ok && apptId4 == "A0004");
  }

  // Appointment Listing and Filtering (HMS-F-022)
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    auto allList = sys.listAppointments("ALL");
    check("Listing: ALL returns 4 appointments", allList.size() == 4);

    // Verify chronological order:
    // A0001: 2026-10-15 10:00
    // A0003: 2026-10-15 10:00
    // A0002: 2026-10-16 14:00
    // A0004: 2026-10-18 09:00
    bool chronoSorted = true;
    for (std::size_t i = 1; i < allList.size(); ++i) {
      if (allList[i].getDate() < allList[i - 1].getDate() ||
          (allList[i].getDate() == allList[i - 1].getDate() &&
           allList[i].getTime() < allList[i - 1].getTime())) {
        chronoSorted = false;
        break;
      }
    }
    check("Listing: appointments are sorted chronologically by date and time", chronoSorted);

    auto pat1List = sys.listAppointments("PATIENT", "P001");
    check("Listing: filter by PATIENT P001 returns 2 records", pat1List.size() == 2);

    auto doc2List = sys.listAppointments("DOCTOR", "D002");
    check("Listing: filter by DOCTOR D002 returns 2 records", doc2List.size() == 2);
  }

  // =========================================================================
  // Section 4: End-to-End Console Interactive Workflow
  // =========================================================================
  std::cout << "\n-- 4. HospitalSystem Interactive Workflow --\n";

  // Positive interactive booking
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    // Provide simulated input: Patient ID, Doctor ID, Date, Time
    std::string inputData =
        "P001\n"
        "D001\n"
        "2026-10-25\n"
        "15:30\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.bookAppointment();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays Appointment booked successfully",
          output.find("Appointment booked successfully.") != std::string::npos);
    check("Console displays Appointment ID: A0005",
          output.find("Appointment ID: A0005") != std::string::npos);
  }

  // Interactive booking with unknown patient rejection
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const std::string contentBefore = readFile(testApptCsv);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string inputData =
        "P888\n"
        "D001\n"
        "2026-10-25\n"
        "16:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.bookAppointment();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays 'Patient not found'",
          output.find("Patient not found") != std::string::npos);
    check("Console unknown patient: CSV row count unchanged", countDataRows(testApptCsv) == rowsBefore);
    check("Console unknown patient: CSV file contents unchanged", readFile(testApptCsv) == contentBefore);
  }

  // Interactive booking with unknown doctor rejection
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);
    const std::string contentBefore = readFile(testApptCsv);
    const int rowsBefore = countDataRows(testApptCsv);

    std::string inputData =
        "P001\n"
        "D888\n"
        "2026-10-25\n"
        "16:00\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.bookAppointment();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays 'Doctor not found'",
          output.find("Doctor not found") != std::string::npos);
    check("Console unknown doctor: CSV row count unchanged", countDataRows(testApptCsv) == rowsBefore);
    check("Console unknown doctor: CSV file contents unchanged", readFile(testApptCsv) == contentBefore);
  }

  // Interactive booking with conflict rejection
  {
    hms::FileManager sysFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem sys(sysFm);

    // Conflict with A0005 (D001 on 2026-10-25 at 15:30)
    std::string inputData =
        "P002\n"
        "D001\n"
        "2026-10-25\n"
        "15:30\n";

    std::istringstream inStream(inputData);
    std::ostringstream outStream;

    auto *cinBuf = std::cin.rdbuf(inStream.rdbuf());
    auto *coutBuf = std::cout.rdbuf(outStream.rdbuf());

    sys.bookAppointment();

    std::cin.rdbuf(cinBuf);
    std::cout.rdbuf(coutBuf);

    std::string output = outStream.str();
    check("Console displays conflict error message",
          output.find("Doctor not available at the requested date and time") != std::string::npos);
  }

  // Cleanup isolated test CSVs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  // =========================================================================
  // Summary
  // =========================================================================
  std::cout << "\n=======================================================\n";
  std::cout << "Tests passed: " << g_passed << ", Tests failed: " << g_failed << '\n';
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
