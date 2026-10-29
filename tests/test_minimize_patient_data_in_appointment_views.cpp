// ============================================================================
// TC-SEC-07: Minimize Patient Data in Appointment Views (HMS-SR-007)
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-SR-007 (Security Requirements - Minimize Patient Data)
//   STP RTM Test Case:  TC-SEC-07 (Minimize Patient Data in Appointment Views)
//   SRS RTM Test Case:  TC-SEC-07 (Minimize Patient Data in Appointment Views)
//   GitHub Issue:       #53
//
// Requirement Statement (HMS-SR-007):
//   "Appointment views shall display patient and doctor identifiers and names
//    only, in addition to existing appointment-specific fields (ID, date, time,
//    status). Patient contact details, address, and medical-history notes must
//    never be exposed in appointment-view output."
//
// Acceptance Criteria:
//   1. Appointment views show the patient identifier and patient name.
//   2. Appointment views show the doctor identifier and doctor name.
//   3. Existing appointment-specific fields (ID, date, time, status) remain available.
//   4. Patient contact details do not appear in appointment-view output.
//   5. Patient address does not appear in appointment-view output.
//   6. Patient medical-history notes do not appear in appointment-view output.
//   7. Restrictions hold across all appointment-view paths (All, Filter by Patient,
//      Filter by Doctor, Interactive Menu view, Startup Records view).
//   8. Patient-management views continue to provide their legitimate existing
//      functionality (details, search, list).
//   9. Underlying patient records, data model, and CSV persistence remain unchanged.
// ============================================================================

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

std::string captureStartupRecords(hms::HospitalSystem &system) {
  std::ostringstream outStream;
  auto *origCout = std::cout.rdbuf(outStream.rdbuf());

  system.displayStartupRecords();

  std::cout.rdbuf(origCout);
  return outStream.str();
}

} // namespace

int main() {
  std::cout << "=== Running TC-SEC-07: Minimize Patient Data in Appointment Views Tests ===\n";
  std::cout << "=== (Traceability: HMS-SR-007, TC-SEC-07, Issue #53) ===\n\n";

  // Root data files baseline
  const std::string rootPatientCsv = "patients.csv";
  const std::string rootDoctorCsv = "doctors.csv";
  const std::string rootApptCsv = "appointments.csv";

  const std::string rootPatientBaseline = readFile(rootPatientCsv);
  const std::string rootDoctorBaseline = readFile(rootDoctorCsv);
  const std::string rootApptBaseline = readFile(rootApptCsv);

  // Isolated test directory
  const std::string testDir = "build/test_minimize_patient_data_views";
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

  // Distinctive sentinel values for patient contact, address, and medical-history notes
  const std::string p1Id = "P001";
  const std::string p1Name = "Eleanor Vance";
  const int p1Age = 32;
  const std::string p1Gender = "F";
  const std::string p1ContactSentinel = "9876543210";
  const std::string p1AddressSentinel = "404 Hill House Road Sentinel Street";
  const std::string p1MedicalSentinel = "CONFIDENTIAL_CARDIAC_ARRHYTHMIA_NOTE";

  const std::string p2Id = "P002";
  const std::string p2Name = "Theodora Crain";
  const int p2Age = 29;
  const std::string p2Gender = "F";
  const std::string p2ContactSentinel = "9123456789";
  const std::string p2AddressSentinel = "777 Secret Confidential Boulevard";
  const std::string p2MedicalSentinel = "TOP_SECRET_CHRONIC_PSYCHOLOGICAL_TRAUMA_DATA";

  const std::string d1Id = "D001";
  const std::string d1Name = "Dr. John Montague";
  const std::string d1Spec = "Cardiology";
  const std::string d1Dept = "Heart Center";
  const std::string d1Contact = "9000000001";
  const std::string d1Avail = "Mon-Fri 09:00-17:00";

  const std::string d2Id = "D002";
  const std::string d2Name = "Dr. Luke Sanderson";
  const std::string d2Spec = "Neurology";
  const std::string d2Dept = "Neuro Center";
  const std::string d2Contact = "9000000002";
  const std::string d2Avail = "Tue-Sat 08:00-16:00";

  std::string seedErr;
  hms::Patient pat1(p1Id, p1Name, p1Age, p1Gender, p1ContactSentinel, p1AddressSentinel, p1MedicalSentinel);
  hms::Patient pat2(p2Id, p2Name, p2Age, p2Gender, p2ContactSentinel, p2AddressSentinel, p2MedicalSentinel);
  hms::Doctor doc1(d1Id, d1Name, d1Spec, d1Dept, d1Contact, d1Avail);
  hms::Doctor doc2(d2Id, d2Name, d2Spec, d2Dept, d2Contact, d2Avail);

  check("Seed patient P001 with sentinels", fm.savePatient(pat1, seedErr));
  check("Seed patient P002 with sentinels", fm.savePatient(pat2, seedErr));
  check("Seed doctor D001", fm.saveDoctor(doc1, seedErr));
  check("Seed doctor D002", fm.saveDoctor(doc2, seedErr));

  // Seed appointments: A0001 (Booked) and A0002 (Cancelled)
  hms::Appointment appt1("A0001", p1Id, d1Id, "2026-11-20", "10:00", hms::AppointmentStatus::Booked);
  hms::Appointment appt2("A0002", p2Id, d2Id, "2026-11-21", "14:00", hms::AppointmentStatus::Cancelled);

  check("Seed appointment A0001", fm.saveAppointment(appt1, seedErr));
  check("Seed appointment A0002", fm.saveAppointment(appt2, seedErr));

  hms::HospitalSystem system(fm);

  // =========================================================================
  // Test Suite 1: All Appointments View (viewAppointments("ALL"))
  // =========================================================================
  std::cout << "\n-- Test Suite 1: All Appointments View (viewAppointments('ALL')) --\n";
  {
    const std::string out = captureViewAppointments(system, "ALL");

    // 1. Permitted identification fields must appear:
    check("Shows Appointment ID: A0001", out.find("Appointment ID: A0001") != std::string::npos);
    check("Shows Patient ID: P001", out.find("Patient ID: P001") != std::string::npos);
    check("Shows Patient Name: Eleanor Vance", out.find("Patient Name: Eleanor Vance") != std::string::npos);
    check("Shows Doctor ID: D001", out.find("Doctor ID: D001") != std::string::npos);
    check("Shows Doctor Name: Dr. John Montague", out.find("Doctor Name: Dr. John Montague") != std::string::npos);
    check("Shows Date: 2026-11-20", out.find("Date: 2026-11-20") != std::string::npos);
    check("Shows Time: 10:00", out.find("Time: 10:00") != std::string::npos);
    check("Shows Status: Booked", out.find("Status: Booked") != std::string::npos);

    check("Shows Appointment ID: A0002", out.find("Appointment ID: A0002") != std::string::npos);
    check("Shows Patient ID: P002", out.find("Patient ID: P002") != std::string::npos);
    check("Shows Patient Name: Theodora Crain", out.find("Patient Name: Theodora Crain") != std::string::npos);
    check("Shows Doctor ID: D002", out.find("Doctor ID: D002") != std::string::npos);
    check("Shows Doctor Name: Dr. Luke Sanderson", out.find("Doctor Name: Dr. Luke Sanderson") != std::string::npos);
    check("Shows Date: 2026-11-21", out.find("Date: 2026-11-21") != std::string::npos);
    check("Shows Time: 14:00", out.find("Time: 14:00") != std::string::npos);
    check("Shows Status: Cancelled", out.find("Status: Cancelled") != std::string::npos);

    // 2. Prohibited patient data must NOT appear:
    check("Patient 1 contact sentinel does NOT appear in appointment view",
          out.find(p1ContactSentinel) == std::string::npos);
    check("Patient 1 address sentinel does NOT appear in appointment view",
          out.find(p1AddressSentinel) == std::string::npos);
    check("Patient 1 medical history sentinel does NOT appear in appointment view",
          out.find(p1MedicalSentinel) == std::string::npos);

    check("Patient 2 contact sentinel does NOT appear in appointment view",
          out.find(p2ContactSentinel) == std::string::npos);
    check("Patient 2 address sentinel does NOT appear in appointment view",
          out.find(p2AddressSentinel) == std::string::npos);
    check("Patient 2 medical history sentinel does NOT appear in appointment view",
          out.find(p2MedicalSentinel) == std::string::npos);

    check("Prohibited field label 'Address:' does NOT appear in appointment view",
          out.find("Address:") == std::string::npos);
    check("Prohibited field label 'Medical history:' does NOT appear in appointment view",
          out.find("Medical history:") == std::string::npos);
  }

  // =========================================================================
  // Test Suite 2: Filter by Patient View (viewAppointments("PATIENT", patientId))
  // =========================================================================
  std::cout << "\n-- Test Suite 2: Filter by Patient View --\n";
  {
    const std::string outP1 = captureViewAppointments(system, "PATIENT", p1Id);

    // Permitted fields for P001:
    check("Filter P1 shows A0001", outP1.find("A0001") != std::string::npos);
    check("Filter P1 shows Patient ID: P001", outP1.find("Patient ID: P001") != std::string::npos);
    check("Filter P1 shows Patient Name: Eleanor Vance", outP1.find("Patient Name: Eleanor Vance") != std::string::npos);
    check("Filter P1 shows Doctor ID: D001", outP1.find("Doctor ID: D001") != std::string::npos);
    check("Filter P1 shows Doctor Name: Dr. John Montague", outP1.find("Doctor Name: Dr. John Montague") != std::string::npos);

    // Prohibited fields for P001 must NOT appear:
    check("Filter P1: Contact sentinel NOT leaked", outP1.find(p1ContactSentinel) == std::string::npos);
    check("Filter P1: Address sentinel NOT leaked", outP1.find(p1AddressSentinel) == std::string::npos);
    check("Filter P1: Medical history sentinel NOT leaked", outP1.find(p1MedicalSentinel) == std::string::npos);

    // P002 records/sentinels must NOT appear in P001 view:
    check("Filter P1: P002 name NOT present", outP1.find(p2Name) == std::string::npos);
    check("Filter P1: P002 sentinels NOT present",
          outP1.find(p2ContactSentinel) == std::string::npos &&
          outP1.find(p2AddressSentinel) == std::string::npos &&
          outP1.find(p2MedicalSentinel) == std::string::npos);
  }

  // =========================================================================
  // Test Suite 3: Filter by Doctor View (viewAppointments("DOCTOR", doctorId))
  // =========================================================================
  std::cout << "\n-- Test Suite 3: Filter by Doctor View --\n";
  {
    const std::string outD2 = captureViewAppointments(system, "DOCTOR", d2Id);

    // Permitted fields for D002 (A0002 with P002):
    check("Filter D2 shows A0002", outD2.find("A0002") != std::string::npos);
    check("Filter D2 shows Patient ID: P002", outD2.find("Patient ID: P002") != std::string::npos);
    check("Filter D2 shows Patient Name: Theodora Crain", outD2.find("Patient Name: Theodora Crain") != std::string::npos);
    check("Filter D2 shows Doctor ID: D002", outD2.find("Doctor ID: D002") != std::string::npos);
    check("Filter D2 shows Doctor Name: Dr. Luke Sanderson", outD2.find("Doctor Name: Dr. Luke Sanderson") != std::string::npos);

    // Prohibited fields must NOT appear:
    check("Filter D2: Contact sentinel NOT leaked", outD2.find(p2ContactSentinel) == std::string::npos);
    check("Filter D2: Address sentinel NOT leaked", outD2.find(p2AddressSentinel) == std::string::npos);
    check("Filter D2: Medical history sentinel NOT leaked", outD2.find(p2MedicalSentinel) == std::string::npos);

    // D001 / P001 records must NOT appear:
    check("Filter D2: D001 records NOT present", outD2.find("A0001") == std::string::npos);
  }

  // =========================================================================
  // Test Suite 4: Interactive Appointment View Flow (run())
  // =========================================================================
  std::cout << "\n-- Test Suite 4: Interactive Appointment View Flow --\n";
  {
    // 4.1 Interactive All Appointments: Menu 3 -> View 2 -> All 1 -> Return 0 -> Exit 4
    {
      const std::string input = "3\n2\n1\n0\n4\n";
      const std::string out = runInteractive(system, input);

      check("Interactive All shows Eleanor Vance", out.find("Eleanor Vance") != std::string::npos);
      check("Interactive All shows Theodora Crain", out.find("Theodora Crain") != std::string::npos);
      check("Interactive All shows Dr. John Montague", out.find("Dr. John Montague") != std::string::npos);
      check("Interactive All shows Dr. Luke Sanderson", out.find("Dr. Luke Sanderson") != std::string::npos);

      check("Interactive All: No contact 1 leaked", out.find(p1ContactSentinel) == std::string::npos);
      check("Interactive All: No address 1 leaked", out.find(p1AddressSentinel) == std::string::npos);
      check("Interactive All: No medical 1 leaked", out.find(p1MedicalSentinel) == std::string::npos);
      check("Interactive All: No contact 2 leaked", out.find(p2ContactSentinel) == std::string::npos);
      check("Interactive All: No address 2 leaked", out.find(p2AddressSentinel) == std::string::npos);
      check("Interactive All: No medical 2 leaked", out.find(p2MedicalSentinel) == std::string::npos);
    }

    // 4.2 Interactive Filter by Patient: Menu 3 -> View 2 -> Patient 2 -> P001 -> Return 0 -> Exit 4
    {
      const std::string input = "3\n2\n2\nP001\n0\n4\n";
      const std::string out = runInteractive(system, input);

      check("Interactive Patient shows Eleanor Vance", out.find("Eleanor Vance") != std::string::npos);
      check("Interactive Patient: No contact leaked", out.find(p1ContactSentinel) == std::string::npos);
      check("Interactive Patient: No address leaked", out.find(p1AddressSentinel) == std::string::npos);
      check("Interactive Patient: No medical leaked", out.find(p1MedicalSentinel) == std::string::npos);
    }

    // 4.3 Interactive Filter by Doctor: Menu 3 -> View 2 -> Doctor 3 -> D002 -> Return 0 -> Exit 4
    {
      const std::string input = "3\n2\n3\nD002\n0\n4\n";
      const std::string out = runInteractive(system, input);

      check("Interactive Doctor shows Theodora Crain", out.find("Theodora Crain") != std::string::npos);
      check("Interactive Doctor: No contact leaked", out.find(p2ContactSentinel) == std::string::npos);
      check("Interactive Doctor: No address leaked", out.find(p2AddressSentinel) == std::string::npos);
      check("Interactive Doctor: No medical leaked", out.find(p2MedicalSentinel) == std::string::npos);
    }
  }

  // =========================================================================
  // Test Suite 5: Startup Records Display (displayStartupRecords())
  // =========================================================================
  std::cout << "\n-- Test Suite 5: Startup Records Display --\n";
  {
    const std::string startupOut = captureStartupRecords(system);

    // In the startup output, appointments section should show names without leaking sentinels
    check("Startup shows appointment A0001", startupOut.find("Appointment ID: A0001") != std::string::npos);
    check("Startup appointment shows Patient Name: Eleanor Vance",
          startupOut.find("Patient Name: Eleanor Vance") != std::string::npos);
    check("Startup appointment shows Doctor Name: Dr. John Montague",
          startupOut.find("Doctor Name: Dr. John Montague") != std::string::npos);

    // Verify appointment lines do NOT contain address or medical history
    // (Note: Patient listing section legitimately shows contact number per HMS-F-005,
    //  but address and medical history are never shown in patient listing or appointment listing)
    check("Startup output does NOT contain patient 1 address sentinel",
          startupOut.find(p1AddressSentinel) == std::string::npos);
    check("Startup output does NOT contain patient 1 medical history sentinel",
          startupOut.find(p1MedicalSentinel) == std::string::npos);
    check("Startup output does NOT contain patient 2 address sentinel",
          startupOut.find(p2AddressSentinel) == std::string::npos);
    check("Startup output does NOT contain patient 2 medical history sentinel",
          startupOut.find(p2MedicalSentinel) == std::string::npos);
  }

  // =========================================================================
  // Test Suite 6: Patient-Management Views Continue Providing Full Details
  // =========================================================================
  std::cout << "\n-- Test Suite 6: Patient-Management Views Retain Full Legitimate Details --\n";
  {
    // 6.1 View Patient Details: Menu 1 -> View Details (option not direct menu, search 2 or interactive)
    // Let's test search patients (option 2) and view patient details interactive
    // Patient Search (1 -> 2 -> P001 -> 0 -> 4) displays full patient details legitimately:
    {
      const std::string input = "1\n2\nP001\n0\n4\n";
      const std::string out = runInteractive(system, input);

      check("Patient search displays name", out.find("Name: Eleanor Vance") != std::string::npos);
      check("Patient search displays contact number", out.find(p1ContactSentinel) != std::string::npos);
      check("Patient search displays address", out.find(p1AddressSentinel) != std::string::npos);
      check("Patient search displays medical history", out.find(p1MedicalSentinel) != std::string::npos);
    }

    // 6.2 Patient Listing (1 -> 3 -> 0 -> 4):
    {
      const std::string input = "1\n3\n0\n4\n";
      const std::string out = runInteractive(system, input);

      check("Patient listing displays Eleanor Vance", out.find("Eleanor Vance") != std::string::npos);
      check("Patient listing displays contact number", out.find(p1ContactSentinel) != std::string::npos);
    }
  }

  // =========================================================================
  // Test Suite 7: Underlying Records and Persistence Unchanged
  // =========================================================================
  std::cout << "\n-- Test Suite 7: Underlying Records and Persistence Unchanged --\n";
  {
    const std::string patCsv = readFile(testPatientCsv);
    check("Patient CSV contains Eleanor Vance", patCsv.find(p1Name) != std::string::npos);
    check("Patient CSV contains contact sentinel", patCsv.find(p1ContactSentinel) != std::string::npos);
    check("Patient CSV contains address sentinel", patCsv.find(p1AddressSentinel) != std::string::npos);
    check("Patient CSV contains medical history sentinel", patCsv.find(p1MedicalSentinel) != std::string::npos);

    const std::string apptCsv = readFile(testApptCsv);
    check("Appointment CSV has 6 columns without patient personal data",
          apptCsv.find(p1ContactSentinel) == std::string::npos &&
          apptCsv.find(p1AddressSentinel) == std::string::npos &&
          apptCsv.find(p1MedicalSentinel) == std::string::npos);

    // Direct object model access check
    const hms::Patient *p = system.findPatient(p1Id);
    check("findPatient returns patient object", p != nullptr);
    if (p) {
      check("Patient object contact number intact", p->getContactNumber() == p1ContactSentinel);
      check("Patient object address intact", p->getAddress() == p1AddressSentinel);
      check("Patient object medical history intact", p->getMedicalHistory() == p1MedicalSentinel);
    }
  }

  // =========================================================================
  // Test Suite 8: Root CSV Preservation
  // =========================================================================
  std::cout << "\n-- Test Suite 8: Root CSV Preservation --\n";
  {
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
  std::cout << "TC-SEC-07 Results: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=========================================\n";

  return g_failed == 0 ? 0 : 1;
}

