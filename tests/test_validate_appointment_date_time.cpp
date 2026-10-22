// TC-APT-04 Validate Appointment Date and Time (HMS-F-018)
//
// Verifies:
//   - Rejection of invalid calendar day: 2026-02-30 (AC-1)
//   - Rejection of invalid month: 2026-13-01 (AC-2)
//   - Rejection of invalid hour: 24:00 (AC-3)
//   - Rejection of incorrect time format: 9:5 (AC-4)
//   - Acceptance of valid date: 2026-10-05 (AC-5)
//   - Acceptance of valid time: 09:30 (AC-6)
//   - Entity-level validation (Appointment::validate, Appointment::isValid, Appointment::fromCsv)
//   - Isolated booking-API persistence verification (isolated test CSV files, root-level CSV untouched)
//
// Self-contained regression test adhering to project C++17 conventions.

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
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

// Helpers for isolated test CSVs
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

// Simulated booking API endpoint matching HMS-F-018 validation order
// Ensures end-to-end validation without touching root-level user CSVs
bool bookAppointmentWorkflow(const hms::FileManager &fm,
                             const std::string &patientId,
                             const std::string &doctorId,
                             const std::string &date,
                             const std::string &time,
                             std::string &outApptId,
                             std::string &errorMessage) {
  outApptId.clear();
  errorMessage.clear();

  // Step 1: Validate Date format and calendar validity (VR-10, HMS-F-018)
  if (!hms::Appointment::validateDate(date, errorMessage)) {
    return false;
  }

  // Step 2: Validate Time format (VR-11, HMS-F-018)
  if (!hms::Appointment::validateTime(time, errorMessage)) {
    return false;
  }

  // Step 3: Verify patient exists (BR-03, HMS-F-016)
  if (!fm.patientIdExists(patientId)) {
    errorMessage = "Patient not found";
    return false;
  }

  // Step 4: Verify doctor exists (BR-04, HMS-F-017)
  if (!fm.doctorIdExists(doctorId)) {
    errorMessage = "Doctor not found";
    return false;
  }

  // Step 5: Check conflict (BR-05, HMS-F-019)
  for (const auto &appt : fm.loadAppointments()) {
    if (appt.conflictsWith(doctorId, date, time)) {
      errorMessage = "Doctor not available at the requested date and time";
      return false;
    }
  }

  // Step 6: Generate ID & persist
  const std::string apptId = "A0001";
  hms::Appointment newAppt(apptId, patientId, doctorId, date, time,
                           hms::AppointmentStatus::Booked);

  if (!fm.saveAppointment(newAppt, errorMessage)) {
    return false;
  }

  outApptId = apptId;
  return true;
}

} // namespace

int main() {
  std::cout << "=== Running TC-APT-04 / HMS-F-018 Validate Appointment Date & Time Tests ===\n\n";

  // =========================================================================
  // Section 1: Mandatory Issue #14 Acceptance Criteria
  // =========================================================================
  std::cout << "-- 1. Mandatory Acceptance Criteria (Issue #14 / TC-APT-04) --\n";

  // AC-1: 2026-02-30 -> Rejected — invalid calendar day
  {
    std::string err;
    bool valid = hms::Appointment::validateDate("2026-02-30", err);
    check("AC-1: 2026-02-30 rejected — invalid calendar day", !valid);
    check("AC-1: Error message populated for invalid calendar day", !err.empty());
  }

  // AC-2: 2026-13-01 -> Rejected — invalid month
  {
    std::string err;
    bool valid = hms::Appointment::validateDate("2026-13-01", err);
    check("AC-2: 2026-13-01 rejected — invalid month", !valid);
    check("AC-2: Error message populated for invalid month", !err.empty());
  }

  // AC-3: 24:00 -> Rejected — invalid hour
  {
    std::string err;
    bool valid = hms::Appointment::validateTime("24:00", err);
    check("AC-3: 24:00 rejected — invalid hour", !valid);
    check("AC-3: Error message populated for invalid hour", !err.empty());
  }

  // AC-4: 9:5 -> Rejected — incorrect time format
  {
    std::string err;
    bool valid = hms::Appointment::validateTime("9:5", err);
    check("AC-4: 9:5 rejected — incorrect time format", !valid);
    check("AC-4: Error message populated for incorrect time format", !err.empty());
  }

  // AC-5: 2026-10-05 -> Accepted as a valid date
  {
    std::string err;
    bool valid = hms::Appointment::validateDate("2026-10-05", err);
    check("AC-5: 2026-10-05 accepted as a valid date", valid);
    check("AC-5: Error message empty on valid date", err.empty());
  }

  // AC-6: 09:30 -> Accepted as a valid time
  {
    std::string err;
    bool valid = hms::Appointment::validateTime("09:30", err);
    check("AC-6: 09:30 accepted as a valid time", valid);
    check("AC-6: Error message empty on valid time", err.empty());
  }

  // =========================================================================
  // Section 2: Comprehensive Date/Time Edge Cases & Boundary Conditions
  // =========================================================================
  std::cout << "\n-- 2. Extended Boundary & Leap-Year Rules (VR-10, VR-11, VR-14) --\n";

  // Leap-year rule verification
  {
    std::string err;
    check("VR-10: 2024-02-29 accepted (standard leap year)",
          hms::Appointment::validateDate("2024-02-29", err));
    check("VR-10: 2000-02-29 accepted (century leap year divisible by 400)",
          hms::Appointment::validateDate("2000-02-29", err));
    check("VR-10: 2100-02-29 rejected (century non-leap year)",
          !hms::Appointment::validateDate("2100-02-29", err));
    check("VR-10: 2026-02-29 rejected (non-leap year February 29)",
          !hms::Appointment::validateDate("2026-02-29", err));
  }

  // Month and day boundaries
  {
    std::string err;
    check("VR-10: 2026-00-15 rejected (month 00)",
          !hms::Appointment::validateDate("2026-00-15", err));
    check("VR-10: 2026-01-00 rejected (day 00)",
          !hms::Appointment::validateDate("2026-01-00", err));
    check("VR-10: 2026-01-32 rejected (day 32)",
          !hms::Appointment::validateDate("2026-01-32", err));
    check("VR-10: 2026-04-31 rejected (April has 30 days)",
          !hms::Appointment::validateDate("2026-04-31", err));
    check("VR-10: 2026-06-31 rejected (June has 30 days)",
          !hms::Appointment::validateDate("2026-06-31", err));
    check("VR-10: 2026-09-31 rejected (September has 30 days)",
          !hms::Appointment::validateDate("2026-09-31", err));
    check("VR-10: 2026-11-31 rejected (November has 30 days)",
          !hms::Appointment::validateDate("2026-11-31", err));
    check("VR-10: 2026-01-01 accepted (month start boundary)",
          hms::Appointment::validateDate("2026-01-01", err));
    check("VR-10: 2026-12-31 accepted (year end boundary)",
          hms::Appointment::validateDate("2026-12-31", err));
    check("VR-10: Malformed format '15-10-2026' rejected",
          !hms::Appointment::validateDate("15-10-2026", err));
    check("VR-10: Empty date rejected",
          !hms::Appointment::validateDate("", err));
  }

  // Time boundaries
  {
    std::string err;
    check("VR-11: 00:00 accepted (midnight start boundary)",
          hms::Appointment::validateTime("00:00", err));
    check("VR-11: 23:59 accepted (day end boundary)",
          hms::Appointment::validateTime("23:59", err));
    check("VR-11: 12:00 accepted (midday)",
          hms::Appointment::validateTime("12:00", err));
    check("VR-11: 24:01 rejected (hour out of range)",
          !hms::Appointment::validateTime("24:01", err));
    check("VR-11: 12:60 rejected (minute 60)",
          !hms::Appointment::validateTime("12:60", err));
    check("VR-11: 09:3 rejected (incomplete minute)",
          !hms::Appointment::validateTime("09:3", err));
    check("VR-11: Empty time rejected",
          !hms::Appointment::validateTime("", err));
  }

  // Security checks (VR-14)
  {
    std::string err;
    check("VR-14: Formula injection in date rejected",
          !hms::Appointment::validateDate("=2026-10-05", err));
    check("VR-14: Formula injection in time rejected",
          !hms::Appointment::validateTime("+09:30", err));
    check("VR-14: Comma in date rejected",
          !hms::Appointment::validateDate("2026,10,05", err));
    check("VR-14: Comma in time rejected",
          !hms::Appointment::validateTime("09,30", err));
  }

  // =========================================================================
  // Section 3: Entity Validation & CSV Parsing Integration
  // =========================================================================
  std::cout << "\n-- 3. Appointment Entity & CSV Parsing Integration --\n";

  {
    std::string err;

    // Entity with invalid calendar day 2026-02-30
    hms::Appointment apptBadDate1("A0001", "P001", "DOC1001", "2026-02-30", "09:30");
    check("Entity validation rejects 2026-02-30", !apptBadDate1.validate(err));
    check("isValid() returns false for 2026-02-30", !apptBadDate1.isValid());

    // Entity with invalid month 2026-13-01
    hms::Appointment apptBadDate2("A0001", "P001", "DOC1001", "2026-13-01", "09:30");
    check("Entity validation rejects 2026-13-01", !apptBadDate2.validate(err));
    check("isValid() returns false for 2026-13-01", !apptBadDate2.isValid());

    // Entity with invalid hour 24:00
    hms::Appointment apptBadTime1("A0001", "P001", "DOC1001", "2026-10-05", "24:00");
    check("Entity validation rejects 24:00", !apptBadTime1.validate(err));
    check("isValid() returns false for 24:00", !apptBadTime1.isValid());

    // Entity with incorrect time format 9:5
    hms::Appointment apptBadTime2("A0001", "P001", "DOC1001", "2026-10-05", "9:5");
    check("Entity validation rejects 9:5", !apptBadTime2.validate(err));
    check("isValid() returns false for 9:5", !apptBadTime2.isValid());

    // Entity with valid date 2026-10-05 and valid time 09:30
    hms::Appointment apptValid("A0001", "P001", "DOC1001", "2026-10-05", "09:30");
    check("Entity validation accepts 2026-10-05 and 09:30", apptValid.validate(err));
    check("isValid() returns true for valid date and time", apptValid.isValid());

    // CSV line parsing with invalid date 2026-02-30
    hms::Appointment parsedAppt;
    check("fromCsv rejects line with 2026-02-30",
          !hms::Appointment::fromCsv("A0001,P001,DOC1001,2026-02-30,09:30,Booked", parsedAppt, err));

    // CSV line parsing with invalid time 24:00
    check("fromCsv rejects line with 24:00",
          !hms::Appointment::fromCsv("A0001,P001,DOC1001,2026-10-05,24:00,Booked", parsedAppt, err));

    // CSV line parsing with valid date and time
    check("fromCsv accepts line with 2026-10-05 and 09:30",
          hms::Appointment::fromCsv("A0001,P001,DOC1001,2026-10-05,09:30,Booked", parsedAppt, err));
  }

  // =========================================================================
  // Section 4: Isolated CSV Persistence & Booking-API Integration
  // =========================================================================
  std::cout << "\n-- 4. Isolated CSV Persistence & Booking Verification --\n";

  const std::string testPatientCsv = "build/test_datetime_patients.csv";
  const std::string testDoctorCsv = "build/test_datetime_doctors.csv";
  const std::string testApptCsv = "build/test_datetime_appointments.csv";

  removeFile(testPatientCsv);
  removeFile(testDoctorCsv);
  removeFile(testApptCsv);

  {
    hms::FileManager fm(testPatientCsv, testDoctorCsv, testApptCsv);
    std::string err;

    // Seed test patient and doctor into isolated CSV files
    fm.savePatient(hms::Patient("P001", "John Doe", 45, "M", "9876543210", "123 Main St", "None"), err);
    fm.saveDoctor(hms::Doctor("DOC1001", "Dr. Smith", "Cardiology", "Cardiology Dept", "9876543210", "Mon-Fri 09:00-17:00"), err);

    check("Isolated test setup: initial appointment count is 0",
          countDataRows(testApptCsv) == 0);

    // Attempt booking with AC-1: 2026-02-30
    {
      std::string apptId;
      std::string error;
      bool ok = bookAppointmentWorkflow(fm, "P001", "DOC1001", "2026-02-30", "09:30", apptId, error);
      check("Booking with 2026-02-30 returns false", !ok);
      check("Booking with 2026-02-30 returns empty appointment ID", apptId.empty());
      check("Booking with 2026-02-30 leaves appointment count at 0", countDataRows(testApptCsv) == 0);
    }

    // Attempt booking with AC-2: 2026-13-01
    {
      std::string apptId;
      std::string error;
      bool ok = bookAppointmentWorkflow(fm, "P001", "DOC1001", "2026-13-01", "09:30", apptId, error);
      check("Booking with 2026-13-01 returns false", !ok);
      check("Booking with 2026-13-01 returns empty appointment ID", apptId.empty());
      check("Booking with 2026-13-01 leaves appointment count at 0", countDataRows(testApptCsv) == 0);
    }

    // Attempt booking with AC-3: 24:00
    {
      std::string apptId;
      std::string error;
      bool ok = bookAppointmentWorkflow(fm, "P001", "DOC1001", "2026-10-05", "24:00", apptId, error);
      check("Booking with 24:00 returns false", !ok);
      check("Booking with 24:00 returns empty appointment ID", apptId.empty());
      check("Booking with 24:00 leaves appointment count at 0", countDataRows(testApptCsv) == 0);
    }

    // Attempt booking with AC-4: 9:5
    {
      std::string apptId;
      std::string error;
      bool ok = bookAppointmentWorkflow(fm, "P001", "DOC1001", "2026-10-05", "9:5", apptId, error);
      check("Booking with 9:5 returns false", !ok);
      check("Booking with 9:5 returns empty appointment ID", apptId.empty());
      check("Booking with 9:5 leaves appointment count at 0", countDataRows(testApptCsv) == 0);
    }

    // Booking with AC-5 & AC-6: valid date 2026-10-05 and valid time 09:30
    {
      std::string apptId;
      std::string error;
      bool ok = bookAppointmentWorkflow(fm, "P001", "DOC1001", "2026-10-05", "09:30", apptId, error);
      check("Booking with valid 2026-10-05 and 09:30 succeeds", ok);
      check("Booking returns generated appointment ID A0001", apptId == "A0001");
      check("Booking increments appointment count to 1", countDataRows(testApptCsv) == 1);

      auto appts = fm.loadAppointments();
      check("Loaded appointment count is 1", appts.size() == 1);
      if (!appts.empty()) {
        check("Persisted appointment date is 2026-10-05", appts[0].getDate() == "2026-10-05");
        check("Persisted appointment time is 09:30", appts[0].getTime() == "09:30");
        check("Persisted appointment status is Booked", appts[0].isBooked());
      }
    }
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
