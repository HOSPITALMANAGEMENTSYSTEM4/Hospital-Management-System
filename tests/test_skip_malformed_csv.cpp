#include "hms/FileManager.h"
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
    if (condition) {
        std::cout << "[PASS] " << message << '\n';
    } else {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
}

void testSkipMalformedPatientRecords() {
    const std::string path = "build/test_malformed_patients.csv";
    const std::string doctorPath = "build/test_malformed_doctors.csv";

    std::filesystem::create_directories("build");

    {
        std::ofstream csv(path);
        csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";

        const std::vector<std::string> names = {
            "Anita", "Rahul", "Priya", "Arun", "Meera",
            "Kiran", "Divya", "Ravi", "Neha", "Suman"
        };

        for (int i = 1; i <= 10; ++i) {
            csv << "P" << i << "," << names[i - 1]
                << ",25,M,9876543210,City,None\n";
        }

        // Malformed: unterminated quoted field.
        csv << "P11,\"Broken name,25,M,9876543211,City,None\n";

        // Malformed: wrong number of columns.
        csv << "P12,Too few columns,25,M\n";

        // Malformed: invalid age.
        csv << "P13,Invalid age,not-a-number,M,9876543213,City,None\n";
    }

    hms::FileManager fileManager(path, doctorPath);
    std::size_t malformedCount = 0;
    const auto patients = fileManager.loadPatients(&malformedCount);

    check(patients.size() == 10,
          "Ten valid patient records are loaded");

    check(malformedCount == 3,
          "Three malformed patient records are counted");

    // Select Patient Management, View All, Return, then Exit.
    hms::HospitalSystem system(fileManager);
    std::istringstream input("1\n3\n0\n4\n");
    std::ostringstream output;

    auto *oldInput = std::cin.rdbuf(input.rdbuf());
    auto *oldOutput = std::cout.rdbuf(output.rdbuf());

    try {
        system.run();
    } catch (...) {
        std::cin.rdbuf(oldInput);
        std::cout.rdbuf(oldOutput);
        throw;
    }

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    const std::string displayed = output.str();

    check(displayed.find("3 malformed record(s) skipped")
              != std::string::npos,
          "The malformed-record warning is displayed");

    check(displayed.find("P10 | Suman") != std::string::npos,
          "The last valid patient is displayed");

    check(displayed.find("Exiting Hospital Management System.")
              != std::string::npos,
          "The application continues and exits normally");

    std::filesystem::remove(path);
    std::filesystem::remove(doctorPath);
}

void testSkipMalformedDoctorRecords() {
    const std::string patientPath = "build/test_malformed_patients.csv";
    const std::string doctorPath = "build/test_malformed_doctors.csv";

    std::filesystem::create_directories("build");

    {
        std::ofstream csv(doctorPath);
        csv << "doctor_id,name,specialization,department,contact_number,availability\n";
        csv << "D001,Dr Smith,Cardiology,Medicine,9876543210,Weekdays\n";
        csv << "D002,Missing Fields\n";
        csv << "D003,,,,,\n";
    }

    hms::FileManager fileManager(patientPath, doctorPath);
    std::size_t malformedCount = 0;
    const auto doctors = fileManager.loadDoctors(&malformedCount);

    check(doctors.size() == 1,
          "Only the valid doctor record is loaded");

    check(malformedCount == 2,
          "Two malformed doctor records are counted");

    std::filesystem::remove(doctorPath);
}

void testSkipMalformedAppointmentRecords() {
    const std::string patientPath = "build/test_malformed_patients.csv";
    const std::string doctorPath = "build/test_malformed_doctors.csv";
    const std::string appointmentPath = "build/test_malformed_appointments.csv";

    std::filesystem::create_directories("build");

    {
        std::ofstream csv(patientPath);
        csv << "patient_id,name,age,gender,contact_number,address,medical_history\n";
        csv << "P001,Alice,25,F,9876543210,City,None\n";
    }

    {
        std::ofstream csv(doctorPath);
        csv << "doctor_id,name,specialization,department,contact_number,availability\n";
        csv << "D001,Dr Smith,Cardiology,Medicine,9876543211,Weekdays\n";
    }

    {
        std::ofstream csv(appointmentPath);
        csv << "appointment_id,patient_id,doctor_id,date,time,status\n";
        csv << "A0001,P001,D001,2026-11-15,10:00,Booked\n";
        // Malformed: unknown appointment status.
        csv << "A002,P001,D001,2026-11-16,11:00,UnknownStatus\n";

        // Malformed: wrong number of columns.
        csv << "A003,P001,D001,2026-11-17\n";
    }

    hms::FileManager fileManager(
        patientPath, doctorPath, appointmentPath);

    std::size_t malformedCount = 0;
    const auto appointments =
        fileManager.loadAppointments(&malformedCount);

    check(appointments.size() == 1,
          "Only the valid appointment record is loaded");

    check(malformedCount == 2,
          "Two malformed appointment records are counted");

    std::filesystem::remove(patientPath);
    std::filesystem::remove(doctorPath);
    std::filesystem::remove(appointmentPath);
}

} // namespace

int main() {
    try {
        testSkipMalformedPatientRecords();
        testSkipMalformedDoctorRecords();
        testSkipMalformedAppointmentRecords();
    } catch (const std::exception &error) {
        std::cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All malformed CSV tests passed.\n";
    return 0;
}