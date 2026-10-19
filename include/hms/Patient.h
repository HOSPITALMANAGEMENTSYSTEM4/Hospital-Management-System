
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

    bool isValid() const;

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

}
