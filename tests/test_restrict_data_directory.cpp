
#include "hms/FileManager.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
    namespace fs = std::filesystem;

    const hms::FileManager defaults;

    assert(defaults.getPatientFilePath() == "data/patients.csv");
    assert(defaults.getDoctorFilePath() == "data/doctors.csv");
    assert(defaults.getAppointmentFilePath() == "data/appointments.csv");

    std::string error;

    // Parent-directory traversal must be rejected for every data category.
    const hms::FileManager badPatient(
        "../outside-patients.csv", "data/doctors.csv",
        "data/appointments.csv");
    assert(badPatient.loadPatients().empty());
    assert(!badPatient.savePatients(std::vector<hms::Patient>{}, error));
    assert(!error.empty());

    error.clear();
    const hms::FileManager badDoctor(
        "data/patients.csv", "../outside-doctors.csv",
        "data/appointments.csv");
    assert(badDoctor.loadDoctors().empty());
    assert(!badDoctor.saveDoctors(std::vector<hms::Doctor>{}, error));
    assert(!error.empty());

    error.clear();
    const hms::FileManager badAppointment(
        "data/patients.csv", "data/doctors.csv",
        "../outside-appointments.csv");
    assert(badAppointment.loadAppointments().empty());
    assert(!badAppointment.saveAppointments(
        std::vector<hms::Appointment>{}, error));
    assert(!error.empty());

    // Absolute paths must also be rejected.
    error.clear();
    const auto absoluteOutside =
        (fs::temp_directory_path() /
         "hms_issue56_must_not_be_created.csv").string();

    const hms::FileManager badAbsolute(
        absoluteOutside, "data/doctors.csv", "data/appointments.csv");
    assert(badAbsolute.loadPatients().empty());
    assert(!badAbsolute.savePatients(std::vector<hms::Patient>{}, error));
    assert(!error.empty());

           // Verify FileManager rejects a symlinked data directory in isolation.
    {
        const fs::path tempRoot =
            fs::temp_directory_path() / "hms_issue56_symlink_test";
        const fs::path targetPath = tempRoot / "target";
        const fs::path linkPath = tempRoot / "data";

        std::error_code ec;
        fs::remove_all(tempRoot, ec);
        ec.clear();

        fs::create_directories(targetPath, ec);
        assert(!ec);

        fs::create_directory_symlink(targetPath, linkPath, ec);

        if (!ec) {
            const fs::path originalCwd = fs::current_path();

            fs::current_path(tempRoot, ec);
            assert(!ec);

            hms::FileManager symlinkedDataManager;
            std::string symlinkError;

            // Loading must fail closed when the data directory is a symlink.
            assert(symlinkedDataManager.loadPatients().empty());

            // Saving must also be rejected.
            const bool saveSucceeded =
                symlinkedDataManager.savePatients(
                    std::vector<hms::Patient>{}, symlinkError);

            assert(!saveSucceeded);
            assert(!symlinkError.empty());

            fs::current_path(originalCwd, ec);
            assert(!ec);
        } else {
            std::cerr
                << "WARNING: Symlink creation unavailable; "
                << "symlink rejection was not tested.\n";
        }

        ec.clear();
        fs::remove_all(tempRoot, ec);
        assert(!ec);
    }
    std::cout << "Data-directory restrictions verified.\n";
    return 0;
}
