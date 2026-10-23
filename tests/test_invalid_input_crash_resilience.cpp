// ============================================================================
// TC-NFR-01: Invalid-Input Crash Resilience Regression Test Suite
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-NF-001 (Non-Functional Quality Attributes)
//   STP RTM Test Case:  TC-NFR-01 (Table 17, Software Test Plan §10)
//   SRS RTM Test Case:  TC-NFR-01 (Reliability & Crash Resilience)
//   GitHub Issue:       #37
//
// Requirement Statement:
//   "Invalid-input crash resilience: Do not crash on invalid keyboard input;
//    show an error and return to a prompt."
//
// Acceptance Criterion:
//   "0 crashes across the 20 defined invalid-input test cases."
//
// Source-of-Truth Note:
//   Software Test Plan Table 17 (p. 13) specifies TC-NFR-01 with a matrix of
//   20 invalid inputs ("", "abc", "-5", "999999", "%x%s", etc.) and an acceptance
//   criterion of zero crashes (0 crash rate) while menus re-prompt cleanly.
//   Because the repository documentation summarizes the count without enumerating
//   every individual case, the authoritative 20-case matrix below was derived to
//   systematically cover all 4 interactive menus across the 4 primary invalid input
//   vectors (16 menu cases) plus 4 key field-validation input boundaries (4 field cases).
//
// Strengthened Recovery Verification:
//   - Uses countOccurrences() to assert that after each invalid menu input, the
//     appropriate menu header and prompt appear again (count == 2) rather than merely once.
//   - Verifies absence of incorrect menu headers (e.g. Doctor or Appointment menu headers
//     must not appear during a Patient menu recovery test).
//   - For field validation cases (17-20), verifies that following an invalid field entry
//     and error display, the parent submenu is re-displayed cleanly and processes
//     subsequent valid navigation/exit choices without data corruption.
//
// 20-Case Test Matrix Definition:
//   --------------------------------------------------------------------------
//   Case | Target Component        | Invalid Input | Expected Observable Behavior
//   --------------------------------------------------------------------------
//   01   | Main Menu               | "9"           | Error displayed, Main Menu re-prompts (count=2), exits on '4'
//   02   | Main Menu               | "abc"         | Error displayed, Main Menu re-prompts (count=2), exits on '4'
//   03   | Main Menu               | "-1"          | Error displayed, Main Menu re-prompts (count=2), exits on '4'
//   04   | Main Menu               | ""            | Error displayed, Main Menu re-prompts (count=2), exits on '4'
//   05   | Patient Submenu         | "9"           | Error displayed, Patient Menu re-prompts (count=2), return '0', exit '4'
//   06   | Patient Submenu         | "abc"         | Error displayed, Patient Menu re-prompts (count=2), return '0', exit '4'
//   07   | Patient Submenu         | "-1"          | Error displayed, Patient Menu re-prompts (count=2), return '0', exit '4'
//   08   | Patient Submenu         | ""            | Error displayed, Patient Menu re-prompts (count=2), return '0', exit '4'
//   09   | Doctor Submenu          | "9"           | Error displayed, Doctor Menu re-prompts (count=2), return '0', exit '4'
//   10   | Doctor Submenu          | "abc"         | Error displayed, Doctor Menu re-prompts (count=2), return '0', exit '4'
//   11   | Doctor Submenu          | "-1"          | Error displayed, Doctor Menu re-prompts (count=2), return '0', exit '4'
//   12   | Doctor Submenu          | ""            | Error displayed, Doctor Menu re-prompts (count=2), return '0', exit '4'
//   13   | Appointment Submenu     | "9"           | Error displayed, Appt Menu re-prompts (count=2), return '0', exit '4'
//   14   | Appointment Submenu     | "abc"         | Error displayed, Appt Menu re-prompts (count=2), return '0', exit '4'
//   15   | Appointment Submenu     | "-1"          | Error displayed, Appt Menu re-prompts (count=2), return '0', exit '4'
//   16   | Appointment Submenu     | ""            | Error displayed, Appt Menu re-prompts (count=2), return '0', exit '4'
//   17   | Patient Age Field       | "abc"         | Error displayed, Patient Menu re-displayed (count=2), no record created
//   18   | Patient Age Field       | "999"         | Error displayed, Patient Menu re-displayed (count=2), no record created
//   19   | Appt Date Field         | "2026-02-30"  | Error displayed, Appt Menu re-displayed (count=2), no record created
//   20   | Appt Time Field         | "25:99"       | Error displayed, Appt Menu re-displayed (count=2), no record created
//   --------------------------------------------------------------------------
// ============================================================================

#include "hms/HospitalSystem.h"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_casesPassed = 0;
int g_casesFailed = 0;
int g_crashCount  = 0;

// Count non-overlapping occurrences of needle in haystack.
std::size_t countOccurrences(const std::string &haystack,
                             const std::string &needle) {
  if (needle.empty()) {
    return 0;
  }
  std::size_t count = 0;
  std::size_t pos   = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

// Read entire file content into a string for strict integrity verification.
std::string readFileContent(const std::filesystem::path &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// Seed isolated CSV files in the isolated test directory.
bool seedTestData(const std::filesystem::path &dataDir) {
  hms::FileManager files((dataDir / "patients.csv").string(),
                         (dataDir / "doctors.csv").string(),
                         (dataDir / "appointments.csv").string());
  std::string err;

  const hms::Patient patient("P001", "Anita Sharma", 28, "F", "9876543210",
                             "12 Residency Rd Bengaluru", "None");
  if (!files.savePatient(patient, err)) {
    std::cerr << "Seed error (Patient): " << err << '\n';
    return false;
  }

  const hms::Doctor doctor("D001", "Dr. Meera Rao", "Cardiology", "Cardiology",
                           "9876543211", "Mon-Fri 09:00-17:00");
  if (!files.saveDoctor(doctor, err)) {
    std::cerr << "Seed error (Doctor): " << err << '\n';
    return false;
  }

  const hms::Appointment appt("A0001", "P001", "D001", "2026-11-10", "10:00",
                              hms::AppointmentStatus::Booked);
  if (!files.saveAppointment(appt, err)) {
    std::cerr << "Seed error (Appointment): " << err << '\n';
    return false;
  }

  return true;
}

// Execute HospitalSystem::run() with injected cin input and captured cout.
// Detects and captures abnormal termination / uncaught exceptions.
struct RunResult {
  std::string output;
  bool crashed{false};
  std::string crashReason;
};

RunResult executeInteractiveSession(const std::filesystem::path &dataDir,
                                    const std::string &input) {
  RunResult res;
  std::istringstream in(input);
  std::ostringstream out;
  auto *oldIn  = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  try {
    hms::HospitalSystem sys(dataDir.string());
    sys.run();
  } catch (const std::exception &ex) {
    res.crashed = true;
    res.crashReason = std::string("std::exception: ") + ex.what();
    ++g_crashCount;
  } catch (...) {
    res.crashed = true;
    res.crashReason = "Unknown non-standard exception";
    ++g_crashCount;
  }

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);
  res.output = out.str();
  return res;
}

void reportCaseResult(int caseNumber, const std::string &caseTitle,
                      bool condition, const std::string &details = "") {
  if (condition) {
    std::cout << "  [PASS] Case " << (caseNumber < 10 ? "0" : "")
              << caseNumber << ": " << caseTitle << '\n';
    ++g_casesPassed;
  } else {
    std::cout << "  [FAIL] Case " << (caseNumber < 10 ? "0" : "")
              << caseNumber << ": " << caseTitle;
    if (!details.empty()) {
      std::cout << " (" << details << ")";
    }
    std::cout << '\n';
    ++g_casesFailed;
  }
}

} // namespace

int main() {
  std::cout << "======================================================================\n"
            << "TC-NFR-01: Invalid-Input Crash Resilience (HMS-NF-001)\n"
            << "Acceptance Criterion: 0 crashes across the 20 defined invalid-input cases\n"
            << "======================================================================\n\n";

  // Use isolated test data directory placed under build/ if present
  std::filesystem::path dataDir = "TC-NFR-01-crash-resilience-data";
  if (std::filesystem::exists("build") && std::filesystem::is_directory("build")) {
    dataDir = std::filesystem::path("build") / "TC-NFR-01-crash-resilience-data";
  }

  std::error_code ec;
  std::filesystem::remove_all(dataDir, ec);
  std::filesystem::create_directories(dataDir, ec);
  if (ec) {
    std::cerr << "Fatal: Failed to create isolated test directory: "
              << ec.message() << '\n';
    return 1;
  }

  if (!seedTestData(dataDir)) {
    std::cerr << "Fatal: Failed to seed baseline test data\n";
    std::filesystem::remove_all(dataDir, ec);
    return 1;
  }

  const std::string baselinePatientsCsv = readFileContent(dataDir / "patients.csv");
  const std::string baselineDoctorsCsv = readFileContent(dataDir / "doctors.csv");
  const std::string baselineAppointmentsCsv = readFileContent(dataDir / "appointments.csv");

  const std::string mainMenuHeader = "=== Hospital Management System ===";
  const std::string patientMenuHeader = "=== Patient Management ===";
  const std::string doctorMenuHeader = "=== Doctor Management ===";
  const std::string appointmentMenuHeader = "=== Appointment Management ===";
  const std::string choicePrompt = "Enter your choice: ";
  const std::string mainMenuError = "Invalid choice. Enter 1, 2, 3, or 4.";
  const std::string submenuError = "Invalid choice. Enter 0 to 4.";
  const std::string exitMessage = "Exiting Hospital Management System.";

  // Helper lambda to check baseline CSV files remain untouched
  auto checkDataIntegrity = [&]() -> bool {
    return (readFileContent(dataDir / "patients.csv") == baselinePatientsCsv) &&
           (readFileContent(dataDir / "doctors.csv") == baselineDoctorsCsv) &&
           (readFileContent(dataDir / "appointments.csv") == baselineAppointmentsCsv);
  };

  // =========================================================================
  // Menu Input Cases 01 to 04: Main Menu
  // Inputs: "9" (out-of-range), "abc" (non-numeric), "-1" (negative), "" (empty)
  // Verification:
  //   - Main Menu header displayed exactly 2 times (initial + redisplay)
  //   - Choice prompt displayed exactly 2 times
  //   - Main menu error displayed exactly once
  //   - Submenu headers (Patient, Doctor, Appointment) do not appear (count == 0)
  //   - Option '4' exits cleanly
  //   - Zero crashes and CSV files unmodified
  // =========================================================================
  std::cout << "--- Testing Category A: Main Menu Invalid Input Vectors (Cases 01-04) ---\n";

  // Case 01: Main menu out-of-range choice "9"
  {
    const RunResult res = executeInteractiveSession(dataDir, "9\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, mainMenuError) == 1 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(1, "Main menu out-of-range choice ('9')", pass,
                     res.crashed ? res.crashReason : "Menu re-prompt count mismatch or state corruption");
  }

  // Case 02: Main menu non-numeric input "abc"
  {
    const RunResult res = executeInteractiveSession(dataDir, "abc\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, mainMenuError) == 1 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(2, "Main menu non-numeric text ('abc')", pass,
                     res.crashed ? res.crashReason : "Menu re-prompt count mismatch or state corruption");
  }

  // Case 03: Main menu negative choice "-1"
  {
    const RunResult res = executeInteractiveSession(dataDir, "-1\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, mainMenuError) == 1 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(3, "Main menu negative choice ('-1')", pass,
                     res.crashed ? res.crashReason : "Menu re-prompt count mismatch or state corruption");
  }

  // Case 04: Main menu empty input line ""
  {
    const RunResult res = executeInteractiveSession(dataDir, "\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, mainMenuError) == 1 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(4, "Main menu empty line ('')", pass,
                     res.crashed ? res.crashReason : "Menu re-prompt count mismatch or state corruption");
  }

  // =========================================================================
  // Menu Input Cases 05 to 08: Patient Management Submenu
  // Inputs: "9", "abc", "-1", ""
  // Verification:
  //   - Patient Submenu header displayed exactly 2 times (initial + redisplay)
  //   - Submenu error displayed exactly once
  //   - Main menu header displayed exactly 2 times (initial entry + return on '0')
  //   - Choice prompt displayed exactly 4 times (Main, Patient, Patient re-prompt, Main re-prompt)
  //   - Doctor and Appointment menu headers do not appear (count == 0)
  //   - Clean exit via Main menu '4'
  //   - Zero crashes and CSV files unmodified
  // =========================================================================
  std::cout << "\n--- Testing Category B: Patient Submenu Invalid Input Vectors (Cases 05-08) ---\n";

  // Case 05: Patient submenu out-of-range choice "9"
  {
    const RunResult res = executeInteractiveSession(dataDir, "1\n9\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(5, "Patient submenu out-of-range choice ('9')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 06: Patient submenu non-numeric input "abc"
  {
    const RunResult res = executeInteractiveSession(dataDir, "1\nabc\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(6, "Patient submenu non-numeric text ('abc')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 07: Patient submenu negative choice "-1"
  {
    const RunResult res = executeInteractiveSession(dataDir, "1\n-1\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(7, "Patient submenu negative choice ('-1')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 08: Patient submenu empty input line ""
  {
    const RunResult res = executeInteractiveSession(dataDir, "1\n\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(8, "Patient submenu empty line ('')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // =========================================================================
  // Menu Input Cases 09 to 12: Doctor Management Submenu
  // Inputs: "9", "abc", "-1", ""
  // Verification:
  //   - Doctor Submenu header displayed exactly 2 times (initial + redisplay)
  //   - Submenu error displayed exactly once
  //   - Main menu header displayed exactly 2 times (initial entry + return on '0')
  //   - Choice prompt displayed exactly 4 times
  //   - Patient and Appointment menu headers do not appear (count == 0)
  //   - Clean exit via Main menu '4'
  //   - Zero crashes and CSV files unmodified
  // =========================================================================
  std::cout << "\n--- Testing Category C: Doctor Submenu Invalid Input Vectors (Cases 09-12) ---\n";

  // Case 09: Doctor submenu out-of-range choice "9"
  {
    const RunResult res = executeInteractiveSession(dataDir, "2\n9\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, doctorMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(9, "Doctor submenu out-of-range choice ('9')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 10: Doctor submenu non-numeric input "abc"
  {
    const RunResult res = executeInteractiveSession(dataDir, "2\nabc\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, doctorMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(10, "Doctor submenu non-numeric text ('abc')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 11: Doctor submenu negative choice "-1"
  {
    const RunResult res = executeInteractiveSession(dataDir, "2\n-1\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, doctorMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(11, "Doctor submenu negative choice ('-1')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 12: Doctor submenu empty input line ""
  {
    const RunResult res = executeInteractiveSession(dataDir, "2\n\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, doctorMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(12, "Doctor submenu empty line ('')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // =========================================================================
  // Menu Input Cases 13 to 16: Appointment Management Submenu
  // Inputs: "9", "abc", "-1", ""
  // Verification:
  //   - Appointment Submenu header displayed exactly 2 times (initial + redisplay)
  //   - Submenu error displayed exactly once
  //   - Main menu header displayed exactly 2 times (initial entry + return on '0')
  //   - Choice prompt displayed exactly 4 times
  //   - Patient and Doctor menu headers do not appear (count == 0)
  //   - Clean exit via Main menu '4'
  //   - Zero crashes and CSV files unmodified
  // =========================================================================
  std::cout << "\n--- Testing Category D: Appointment Submenu Invalid Input Vectors (Cases 13-16) ---\n";

  // Case 13: Appointment submenu out-of-range choice "9"
  {
    const RunResult res = executeInteractiveSession(dataDir, "3\n9\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(13, "Appointment submenu out-of-range choice ('9')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 14: Appointment submenu non-numeric input "abc"
  {
    const RunResult res = executeInteractiveSession(dataDir, "3\nabc\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(14, "Appointment submenu non-numeric text ('abc')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 15: Appointment submenu negative choice "-1"
  {
    const RunResult res = executeInteractiveSession(dataDir, "3\n-1\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(15, "Appointment submenu negative choice ('-1')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // Case 16: Appointment submenu empty input line ""
  {
    const RunResult res = executeInteractiveSession(dataDir, "3\n\n0\n4\n");
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, submenuError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, choicePrompt) == 4 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(16, "Appointment submenu empty line ('')", pass,
                     res.crashed ? res.crashReason : "Submenu re-prompt count mismatch or state corruption");
  }

  // =========================================================================
  // Field Validation Cases 17 to 20: Registration & Booking Input Boundaries
  // Verification:
  //   - Expected field-level error message is output exactly once
  //   - Parent submenu header is redisplayed after error (count == 2)
  //   - Main menu header is displayed 2 times (entry + return)
  //   - Incorrect submenu headers do NOT appear (count == 0)
  //   - Subsequent input ('0' return to main menu, '4' exit) processes cleanly
  //   - No invalid patient or appointment record is persisted in CSV files
  //   - Zero crashes
  // =========================================================================
  std::cout << "\n--- Testing Category E: Field Validation Boundaries (Cases 17-20) ---\n";

  const std::string expectedAgeError = "Age: must be a whole number from 0 to 120.";
  const std::string expectedDateError = "Appointment date: day is invalid for the specified month and year.";
  const std::string expectedTimeError = "Appointment time: hour must be between 00 and 23.";

  // Case 17: Non-numeric patient age ("abc")
  // Flow: 1 (Patient Menu) -> 1 (Register) -> P099 -> Test Patient -> abc (Age) ->
  //       re-prompts Patient Menu -> 0 (Return) -> 4 (Exit)
  {
    const std::string input = "1\n1\nP099\nTest Patient\nabc\n0\n4\n";
    const RunResult res = executeInteractiveSession(dataDir, input);
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, expectedAgeError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(17, "Patient registration non-numeric age ('abc')", pass,
                     res.crashed ? res.crashReason : "Error not displayed or incorrect submenu recovery");
  }

  // Case 18: Out-of-range patient age ("999")
  // Flow: 1 (Patient Menu) -> 1 (Register) -> P099 -> Test Patient -> 999 (Age) ->
  //       re-prompts Patient Menu -> 0 (Return) -> 4 (Exit)
  {
    const std::string input = "1\n1\nP099\nTest Patient\n999\n0\n4\n";
    const RunResult res = executeInteractiveSession(dataDir, input);
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, expectedAgeError) == 1 &&
                      countOccurrences(res.output, patientMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(18, "Patient registration out-of-range age ('999')", pass,
                     res.crashed ? res.crashReason : "Error not displayed or incorrect submenu recovery");
  }

  // Case 19: Invalid appointment date ("2026-02-30" — 30 Feb does not exist)
  // Flow: 3 (Appointment Menu) -> 1 (Book) -> P001 -> D001 -> 2026-02-30 -> 10:00 ->
  //       re-prompts Appointment Menu -> 0 (Return) -> 4 (Exit)
  {
    const std::string input = "3\n1\nP001\nD001\n2026-02-30\n10:00\n0\n4\n";
    const RunResult res = executeInteractiveSession(dataDir, input);
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, expectedDateError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(19, "Appointment booking invalid date ('2026-02-30')", pass,
                     res.crashed ? res.crashReason : "Error not displayed or incorrect submenu recovery");
  }

  // Case 20: Invalid appointment time ("25:99" — 25h 99m invalid 24-hr clock)
  // Flow: 3 (Appointment Menu) -> 1 (Book) -> P001 -> D001 -> 2026-11-10 -> 25:99 ->
  //       re-prompts Appointment Menu -> 0 (Return) -> 4 (Exit)
  {
    const std::string input = "3\n1\nP001\nD001\n2026-11-10\n25:99\n0\n4\n";
    const RunResult res = executeInteractiveSession(dataDir, input);
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, expectedTimeError) == 1 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 2 &&
                      countOccurrences(res.output, mainMenuHeader) == 2 &&
                      countOccurrences(res.output, patientMenuHeader) == 0 &&
                      countOccurrences(res.output, doctorMenuHeader) == 0 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    reportCaseResult(20, "Appointment booking invalid time ('25:99')", pass,
                     res.crashed ? res.crashReason : "Error not displayed or incorrect submenu recovery");
  }

  // =========================================================================
  // Supplementary Robustness Scenario 1: Combined multi-menu invalid flow
  // Exercises all 4 menus sequentially in a single process execution
  // =========================================================================
  std::cout << "\n--- Supplementary Robustness Scenario 1: Continuous Multi-Menu Sequence ---\n";
  {
    const std::string combinedInput =
        "9\nabc\n-1\n\n1\n"   // Main menu: 4 invalid inputs, then 1 (Patient)
        "9\nabc\n-1\n\n0\n"   // Patient menu: 4 invalid inputs, then 0 (Return)
        "2\n"                 // Main menu: 2 (Doctor)
        "9\nabc\n-1\n\n0\n"   // Doctor menu: 4 invalid inputs, then 0 (Return)
        "3\n"                 // Main menu: 3 (Appointment)
        "9\nabc\n-1\n\n0\n"   // Appointment menu: 4 invalid inputs, then 0 (Return)
        "4\n";                // Main menu: 4 (Exit)

    const RunResult res = executeInteractiveSession(dataDir, combinedInput);
    const bool pass = !res.crashed &&
                      countOccurrences(res.output, mainMenuError) == 4 &&
                      countOccurrences(res.output, submenuError) == 12 &&
                      countOccurrences(res.output, mainMenuHeader) == 8 &&
                      countOccurrences(res.output, patientMenuHeader) == 5 &&
                      countOccurrences(res.output, doctorMenuHeader) == 5 &&
                      countOccurrences(res.output, appointmentMenuHeader) == 5 &&
                      countOccurrences(res.output, choicePrompt) == 23 &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    if (pass) {
      std::cout << "  [PASS] Supplementary: Continuous multi-menu invalid stream handled without crash\n";
    } else {
      std::cout << "  [FAIL] Supplementary: Continuous multi-menu invalid stream failed\n";
      ++g_casesFailed;
    }
  }

  // =========================================================================
  // Supplementary Robustness Scenario 2: Boundary & Extreme Input Stress
  // Tests huge input line, format strings, whitespace-only line, and safe EOF
  // =========================================================================
  std::cout << "\n--- Supplementary Robustness Scenario 2: Extreme & Boundary Stress Vectors ---\n";
  {
    // Extremely long line (1000 'A's) followed by format string "%s%x%n"
    std::string longInput(1000, 'A');
    std::string stressInput = longInput + "\n%s%x%n%d\n   \t  \n4\n";
    const RunResult res = executeInteractiveSession(dataDir, stressInput);
    const bool pass = !res.crashed &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    if (pass) {
      std::cout << "  [PASS] Supplementary: Extreme string length and format characters handled safely\n";
    } else {
      std::cout << "  [FAIL] Supplementary: Extreme string length or format characters caused failure\n";
      ++g_casesFailed;
    }
  }

  // Safe EOF handling: immediate EOF at startup without infinite loop or crash
  {
    const RunResult res = executeInteractiveSession(dataDir, "");
    const bool pass = !res.crashed &&
                      res.output.find(exitMessage) != std::string::npos &&
                      checkDataIntegrity();
    if (pass) {
      std::cout << "  [PASS] Supplementary: Immediate EOF at main prompt terminates cleanly\n";
    } else {
      std::cout << "  [FAIL] Supplementary: Immediate EOF did not terminate cleanly\n";
      ++g_casesFailed;
    }
  }

  // Final data integrity verification
  std::cout << "\n--- Verifying CSV Persistence Integrity Across All Scenarios ---\n";
  if (checkDataIntegrity()) {
    std::cout << "  [PASS] CSV Data Integrity: patients.csv, doctors.csv, and appointments.csv unmodified\n";
  } else {
    std::cout << "  [FAIL] CSV Data Integrity: files modified or corrupted during invalid input testing\n";
    ++g_casesFailed;
  }

  // Clean up isolated test data
  std::filesystem::remove_all(dataDir, ec);

  // =========================================================================
  // Final Evaluation and Summary
  // =========================================================================
  std::cout << "\n======================================================================\n"
            << "TC-NFR-01 Summary:\n"
            << "  Target Test Cases: 20\n"
            << "  Passed Cases:      " << g_casesPassed << " / 20\n"
            << "  Failed Cases:      " << g_casesFailed << "\n"
            << "  Total Crashes:     " << g_crashCount << " (Acceptance Criterion: 0 crashes)\n"
            << "======================================================================\n";

  const bool success = (g_casesPassed >= 20) && (g_casesFailed == 0) && (g_crashCount == 0);
  return success ? 0 : 1;
}
