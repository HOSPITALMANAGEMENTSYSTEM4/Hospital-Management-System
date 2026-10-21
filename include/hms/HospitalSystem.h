
#pragma once

#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/Patient.h"

#include <string>
#include <vector>

namespace hms {

// Outcome of modifying operations (ADR-05)
struct OpResult {
  bool success{false};
  std::string message;
};

class HospitalSystem {
public:
  HospitalSystem() = default;
  explicit HospitalSystem(FileManager fileManager);
  explicit HospitalSystem(const std::string &dataDir);

  void run();
  void addDoctor();

  // Interactive booking and view workflows
  void bookAppointment();
  void viewAppointments();

  // Programmatic API for Appointment booking (HMS-F-015)
  // Validates Patient ID, Doctor ID, date, time, checks availability,
  // generates sequential Appointment ID, and persists record with status Booked.
  OpResult bookAppointment(const std::string &patientId,
                           const std::string &doctorId,
                           const std::string &date,
                           const std::string &time,
                           std::string &outAppointmentId);

  bool bookAppointment(const std::string &patientId,
                       const std::string &doctorId,
                       const std::string &date,
                       const std::string &time,
                       std::string &outAppointmentId,
                       std::string &errorMessage);

  // Availability check (BR-05, HMS-F-020)
  bool checkDoctorAvailability(const std::string &doctorId,
                               const std::string &date,
                               const std::string &time,
                               std::string &message) const;

  // Appointment listing (HMS-F-022)
  // filterType: "ALL", "PATIENT", "DOCTOR"
  std::vector<Appointment> listAppointments(
      const std::string &filterType = "ALL",
      const std::string &filterId = "") const;

  // Cancellation (BR-06, HMS-F-023)
  OpResult cancelAppointment(const std::string &appointmentId,
                             bool confirmed = true);

  // Lookup appointment by ID
  const Appointment *findAppointment(const std::string &appointmentId) const;

  // Generates next unique sequential Appointment ID (VR-12, HMS-F-015)
  std::string nextAppointmentId() const;

  const FileManager &getFileManager() const;
  FileManager &getFileManager();

private:
  void registerPatient();
  void searchPatients();
  void searchDoctors();
  void viewPatientDetails();
  void listPatients();

  FileManager fileManager_;
  mutable std::vector<Appointment> appointmentsCache_;
};

} // namespace hms
