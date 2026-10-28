// ============================================================================
// TC-SEC-01: Input Validation Against Allow-List (HMS-SR-001)
// ============================================================================
// Traceability:
//   Requirement ID:     HMS-SR-001 (Security Requirements - Input Validation)
//   SRS Reference:      Table 4-1 (Input Validation Rules VR-01 to VR-14)
//   STP RTM Test Case:  TC-SEC-01 (Table 8 & Section 9, Software Test Plan)
//   GitHub Issue:       #47
//
// Objective:
//   Implement robust allow-list (accept-list) validation for keyboard input in
//   the existing Hospital Management System C++17 console application.
//
// Requirement Statement (HMS-SR-001):
//   "Validate keyboard input type, length, format and range against Table 4-1 before use."
//
// Acceptance Criterion:
//   "Boundary values must be accepted or rejected exactly as defined in Table 4-1."
//
// Table 4-1 Specification & Boundaries Verified:
//   - VR-01 (Patient ID / Doctor ID): 1-10 alphanumeric characters.
//     * Min length 1: accepted ("P", "1")
//     * Below min (empty): rejected ("")
//     * Max length 10: accepted ("P123456789")
//     * Above max 11: rejected ("P1234567890")
//     * Disallowed chars (spaces, hyphens, punctuation, symbols, delimiters, formula prefixes): rejected
//   - VR-02 (Name): 1-50 characters [A-Za-z .'-].
//     * Min length 1: accepted ("A")
//     * Below min (empty): rejected ("")
//     * Max length 50: accepted (50 permitted characters)
//     * Above max 51: rejected (51 permitted characters)
//     * Disallowed chars (digits, symbols, delimiters, formula prefixes): rejected
//   - VR-03 (Age): Whole number 0 to 120 (inclusive).
//     * Min value 0: accepted
//     * Below min (-1): rejected
//     * Max value 120: accepted
//     * Above max (121, 999): rejected
//     * Disallowed types (non-numeric, decimals, formula prefixes, empty): rejected
//   - VR-04 (Gender): Single letter 'M', 'F', or 'O' (case-insensitive).
//     * Valid values: "M", "F", "O", "m", "f", "o" accepted
//     * Below min (empty): rejected
//     * Above max ("Male", "Female", "Other", "MF"): rejected
//     * Disallowed letters ("X", "A", "Z"), digits, symbols, formula prefixes: rejected
//   - VR-05 (Contact Number): Exactly 10 digits.
//     * Min/Max exact length 10: accepted ("9876543210")
//     * Below min (9 digits): rejected ("987654321")
//     * Above max (11 digits): rejected ("98765432101")
//     * Non-digits, symbols, delimiters, formula prefixes: rejected
//   - VR-06 (Address): 1-100 characters.
//     * Min length 1: accepted ("A")
//     * Below min (empty): rejected ("")
//     * Max length 100: accepted (100 characters)
//     * Above max 101: rejected (101 characters)
//     * Delimiters, formula prefixes: rejected
//   - VR-07 (Medical History Note): Optional, 0-200 characters.
//     * Empty (0 characters): accepted
//     * Min non-empty 1: accepted ("A")
//     * Max length 200: accepted (200 characters)
//     * Above max 201: rejected (201 characters)
//     * Delimiters, formula prefixes (when non-empty): rejected
//   - VR-08 (Specialization & Department): 1-40 characters each.
//     * Min length 1: accepted ("A")
//     * Below min (empty): rejected ("")
//     * Max length 40: accepted (40 characters)
//     * Above max 41: rejected (41 characters)
//     * Delimiters, formula prefixes: rejected
//   - VR-09 (Availability Information): 1-60 characters free text.
//     * Min length 1: accepted ("M")
//     * Below min (empty): rejected ("")
//     * Max length 60: accepted (60 characters)
//     * Above max 61: rejected (61 characters)
//     * Delimiters, formula prefixes: rejected
//   - VR-10 (Appointment Date): Calendar date, YYYY-MM-DD. Valid date: month 01-12,
//     day valid for month and leap year (2000-2100).
//     * Min year 2000 ("2000-01-01"): accepted
//     * Below min year 1999 ("1999-12-31"): rejected
//     * Max year 2100 ("2100-12-31"): accepted
//     * Above max year 2101 ("2101-01-01"): rejected
//     * Leap years: 2000-02-29 accepted, 2024-02-29 accepted, 2026-02-29 rejected, 2100-02-29 rejected
//     * Month boundaries: 00 rejected, 01 accepted, 12 accepted, 13 rejected
//     * Day boundaries: 00 rejected, 01 accepted, 31 accepted (Jan), 32 rejected (Jan), 31 rejected (Apr)
//     * Format, non-numeric, delimiters, formula prefixes: rejected
//   - VR-11 (Appointment Time): HH:MM, 24-hour clock (00:00 to 23:59).
//     * Min time "00:00": accepted
//     * Max time "23:59": accepted
//     * Above max hour "24:00", "25:00": rejected
//     * Above max minute "12:60": rejected
//     * Format violations ("9:5", "09:5", "9:05", "09-30", "09:30:00"): rejected
//     * Delimiters, formula prefixes: rejected
//   - VR-12 (Appointment ID): Prefix 'A' + at least 4 digits.
//     * Min length 5 ("A0001"): accepted
//     * More digits ("A9999", "A10000"): accepted
//     * Below min length 4 ("A001"): rejected
//     * Empty (""), prefix-only ("A"), wrong prefix ("B0001", "10001"): rejected
//     * Non-digits after prefix ("A000a"), formula prefixes, delimiters: rejected
//   - VR-13 (Appointment Status): Exactly "Booked" or "Cancelled".
//     * "Booked", "Cancelled": accepted
//     * Case variation ("booked", "CANCELLED"), empty (""), invalid ("Pending"): rejected
//   - VR-14 (Text-safety Rule): Must not contain commas, CR, LF, or begin with =, +, -, @.
//     * Verified across all fields.
//   - Safe Stream Handling & Keyboard Recovery:
//     * Simulated interactive input for non-numeric age, formula injection, and recovery.
//     * Verified using isolated temporary directory so production CSV data is never touched.
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

// Helper to generate string of specific length with fill char
std::string makeString(std::size_t len, char fill = 'a') {
  return std::string(len, fill);
}

// ---------------------------------------------------------------------------
// 1. Table 4-1 VR-01: Patient ID & Doctor ID
// ---------------------------------------------------------------------------
void testVR01_EntityIds() {
  std::cout << "\n--- Testing VR-01: Patient ID & Doctor ID (1-10 alphanumeric characters) ---\n";
  std::string err;

  // Boundary: Min length 1
  check("Patient ID min length (1 char alphanumeric 'P'): Accepted",
        hms::Patient::validateId("P", err));
  check("Doctor ID min length (1 char alphanumeric 'D'): Accepted",
        hms::Doctor::validateId("D", err));
  check("Patient ID numeric digit '1': Accepted",
        hms::Patient::validateId("1", err));

  // Boundary: Below min length 0 (Empty)
  check("Patient ID empty string: Rejected",
        !hms::Patient::validateId("", err));
  check("Doctor ID empty string: Rejected",
        !hms::Doctor::validateId("", err));

  // Boundary: Max length 10
  check("Patient ID max length (10 chars 'P123456789'): Accepted",
        hms::Patient::validateId("P123456789", err));
  check("Doctor ID max length (10 chars 'D123456789'): Accepted",
        hms::Doctor::validateId("D123456789", err));

  // Boundary: Above max length 11
  check("Patient ID above max length (11 chars): Rejected",
        !hms::Patient::validateId("P1234567890", err));
  check("Doctor ID above max length (11 chars): Rejected",
        !hms::Doctor::validateId("D1234567890", err));

  // Disallowed characters
  check("Patient ID with space ('P 001'): Rejected",
        !hms::Patient::validateId("P 001", err));
  check("Patient ID with hyphen ('P-001'): Rejected",
        !hms::Patient::validateId("P-001", err));
  check("Patient ID with dot ('P.001'): Rejected",
        !hms::Patient::validateId("P.001", err));
  check("Patient ID with special char ('P@001'): Rejected",
        !hms::Patient::validateId("P@001", err));
  check("Doctor ID with space ('D 001'): Rejected",
        !hms::Doctor::validateId("D 001", err));
  check("Doctor ID with hyphen ('D-001'): Rejected",
        !hms::Doctor::validateId("D-001", err));
  check("Doctor ID with special char ('D#001'): Rejected",
        !hms::Doctor::validateId("D#001", err));

  // VR-14: Delimiters and formula prefixes
  check("Patient ID with comma delimiter ('P1,2'): Rejected",
        !hms::Patient::validateId("P1,2", err));
  check("Patient ID with newline ('P1\\n'): Rejected",
        !hms::Patient::validateId("P1\n", err));
  check("Patient ID with formula prefix '=' ('=P1'): Rejected",
        !hms::Patient::validateId("=P1", err));
  check("Patient ID with formula prefix '+' ('+P1'): Rejected",
        !hms::Patient::validateId("+P1", err));
  check("Patient ID with formula prefix '-' ('-P1'): Rejected",
        !hms::Patient::validateId("-P1", err));
  check("Patient ID with formula prefix '@' ('@P1'): Rejected",
        !hms::Patient::validateId("@P1", err));
  check("Doctor ID with formula prefix '=' ('=D1'): Rejected",
        !hms::Doctor::validateId("=D1", err));
}

// ---------------------------------------------------------------------------
// 2. Table 4-1 VR-02: Name (Patient & Doctor)
// ---------------------------------------------------------------------------
void testVR02_Name() {
  std::cout << "\n--- Testing VR-02: Name (1-50 chars; letters, spaces, '.', '-', ''') ---\n";
  std::string err;

  // Boundary: Min length 1
  check("Patient Name min length (1 letter 'A'): Accepted",
        hms::Patient::validateName("A", err));
  check("Doctor Name min length (1 letter 'D'): Accepted",
        hms::Doctor::validateName("D", err));

  // Boundary: Below min length 0 (Empty)
  check("Patient Name empty string: Rejected",
        !hms::Patient::validateName("", err));
  check("Doctor Name empty string: Rejected",
        !hms::Doctor::validateName("", err));

  // Boundary: Max length 50
  const std::string name50(50, 'A');
  check("Patient Name max length (50 characters): Accepted",
        hms::Patient::validateName(name50, err));
  check("Doctor Name max length (50 characters): Accepted",
        hms::Doctor::validateName(name50, err));

  // Boundary: Above max length 51
  const std::string name51(51, 'A');
  check("Patient Name above max length (51 characters): Rejected",
        !hms::Patient::validateName(name51, err));
  check("Doctor Name above max length (51 characters): Rejected",
        !hms::Doctor::validateName(name51, err));

  // Allowed punctuation: letters, spaces, '.', '-', '\''
  check("Patient Name with allowed punctuation (\"Mary-Jane O'Connor, Jr.\" without comma -> \"Mary-Jane O'Connor Jr.\"): Accepted",
        hms::Patient::validateName("Mary-Jane O'Connor Jr.", err));
  check("Doctor Name with title (\"Dr. John Smith\"): Accepted",
        hms::Doctor::validateName("Dr. John Smith", err));

  // Disallowed characters
  check("Patient Name with digits ('John2'): Rejected",
        !hms::Patient::validateName("John2", err));
  check("Doctor Name with digits ('Dr. Smith 3rd'): Rejected",
        !hms::Doctor::validateName("Dr. Smith 3rd", err));
  check("Patient Name with symbols ('John@Doe'): Rejected",
        !hms::Patient::validateName("John@Doe", err));
  check("Patient Name with exclamation ('Jane!'): Rejected",
        !hms::Patient::validateName("Jane!", err));

  // VR-14: Delimiters and formula prefixes
  check("Patient Name with comma ('Smith, John'): Rejected",
        !hms::Patient::validateName("Smith, John", err));
  check("Patient Name with newline ('John\\nDoe'): Rejected",
        !hms::Patient::validateName("John\nDoe", err));
  check("Patient Name with formula prefix '=' ('=John'): Rejected",
        !hms::Patient::validateName("=John", err));
  check("Doctor Name with formula prefix '+' ('+Dr. Smith'): Rejected",
        !hms::Doctor::validateName("+Dr. Smith", err));
  check("Patient Name with formula prefix '-' ('-Alice'): Rejected",
        !hms::Patient::validateName("-Alice", err));
  check("Patient Name with formula prefix '@' ('@Bob'): Rejected",
        !hms::Patient::validateName("@Bob", err));
}

// ---------------------------------------------------------------------------
// 3. Table 4-1 VR-03: Age
// ---------------------------------------------------------------------------
void testVR03_Age() {
  std::cout << "\n--- Testing VR-03: Age (Whole number 0 to 120 inclusive) ---\n";
  std::string err;

  // Boundary: Min value 0
  check("Age min value (0): Accepted",
        hms::Patient::validateAge(0, err));

  // Boundary: Below min value (-1)
  check("Age below min (-1): Rejected",
        !hms::Patient::validateAge(-1, err));
  check("Age negative (-50): Rejected",
        !hms::Patient::validateAge(-50, err));

  // Boundary: Max value 120
  check("Age max value (120): Accepted",
        hms::Patient::validateAge(120, err));

  // Boundary: Above max value (121, 999)
  check("Age above max (121): Rejected",
        !hms::Patient::validateAge(121, err));
  check("Age far above max (999): Rejected",
        !hms::Patient::validateAge(999, err));

  // String age input validation (VR-03: digits-only, range 0-120, no arbitrary length limit)
  int parsedAge = -1;
  check("String age min boundary (\"0\"): Accepted",
        hms::Patient::validateAge("0", parsedAge, err) && parsedAge == 0);
  check("String age max boundary (\"120\"): Accepted",
        hms::Patient::validateAge("120", parsedAge, err) && parsedAge == 120);
  check("String age typical (\"25\"): Accepted",
        hms::Patient::validateAge("25", parsedAge, err) && parsedAge == 25);
  check("String age with leading zeros (\"025\"): Accepted",
        hms::Patient::validateAge("025", parsedAge, err) && parsedAge == 25);
  check("String age with multiple zeros (\"00\"): Accepted",
        hms::Patient::validateAge("00", parsedAge, err) && parsedAge == 0);
  check("String age boundary with leading zeros (\"00120\"): Accepted",
        hms::Patient::validateAge("00120", parsedAge, err) && parsedAge == 120);

  // String age rejections
  check("String age empty (\"\"): Rejected",
        !hms::Patient::validateAge("", parsedAge, err));
  check("String age above max (\"121\"): Rejected",
        !hms::Patient::validateAge("121", parsedAge, err));
  check("String age above max with leading zeros (\"0121\"): Rejected",
        !hms::Patient::validateAge("0121", parsedAge, err));
  check("String age non-digit (\"abc\"): Rejected",
        !hms::Patient::validateAge("abc", parsedAge, err));
  check("String age formula prefix (\"+25\"): Rejected",
        !hms::Patient::validateAge("+25", parsedAge, err));
  check("String age negative sign (\"-1\"): Rejected",
        !hms::Patient::validateAge("-1", parsedAge, err));
  check("String age decimal (\"25.5\"): Rejected",
        !hms::Patient::validateAge("25.5", parsedAge, err));
  check("String age with spaces (\" 25\"): Rejected",
        !hms::Patient::validateAge(" 25", parsedAge, err));
  check("String age very large overflow (\"99999999999999999999999999999999\"): Safely Rejected",
        !hms::Patient::validateAge("99999999999999999999999999999999", parsedAge, err));
}

// ---------------------------------------------------------------------------
// 4. Table 4-1 VR-04: Gender
// ---------------------------------------------------------------------------
void testVR04_Gender() {
  std::cout << "\n--- Testing VR-04: Gender (Single letter 'M', 'F', 'O', case-insensitive) ---\n";
  std::string err;

  // Valid values
  check("Gender 'M': Accepted",
        hms::Patient::validateGender("M", err));
  check("Gender 'F': Accepted",
        hms::Patient::validateGender("F", err));
  check("Gender 'O': Accepted",
        hms::Patient::validateGender("O", err));
  check("Gender 'm' (lowercase): Accepted",
        hms::Patient::validateGender("m", err));
  check("Gender 'f' (lowercase): Accepted",
        hms::Patient::validateGender("f", err));
  check("Gender 'o' (lowercase): Accepted",
        hms::Patient::validateGender("o", err));

  // Disallowed: Empty
  check("Gender empty string: Rejected",
        !hms::Patient::validateGender("", err));

  // Disallowed: Words / multi-character
  check("Gender 'Male': Rejected",
        !hms::Patient::validateGender("Male", err));
  check("Gender 'Female': Rejected",
        !hms::Patient::validateGender("Female", err));
  check("Gender 'Other': Rejected",
        !hms::Patient::validateGender("Other", err));
  check("Gender 'MF': Rejected",
        !hms::Patient::validateGender("MF", err));

  // Disallowed: Other characters
  check("Gender 'X': Rejected",
        !hms::Patient::validateGender("X", err));
  check("Gender 'A': Rejected",
        !hms::Patient::validateGender("A", err));
  check("Gender '1': Rejected",
        !hms::Patient::validateGender("1", err));
  check("Gender '?': Rejected",
        !hms::Patient::validateGender("?", err));

  // VR-14: Delimiters and formula prefixes
  check("Gender with comma (','): Rejected",
        !hms::Patient::validateGender(",", err));
  check("Gender with formula prefix '=' ('=M'): Rejected",
        !hms::Patient::validateGender("=M", err));
  check("Gender with formula prefix '+' ('+F'): Rejected",
        !hms::Patient::validateGender("+F", err));
}

// ---------------------------------------------------------------------------
// 5. Table 4-1 VR-05: Contact Number (Patient & Doctor)
// ---------------------------------------------------------------------------
void testVR05_ContactNumber() {
  std::cout << "\n--- Testing VR-05: Contact Number (Exactly 10 digits) ---\n";
  std::string err;

  // Boundary: Exact length 10 digits
  check("Patient Contact Number exact 10 digits ('9876543210'): Accepted",
        hms::Patient::validateContactNumber("9876543210", err));
  check("Doctor Contact Number exact 10 digits ('0123456789'): Accepted",
        hms::Doctor::validateContactNumber("0123456789", err));

  // Boundary: Below exact length (9 digits)
  check("Patient Contact Number 9 digits ('987654321'): Rejected",
        !hms::Patient::validateContactNumber("987654321", err));
  check("Doctor Contact Number 9 digits ('012345678'): Rejected",
        !hms::Doctor::validateContactNumber("012345678", err));

  // Boundary: Above exact length (11 digits)
  check("Patient Contact Number 11 digits ('98765432101'): Rejected",
        !hms::Patient::validateContactNumber("98765432101", err));
  check("Doctor Contact Number 11 digits ('01234567890'): Rejected",
        !hms::Doctor::validateContactNumber("01234567890", err));

  // Empty string
  check("Contact Number empty string: Rejected",
        !hms::Patient::validateContactNumber("", err));

  // Disallowed: Non-digit characters
  check("Contact Number with letters ('987654321a'): Rejected",
        !hms::Patient::validateContactNumber("987654321a", err));
  check("Contact Number with hyphens ('987-654-32'): Rejected",
        !hms::Patient::validateContactNumber("987-654-32", err));
  check("Contact Number with spaces ('987 654 32'): Rejected",
        !hms::Patient::validateContactNumber("987 654 32", err));

  // VR-14: Delimiters and formula prefixes
  check("Contact Number with comma ('9876,54321'): Rejected",
        !hms::Patient::validateContactNumber("9876,54321", err));
  check("Contact Number with formula prefix '+' ('+1234567890'): Rejected",
        !hms::Patient::validateContactNumber("+1234567890", err));
  check("Contact Number with formula prefix '=' ('=1234567890'): Rejected",
        !hms::Doctor::validateContactNumber("=1234567890", err));
}

// ---------------------------------------------------------------------------
// 6. Table 4-1 VR-06: Address
// ---------------------------------------------------------------------------
void testVR06_Address() {
  std::cout << "\n--- Testing VR-06: Address (1-100 characters) ---\n";
  std::string err;

  // Boundary: Min length 1
  check("Address min length (1 char 'A'): Accepted",
        hms::Patient::validateAddress("A", err));

  // Boundary: Below min length 0 (Empty)
  check("Address empty string: Rejected",
        !hms::Patient::validateAddress("", err));

  // Boundary: Max length 100
  const std::string addr100(100, 'X');
  check("Address max length (100 characters): Accepted",
        hms::Patient::validateAddress(addr100, err));

  // Boundary: Above max length 101
  const std::string addr101(101, 'X');
  check("Address above max length (101 characters): Rejected",
        !hms::Patient::validateAddress(addr101, err));

  // Permitted punctuation and spaces
  check("Address with permitted punctuation and spaces ('Suite #100 - 42 Main St. (2nd Floor)'): Accepted",
        hms::Patient::validateAddress("Suite #100 - 42 Main St. (2nd Floor)", err));

  // Non-printable control characters rejected
  check("Address with control char SOH ('123 Main St\\x01'): Rejected",
        !hms::Patient::validateAddress("123 Main St\x01", err));
  check("Address with control char ESC ('123 Main St\\x1b'): Rejected",
        !hms::Patient::validateAddress("123 Main St\x1b", err));
  check("Address with control char TAB ('123 Main St\\tApt 4'): Rejected",
        !hms::Patient::validateAddress("123 Main St\tApt 4", err));

  // Valid UTF-8 non-ASCII text: high bytes must NOT be rejected
  // "Rue de l'\xc3\x89glise 12" == "Rue de l'Église 12" (UTF-8 accented E)
  check("Address with UTF-8 accented character (\"Rue de l'\xc3\x89glise 12\"): Accepted",
        hms::Patient::validateAddress("Rue de l'\xc3\x89glise 12", err));
  // "\xe4\xb8\xad\xe5\xb1\xb1\xe8\xb7\xaf 1\xe5\x8f\xb7" == "中山路 1号" (Chinese street, UTF-8)
  check("Address with UTF-8 CJK characters (Chinese street): Accepted",
        hms::Patient::validateAddress("\xe4\xb8\xad\xe5\xb1\xb1\xe8\xb7\xaf 1\xe5\x8f\xb7", err));

  // VR-14: Delimiters and formula prefixes
  check("Address with comma ('123 Main St, Apt 4'): Rejected",
        !hms::Patient::validateAddress("123 Main St, Apt 4", err));
  check("Address with newline ('123 Main St\\nApt 4'): Rejected",
        !hms::Patient::validateAddress("123 Main St\nApt 4", err));
  check("Address with formula prefix '=' ('=123 Main St'): Rejected",
        !hms::Patient::validateAddress("=123 Main St", err));
  check("Address with formula prefix '+' ('+123 Main St'): Rejected",
        !hms::Patient::validateAddress("+123 Main St", err));
  check("Address with formula prefix '-' ('-Suite 500'): Rejected",
        !hms::Patient::validateAddress("-Suite 500", err));
  check("Address with formula prefix '@' ('@Hospital Way'): Rejected",
        !hms::Patient::validateAddress("@Hospital Way", err));
}

// ---------------------------------------------------------------------------
// 7. Table 4-1 VR-07: Medical History Note
// ---------------------------------------------------------------------------
void testVR07_MedicalHistory() {
  std::cout << "\n--- Testing VR-07: Medical History Note (Optional, 0-200 characters) ---\n";
  std::string err;

  // Optional: Empty string (0 chars) is valid
  check("Medical History optional empty string (0 chars): Accepted",
        hms::Patient::validateMedicalHistory("", err));

  // Boundary: Min non-empty 1 char
  check("Medical History min non-empty (1 char 'A'): Accepted",
        hms::Patient::validateMedicalHistory("A", err));

  // Boundary: Max length 200
  const std::string note200(200, 'M');
  check("Medical History max length (200 characters): Accepted",
        hms::Patient::validateMedicalHistory(note200, err));

  // Boundary: Above max length 201
  const std::string note201(201, 'M');
  check("Medical History above max length (201 characters): Rejected",
        !hms::Patient::validateMedicalHistory(note201, err));

  // Permitted punctuation and spaces
  check("Medical History with permitted punctuation and spaces ('Hypertension; mild asthma (controlled) - treated.'): Accepted",
        hms::Patient::validateMedicalHistory("Hypertension; mild asthma (controlled) - treated.", err));

  // Non-printable control characters rejected
  check("Medical History with control char SOH ('Asthma\\x01'): Rejected",
        !hms::Patient::validateMedicalHistory("Asthma\x01", err));
  check("Medical History with control char ESC ('Asthma\\x1b'): Rejected",
        !hms::Patient::validateMedicalHistory("Asthma\x1b", err));
  check("Medical History with control char TAB ('Asthma\\tAllergy'): Rejected",
        !hms::Patient::validateMedicalHistory("Asthma\tAllergy", err));

  // Valid UTF-8 non-ASCII text: high bytes must NOT be rejected
  // "Diab\xc3\xa8te type 2" == "Diabète type 2" (French, UTF-8)
  check("Medical History with UTF-8 accented character (\"Diab\xc3\xa8te type 2\"): Accepted",
        hms::Patient::validateMedicalHistory("Diab\xc3\xa8te type 2", err));

  // VR-14: Delimiters and formula prefixes when non-empty
  check("Medical History with comma ('Asthma, Allergy'): Rejected",
        !hms::Patient::validateMedicalHistory("Asthma, Allergy", err));
  check("Medical History with line feed ('Asthma\\nAllergy'): Rejected",
        !hms::Patient::validateMedicalHistory("Asthma\nAllergy", err));
  check("Medical History with formula prefix '=' ('=Asthma'): Rejected",
        !hms::Patient::validateMedicalHistory("=Asthma", err));
  check("Medical History with formula prefix '+' ('+Diabetes'): Rejected",
        !hms::Patient::validateMedicalHistory("+Diabetes", err));
}

// ---------------------------------------------------------------------------
// 8. Table 4-1 VR-08: Specialization & Department
// ---------------------------------------------------------------------------
void testVR08_SpecializationDepartment() {
  std::cout << "\n--- Testing VR-08: Specialization & Department (1-40 characters each) ---\n";
  std::string err;

  // Boundary: Min length 1
  check("Specialization min length (1 char 'C'): Accepted",
        hms::Doctor::validateSpecialization("C", err));
  check("Department min length (1 char 'E'): Accepted",
        hms::Doctor::validateDepartment("E", err));

  // Boundary: Below min length 0 (Empty)
  check("Specialization empty string: Rejected",
        !hms::Doctor::validateSpecialization("", err));
  check("Department empty string: Rejected",
        !hms::Doctor::validateDepartment("", err));

  // Boundary: Max length 40
  const std::string str40(40, 'S');
  check("Specialization max length (40 characters): Accepted",
        hms::Doctor::validateSpecialization(str40, err));
  check("Department max length (40 characters): Accepted",
        hms::Doctor::validateDepartment(str40, err));

  // Boundary: Above max length 41
  const std::string str41(41, 'S');
  check("Specialization above max length (41 characters): Rejected",
        !hms::Doctor::validateSpecialization(str41, err));
  check("Department above max length (41 characters): Rejected",
        !hms::Doctor::validateDepartment(str41, err));

  // Permitted punctuation and spaces
  check("Specialization with permitted punctuation ('Cardio-Thoracic & Vascular'): Accepted",
        hms::Doctor::validateSpecialization("Cardio-Thoracic & Vascular", err));
  check("Department with permitted punctuation ('Pediatrics (Unit #2)'): Accepted",
        hms::Doctor::validateDepartment("Pediatrics (Unit #2)", err));

  // Non-printable control characters rejected
  check("Specialization with control char SOH ('Cardiology\\x01'): Rejected",
        !hms::Doctor::validateSpecialization("Cardiology\x01", err));
  check("Specialization with control char ESC ('Cardiology\\x1b'): Rejected",
        !hms::Doctor::validateSpecialization("Cardiology\x1b", err));
  check("Department with control char SOH ('Emergency\\x01'): Rejected",
        !hms::Doctor::validateDepartment("Emergency\x01", err));
  check("Department with control char TAB ('Emergency\\tUnit'): Rejected",
        !hms::Doctor::validateDepartment("Emergency\tUnit", err));

  // Valid UTF-8 non-ASCII text: high bytes must NOT be rejected
  // "\xc3\x85rhus Onkologi" == "Århus Onkologi" (Danish, UTF-8)
  check("Specialization with UTF-8 non-ASCII ('Aarhus Onkologi' in Danish UTF-8): Accepted",
        hms::Doctor::validateSpecialization("\xc3\x85rhus Onkologi", err));
  // "\xe7\xa5\x9e\xe7\xbb\x8f\xe5\xa4\x96\xe7\xa7\x91" == "神经外科" (Neurosurgery, Chinese UTF-8)
  check("Department with UTF-8 CJK characters (Chinese department name): Accepted",
        hms::Doctor::validateDepartment("\xe7\xa5\x9e\xe7\xbb\x8f\xe5\xa4\x96\xe7\xa7\x91", err));

  // VR-14: Delimiters and formula prefixes
  check("Specialization with comma ('Cardio, Neuro'): Rejected",
        !hms::Doctor::validateSpecialization("Cardio, Neuro", err));
  check("Department with newline ('Pediatrics\\nUnit'): Rejected",
        !hms::Doctor::validateDepartment("Pediatrics\nUnit", err));
  check("Specialization with formula prefix '=' ('=Cardiology'): Rejected",
        !hms::Doctor::validateSpecialization("=Cardiology", err));
  check("Department with formula prefix '+' ('+Emergency'): Rejected",
        !hms::Doctor::validateDepartment("+Emergency", err));
}

// ---------------------------------------------------------------------------
// 9. Table 4-1 VR-09: Availability Information
// ---------------------------------------------------------------------------
void testVR09_Availability() {
  std::cout << "\n--- Testing VR-09: Availability Information (1-60 characters free text) ---\n";
  std::string err;

  // Boundary: Min length 1
  check("Availability min length (1 char 'M'): Accepted",
        hms::Doctor::validateAvailability("M", err));

  // Boundary: Below min length 0 (Empty)
  check("Availability empty string: Rejected",
        !hms::Doctor::validateAvailability("", err));

  // Boundary: Max length 60
  const std::string avail60(60, 'A');
  check("Availability max length (60 characters): Accepted",
        hms::Doctor::validateAvailability(avail60, err));

  // Boundary: Above max length 61
  const std::string avail61(61, 'A');
  check("Availability above max length (61 characters): Rejected",
        !hms::Doctor::validateAvailability(avail61, err));

  // Typical valid value
  check("Availability typical ('Mon-Fri 09:00-17:00'): Accepted",
        hms::Doctor::validateAvailability("Mon-Fri 09:00-17:00", err));

  // Permitted punctuation and spaces
  check("Availability with permitted punctuation ('Mon-Fri: 09:00 - 17:00 (Room #101)'): Accepted",
        hms::Doctor::validateAvailability("Mon-Fri: 09:00 - 17:00 (Room #101)", err));

  // Non-printable control characters rejected
  check("Availability with control char SOH ('Mon-Fri\\x01'): Rejected",
        !hms::Doctor::validateAvailability("Mon-Fri\x01", err));
  check("Availability with control char ESC ('Mon-Fri\\x1b'): Rejected",
        !hms::Doctor::validateAvailability("Mon-Fri\x1b", err));
  check("Availability with control char TAB ('Mon-Fri\\t09:00-17:00'): Rejected",
        !hms::Doctor::validateAvailability("Mon-Fri\t09:00-17:00", err));

  // Valid UTF-8 non-ASCII text: high bytes must NOT be rejected
  // "Lun-Ven 09h-17h (H\xc3\xb4pital Nord)" == "Lun-Ven 09h-17h (Hôpital Nord)" (French, UTF-8)
  check("Availability with UTF-8 accented character (French hospital name): Accepted",
        hms::Doctor::validateAvailability("Lun-Ven 09h-17h (H\xc3\xb4pital Nord)", err));

  // VR-14: Delimiters and formula prefixes
  check("Availability with comma ('Mon, Tue 09:00-17:00'): Rejected",
        !hms::Doctor::validateAvailability("Mon, Tue 09:00-17:00", err));
  check("Availability with newline ('Mon-Fri\\n09:00-17:00'): Rejected",
        !hms::Doctor::validateAvailability("Mon-Fri\n09:00-17:00", err));
  check("Availability with formula prefix '=' ('=Mon-Fri'): Rejected",
        !hms::Doctor::validateAvailability("=Mon-Fri", err));
  check("Availability with formula prefix '+' ('+09:00-17:00'): Rejected",
        !hms::Doctor::validateAvailability("+09:00-17:00", err));
}

// ---------------------------------------------------------------------------
// 10. Table 4-1 VR-10: Appointment Date
// ---------------------------------------------------------------------------
void testVR10_AppointmentDate() {
  std::cout << "\n--- Testing VR-10: Appointment Date (YYYY-MM-DD, 2000-2100) ---\n";
  std::string err;

  // Year Boundaries (2000 to 2100)
  check("Date min year ('2000-01-01'): Accepted",
        hms::Appointment::validateDate("2000-01-01", err));
  check("Date below min year ('1999-12-31'): Rejected",
        !hms::Appointment::validateDate("1999-12-31", err));
  check("Date max year ('2100-12-31'): Accepted",
        hms::Appointment::validateDate("2100-12-31", err));
  check("Date above max year ('2101-01-01'): Rejected",
        !hms::Appointment::validateDate("2101-01-01", err));

  // Leap Year Boundaries
  check("Date leap year 2000-02-29 (divisible by 400): Accepted",
        hms::Appointment::validateDate("2000-02-29", err));
  check("Date leap year 2024-02-29 (standard leap year): Accepted",
        hms::Appointment::validateDate("2024-02-29", err));
  check("Date non-leap year 2026-02-29: Rejected",
        !hms::Appointment::validateDate("2026-02-29", err));
  check("Date century non-leap year 2100-02-29 (not div by 400): Rejected",
        !hms::Appointment::validateDate("2100-02-29", err));

  // Month Boundaries (01 to 12)
  check("Date month min ('2026-01-15'): Accepted",
        hms::Appointment::validateDate("2026-01-15", err));
  check("Date month max ('2026-12-15'): Accepted",
        hms::Appointment::validateDate("2026-12-15", err));
  check("Date month below min ('2026-00-15'): Rejected",
        !hms::Appointment::validateDate("2026-00-15", err));
  check("Date month above max ('2026-13-15'): Rejected",
        !hms::Appointment::validateDate("2026-13-15", err));

  // Day Boundaries per Month
  check("Date day min ('2026-05-01'): Accepted",
        hms::Appointment::validateDate("2026-05-01", err));
  check("Date day below min ('2026-05-00'): Rejected",
        !hms::Appointment::validateDate("2026-05-00", err));
  check("Date day max 31 in May ('2026-05-31'): Accepted",
        hms::Appointment::validateDate("2026-05-31", err));
  check("Date day above max 32 in May ('2026-05-32'): Rejected",
        !hms::Appointment::validateDate("2026-05-32", err));
  check("Date day max 30 in April ('2026-04-30'): Accepted",
        hms::Appointment::validateDate("2026-04-30", err));
  check("Date day 31 in 30-day month April ('2026-04-31'): Rejected",
        !hms::Appointment::validateDate("2026-04-31", err));

  // Format Boundaries
  check("Date invalid separator slashes ('2026/10/05'): Rejected",
        !hms::Appointment::validateDate("2026/10/05", err));
  check("Date invalid separator dots ('2026.10.05'): Rejected",
        !hms::Appointment::validateDate("2026.10.05", err));
  check("Date incomplete year ('26-10-05'): Rejected",
        !hms::Appointment::validateDate("26-10-05", err));
  check("Date single digit month ('2026-5-01'): Rejected",
        !hms::Appointment::validateDate("2026-5-01", err));
  check("Date single digit day ('2026-05-1'): Rejected",
        !hms::Appointment::validateDate("2026-05-1", err));
  check("Date alphabetic ('YYYY-MM-DD'): Rejected",
        !hms::Appointment::validateDate("YYYY-MM-DD", err));
  check("Date empty string: Rejected",
        !hms::Appointment::validateDate("", err));

  // VR-14: Delimiters and formula prefixes
  check("Date with comma ('2026,10-05'): Rejected",
        !hms::Appointment::validateDate("2026,10-05", err));
  check("Date with newline ('2026-10-05\\n'): Rejected",
        !hms::Appointment::validateDate("2026-10-05\n", err));
  check("Date with formula prefix '=' ('=2026-10-05'): Rejected",
        !hms::Appointment::validateDate("=2026-10-05", err));
  check("Date with formula prefix '+' ('+2026-10-05'): Rejected",
        !hms::Appointment::validateDate("+2026-10-05", err));
}

// ---------------------------------------------------------------------------
// 11. Table 4-1 VR-11: Appointment Time
// ---------------------------------------------------------------------------
void testVR11_AppointmentTime() {
  std::cout << "\n--- Testing VR-11: Appointment Time (HH:MM 24-hour clock) ---\n";
  std::string err;

  // Boundary: Min and Max times
  check("Time min boundary ('00:00'): Accepted",
        hms::Appointment::validateTime("00:00", err));
  check("Time max boundary ('23:59'): Accepted",
        hms::Appointment::validateTime("23:59", err));

  // Hour Boundaries (00 to 23)
  check("Time valid hour 23 ('23:00'): Accepted",
        hms::Appointment::validateTime("23:00", err));
  check("Time hour above max ('24:00'): Rejected",
        !hms::Appointment::validateTime("24:00", err));
  check("Time hour far above max ('25:00'): Rejected",
        !hms::Appointment::validateTime("25:00", err));

  // Minute Boundaries (00 to 59)
  check("Time valid minute 59 ('12:59'): Accepted",
        hms::Appointment::validateTime("12:59", err));
  check("Time minute above max ('12:60'): Rejected",
        !hms::Appointment::validateTime("12:60", err));
  check("Time minute far above max ('12:99'): Rejected",
        !hms::Appointment::validateTime("12:99", err));

  // Format Boundaries
  check("Time single digit hour and min ('9:5'): Rejected",
        !hms::Appointment::validateTime("9:5", err));
  check("Time single digit min ('09:5'): Rejected",
        !hms::Appointment::validateTime("09:5", err));
  check("Time single digit hour ('9:05'): Rejected",
        !hms::Appointment::validateTime("9:05", err));
  check("Time hyphen separator ('09-30'): Rejected",
        !hms::Appointment::validateTime("09-30", err));
  check("Time with seconds ('09:30:00'): Rejected",
        !hms::Appointment::validateTime("09:30:00", err));
  check("Time alphabetic ('HH:MM'): Rejected",
        !hms::Appointment::validateTime("HH:MM", err));
  check("Time empty string: Rejected",
        !hms::Appointment::validateTime("", err));

  // VR-14: Delimiters and formula prefixes
  check("Time with comma ('09,30'): Rejected",
        !hms::Appointment::validateTime("09,30", err));
  check("Time with formula prefix '=' ('=09:30'): Rejected",
        !hms::Appointment::validateTime("=09:30", err));
  check("Time with formula prefix '+' ('+09:30'): Rejected",
        !hms::Appointment::validateTime("+09:30", err));
}

// ---------------------------------------------------------------------------
// 12. Table 4-1 VR-12: Appointment ID
// ---------------------------------------------------------------------------
void testVR12_AppointmentId() {
  std::cout << "\n--- Testing VR-12: Appointment ID (A + at least 4 digits) ---\n";
  std::string err;

  // Boundary: Min length 5 ('A' + 4 digits)
  check("Appointment ID min length ('A0001'): Accepted",
        hms::Appointment::validateId("A0001", err));
  check("Appointment ID 'A9999': Accepted",
        hms::Appointment::validateId("A9999", err));
  check("Appointment ID 5 digits ('A10000'): Accepted",
        hms::Appointment::validateId("A10000", err));

  // Boundary: Below min length 4 ('A' + 3 digits)
  check("Appointment ID below min length ('A001'): Rejected",
        !hms::Appointment::validateId("A001", err));
  check("Appointment ID prefix only ('A'): Rejected",
        !hms::Appointment::validateId("A", err));
  check("Appointment ID empty string: Rejected",
        !hms::Appointment::validateId("", err));

  // Prefix checks
  check("Appointment ID wrong prefix ('B0001'): Rejected",
        !hms::Appointment::validateId("B0001", err));
  check("Appointment ID no prefix ('10001'): Rejected",
        !hms::Appointment::validateId("10001", err));
  check("Appointment ID with non-digit suffix ('A0001X'): Rejected",
        !hms::Appointment::validateId("A0001X", err));

  // VR-14: Delimiters and formula prefixes
  check("Appointment ID with comma ('A00,1'): Rejected",
        !hms::Appointment::validateId("A00,1", err));
  check("Appointment ID with formula prefix '=' ('=A0001'): Rejected",
        !hms::Appointment::validateId("=A0001", err));
  check("Appointment ID with formula prefix '+' ('+A0001'): Rejected",
        !hms::Appointment::validateId("+A0001", err));
}

// ---------------------------------------------------------------------------
// 13. Table 4-1 VR-13: Appointment Status
// ---------------------------------------------------------------------------
void testVR13_AppointmentStatus() {
  std::cout << "\n--- Testing VR-13: Appointment Status (Exactly 'Booked' or 'Cancelled') ---\n";
  hms::AppointmentStatus status;
  std::string err;

  // Permitted values
  check("Status 'Booked': Accepted",
        hms::Appointment::validateStatus("Booked", status, err) &&
        status == hms::AppointmentStatus::Booked);
  check("Status 'Cancelled': Accepted",
        hms::Appointment::validateStatus("Cancelled", status, err) &&
        status == hms::AppointmentStatus::Cancelled);

  // Disallowed values
  check("Status lowercase 'booked': Rejected",
        !hms::Appointment::validateStatus("booked", status, err));
  check("Status uppercase 'CANCELLED': Rejected",
        !hms::Appointment::validateStatus("CANCELLED", status, err));
  check("Status 'Pending': Rejected",
        !hms::Appointment::validateStatus("Pending", status, err));
  check("Status 'Completed': Rejected",
        !hms::Appointment::validateStatus("Completed", status, err));
  check("Status empty string: Rejected",
        !hms::Appointment::validateStatus("", status, err));
  check("Status formula prefix '=Booked': Rejected",
        !hms::Appointment::validateStatus("=Booked", status, err));
}

// ---------------------------------------------------------------------------
// 14. Entity-level Integration Validation (Patient, Doctor, Appointment)
// ---------------------------------------------------------------------------
void testEntityValidation() {
  std::cout << "\n--- Testing Entity-level validate() & isValid() Gate ---\n";
  std::string err;

  // Fully valid Patient
  hms::Patient validPatient("P001", "John Doe", 35, "M", "9876543210", "123 Elm St", "None");
  check("Valid Patient passes validate():", validPatient.validate(err));
  check("Valid Patient passes isValid():", validPatient.isValid());

  // Fully valid Doctor
  hms::Doctor validDoctor("D001", "Dr. Jane Smith", "Cardiology", "Cardiology", "9876543210", "Mon-Fri 09:00-17:00");
  check("Valid Doctor passes validate():", validDoctor.validate(err));
  check("Valid Doctor passes isValid():", validDoctor.isValid());

  // Fully valid Appointment
  hms::Appointment validAppt("A0001", "P001", "D001", "2026-10-05", "14:30", hms::AppointmentStatus::Booked);
  check("Valid Appointment passes validate():", validAppt.validate(err));
  check("Valid Appointment passes isValid():", validAppt.isValid());

  // Invalid Patient (age 121)
  hms::Patient invalidPatientAge("P002", "John Doe", 121, "M", "9876543210", "123 Elm St");
  check("Patient with age 121 rejected by validate():", !invalidPatientAge.validate(err));
  check("Patient with age 121 rejected by isValid():", !invalidPatientAge.isValid());

  // Invalid Doctor (specialization > 40 chars)
  hms::Doctor invalidDocSpec("D002", "Dr. Smith", makeString(41, 'S'), "Cardiology", "9876543210", "Mon-Fri");
  check("Doctor with 41-char specialization rejected by validate():", !invalidDocSpec.validate(err));
  check("Doctor with 41-char specialization rejected by isValid():", !invalidDocSpec.isValid());

  // Invalid Appointment (year 1999)
  hms::Appointment invalidApptDate("A0002", "P001", "D001", "1999-12-31", "14:30");
  check("Appointment with year 1999 rejected by validate():", !invalidApptDate.validate(err));
  check("Appointment with year 1999 rejected by isValid():", !invalidApptDate.isValid());
}

// ---------------------------------------------------------------------------
// 15. Safe Keyboard Input Simulation & Interactive Recovery (HMS-SR-001)
// ---------------------------------------------------------------------------
void testKeyboardInputRecoveryAndSafety() {
  std::cout << "\n--- Testing Keyboard Input Recovery & Non-numeric Stream Safety ---\n";

  // Create isolated temp directory for test database
  const auto tempDir = std::filesystem::temp_directory_path() / "hms_sec01_test_db";
  std::filesystem::remove_all(tempDir);
  std::filesystem::create_directories(tempDir);

  const auto pCsv = tempDir / "patients.csv";
  const auto dCsv = tempDir / "doctors.csv";
  const auto aCsv = tempDir / "appointments.csv";

  {
    std::ofstream pf(pCsv);
    pf << "patient_id,name,age,gender,contact_number,address,medical_history\n";
    std::ofstream df(dCsv);
    df << "doctor_id,name,specialization,department,contact_number,availability\n";
    std::ofstream af(aCsv);
    af << "appointment_id,patient_id,doctor_id,date,time,status\n";
  }

  // Backup stream buffers
  std::streambuf *origCin = std::cin.rdbuf();
  std::streambuf *origCout = std::cout.rdbuf();

  std::ostringstream capturedOut;
  std::cout.rdbuf(capturedOut.rdbuf());

  // Scenario 1: Non-numeric Age Input followed by clean recovery
  // Inputs:
  // 1 (Patient menu)
  // 1 (Register patient)
  // P100 (Patient ID)
  // Alice Smith (Name)
  // abc (Non-numeric age - should fail safely with error message)
  // 0 (Back to main menu)
  // 4 (Exit system cleanly)
  {
    std::istringstream testInput(
        "1\n"            // Main menu -> Patient menu
        "1\n"            // Patient menu -> Register patient
        "P100\n"         // ID
        "Alice Smith\n"  // Name
        "abc\n"          // Age: Non-numeric -> rejected!
        "0\n"            // Patient menu -> Back to main
        "4\n"            // Main menu -> Exit
    );
    std::cin.rdbuf(testInput.rdbuf());

    hms::HospitalSystem sys(tempDir.string());
    sys.run();

    const std::string out = capturedOut.str();
    check("Non-numeric age 'abc' safely rejected with prompt error:",
          out.find("Age: must be a whole number from 0 to 120.") != std::string::npos);

    // Verify database was NOT corrupted or written to
    hms::FileManager fm(pCsv.string(), dCsv.string(), aCsv.string());
    check("No patient record created after non-numeric input failure:",
          !fm.patientIdExists("P100"));
  }

  // Scenario 2: Formula Injection Age Input ("+25") rejected safely
  // Inputs:
  // 1 (Patient menu)
  // 1 (Register patient)
  // P101 (Patient ID)
  // Bob Jones (Name)
  // +25 (Formula prefix in age -> rejected!)
  // 0 (Back to main)
  // 4 (Exit)
  capturedOut.str("");
  capturedOut.clear();
  {
    std::istringstream testInput(
        "1\n"
        "1\n"
        "P101\n"
        "Bob Jones\n"
        "+25\n"
        "0\n"
        "4\n"
    );
    std::cin.rdbuf(testInput.rdbuf());

    hms::HospitalSystem sys(tempDir.string());
    sys.run();

    const std::string out = capturedOut.str();
    check("Formula prefix '+25' in age safely rejected:",
          out.find("Age: must be a whole number from 0 to 120.") != std::string::npos);

    hms::FileManager fm(pCsv.string(), dCsv.string(), aCsv.string());
    check("No patient record created after formula prefix input failure:",
          !fm.patientIdExists("P101"));
  }

  // Scenario 3: Out-of-range Age Input ("121") rejected safely
  capturedOut.str("");
  capturedOut.clear();
  {
    std::istringstream testInput(
        "1\n"
        "1\n"
        "P102\n"
        "Carol White\n"
        "121\n"
        "0\n"
        "4\n"
    );
    std::cin.rdbuf(testInput.rdbuf());

    hms::HospitalSystem sys(tempDir.string());
    sys.run();

    const std::string out = capturedOut.str();
    check("Out-of-range age '121' safely rejected:",
          out.find("Age: must be a whole number from 0 to 120.") != std::string::npos);

    hms::FileManager fm(pCsv.string(), dCsv.string(), aCsv.string());
    check("No patient record created after out-of-range age failure:",
          !fm.patientIdExists("P102"));
  }

  // Scenario 4: Valid Registration following Recovery
  // User enters valid data meeting all Table 4-1 rules
  capturedOut.str("");
  capturedOut.clear();
  {
    std::istringstream testInput(
        "1\n"            // Patient menu
        "1\n"            // Register patient
        "P103\n"         // ID (VR-01)
        "David Miller\n" // Name (VR-02)
        "30\n"           // Age (VR-03)
        "M\n"            // Gender (VR-04)
        "9876543210\n"   // Contact (VR-05)
        "100 Market St\n"// Address (VR-06)
        "None\n"         // Med history (VR-07)
        "0\n"            // Return
        "4\n"            // Exit
    );
    std::cin.rdbuf(testInput.rdbuf());

    hms::HospitalSystem sys(tempDir.string());
    sys.run();

    const std::string out = capturedOut.str();
    check("Valid registration succeeds after recovery:",
          out.find("Patient registered successfully.") != std::string::npos);

    hms::FileManager fm(pCsv.string(), dCsv.string(), aCsv.string());
    check("Patient record saved to persistent CSV:",
          fm.patientIdExists("P103"));
  }

  // Restore streams
  std::cin.rdbuf(origCin);
  std::cout.rdbuf(origCout);

  // Clean up test directory
  std::error_code ec;
  std::filesystem::remove_all(tempDir, ec);
}

} // namespace

int main() {
  std::cout << "=================================================================\n";
  std::cout << "TC-SEC-01: Accept-List Input Validation (HMS-SR-001 / Table 4-1)\n";
  std::cout << "=================================================================\n";

  testVR01_EntityIds();
  testVR02_Name();
  testVR03_Age();
  testVR04_Gender();
  testVR05_ContactNumber();
  testVR06_Address();
  testVR07_MedicalHistory();
  testVR08_SpecializationDepartment();
  testVR09_Availability();
  testVR10_AppointmentDate();
  testVR11_AppointmentTime();
  testVR12_AppointmentId();
  testVR13_AppointmentStatus();
  testEntityValidation();
  testKeyboardInputRecoveryAndSafety();

  std::cout << "\n=================================================================\n";
  std::cout << "TC-SEC-01 Summary: " << g_passed << " passed, " << g_failed << " failed.\n";
  std::cout << "=================================================================\n";

  return (g_failed == 0) ? 0 : 1;
}
