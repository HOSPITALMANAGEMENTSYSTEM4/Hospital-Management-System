#pragma once

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/Patient.h"

#include <string>
#include <vector>

namespace hms {

class FileManager {
public:
  explicit FileManager(std::string patientFilePath = "data/patients.csv",
                       std::string doctorFilePath = "data/doctors.csv",
                       std::string appointmentFilePath = "data/appointments.csv");

  std::vector<Patient> loadPatients() const;
  bool patientIdExists(const std::string &patientId) const;
  bool savePatient(const Patient &patient, std::string &errorMessage) const;

  std::vector<Doctor> loadDoctors() const;
  bool doctorIdExists(const std::string &doctorId) const;
  bool saveDoctor(const Doctor &doctor, std::string &errorMessage) const;

  std::vector<Appointment> loadAppointments() const;
  bool appointmentIdExists(const std::string &appointmentId) const;
  bool saveAppointment(const Appointment &appointment, std::string &errorMessage) const;
  bool saveAppointments(const std::vector<Appointment> &appointments, std::string &errorMessage) const;

  const std::string &getPatientFilePath() const;
  const std::string &getDoctorFilePath() const;
  const std::string &getAppointmentFilePath() const;

private:
  std::string patientFilePath_;
  std::string doctorFilePath_;
  std::string appointmentFilePath_;
};

} // namespace hms
