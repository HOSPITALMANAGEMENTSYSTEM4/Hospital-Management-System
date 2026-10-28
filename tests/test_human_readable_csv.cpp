#include "hms/Appointment.h"
#include "hms/Doctor.h"
#include "hms/FileManager.h"
#include "hms/Patient.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    namespace fs = std::filesystem;

    const fs::path testDir = fs::path("build") / "human-readable-csv";
    fs::create_directories(testDir);

    const fs::path patientsFile = testDir / "patients.csv";
    const fs::path doctorsFile = testDir / "doctors.csv";
    const fs::path appointmentsFile = testDir / "appointments.csv";

    hms::FileManager fm(
        patientsFile.string(),
        doctorsFile.string(),
        appointmentsFile.string()
    );

    std::string error;
    int failures = 0;

    const hms::Patient patient(
        "P001", "John Doe", 35, "M", "9876543210",
        "123 Main St", "None"
    );

    const hms::Doctor doctor(
        "D001", "Dr. Alice Adams", "Cardiology",
        "Cardiology", "9123456780", "Mon-Fri 09:00-17:00"
    );

    const hms::Appointment appointment(
        "A0001", "P001", "D001", "2026-11-10", "09:00"
    );

    if (!fm.savePatient(patient, error)) {
        std::cerr << "FAIL: Save patient: " << error << '\n';
        ++failures;
    }

    if (!fm.saveDoctor(doctor, error)) {
        std::cerr << "FAIL: Save doctor: " << error << '\n';
        ++failures;
    }

    if (!fm.saveAppointments({appointment}, error)) {
        std::cerr << "FAIL: Save appointment: " << error << '\n';
        ++failures;
    }

    const std::vector<std::pair<fs::path, std::string>> expected = {
        {patientsFile, "patient_id,name,age,gender,contact_number,address,medical_history"},
        {doctorsFile, "doctor_id,name,specialization,department,contact_number,availability"},
        {appointmentsFile, "AppointmentID,PatientID,DoctorID,Date,Time,Status"}
    };

    for (const auto& [path, header] : expected) {
        std::ifstream input(path);

        std::string firstLine;
        if (!std::getline(input, firstLine) || firstLine != header) {
            std::cerr << "FAIL: Incorrect or missing CSV header in "
                      << path.filename().string() << '\n';
            ++failures;
            continue;
        }

        std::string line;
        int recordCount = 0;

        while (std::getline(input, line)) {
            if (!line.empty()) {
                ++recordCount;
            }
        }

        if (recordCount != 1) {
            std::cerr << "FAIL: Expected one record in "
                      << path.filename().string()
                      << ", got " << recordCount << '\n';
            ++failures;
        }
    }

    fs::remove_all(testDir);

    if (failures == 0) {
        std::cout << "PASS: CSV headers and one-record-per-line format verified.\n";
        return 0;
    }

    std::cerr << failures << " CSV format check(s) failed.\n";
    return 1;
}