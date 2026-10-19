#pragma once

#include "hms/FileManager.h"

namespace hms {

class HospitalSystem {
public:
  HospitalSystem() = default;
  explicit HospitalSystem(FileManager fileManager);

  void run();
  void addDoctor();

private:
  void registerPatient();
  void viewPatientDetails();
  void listPatients();

  FileManager fileManager_;
};

} // namespace hms
