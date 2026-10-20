/*#include "hms/Appointment.h"*/

#include "hms/Appointment.h"

#include <utility>

namespace hms {

Appointment::Appointment(std::string id,
                         std::string patientId,
                         std::string doctorId,
                         std::string date,
                         std::string time,
                         std::string status)
    : id_(std::move(id)),
      patientId_(std::move(patientId)),
      doctorId_(std::move(doctorId)),
      date_(std::move(date)),
      time_(std::move(time)),
      status_(std::move(status)) {}

const std::string& Appointment::getId() const {
    return id_;
}

const std::string& Appointment::getPatientId() const {
    return patientId_;
}

const std::string& Appointment::getDoctorId() const {
    return doctorId_;
}

const std::string& Appointment::getDate() const {
    return date_;
}

const std::string& Appointment::getTime() const {
    return time_;
}

const std::string& Appointment::getStatus() const {
    return status_;
}

}

