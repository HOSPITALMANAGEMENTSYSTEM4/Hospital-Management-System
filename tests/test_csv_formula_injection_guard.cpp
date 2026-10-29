// ============================================================================
// TC-SEC-02: CSV and Formula Injection Guard (HMS-SR-002)
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-SR-002 (Security Requirements - CSV/Formula Injection)
//   SRS Reference:      VR-14 (Text-safety Rule, Table 4-1)
//   STP RTM Test Case:  TC-SEC-02 (CSV and Formula Injection Guard)
//   GitHub Issue:       #48
//
// Objective:
//   Verify that the HMS rejects any text input containing CSV delimiters or
//   formula-injection prefixes before modifying in-memory state or persisting
//   any record.  The acceptance criterion is:
//
//     "Delimiter/formula inputs are rejected; no record is stored."
//
// Exact validation rules tested (VR-14, all seven conditions):
//   1. Contains a comma ','
//   2. Contains a carriage return '\r'
//   3. Contains a line feed '\n'
//   4. First character is '='
//   5. First character is '+'
//   6. First character is '-'
//   7. First character is '@'
//
// Coverage matrix (per requirement):
//   - Patient operations : registerPatient (add), updatePatient (update)
//   - Doctor operations  : addDoctor (add), updateDoctor (update)
//   - Appointment ops    : bookAppointment (add); date/time fields carry VR-14
//
// Test strategy:
//   Each test scenario:
//     a) Injects a single bad value into a specific field.
//     b) Invokes the relevant HospitalSystem API directly.
//     c) Asserts the operation is rejected (returns false / error message set).
//     d) Asserts no new record exists in the persistent CSV file.
//     e) Where applicable, asserts that a pre-existing record is not mutated.
//
// Convention:
//   Follows the repository's existing test conventions (manual check()/assert
//   harness, isolated temp directories, FileManager for post-op verification).
// ============================================================================

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"
#include "hms/Patient.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Minimal pass/fail harness (same pattern as the rest of the test suite)
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Isolated temp-directory fixture
// ---------------------------------------------------------------------------

struct TempDb {
  std::filesystem::path dir;
  std::filesystem::path pCsv;
  std::filesystem::path dCsv;
  std::filesystem::path aCsv;

  explicit TempDb(const std::string &label) {
    dir = std::filesystem::temp_directory_path() / ("hms_sec02_" + label);
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

// ---------------------------------------------------------------------------
// Helper: seed a known-good patient so update tests have something to mutate.
// Returns false if seeding fails (test will catch it).
// ---------------------------------------------------------------------------
bool seedPatient(hms::HospitalSystem &sys,
                 const std::string &id, const std::string &name, int age,
                 const std::string &gender, const std::string &contact,
                 const std::string &address,
                 const std::string &medHist = "None") {
  std::string apptId, err;
  // Use the programmatic updatePatient API first — need to register via the
  // FileManager directly because we do not have a registerPatient() API that
  // bypasses the interactive console.  Use FileManager::savePatient().
  hms::Patient p(id, name, age, gender, contact, address, medHist);
  return p.isValid() && sys.getFileManager().savePatient(p, err);
}

bool seedDoctor(hms::HospitalSystem &sys,
                const std::string &id, const std::string &name,
                const std::string &spec, const std::string &dept,
                const std::string &contact, const std::string &avail) {
  std::string err;
  hms::Doctor d(id, name, spec, dept, contact, avail);
  return d.isValid() && sys.getFileManager().saveDoctor(d, err);
}

// ===========================================================================
// SECTION 1: Patient – Add (registerPatient via programmatic API)
//
// The programmatic path that tests call is:
//   HospitalSystem does not expose registerPatient() publicly, so we use
//   the FileManager::savePatient() gate, which calls Patient::isValid().
//   That in turn calls Patient::validate() -> passesSecurityCheck().
// ===========================================================================

void testPatientAdd_Comma() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for comma in name ---\n";
  TempDb db("p_add_comma");
  auto sys = db.makeSystem();

  // Attempt to add a patient whose name contains a CSV delimiter (comma).
  hms::Patient p("P001", "Smith, John", 30, "M", "9876543210", "123 Main St", "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with comma in name rejected", !saved);
  check("No patient record stored after comma-name rejection",
        !db.makeFileManager().patientIdExists("P001"));
}

void testPatientAdd_CR() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for CR in address ---\n";
  TempDb db("p_add_cr");
  auto sys = db.makeSystem();

  hms::Patient p("P002", "Jane Doe", 25, "F", "9876543210",
                 std::string("123 Main St\r"), "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with CR in address rejected", !saved);
  check("No patient record stored after CR-address rejection",
        !db.makeFileManager().patientIdExists("P002"));
}

void testPatientAdd_LF() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for LF in address ---\n";
  TempDb db("p_add_lf");
  auto sys = db.makeSystem();

  hms::Patient p("P003", "Bob Brown", 40, "M", "9876543210",
                 std::string("123 Main\nSt"), "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with LF in address rejected", !saved);
  check("No patient record stored after LF-address rejection",
        !db.makeFileManager().patientIdExists("P003"));
}

void testPatientAdd_PrefixEquals() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for '=' prefix in name ---\n";
  TempDb db("p_add_eq");
  auto sys = db.makeSystem();

  hms::Patient p("P004", "=EvilName", 30, "M", "9876543210", "123 Main St", "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with '=' prefix in name rejected", !saved);
  check("No patient record stored after '='-prefix-name rejection",
        !db.makeFileManager().patientIdExists("P004"));
}

void testPatientAdd_PrefixPlus() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for '+' prefix in address ---\n";
  TempDb db("p_add_plus");
  auto sys = db.makeSystem();

  hms::Patient p("P005", "Alice Green", 28, "F", "9876543210", "+HYPERLINK()", "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with '+' prefix in address rejected", !saved);
  check("No patient record stored after '+'-prefix-address rejection",
        !db.makeFileManager().patientIdExists("P005"));
}

void testPatientAdd_PrefixMinus() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for '-' prefix in medicalHistory ---\n";
  TempDb db("p_add_minus");
  auto sys = db.makeSystem();

  hms::Patient p("P006", "Charlie Red", 50, "M", "9876543210",
                 "42 Oak Ave", "-2+3*cmd|'ls'!A0");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with '-' prefix in medical history rejected", !saved);
  check("No patient record stored after '-'-prefix-medicalHistory rejection",
        !db.makeFileManager().patientIdExists("P006"));
}

void testPatientAdd_PrefixAt() {
  std::cout << "\n--- TC-SEC-02/P-ADD: Patient add rejected for '@' prefix in medicalHistory ---\n";
  TempDb db("p_add_at");
  auto sys = db.makeSystem();

  hms::Patient p("P007", "Diana Blue", 35, "F", "9876543210",
                 "99 Park Blvd", "@SUM(A1:A100)");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Patient with '@' prefix in medical history rejected", !saved);
  check("No patient record stored after '@'-prefix-medicalHistory rejection",
        !db.makeFileManager().patientIdExists("P007"));
}

// ===========================================================================
// SECTION 2: Patient – Update
//
// updatePatient() validates the mutated Patient before persisting, so the
// security check fires before any CSV write.
// ===========================================================================

void testPatientUpdate_Comma() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for comma in name ---\n";
  TempDb db("p_upd_comma");
  auto sys = db.makeSystem();

  // Seed a valid patient.
  assert(seedPatient(sys, "P010", "Eve White", 22, "F", "1234567890", "1 Pine St"));

  // Try to update the name to include a comma.
  hms::OpResult res = sys.updatePatient("P010", "White, Eve", 22, "F",
                                        "1234567890", "1 Pine St", "None");
  check("Update with comma in name rejected", !res.success);

  // Original record must be intact.
  const hms::Patient *p = sys.findPatient("P010");
  check("Original patient record unmodified after comma-name update failure",
        p != nullptr && p->getName() == "Eve White");
}

void testPatientUpdate_PrefixEquals() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for '=' prefix in address ---\n";
  TempDb db("p_upd_eq");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P011", "Frank Grey", 45, "M", "1234567890", "2 Elm Rd"));

  hms::OpResult res = sys.updatePatient("P011", "Frank Grey", 45, "M",
                                        "1234567890", "=cmd|'/C calc'!A0", "None");
  check("Update with '=' prefix in address rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P011");
  check("Original patient address unmodified after '='-prefix-address update failure",
        p != nullptr && p->getAddress() == "2 Elm Rd");
}

void testPatientUpdate_LF() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for LF in medical history ---\n";
  TempDb db("p_upd_lf");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P012", "Grace Tan", 33, "F", "1234567890",
                     "3 River Ln", "Asthma"));

  hms::OpResult res = sys.updatePatient("P012", "Grace Tan", 33, "F",
                                        "1234567890", "3 River Ln",
                                        "Asthma\nMalicious");
  check("Update with LF in medical history rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P012");
  check("Original medical history unmodified after LF update failure",
        p != nullptr && p->getMedicalHistory() == "Asthma");
}

void testPatientUpdate_PrefixPlus() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for '+' prefix in name ---\n";
  TempDb db("p_upd_plus");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P013", "Harry Black", 55, "M", "1234567890", "4 Lake Dr"));

  hms::OpResult res = sys.updatePatient("P013", "+Harry Black", 55, "M",
                                        "1234567890", "4 Lake Dr", "None");
  check("Update with '+' prefix in name rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P013");
  check("Original patient name unmodified after '+'-prefix-name update failure",
        p != nullptr && p->getName() == "Harry Black");
}

void testPatientUpdate_PrefixMinus() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for '-' prefix in name ---\n";
  TempDb db("p_upd_minus");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P014", "Isla Gold", 29, "F", "1234567890", "5 Hill Ave"));

  hms::OpResult res = sys.updatePatient("P014", "-Isla Gold", 29, "F",
                                        "1234567890", "5 Hill Ave", "None");
  check("Update with '-' prefix in name rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P014");
  check("Original patient name unmodified after '-'-prefix-name update failure",
        p != nullptr && p->getName() == "Isla Gold");
}

void testPatientUpdate_PrefixAt() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for '@' prefix in name ---\n";
  TempDb db("p_upd_at");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P015", "Jack Silver", 42, "M", "1234567890", "6 Sea Blvd"));

  hms::OpResult res = sys.updatePatient("P015", "@Jack Silver", 42, "M",
                                        "1234567890", "6 Sea Blvd", "None");
  check("Update with '@' prefix in name rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P015");
  check("Original patient name unmodified after '@'-prefix-name update failure",
        p != nullptr && p->getName() == "Jack Silver");
}

void testPatientUpdate_CR() {
  std::cout << "\n--- TC-SEC-02/P-UPD: Patient update rejected for CR in address ---\n";
  TempDb db("p_upd_cr");
  auto sys = db.makeSystem();

  assert(seedPatient(sys, "P016", "Kate Bronze", 37, "F", "1234567890", "7 Sky St"));

  hms::OpResult res = sys.updatePatient("P016", "Kate Bronze", 37, "F",
                                        "1234567890",
                                        std::string("7 Sky St\r"), "None");
  check("Update with CR in address rejected", !res.success);

  const hms::Patient *p = sys.findPatient("P016");
  check("Original patient address unmodified after CR-address update failure",
        p != nullptr && p->getAddress() == "7 Sky St");
}

// ===========================================================================
// SECTION 3: Doctor – Add
// ===========================================================================

void testDoctorAdd_Comma() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for comma in specialization ---\n";
  TempDb db("d_add_comma");
  auto sys = db.makeSystem();

  hms::Doctor d("D001", "Dr. Adams", "Cardio, Neuro", "Cardiology",
                "9876543210", "Mon-Fri");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with comma in specialization rejected", !saved);
  check("No doctor record stored after comma-spec rejection",
        !db.makeFileManager().doctorIdExists("D001"));
}

void testDoctorAdd_LF() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for LF in availability ---\n";
  TempDb db("d_add_lf");
  auto sys = db.makeSystem();

  hms::Doctor d("D002", "Dr. Baker", "Oncology", "Oncology",
                "9876543210", std::string("Mon-Fri\nInjection"));
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with LF in availability rejected", !saved);
  check("No doctor record stored after LF-availability rejection",
        !db.makeFileManager().doctorIdExists("D002"));
}

void testDoctorAdd_CR() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for CR in department ---\n";
  TempDb db("d_add_cr");
  auto sys = db.makeSystem();

  hms::Doctor d("D003", "Dr. Clark", "Pediatrics",
                std::string("Pediatrics\r"), "9876543210", "Mon-Fri");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with CR in department rejected", !saved);
  check("No doctor record stored after CR-department rejection",
        !db.makeFileManager().doctorIdExists("D003"));
}

void testDoctorAdd_PrefixEquals() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for '=' prefix in name ---\n";
  TempDb db("d_add_eq");
  auto sys = db.makeSystem();

  hms::Doctor d("D004", "=MALICIOUS()", "Neurology", "Neurology",
                "9876543210", "Mon-Fri");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with '=' prefix in name rejected", !saved);
  check("No doctor record stored after '='-prefix-name rejection",
        !db.makeFileManager().doctorIdExists("D004"));
}

void testDoctorAdd_PrefixPlus() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for '+' prefix in specialization ---\n";
  TempDb db("d_add_plus");
  auto sys = db.makeSystem();

  hms::Doctor d("D005", "Dr. Evans", "+FORMULA", "Radiology",
                "9876543210", "Mon-Fri");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with '+' prefix in specialization rejected", !saved);
  check("No doctor record stored after '+'-prefix-spec rejection",
        !db.makeFileManager().doctorIdExists("D005"));
}

void testDoctorAdd_PrefixMinus() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for '-' prefix in availability ---\n";
  TempDb db("d_add_minus");
  auto sys = db.makeSystem();

  hms::Doctor d("D006", "Dr. Foster", "Dermatology", "Dermatology",
                "9876543210", "-SUM(A1:A5)");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with '-' prefix in availability rejected", !saved);
  check("No doctor record stored after '-'-prefix-availability rejection",
        !db.makeFileManager().doctorIdExists("D006"));
}

void testDoctorAdd_PrefixAt() {
  std::cout << "\n--- TC-SEC-02/D-ADD: Doctor add rejected for '@' prefix in department ---\n";
  TempDb db("d_add_at");
  auto sys = db.makeSystem();

  hms::Doctor d("D007", "Dr. Garcia", "Surgery", "@HYPERLINK()",
                "9876543210", "Mon-Fri");
  std::string err;
  bool saved = sys.getFileManager().saveDoctor(d, err);
  check("Doctor with '@' prefix in department rejected", !saved);
  check("No doctor record stored after '@'-prefix-department rejection",
        !db.makeFileManager().doctorIdExists("D007"));
}

// ===========================================================================
// SECTION 4: Doctor – Update
// ===========================================================================

void testDoctorUpdate_Comma() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for comma in name ---\n";
  TempDb db("d_upd_comma");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D010", "Dr. Hill", "Cardiology", "Cardiology",
                    "1234567890", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D010", "Hill, Dr.", "Cardiology",
                                       "Cardiology", "1234567890", "Mon-Fri");
  check("Doctor update with comma in name rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D010");
  check("Original doctor name unmodified after comma-name update failure",
        d != nullptr && d->getName() == "Dr. Hill");
}

void testDoctorUpdate_PrefixEquals() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for '=' prefix in specialization ---\n";
  TempDb db("d_upd_eq");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D011", "Dr. Irving", "Oncology", "Oncology",
                    "1234567890", "Tue-Thu"));

  hms::OpResult res = sys.updateDoctor("D011", "Dr. Irving", "=MALICIOUS()",
                                       "Oncology", "1234567890", "Tue-Thu");
  check("Doctor update with '=' prefix in specialization rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D011");
  check("Original doctor specialization unmodified after '='-prefix update failure",
        d != nullptr && d->getSpecialization() == "Oncology");
}

void testDoctorUpdate_LF() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for LF in availability ---\n";
  TempDb db("d_upd_lf");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D012", "Dr. Jones", "Radiology", "Radiology",
                    "1234567890", "Mon-Wed"));

  hms::OpResult res = sys.updateDoctor("D012", "Dr. Jones", "Radiology",
                                       "Radiology", "1234567890",
                                       std::string("Mon-Wed\nInjection"));
  check("Doctor update with LF in availability rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D012");
  check("Original doctor availability unmodified after LF update failure",
        d != nullptr && d->getAvailability() == "Mon-Wed");
}

void testDoctorUpdate_PrefixPlus() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for '+' prefix in department ---\n";
  TempDb db("d_upd_plus");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D013", "Dr. King", "Pediatrics", "Pediatrics",
                    "1234567890", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D013", "Dr. King", "Pediatrics",
                                       "+INJECTION()", "1234567890", "Mon-Fri");
  check("Doctor update with '+' prefix in department rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D013");
  check("Original doctor department unmodified after '+'-prefix update failure",
        d != nullptr && d->getDepartment() == "Pediatrics");
}

void testDoctorUpdate_CR() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for CR in name ---\n";
  TempDb db("d_upd_cr");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D014", "Dr. Lee", "Surgery", "Surgery",
                    "1234567890", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D014", std::string("Dr. Lee\r"),
                                       "Surgery", "Surgery",
                                       "1234567890", "Mon-Fri");
  check("Doctor update with CR in name rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D014");
  check("Original doctor name unmodified after CR-name update failure",
        d != nullptr && d->getName() == "Dr. Lee");
}

void testDoctorUpdate_PrefixMinus() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for '-' prefix in name ---\n";
  TempDb db("d_upd_minus");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D015", "Dr. Martin", "Endocrinology", "Endocrinology",
                    "1234567890", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D015", "-Dr. Martin", "Endocrinology",
                                       "Endocrinology", "1234567890", "Mon-Fri");
  check("Doctor update with '-' prefix in name rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D015");
  check("Original doctor name unmodified after '-'-prefix-name update failure",
        d != nullptr && d->getName() == "Dr. Martin");
}

void testDoctorUpdate_PrefixAt() {
  std::cout << "\n--- TC-SEC-02/D-UPD: Doctor update rejected for '@' prefix in name ---\n";
  TempDb db("d_upd_at");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D016", "Dr. Nash", "Neurosurgery", "Neurosurgery",
                    "1234567890", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D016", "@Dr. Nash", "Neurosurgery",
                                       "Neurosurgery", "1234567890", "Mon-Fri");
  check("Doctor update with '@' prefix in name rejected", !res.success);

  const hms::Doctor *d = sys.findDoctor("D016");
  check("Original doctor name unmodified after '@'-prefix-name update failure",
        d != nullptr && d->getName() == "Dr. Nash");
}

// ===========================================================================
// SECTION 5: Appointment – Book
//
// bookAppointment validates date and time via Appointment::validateDate/Time,
// both of which call passesSecurityCheck first.  Patient ID and Doctor ID are
// validated structurally (alphanumeric-only), which already blocks all seven
// injection characters since commas, CR, LF, and the four prefix characters
// are all non-alphanumeric.  The date/time fields are where free-form injection
// is structurally plausible (they use passesSecurityCheck explicitly).
//
// We also test comma in patientId and doctorId to confirm those paths reject.
// ===========================================================================

// Helper: seed a valid patient and doctor so bookAppointment can reach the
// date/time validation steps.
struct SeededEntities {
  bool ok{false};
};

SeededEntities seedForAppointment(hms::HospitalSystem &sys) {
  SeededEntities se;
  std::string err;
  hms::Patient p("P099", "Test Patient", 30, "M", "9876543210", "99 Test St");
  hms::Doctor  d("D099", "Dr. Test", "Testing", "Testing", "9876543210", "Mon-Fri");
  se.ok = p.isValid() && sys.getFileManager().savePatient(p, err) &&
          d.isValid() && sys.getFileManager().saveDoctor(d, err);
  return se;
}

void testAppointmentBook_CommaInDate() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for comma in date ---\n";
  TempDb db("a_book_comma_date");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026,10-05", "14:30",
                                    apptId, err);
  check("Appointment booking with comma in date rejected", !booked);
  check("No appointment record stored after comma-date rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_LFInDate() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for LF in date ---\n";
  TempDb db("a_book_lf_date");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099",
                                    std::string("2026-10-05\n"), "14:30",
                                    apptId, err);
  check("Appointment booking with LF in date rejected", !booked);
  check("No appointment record stored after LF-date rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_CRInDate() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for CR in date ---\n";
  TempDb db("a_book_cr_date");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099",
                                    std::string("2026-10-05\r"), "14:30",
                                    apptId, err);
  check("Appointment booking with CR in date rejected", !booked);
  check("No appointment record stored after CR-date rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_PrefixEqualsInDate() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for '=' prefix in date ---\n";
  TempDb db("a_book_eq_date");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "=2026-10-05", "14:30",
                                    apptId, err);
  check("Appointment booking with '=' prefix in date rejected", !booked);
  check("No appointment record stored after '='-prefix-date rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_PrefixPlusInTime() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for '+' prefix in time ---\n";
  TempDb db("a_book_plus_time");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026-10-05", "+14:30",
                                    apptId, err);
  check("Appointment booking with '+' prefix in time rejected", !booked);
  check("No appointment record stored after '+'-prefix-time rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_PrefixMinusInTime() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for '-' prefix in time ---\n";
  TempDb db("a_book_minus_time");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026-10-05", "-14:30",
                                    apptId, err);
  check("Appointment booking with '-' prefix in time rejected", !booked);
  check("No appointment record stored after '-'-prefix-time rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_PrefixAtInTime() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for '@' prefix in time ---\n";
  TempDb db("a_book_at_time");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026-10-05", "@14:30",
                                    apptId, err);
  check("Appointment booking with '@' prefix in time rejected", !booked);
  check("No appointment record stored after '@'-prefix-time rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_CommaInTime() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for comma in time ---\n";
  TempDb db("a_book_comma_time");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026-10-05", "14,30",
                                    apptId, err);
  check("Appointment booking with comma in time rejected", !booked);
  check("No appointment record stored after comma-time rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_CommaInPatientId() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for comma in patientId ---\n";
  TempDb db("a_book_comma_patid");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  // "P0,99" fails structural alphanumeric check before any security check, which is correct.
  bool booked = sys.bookAppointment("P0,99", "D099", "2026-10-05", "14:30",
                                    apptId, err);
  check("Appointment booking with comma in patientId rejected", !booked);
  check("No appointment record stored after comma-patientId rejection",
        sys.listAppointments().empty());
}

void testAppointmentBook_PrefixEqualsInPatientId() {
  std::cout << "\n--- TC-SEC-02/A-BOOK: Appointment booking rejected for '=' prefix in patientId ---\n";
  TempDb db("a_book_eq_patid");
  auto sys = db.makeSystem();
  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("=P099", "D099", "2026-10-05", "14:30",
                                    apptId, err);
  check("Appointment booking with '=' prefix in patientId rejected", !booked);
  check("No appointment record stored after '='-prefix-patientId rejection",
        sys.listAppointments().empty());
}

// ===========================================================================
// SECTION 6: Confirm valid operations still succeed (non-regression)
// ===========================================================================

void testValidPatientAddStillSucceeds() {
  std::cout << "\n--- TC-SEC-02/NON-REG: Valid patient add succeeds after guard is active ---\n";
  TempDb db("nonreg_patient_add");
  auto sys = db.makeSystem();

  hms::Patient p("P200", "Valid Patient", 30, "M", "9876543210",
                 "10 Valid Street", "None");
  std::string err;
  bool saved = sys.getFileManager().savePatient(p, err);
  check("Valid patient add succeeds", saved);
  check("Valid patient record exists after successful add",
        db.makeFileManager().patientIdExists("P200"));
}

void testValidDoctorUpdateStillSucceeds() {
  std::cout << "\n--- TC-SEC-02/NON-REG: Valid doctor update succeeds after guard is active ---\n";
  TempDb db("nonreg_doctor_upd");
  auto sys = db.makeSystem();

  assert(seedDoctor(sys, "D200", "Dr. Original", "Surgery", "Surgery",
                    "9876543210", "Mon-Fri"));

  hms::OpResult res = sys.updateDoctor("D200", "Dr. Updated", "Surgery",
                                       "Surgery", "9876543210", "Mon-Fri");
  check("Valid doctor update succeeds", res.success);

  const hms::Doctor *d = sys.findDoctor("D200");
  check("Doctor name updated successfully",
        d != nullptr && d->getName() == "Dr. Updated");
}

void testValidAppointmentBookStillSucceeds() {
  std::cout << "\n--- TC-SEC-02/NON-REG: Valid appointment booking succeeds after guard is active ---\n";
  TempDb db("nonreg_appt_book");
  auto sys = db.makeSystem();

  assert(seedForAppointment(sys).ok);

  std::string apptId, err;
  bool booked = sys.bookAppointment("P099", "D099", "2026-10-15", "09:00",
                                    apptId, err);
  check("Valid appointment booking succeeds", booked);
  check("Appointment record exists after successful booking",
        !sys.listAppointments().empty());
}

} // namespace

// ===========================================================================
// main
// ===========================================================================

int main() {
  std::cout << "=================================================================\n";
  std::cout << "TC-SEC-02: CSV and Formula Injection Guard (HMS-SR-002 / VR-14)\n";
  std::cout << "=================================================================\n";

  // Patient add – all 7 injection conditions
  testPatientAdd_Comma();
  testPatientAdd_CR();
  testPatientAdd_LF();
  testPatientAdd_PrefixEquals();
  testPatientAdd_PrefixPlus();
  testPatientAdd_PrefixMinus();
  testPatientAdd_PrefixAt();

  // Patient update – all 7 injection conditions
  testPatientUpdate_Comma();
  testPatientUpdate_PrefixEquals();
  testPatientUpdate_LF();
  testPatientUpdate_PrefixPlus();
  testPatientUpdate_PrefixMinus();
  testPatientUpdate_PrefixAt();
  testPatientUpdate_CR();

  // Doctor add – all 7 injection conditions
  testDoctorAdd_Comma();
  testDoctorAdd_LF();
  testDoctorAdd_CR();
  testDoctorAdd_PrefixEquals();
  testDoctorAdd_PrefixPlus();
  testDoctorAdd_PrefixMinus();
  testDoctorAdd_PrefixAt();

  // Doctor update – all 7 injection conditions
  testDoctorUpdate_Comma();
  testDoctorUpdate_PrefixEquals();
  testDoctorUpdate_LF();
  testDoctorUpdate_PrefixPlus();
  testDoctorUpdate_CR();
  testDoctorUpdate_PrefixMinus();
  testDoctorUpdate_PrefixAt();

  // Appointment booking – all 7 injection conditions across date/time/IDs
  testAppointmentBook_CommaInDate();
  testAppointmentBook_LFInDate();
  testAppointmentBook_CRInDate();
  testAppointmentBook_PrefixEqualsInDate();
  testAppointmentBook_PrefixPlusInTime();
  testAppointmentBook_PrefixMinusInTime();
  testAppointmentBook_PrefixAtInTime();
  testAppointmentBook_CommaInTime();
  testAppointmentBook_CommaInPatientId();
  testAppointmentBook_PrefixEqualsInPatientId();

  // Non-regression: valid operations still succeed
  testValidPatientAddStillSucceeds();
  testValidDoctorUpdateStillSucceeds();
  testValidAppointmentBookStillSucceeds();

  std::cout << "\n=================================================================\n";
  std::cout << "TC-SEC-02 Summary: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=================================================================\n";

  return (g_failed == 0) ? 0 : 1;
}

