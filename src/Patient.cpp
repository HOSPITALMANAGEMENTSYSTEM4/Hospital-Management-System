
#include "hms/Patient.h"

#include <utility>

namespace hms {

Patient::Patient(std::string id, std::string name, int age, std::string gender,
                 std::string contactNumber, std::string address,
                 std::string medicalHistory)
    : id_(std::move(id)), name_(std::move(name)), age_(age),
      gender_(std::move(gender)), contactNumber_(std::move(contactNumber)),
      address_(std::move(address)), medicalHistory_(std::move(medicalHistory)) {
}

bool Patient::isValid() const {
  return !id_.empty() && !name_.empty() && age_ >= 1 && age_ <= 130 &&
         !gender_.empty() && !contactNumber_.empty() && !address_.empty();
}

const std::string &Patient::getId() const { return id_; }
const std::string &Patient::getName() const { return name_; }
int Patient::getAge() const { return age_; }
const std::string &Patient::getGender() const { return gender_; }
const std::string &Patient::getContactNumber() const { return contactNumber_; }
const std::string &Patient::getAddress() const { return address_; }
const std::string &Patient::getMedicalHistory() const {
  return medicalHistory_;
}

} // namespace hms
