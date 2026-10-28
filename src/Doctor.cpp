#include "hms/Doctor.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace hms {
namespace {

std::string toUpper(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Security check helper (VR-14)
// ---------------------------------------------------------------------------

bool Doctor::passesSecurityCheck(const std::string &value,
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
      errorMessage = fieldName + ": value must not begin with =, +, -, or @.";
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Static field validators (Table 4-1 rules)
// ---------------------------------------------------------------------------

// VR-01: Doctor ID — 1-10 alphanumeric characters.
bool Doctor::validateId(const std::string &id, std::string &errorMessage) {
  if (id.empty() || id.size() > 10) {
    errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : id) {
    if (!std::isalnum(c)) {
      errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(id, errorMessage, "Doctor ID")) {
    return false;
  }
  return true;
}

// VR-02: Name — 1-50 chars; letters, spaces, '.', '-', '\''.
bool Doctor::validateName(const std::string &name, std::string &errorMessage) {
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

// VR-08: Specialization — 1-40 characters.
bool Doctor::validateSpecialization(const std::string &specialization,
                                    std::string &errorMessage) {
  if (specialization.empty() || specialization.size() > 40) {
    errorMessage = "Specialization: must contain 1-40 characters.";
    return false;
  }
  if (!passesSecurityCheck(specialization, errorMessage, "Specialization")) {
    return false;
  }
  return true;
}

// VR-08: Department — 1-40 characters.
bool Doctor::validateDepartment(const std::string &department,
                                std::string &errorMessage) {
  if (department.empty() || department.size() > 40) {
    errorMessage = "Department: must contain 1-40 characters.";
    return false;
  }
  if (!passesSecurityCheck(department, errorMessage, "Department")) {
    return false;
  }
  return true;
}

// VR-05: Contact number — Exactly 10 digits.
bool Doctor::validateContactNumber(const std::string &contactNumber,
                                   std::string &errorMessage) {
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

// VR-09: Availability information — 1-60 characters free text.
bool Doctor::validateAvailability(const std::string &availability,
                                  std::string &errorMessage) {
  if (availability.empty() || availability.size() > 60) {
    errorMessage = "Availability: must contain 1-60 characters.";
    return false;
  }
  if (!passesSecurityCheck(availability, errorMessage, "Availability")) {
    return false;
  }
  return true;
}

Doctor::Doctor(std::string id, std::string name, std::string specialization,
               std::string department, std::string contactNumber,
               std::string availability)
    : id_(toUpper(std::move(id))), name_(std::move(name)),
      specialization_(std::move(specialization)),
      department_(std::move(department)),
      contactNumber_(std::move(contactNumber)),
      availability_(std::move(availability)) {}

bool Doctor::validate(std::string &errorMessage) const {
  if (!validateId(id_, errorMessage)) {
    return false;
  }
  if (!validateName(name_, errorMessage)) {
    return false;
  }
  if (!validateSpecialization(specialization_, errorMessage)) {
    return false;
  }
  if (!validateDepartment(department_, errorMessage)) {
    return false;
  }
  if (!validateContactNumber(contactNumber_, errorMessage)) {
    return false;
  }
  if (!validateAvailability(availability_, errorMessage)) {
    return false;
  }
  return true;
}

bool Doctor::isValid() const {
  std::string unused;
  return validate(unused);
}

const std::string &Doctor::getId() const { return id_; }
const std::string &Doctor::getName() const { return name_; }
const std::string &Doctor::getSpecialization() const { return specialization_; }
const std::string &Doctor::getDepartment() const { return department_; }
const std::string &Doctor::getContactNumber() const { return contactNumber_; }
const std::string &Doctor::getAvailability() const { return availability_; }

} // namespace hms
