/*#pragma once

namespace hms {

class Appointment {
public:
    Appointment() = default;
};

}*/

#pragma once

#include <string>

namespace hms {

class Appointment {
public:
    Appointment() = default;

    Appointment(std::string id,
                std::string patientId,
                std::string doctorId,
                std::string date,
                std::string time,
                std::string status);

    const std::string& getId() const;
    const std::string& getPatientId() const;
    const std::string& getDoctorId() const;
    const std::string& getDate() const;
    const std::string& getTime() const;
    const std::string& getStatus() const;

private:
    std::string id_;
    std::string patientId_;
    std::string doctorId_;
    std::string date_;
    std::string time_;
    std::string status_;
};

}

