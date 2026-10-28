
#include "hms/Patient.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace hms {
namespace {

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

std::string toUpper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

} // namespace

// ---------------------------------------------------------------------------
// VR-14: reject commas, CR, LF, and formula-injection prefixes.
// ---------------------------------------------------------------------------
bool Patient::passesSecurityCheck(const std::string &value,
                                  std::string &errorMessage,
                                  const std::string &fieldName) {
  if (value.find(',') != std::string::npos ||
      value.find('\r') != std::string::npos ||
      value.find('\n') != std::string::npos) {
    errorMessage = fieldName + ": commas and line breaks are not allowed.";
    return false;
  }
  for (unsigned char c : value) {
    // Reject C0 control characters (0x00-0x1F) and DEL (0x7F).
    // High bytes (0x80-0xFF) are valid UTF-8 continuation/lead bytes and
    // are not rejected here; VR-14 only mandates exclusion of control chars.
    if (c < 0x20u || c == 0x7Fu) {
      errorMessage = fieldName + ": non-printable control characters are not allowed.";
      return false;
    }
  }
  if (!value.empty()) {
    const char first = value.front();
    if (first == '=' || first == '+' || first == '-' || first == '@') {
      errorMessage = fieldName +
                     ": value must not begin with =, +, -, or @.";
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Static field validators (Table 4-1 rules)
// ---------------------------------------------------------------------------

// VR-01: Patient ID — 1-10 alphanumeric characters.
bool Patient::validateId(const std::string &id, std::string &errorMessage) {
  if (id.empty() || id.size() > 10) {
    errorMessage = "Patient ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : id) {
    if (!std::isalnum(c)) {
      errorMessage = "Patient ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(id, errorMessage, "Patient ID")) {
    return false;
  }
  return true;
}

// VR-02: Name — 1-50 chars; letters, spaces, '.', '-', '\''.
bool Patient::validateName(const std::string &name, std::string &errorMessage) {
  if (name.empty() || name.size() > 50) {
    errorMessage = "Name: must contain 1-50 permitted characters.";
    return false;
  }
  for (unsigned char c : name) {
    if (!std::isalpha(c) && c != ' ' && c != '.' && c != '-' && c != '\'') {
      errorMessage =
          "Name: must contain 1-50 permitted characters (letters, spaces, "
          "'.', '-', \"'\").";
      return false;
    }
  }
  if (!passesSecurityCheck(name, errorMessage, "Name")) {
    return false;
  }
  return true;
}

// VR-03: Age — Whole number 0-120 (inclusive).
bool Patient::validateAge(int age, std::string &errorMessage) {
  if (age < 0 || age > 120) {
    errorMessage = "Age: must be a whole number from 0 to 120.";
    return false;
  }
  return true;
}

bool Patient::validateAge(const std::string &input, int &outAge, std::string &errorMessage) {
  if (input.empty()) {
    errorMessage = "Age: must be a whole number from 0 to 120.";
    return false;
  }
  for (unsigned char c : input) {
    if (!std::isdigit(c)) {
      errorMessage = "Age: must be a whole number from 0 to 120.";
      return false;
    }
  }
  try {
    outAge = std::stoi(input);
  } catch (...) {
    errorMessage = "Age: must be a whole number from 0 to 120.";
    return false;
  }
  return validateAge(outAge, errorMessage);
}

// VR-04: Gender — M, F, or O (case-insensitive).
bool Patient::validateGender(const std::string &gender, std::string &errorMessage) {
  const std::string g = toUpper(gender);
  if (g != "M" && g != "F" && g != "O") {
    errorMessage = "Gender: must be M, F, or O.";
    return false;
  }
  return true;
}

// VR-05: Contact number — Exactly 10 digits.
bool Patient::validateContactNumber(const std::string &contactNumber, std::string &errorMessage) {
  if (contactNumber.size() != 10) {
    errorMessage = "Contact number: must contain exactly 10 digits.";
    return false;
  }
  for (unsigned char c : contactNumber) {
    if (!std::isdigit(c)) {
      errorMessage = "Contact number: must contain exactly 10 digits.";
      return false;
    }
  }
  if (!passesSecurityCheck(contactNumber, errorMessage, "Contact number")) {
    return false;
  }
  return true;
}

// VR-06: Address — 1-100 characters.
bool Patient::validateAddress(const std::string &address, std::string &errorMessage) {
  if (address.empty() || address.size() > 100) {
    errorMessage = "Address: must contain 1-100 characters.";
    return false;
  }
  if (!passesSecurityCheck(address, errorMessage, "Address")) {
    return false;
  }
  return true;
}

// VR-07: Medical history note — Optional (empty is valid); max 200 characters.
bool Patient::validateMedicalHistory(const std::string &medicalHistory, std::string &errorMessage) {
  if (medicalHistory.size() > 200) {
    errorMessage = "Medical history: must not exceed 200 characters.";
    return false;
  }
  if (!passesSecurityCheck(medicalHistory, errorMessage, "Medical history")) {
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Constructor — normalizes ID to uppercase and gender to uppercase.
// ---------------------------------------------------------------------------
Patient::Patient(std::string id, std::string name, int age, std::string gender,
                 std::string contactNumber, std::string address,
                 std::string medicalHistory)
    : id_(toUpper(std::move(id))),
      name_(std::move(name)),
      age_(age),
      gender_(toUpper(std::move(gender))),
      contactNumber_(std::move(contactNumber)),
      address_(std::move(address)),
      medicalHistory_(std::move(medicalHistory)) {}

// ---------------------------------------------------------------------------
// validate() — checks VR-01 through VR-07 and VR-14 in field order.
// Returns false and sets errorMessage on the first failure.
// ---------------------------------------------------------------------------
bool Patient::validate(std::string &errorMessage) const {
  if (!validateId(id_, errorMessage)) {
    return false;
  }
  if (!validateName(name_, errorMessage)) {
    return false;
  }
  if (!validateAge(age_, errorMessage)) {
    return false;
  }
  if (!validateGender(gender_, errorMessage)) {
    return false;
  }
  if (!validateContactNumber(contactNumber_, errorMessage)) {
    return false;
  }
  if (!validateAddress(address_, errorMessage)) {
    return false;
  }
  if (!validateMedicalHistory(medicalHistory_, errorMessage)) {
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// isValid() — thin boolean gate used by FileManager before persisting.
// ---------------------------------------------------------------------------
bool Patient::isValid() const {
  std::string unused;
  return validate(unused);
}

const std::string &Patient::getId() const { return id_; }
const std::string &Patient::getName() const { return name_; }
int Patient::getAge() const { return age_; }
const std::string &Patient::getGender() const { return gender_; }
const std::string &Patient::getContactNumber() const { return contactNumber_; }
const std::string &Patient::getAddress() const { return address_; }
const std::string &Patient::getMedicalHistory() const {
  return medicalHistory_;
}

} // namespace hms
