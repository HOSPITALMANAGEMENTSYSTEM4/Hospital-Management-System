#include "hms/FileManager.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace hms {
namespace {

const char *CSV_HEADER =
    "patient_id,name,age,gender,contact_number,address,medical_history\n";

const char *DOCTOR_CSV_HEADER =
    "doctor_id,name,specialization,department,contact_number,availability\n";

const char *APPOINTMENT_CSV_HEADER =
    "AppointmentID,PatientID,DoctorID,Date,Time,Status\n";

std::string escapeCsv(const std::string &value) {
  if (value.find_first_of(",\"\r\n") == std::string::npos) {
    return value;
  }

  std::string escaped = "\"";

  for (char character : value) {
    if (character == '"') {
      escaped += "\"\"";
    } else {
      escaped += character;
    }
  }

  escaped += '"';
  return escaped;
}

std::vector<std::vector<std::string>> parseCsv(const std::string &content) {
  std::vector<std::vector<std::string>> records;
  std::vector<std::string> record;
  std::string field;
  bool insideQuotes = false;

  for (std::size_t i = 0; i < content.size(); ++i) {
    const char character = content[i];

    if (insideQuotes) {
      if (character == '"') {
        if (i + 1 < content.size() && content[i + 1] == '"') {
          field += '"';
          ++i;
        } else {
          insideQuotes = false;
        }
      } else {
        field += character;
      }
    } else if (character == '"' && field.empty()) {
      insideQuotes = true;
    } else if (character == ',') {
      record.push_back(field);
      field.clear();
    } else if (character == '\n' || character == '\r') {
      if (character == '\r' && i + 1 < content.size() &&
          content[i + 1] == '\n') {
        ++i;
      }

      record.push_back(field);
      field.clear();

      if (!(record.size() == 1 && record.front().empty())) {
        records.push_back(record);
      }

      record.clear();
    } else {
      field += character;
    }
  }

  if (insideQuotes) {
    return {}; // Reject a file with an unterminated quoted field.
  }

  if (!field.empty() || !record.empty()) {
    record.push_back(field);

    if (!(record.size() == 1 && record.front().empty())) {
      records.push_back(record);
    }
  }

  return records;
}

bool parseAge(const std::string &value, int &age) {
  std::istringstream input(value);
  char extra = '\0';

  if (!(input >> age) || (input >> extra)) {
    return false;
  }

  return age >= 0 && age <= 120;
}

std::string toLower(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

} // namespace

FileManager::FileManager(std::string patientFilePath,
                         std::string doctorFilePath,
                         std::string appointmentFilePath)
    : patientFilePath_(std::move(patientFilePath)),
      doctorFilePath_(std::move(doctorFilePath)),
      appointmentFilePath_(std::move(appointmentFilePath)) {}

const std::string &FileManager::getPatientFilePath() const {
  return patientFilePath_;
}

const std::string &FileManager::getDoctorFilePath() const {
  return doctorFilePath_;
}

const std::string &FileManager::getAppointmentFilePath() const {
  return appointmentFilePath_;
}

std::vector<Patient> FileManager::loadPatients() const {
  std::vector<Patient> patients;
  std::ifstream input(patientFilePath_);

  if (!input) {
    return patients;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  const auto records = parseCsv(buffer.str());

  for (const auto &row : records) {
    if (row.size() != 7 || row[0] == "patient_id") {
      continue;
    }

    int age = 0;

    if (!parseAge(row[2], age)) {
      continue;
    }

    Patient patient(row[0], row[1], age, row[3], row[4], row[5], row[6]);

    if (patient.isValid()) {
      patients.push_back(std::move(patient));
    }
  }

  return patients;
}

bool FileManager::patientIdExists(const std::string &patientId) const {
  const std::string needle = toLower(patientId);

  for (const Patient &patient : loadPatients()) {
    if (toLower(patient.getId()) == needle) {
      return true;
    }
  }

  return false;
}

bool FileManager::savePatient(const Patient &patient,
                              std::string &errorMessage) const {
  std::string validationError;
  if (!patient.validate(validationError)) {
    errorMessage = validationError;
    return false;
  }

  if (patientIdExists(patient.getId())) {
    errorMessage = "A patient with this ID already exists.";
    return false;
  }

  const std::filesystem::path filePath(patientFilePath_);
  const std::filesystem::path parent = filePath.parent_path();

  std::error_code filesystemError;

  if (!parent.empty()) {
    std::filesystem::create_directories(parent, filesystemError);

    if (filesystemError) {
      errorMessage =
          "Could not create the data directory: " + filesystemError.message();
      return false;
    }
  }

  bool needsHeader = true;

  {
    std::ifstream existing(patientFilePath_, std::ios::binary);

    if (existing) {
      existing.seekg(0, std::ios::end);
      needsHeader = existing.tellg() == std::streampos(0);
    }
  }

  std::ofstream output(patientFilePath_,
                       std::ios::out | std::ios::app | std::ios::binary);

  if (!output) {
    errorMessage = "Could not open the patient CSV file for writing.";
    return false;
  }

  if (needsHeader) {
    output << CSV_HEADER;
  }

  output << escapeCsv(patient.getId()) << ',' << escapeCsv(patient.getName())
         << ',' << patient.getAge() << ',' << escapeCsv(patient.getGender())
         << ',' << escapeCsv(patient.getContactNumber()) << ','
         << escapeCsv(patient.getAddress()) << ','
         << escapeCsv(patient.getMedicalHistory()) << '\n';

  output.flush();

  if (!output) {
    errorMessage = "Writing the patient record failed.";
    return false;
  }

  return true;
}

std::vector<Doctor> FileManager::loadDoctors() const {
  std::vector<Doctor> doctors;
  std::ifstream input(doctorFilePath_);

  if (!input) {
    return doctors;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  const auto records = parseCsv(buffer.str());

  for (const auto &row : records) {
    if (row.size() != 6) {
      continue;
    }

    const std::string firstCol = toLower(row[0]);
    if (firstCol == "doctor_id" || firstCol == "doctorid") {
      continue;
    }

    Doctor doctor(row[0], row[1], row[2], row[3], row[4], row[5]);

    if (doctor.isValid()) {
      doctors.push_back(std::move(doctor));
    }
  }

  return doctors;
}

bool FileManager::doctorIdExists(const std::string &doctorId) const {
  const std::string needle = toLower(doctorId);

  for (const Doctor &doctor : loadDoctors()) {
    if (toLower(doctor.getId()) == needle) {
      return true;
    }
  }

  return false;
}

bool FileManager::saveDoctor(const Doctor &doctor,
                             std::string &errorMessage) const {
  std::string validationError;
  if (!doctor.validate(validationError)) {
    errorMessage = validationError;
    return false;
  }

  if (doctorIdExists(doctor.getId())) {
    errorMessage = "A doctor with this ID already exists.";
    return false;
  }

  const std::filesystem::path filePath(doctorFilePath_);
  const std::filesystem::path parent = filePath.parent_path();

  std::error_code filesystemError;

  if (!parent.empty()) {
    std::filesystem::create_directories(parent, filesystemError);

    if (filesystemError) {
      errorMessage =
          "Could not create the data directory: " + filesystemError.message();
      return false;
    }
  }

  bool needsHeader = true;

  {
    std::ifstream existing(doctorFilePath_, std::ios::binary);

    if (existing) {
      existing.seekg(0, std::ios::end);
      needsHeader = existing.tellg() == std::streampos(0);
    }
  }

  std::ofstream output(doctorFilePath_,
                       std::ios::out | std::ios::app | std::ios::binary);

  if (!output) {
    errorMessage = "Could not open the doctor CSV file for writing.";
    return false;
  }

  if (needsHeader) {
    output << DOCTOR_CSV_HEADER;
  }

  output << escapeCsv(doctor.getId()) << ',' << escapeCsv(doctor.getName())
         << ',' << escapeCsv(doctor.getSpecialization()) << ','
         << escapeCsv(doctor.getDepartment()) << ','
         << escapeCsv(doctor.getContactNumber()) << ','
         << escapeCsv(doctor.getAvailability()) << '\n';

  output.flush();

  if (!output) {
    errorMessage = "Writing the doctor record failed.";
    return false;
  }

  return true;
}

std::vector<Appointment> FileManager::loadAppointments() const {
  std::vector<Appointment> appointments;
  std::ifstream input(appointmentFilePath_);

  if (!input) {
    return appointments;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  const auto records = parseCsv(buffer.str());

  for (const auto &row : records) {
    if (row.size() != 6) {
      continue;
    }

    const std::string firstCol = toLower(row[0]);
    if (firstCol == "appointment_id" || firstCol == "appointmentid") {
      continue;
    }

    AppointmentStatus st = AppointmentStatus::Booked;
    if (!stringToStatus(row[5], st)) {
      continue;
    }

    Appointment appt(row[0], row[1], row[2], row[3], row[4], st);

    if (appt.isValid()) {
      bool dup = false;
      for (const auto &existing : appointments) {
        if (toLower(existing.getId()) == toLower(appt.getId())) {
          dup = true;
          break;
        }
      }
      if (!dup) {
        appointments.push_back(std::move(appt));
      }
    }
  }

  return appointments;
}

bool FileManager::appointmentIdExists(const std::string &appointmentId) const {
  const std::string needle = toLower(appointmentId);

  for (const Appointment &appt : loadAppointments()) {
    if (toLower(appt.getId()) == needle) {
      return true;
    }
  }

  return false;
}

bool FileManager::saveAppointment(const Appointment &appointment,
                                 std::string &errorMessage) const {
  std::string validationError;
  if (!appointment.validate(validationError)) {
    errorMessage = validationError;
    return false;
  }

  if (appointmentIdExists(appointment.getId())) {
    errorMessage = "An appointment with this ID already exists.";
    return false;
  }

  const std::filesystem::path filePath(appointmentFilePath_);
  const std::filesystem::path parent = filePath.parent_path();

  std::error_code filesystemError;

  if (!parent.empty()) {
    std::filesystem::create_directories(parent, filesystemError);

    if (filesystemError) {
      errorMessage =
          "Could not create the data directory: " + filesystemError.message();
      return false;
    }
  }

  bool needsHeader = true;

  {
    std::ifstream existing(appointmentFilePath_, std::ios::binary);

    if (existing) {
      existing.seekg(0, std::ios::end);
      needsHeader = existing.tellg() == std::streampos(0);
    }
  }

  std::ofstream output(appointmentFilePath_,
                       std::ios::out | std::ios::app | std::ios::binary);

  if (!output) {
    errorMessage = "Could not open the appointment CSV file for writing.";
    return false;
  }

  if (needsHeader) {
    output << APPOINTMENT_CSV_HEADER;
  }

  output << escapeCsv(appointment.getId()) << ','
         << escapeCsv(appointment.getPatientId()) << ','
         << escapeCsv(appointment.getDoctorId()) << ','
         << escapeCsv(appointment.getDate()) << ','
         << escapeCsv(appointment.getTime()) << ','
         << escapeCsv(appointment.getStatusString()) << '\n';

  output.flush();

  if (!output) {
    errorMessage = "Writing the appointment record failed.";
    return false;
  }

  return true;
}

bool FileManager::saveAppointments(const std::vector<Appointment> &appointments,
                                  std::string &errorMessage) const {
  const std::filesystem::path filePath(appointmentFilePath_);
  const std::filesystem::path parent = filePath.parent_path();

  std::error_code filesystemError;

  if (!parent.empty()) {
    std::filesystem::create_directories(parent, filesystemError);

    if (filesystemError) {
      errorMessage =
          "Could not create the data directory: " + filesystemError.message();
      return false;
    }
  }

  const std::string tempPath = appointmentFilePath_ + ".tmp";
  const std::string backupPath = appointmentFilePath_ + ".bak";

  {
    std::ofstream output(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output) {
      errorMessage = "Changes may not have been saved";
      return false;
    }

    output << APPOINTMENT_CSV_HEADER;
    for (const auto &appt : appointments) {
      output << escapeCsv(appt.getId()) << ','
             << escapeCsv(appt.getPatientId()) << ','
             << escapeCsv(appt.getDoctorId()) << ','
             << escapeCsv(appt.getDate()) << ','
             << escapeCsv(appt.getTime()) << ','
             << escapeCsv(appt.getStatusString()) << '\n';
    }

    output.flush();
    if (!output.good()) {
      output.close();
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }
  }

  std::error_code ec;
  if (std::filesystem::exists(filePath, ec)) {
    std::filesystem::remove(backupPath, ec);
    std::filesystem::rename(filePath, backupPath, ec);
    if (ec) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }

    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
      std::filesystem::rename(backupPath, filePath, ec);
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }

    std::filesystem::remove(backupPath, ec);
  } else {
    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }
  }

  return true;
}

} // namespace hms
