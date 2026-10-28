
#pragma once

#include <string>

namespace hms {

class Patient {
public:
    Patient(std::string id,
            std::string name,
            int age,
            std::string gender,
            std::string contactNumber,
            std::string address,
            std::string medicalHistory = "");

    // Returns true only when every field satisfies VR-01 through VR-07 and
    // VR-14.  Used by FileManager::savePatient() as a gate before persistence.
    bool isValid() const;

    // Validates each field in turn.  On the first failure, writes a
    // human-readable message describing the field and the violated rule into
    // `errorMessage` and returns false.  Returns true and leaves `errorMessage`
    // unchanged when all fields are valid.
    bool validate(std::string &errorMessage) const;

    // Static allow-list validation helpers (Table 4-1 rules)
    static bool validateId(const std::string &id, std::string &errorMessage);
    static bool validateName(const std::string &name, std::string &errorMessage);
    static bool validateAge(int age, std::string &errorMessage);
    static bool validateAge(const std::string &input, int &outAge, std::string &errorMessage);
    static bool validateGender(const std::string &gender, std::string &errorMessage);
    static bool validateContactNumber(const std::string &contactNumber, std::string &errorMessage);
    static bool validateAddress(const std::string &address, std::string &errorMessage);
    static bool validateMedicalHistory(const std::string &medicalHistory, std::string &errorMessage);
    static bool passesSecurityCheck(const std::string &value, std::string &errorMessage, const std::string &fieldName);

    const std::string& getId() const;
    const std::string& getName() const;
    int getAge() const;
    const std::string& getGender() const;
    const std::string& getContactNumber() const;
    const std::string& getAddress() const;
    const std::string& getMedicalHistory() const;

private:
    std::string id_;
    std::string name_;
    int age_;
    std::string gender_;
    std::string contactNumber_;
    std::string address_;
    std::string medicalHistory_;
};

} // namespace hms
