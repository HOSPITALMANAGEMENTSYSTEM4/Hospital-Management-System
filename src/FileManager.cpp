

#include "hms/FileManager.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>
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

struct ParsedCsv {
  std::vector<std::vector<std::string>> records;
  std::size_t malformedCount{0};
};

// Parse one physical line at a time so a malformed quoted record cannot
// consume the remaining file. Newlines inside fields are not supported by
// the application data models and are treated as malformed records.
ParsedCsv parseCsv(const std::string &content) {
  ParsedCsv parsed;
  std::istringstream lines(content);
  std::string line;

  while (std::getline(lines, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }

    std::vector<std::string> record;
    std::string field;
    bool insideQuotes = false;
    bool afterClosingQuote = false;
    bool malformed = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
      const char character = line[i];

      if (insideQuotes) {
        if (character == '"') {
          if (i + 1 < line.size() && line[i + 1] == '"') {
            field += '"';
            ++i;
          } else {
            insideQuotes = false;
            afterClosingQuote = true;
          }
        } else {
          field += character;
        }
      } else if (afterClosingQuote) {
        if (character == ',') {
          record.push_back(field);
          field.clear();
          afterClosingQuote = false;
        } else {
          malformed = true;
          break;
        }
      } else if (character == ',') {
        record.push_back(field);
        field.clear();
      } else if (character == '"') {
        if (field.empty()) {
          insideQuotes = true;
        } else {
          malformed = true;
          break;
        }
      } else {
        field += character;
      }
    }

    if (insideQuotes) {
      malformed = true;
    }

    if (malformed) {
      ++parsed.malformedCount;
      continue;
    }

    record.push_back(field);
    parsed.records.push_back(std::move(record));
  }

  return parsed;
}

bool parseAge(const std::string &value, int &age) {
  std::istringstream input(value);
  char extra = '\0';

  if (!(input >> age) || (input >> extra)) {
    return false;
  }

  return age >= 0 && age <= 120;
}

std::string trim(const std::string &value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t first = value.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = value.find_last_not_of(whitespace);
  return value.substr(first, last - first + 1);
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

std::vector<Patient> FileManager::loadPatients(std::size_t *malformedCount) const {
  std::vector<Patient> patients;
  std::size_t skipped = 0;
  std::unordered_set<std::string> seenPatientIds;
  std::ifstream input(patientFilePath_);

  if (!input) {
    if (malformedCount) {
      *malformedCount = 0;
    }
    return patients;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  ParsedCsv parsed = parseCsv(buffer.str());
  skipped += parsed.malformedCount;

  for (const auto &row : parsed.records) {
    if (!row.empty()) {
      const std::string firstCol = toLower(trim(row[0]));
      if (firstCol == "patient_id" || firstCol == "patientid") {
        continue;
      }
    }
    if (row.size() != 7) {
      ++skipped;
      continue;
    }

    int age = 0;
    if (!parseAge(row[2], age)) {
      ++skipped;
      continue;
    }

    Patient patient(row[0], row[1], age, row[3], row[4], row[5], row[6]);
    if (!patient.isValid()) {
      ++skipped;
      continue;
    }

    const std::string normalizedId =
        toLower(trim(patient.getId()));

    if (!seenPatientIds.insert(normalizedId).second) {
      ++skipped;
      continue;
    }

    patients.push_back(std::move(patient));
  }

  if (malformedCount) {
    *malformedCount = skipped;
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

bool FileManager::savePatients(const std::vector<Patient> &patients,
                               std::string &errorMessage) const {
  for (const auto &patient : patients) {
    std::string validationError;
    if (!patient.validate(validationError)) {
      errorMessage = validationError;
      return false;
    }
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

  const std::string tempPath = patientFilePath_ + ".tmp";
  const std::string backupPath = patientFilePath_ + ".bak";

  {
    std::ofstream output(tempPath,
                         std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output) {
      errorMessage = "Changes may not have been saved";
      return false;
    }

    output << CSV_HEADER;
    for (const auto &p : patients) {
      output << escapeCsv(p.getId()) << ',' << escapeCsv(p.getName()) << ','
             << p.getAge() << ',' << escapeCsv(p.getGender()) << ','
             << escapeCsv(p.getContactNumber()) << ','
             << escapeCsv(p.getAddress()) << ','
             << escapeCsv(p.getMedicalHistory()) << '\n';
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
    // Never delete an existing backup automatically: it may be the only
    // remaining copy of the original data after an earlier failed write.
    const bool backupExists = std::filesystem::exists(backupPath, ec);
    if (ec || backupExists) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      if (backupExists) {
        errorMessage += ". Existing backup was preserved: " + backupPath;
      }
      return false;
    }

    std::filesystem::rename(filePath, backupPath, ec);
    if (ec) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }

    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
      std::error_code restoreError;
      std::filesystem::rename(backupPath, filePath, restoreError);
      std::remove(tempPath.c_str());

      errorMessage = "Changes may not have been saved";
      if (restoreError) {
        errorMessage += ". Original data remains in backup file: " + backupPath;
      }
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

bool FileManager::updatePatient(const Patient &patient,
                                std::string &errorMessage) const {
  std::string validationError;
  if (!patient.validate(validationError)) {
    errorMessage = validationError;
    return false;
  }

  std::vector<Patient> patients = loadPatients();
  const std::string targetId = toLower(patient.getId());
  bool found = false;

  for (auto &existing : patients) {
    if (toLower(existing.getId()) == targetId) {
      existing = patient;
      found = true;
      break;
    }
  }

  if (!found) {
    errorMessage = "Patient not found";
    return false;
  }

  return savePatients(patients, errorMessage);
}

std::vector<Doctor> FileManager::loadDoctors(std::size_t *malformedCount) const {
  std::vector<Doctor> doctors;
  std::size_t skipped = 0;
  std::unordered_set<std::string> seenDoctorIds;
  std::ifstream input(doctorFilePath_);

  if (!input) {
    if (malformedCount) {
      *malformedCount = 0;
    }
    return doctors;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  ParsedCsv parsed = parseCsv(buffer.str());
  skipped += parsed.malformedCount;

  for (const auto &row : parsed.records) {
    if (!row.empty()) {
      const std::string firstCol = toLower(trim(row[0]));
      if (firstCol == "doctor_id" || firstCol == "doctorid") {
        continue;
      }
    }
    if (row.size() != 6) {
      ++skipped;
      continue;
    }

    Doctor doctor(row[0], row[1], row[2], row[3], row[4], row[5]);
    if (!doctor.isValid()) {
      ++skipped;
      continue;
    }

    const std::string normalizedId =
        toLower(trim(doctor.getId()));

    if (!seenDoctorIds.insert(normalizedId).second) {
      ++skipped;
      continue;
    }

    doctors.push_back(std::move(doctor));
  }

  if (malformedCount) {
    *malformedCount = skipped;
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

bool FileManager::saveDoctors(const std::vector<Doctor> &doctors,
                              std::string &errorMessage) const {
  for (const auto &doctor : doctors) {
    std::string validationError;
    if (!doctor.validate(validationError)) {
      errorMessage = validationError;
      return false;
    }
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

  const std::string tempPath = doctorFilePath_ + ".tmp";
  const std::string backupPath = doctorFilePath_ + ".bak";

  {
    std::ofstream output(tempPath,
                         std::ios::out | std::ios::trunc | std::ios::binary);
    if (!output) {
      errorMessage = "Changes may not have been saved";
      return false;
    }

    output << DOCTOR_CSV_HEADER;
    for (const auto &d : doctors) {
      output << escapeCsv(d.getId()) << ',' << escapeCsv(d.getName()) << ','
             << escapeCsv(d.getSpecialization()) << ','
             << escapeCsv(d.getDepartment()) << ','
             << escapeCsv(d.getContactNumber()) << ','
             << escapeCsv(d.getAvailability()) << '\n';
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
    // Never delete an existing backup automatically: it may be the only
    // remaining copy of the original data after an earlier failed write.
    const bool backupExists = std::filesystem::exists(backupPath, ec);
    if (ec || backupExists) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      if (backupExists) {
        errorMessage += ". Existing backup was preserved: " + backupPath;
      }
      return false;
    }

    std::filesystem::rename(filePath, backupPath, ec);
    if (ec) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }

    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
      std::error_code restoreError;
      std::filesystem::rename(backupPath, filePath, restoreError);
      std::remove(tempPath.c_str());

      errorMessage = "Changes may not have been saved";
      if (restoreError) {
        errorMessage +=
            ". Original data remains in backup file: " + backupPath;
      }
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

bool FileManager::updateDoctor(const Doctor &doctor,
                               std::string &errorMessage) const {
  std::string validationError;
  if (!doctor.validate(validationError)) {
    errorMessage = validationError;
    return false;
  }

  std::vector<Doctor> doctors = loadDoctors();
  const std::string targetId = toLower(doctor.getId());
  bool found = false;

  for (auto &existing : doctors) {
    if (toLower(existing.getId()) == targetId) {
      existing = doctor;
      found = true;
      break;
    }
  }

  if (!found) {
    errorMessage = "Doctor not found";
    return false;
  }

  return saveDoctors(doctors, errorMessage);
}



std::vector<Appointment> FileManager::loadAppointments(
    std::size_t *malformedCount) const {
  std::vector<Appointment> appointments;
  std::size_t skipped = 0;
  std::ifstream input(appointmentFilePath_);

  if (!input) {
    if (malformedCount) {
      *malformedCount = 0;
    }
    return appointments;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  ParsedCsv parsed = parseCsv(buffer.str());
  skipped += parsed.malformedCount;

  const bool checkPatients =
      std::filesystem::exists(patientFilePath_);
  const bool checkDoctors =
      std::filesystem::exists(doctorFilePath_);

  std::unordered_set<std::string> validPatientIds;
  if (checkPatients) {
    for (const auto &patient : loadPatients()) {
      validPatientIds.insert(toLower(trim(patient.getId())));
    }
  }

  std::unordered_set<std::string> validDoctorIds;
  if (checkDoctors) {
    for (const auto &doctor : loadDoctors()) {
      validDoctorIds.insert(toLower(trim(doctor.getId())));
    }
  }

  // Track IDs in a hash set instead of scanning every prior appointment.
  std::unordered_set<std::string> seenAppointmentIds;
  seenAppointmentIds.reserve(parsed.records.size());

  appointments.reserve(parsed.records.size());

  for (const auto &row : parsed.records) {
    if (!row.empty()) {
      const std::string firstCol = toLower(trim(row[0]));
      if (firstCol == "appointment_id" ||
          firstCol == "appointmentid") {
        continue;
      }
    }

    if (row.size() != 6) {
      ++skipped;
      continue;
    }

    AppointmentStatus status = AppointmentStatus::Booked;
    if (!stringToStatus(row[5], status)) {
      ++skipped;
      continue;
    }

    Appointment appointment(row[0], row[1], row[2], row[3], row[4],
                            status);

    if (!appointment.isValid()) {
      ++skipped;
      continue;
    }

    bool isOrphan = false;

    if (checkPatients &&
        validPatientIds.find(
            toLower(trim(appointment.getPatientId()))) ==
            validPatientIds.end()) {
      isOrphan = true;
    }

    if (checkDoctors &&
        validDoctorIds.find(
            toLower(trim(appointment.getDoctorId()))) ==
            validDoctorIds.end()) {
      isOrphan = true;
    }

    if (isOrphan) {
      ++skipped;
      continue;
    }

    const std::string normalizedId =
        toLower(trim(appointment.getId()));

    if (!seenAppointmentIds.insert(normalizedId).second) {
      ++skipped;
      continue;
    }

    appointments.push_back(std::move(appointment));
  }

  if (malformedCount) {
    *malformedCount = skipped;
  }

  return appointments;
}


bool FileManager::appointmentIdExists(
    const std::string &appointmentId) const {
  const std::string needle = toLower(trim(appointmentId));

  std::ifstream input(appointmentFilePath_);
  if (!input) {
    return false;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();

  const ParsedCsv parsed = parseCsv(buffer.str());

  for (const auto &row : parsed.records) {
    if (row.empty()) {
      continue;
    }

    const std::string id = toLower(trim(row[0]));

    // Ignore the CSV header.
    if (id == "appointment_id" || id == "appointmentid") {
      continue;
    }

    if (id == needle) {
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
    // Never delete an existing backup automatically: it may be the only
    // remaining copy of the original data after an earlier failed write.
    const bool backupExists = std::filesystem::exists(backupPath, ec);
    if (ec || backupExists) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      if (backupExists) {
        errorMessage += ". Existing backup was preserved: " + backupPath;
      }
      return false;
    }

    std::filesystem::rename(filePath, backupPath, ec);
    if (ec) {
      std::remove(tempPath.c_str());
      errorMessage = "Changes may not have been saved";
      return false;
    }

    std::filesystem::rename(tempPath, filePath, ec);
    if (ec) {
      std::error_code restoreError;
      std::filesystem::rename(backupPath, filePath, restoreError);
      std::remove(tempPath.c_str());

      errorMessage = "Changes may not have been saved";
      if (restoreError) {
        errorMessage +=
            ". Original data remains in backup file: " + backupPath;
      }
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
