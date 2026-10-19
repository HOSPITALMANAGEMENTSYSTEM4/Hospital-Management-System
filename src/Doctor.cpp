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
      errorMessage = fieldName + ": value must not begin with =, +, -, or @.";
      return false;
    }
  }
  return true;
}

} // namespace

Doctor::Doctor(std::string id, std::string name, std::string specialization,
               std::string department, std::string contactNumber,
               std::string availability)
    : id_(toUpper(std::move(id))), name_(std::move(name)),
      specialization_(std::move(specialization)),
      department_(std::move(department)),
      contactNumber_(std::move(contactNumber)),
      availability_(std::move(availability)) {}

bool Doctor::validate(std::string &errorMessage) const {
  // --- VR-01: Doctor ID ---
  // 1-10 alphanumeric characters (already uppercased in constructor).
  if (id_.empty() || id_.size() > 10) {
    errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : id_) {
    if (!std::isalnum(c)) {
      errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(id_, errorMessage, "Doctor ID")) {
    return false;
  }

  // --- VR-02: Name ---
  // 1-50 chars; letters, spaces, '.', '-', '\''.
  if (name_.empty() || name_.size() > 50) {
    errorMessage = "Name: must contain 1-50 permitted characters.";
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

  // --- VR-08: Specialization ---
  // 1-40 characters.
  if (specialization_.empty() || specialization_.size() > 40) {
    errorMessage = "Specialization: must contain 1-40 characters.";
    return false;
  }
  if (!passesSecurityCheck(specialization_, errorMessage, "Specialization")) {
    return false;
  }

  // --- VR-08: Department ---
  // 1-40 characters.
  if (department_.empty() || department_.size() > 40) {
    errorMessage = "Department: must contain 1-40 characters.";
    return false;
  }
  if (!passesSecurityCheck(department_, errorMessage, "Department")) {
    return false;
  }

  // --- VR-05: Contact number ---
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

  // --- VR-09: Availability information ---
  // 1-60 characters of free text (e.g., “Mon-Fri 09:00-17:00”). Informational
  // only.
  if (availability_.empty() || availability_.size() > 60) {
    errorMessage = "Availability: must contain 1-60 characters.";
    return false;
  }
  if (!passesSecurityCheck(availability_, errorMessage, "Availability")) {
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
