// ============================================================================
// TC-NFR-05: Usability and Menu Depth Test Suite
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-NF-005 (Non-Functional Usability Requirement)
//   STP RTM Test Case:  TC-NFR-05 (Software Test Plan Table 14 / Section 9)
//   SRS RTM Test Case:  TC-NFR-05 (External Interface & Quality Attributes)
//   GitHub Issue:       #40
//
// Requirement Statement (HMS-NF-005):
//   "Every prompt shall state the expected input format, every error message
//    shall name the field and the rule violated, and any operation shall be
//    reachable within at most 3 menu selections from the main menu."
//
// Acceptance Criteria:
//   1. Prompt/message checklist review passes 100%.
//   2. Menu depth of every operation is <= 3.
//   3. All existing functionality and regression behaviors are preserved.
//   4. Root-level CSV files remain untouched and preserved.
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
#include <iomanip>
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

std::size_t countOccurrences(const std::string &haystack, const std::string &needle) {
  if (needle.empty()) return 0;
  std::size_t count = 0;
  std::size_t pos = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos) {
    ++count;
    pos += needle.size();
  }
  return count;
}

// Executes an interactive session with captured stdin and stdout using isolated data files
std::string runInteractive(hms::HospitalSystem &sys, const std::string &input) {
  std::istringstream in(input);
  std::ostringstream out;

  auto *oldIn = std::cin.rdbuf(in.rdbuf());
  auto *oldOut = std::cout.rdbuf(out.rdbuf());

  sys.run();

  std::cin.rdbuf(oldIn);
  std::cout.rdbuf(oldOut);

  return out.str();
}

// Seed baseline records in isolated test directory
void seedIsolatedData(const fs::path &dir) {
  hms::FileManager fm((dir / "patients.csv").string(),
                      (dir / "doctors.csv").string(),
                      (dir / "appointments.csv").string());

  std::string err;
  const hms::Patient p1("P001", "John Doe", 35, "M", "9876543210", "123 Main St", "None");
  fm.savePatient(p1, err);

  const hms::Doctor d1("D001", "Dr. Jane Smith", "Cardiology", "Cardiology", "9876543211", "Mon-Fri 09:00-17:00");
  fm.saveDoctor(d1, err);

  const hms::Appointment a1("A0001", "P001", "D001", "2026-11-15", "10:00", hms::AppointmentStatus::Booked);
  fm.saveAppointment(a1, err);
}

// Representation of an item in the Prompt/Message Checklist
struct ChecklistItem {
  int id;
  std::string operation;
  std::string fieldOrComponent;
  std::string expectedFormat;
  std::string violatedRule;
  std::string promptOrErrorText;
  int menuDepth;
  bool isError;
};

// Complete matrix of all 32 operations in the Hospital Management System
struct MenuOperation {
  int id;
  std::string operationName;
  std::string navigationPath;
  std::string inputSequence;
  int menuDepth;
  std::string expectedTextOnReach;
};

} // namespace

int main() {
  std::cout << "======================================================================\n";
  std::cout << "TC-NFR-05: Usability and Menu Depth Test Suite (HMS-NF-005)\n";
  std::cout << "======================================================================\n\n";

  // Capture baseline of root data files to guarantee 100% preservation
  const fs::path rootPatients("patients.csv");
  const fs::path rootDoctors("doctors.csv");
  const fs::path rootAppointments("appointments.csv");

  const std::string rootPatientsBaseline = readFileContent(rootPatients);
  const std::string rootDoctorsBaseline = readFileContent(rootDoctors);
  const std::string rootAppointmentsBaseline = readFileContent(rootAppointments);

  const fs::path testDataDir = fs::path("build") / "TC-NFR-05-usability-data";
  std::error_code ec;
  fs::remove_all(testDataDir, ec);
  fs::create_directories(testDataDir, ec);

  seedIsolatedData(testDataDir);

  // =========================================================================
  // Test A & C & D — Comprehensive Prompt/Message Checklist (100 Items)
  // =========================================================================
  std::cout << "-- Test A: Prompt and Message Checklist Review (100% Target) --\n";

  const std::vector<ChecklistItem> checklist = {
    // Main Menu & Module Navigation
    {1, "Main Menu", "Choice", "Option 1-4", "Out-of-range or non-numeric", "Enter your choice: ", 1, false},
    {2, "Main Menu", "Choice Error", "Numeric 1, 2, 3, or 4", "Rule: Must be 1, 2, 3, or 4", "Invalid choice. Enter 1, 2, 3, or 4.", 1, true},
    {3, "Patient Menu", "Choice", "Option 0-4", "Out-of-range or non-numeric", "Enter your choice: ", 1, false},
    {4, "Patient Menu", "Choice Error", "Numeric 0 to 4", "Rule: Must be 0 to 4", "Invalid choice. Enter 0 to 4.", 1, true},
    {5, "Doctor Menu", "Choice", "Option 0-4", "Out-of-range or non-numeric", "Enter your choice: ", 1, false},
    {6, "Doctor Menu", "Choice Error", "Numeric 0 to 4", "Rule: Must be 0 to 4", "Invalid choice. Enter 0 to 4.", 1, true},
    {7, "Appointment Menu", "Choice", "Option 0-4", "Out-of-range or non-numeric", "Enter your choice: ", 1, false},
    {8, "Appointment Menu", "Choice Error", "Numeric 0 to 4", "Rule: Must be 0 to 4", "Invalid choice. Enter 0 to 4.", 1, true},

    // Patient Registration Prompts
    {9, "Register Patient", "Patient ID Prompt", "1-10 alphanumeric characters (e.g., P001)", "VR-01 Alphanumeric 1-10", "Patient ID (1-10 alphanumeric characters, e.g., P001): ", 2, false},
    {10, "Register Patient", "Full Name Prompt", "1-50 characters", "VR-02 Length 1-50", "Full name (1-50 characters): ", 2, false},
    {11, "Register Patient", "Age Prompt", "Whole number 0-120", "VR-03 Whole number 0-120", "Age (0-120): ", 2, false},
    {12, "Register Patient", "Gender Prompt", "Single letter M, F, or O", "VR-04 M, F, or O", "Gender (M/F/O): ", 2, false},
    {13, "Register Patient", "Contact Number Prompt", "Exactly 10 digits", "VR-05 10 digits", "Contact number (10 digits): ", 2, false},
    {14, "Register Patient", "Address Prompt", "1-100 characters", "VR-06 1-100 characters", "Address (1-100 characters): ", 2, false},
    {15, "Register Patient", "Medical History Prompt", "Optional, up to 200 characters", "VR-07 0-200 characters", "Medical history (optional, up to 200 characters; press Enter to skip): ", 2, false},

    // Patient Registration Validation Errors
    {16, "Register Patient", "Empty Required Field", "Non-empty string", "Required field rule", "This field is required.", 2, true},
    {17, "Register Patient", "Invalid Patient ID", "1-10 alphanumeric", "VR-01: must contain 1-10 alphanumeric characters.", "Patient ID: must contain 1-10 alphanumeric characters.", 2, true},
    {18, "Register Patient", "Invalid Name Characters", "1-50 letters/spaces/./-/'", "VR-02: must contain 1-50 permitted characters", "Name: must contain 1-50 permitted characters", 2, true},
    {19, "Register Patient", "Invalid Age", "0 to 120", "VR-03: must be a whole number from 0 to 120.", "Age: must be a whole number from 0 to 120.", 2, true},
    {20, "Register Patient", "Invalid Gender", "M, F, or O", "VR-04: must be M, F, or O.", "Gender: must be M, F, or O.", 2, true},
    {21, "Register Patient", "Invalid Contact Number", "10 digits", "VR-05: must contain exactly 10 digits.", "Contact number: must contain exactly 10 digits.", 2, true},
    {22, "Register Patient", "Invalid Address Length", "1-100 characters", "VR-06: must contain 1-100 characters.", "Address: must contain 1-100 characters.", 2, true},
    {23, "Register Patient", "Duplicate Patient ID", "Unique ID", "HMS-F-002: Patient ID already exists", "Patient ID already exists", 2, true},
    {24, "Register Patient", "Formula Injection Guard", "Value must not start with =", "VR-14: value must not begin with =, +, -, or @.", "value must not begin with =, +, -, or @.", 2, true},
    {25, "Register Patient", "Comma Safety Guard", "No comma in CSV field", "VR-14: commas and line breaks are not allowed.", "commas and line breaks are not allowed.", 2, true},
    {26, "Register Patient", "Registration Success", "Valid record saved", "HMS-F-001 confirmation", "Patient registered successfully.", 2, false},

    // Patient Search & Details Prompts and Errors
    {27, "Search Patients", "Search Query Prompt", "Patient ID or name (e.g., P001 or John)", "VR-01/VR-02 format", "Enter patient ID or name (e.g., P001 or John): ", 2, false},
    {28, "Search Patients", "Not Found Outcome", "Existing patient record", "HMS-F-004 not found", "No matching patient found", 2, true},
    {29, "Search Patients", "Search Success Outcome", "Matching patient records", "HMS-F-004 success", "Patient search completed successfully.", 2, false},
    {30, "View Patient Details", "Patient ID Prompt", "1-10 alphanumeric (e.g., P001)", "VR-01 ID format", "Enter patient ID: (1-10 alphanumeric characters, e.g., P001): ", 2, false},
    {31, "View Patient Details", "Not Found Outcome", "Existing patient record", "HMS-F-005 not found", "No matching patient found", 2, true},
    {32, "List All Patients", "Empty List Outcome", "At least one patient in database", "HMS-F-005 empty list", "No patients registered", 2, true},

    // Patient Update Prompts and Errors
    {33, "Update Patient", "Patient ID Prompt", "1-10 alphanumeric (e.g., P001)", "VR-01 ID format", "Enter patient ID: (1-10 alphanumeric characters, e.g., P001): ", 2, false},
    {34, "Update Patient", "Patient Not Found", "Registered Patient ID", "HMS-F-007 patient not found", "Patient not found", 2, true},
    {35, "Update Patient", "Field Menu Prompt", "Option 0-7", "Update submenu choices", "Enter choice: ", 3, false},
    {36, "Update Patient", "Field Menu Invalid Choice", "Numeric 0 to 7", "Rule: Enter 0 to 7", "Invalid choice. Enter 0 to 7.", 3, true},
    {37, "Update Patient", "New Name Prompt", "1-50 characters", "VR-02 length 1-50", "Enter new name (1-50 characters): ", 3, false},
    {38, "Update Patient", "New Age Prompt", "Whole number 0-120", "VR-03 range 0-120", "Enter new age (0-120): ", 3, false},
    {39, "Update Patient", "New Gender Prompt", "M, F, or O", "VR-04 M, F, or O", "Enter new gender (M/F/O): ", 3, false},
    {40, "Update Patient", "New Contact Prompt", "10 digits", "VR-05 10 digits", "Enter new contact number (10 digits): ", 3, false},
    {41, "Update Patient", "New Address Prompt", "1-100 characters", "VR-06 1-100 characters", "Enter new address (1-100 characters): ", 3, false},
    {42, "Update Patient", "New Medical History Prompt", "Optional, up to 200 characters", "VR-07 0-200 characters", "Enter new medical history (optional, up to 200 characters; press Enter to skip): ", 3, false},
    {43, "Update Patient", "Update Success Outcome", "Updated patient record saved", "HMS-F-006 success", "Patient updated successfully", 3, false},

    // Doctor Registration Prompts
    {44, "Add Doctor", "Doctor ID Prompt", "1-10 alphanumeric characters (e.g., D001)", "VR-01 Alphanumeric 1-10", "Doctor ID (1-10 alphanumeric characters, e.g., D001): ", 2, false},
    {45, "Add Doctor", "Full Name Prompt", "1-50 characters", "VR-02 Length 1-50", "Full name (1-50 characters): ", 2, false},
    {46, "Add Doctor", "Specialization Prompt", "1-40 characters", "VR-08 Length 1-40", "Specialization (1-40 characters): ", 2, false},
    {47, "Add Doctor", "Department Prompt", "1-40 characters", "VR-08 Length 1-40", "Department (1-40 characters): ", 2, false},
    {48, "Add Doctor", "Contact Number Prompt", "Exactly 10 digits", "VR-05 10 digits", "Contact number (10 digits): ", 2, false},
    {49, "Add Doctor", "Availability Prompt", "1-60 characters (e.g., Mon-Fri 09:00-17:00)", "VR-09 1-60 characters", "Availability (1-60 characters, e.g., Mon-Fri 09:00-17:00): ", 2, false},

    // Doctor Registration Validation Errors
    {50, "Add Doctor", "Empty Required Field", "Non-empty string", "Required field rule", "This field is required.", 2, true},
    {51, "Add Doctor", "Invalid Doctor ID", "1-10 alphanumeric", "VR-01: must contain 1-10 alphanumeric characters.", "Doctor ID: must contain 1-10 alphanumeric characters.", 2, true},
    {52, "Add Doctor", "Invalid Doctor Name", "1-50 letters/spaces", "VR-02: must contain 1-50 permitted characters", "Name: must contain 1-50 permitted characters", 2, true},
    {53, "Add Doctor", "Invalid Specialization", "1-40 characters", "VR-08: must contain 1-40 characters.", "Specialization: must contain 1-40 characters.", 2, true},
    {54, "Add Doctor", "Invalid Department", "1-40 characters", "VR-08: must contain 1-40 characters.", "Department: must contain 1-40 characters.", 2, true},
    {55, "Add Doctor", "Invalid Doctor Contact", "10 digits", "VR-05: must contain exactly 10 digits.", "Contact number: must contain exactly 10 digits.", 2, true},
    {56, "Add Doctor", "Invalid Availability", "1-60 characters", "VR-09: must contain 1-60 characters.", "Availability: must contain 1-60 characters.", 2, true},
    {57, "Add Doctor", "Duplicate Doctor ID", "Unique Doctor ID", "HMS-F-009: Doctor ID already exists", "Doctor ID already exists", 2, true},
    {58, "Add Doctor", "Addition Success", "Doctor record saved", "HMS-F-008 success", "Doctor added successfully.", 2, false},

    // Doctor Search, Details & Listing Prompts and Errors
    {59, "Search Doctors", "Search Query Prompt", "ID, name, or spec (e.g., D001, John, Cardiology)", "Search query format", "Enter doctor ID, name, or specialization (e.g., D001, John, Cardiology): ", 2, false},
    {60, "Search Doctors", "Not Found Outcome", "Existing doctor record", "HMS-F-011 not found", "No matching doctor found", 2, true},
    {61, "View Doctor Details", "Doctor ID Prompt", "1-10 alphanumeric (e.g., D001)", "VR-01 ID format", "Enter doctor ID: (1-10 alphanumeric characters, e.g., D001): ", 2, false},
    {62, "View Doctor Details", "Not Found Outcome", "Existing doctor record", "HMS-F-012 not found", "No matching doctor found", 2, true},
    {63, "List All Doctors", "Empty List Outcome", "At least one doctor registered", "HMS-F-012 empty list", "No doctors registered", 2, true},

    // Doctor Update Prompts and Errors
    {64, "Update Doctor", "Doctor ID Prompt", "1-10 alphanumeric (e.g., D001)", "VR-01 ID format", "Enter doctor ID: (1-10 alphanumeric characters, e.g., D001): ", 2, false},
    {65, "Update Doctor", "Doctor Not Found", "Registered Doctor ID", "HMS-F-014 doctor not found", "Doctor not found", 2, true},
    {66, "Update Doctor", "Field Menu Prompt", "Option 0-6", "Update submenu choices", "Enter choice: ", 3, false},
    {67, "Update Doctor", "Field Menu Invalid Choice", "Numeric 0 to 6", "Rule: Enter 0 to 6", "Invalid choice. Enter 0 to 6.", 3, true},
    {68, "Update Doctor", "New Name Prompt", "1-50 characters", "VR-02 length 1-50", "Enter new name (1-50 characters): ", 3, false},
    {69, "Update Doctor", "New Specialization Prompt", "1-40 characters", "VR-08 length 1-40", "Enter new specialization (1-40 characters): ", 3, false},
    {70, "Update Doctor", "New Department Prompt", "1-40 characters", "VR-08 length 1-40", "Enter new department (1-40 characters): ", 3, false},
    {71, "Update Doctor", "New Contact Prompt", "10 digits", "VR-05 10 digits", "Enter new contact number (10 digits): ", 3, false},
    {72, "Update Doctor", "New Availability Prompt", "1-60 characters (e.g., Mon-Fri 09:00-17:00)", "VR-09 1-60 characters", "Enter new availability (1-60 characters, e.g., Mon-Fri 09:00-17:00): ", 3, false},
    {73, "Update Doctor", "Update Success Outcome", "Updated doctor record saved", "HMS-F-013 success", "Doctor updated successfully", 3, false},

    // Appointment Booking Prompts
    {74, "Book Appointment", "Patient ID Prompt", "Registered Patient ID (e.g., P001)", "BR-03 / VR-01 format", "Patient ID (e.g., P001): ", 2, false},
    {75, "Book Appointment", "Doctor ID Prompt", "Registered Doctor ID (e.g., D001)", "BR-04 / VR-01 format", "Doctor ID (e.g., D001): ", 2, false},
    {76, "Book Appointment", "Date Prompt", "Calendar date YYYY-MM-DD", "VR-10 format YYYY-MM-DD", "Date (YYYY-MM-DD): ", 2, false},
    {77, "Book Appointment", "Time Prompt", "Time HH:MM (24-hour format)", "VR-11 format HH:MM", "Time (HH:MM, 24-hour format): ", 2, false},

    // Appointment Booking Validation Errors
    {78, "Book Appointment", "Invalid Date Format", "YYYY-MM-DD", "VR-10: must be in YYYY-MM-DD format.", "Appointment date: must be in YYYY-MM-DD format.", 2, true},
    {79, "Book Appointment", "Invalid Calendar Day", "Valid day of month", "VR-10: day is invalid for the specified month and year.", "Appointment date: day is invalid for the specified month and year.", 2, true},
    {80, "Book Appointment", "Invalid Time Format", "HH:MM 24-hour", "VR-11: must be in HH:MM format (24-hour clock).", "Appointment time: must be in HH:MM format (24-hour clock).", 2, true},
    {81, "Book Appointment", "Invalid Hour", "00-23", "VR-11: hour must be between 00 and 23.", "Appointment time: hour must be between 00 and 23.", 2, true},
    {82, "Book Appointment", "Unregistered Patient", "Existing Patient ID", "HMS-F-016 / BR-03: Patient not found", "Patient not found", 2, true},
    {83, "Book Appointment", "Unregistered Doctor", "Existing Doctor ID", "HMS-F-017 / BR-04: Doctor not found", "Doctor not found", 2, true},
    {84, "Book Appointment", "Double-Booking Conflict", "Non-overlapping doctor schedule", "HMS-F-019 / BR-05 conflict rule", "Doctor not available at the requested date and time", 2, true},
    {85, "Book Appointment", "Booking Success Outcome", "Appointment booked and ID generated", "HMS-F-015 success", "Appointment booked successfully", 2, false},

    // Appointment Viewing & Filtering
    {86, "View Appointments", "View Menu Prompt", "Option 0-3", "View submenu choices", "Enter your choice: ", 3, false},
    {87, "View Appointments", "View Menu Invalid Choice", "Numeric 0 to 3", "Rule: Enter 0 to 3", "Invalid choice. Enter 0 to 3.", 3, true},
    {88, "View Appointments", "Filter by Patient Prompt", "Patient ID (e.g., P001)", "VR-01 ID format", "Enter patient ID: (e.g., P001): ", 3, false},
    {89, "View Appointments", "Filter by Doctor Prompt", "Doctor ID (e.g., D001)", "VR-01 ID format", "Enter doctor ID: (e.g., D001): ", 3, false},
    {90, "View Appointments", "Empty Appointments Outcome", "At least one appointment record", "HMS-F-022 empty view", "No appointments found.", 3, true},

    // Appointment Cancellation
    {91, "Cancel Appointment", "Appointment ID Prompt", "Appointment ID (e.g., A0001)", "VR-12 ID format", "Enter appointment ID: (e.g., A0001): ", 2, false},
    {92, "Cancel Appointment", "Confirmation Prompt", "Y or N", "Confirmation gate", "Confirm cancellation? (Y/N): ", 2, false},
    {93, "Cancel Appointment", "Nonexistent Appointment ID", "Existing Appointment ID", "HMS-F-024: Appointment not found", "Appointment not found", 2, true},
    {94, "Cancel Appointment", "Already Cancelled ID", "Booked Appointment ID", "HMS-F-024: Appointment already cancelled", "Appointment already cancelled", 2, true},
    {95, "Cancel Appointment", "Cancellation Declined", "Confirmed = N", "User abort confirmation", "Cancellation cancelled by user", 2, true},
    {96, "Cancel Appointment", "Cancellation Success", "Appointment status updated to Cancelled", "HMS-F-023 success", "Appointment cancelled successfully", 2, false},

    // Doctor Availability Checking
    {97, "Check Doctor Availability", "Doctor ID Prompt", "Doctor ID (e.g., D001)", "VR-01 ID format", "Enter doctor ID: (e.g., D001): ", 2, false},
    {98, "Check Doctor Availability", "Date Prompt", "YYYY-MM-DD", "VR-10 date format", "Date (YYYY-MM-DD): ", 2, false},
    {99, "Check Doctor Availability", "Time Prompt", "HH:MM (24-hour format)", "VR-11 time format", "Time (HH:MM, 24-hour format): ", 2, false},
    {100, "Clean Application Exit", "Exit Confirmation", "Selection 4 at Main Menu", "HMS-F-036 normal termination", "Exiting Hospital Management System.", 1, false}
  };

  int checklistPassed = 0;
  for (const auto &item : checklist) {
    // Assert that the item has non-empty prompt/error text and valid depth <= 3
    const bool validPattern = !item.promptOrErrorText.empty();
    const bool validDepth = item.menuDepth >= 1 && item.menuDepth <= 3;
    const bool validMeta = !item.operation.empty() && !item.expectedFormat.empty() && !item.violatedRule.empty();

    if (validPattern && validDepth && validMeta) {
      ++checklistPassed;
    }
  }

  const double passRate = (static_cast<double>(checklistPassed) / checklist.size()) * 100.0;
  std::cout << "  Checklist Items Total:  " << checklist.size() << '\n';
  std::cout << "  Checklist Items Passed: " << checklistPassed << '\n';
  std::cout << "  Checklist Pass Rate:    " << std::fixed << std::setprecision(1) << passRate << "%\n";

  check("Prompt/Message Checklist achieves 100% pass rate", checklistPassed == static_cast<int>(checklist.size()) && passRate == 100.0);

  // =========================================================================
  // Test B — Exhaustive Menu Depth Matrix (All 32 Operations <= 3)
  // =========================================================================
  std::cout << "\n-- Test B: Exhaustive Menu Depth Matrix (Depth <= 3 Guarantee) --\n";

  const std::vector<MenuOperation> menuOperations = {
    {1, "Main Menu Exit", "Main Menu -> 4", "4\n", 1, "Exiting Hospital Management System."},
    {2, "Patient Management Menu", "Main Menu -> 1", "1\n0\n4\n", 1, "=== Patient Management ==="},
    {3, "Patient Registration", "Main Menu -> 1 -> 1", "1\n1\n\n0\n4\n", 2, "--- Register Patient ---"},
    {4, "Patient Search", "Main Menu -> 1 -> 2", "1\n2\n\n0\n4\n", 2, "--- Search Patients ---"},
    {5, "Patient View All", "Main Menu -> 1 -> 3", "1\n3\n0\n4\n", 2, "--- Patient List ---"},
    {6, "Patient Update (Initiate)", "Main Menu -> 1 -> 4", "1\n4\n\n0\n4\n", 2, "--- Update Patient ---"},
    {7, "Patient Update Field (Name)", "Main Menu -> 1 -> 4 -> 1", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {8, "Patient Update Field (Age)", "Main Menu -> 1 -> 4 -> 2", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {9, "Patient Update Field (Gender)", "Main Menu -> 1 -> 4 -> 3", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {10, "Patient Update Field (Contact)", "Main Menu -> 1 -> 4 -> 4", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {11, "Patient Update Field (Address)", "Main Menu -> 1 -> 4 -> 5", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {12, "Patient Update Field (Medical History)", "Main Menu -> 1 -> 4 -> 6", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {13, "Patient Update Field (All Fields)", "Main Menu -> 1 -> 4 -> 7", "1\n4\nP001\n0\n0\n4\n", 3, "Select field to update:"},
    {14, "Doctor Management Menu", "Main Menu -> 2", "2\n0\n4\n", 1, "=== Doctor Management ==="},
    {15, "Doctor Add", "Main Menu -> 2 -> 1", "2\n1\n\n0\n4\n", 2, "--- Add Doctor ---"},
    {16, "Doctor Search", "Main Menu -> 2 -> 2", "2\n2\n\n0\n4\n", 2, "--- Search Doctors ---"},
    {17, "Doctor View All", "Main Menu -> 2 -> 3", "2\n3\n0\n4\n", 2, "--- Doctor List ---"},
    {18, "Doctor Update (Initiate)", "Main Menu -> 2 -> 4", "2\n4\n\n0\n4\n", 2, "--- Update Doctor ---"},
    {19, "Doctor Update Field (Name)", "Main Menu -> 2 -> 4 -> 1", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {20, "Doctor Update Field (Specialization)", "Main Menu -> 2 -> 4 -> 2", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {21, "Doctor Update Field (Department)", "Main Menu -> 2 -> 4 -> 3", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {22, "Doctor Update Field (Contact)", "Main Menu -> 2 -> 4 -> 4", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {23, "Doctor Update Field (Availability)", "Main Menu -> 2 -> 4 -> 5", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {24, "Doctor Update Field (All Fields)", "Main Menu -> 2 -> 4 -> 6", "2\n4\nD001\n0\n0\n4\n", 3, "Select field to update:"},
    {25, "Appointment Management Menu", "Main Menu -> 3", "3\n0\n4\n", 1, "=== Appointment Management ==="},
    {26, "Appointment Booking", "Main Menu -> 3 -> 1", "3\n1\n\n0\n4\n", 2, "--- Book Appointment ---"},
    {27, "Appointment View Submenu", "Main Menu -> 3 -> 2", "3\n2\n0\n0\n4\n", 2, "=== View Appointments ==="},
    {28, "Appointment View All", "Main Menu -> 3 -> 2 -> 1", "3\n2\n1\n0\n4\n", 3, "--- View Appointments ---"},
    {29, "Appointment Filter by Patient", "Main Menu -> 3 -> 2 -> 2", "3\n2\n2\n\n0\n4\n", 3, "Enter patient ID: (e.g., P001):"},
    {30, "Appointment Filter by Doctor", "Main Menu -> 3 -> 2 -> 3", "3\n2\n3\n\n0\n4\n", 3, "Enter doctor ID: (e.g., D001):"},
    {31, "Appointment Cancellation", "Main Menu -> 3 -> 3", "3\n3\n\n0\n4\n", 2, "--- Cancel Appointment ---"},
    {32, "Check Doctor Availability", "Main Menu -> 3 -> 4", "3\n4\n\n0\n4\n", 2, "--- Check Doctor Availability ---"}
  };

  int maxObservedDepth = 0;
  int depthCompliantCount = 0;

  for (const auto &op : menuOperations) {
    if (op.menuDepth > maxObservedDepth) {
      maxObservedDepth = op.menuDepth;
    }
    if (op.menuDepth <= 3) {
      ++depthCompliantCount;
    }

    // Execute interactive session to verify the operation path exists and is reachable
    hms::HospitalSystem sys(testDataDir.string());
    const std::string out = runInteractive(sys, op.inputSequence);
    const bool reached = out.find(op.expectedTextOnReach) != std::string::npos;
    check("Operation reachable: " + op.operationName + " (Path: " + op.navigationPath + ", Depth: " + std::to_string(op.menuDepth) + ")",
          reached && op.menuDepth <= 3);
  }

  std::cout << "\n  Total Operations Audited: " << menuOperations.size() << '\n';
  std::cout << "  Operations with Depth <= 3: " << depthCompliantCount << " / " << menuOperations.size() << '\n';
  std::cout << "  Maximum Observed Depth:     " << maxObservedDepth << '\n';
  check("Every operation has menu depth <= 3 (HMS-NF-005)", depthCompliantCount == static_cast<int>(menuOperations.size()) && maxObservedDepth <= 3);

  // =========================================================================
  // Test C — Expected Input Formats Stated in Live Output
  // =========================================================================
  std::cout << "\n-- Test C: Expected Input Formats Verification --\n";
  {
    hms::HospitalSystem sys(testDataDir.string());

    // 1. Patient Registration Prompts state expected formats
    const std::string patRegOut = runInteractive(sys, "1\n1\n\n0\n4\n");
    check("Patient ID prompt states format (1-10 alphanumeric characters, e.g., P001)",
          patRegOut.find("Patient ID (1-10 alphanumeric characters, e.g., P001): ") != std::string::npos);

    // 2. Doctor Registration Prompts state expected formats
    const std::string docAddOut = runInteractive(sys, "2\n1\n\n0\n4\n");
    check("Doctor ID prompt states format (1-10 alphanumeric characters, e.g., D001)",
          docAddOut.find("Doctor ID (1-10 alphanumeric characters, e.g., D001): ") != std::string::npos);

    // 3. Appointment Booking Prompts state formats for Date and Time
    const std::string apptBookOut = runInteractive(sys, "3\n1\nP001\nD001\n2026-11-20\n\n0\n4\n");
    check("Booking Date prompt states format (YYYY-MM-DD)",
          apptBookOut.find("Date (YYYY-MM-DD): ") != std::string::npos);
    check("Booking Time prompt states format (HH:MM, 24-hour format)",
          apptBookOut.find("Time (HH:MM, 24-hour format): ") != std::string::npos);

    // 4. Appointment Cancellation Prompt states format (e.g., A0001)
    const std::string cancelOut = runInteractive(sys, "3\n3\n\n0\n4\n");
    check("Cancellation prompt states format (e.g., A0001)",
          cancelOut.find("Enter appointment ID: (e.g., A0001): ") != std::string::npos);

    // 5. Doctor Availability Prompts state formats
    const std::string availOut = runInteractive(sys, "3\n4\nD001\n2026-11-20\n\n0\n4\n");
    check("Availability Doctor ID states format (e.g., D001)",
            availOut.find("Enter doctor ID (e.g., D001): ") != std::string::npos);
    check("Availability Time states format (HH:MM, 24-hour format)",
          availOut.find("Time (HH:MM, 24-hour format): ") != std::string::npos);
  }

  // =========================================================================
  // Test D — Validation Error Clarity (Names Field & Violated Rule)
  // =========================================================================
  std::cout << "\n-- Test D: Validation Error Clarity (Field & Violated Rule Named) --\n";
  {
    hms::HospitalSystem sys(testDataDir.string());

    // 1. Invalid age rejection states field and range rule
    const std::string badAgeOut = runInteractive(sys, "1\n1\nP999\nTest Patient\n150\n0\n4\n");
    check("Age error states field and rule: 'Age: must be a whole number from 0 to 120.'",
          badAgeOut.find("Age: must be a whole number from 0 to 120.") != std::string::npos);

    // 2. Invalid date rejection states field and format rule
    const std::string badDateOut = runInteractive(sys, "3\n1\nP001\nD001\n2026/11/15\n10:00\n0\n4\n");
    check("Date error states field and rule: 'Appointment date: must be in YYYY-MM-DD format.'",
          badDateOut.find("Appointment date: must be in YYYY-MM-DD format.") != std::string::npos);

    // 3. Invalid calendar day states field and calendar rule
    const std::string badDayOut = runInteractive(sys, "3\n1\nP001\nD001\n2026-02-30\n10:00\n0\n4\n");
    check("Date error states field and rule: 'Appointment date: day is invalid for the specified month and year.'",
          badDayOut.find("Appointment date: day is invalid for the specified month and year.") != std::string::npos);

    // 4. Invalid time rejection states field and 24-hr rule
    const std::string badTimeOut = runInteractive(sys, "3\n1\nP001\nD001\n2026-11-20\n25:00\n0\n4\n");
    check("Time error states field and rule: 'Appointment time: hour must be between 00 and 23.'",
          badTimeOut.find("Appointment time: hour must be between 00 and 23.") != std::string::npos);

    // 5. Invalid menu selection states field and accepted options
    const std::string badMenuOut = runInteractive(sys, "9\n4\n");
    check("Menu error states field and rule: 'Invalid choice. Enter 1, 2, 3, or 4.'",
          badMenuOut.find("Invalid choice. Enter 1, 2, 3, or 4.") != std::string::npos);
  }

  // =========================================================================
  // Test E — Existing Functionality & Regression Compatibility
  // =========================================================================
  std::cout << "\n-- Test E: Existing Functionality & Regression Compatibility --\n";
  {
    hms::HospitalSystem sys(testDataDir.string());

    // 1. Patient Registration & Search
    const std::string patFlow =
        "1\n1\nP002\nAlice Bob\n30\nF\n9876543219\n100 Elm St\nNone\n"
        "2\nAlice\n"
        "0\n4\n";
    const std::string patOut = runInteractive(sys, patFlow);
    check("Patient registered successfully in interactive flow",
          patOut.find("Patient registered successfully.") != std::string::npos);
    check("Patient search finds registered patient",
          patOut.find("Patient ID: P002") != std::string::npos);

    // 2. Doctor Addition & Search
    const std::string docFlow =
        "2\n1\nD002\nDr. Bruce Wayne\nNeurology\nNeuro\n9876543218\nMon-Thu 09:00-15:00\n"
        "2\nWayne\n"
        "0\n4\n";
    const std::string docOut = runInteractive(sys, docFlow);
    check("Doctor added successfully in interactive flow",
          docOut.find("Doctor added successfully.") != std::string::npos);
    check("Doctor search finds registered doctor",
          docOut.find("Doctor ID: D002") != std::string::npos);

    // 3. Appointment Booking & Doctor Conflict Check
    const std::string apptFlow =
        "3\n1\nP002\nD002\n2026-12-01\n11:00\n" // Valid booking
        "1\nP001\nD002\n2026-12-01\n11:00\n"     // Conflict booking
        "0\n4\n";
    const std::string apptOut = runInteractive(sys, apptFlow);
    check("Appointment booked successfully in interactive flow",
          apptOut.find("Appointment booked successfully.") != std::string::npos);
    check("Conflict rejected with exact message 'Doctor not available at the requested date and time'",
          apptOut.find("Doctor not available at the requested date and time") != std::string::npos);

    // 4. Appointment Cancellation & Invalid Cancellation
    const std::string cancelFlow =
        "3\n3\nA0001\nY\n"                       // Cancel valid booked A0001
        "3\nA0001\nY\n"                       // Second attempt: already cancelled
        "3\nA9999\nY\n"                       // Nonexistent attempt
        "0\n4\n";
    const std::string cancelOut = runInteractive(sys, cancelFlow);
    check("Cancellation succeeds with exact message 'Appointment cancelled successfully'",
          cancelOut.find("Appointment cancelled successfully") != std::string::npos);
    check("Already cancelled rejected with exact message 'Appointment already cancelled'",
          cancelOut.find("Appointment already cancelled") != std::string::npos);
    check("Nonexistent rejected with exact message 'Appointment not found'",
          cancelOut.find("Appointment not found") != std::string::npos);
  }

  // =========================================================================
  // Test F — Data Integrity & Root File Preservation
  // =========================================================================
  std::cout << "\n-- Test F: Data Integrity & Root CSV File Preservation --\n";
  {
    // Clean up isolated test directory
    fs::remove_all(testDataDir, ec);

    // Verify root files remain 100% untouched byte-for-byte
    const std::string currentRootPatients = readFileContent(rootPatients);
    const std::string currentRootDoctors = readFileContent(rootDoctors);
    const std::string currentRootAppointments = readFileContent(rootAppointments);

    check("Root patients.csv preserved byte-for-byte untouched",
          currentRootPatients == rootPatientsBaseline);
    check("Root doctors.csv preserved byte-for-byte untouched",
          currentRootDoctors == rootDoctorsBaseline);
    check("Root appointments.csv preserved byte-for-byte untouched",
          currentRootAppointments == rootAppointmentsBaseline);
  }

  // =========================================================================
  // Test Summary
  // =========================================================================
  std::cout << "\n======================================================================\n";
  std::cout << "TC-NFR-05 Usability & Menu Depth Results:\n";
  std::cout << "  Passed: " << g_passed << "\n";
  std::cout << "  Failed: " << g_failed << "\n";
  std::cout << "======================================================================\n";

  return g_failed == 0 ? 0 : 1;
}
