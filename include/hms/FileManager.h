
#pragma once

#include "hms/Patient.h"

#include <string>
#include <vector>

namespace hms {

class FileManager {
public:
  explicit FileManager(std::string patientFilePath = "data/patients.csv");

  std::vector<Patient> loadPatients() const;
  bool patientIdExists(const std::string &patientId) const;
  bool savePatient(const Patient &patient, std::string &errorMessage) const;

private:
  std::string patientFilePath_;
};

} // namespace hms
