
#pragma once

#include "hms/FileManager.h"

namespace hms {

class HospitalSystem {
public:
  void run();

private:
  void registerPatient();

  FileManager fileManager_;
};

} // namespace hms
