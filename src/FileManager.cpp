#include "hms/FileManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace hms {
namespace {

const char *CSV_HEADER =
    "patient_id,name,age,gender,contact_number,address,medical_history\n";

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

  return age >= 1 && age <= 130;
}

} // namespace

FileManager::FileManager(std::string patientFilePath)
    : patientFilePath_(std::move(patientFilePath)) {}

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
  for (const Patient &patient : loadPatients()) {
    if (patient.getId() == patientId) {
      return true;
    }
  }

  return false;
}

bool FileManager::savePatient(const Patient &patient,
                              std::string &errorMessage) const {
  if (!patient.isValid()) {
    errorMessage = "Patient details are invalid.";
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

} // namespace hms
