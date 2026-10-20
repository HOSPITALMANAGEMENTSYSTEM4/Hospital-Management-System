
#include "hms/FileManager.h"
#include "hms/Patient.h"
#include "hms/Doctor.h"
#include "hms/Appointment.h"

#include <filesystem>
#include <iostream>
#include <string>

int main() {
    namespace fs = std::filesystem;

    const fs::path dir = "build/test_missing_csv_data";

    fs::remove_all(dir);
    fs::create_directories(dir);

    const std::string patients = (dir / "patients.csv").string();
    const std::string doctors = (dir / "doctors.csv").string();
    const std::string appointments = (dir / "appointments.csv").string();

    hms::FileManager fm(patients, doctors, appointments);
    int failures = 0;

    auto check = [&](bool condition, const std::string& message) {
        std::cout << (condition ? "PASS: " : "FAIL: ")
                  << message << '\n';

        if (!condition) {
            ++failures;
        }
    };

    // Missing CSV files should load as empty collections.
    check(fm.loadPatients().empty(),
          "Missing patient CSV loads empty");

    check(fm.loadDoctors().empty(),
          "Missing doctor CSV loads empty");

    check(fm.loadAppointments().empty(),
          "Missing appointment CSV loads empty");

    // Saving a valid patient should create the missing CSV.
    std::string error;

    hms::Patient patient(
        "P9001", "Test Patient", 30, "F",
        "9876543210", "Test Address", "None"
    );

    const bool patientSaved = fm.savePatient(patient, error);

    if (!patientSaved) {
        std::cout << "Patient save error: " << error << '\n';
    }

    check(patientSaved,
          "Patient can be saved when CSV is missing");

    check(fs::exists(patients),
          "Patient CSV is created");

    // Saving a valid doctor should create the missing CSV.
    error.clear();

    hms::Doctor doctor(
        "D9001", "Dr. Test", "General", "General",
        "9876543211", "Monday-Friday 9 AM-5 PM"
    );

    const bool doctorSaved = fm.saveDoctor(doctor, error);

    if (!doctorSaved) {
        std::cout << "Doctor save error: " << error << '\n';
    }

    check(doctorSaved,
          "Doctor can be saved when CSV is missing");

    check(fs::exists(doctors),
          "Doctor CSV is created");

    // Saving an appointment should create the missing CSV.
    error.clear();

    hms::Appointment appointment(
        "A9001", "P9001", "D9001",
        "2026-11-01", "10:00",
        hms::AppointmentStatus::Booked
    );

    const bool appointmentSaved =
        fm.saveAppointment(appointment, error);

    if (!appointmentSaved) {
        std::cout << "Appointment save error: " << error << '\n';
    }

    check(appointmentSaved,
          "Appointment can be saved when CSV is missing");

    check(fs::exists(appointments),
          "Appointment CSV is created");

    fs::remove_all(dir);

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All missing-CSV tests passed.\n";
    return 0;
}
