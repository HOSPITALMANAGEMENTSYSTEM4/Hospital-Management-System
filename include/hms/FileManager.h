#pragma once

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/Patient.h"

#include <cstddef>
#include <string>
#include <vector>

namespace hms {

class FileManager {
public:
    explicit FileManager(
        std::string patientFilePath = "data/patients.csv",
        std::string doctorFilePath = "data/doctors.csv",
        std::string appointmentFilePath = "data/appointments.csv"
    );

    // Patient operations
    std::vector<Patient> loadPatients(
        std::size_t *malformedCount = nullptr
    ) const;

    bool patientIdExists(const std::string &patientId) const;

    bool savePatient(
        const Patient &patient,
        std::string &errorMessage
    ) const;

    bool savePatients(
        const std::vector<Patient> &patients,
        std::string &errorMessage
    ) const;

    bool updatePatient(
        const Patient &patient,
        std::string &errorMessage
    ) const;

    // Doctor operations
    std::vector<Doctor> loadDoctors(
        std::size_t *malformedCount = nullptr
    ) const;

    bool doctorIdExists(const std::string &doctorId) const;

    bool saveDoctor(
        const Doctor &doctor,
        std::string &errorMessage
    ) const;

    // Appointment operations
    std::vector<Appointment> loadAppointments(
        std::size_t *malformedCount = nullptr
    ) const;

    bool appointmentIdExists(const std::string &appointmentId) const;

    bool saveAppointment(
        const Appointment &appointment,
        std::string &errorMessage
    ) const;

    bool saveAppointments(
        const std::vector<Appointment> &appointments,
        std::string &errorMessage
    ) const;

    // File path getters
    const std::string &getPatientFilePath() const;
    const std::string &getDoctorFilePath() const;
    const std::string &getAppointmentFilePath() const;

private:
    std::string patientFilePath_;
    std::string doctorFilePath_;
    std::string appointmentFilePath_;
};

} // namespace hms