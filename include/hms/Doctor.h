#pragma once

#include <string>

namespace hms {

class Doctor {
public:
  Doctor() = default;
  Doctor(std::string id,
         std::string name,
         std::string specialization,
         std::string department,
         std::string contactNumber,
         std::string availability);

  // Returns true only when every field satisfies VR-01, VR-02, VR-05, VR-08,
  // VR-09, and VR-14. Used by FileManager::saveDoctor() as a gate before persistence.
  bool isValid() const;

  // Validates each field in turn. On the first failure, writes a
  // human-readable message describing the field and the violated rule into
  // `errorMessage` and returns false. Returns true and leaves `errorMessage`
  // unchanged when all fields are valid.
  bool validate(std::string &errorMessage) const;

  // Static allow-list validation helpers (Table 4-1 rules)
  static bool validateId(const std::string &id, std::string &errorMessage);
  static bool validateName(const std::string &name, std::string &errorMessage);
  static bool validateSpecialization(const std::string &specialization,
                                    std::string &errorMessage);
  static bool validateDepartment(const std::string &department,
                                 std::string &errorMessage);
  static bool validateContactNumber(const std::string &contactNumber,
                                    std::string &errorMessage);
  static bool validateAvailability(const std::string &availability,
                                   std::string &errorMessage);
  static bool passesSecurityCheck(const std::string &value,
                                  std::string &errorMessage,
                                  const std::string &fieldName);

  const std::string &getId() const;
  const std::string &getName() const;
  const std::string &getSpecialization() const;
  const std::string &getDepartment() const;
  const std::string &getContactNumber() const;
  const std::string &getAvailability() const;

private:
  std::string id_;
  std::string name_;
  std::string specialization_;
  std::string department_;
  std::string contactNumber_;
  std::string availability_;
};

} // namespace hms
