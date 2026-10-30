// ============================================================================
// TC-SEC-03: Over-Length Input Safety (HMS-SR-003)
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-SR-003 (Security Requirements - Over-Length Input Safety)
//   SRS Reference:      HMS-SR-003 / Table 4-1 (VR-01 to VR-14)
//   STP RTM Test Case:  TC-SEC-03 (Over-Length Input Safety)
//   GitHub Issue:       #49
//
// Objective:
//   Verify that the HMS application safely handles and rejects arbitrary
//   over-length keyboard input (specifically testing 10,000-character inputs)
//   without buffer overflow, memory corruption, or silent truncation, while
//   allowing the application to continue running normally and process subsequent
//   valid input.
//
// Acceptance Criterion:
//   "A 10,000-character input is rejected and the application continues running."
//
// Test Suite Coverage:
//   1. Interactive Menu Navigation:
//      - 10,000-character input at Main Menu prompt: rejected, re-prompts,
//        subsequent navigation and clean exit succeed.
//      - 10,000-character input at Submenus (Patient, Doctor, Appointment):
//        rejected with menu error, subsequent return to main and clean exit succeed.
//   2. Interactive Record Entry & Subsequent Processing:
//      - Patient registration with 10,000-character Name: rejected, 0 records
//        stored, subsequent valid registration succeeds.
//      - Doctor addition with 10,000-character Specialization: rejected, 0 records
//        stored, subsequent valid addition succeeds.
//      - Appointment booking with 10,000-character Date: rejected, 0 records
//        stored, subsequent valid booking succeeds.
//   3. Field-Validator Boundary Rejection (10,000-character inputs):
//      - All Table 4-1 fields tested against 10,000-character strings.
//      - Validates that every validator returns false without crashing.
//   4. Programmatic State Preservation:
//      - updatePatient with 10,000-character Address: rejected, baseline intact.
//      - updateDoctor with 10,000-character Availability: rejected, baseline intact.
//      - bookAppointment with 10,000-character Date: rejected, CSV untouched.
//   5. No Silent Truncation:
//      - Verifies that records are not truncated to field limits and stored.
// ============================================================================

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_passed = 0;
int g_failed = 0;

void check(const std::string &name, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << name << '\n';
    ++g_passed;
  } else {
    std::cout << "  [FAIL] " << name << '\n';
    ++g_failed;
  }
}

// Helper to generate a 10,000-character string
std::string make10kString(char fill = 'A') {
  return std::string(10000, fill);
}

// ---------------------------------------------------------------------------
// Isolated temp-directory fixture
// ---------------------------------------------------------------------------

struct TempDb {
  std::filesystem::path dir;
  std::filesystem::path pCsv;
  std::filesystem::path dCsv;
  std::filesystem::path aCsv;

  explicit TempDb(const std::string &label) {
    dir = std::filesystem::temp_directory_path() / ("hms_sec03_" + label);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    pCsv = dir / "patients.csv";
    dCsv = dir / "doctors.csv";
    aCsv = dir / "appointments.csv";
    // Write header-only CSV files so FileManager loads cleanly.
    { std::ofstream f(pCsv); f << "patient_id,name,age,gender,contact_number,address,medical_history\n"; }
    { std::ofstream f(dCsv); f << "doctor_id,name,specialization,department,contact_number,availability\n"; }
    { std::ofstream f(aCsv); f << "appointment_id,patient_id,doctor_id,date,time,status\n"; }
  }

  hms::HospitalSystem makeSystem() const {
    hms::FileManager fm(pCsv.string(), dCsv.string(), aCsv.string());
    return hms::HospitalSystem(std::move(fm));
  }

  hms::FileManager makeFileManager() const {
    return hms::FileManager(pCsv.string(), dCsv.string(), aCsv.string());
  }

  ~TempDb() {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
};

// Seeding helpers
bool seedPatient(hms::HospitalSystem &sys,
                 const std::string &id, const std::string &name, int age,
                 const std::string &gender, const std::string &contact,
                 const std::string &address,
                 const std::string &medHist = "None") {
  std::string err;
  hms::Patient p(id, name, age, gender, contact, address, medHist);
  if (!p.isValid() || !sys.getFileManager().savePatient(p, err)) {
    return false;
  }
  return sys.findPatient(id) != nullptr;
}

bool seedDoctor(hms::HospitalSystem &sys,
                const std::string &id, const std::string &name,
                const std::string &spec, const std::string &dept,
                const std::string &contact, const std::string &avail) {
  std::string err;
  hms::Doctor d(id, name, spec, dept, contact, avail);
  if (!d.isValid() || !sys.getFileManager().saveDoctor(d, err)) {
    return false;
  }
  return sys.findDoctor(id) != nullptr;
}

// ===========================================================================
// SECTION 1: Interactive Menu Navigation with 10,000-Character Input
// ===========================================================================

void testMainMenu_OverLengthInput() {
  std::cout << "\n--- TC-SEC-03: Main Menu Over-Length Input ---\n";
  TempDb db("main_menu_10k");
  auto sys = db.makeSystem();

  // Feed 10,000-character input at main menu prompt, then choose '4' to exit cleanly.
  std::istringstream input(make10kString('9') + "\n4\n");
  std::ostringstream output;

  auto *origCin = std::cin.rdbuf(input.rdbuf());
  auto *origCout = std::cout.rdbuf(output.rdbuf());

  sys.run();

  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  const std::string out = output.str();
  check("Main menu rejects 10,000-character option with invalid choice error",
        out.find("Invalid choice. Enter 1, 2, 3, or 4.") != std::string::npos);
  check("Application continues running and exits cleanly on subsequent input '4'",
        out.find("Exiting Hospital Management System.") != std::string::npos);
}

void testSubmenus_OverLengthInput() {
  std::cout << "\n--- TC-SEC-03: Submenu Over-Length Input ---\n";

  // Patient Submenu
  {
    TempDb db("pat_menu_10k");
    auto sys = db.makeSystem();

    std::istringstream input("1\n" + make10kString('X') + "\n0\n4\n");
    std::ostringstream output;
    auto *origCin = std::cin.rdbuf(input.rdbuf());
    auto *origCout = std::cout.rdbuf(output.rdbuf());

    sys.run();

    std::cin.rdbuf(origCin);
    std::cout.rdbuf(origCout);

    const std::string out = output.str();
    check("Patient submenu rejects 10,000-character option",
          out.find("Invalid choice. Enter 0 to 4.") != std::string::npos);
    check("Patient submenu processes subsequent return '0' and exits on '4'",
          out.find("Exiting Hospital Management System.") != std::string::npos);
  }

  // Doctor Submenu
  {
    TempDb db("doc_menu_10k");
    auto sys = db.makeSystem();

    std::istringstream input("2\n" + make10kString('Y') + "\n0\n4\n");
    std::ostringstream output;
    auto *origCin = std::cin.rdbuf(input.rdbuf());
    auto *origCout = std::cout.rdbuf(output.rdbuf());

    sys.run();

    std::cin.rdbuf(origCin);
    std::cout.rdbuf(origCout);

    const std::string out = output.str();
    check("Doctor submenu rejects 10,000-character option",
          out.find("Invalid choice. Enter 0 to 4.") != std::string::npos);
    check("Doctor submenu processes subsequent return '0' and exits on '4'",
          out.find("Exiting Hospital Management System.") != std::string::npos);
  }

  // Appointment Submenu
  {
    TempDb db("apt_menu_10k");
    auto sys = db.makeSystem();

    std::istringstream input("3\n" + make10kString('Z') + "\n0\n4\n");
    std::ostringstream output;
    auto *origCin = std::cin.rdbuf(input.rdbuf());
    auto *origCout = std::cout.rdbuf(output.rdbuf());

    sys.run();

    std::cin.rdbuf(origCin);
    std::cout.rdbuf(origCout);

    const std::string out = output.str();
    check("Appointment submenu rejects 10,000-character option",
          out.find("Invalid choice. Enter 0 to 4.") != std::string::npos);
    check("Appointment submenu processes subsequent return '0' and exits on '4'",
          out.find("Exiting Hospital Management System.") != std::string::npos);
  }
}

// ===========================================================================
// SECTION 2: Interactive Record Entry & Subsequent Processing
// ===========================================================================


void testInteractivePatientRegistration_OverLengthName() {
  std::cout << "\n--- TC-SEC-03: Interactive Patient Registration with 10k Name ---\n";
  TempDb db("pat_reg_10k");
  auto sys = db.makeSystem();

  // Enter an invalid name, then retry the same field with a valid name.
  // The valid details must complete registration for P001.
  std::string inputSequence =
      "1\n"                         // Main menu -> Patient menu
      "1\n"                         // Register patient
      "P001\n"                      // ID
      + make10kString('A') + "\n"   // Over-length Name (10,000 chars)
      "Alice Smith\n"               // Retry Name with valid input
      "28\n"                        // Age
      "F\n"                         // Gender
      "9123456789\n"                // Contact
      "456 Oak Ave\n"               // Address
      "None\n"                      // Medical history
      "0\n"                         // Return to main
      "4\n";                        // Exit

  std::istringstream input(inputSequence);
  std::ostringstream output;

  auto *origCin = std::cin.rdbuf(input.rdbuf());
  auto *origCout = std::cout.rdbuf(output.rdbuf());

  sys.run();

  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  const std::string out = output.str();

  const std::string namePrompt = "Full name (1-50 characters): ";
  const std::string nameError =
      "Name: must contain 1-50 permitted characters";

  const auto firstNamePrompt = out.find(namePrompt);
  const auto validationError = out.find(nameError);
  const auto retryNamePrompt =
      firstNamePrompt == std::string::npos
          ? std::string::npos
          : out.find(namePrompt, firstNamePrompt + namePrompt.size());

  check("Console displays Name validation error for 10,000-character name",
        validationError != std::string::npos);

  check("Name prompt is repeated after the validation error",
        validationError != std::string::npos &&
        retryNamePrompt != std::string::npos &&
        validationError < retryNamePrompt);

  check("Patient P001 is stored after valid name retry",
        db.makeFileManager().patientIdExists("P001"));

  check("No unintended P002 patient record is created",
        !db.makeFileManager().patientIdExists("P002"));

  check("Exactly one patient is stored",
        db.makeFileManager().loadPatients().size() == 1);
}

void testInteractiveDoctorAddition_OverLengthSpecialization() {
  std::cout << "\n--- TC-SEC-03: Interactive Doctor Addition with 10k Specialization ---\n";
  TempDb db("doc_add_10k");
  auto sys = db.makeSystem();

    std::string inputSequence =
      "2\n"
      "1\n"
      "D001\n"
      "Dr. Gregory House\n"
      + make10kString('S') + "\n"
      "Diagnostics\n"               // Retry: valid specialization
      "Diagnostics Dept\n"          // Department
      "9876543210\n"
      "Mon-Fri 09:00-17:00\n"
      "1\n"
      "D002\n"
      "Dr. James Wilson\n"
      "Oncology\n"
      "Oncology Dept\n"
      "9123456789\n"
      "Mon-Fri 08:00-16:00\n"
      "0\n"
      "4\n";

  std::istringstream input(inputSequence);
  std::ostringstream output;

  auto *origCin = std::cin.rdbuf(input.rdbuf());
  auto *origCout = std::cout.rdbuf(output.rdbuf());

  sys.run();

  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  const std::string out = output.str();
  check("Console displays Specialization validation error for 10,000-character input",
        out.find("Specialization: must contain 1-40 characters.") != std::string::npos);
    check("Doctor D001 is stored only after valid specialization retry",
        db.makeFileManager().doctorIdExists("D001"));  check("Subsequent valid doctor addition succeeds",
        out.find("Doctor added successfully.") != std::string::npos &&
        out.find("Doctor ID: D002") != std::string::npos);
  check("Valid doctor D002 is stored in persistent storage",
        db.makeFileManager().doctorIdExists("D002"));
    check("Total stored doctors is exactly 2 after valid retry and subsequent addition",
        db.makeFileManager().loadDoctors().size() == 2);
}

void testInteractiveAppointmentBooking_OverLengthDate() {
  std::cout << "\n--- TC-SEC-03: Interactive Appointment Booking with 10k Date ---\n";
  TempDb db("apt_book_10k");
  auto sys = db.makeSystem();

  // Seed baseline patient and doctor
  seedPatient(sys, "P099", "Test Patient", 30, "M", "9876543210", "123 Elm St");
  seedDoctor(sys, "D099", "Dr. Test", "Cardiology", "Cardiology", "9876543210", "Mon-Fri");


  std::string inputSequence =
      "3\n"                         // Main menu -> Appointment menu
      "1\n"                         // Book appointment
      "P099\n"                      // Patient ID
      "D099\n"                      // Doctor ID
      + make10kString('9') + "\n"   // Over-length Date (10,000 chars)
      "14:00\n"                     // Time
      "1\n"                         // Book appointment again
      "P099\n"                      // Valid Patient ID
      "D099\n"                      // Valid Doctor ID
      "2026-11-20\n"                // Valid Date
      "14:00\n"                     // Valid Time
      "0\n"                         // Return
      "4\n";                        // Exit

  std::istringstream input(inputSequence);
  std::ostringstream output;

  auto *origCin = std::cin.rdbuf(input.rdbuf());
  auto *origCout = std::cout.rdbuf(output.rdbuf());

  sys.run();

  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  const std::string out = output.str();
  check("Console displays Date validation error for 10,000-character date",
        out.find("Appointment date: must be in YYYY-MM-DD format.") != std::string::npos);
  check("Subsequent valid appointment booking succeeds",
        out.find("Appointment booked successfully.") != std::string::npos);
  check("Total stored appointments is exactly 1",
        sys.listAppointments().size() == 1);
}

// ===========================================================================
// SECTION 3: Field-Validator Boundary Rejection (All 17 fields with 10k input)
// ===========================================================================

void testFieldValidators_OverLengthInputs() {
  std::cout << "\n--- TC-SEC-03: Field-Validator Boundaries with 10,000-Character Inputs ---\n";
  const std::string tenK = make10kString('A');
  const std::string tenKDigits = make10kString('9');
  std::string err;

  // Patient fields
  check("Patient::validateId rejects 10,000-character ID",
        !hms::Patient::validateId(tenK, err) && !err.empty());

  check("Patient::validateName rejects 10,000-character Name",
        !hms::Patient::validateName(tenK, err) && !err.empty());

  int outAge = 0;
  check("Patient::validateAge rejects 10,000-character Age string",
        !hms::Patient::validateAge(tenKDigits, outAge, err) && !err.empty());

  check("Patient::validateGender rejects 10,000-character Gender",
        !hms::Patient::validateGender(tenK, err) && !err.empty());

  check("Patient::validateContactNumber rejects 10,000-character Contact",
        !hms::Patient::validateContactNumber(tenKDigits, err) && !err.empty());

  check("Patient::validateAddress rejects 10,000-character Address",
        !hms::Patient::validateAddress(tenK, err) && !err.empty());

  check("Patient::validateMedicalHistory rejects 10,000-character MedHistory",
        !hms::Patient::validateMedicalHistory(tenK, err) && !err.empty());

  // Doctor fields
  check("Doctor::validateId rejects 10,000-character ID",
        !hms::Doctor::validateId(tenK, err) && !err.empty());

  check("Doctor::validateName rejects 10,000-character Name",
        !hms::Doctor::validateName(tenK, err) && !err.empty());

  check("Doctor::validateSpecialization rejects 10,000-character Specialization",
        !hms::Doctor::validateSpecialization(tenK, err) && !err.empty());

  check("Doctor::validateDepartment rejects 10,000-character Department",
        !hms::Doctor::validateDepartment(tenK, err) && !err.empty());

  check("Doctor::validateContactNumber rejects 10,000-character Contact",
        !hms::Doctor::validateContactNumber(tenKDigits, err) && !err.empty());

  check("Doctor::validateAvailability rejects 10,000-character Availability",
        !hms::Doctor::validateAvailability(tenK, err) && !err.empty());

  // Appointment fields
  check("Appointment::validateId rejects 10,000-character ID ('A' + 9999 digits)",
        !hms::Appointment::validateId("A" + make10kString('0'), err) && !err.empty());

  check("Appointment::validateDate rejects 10,000-character Date",
        !hms::Appointment::validateDate(tenKDigits, err) && !err.empty());

  check("Appointment::validateTime rejects 10,000-character Time",
        !hms::Appointment::validateTime(tenKDigits, err) && !err.empty());

  hms::AppointmentStatus status;
  check("Appointment::validateStatus rejects 10,000-character Status",
        !hms::Appointment::validateStatus(tenK, status, err) && !err.empty());
}

// ===========================================================================
// SECTION 4: Programmatic Modification Rejection & State Preservation
// ===========================================================================

void testProgrammaticUpdate_OverLengthRejection() {
  std::cout << "\n--- TC-SEC-03: Programmatic Update Rejection & State Preservation ---\n";
  TempDb db("prog_upd_10k");
  auto sys = db.makeSystem();

  // Seed baseline patient and doctor
  seedPatient(sys, "P050", "Original Patient", 40, "M", "9876543210", "10 Elm Street");
  seedDoctor(sys, "D050", "Dr. Original", "Cardiology", "Cardiology", "9876543210", "Mon-Fri");

  // Attempt to update Patient with 10,000-character address
  hms::OpResult patRes = sys.updatePatient("P050", "Original Patient", 40, "M",
                                          "9876543210", make10kString('Z'), "None");
  check("updatePatient rejects 10,000-character address", !patRes.success);

  const hms::Patient *p = sys.findPatient("P050");
  const auto storedPatients = db.makeFileManager().loadPatients();
  check("Baseline patient record intact in memory after over-length update attempt",
        p != nullptr && p->getAddress() == "10 Elm Street");
  check("Baseline patient record intact in CSV after over-length update attempt",
        storedPatients.size() == 1 && storedPatients[0].getAddress() == "10 Elm Street");

  // Attempt to update Doctor with 10,000-character availability
  hms::OpResult docRes = sys.updateDoctor("D050", "Dr. Original", "Cardiology",
                                          "Cardiology", "9876543210", make10kString('W'));
  check("updateDoctor rejects 10,000-character availability", !docRes.success);

  const hms::Doctor *d = sys.findDoctor("D050");
  const auto storedDoctors = db.makeFileManager().loadDoctors();
  check("Baseline doctor record intact in memory after over-length update attempt",
        d != nullptr && d->getAvailability() == "Mon-Fri");
  check("Baseline doctor record intact in CSV after over-length update attempt",
        storedDoctors.size() == 1 && storedDoctors[0].getAvailability() == "Mon-Fri");
}

void testProgrammaticBooking_OverLengthRejection() {
  std::cout << "\n--- TC-SEC-03: Programmatic Booking Rejection & State Preservation ---\n";
  TempDb db("prog_book_10k");
  auto sys = db.makeSystem();

  seedPatient(sys, "P060", "Valid Patient", 35, "F", "9876543210", "42 Park Ave");
  seedDoctor(sys, "D060", "Dr. Valid", "Pediatrics", "Pediatrics", "9876543210", "Mon-Fri");

  std::string apptId, err;
  bool booked = sys.bookAppointment("P060", "D060", make10kString('9'), "10:00", apptId, err);
  check("bookAppointment rejects 10,000-character date", !booked);
  check("Appointments storage remains completely empty",
        sys.listAppointments().empty());

  // Subsequent valid booking succeeds
  bool validBooked = sys.bookAppointment("P060", "D060", "2026-11-25", "10:00", apptId, err);
  check("Subsequent valid booking succeeds after over-length rejection", validBooked);
  check("Appointments storage contains exactly 1 record",
        sys.listAppointments().size() == 1);
}

// ===========================================================================
// SECTION 5: Verification of No Silent Truncation
// ===========================================================================

void testNoSilentTruncation() {
  std::cout << "\n--- TC-SEC-03: Verification of No Silent Truncation ---\n";
  TempDb db("no_trunc_10k");
  auto fm = db.makeFileManager();
  std::string err;

  // A 10,000-character name must NOT be silently truncated to 50 chars and saved
  hms::Patient badPatient("P070", make10kString('A'), 25, "M", "9876543210", "Address", "None");
  bool patSaved = fm.savePatient(badPatient, err);
  check("FileManager::savePatient rejects 10,000-character name (not truncated)", !patSaved);
  check("No patient record persisted (0 rows in CSV)", fm.loadPatients().empty());

  // A 10,000-character specialization must NOT be silently truncated to 40 chars and saved
  hms::Doctor badDoctor("D070", "Dr. Test", make10kString('S'), "Dept", "9876543210", "Mon-Fri");
  bool docSaved = fm.saveDoctor(badDoctor, err);
  check("FileManager::saveDoctor rejects 10,000-character specialization (not truncated)", !docSaved);
  check("No doctor record persisted (0 rows in CSV)", fm.loadDoctors().empty());

  // An over-length Appointment ID must NOT be silently truncated and saved
  hms::Appointment badAppt("A" + make10kString('1'), "P070", "D070", "2026-10-15", "10:00");
  bool apptSaved = fm.saveAppointment(badAppt, err);
  check("FileManager::saveAppointment rejects over-length appointment ID (not truncated)", !apptSaved);
  check("No appointment record persisted (0 rows in CSV)", fm.loadAppointments().empty());
}

} // namespace

// ===========================================================================
// main
// ===========================================================================

int main() {
  std::cout << "=================================================================\n";
  std::cout << "TC-SEC-03: Over-Length Input Safety (HMS-SR-003)\n";
  std::cout << "=================================================================\n";

  // Section 1: Interactive Menu Navigation
  testMainMenu_OverLengthInput();
  testSubmenus_OverLengthInput();

  // Section 2: Interactive Record Entry & Subsequent Processing
  testInteractivePatientRegistration_OverLengthName();
  testInteractiveDoctorAddition_OverLengthSpecialization();
  testInteractiveAppointmentBooking_OverLengthDate();

  // Section 3: Field-Validator Boundaries (10,000 characters)
  testFieldValidators_OverLengthInputs();

  // Section 4: Programmatic Modification Rejection & State Preservation
  testProgrammaticUpdate_OverLengthRejection();
  testProgrammaticBooking_OverLengthRejection();

  // Section 5: Verification of No Silent Truncation
  testNoSilentTruncation();

  std::cout << "\n=================================================================\n";
  std::cout << "TC-SEC-03 Summary: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=================================================================\n";

  return (g_failed == 0) ? 0 : 1;
}

