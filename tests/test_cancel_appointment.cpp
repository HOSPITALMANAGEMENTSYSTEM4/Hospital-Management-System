// TC-APT-09: Cancel Appointment
// Traceability:
//   - SRS Requirement: HMS-F-023
//   - STP RTM Test Case: TC-APT-09
//   - SRS RTM Test Case: TC-APT-09
//   - Dependency: HMS-F-015 (Book Appointment)
//   - GitHub Issue: #32
//
// Acceptance Requirements:
//   1. Test A: Valid appointment exists with status Booked; capture baseline bytes & record count.
//   2. Test B: Confirmed cancellation sets status to Cancelled, retains record in CSV, preserves all other fields,
//              returns "Appointment cancelled successfully", reloads as Cancelled, listing displays Cancelled.
//   3. Test C: Declined cancellation leaves status Booked, returns "Cancellation cancelled by user",
//              file remains byte-for-byte identical, listing displays Booked.
//   4. Test D: Interactive confirmation workflow exercises menu option 3, prompts for ID & confirmation,
//              'Y'/'y' cancels successfully, 'N'/'n' leaves data unchanged, returns safely to menu.
//   5. Test E: Regression & error cases: unknown appointment ID rejected, already-cancelled appointment
//              rejected, no duplicate records created, slot rebooking allowed after cancellation.
//   6. Test F: Data integrity on multi-appointment dataset: only target record modified, others unchanged,
//              durability across reload, workspace root CSV files remain untouched.

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
  std::cout << "=== Running TC-APT-09 Cancel Appointment Tests ===\n";
  std::cout << "=== (Traceability: HMS-F-023, STP: TC-APT-09, SRS: TC-APT-09, Issue #32) ===\n\n";

  // Capture baseline of root data files to guarantee they remain untouched
  const std::string rootPatientCsv = "patients.csv";
  const std::string rootDoctorCsv = "doctors.csv";
  const std::string rootApptCsv = "appointments.csv";

  const std::string rootPatientBaseline = readFile(rootPatientCsv);
  const std::string rootDoctorBaseline = readFile(rootDoctorCsv);
  const std::string rootApptBaseline = readFile(rootApptCsv);

  // Isolated test directory
  const std::string testDir = "build/test_cancel_apt_09";
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

  // Setup: Seed valid patient and doctor
  std::string err;
  hms::Patient p1("P001", "Alice Smith", 30, "F", "1234567890", "123 Maple St", "None");
  hms::Doctor d1("D001", "Dr. Bob Jones", "Cardiology", "Cardiology", "0987654321", "Mon-Fri");
  hms::Patient p2("P002", "Charlie Brown", 45, "M", "9876543210", "456 Oak Ave", "Asthma");
  hms::Doctor d2("D002", "Dr. Diana Prince", "Neurology", "Neurology", "1122334455", "Tue-Sat");

  check("Seed patient P001", fm.savePatient(p1, err));
  check("Seed patient P002", fm.savePatient(p2, err));
  check("Seed doctor D001", fm.saveDoctor(d1, err));
  check("Seed doctor D002", fm.saveDoctor(d2, err));

  // =========================================================================
  // Test A — Valid appointment exists
  // =========================================================================
  std::cout << "\n-- Test A: Valid appointment exists --\n";
  {
    hms::Appointment appt1("A0001", "P001", "D001", "2026-11-10", "10:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0001 (Booked)", fm.saveAppointment(appt1, err));

    hms::HospitalSystem system(fm);
    const hms::Appointment *loaded = system.findAppointment("A0001");
    check("Appointment A0001 found in system", loaded != nullptr);
    if (loaded) {
      check("Initial status is Booked", loaded->isBooked());
      check("Initial isCancelled is false", !loaded->isCancelled());
      check("Initial status string is 'Booked'", loaded->getStatusString() == "Booked");
      check("Initial patient ID is P001", loaded->getPatientId() == "P001");
      check("Initial doctor ID is D001", loaded->getDoctorId() == "D001");
      check("Initial date is 2026-11-10", loaded->getDate() == "2026-11-10");
      check("Initial time is 10:00", loaded->getTime() == "10:00");
    }

    const int rowCount = countDataRows(testApptCsv);
    check("Initial appointment CSV record count is 1", rowCount == 1);
  }

  // =========================================================================
  // Test B — Confirm cancellation
  // =========================================================================
  std::cout << "\n-- Test B: Confirm cancellation --\n";
  {
    hms::HospitalSystem system(fm);

    // Cancel with confirmed = true
    const hms::OpResult res = system.cancelAppointment("A0001", true);
    check("Confirmed cancellation operation succeeds", res.success);
    check("Success message is 'Appointment cancelled successfully'",
          res.message == "Appointment cancelled successfully");

    // Check appointment object in system
    const hms::Appointment *appt = system.findAppointment("A0001");
    check("Appointment A0001 still exists under same ID", appt != nullptr);
    if (appt) {
      check("Status is Cancelled", appt->isCancelled());
      check("isBooked is false", !appt->isBooked());
      check("Status string is 'Cancelled'", appt->getStatusString() == "Cancelled");
      check("Patient ID remains P001", appt->getPatientId() == "P001");
      check("Doctor ID remains D001", appt->getDoctorId() == "D001");
      check("Date remains 2026-11-10", appt->getDate() == "2026-11-10");
      check("Time remains 10:00", appt->getTime() == "10:00");
    }

    // Verify record in CSV file on disk
    const std::string csvContent = readFile(testApptCsv);
    check("Record remains in appointments.csv", csvContent.find("A0001") != std::string::npos);
    check("Record in CSV has status Cancelled",
          csvContent.find("A0001,P001,D001,2026-11-10,10:00,Cancelled") != std::string::npos);
    check("Record count in appointments.csv remains 1 (updated, not deleted)",
          countDataRows(testApptCsv) == 1);

    // Durability across system reload from persisted storage
    hms::FileManager reloadFm(testPatientCsv, testDoctorCsv, testApptCsv);
    hms::HospitalSystem reloadSys(reloadFm);
    const hms::Appointment *reloadedAppt = reloadSys.findAppointment("A0001");
    check("Reloaded appointment exists after system restart", reloadedAppt != nullptr);
    if (reloadedAppt) {
      check("Reloaded status is still Cancelled", reloadedAppt->isCancelled());
      check("Reloaded status string is still 'Cancelled'",
            reloadedAppt->getStatusString() == "Cancelled");
      check("Reloaded fields intact",
            reloadedAppt->getPatientId() == "P001" &&
            reloadedAppt->getDoctorId() == "D001" &&
            reloadedAppt->getDate() == "2026-11-10" &&
            reloadedAppt->getTime() == "10:00");
    }

    // Verify appointment listing displays record with status Cancelled
    const auto listAll = reloadSys.listAppointments("ALL");
    check("listAppointments('ALL') returns 1 appointment", listAll.size() == 1);
    if (!listAll.empty()) {
      check("listAppointments record is Cancelled", listAll[0].isCancelled());
    }

    const std::string viewAllOut = captureViewAppointments(reloadSys, "ALL");
    check("viewAppointments('ALL') displays status Cancelled",
          viewAllOut.find("Status: Cancelled") != std::string::npos &&
          viewAllOut.find("A0001") != std::string::npos);

    const std::string viewPatOut = captureViewAppointments(reloadSys, "PATIENT", "P001");
    check("viewAppointments('PATIENT', 'P001') displays status Cancelled",
          viewPatOut.find("Status: Cancelled") != std::string::npos);

    const std::string viewDocOut = captureViewAppointments(reloadSys, "DOCTOR", "D001");
    check("viewAppointments('DOCTOR', 'D001') displays status Cancelled",
          viewDocOut.find("Status: Cancelled") != std::string::npos);
  }

  // =========================================================================
  // Test C — Decline cancellation
  // =========================================================================
  std::cout << "\n-- Test C: Decline cancellation --\n";
  {
    // Seed a new booked appointment A0002
    hms::Appointment appt2("A0002", "P002", "D002", "2026-11-15", "14:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0002 (Booked)", fm.saveAppointment(appt2, err));

    const int rowCountBefore = countDataRows(testApptCsv);
    const std::string baselineBytes = readFile(testApptCsv);

    hms::HospitalSystem system(fm);

    // Decline cancellation via confirmed = false
    const hms::OpResult declineRes = system.cancelAppointment("A0002", false);
    check("Decline operation returns success = false", !declineRes.success);
    check("Decline message is 'Cancellation cancelled by user'",
          declineRes.message == "Cancellation cancelled by user");

    // Verify appointment remains Booked
    const hms::Appointment *appt = system.findAppointment("A0002");
    check("Appointment A0002 found after decline", appt != nullptr);
    if (appt) {
      check("Appointment A0002 remains Booked", appt->isBooked());
      check("Appointment A0002 is not Cancelled", !appt->isCancelled());
      check("Status string remains 'Booked'", appt->getStatusString() == "Booked");
      check("All fields unchanged",
            appt->getPatientId() == "P002" &&
            appt->getDoctorId() == "D002" &&
            appt->getDate() == "2026-11-15" &&
            appt->getTime() == "14:00");
    }

    // Verify record count unchanged and CSV contents byte-for-byte identical
    check("Record count remains unchanged after decline",
          countDataRows(testApptCsv) == rowCountBefore);
    const std::string afterDeclineBytes = readFile(testApptCsv);
    check("CSV file is byte-for-byte identical after decline",
          afterDeclineBytes == baselineBytes);

    // Verify appointment listing displays A0002 as Booked
    const std::string viewOut = captureViewAppointments(system, "PATIENT", "P002");
    check("Listing displays A0002 with status Booked",
          viewOut.find("Status: Booked") != std::string::npos &&
          viewOut.find("A0002") != std::string::npos);
  }

  // =========================================================================
  // Test D — Interactive confirmation workflow
  // =========================================================================
  std::cout << "\n-- Test D: Interactive confirmation workflow --\n";
  {
    // D1: Confirmed cancellation with uppercase 'Y'
    // Flow: Main menu 3 (Appointment) -> 3 (Cancel) -> A0002 -> Y -> 0 (Return) -> 4 (Exit)
    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA0002\nY\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D1: Displays '--- Cancel Appointment ---'",
            output.find("--- Cancel Appointment ---") != std::string::npos);
      check("Interactive D1: Prompts 'Enter appointment ID: '",
            output.find("Enter appointment ID: ") != std::string::npos);
      check("Interactive D1: Prompts 'Confirm cancellation? (Y/N): '",
            output.find("Confirm cancellation? (Y/N): ") != std::string::npos);
      const size_t summaryPos = output.find("--- Appointment Summary ---");
      const size_t confirmationPos =
          output.find("Confirm cancellation? (Y/N): ");

      check("Interactive D1: Displays appointment summary",
            summaryPos != std::string::npos);
      check("Interactive D1: Summary appears before confirmation",
            summaryPos != std::string::npos &&
            confirmationPos != std::string::npos &&
            summaryPos < confirmationPos);
      check("Interactive D1: Displays appointment ID",
            output.find("Appointment ID : A0002") != std::string::npos);
      check("Interactive D1: Displays patient name",
            output.find("Patient Name") != std::string::npos &&
            output.find("Charlie Brown") != std::string::npos);
      check("Interactive D1: Displays doctor name",
            output.find("Doctor Name") != std::string::npos &&
            output.find("Dr. Diana Prince") != std::string::npos);
      check("Interactive D1: Displays appointment date",
            output.find("2026-11-15") != std::string::npos);
      check("Interactive D1: Displays appointment time",
            output.find("14:00") != std::string::npos);
      check("Interactive D1: Displays current status",
            output.find("Status         : Booked") != std::string::npos);
      check("Interactive D1: Displays 'Appointment cancelled successfully'",
            output.find("Appointment cancelled successfully") != std::string::npos);
      check("Interactive D1: Clean exit message displayed",
            output.find("Exiting Hospital Management System.") != std::string::npos);

      const hms::Appointment *a2 = sys.findAppointment("A0002");
      check("Interactive D1: A0002 status is now Cancelled", a2 != nullptr && a2->isCancelled());
    }

    // D2: Confirmed cancellation with lowercase 'y'
    // Seed A0003 as Booked
    hms::Appointment appt3("A0003", "P001", "D002", "2026-11-20", "11:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0003 (Booked)", fm.saveAppointment(appt3, err));

    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA0003\ny\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D2 ('y'): Displays 'Appointment cancelled successfully'",
            output.find("Appointment cancelled successfully") != std::string::npos);

      const hms::Appointment *a3 = sys.findAppointment("A0003");
      check("Interactive D2: A0003 status is Cancelled", a3 != nullptr && a3->isCancelled());
    }

    // D3: Declined cancellation with uppercase 'N'
    // Seed A0004 as Booked
    hms::Appointment appt4("A0004", "P002", "D001", "2026-11-22", "15:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0004 (Booked)", fm.saveAppointment(appt4, err));

    const std::string d3Baseline = readFile(testApptCsv);
    const int d3Rows = countDataRows(testApptCsv);

    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA0004\nN\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D3 ('N'): Displays 'Cancellation cancelled by user'",
            output.find("Cancellation cancelled by user") != std::string::npos);
      check("Interactive D3: Clean exit message displayed",
            output.find("Exiting Hospital Management System.") != std::string::npos);

      const hms::Appointment *a4 = sys.findAppointment("A0004");
      check("Interactive D3: A0004 remains Booked", a4 != nullptr && a4->isBooked());
      check("Interactive D3: Row count unchanged", countDataRows(testApptCsv) == d3Rows);
      check("Interactive D3: CSV byte-for-byte identical", readFile(testApptCsv) == d3Baseline);
    }

    // D4: Declined cancellation with lowercase 'n'
    // Seed A0005 as Booked
    hms::Appointment appt5("A0005", "P001", "D001", "2026-11-25", "09:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0005 (Booked)", fm.saveAppointment(appt5, err));

    const std::string d4Baseline = readFile(testApptCsv);
    const int d4Rows = countDataRows(testApptCsv);

    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA0005\nn\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D4 ('n'): Displays 'Cancellation cancelled by user'",
            output.find("Cancellation cancelled by user") != std::string::npos);

      const hms::Appointment *a5 = sys.findAppointment("A0005");
      check("Interactive D4: A0005 remains Booked", a5 != nullptr && a5->isBooked());
      check("Interactive D4: Row count unchanged", countDataRows(testApptCsv) == d4Rows);
      check("Interactive D4: CSV byte-for-byte identical", readFile(testApptCsv) == d4Baseline);
    }

    // D5: Empty appointment ID in interactive flow
    {
      hms::HospitalSystem sys(fm);
      // Empty appointment ID is rejected immediately by Appointment::validateId.
      // The flow returns safely to the appointment menu; then cancel A0005 and decline.
      const std::string input = "3\n3\n\n3\nA0005\nN\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D5: Empty ID displays the specific appointment ID validation error",
            output.find("Appointment ID: must start with 'A' followed by at least 4 digits.") != std::string::npos);
      check("Interactive D5: Prompt recovers and completes successfully",
            output.find("Cancellation cancelled by user") != std::string::npos);
    }

    // D6: Unknown appointment ID is rejected before confirmation.
    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA9999\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D6: Unknown ID displays 'Appointment not found'",
            output.find("Appointment not found") != std::string::npos);

      check("Interactive D6: Unknown ID does not ask for confirmation",
            output.find("Confirm cancellation? (Y/N): ") == std::string::npos);
    }

    // D7: Already-cancelled appointment is rejected before confirmation.
    {
      hms::HospitalSystem sys(fm);
      const std::string input = "3\n3\nA0001\n0\n4\n";
      const std::string output = runInteractive(sys, input);

      check("Interactive D7: Already-cancelled ID displays the correct message",
            output.find("Appointment already cancelled") != std::string::npos);
      check("Interactive D7: Already-cancelled ID does not ask for confirmation",
            output.find("Confirm cancellation? (Y/N): ") == std::string::npos);
    }
     }
  // =========================================================================
  // Test E — Regression and error cases
  // =========================================================================
  std::cout << "\n-- Test E: Regression and error cases --\n";
  {
    hms::HospitalSystem system(fm);

    // E1: Unknown appointment ID rejected
    const hms::OpResult unknownRes = system.cancelAppointment("A9999", true);
    check("Unknown appointment ID rejected (success = false)", !unknownRes.success);
    check("Unknown appointment ID message is 'Appointment not found'",
          unknownRes.message == "Appointment not found");

    // E2: Already-cancelled appointment rejected
    // A0001 is already cancelled
    const int rowsBeforeReCancel = countDataRows(testApptCsv);
    const std::string bytesBeforeReCancel = readFile(testApptCsv);

    const hms::OpResult alreadyCancelledRes = system.cancelAppointment("A0001", true);
    check("Already-cancelled appointment rejected (success = false)", !alreadyCancelledRes.success);
    check("Already-cancelled message is 'Appointment already cancelled'",
          alreadyCancelledRes.message == "Appointment already cancelled");
    check("Already-cancelled does not create duplicate record in CSV",
          countDataRows(testApptCsv) == rowsBeforeReCancel);
    check("Already-cancelled does not alter CSV file",
          readFile(testApptCsv) == bytesBeforeReCancel);

    // E3: Case-insensitive appointment ID cancellation
    // Seed A0006
    hms::Appointment appt6("A0006", "P001", "D001", "2026-11-28", "16:00", hms::AppointmentStatus::Booked);
    check("Seed appointment A0006 (Booked)", fm.saveAppointment(appt6, err));

    const hms::OpResult caseRes = system.cancelAppointment("a0006", true);
    check("Lowercase appointment ID 'a0006' succeeds", caseRes.success);
    const hms::Appointment *a6 = system.findAppointment("A0006");
    check("A0006 status is Cancelled", a6 != nullptr && a6->isCancelled());

    // E4: Programmatic cancellation with confirmed = false does not modify data
    const std::string bytesBeforeConfirmedFalse = readFile(testApptCsv);
    const hms::OpResult noConfirmRes = system.cancelAppointment("A0005", false);
    check("confirmed = false returns false", !noConfirmRes.success);
    check("confirmed = false returns 'Cancellation cancelled by user'",
          noConfirmRes.message == "Cancellation cancelled by user");
    check("confirmed = false leaves CSV untouched",
          readFile(testApptCsv) == bytesBeforeConfirmedFalse);

    // E5: Rebooking a vacated slot after cancellation (HMS-F-021)
    // A0001 was Doctor D001 on 2026-11-10 at 10:00, now cancelled.
    std::string availMsg;
    const bool available = system.checkDoctorAvailability("D001", "2026-11-10", "10:00", availMsg);
    check("Doctor D001 is available on 2026-11-10 10:00 after cancellation", available);

    std::string newApptId;
    const hms::OpResult rebookRes = system.bookAppointment("P002", "D001", "2026-11-10", "10:00", newApptId);
    check("Rebooking vacated slot succeeds", rebookRes.success);
    check("New appointment has new ID", !newApptId.empty() && newApptId != "A0001");

    // E6: Double-booking prevention still blocks second booking of the now active slot
    std::string conflictId;
    const hms::OpResult conflictRes = system.bookAppointment("P001", "D001", "2026-11-10", "10:00", conflictId);
    check("Double-booking conflicting slot is rejected", !conflictRes.success);
    check("Conflict message reports doctor not available",
          conflictRes.message.find("not available") != std::string::npos);
  }

  // =========================================================================
  // Test F — Data integrity
  // =========================================================================
  std::cout << "\n-- Test F: Data integrity --\n";
  {
    // Use an independent test directory to verify isolated multi-record behavior
    const std::string integrityDir = "build/test_cancel_integrity";
    std::filesystem::create_directories(integrityDir, dirEc);

    const std::string intPatCsv = integrityDir + "/patients.csv";
    const std::string intDocCsv = integrityDir + "/doctors.csv";
    const std::string intApptCsv = integrityDir + "/appointments.csv";

    removeFile(intPatCsv);
    removeFile(intDocCsv);
    removeFile(intApptCsv);

    hms::FileManager intFm(intPatCsv, intDocCsv, intApptCsv);
    intFm.savePatient(p1, err);
    intFm.savePatient(p2, err);
    intFm.saveDoctor(d1, err);
    intFm.saveDoctor(d2, err);

    // 3 appointments: A0001, A0002, A0003 all Booked
    std::vector<hms::Appointment> initialList = {
      hms::Appointment("A0001", "P001", "D001", "2026-12-01", "09:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0002", "P002", "D001", "2026-12-01", "10:00", hms::AppointmentStatus::Booked),
      hms::Appointment("A0003", "P001", "D002", "2026-12-01", "11:00", hms::AppointmentStatus::Booked)
    };
    check("Seed 3 appointments", intFm.saveAppointments(initialList, err));
    check("Initial row count is 3", countDataRows(intApptCsv) == 3);

    hms::HospitalSystem intSys(intFm);

    // Cancel A0002 with confirmation
    const hms::OpResult intCancelRes = intSys.cancelAppointment("A0002", true);
    check("Integrity: Cancel A0002 succeeds", intCancelRes.success);
    check("Integrity: Row count remains 3 (no deletion or addition)",
          countDataRows(intApptCsv) == 3);

    // Verify other appointments A0001 and A0003 are completely unchanged
    const hms::Appointment *a1 = intSys.findAppointment("A0001");
    check("A0001 still exists", a1 != nullptr);
    if (a1) {
      check("A0001 still Booked", a1->isBooked());
      check("A0001 patient is P001", a1->getPatientId() == "P001");
      check("A0001 doctor is D001", a1->getDoctorId() == "D001");
      check("A0001 date is 2026-12-01", a1->getDate() == "2026-12-01");
      check("A0001 time is 09:00", a1->getTime() == "09:00");
    }

    const hms::Appointment *a2 = intSys.findAppointment("A0002");
    check("A0002 still exists", a2 != nullptr);
    if (a2) {
      check("A0002 is Cancelled", a2->isCancelled());
      check("A0002 patient is P002", a2->getPatientId() == "P002");
      check("A0002 doctor is D001", a2->getDoctorId() == "D001");
      check("A0002 date is 2026-12-01", a2->getDate() == "2026-12-01");
      check("A0002 time is 10:00", a2->getTime() == "10:00");
    }

    const hms::Appointment *a3 = intSys.findAppointment("A0003");
    check("A0003 still exists", a3 != nullptr);
    if (a3) {
      check("A0003 still Booked", a3->isBooked());
      check("A0003 patient is P001", a3->getPatientId() == "P001");
      check("A0003 doctor is D002", a3->getDoctorId() == "D002");
      check("A0003 date is 2026-12-01", a3->getDate() == "2026-12-01");
      check("A0003 time is 11:00", a3->getTime() == "11:00");
    }

    // Capture bytes before decline attempt on A0003
    const std::string bytesBeforeDecline = readFile(intApptCsv);
    const hms::OpResult intDeclineRes = intSys.cancelAppointment("A0003", false);
    check("Integrity: Decline A0003 returns false", !intDeclineRes.success);
    check("Integrity: File byte-for-byte unchanged after decline",
          readFile(intApptCsv) == bytesBeforeDecline);

    // Durability: Reload from storage into fresh system instance
    hms::FileManager freshIntFm(intPatCsv, intDocCsv, intApptCsv);
    hms::HospitalSystem freshIntSys(freshIntFm);

    const auto freshList = freshIntSys.listAppointments("ALL");
    check("Fresh system loads 3 appointments", freshList.size() == 3);
    int bookedCount = 0;
    int cancelledCount = 0;
    for (const auto &item : freshList) {
      if (item.isBooked()) ++bookedCount;
      if (item.isCancelled()) ++cancelledCount;
    }
    check("Fresh system has exactly 2 Booked appointments", bookedCount == 2);
    check("Fresh system has exactly 1 Cancelled appointment", cancelledCount == 1);

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
  std::cout << "TC-APT-09 Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=========================================\n";

  return g_failed == 0 ? 0 : 1;
}
