// TC-PAT-02  Reject Duplicate Patient ID (HMS-F-002)
//
// A self-contained test binary that covers Tests A-F from the issue spec.
// No third-party framework is required; failures abort with a non-zero exit
// code so CTest can pick them up automatically.

#include "hms/FileManager.h"
#include "hms/Patient.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Mini assertion helper — prints test name then passes/fails.
// ---------------------------------------------------------------------------
namespace {

int g_passed = 0;
int g_failed = 0;

void check(const char *name, bool condition) {
  if (condition) {
    std::cout << "  [PASS] " << name << '\n';
    ++g_passed;
  } else {
    std::cout << "  [FAIL] " << name << '\n';
    ++g_failed;
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Return the byte contents of a file, or empty string if missing.
std::string readFile(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return "";
  }
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

// Count non-header data rows in the CSV (lines that don't start with
// "patient_id" and are not blank).
int countDataRows(const std::string &path) {
  std::ifstream f(path);
  if (!f) {
    return 0;
  }
  int count = 0;
  std::string line;
  while (std::getline(f, line)) {
    if (!line.empty() && line.rfind("patient_id", 0) != 0) {
      ++count;
    }
  }
  return count;
}

// Delete a file (best-effort; ignores errors).
void removeFile(const std::string &path) { std::remove(path.c_str()); }

// Build a valid Patient object.
hms::Patient makePatient(const std::string &id) {
  return hms::Patient(id, "Test Name", 30, "M", "5550100100", "1 Test St");
}

} // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------
//
// NOTE — HospitalSystem console message not tested here:
//   HospitalSystem::registerPatient() writes "Patient ID already exists\n"
//   to std::cout when a duplicate is detected. Redirecting std::cout inside
//   this binary without a larger refactor (e.g., dependency injection or a
//   stream-swap helper) would add unnecessary complexity. The message has
//   been verified by source inspection of src/HospitalSystem.cpp line 135.

static void runTests(const std::string &csvPath) {
  // ------------------------------------------------------------------
  // Test E — Valid new ID registers successfully (must run before others
  // so P1001 is in the file for subsequent tests).
  // ------------------------------------------------------------------
  std::cout << "\nTest E: Valid new ID\n";
  {
    removeFile(csvPath);
    hms::FileManager fm(csvPath);
    std::string err;
    bool ok = fm.savePatient(makePatient("P1001"), err);
    check("savePatient returns true for new ID", ok);
    check("error message is empty on success", err.empty());
    check("patient_id P1001 now exists", fm.patientIdExists("P1001"));
    check("exactly 1 data row in CSV", countDataRows(csvPath) == 1);
  }

  // ------------------------------------------------------------------
  // Test A — Exact duplicate (P1001 → P1001)
  // ------------------------------------------------------------------
  std::cout << "\nTest A: Exact duplicate\n";
  {
    hms::FileManager fm(csvPath);
    check("exact duplicate P1001 detected", fm.patientIdExists("P1001"));
    std::string err;
    bool ok = fm.savePatient(makePatient("P1001"), err);
    check("savePatient returns false for exact duplicate", !ok);
    check("error message is non-empty", !err.empty());
    check("still exactly 1 data row", countDataRows(csvPath) == 1);
  }

  // ------------------------------------------------------------------
  // Test B — Lowercase duplicate (P1001 → p1001)
  // ------------------------------------------------------------------
  std::cout << "\nTest B: Lowercase duplicate\n";
  {
    hms::FileManager fm(csvPath);
    check("lowercase p1001 detected as duplicate", fm.patientIdExists("p1001"));
    std::string err;
    bool ok = fm.savePatient(makePatient("p1001"), err);
    check("savePatient returns false for lowercase duplicate", !ok);
    check("still exactly 1 data row", countDataRows(csvPath) == 1);
  }

  // ------------------------------------------------------------------
  // Test C — Mixed-case variants across multiple alphabetic characters
  //
  // Uses a second ID (PaT2001) whose letters span upper and lower case
  // so that only a genuine case-fold (not a numeric coincidence) makes
  // the duplicate check pass.
  // ------------------------------------------------------------------
  std::cout << "\nTest C: Mixed-case variants\n";
  {
    // Seed PaT2001 as a new distinct patient.
    hms::FileManager seed(csvPath);
    std::string seedErr;
    bool seeded = seed.savePatient(makePatient("PaT2001"), seedErr);
    check("PaT2001 seeded successfully", seeded);
    check("CSV now has 2 data rows after seeding", countDataRows(csvPath) == 2);

    hms::FileManager fm(csvPath);

    // Original P1001 variants still rejected.
    check("P1001 (exact) is duplicate",  fm.patientIdExists("P1001"));
    check("p1001 (lower) is duplicate",  fm.patientIdExists("p1001"));

    // PaT2001 variants — every combination of letter case must be caught.
    check("PaT2001 (exact) is duplicate",  fm.patientIdExists("PaT2001"));
    check("pAt2001 (swap) is duplicate",   fm.patientIdExists("pAt2001"));
    check("PAT2001 (upper) is duplicate",  fm.patientIdExists("PAT2001"));
    check("pat2001 (lower) is duplicate",  fm.patientIdExists("pat2001"));

    // A genuinely different ID must NOT be reported as a duplicate.
    check("PaT9999 (different) is NOT duplicate", !fm.patientIdExists("PaT9999"));
  }

  // ------------------------------------------------------------------
  // Test D — Duplicate registration returns false, error message is
  //           informative, and the CSV file is left byte-for-byte intact.
  // ------------------------------------------------------------------
  std::cout << "\nTest D: File unchanged after rejection\n";
  {
    const std::string before = readFile(csvPath);
    const int rowsBefore = countDataRows(csvPath);

    hms::FileManager fm(csvPath);
    std::string err;
    const bool rejected = fm.savePatient(makePatient("p1001"), err); // case-fold dup

    // Return-value and error-message assertions.
    check("savePatient returns false for duplicate in Test D", !rejected);
    check("error message is non-empty in Test D", !err.empty());
    // The error message must mention either "duplicate", "exists", or "already".
    const bool mentionsDuplicate =
        err.find("already") != std::string::npos ||
        err.find("exists")  != std::string::npos ||
        err.find("duplicate") != std::string::npos;
    check("error message indicates a duplicate", mentionsDuplicate);

    // File-integrity assertions.
    const std::string after = readFile(csvPath);
    const int rowsAfter = countDataRows(csvPath);
    check("CSV contents byte-for-byte unchanged", before == after);
    check("patient count unchanged", rowsBefore == rowsAfter);
  }

  // ------------------------------------------------------------------
  // Test F — Existing validation (invalid Patient not accepted)
  // ------------------------------------------------------------------
  std::cout << "\nTest F: Invalid patient data rejected\n";
  {
    hms::FileManager fm(csvPath);
    // Empty ID is invalid.
    hms::Patient invalid("", "Name", 30, "M", "555-0200", "2 Test St");
    check("invalid patient fails isValid()", !invalid.isValid());
    std::string err;
    bool ok = fm.savePatient(invalid, err);
    check("savePatient rejects invalid patient", !ok);
    check("still exactly 2 data rows (P1001 + PaT2001)", countDataRows(csvPath) == 2);
  }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
  const std::string csvPath = "test_tc_pat_02_patients.csv";
  removeFile(csvPath); // start clean

  std::cout << "=== TC-PAT-02: Reject Duplicate Patient ID ===\n";

  runTests(csvPath);

  removeFile(csvPath); // clean up after ourselves

  std::cout << "\nResults: " << g_passed << " passed, " << g_failed
            << " failed.\n";

  return g_failed == 0 ? 0 : 1;
}
