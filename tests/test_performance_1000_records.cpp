
#include "hms/HospitalSystem.h"
#include "hms/FileManager.h"
#include "hms/Patient.h"
#include "hms/Doctor.h"
#include "hms/Appointment.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace hms;

using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

static constexpr int RECORDS = 1000;
static constexpr double OPERATION_LIMIT_MS = 2000.0;
static constexpr double STARTUP_LIMIT_MS = 3000.0;
static const std::string DIR = "build/performance_test";

template <typename Function>
double measureMs(Function operation) {
    const auto start = Clock::now();
    operation();
    const auto end = Clock::now();

    return std::chrono::duration<double, std::milli>(
        end - start
    ).count();
}

static void fail(const std::string& message) {
    std::cerr << "ERROR: " << message << '\n';
    std::exit(1);
}

static std::string patientId(int i) {
    std::string number = std::to_string(i);
    return "P" + std::string(
        number.length() < 3 ? 3 - number.length() : 0, '0'
    ) + number;
}

static std::string doctorId(int i) {
    std::string number = std::to_string(i);
    return "D" + std::string(
        number.length() < 3 ? 3 - number.length() : 0, '0'
    ) + number;
}

static std::string appointmentId(int i) {
    std::string number = std::to_string(i);
    return "A" + std::string(
        number.length() < 4 ? 4 - number.length() : 0, '0'
    ) + number;
}

int main() {
    std::error_code ec;
    fs::create_directories(DIR, ec);

    if (ec) {
        fail("Could not create benchmark directory: " + ec.message());
    }

    const std::string patientsPath = DIR + "/patients.csv";
    const std::string doctorsPath = DIR + "/doctors.csv";
    const std::string appointmentsPath = DIR + "/appointments.csv";

    std::remove(patientsPath.c_str());
    std::remove(doctorsPath.c_str());
    std::remove(appointmentsPath.c_str());

    FileManager files(
        patientsPath,
        doctorsPath,
        appointmentsPath
    );

    std::vector<Patient> patients;
    std::vector<Doctor> doctors;
    std::vector<Appointment> appointments;

    patients.reserve(RECORDS);
    doctors.reserve(RECORDS);
    appointments.reserve(RECORDS);

    for (int i = 1; i <= RECORDS; ++i) {
        const std::string pId = patientId(i);
        const std::string dId = doctorId(i);
        const std::string aId = appointmentId(i);

        patients.emplace_back(
            pId, "Test Patient", 30, "M", "9876543210",
            "Address " + std::to_string(i), "None"
        );

        doctors.emplace_back(
            dId, "Test Doctor", "General", "Medicine",
            "9876543210", "Available"
        );

        appointments.emplace_back(
            aId, pId, dId, "2030-01-01", "09:00",
            AppointmentStatus::Booked
        );
    }

    std::string error;

    // Save operations.
    const double savePatientsMs = measureMs([&] {
        if (!files.savePatients(patients, error)) {
            fail("Saving patients failed: " + error);
        }
    });

    const double saveDoctorsMs = measureMs([&] {
        if (!files.saveDoctors(doctors, error)) {
            fail("Saving doctors failed: " + error);
        }
    });

    const double saveAppointmentsMs = measureMs([&] {
        if (!files.saveAppointments(appointments, error)) {
            fail("Saving appointments failed: " + error);
        }
    });

    // Load operations.
    const double loadPatientsMs = measureMs([&] {
        if (files.loadPatients().size() != RECORDS) {
            fail("Patient record count mismatch");
        }
    });

    const double loadDoctorsMs = measureMs([&] {
        if (files.loadDoctors().size() != RECORDS) {
            fail("Doctor record count mismatch");
        }
    });

    const double loadAppointmentsMs = measureMs([&] {
        if (files.loadAppointments().size() != RECORDS) {
            fail("Appointment record count mismatch");
        }
    });

    HospitalSystem system(files);

    // Measure startup record loading and display without console I/O
    // dominating the timing. This is not the full process launch time.
    const double startupMs = measureMs([&] {
        std::ostringstream suppressedOutput;
        std::streambuf* originalBuffer =
            std::cout.rdbuf(suppressedOutput.rdbuf());

        system.displayStartupRecords();

        std::cout.rdbuf(originalBuffer);
    });

    // Patient lookup.
    const double patientLookupMs = measureMs([&] {
        if (system.findPatient("P500") == nullptr) {
            fail("Could not find patient P500");
        }
    });

    // Doctor lookup.
    const double doctorLookupMs = measureMs([&] {
        if (system.findDoctor("D500") == nullptr) {
            fail("Could not find doctor D500");
        }
    });

    // List all appointments.
    const double listingMs = measureMs([&] {
        if (system.listAppointments().size() != RECORDS) {
            fail("Appointment listing count mismatch");
        }
    });

    // Filter appointments for one patient.
    const double filteringMs = measureMs([&] {
        if (system.listAppointments("PATIENT", "P500").size() != 1) {
            fail("Appointment filter count mismatch");
        }
    });

    // Book an appointment on a different date and time from the
    // seeded appointments, so it does not conflict with existing data.
    std::string newAppointmentId;

    const double bookingMs = measureMs([&] {
        const OpResult result = system.bookAppointment(
            "P500", "D500", "2030-01-02", "10:00",
            newAppointmentId
        );

        if (!result.success) {
            fail("Booking failed: " + result.message);
        }

        if (newAppointmentId.empty()) {
            fail("Booking succeeded without returning an appointment ID");
        }
    });

    // Cancel the appointment created above.
    const double cancellationMs = measureMs([&] {
        const OpResult result =
            system.cancelAppointment(newAppointmentId, true);

        if (!result.success) {
            fail("Cancellation failed: " + result.message);
        }
    });

    // Print results.
    std::cout << std::fixed << std::setprecision(3);

    std::cout << "\n========================================\n";
    std::cout << " Performance Benchmark: 1,000 Records\n";
    std::cout << "========================================\n";

    std::cout << "\n--- Save Operations ---\n";
    std::cout << "Save 1000 patients:       "
              << savePatientsMs << " ms\n";
    std::cout << "Save 1000 doctors:        "
              << saveDoctorsMs << " ms\n";
    std::cout << "Save 1000 appointments:   "
              << saveAppointmentsMs << " ms\n";

    std::cout << "\n--- Load Operations ---\n";
    std::cout << "Load 1000 patients:       "
              << loadPatientsMs << " ms\n";
    std::cout << "Load 1000 doctors:        "
              << loadDoctorsMs << " ms\n";
    std::cout << "Load 1000 appointments:   "
              << loadAppointmentsMs << " ms\n";

    std::cout << "\n--- Startup, Search and View ---\n";
    std::cout << "Startup record display:   "
              << startupMs << " ms\n";
    std::cout << "Single patient lookup:    "
              << patientLookupMs << " ms\n";
    std::cout << "Single doctor lookup:     "
              << doctorLookupMs << " ms\n";
    std::cout << "List 1000 appointments:   "
              << listingMs << " ms\n";
    std::cout << "Filter appointments:      "
              << filteringMs << " ms\n";

    std::cout << "\n--- Booking and Cancellation ---\n";
    std::cout << "Book appointment:         "
              << bookingMs << " ms\n";
    std::cout << "Cancel appointment:       "
              << cancellationMs << " ms\n";

    std::cout << "\n--- Acceptance Checks ---\n";

    bool passed = true;

    const auto checkLimit = [&](const std::string& name,
                                double actual,
                                double limit) {
        const bool ok = actual <= limit;
        std::cout << (ok ? "PASS: " : "FAIL: ")
                  << name << " (" << actual
                  << " ms; limit " << limit << " ms)\n";

        if (!ok) {
            passed = false;
        }
    };

    checkLimit("Startup record display", startupMs, STARTUP_LIMIT_MS);
    checkLimit("Patient lookup", patientLookupMs, OPERATION_LIMIT_MS);
    checkLimit("Doctor lookup", doctorLookupMs, OPERATION_LIMIT_MS);
    checkLimit("Appointment listing", listingMs, OPERATION_LIMIT_MS);
    checkLimit("Appointment filtering", filteringMs, OPERATION_LIMIT_MS);
    checkLimit("Booking", bookingMs, OPERATION_LIMIT_MS);
    checkLimit("Cancellation", cancellationMs, OPERATION_LIMIT_MS);

    std::cout << "\nTemporary CSV files: " << DIR << '\n';

    if (!passed) {
        std::cout << "Benchmark completed, but one or more timing "
                     "limits were exceeded.\n";
        return 1;
    }

    std::cout << "All measured timing limits passed.\n";
    return 0;
}
