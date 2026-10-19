#pragma once

#include "hms/FileManager.h"

namespace hms {

class HospitalSystem {
public:
  HospitalSystem() = default;
  explicit HospitalSystem(FileManager fileManager);

  void run();
  void addDoctor();
  void searchPatients();

private:
  void registerPatient();

  FileManager fileManager_;
};

} // namespace hms
