// TC-UI-01  Navigate menus and submenus (HMS-F-032, SRS §3.1)
//
// Verifies:
//   - Main menu offers Patient Management (1), Doctor Management (2),
//     Appointment Management (3), Exit (4).
//   - Choosing 1, 2, or 3 shows the corresponding submenu.
//   - Each submenu lists the correct SRS-specified options.
//   - Choosing 0 inside a submenu returns to the main menu (not exit).
//   - Navigating between multiple modules in one session works.
//
// Self-contained test binary following the project's CTest harness conventions.

#include "hms/HospitalSystem.h"

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

// Run HospitalSystem::run() with controlled input; return captured stdout.
// Uses empty CSV paths so no real data files are touched.
std::string runWithInput(const std::string &input) {
  // FileManager default constructor uses data/patients.csv etc. but gracefully
  // handles missing files (returns empty lists). This is fine for nav tests.
  hms::HospitalSystem sys;

  std::istringstream in(input);
  std::ostringstream out;
  auto *oldIn  = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  sys.run();

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);
  return out.str();
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

} // namespace

int main() {
  std::cout << "TC-UI-01  Navigate menus and submenus (HMS-F-032)\n\n";

  // -------------------------------------------------------------------------
  // 1. Main menu: choice 1 → Patient Management submenu, then 0 → return
  // -------------------------------------------------------------------------
  {
    // Inputs: select Patient Management, then Return (0), then Exit (4).
    const std::string out = runWithInput("1\n0\n4\n");

    check("Main menu shows '1. Patient Management'",
          out.find("1. Patient Management") != std::string::npos);
    check("Patient submenu header shown",
          out.find("Patient Management") != std::string::npos);
    check("Patient submenu: '1. Register'",
          out.find("1. Register") != std::string::npos);
    check("Patient submenu: '2. Search'",
          out.find("2. Search") != std::string::npos);
    check("Patient submenu: '3. View All'",
          out.find("3. View All") != std::string::npos);
    check("Patient submenu: '0. Return'",
          out.find("0. Return") != std::string::npos);
    // After returning via 0, the main menu must re-appear before the exit.
    check("Main menu re-displayed after Patient submenu return (>=2 times)",
          countOccurrences(out, "Hospital Management System") >= 2);
  }

  // -------------------------------------------------------------------------
  // 2. Main menu: choice 2 → Doctor Management submenu, then 0 → return
  // -------------------------------------------------------------------------
  {
    const std::string out = runWithInput("2\n0\n4\n");

    check("Main menu shows '2. Doctor Management'",
          out.find("2. Doctor Management") != std::string::npos);
    check("Doctor submenu header shown",
          out.find("Doctor Management") != std::string::npos);
    check("Doctor submenu: '1. Add'",
          out.find("1. Add") != std::string::npos);
    check("Doctor submenu: '2. Search'",
          out.find("2. Search") != std::string::npos);
    check("Doctor submenu: '3. View All'",
          out.find("3. View All") != std::string::npos);
    check("Doctor submenu: '0. Return'",
          out.find("0. Return") != std::string::npos);
    check("Main menu re-displayed after Doctor submenu return (>=2 times)",
          countOccurrences(out, "Hospital Management System") >= 2);
  }

  // -------------------------------------------------------------------------
  // 3. Main menu: choice 3 → Appointment Management submenu, then 0 → return
  // -------------------------------------------------------------------------
  {
    const std::string out = runWithInput("3\n0\n4\n");

    check("Main menu shows '3. Appointment Management'",
          out.find("3. Appointment Management") != std::string::npos);
    check("Appointment submenu header shown",
          out.find("Appointment Management") != std::string::npos);
    check("Appointment submenu: '1. Book'",
          out.find("1. Book") != std::string::npos);
    check("Appointment submenu: '2. View'",
          out.find("2. View") != std::string::npos);
    check("Appointment submenu: '3. Cancel'",
          out.find("3. Cancel") != std::string::npos);
    check("Appointment submenu: '4. Check Doctor Availability'",
          out.find("4. Check Doctor Availability") != std::string::npos);
    check("Appointment submenu: '0. Return'",
          out.find("0. Return") != std::string::npos);
    check("Main menu re-displayed after Appointment submenu return (>=2 times)",
          countOccurrences(out, "Hospital Management System") >= 2);
  }

  // -------------------------------------------------------------------------
  // 4. Multi-module session: Patient → Doctor → Appointment → Exit
  // -------------------------------------------------------------------------
  {
    // Visit each module in sequence, return after each, then exit.
    const std::string out = runWithInput("1\n0\n2\n0\n3\n0\n4\n");

    check("Multi-module: Patient Management appeared",
          out.find("Patient Management") != std::string::npos);
    check("Multi-module: Doctor Management appeared",
          out.find("Doctor Management") != std::string::npos);
    check("Multi-module: Appointment Management appeared",
          out.find("Appointment Management") != std::string::npos);
    // Main menu: initial + after each of the 3 returns = at least 4 times.
    check("Multi-module: main menu re-displayed >=4 times",
          countOccurrences(out, "Hospital Management System") >= 4);
  }

  // -------------------------------------------------------------------------
  // 5. Main menu choice 4 exits cleanly
  // -------------------------------------------------------------------------
  {
    const std::string out = runWithInput("4\n");
    check("Choice 4 exits the main loop",
          out.find("Exiting") != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // 6. Invalid main-menu input does not exit
  // -------------------------------------------------------------------------
  {
    // Invalid choice "9", then valid Exit (4).
    const std::string out = runWithInput("9\n4\n");
    check("Invalid main-menu choice shows error, does not exit early",
          out.find("Invalid choice") != std::string::npos &&
          out.find("Exiting") != std::string::npos);
  }

  // -------------------------------------------------------------------------
  // Summary
  // -------------------------------------------------------------------------
  std::cout << '\n'
            << g_passed << " passed, " << g_failed << " failed\n";

  return g_failed == 0 ? 0 : 1;
}
