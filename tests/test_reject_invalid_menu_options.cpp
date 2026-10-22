// TC-UI-02: Reject invalid menu options (HMS-F-033, SRS §3.1)
//
// Acceptance Criteria:
//   Inputs "9", "abc", "-1", and an empty line at each menu produce
//   a clear error message and redisplay the same menu; application does not terminate.
//
// Verified Menus:
//   1. Main Menu (1: Patient, 2: Doctor, 3: Appointment, 4: Exit)
//   2. Patient Management Submenu (1: Register, 2: Search, 3: View All, 4: Update, 0: Return)
//   3. Doctor Management Submenu (1: Add, 2: Search, 3: View All, 4: Update, 0: Return)
//   4. Appointment Management Submenu (1: Book, 2: View, 3: Cancel, 4: Check Availability, 0: Return)
//
// Test target: hms_reject_invalid_menu_options_tests
// CTest name:  TC-UI-02_RejectInvalidMenuOptions

#include "hms/HospitalSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

// Count non-overlapping occurrences of needle in haystack.
std::size_t countOccurrences(const std::string &haystack,
                             const std::string &needle) {
  std::size_t count = 0;
  std::size_t pos   = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

// Read entire file content into string
std::string readFileContent(const std::filesystem::path &filePath) {
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open()) {
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// Seed isolated CSV files in dataDir
bool seedTestData(const std::filesystem::path &dataDir) {
  hms::FileManager files((dataDir / "patients.csv").string(),
                         (dataDir / "doctors.csv").string(),
                         (dataDir / "appointments.csv").string());
  std::string err;
  const hms::Patient patient("P001", "Anita Sharma", 28, "F", "9876543210",
                             "12 Residency Rd Bengaluru", "None");
  if (!files.savePatient(patient, err)) {
    std::cerr << "Could not seed patient: " << err << '\n';
    return false;
  }

  const hms::Doctor doctor("D001", "Dr. Meera Rao", "Cardiology", "Cardiology",
                           "9876543211", "Mon-Fri 09:00-17:00");
  if (!files.saveDoctor(doctor, err)) {
    std::cerr << "Could not seed doctor: " << err << '\n';
    return false;
  }

  const hms::Appointment appt("A0001", "P001", "D001", "2026-11-10", "10:00",
                             hms::AppointmentStatus::Booked);
  if (!files.saveAppointment(appt, err)) {
    std::cerr << "Could not seed appointment: " << err << '\n';
    return false;
  }

  return true;
}

// Run HospitalSystem::run() with controlled input; return captured stdout.
std::string runWithInput(hms::HospitalSystem &sys, const std::string &input) {
  std::istringstream in(input);
  std::ostringstream out;
  auto *oldIn  = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  sys.run();

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);
  return out.str();
}

} // namespace

int main() {
  std::cout << "TC-UI-02  Reject Invalid Menu Options (HMS-F-033)\n\n";

  // Isolated test data directory placed under build/
  std::filesystem::path dataDir = "TC-UI-02-reject-invalid-menu-data";
  if (std::filesystem::exists("build") && std::filesystem::is_directory("build")) {
    dataDir = std::filesystem::path("build") / "TC-UI-02-reject-invalid-menu-data";
  }

  std::error_code ec;
  std::filesystem::remove_all(dataDir, ec);
  std::filesystem::create_directories(dataDir, ec);
  if (ec) {
    std::cerr << "Could not create isolated test directory: " << ec.message() << '\n';
    return 1;
  }

  if (!seedTestData(dataDir)) {
    std::cerr << "Failed to seed test data in " << dataDir << '\n';
    std::filesystem::remove_all(dataDir, ec);
    return 1;
  }

  const std::string initialPatientsCsv = readFileContent(dataDir / "patients.csv");
  const std::string initialDoctorsCsv = readFileContent(dataDir / "doctors.csv");
  const std::string initialAppointmentsCsv = readFileContent(dataDir / "appointments.csv");

  const std::string mainMenuHeader = "=== Hospital Management System ===";
  const std::string patientMenuHeader = "=== Patient Management ===";
  const std::string doctorMenuHeader = "=== Doctor Management ===";
  const std::string appointmentMenuHeader = "=== Appointment Management ===";
  const std::string prompt = "Enter your choice: ";
  const std::string mainMenuError = "Invalid choice. Enter 1, 2, 3, or 4.";
  const std::string submenuError = "Invalid choice. Enter 0 to 4.";
  const std::string exitMessage = "Exiting Hospital Management System.";

  // -------------------------------------------------------------------------
  // Scenario 1: Main menu rejects invalid options ("9", "abc", "-1", "")
  // -------------------------------------------------------------------------
  std::cout << "[Scenario 1] Main menu invalid options (9, abc, -1, empty line)\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Inputs: "9", "abc", "-1", empty line, then exit "4"
    const std::string input = "9\nabc\n-1\n\n4\n";
    const std::string out = runWithInput(sys, input);

    check("Main menu: 4 invalid error messages displayed",
          countOccurrences(out, mainMenuError) == 4);
    check("Main menu: header displayed 5 times (initial + 4 redisplays)",
          countOccurrences(out, mainMenuHeader) == 5);
    check("Main menu: prompt displayed 5 times",
          countOccurrences(out, prompt) == 5);
    check("Main menu: choice 4 exits cleanly",
          out.find(exitMessage) != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Scenario 2: Patient submenu rejects invalid options ("9", "abc", "-1", "")
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 2] Patient submenu invalid options (9, abc, -1, empty line)\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Inputs: "1" (enter patient menu), "9", "abc", "-1", empty line, "0" (return), "4" (exit)
    const std::string input = "1\n9\nabc\n-1\n\n0\n4\n";
    const std::string out = runWithInput(sys, input);

    check("Patient submenu: 4 submenu error messages displayed",
          countOccurrences(out, submenuError) == 4);
    check("Patient submenu: header displayed 5 times (initial + 4 redisplays)",
          countOccurrences(out, patientMenuHeader) == 5);
    check("Patient submenu: option 0 returns to main menu",
          countOccurrences(out, mainMenuHeader) == 2);
    check("Patient submenu session: exits cleanly via choice 4",
          out.find(exitMessage) != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Scenario 3: Doctor submenu rejects invalid options ("9", "abc", "-1", "")
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 3] Doctor submenu invalid options (9, abc, -1, empty line)\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Inputs: "2" (enter doctor menu), "9", "abc", "-1", empty line, "0" (return), "4" (exit)
    const std::string input = "2\n9\nabc\n-1\n\n0\n4\n";
    const std::string out = runWithInput(sys, input);

    check("Doctor submenu: 4 submenu error messages displayed",
          countOccurrences(out, submenuError) == 4);
    check("Doctor submenu: header displayed 5 times (initial + 4 redisplays)",
          countOccurrences(out, doctorMenuHeader) == 5);
    check("Doctor submenu: option 0 returns to main menu",
          countOccurrences(out, mainMenuHeader) == 2);
    check("Doctor submenu session: exits cleanly via choice 4",
          out.find(exitMessage) != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Scenario 4: Appointment submenu rejects invalid options ("9", "abc", "-1", "")
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 4] Appointment submenu invalid options (9, abc, -1, empty line)\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Inputs: "3" (enter appointment menu), "9", "abc", "-1", empty line, "0" (return), "4" (exit)
    const std::string input = "3\n9\nabc\n-1\n\n0\n4\n";
    const std::string out = runWithInput(sys, input);

    check("Appointment submenu: 4 submenu error messages displayed",
          countOccurrences(out, submenuError) == 4);
    check("Appointment submenu: header displayed 5 times (initial + 4 redisplays)",
          countOccurrences(out, appointmentMenuHeader) == 5);
    check("Appointment submenu: option 0 returns to main menu",
          countOccurrences(out, mainMenuHeader) == 2);
    check("Appointment submenu session: exits cleanly via choice 4",
          out.find(exitMessage) != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Scenario 5: Combined navigation flow exercising all 4 menus and 16 invalid inputs
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 5] Combined flow exercising all 4 menus and 16 invalid inputs\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Sequence:
    // Main menu: 9, abc, -1, "", then 1 (Patient)
    // Patient menu: 9, abc, -1, "", then 0 (Return)
    // Main menu: 2 (Doctor)
    // Doctor menu: 9, abc, -1, "", then 0 (Return)
    // Main menu: 3 (Appointment)
    // Appointment menu: 9, abc, -1, "", then 0 (Return)
    // Main menu: 4 (Exit)
    const std::string input =
        "9\nabc\n-1\n\n1\n"
        "9\nabc\n-1\n\n0\n"
        "2\n"
        "9\nabc\n-1\n\n0\n"
        "3\n"
        "9\nabc\n-1\n\n0\n"
        "4\n";

    const std::string out = runWithInput(sys, input);

    // Assert all 16 invalid choices produce their menu-specific error messages
    const std::size_t mainErrors = countOccurrences(out, mainMenuError);
    const std::size_t subErrors = countOccurrences(out, submenuError);
    check("Combined: Main menu error message appears exactly 4 times",
          mainErrors == 4);
    check("Combined: Submenu error message appears exactly 12 times (4x each for Patient, Doctor, Appointment)",
          subErrors == 12);
    check("Combined: All 16 invalid choices produced error messages (4 main + 12 submenu)",
          (mainErrors + subErrors) == 16);

    // Assert menu headers appear expected number of times:
    // Main menu: initial (1) + 4 invalid (4) + 3 returns (3) = 8
    check("Combined: Main menu header displayed exactly 8 times",
          countOccurrences(out, mainMenuHeader) == 8);
    // Patient menu: initial (1) + 4 invalid (4) = 5
    check("Combined: Patient menu header displayed exactly 5 times",
          countOccurrences(out, patientMenuHeader) == 5);
    // Doctor menu: initial (1) + 4 invalid (4) = 5
    check("Combined: Doctor menu header displayed exactly 5 times",
          countOccurrences(out, doctorMenuHeader) == 5);
    // Appointment menu: initial (1) + 4 invalid (4) = 5
    check("Combined: Appointment menu header displayed exactly 5 times",
          countOccurrences(out, appointmentMenuHeader) == 5);

    // Total prompt occurrences: 8 + 5 + 5 + 5 = 23
    check("Combined: Total 'Enter your choice:' prompts displayed exactly 23 times",
          countOccurrences(out, prompt) == 23);

    // Valid exit
    check("Combined: Final exit choice 4 terminates normally",
          out.find(exitMessage) != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Scenario 6: Individual invalid input validation (granular error diagnosis)
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 6] Granular individual invalid input checks\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Out-of-range positive
    std::string out = runWithInput(sys, "9\n4\n");
    check("Granular Main: '9' triggers main error message once and redisplays",
          countOccurrences(out, mainMenuError) == 1 &&
          countOccurrences(out, mainMenuHeader) == 2);

    // Non-numeric
    out = runWithInput(sys, "abc\n4\n");
    check("Granular Main: 'abc' triggers main error message once and redisplays",
          countOccurrences(out, mainMenuError) == 1 &&
          countOccurrences(out, mainMenuHeader) == 2);

    // Negative out-of-range
    out = runWithInput(sys, "-1\n4\n");
    check("Granular Main: '-1' triggers main error message once and redisplays",
          countOccurrences(out, mainMenuError) == 1 &&
          countOccurrences(out, mainMenuHeader) == 2);

    // Empty line
    out = runWithInput(sys, "\n4\n");
    check("Granular Main: empty line triggers main error message once and redisplays",
          countOccurrences(out, mainMenuError) == 1 &&
          countOccurrences(out, mainMenuHeader) == 2);
  }

  // -------------------------------------------------------------------------
  // Scenario 7: Data integrity check — verify no records created, updated, or modified
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 7] Data integrity verification\n";
  {
    hms::FileManager files((dataDir / "patients.csv").string(),
                           (dataDir / "doctors.csv").string(),
                           (dataDir / "appointments.csv").string());

    const auto patients = files.loadPatients();
    const auto doctors = files.loadDoctors();
    const auto appts = files.loadAppointments();

    check("Data integrity: patient count unchanged (1)", patients.size() == 1);
    check("Data integrity: doctor count unchanged (1)", doctors.size() == 1);
    check("Data integrity: appointment count unchanged (1)", appts.size() == 1);

    const std::string finalPatientsCsv = readFileContent(dataDir / "patients.csv");
    const std::string finalDoctorsCsv = readFileContent(dataDir / "doctors.csv");
    const std::string finalAppointmentsCsv = readFileContent(dataDir / "appointments.csv");

    check("Data integrity: patients.csv bytes unchanged",
          finalPatientsCsv == initialPatientsCsv);
    check("Data integrity: doctors.csv bytes unchanged",
          finalDoctorsCsv == initialDoctorsCsv);
    check("Data integrity: appointments.csv bytes unchanged",
          finalAppointmentsCsv == initialAppointmentsCsv);
  }

  // -------------------------------------------------------------------------
  // Scenario 8: Safe EOF handling without infinite loop
  // -------------------------------------------------------------------------
  std::cout << "\n[Scenario 8] Safe EOF handling without infinite loop\n";
  {
    hms::HospitalSystem sys(dataDir.string());
    // Immediate EOF at main menu
    const std::string outMainEOF = runWithInput(sys, "");
    check("EOF at main menu terminates safely with exit message",
          outMainEOF.find(exitMessage) != std::string::npos);

    // EOF inside submenu
    const std::string outSubEOF = runWithInput(sys, "1\n");
    check("EOF inside Patient submenu terminates safely",
          outSubEOF.find(exitMessage) != std::string::npos);
  }

  // Clean up isolated test data
  std::filesystem::remove_all(dataDir, ec);

  // -------------------------------------------------------------------------
  // Summary
  // -------------------------------------------------------------------------
  std::cout << "\n========================================\n"
            << "TC-UI-02 Summary: " << g_passed << " passed, " << g_failed << " failed\n"
            << "========================================\n";

  return g_failed == 0 ? 0 : 1;
}
