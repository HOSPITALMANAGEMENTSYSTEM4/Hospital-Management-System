#include "hms/Appointment.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>
#include <vector>

namespace hms {
namespace {

std::string trim(const std::string &value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t first = value.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = value.find_last_not_of(whitespace);
  return value.substr(first, last - first + 1);
}

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

std::vector<std::string> splitCsvLine(const std::string &line) {
  std::vector<std::string> tokens;
  std::string current;
  for (char c : line) {
    if (c == '\r' || c == '\n') {
      continue;
    }
    if (c == ',') {
      tokens.push_back(trim(current));
      current.clear();
    } else {
      current += c;
    }
  }
  tokens.push_back(trim(current));
  return tokens;
}

} // namespace

std::string statusToString(AppointmentStatus status) {
  switch (status) {
  case AppointmentStatus::Booked:
    return "Booked";
  case AppointmentStatus::Cancelled:
    return "Cancelled";
  default:
    return "Booked";
  }
}

bool stringToStatus(const std::string &str, AppointmentStatus &outStatus) {
  const std::string s = trim(str);
  if (s == "Booked") {
    outStatus = AppointmentStatus::Booked;
    return true;
  }
  if (s == "Cancelled") {
    outStatus = AppointmentStatus::Cancelled;
    return true;
  }
  return false;
}

Appointment::Appointment()
    : status_(AppointmentStatus::Booked) {}

Appointment::Appointment(std::string id,
                         std::string patientId,
                         std::string doctorId,
                         std::string date,
                         std::string time,
                         AppointmentStatus status)
    : id_(toUpper(trim(id))),
      patientId_(toUpper(trim(patientId))),
      doctorId_(toUpper(trim(doctorId))),
      date_(trim(date)),
      time_(trim(time)),
      status_(status) {}

bool Appointment::validateId(const std::string &id, std::string &errorMessage) {
  if (!passesSecurityCheck(id, errorMessage, "Appointment ID")) {
    return false;
  }
  // VR-12: System-generated A + at least four digits (A0001, A0002, ...)
  if (id.size() < 5 || (id[0] != 'A' && id[0] != 'a')) {
    errorMessage = "Appointment ID: must start with 'A' followed by at least 4 digits.";
    return false;
  }
  for (std::size_t i = 1; i < id.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(id[i]))) {
      errorMessage = "Appointment ID: must contain digits following prefix 'A'.";
      return false;
    }
  }
  return true;
}

bool Appointment::validateDate(const std::string &date, std::string &errorMessage) {
  if (!passesSecurityCheck(date, errorMessage, "Appointment date")) {
    return false;
  }

  // VR-10: YYYY-MM-DD
  if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
    errorMessage = "Appointment date: must be in YYYY-MM-DD format.";
    return false;
  }

  for (std::size_t i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) continue;
    if (!std::isdigit(static_cast<unsigned char>(date[i]))) {
      errorMessage = "Appointment date: must be in YYYY-MM-DD format.";
      return false;
    }
  }

  int year = 0;
  int month = 0;
  int day = 0;
  try {
    year = std::stoi(date.substr(0, 4));
    month = std::stoi(date.substr(5, 2));
    day = std::stoi(date.substr(8, 2));
  } catch (...) {
    errorMessage = "Appointment date: invalid date values.";
    return false;
  }

  if (year < 1900 || year > 2100) {
    errorMessage = "Appointment date: year must be between 1900 and 2100.";
    return false;
  }

  if (month < 1 || month > 12) {
    errorMessage = "Appointment date: month must be between 01 and 12.";
    return false;
  }

  int daysInMonth = 31;
  if (month == 4 || month == 6 || month == 9 || month == 11) {
    daysInMonth = 30;
  } else if (month == 2) {
    bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    daysInMonth = isLeap ? 29 : 28;
  }

  if (day < 1 || day > daysInMonth) {
    errorMessage = "Appointment date: day is invalid for the specified month and year.";
    return false;
  }

  return true;
}

bool Appointment::validateTime(const std::string &time, std::string &errorMessage) {
  if (!passesSecurityCheck(time, errorMessage, "Appointment time")) {
    return false;
  }

  // VR-11: HH:MM 24-hour clock
  if (time.size() != 5 || time[2] != ':') {
    errorMessage = "Appointment time: must be in HH:MM format (24-hour clock).";
    return false;
  }

  for (std::size_t i = 0; i < 5; ++i) {
    if (i == 2) continue;
    if (!std::isdigit(static_cast<unsigned char>(time[i]))) {
      errorMessage = "Appointment time: must be in HH:MM format (24-hour clock).";
      return false;
    }
  }

  int hour = 0;
  int minute = 0;
  try {
    hour = std::stoi(time.substr(0, 2));
    minute = std::stoi(time.substr(3, 2));
  } catch (...) {
    errorMessage = "Appointment time: invalid time values.";
    return false;
  }

  if (hour < 0 || hour > 23) {
    errorMessage = "Appointment time: hour must be between 00 and 23.";
    return false;
  }

  if (minute < 0 || minute > 59) {
    errorMessage = "Appointment time: minute must be between 00 and 59.";
    return false;
  }

  return true;
}

bool Appointment::validateStatus(const std::string &statusStr,
                                 AppointmentStatus &outStatus,
                                 std::string &errorMessage) {
  if (!stringToStatus(statusStr, outStatus)) {
    errorMessage = "Appointment status: must be exactly 'Booked' or 'Cancelled'.";
    return false;
  }
  return true;
}

bool Appointment::validate(std::string &errorMessage) const {
  // VR-12: Appointment ID
  if (!validateId(id_, errorMessage)) {
    return false;
  }

  // VR-01: Patient ID
  if (patientId_.empty() || patientId_.size() > 10) {
    errorMessage = "Patient ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : patientId_) {
    if (!std::isalnum(c)) {
      errorMessage = "Patient ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(patientId_, errorMessage, "Patient ID")) {
    return false;
  }

  // VR-01: Doctor ID
  if (doctorId_.empty() || doctorId_.size() > 10) {
    errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
    return false;
  }
  for (unsigned char c : doctorId_) {
    if (!std::isalnum(c)) {
      errorMessage = "Doctor ID: must contain 1-10 alphanumeric characters.";
      return false;
    }
  }
  if (!passesSecurityCheck(doctorId_, errorMessage, "Doctor ID")) {
    return false;
  }

  // VR-10: Appointment Date
  if (!validateDate(date_, errorMessage)) {
    return false;
  }

  // VR-11: Appointment Time
  if (!validateTime(time_, errorMessage)) {
    return false;
  }

  // VR-13: Status
  if (status_ != AppointmentStatus::Booked &&
      status_ != AppointmentStatus::Cancelled) {
    errorMessage = "Appointment status: must be Booked or Cancelled.";
    return false;
  }

  return true;
}

bool Appointment::isValid() const {
  std::string unused;
  return validate(unused);
}

bool Appointment::conflictsWith(const std::string &docId,
                                const std::string &d,
                                const std::string &t) const {
  if (status_ != AppointmentStatus::Booked) {
    return false; // Cancelled appointments are excluded from conflict check (HMS-F-021)
  }
  return (toUpper(doctorId_) == toUpper(trim(docId))) &&
         (date_ == trim(d)) &&
         (time_ == trim(t));
}

void Appointment::cancel() {
  status_ = AppointmentStatus::Cancelled;
}

const std::string &Appointment::getId() const { return id_; }
const std::string &Appointment::getPatientId() const { return patientId_; }
const std::string &Appointment::getDoctorId() const { return doctorId_; }
const std::string &Appointment::getDate() const { return date_; }
const std::string &Appointment::getTime() const { return time_; }
AppointmentStatus Appointment::getStatus() const { return status_; }
std::string Appointment::getStatusString() const { return statusToString(status_); }

bool Appointment::isBooked() const { return status_ == AppointmentStatus::Booked; }
bool Appointment::isCancelled() const { return status_ == AppointmentStatus::Cancelled; }

std::string Appointment::toCsv() const {
  std::ostringstream oss;
  oss << id_ << ','
      << patientId_ << ','
      << doctorId_ << ','
      << date_ << ','
      << time_ << ','
      << getStatusString();
  return oss.str();
}

bool Appointment::fromCsv(const std::string &line,
                          Appointment &outAppt,
                          std::string &errorMessage) {
  const std::vector<std::string> tokens = splitCsvLine(line);
  if (tokens.size() != 6) {
    errorMessage = "Malformed appointment CSV line (expected 6 columns).";
    return false;
  }

  const std::string &apptId = tokens[0];
  const std::string &patId = tokens[1];
  const std::string &docId = tokens[2];
  const std::string &date = tokens[3];
  const std::string &time = tokens[4];
  const std::string &statusStr = tokens[5];

  AppointmentStatus st = AppointmentStatus::Booked;
  if (!validateStatus(statusStr, st, errorMessage)) {
    return false;
  }

  Appointment candidate(apptId, patId, docId, date, time, st);
  if (!candidate.validate(errorMessage)) {
    return false;
  }

  outAppt = std::move(candidate);
  return true;
}

} // namespace hms
