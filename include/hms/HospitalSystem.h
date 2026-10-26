
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

  void displayStartupRecords();
  void run();
  void addDoctor();
  void viewDoctorDetails();
  void listDoctors();

  void bookAppointment();
  void viewAppointments(const std::string &filterType = "ALL",
                        const std::string &filterId = "");
  void updatePatient();

  // Programmatic API for updating patient details (HMS-F-006, HMS-F-007)
  OpResult updatePatient(const std::string &patientId,
                         const std::string &name,
                         int age,
                         const std::string &gender,
                         const std::string &contactNumber,
                         const std::string &address,
                         const std::string &medicalHistory = "");

  OpResult updatePatient(const Patient &patient);

  bool updatePatient(const std::string &patientId,
                     const std::string &name,
                     int age,
                     const std::string &gender,
                     const std::string &contactNumber,
                     const std::string &address,
                     const std::string &medicalHistory,
                     std::string &errorMessage);

  // Lookup patient by ID
  const Patient *findPatient(const std::string &patientId) const;

  void updateDoctor();

  // Programmatic API for updating doctor details (HMS-F-013, HMS-F-014)
  OpResult updateDoctor(const std::string &doctorId,
                        const std::string &name,
                        const std::string &specialization,
                        const std::string &department,
                        const std::string &contactNumber,
                        const std::string &availability);

  OpResult updateDoctor(const Doctor &doctor);

  bool updateDoctor(const std::string &doctorId,
                    const std::string &name,
                    const std::string &specialization,
                    const std::string &department,
                    const std::string &contactNumber,
                    const std::string &availability,
                    std::string &errorMessage);

  // Lookup doctor by ID
  const Doctor *findDoctor(const std::string &doctorId) const;

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
      const std::string &filterId = "",
      std::size_t *malformedCount = nullptr) const;

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

  // HMS-F-032: submenu loops (SRS §3.1)
  void patientMenu();
  void doctorMenu();
  void appointmentMenu();

  // Interactive wrappers for cancellation, availability check, and viewing
  void viewAppointmentsInteractive();
  void cancelAppointmentInteractive();
  void checkDoctorAvailabilityInteractive();

  FileManager fileManager_;
  mutable std::vector<Appointment> appointmentsCache_;
  mutable std::vector<Patient> patientsCache_;
  mutable std::vector<Doctor> doctorsCache_;
};

} // namespace hms
