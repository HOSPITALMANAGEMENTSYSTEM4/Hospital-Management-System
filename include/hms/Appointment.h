#pragma once

#include <string>

namespace hms {

enum class AppointmentStatus {
    Booked,
    Cancelled
};

using Status = AppointmentStatus;

std::string statusToString(AppointmentStatus status);
bool stringToStatus(const std::string& str, AppointmentStatus& outStatus);

class Appointment {
public:
    Appointment();

    Appointment(
        std::string id,
        std::string patientId,
        std::string doctorId,
        std::string date,
        std::string time,
        AppointmentStatus status = AppointmentStatus::Booked
    );

    bool isValid() const;
    bool validate(std::string& errorMessage) const;

    static bool validateId(
        const std::string& id,
        std::string& errorMessage
    );

    static bool validateDate(
        const std::string& date,
        std::string& errorMessage
    );

    static bool validateTime(
        const std::string& time,
        std::string& errorMessage
    );

    static bool validateStatus(
        const std::string& statusStr,
        AppointmentStatus& outStatus,
        std::string& errorMessage
    );

    static bool passesSecurityCheck(
        const std::string& value,
        std::string& errorMessage,
        const std::string& fieldName
    );

    bool conflictsWith(
        const std::string& docId,
        const std::string& date,
        const std::string& time
    ) const;

    void cancel();

    const std::string& getId() const;
    const std::string& getPatientId() const;
    const std::string& getDoctorId() const;
    const std::string& getDate() const;
    const std::string& getTime() const;

    AppointmentStatus getStatus() const;
    std::string getStatusString() const;

    bool isBooked() const;
    bool isCancelled() const;

    std::string toCsv() const;

    static bool fromCsv(
        const std::string& line,
        Appointment& outAppt,
        std::string& errorMessage
    );

private:
    std::string id_;
    std::string patientId_;
    std::string doctorId_;
    std::string date_;
    std::string time_;
    AppointmentStatus status_{AppointmentStatus::Booked};
};

} // namespace hms
