
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

// VR-14: reject commas, CR, LF, and formula-injection prefixes.
bool passesSecurityCheck(const std::string &value, std::string &errorMessage,
                         const std::string &fieldName) {
  if (value.find(',') != std::string::npos ||
      value.find('\r') != std::string::npos ||
      value.find('\n') != std::string::npos) {
    errorMessage = fieldName + ": commas and line breaks are not allowed.";
    return false;
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

} // namespace

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
  // --- VR-01: Patient ID ---------------------------------------------------
  // 1-10 alphanumeric characters (already uppercased in constructor).
  if (id_.empty() || id_.size() > 10) {
    errorMessage =
        "Patient ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : id_) {
    if (!std::isalnum(c)) {
      errorMessage =
          "Patient ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(id_, errorMessage, "Patient ID")) {
    return false;
  }

  // --- VR-02: Name ---------------------------------------------------------
  // 1-50 chars; letters, spaces, '.', '-', '\''.
  if (name_.empty() || name_.size() > 50) {
    errorMessage =
        "Name: must contain 1-50 permitted characters.";
    return false;
  }
  for (unsigned char c : name_) {
    if (!std::isalpha(c) && c != ' ' && c != '.' && c != '-' && c != '\'') {
      errorMessage =
          "Name: must contain 1-50 permitted characters (letters, spaces, "
          "'.', '-', \"'\").";
      return false;
    }
  }
  if (!passesSecurityCheck(name_, errorMessage, "Name")) {
    return false;
  }

  // --- VR-03: Age ----------------------------------------------------------
  // Whole number 0-120 (inclusive).
  if (age_ < 0 || age_ > 120) {
    errorMessage = "Age: must be a whole number from 0 to 120.";
    return false;
  }

  // --- VR-04: Gender -------------------------------------------------------
  // M, F, or O (already uppercased in constructor).
  if (gender_ != "M" && gender_ != "F" && gender_ != "O") {
    errorMessage = "Gender: must be M, F, or O.";
    return false;
  }

  // --- VR-05: Contact number -----------------------------------------------
  // Exactly 10 digits.
  if (contactNumber_.size() != 10) {
    errorMessage = "Contact number: must contain exactly 10 digits.";
    return false;
  }
  for (unsigned char c : contactNumber_) {
    if (!std::isdigit(c)) {
      errorMessage = "Contact number: must contain exactly 10 digits.";
      return false;
    }
  }
  if (!passesSecurityCheck(contactNumber_, errorMessage, "Contact number")) {
    return false;
  }

  // --- VR-06: Address ------------------------------------------------------
  // 1-100 characters.
  if (address_.empty() || address_.size() > 100) {
    errorMessage = "Address: must contain 1-100 characters.";
    return false;
  }
  if (!passesSecurityCheck(address_, errorMessage, "Address")) {
    return false;
  }

  // --- VR-07: Medical history ----------------------------------------------
  // Optional (empty is valid); max 200 characters.
  if (medicalHistory_.size() > 200) {
    errorMessage = "Medical history: must not exceed 200 characters.";
    return false;
  }
  if (!passesSecurityCheck(medicalHistory_, errorMessage, "Medical history")) {
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
