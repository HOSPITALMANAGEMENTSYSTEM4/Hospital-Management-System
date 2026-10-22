
#include "hms/HospitalSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
    if (condition) {
        std::cout << "[PASS] " << message << '\n';
    } else {
        std::cout << "[FAIL] " << message << '\n';
        ++failures;
    }
}

bool createFile(const fs::path &path) {
    std::ofstream file(path);
    return file.good();
}

std::string readFile(const fs::path &path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

bool prepareDirectory(const fs::path &dataDir) {
    std::error_code ec;
    fs::remove_all(dataDir, ec);

    if (ec) {
        return false;
    }

    fs::create_directories(dataDir, ec);
    return !ec;
}

// Existing startup tests: missing CSV files must not prevent startup.
void testMissingFiles(const std::string &scenario,
                      bool omitPatients,
                      bool omitDoctors,
                      bool omitAppointments) {
    const fs::path dataDir =
        fs::path("build") / ("TC-PER-02-" + scenario);

    if (!prepareDirectory(dataDir)) {
        check(false, scenario + ": temporary directory created");
        return;
    }

    bool setupOk = true;

    if (!omitPatients) {
        setupOk = createFile(dataDir / "patients.csv") && setupOk;
    }
    if (!omitDoctors) {
        setupOk = createFile(dataDir / "doctors.csv") && setupOk;
    }
    if (!omitAppointments) {
        setupOk = createFile(dataDir / "appointments.csv") && setupOk;
    }

    if (!setupOk) {
        check(false, scenario + ": test files prepared");
        std::error_code ec;
        fs::remove_all(dataDir, ec);
        return;
    }

    hms::HospitalSystem system(dataDir.string());

    std::istringstream input("0\n");
    std::ostringstream output;

    auto *oldInput = std::cin.rdbuf(input.rdbuf());
    auto *oldOutput = std::cout.rdbuf(output.rdbuf());

    system.displayStartupRecords();
    system.run();

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    const std::string displayed = output.str();

    check(displayed.find("=== Hospital Management System ===")
              != std::string::npos,
          scenario + ": main menu appears");

    check(displayed.find("Enter your choice:")
              != std::string::npos,
          scenario + ": menu accepts input");

    check(displayed.find("Exiting Hospital Management System.")
              != std::string::npos,
          scenario + ": application exits normally");

    check(displayed.find("Records Loaded at Startup")
              != std::string::npos,
          scenario + ": startup records section appears");

    std::error_code ec;
    fs::remove_all(dataDir, ec);
}

// New test: registering a patient creates a missing patients.csv.
void testPatientRegistrationCreatesMissingCsv() {
    const fs::path dataDir =
        fs::path("build") / "TC-PER-02-CreatePatient";

    if (!prepareDirectory(dataDir)) {
        check(false, "Patient registration: temporary directory created");
        return;
    }

    // Leave patients.csv missing; create the other CSV files.
    const bool setupOk =
        createFile(dataDir / "doctors.csv") &&
        createFile(dataDir / "appointments.csv");

    check(setupOk, "Patient registration: setup succeeds");

    if (!setupOk) {
        std::error_code ec;
        fs::remove_all(dataDir, ec);
        return;
    }

    hms::HospitalSystem system(dataDir.string());

    std::istringstream input(
        "1\n"
        "P900\n"
        "Alice Patient\n"
        "30\n"
        "F\n"
        "9876543210\n"
        "123 Test Street\n"
        "None\n"
        "0\n");

    std::ostringstream output;

    auto *oldInput = std::cin.rdbuf(input.rdbuf());
    auto *oldOutput = std::cout.rdbuf(output.rdbuf());

    system.run();

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    const fs::path patientFile = dataDir / "patients.csv";
    const std::string displayed = output.str();

    check(displayed.find("Patient registered successfully.")
              != std::string::npos,
          "Patient registration: operation succeeds");

    check(fs::exists(patientFile),
          "Patient registration: missing patients.csv is created");

    if (fs::exists(patientFile)) {
        const std::string contents = readFile(patientFile);

        check(contents.find(
                  "patient_id,name,age,gender,contact_number,address,medical_history")
                  != std::string::npos,
              "Patient registration: CSV header is written");

        check(contents.find("P900,Alice Patient,30,F,9876543210")
                  != std::string::npos,
              "Patient registration: patient record is persisted");
    }

    std::error_code ec;
    fs::remove_all(dataDir, ec);
}

// New test: adding a doctor creates a missing doctors.csv.
void testDoctorAdditionCreatesMissingCsv() {
    const fs::path dataDir =
        fs::path("build") / "TC-PER-02-CreateDoctor";

    if (!prepareDirectory(dataDir)) {
        check(false, "Doctor addition: temporary directory created");
        return;
    }

    // Leave doctors.csv missing; create the other CSV files.
    const bool setupOk =
        createFile(dataDir / "patients.csv") &&
        createFile(dataDir / "appointments.csv");

    check(setupOk, "Doctor addition: setup succeeds");

    if (!setupOk) {
        std::error_code ec;
        fs::remove_all(dataDir, ec);
        return;
    }

    hms::HospitalSystem system(dataDir.string());

    std::istringstream input(
        "D900\n"
        "Dr Test\n"
        "Cardiology\n"
        "Cardiology Dept\n"
        "9876543211\n"
        "Mon-Fri 09:00-17:00\n");

    std::ostringstream output;

    auto *oldInput = std::cin.rdbuf(input.rdbuf());
    auto *oldOutput = std::cout.rdbuf(output.rdbuf());

    system.addDoctor();

    std::cin.rdbuf(oldInput);
    std::cout.rdbuf(oldOutput);

    const fs::path doctorFile = dataDir / "doctors.csv";
    const std::string displayed = output.str();

    check(displayed.find("Doctor added successfully.")
              != std::string::npos,
          "Doctor addition: operation succeeds");

    check(fs::exists(doctorFile),
          "Doctor addition: missing doctors.csv is created");

    if (fs::exists(doctorFile)) {
        const std::string contents = readFile(doctorFile);

        check(contents.find(
                  "doctor_id,name,specialization,department,contact_number,availability")
                  != std::string::npos,
              "Doctor addition: CSV header is written");

        check(contents.find("D900,Dr Test,Cardiology")
                  != std::string::npos,
              "Doctor addition: doctor record is persisted");
    }

    std::error_code ec;
    fs::remove_all(dataDir, ec);
}

// New test: booking an appointment creates a missing appointments.csv.
void testAppointmentBookingCreatesMissingCsv() {
    const fs::path dataDir =
        fs::path("build") / "TC-PER-02-CreateAppointment";

    if (!prepareDirectory(dataDir)) {
        check(false, "Appointment booking: temporary directory created");
        return;
    }

    const fs::path patientFile = dataDir / "patients.csv";
    const fs::path doctorFile = dataDir / "doctors.csv";
    const fs::path appointmentFile = dataDir / "appointments.csv";

    // Create valid patient and doctor records first.
    // Deliberately leave appointments.csv missing.
    hms::FileManager fileManager(
        patientFile.string(),
        doctorFile.string(),
        appointmentFile.string());

    std::string errorMessage;

    const bool patientSaved = fileManager.savePatient(
        hms::Patient("P901", "Booking Patient", 35, "M",
                     "9876543212", "456 Test Street", "None"),
        errorMessage);

    check(patientSaved,
          "Appointment booking: test patient is saved");

    errorMessage.clear();

    const bool doctorSaved = fileManager.saveDoctor(
        hms::Doctor("D901", "Dr Booking", "Cardiology",
                    "Cardiology Dept", "9876543213",
                    "Mon-Fri 09:00-17:00"),
        errorMessage);

    check(doctorSaved,
          "Appointment booking: test doctor is saved");

    check(!fs::exists(appointmentFile),
          "Appointment booking: appointments.csv is initially missing");

    if (!patientSaved || !doctorSaved) {
        std::error_code ec;
        fs::remove_all(dataDir, ec);
        return;
    }

    hms::HospitalSystem system(fileManager);

    std::string appointmentId;
    errorMessage.clear();

    const bool booked = system.bookAppointment(
        "P901", "D901", "2026-10-15", "10:00",
        appointmentId, errorMessage);

    check(booked,
          "Appointment booking: operation succeeds");

    check(fs::exists(appointmentFile),
          "Appointment booking: missing appointments.csv is created");

    if (booked && fs::exists(appointmentFile)) {
        const std::string contents = readFile(appointmentFile);

        check(contents.find(
                  "AppointmentID,PatientID,DoctorID,Date,Time,Status")
                  != std::string::npos,
              "Appointment booking: CSV header is written");

        check(contents.find(
                  appointmentId + ",P901,D901,2026-10-15,10:00")
                  != std::string::npos,
              "Appointment booking: appointment record is persisted");
    } else if (!booked) {
        std::cout << "[INFO] Appointment booking error: "
                  << errorMessage << '\n';
    }

    std::error_code ec;
    fs::remove_all(dataDir, ec);
}

} // namespace

int main() {
    // Original four startup scenarios.
    testMissingFiles("AllMissing", true, true, true);
    testMissingFiles("PatientsMissing", true, false, false);
    testMissingFiles("DoctorsMissing", false, true, false);
    testMissingFiles("AppointmentsMissing", false, false, true);

    // Additional acceptance-criterion coverage.
    testPatientRegistrationCreatesMissingCsv();
    testDoctorAdditionCreatesMissingCsv();
    testAppointmentBookingCreatesMissingCsv();

    if (failures != 0) {
        std::cerr << failures << " assertion(s) failed.\n";
        return 1;
    }

    std::cout << "All Issue #16 missing-CSV scenarios passed.\n";
    return 0;
}
