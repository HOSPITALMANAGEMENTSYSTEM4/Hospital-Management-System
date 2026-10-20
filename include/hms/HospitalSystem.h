
#pragma once

#include "hms/FileManager.h"

#include <vector>

namespace hms {

class HospitalSystem {
public:
  HospitalSystem() = default;
  explicit HospitalSystem(FileManager fileManager);

  void run();
  void addDoctor();

private:
  void registerPatient();

  // Load and display records when the application starts.
  void loadRecordsAtStartup();
  void displayRecords() const;

  FileManager fileManager_;

  // Keep the loaded records in memory.
  std::vector<Patient> patients_;
  std::vector<Doctor> doctors_;
  std::vector<Appointment> appointments_;
};

} // namespace hms
