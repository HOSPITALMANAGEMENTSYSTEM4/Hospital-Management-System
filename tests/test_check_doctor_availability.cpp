// TC-APT-06: Check Doctor Availability (HMS-F-020, UC-10, BR-05)
//
// Acceptance Requirements:
//   1. Valid, unoccupied doctor/date/time slot returns "Available".
//   2. Valid, occupied doctor/date/time slot returns "Not Available" / "Doctor not available at the requested date and time".
//   3. Unknown doctors, invalid dates, and invalid times are rejected with appropriate error messages.
//   4. Cancelled appointments are excluded from conflict checking and do not block the slot.
//   5. Different doctors, dates, and times are treated as independent slots.
//   6. The availability check is strictly read-only:
//      - appointments.csv remains byte-for-byte identical.
//      - Appointment counts, IDs, and statuses remain completely unchanged.
//      - No appointment record is created, appended, or modified.
//   7. Interactive menu workflow clearly reports "Available" or "Not available",
//      handles empty input gracefully, and returns control safely to the menu.
//
// Test Environment:
//   - Uses isolated test CSV files in build/, never root-level user data files.
//   - Verifies programmatic API (checkDoctorAvailability) and interactive console workflow.

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
  std::cout << "=== Running TC-APT-06 Check Doctor Availability Tests ===\n\n";

  const std::string testPatientCsv = "build/test_avail_patients.csv";
  const std::string testDoctorCsv = "build/test_avail_doctors.csv";
  const std::string testApptCsv = "build/test_avail_appointments.csv";

  // Clean up any stale files from previous runs
  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);

  // -------------------------------------------------------------------------
  // Setup / Arrange: Seed registered patients, registered doctors, and appointments
  // -------------------------------------------------------------------------
  std::cout << "-- Setup: Seeding patients, doctors, and baseline appointments --\n";

  std::string seedErr;
  bool pat1Saved = fm.savePatient(
      hms::Patient("P001", "Alice Smith", 30, "F", "9876543210", "10 Main St", "None"),
      seedErr);
  check("Seed registered patient P001", pat1Saved);

  bool pat2Saved = fm.savePatient(
      hms::Patient("P002", "Bob Jones", 45, "M", "9123456789", "20 Oak St", "Hypertension"),
      seedErr);
  check("Seed registered patient P002", pat2Saved);

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

  // Seed baseline appointment records:
  // 1. Active appointment A0001: D001, 2026-11-10 at 10:00 (Booked)
  // 2. Cancelled appointment A0002: D001, 2026-11-10 at 14:00 (Cancelled)
  const hms::Appointment appt1("A0001", "P001", "D001", "2026-11-10", "10:00",
                               hms::AppointmentStatus::Booked);
  bool appt1Saved = fm.saveAppointment(appt1, seedErr);
  check("Seed active appointment A0001 (D001, 2026-11-10 10:00 Booked)", appt1Saved);

  const hms::Appointment appt2("A0002", "P002", "D001", "2026-11-10", "14:00",
                               hms::AppointmentStatus::Cancelled);
  bool appt2Saved = fm.saveAppointment(appt2, seedErr);
  check("Seed cancelled appointment A0002 (D001, 2026-11-10 14:00 Cancelled)", appt2Saved);

  hms::HospitalSystem system(fm);

  // Capture baseline state
  const std::string baselineApptContent = readFile(testApptCsv);
  const int baselineApptCount = countDataRows(testApptCsv);
  const std::string baselinePatContent = readFile(testPatientCsv);
  const std::string baselineDocContent = readFile(testDoctorCsv);

  check("Guard: appointments.csv exists and is non-empty", !baselineApptContent.empty());
  check("Guard: appointments.csv contains exactly 2 baseline records", baselineApptCount == 2);
  check("Guard: doctor D001 exists in repository", fm.doctorIdExists("D001"));
  check("Guard: doctor D002 exists in repository", fm.doctorIdExists("D002"));

  // =========================================================================
  // Section A: Available Slot (Unoccupied)
  // =========================================================================
  std::cout << "\n-- A. Available Slot: Unoccupied Date and Time --\n";
  {
    std::string msg;
    bool available = system.checkDoctorAvailability("D001", "2026-11-10", "11:00", msg);

    check("Unoccupied slot (D001, 2026-11-10 11:00) returns true", available);
    check("Exact message for available slot is 'Available'", msg == "Available");
    check("appointments.csv byte-for-byte unchanged after checking available slot",
          readFile(testApptCsv) == baselineApptContent);
    check("appointments row count unchanged after checking available slot",
          countDataRows(testApptCsv) == baselineApptCount);
  }

  // =========================================================================
  // Section B: Occupied Slot (Active Non-Cancelled Appointment)
  // =========================================================================
  std::cout << "\n-- B. Occupied Slot: Active Appointment Conflicts --\n";
  {
    std::string msg;
    bool available = system.checkDoctorAvailability("D001", "2026-11-10", "10:00", msg);

    check("Occupied slot (D001, 2026-11-10 10:00) returns false", !available);
    check("Exact message for occupied slot is 'Doctor not available at the requested date and time'",
          msg == "Doctor not available at the requested date and time");
    check("appointments.csv byte-for-byte unchanged after checking occupied slot",
          readFile(testApptCsv) == baselineApptContent);
    check("appointments row count unchanged after checking occupied slot",
          countDataRows(testApptCsv) == baselineApptCount);

    // Case-insensitivity check for doctor ID
    std::string lowerMsg;
    bool lowerAvailable = system.checkDoctorAvailability("d001", "2026-11-10", "10:00", lowerMsg);
    check("Occupied slot with lowercase doctor ID 'd001' returns false", !lowerAvailable);
    check("Exact message for lowercase 'd001' matches specification",
          lowerMsg == "Doctor not available at the requested date and time");
    check("appointments.csv byte-for-byte unchanged after lowercase check",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section C: Cancelled Appointment Excluded From Conflict Checking
  // =========================================================================
  std::cout << "\n-- C. Cancelled Slot: Cancelled Appointment Does Not Block Slot --\n";
  {
    std::string msg;
    // Slot 2026-11-10 14:00 belongs to cancelled appointment A0002
    bool available = system.checkDoctorAvailability("D001", "2026-11-10", "14:00", msg);

    check("Cancelled slot (D001, 2026-11-10 14:00) returns true (available)", available);
    check("Exact message for cancelled slot is 'Available'", msg == "Available");
    check("appointments.csv byte-for-byte unchanged after cancelled slot check",
          readFile(testApptCsv) == baselineApptContent);
    check("appointments row count unchanged after cancelled slot check",
          countDataRows(testApptCsv) == baselineApptCount);

    // Verify cancelled appointment record still has Cancelled status
    const auto appointments = fm.loadAppointments();
    check("Loaded appointments count is still 2", appointments.size() == 2);
    if (appointments.size() == 2) {
      check("A0002 retains Cancelled status in persistence", appointments[1].isCancelled());
      check("A0001 retains Booked status in persistence", appointments[0].isBooked());
    }
  }

  // =========================================================================
  // Section D: Independent Slots (Different Doctors, Dates, Times)
  // =========================================================================
  std::cout << "\n-- D. Independent Slots: Different Doctors, Dates, and Times --\n";
  {
    // 1. Different doctor at the exact same date and time as active appointment A0001
    std::string msgDoc2;
    bool availDoc2 = system.checkDoctorAvailability("D002", "2026-11-10", "10:00", msgDoc2);
    check("Different doctor D002 at same date and time (10:00) is Available", availDoc2);
    check("Message for D002 at same slot is 'Available'", msgDoc2 == "Available");

    // 2. Same doctor at a different time on the same date
    std::string msgDiffTime;
    bool availDiffTime = system.checkDoctorAvailability("D001", "2026-11-10", "09:30", msgDiffTime);
    check("Same doctor D001 at different time (09:30) is Available", availDiffTime);
    check("Message for different time is 'Available'", msgDiffTime == "Available");

    // 3. Same doctor on a different date at the same time
    std::string msgDiffDate;
    bool availDiffDate = system.checkDoctorAvailability("D001", "2026-11-11", "10:00", msgDiffDate);
    check("Same doctor D001 on different date (2026-11-11) is Available", availDiffDate);
    check("Message for different date is 'Available'", msgDiffDate == "Available");

    check("appointments.csv byte-for-byte unchanged after independent slot checks",
          readFile(testApptCsv) == baselineApptContent);
  }

  // =========================================================================
  // Section E: Invalid Input Handling
  // =========================================================================
  std::cout << "\n-- E. Invalid Input: Unknown Doctor, Bad Date, Bad Time --\n";
  {
    std::string err;

    // 1. Unknown doctor ID
    bool badDoc = system.checkDoctorAvailability("DOC999", "2026-11-10", "10:00", err);
    check("Unknown doctor DOC999 returns false", !badDoc);
    check("Error message for unknown doctor is 'Doctor not found'", err == "Doctor not found");

    // 2. Non-existent calendar day (2026-02-30)
    bool badDay = system.checkDoctorAvailability("D001", "2026-02-30", "10:00", err);
    check("Non-existent calendar day 2026-02-30 returns false", !badDay);
    check("Error message indicates date validation failure",
          err.find("date") != std::string::npos || err.find("Date") != std::string::npos);

    // 3. Invalid calendar month (2026-13-01)
    bool badMonth = system.checkDoctorAvailability("D001", "2026-13-01", "10:00", err);
    check("Invalid calendar month 2026-13-01 returns false", !badMonth);
    check("Error message indicates month validation failure",
          err.find("month") != std::string::npos || err.find("date") != std::string::npos);

    // 4. Invalid 24-hour time (25:99)
    bool badTime = system.checkDoctorAvailability("D001", "2026-11-10", "25:99", err);
    check("Invalid time 25:99 returns false", !badTime);
    check("Error message indicates time validation failure",
          err.find("time") != std::string::npos || err.find("Time") != std::string::npos);

    // 5. Invalid time format (9:5)
    bool badTimeFormat = system.checkDoctorAvailability("D001", "2026-11-10", "9:5", err);
    check("Incorrect time format 9:5 returns false", !badTimeFormat);
    check("Error message indicates time format failure",
          err.find("format") != std::string::npos || err.find("time") != std::string::npos);

    // 6. Formula injection attempt in date
    bool injectionDate = system.checkDoctorAvailability("D001", "=2026-11-10", "10:00", err);
    check("Formula injection in date returns false", !injectionDate);

    // 7. Formula injection attempt in time
    bool injectionTime = system.checkDoctorAvailability("D001", "2026-11-10", "@10:00", err);
    check("Formula injection in time returns false", !injectionTime);

    check("appointments.csv byte-for-byte unchanged after invalid input checks",
          readFile(testApptCsv) == baselineApptContent);
    check("patients.csv byte-for-byte untouched",
          readFile(testPatientCsv) == baselinePatContent);
    check("doctors.csv byte-for-byte untouched",
          readFile(testDoctorCsv) == baselineDocContent);
  }

  // =========================================================================
  // Section F: Read-Only Data Integrity Across Invocations
  // =========================================================================
  std::cout << "\n-- F. Read-Only Data Integrity Verification --\n";
  {
    // Perform multiple varied queries
    for (int i = 0; i < 5; ++i) {
      std::string m;
      system.checkDoctorAvailability("D001", "2026-11-10", "10:00", m);
      system.checkDoctorAvailability("D001", "2026-11-10", "11:00", m);
      system.checkDoctorAvailability("D002", "2026-11-10", "10:00", m);
      system.checkDoctorAvailability("DOC999", "2026-11-10", "10:00", m);
    }

    check("appointments.csv byte-for-byte unchanged after batch queries",
          readFile(testApptCsv) == baselineApptContent);
    check("appointments row count remains exactly baseline count (2)",
          countDataRows(testApptCsv) == baselineApptCount);

    const auto loaded = fm.loadAppointments();
    check("Total loaded appointment count is exactly 2", loaded.size() == 2);
    if (loaded.size() == 2) {
      check("A0001 ID preserved", loaded[0].getId() == "A0001");
      check("A0001 Doctor preserved", loaded[0].getDoctorId() == "D001");
      check("A0001 Date preserved", loaded[0].getDate() == "2026-11-10");
      check("A0001 Time preserved", loaded[0].getTime() == "10:00");
      check("A0001 Status preserved as Booked", loaded[0].isBooked());

      check("A0002 ID preserved", loaded[1].getId() == "A0002");
      check("A0002 Doctor preserved", loaded[1].getDoctorId() == "D001");
      check("A0002 Date preserved", loaded[1].getDate() == "2026-11-10");
      check("A0002 Time preserved", loaded[1].getTime() == "14:00");
      check("A0002 Status preserved as Cancelled", loaded[1].isCancelled());
    }
  }

  // =========================================================================
  // Section G: Interactive Console Menu Workflow
  // =========================================================================
  std::cout << "\n-- G. Interactive Console Menu Workflow (HMS-F-020 UI) --\n";
  {
    // 1. Interactive check for Available slot (Option 3 -> Option 4 -> valid details -> 0 -> 4)
    {
      std::istringstream input("3\n4\nD001\n2026-11-10\n11:00\n0\n4\n");
      std::ostringstream output;

      auto *origCin = std::cin.rdbuf(input.rdbuf());
      auto *origCout = std::cout.rdbuf(output.rdbuf());

      system.run();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      const std::string out = output.str();
      check("Interactive menu reports 'Available: Available'",
            out.find("Available: Available") != std::string::npos);
    }

    // 2. Interactive check for Occupied slot
    {
      std::istringstream input("3\n4\nD001\n2026-11-10\n10:00\n0\n4\n");
      std::ostringstream output;

      auto *origCin = std::cin.rdbuf(input.rdbuf());
      auto *origCout = std::cout.rdbuf(output.rdbuf());

      system.run();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      const std::string out = output.str();
      check("Interactive menu reports 'Not available: Doctor not available at the requested date and time'",
            out.find("Not available: Doctor not available at the requested date and time") != std::string::npos);
    }

    // 3. Interactive check for Cancelled slot
    {
      std::istringstream input("3\n4\nD001\n2026-11-10\n14:00\n0\n4\n");
      std::ostringstream output;

      auto *origCin = std::cin.rdbuf(input.rdbuf());
      auto *origCout = std::cout.rdbuf(output.rdbuf());

      system.run();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      const std::string out = output.str();
      check("Interactive menu reports cancelled slot as 'Available: Available'",
            out.find("Available: Available") != std::string::npos);
    }

    // 4. Interactive check for Unknown doctor
    {
      std::istringstream input("3\n4\nDOC999\n2026-11-10\n10:00\n0\n4\n");
      std::ostringstream output;

      auto *origCin = std::cin.rdbuf(input.rdbuf());
      auto *origCout = std::cout.rdbuf(output.rdbuf());

      system.run();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      const std::string out = output.str();
      check("Interactive menu reports 'Not available: Doctor not found'",
            out.find("Not available: Doctor not found") != std::string::npos);
    }

    // 5. Interactive check with empty required input (handles gracefully)
    {
      std::istringstream input("3\n4\n\n0\n4\n");
      std::ostringstream output;

      auto *origCin = std::cin.rdbuf(input.rdbuf());
      auto *origCout = std::cout.rdbuf(output.rdbuf());

      system.run();

      std::cin.rdbuf(origCin);
      std::cout.rdbuf(origCout);

      const std::string out = output.str();
      check("Interactive menu detects empty field with 'This field is required.'",
            out.find("This field is required.") != std::string::npos);
      check("Interactive menu returns safely to appointment management menu",
            out.find("=== Appointment Management ===") != std::string::npos);
    }

    check("appointments.csv byte-for-byte unchanged after all interactive checks",
          readFile(testApptCsv) == baselineApptContent);
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
  std::cout << "TC-APT-06 Results: " << g_passed << " passed, "
            << g_failed << " failed.\n";
  std::cout << "=======================================================\n";

  return g_failed == 0 ? 0 : 1;
}
