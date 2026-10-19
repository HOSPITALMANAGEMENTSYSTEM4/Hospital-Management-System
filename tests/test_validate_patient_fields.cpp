// TC-PAT-03  Validate Patient Fields (HMS-F-003)
//
// Covers VR-01 through VR-07 and VR-14:
//   VR-01 Patient ID  : 1-10 alphanumeric, normalised to uppercase
//   VR-02 Name        : 1-50, letters/space/./- /'
//   VR-03 Age         : 0-120 inclusive
//   VR-04 Gender      : M, F, or O (case-insensitive, normalised to uppercase)
//   VR-05 Contact     : exactly 10 digits
//   VR-06 Address     : 1-100 characters
//   VR-07 MedHistory  : optional, max 200 characters
//   VR-14 Security    : no commas, CR/LF; no leading =+-@
//
// The test binary is self-contained (no external framework).  CTest detects
// failure via the non-zero exit code returned when g_failed > 0.

#include "hms/FileManager.h"
#include "hms/Patient.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Minimal assertion framework
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
// File helpers
// ---------------------------------------------------------------------------

std::string readFile(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return "";
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

int countDataRows(const std::string &path) {
  std::ifstream f(path);
  if (!f) return 0;
  int n = 0;
  std::string line;
  while (std::getline(f, line))
    if (!line.empty() && line.rfind("patient_id", 0) != 0) ++n;
  return n;
}

void removeFile(const std::string &path) { std::remove(path.c_str()); }

// ---------------------------------------------------------------------------
// Patient factory — baseline all-valid patient; callers override one field.
// Contact is 10 digits (VR-05). Gender is uppercase (VR-04).
// ---------------------------------------------------------------------------
hms::Patient make(const std::string &id    = "P1001",
                  const std::string &name  = "Alice",
                  int                age   = 30,
                  const std::string &gender= "M",
                  const std::string &phone = "5551234567",
                  const std::string &addr  = "123 Main St",
                  const std::string &hist  = "") {
  return hms::Patient(id, name, age, gender, phone, addr, hist);
}

// Returns true when validate() returns true (valid patient).
bool valid(const hms::Patient &p) {
  std::string err;
  return p.validate(err);
}

// Returns true when validate() returns false (invalid patient).
bool invalid(const hms::Patient &p) { return !valid(p); }

// Returns true when validate() sets an error message containing `substr`.
bool errorContains(const hms::Patient &p, const std::string &substr) {
  std::string err;
  if (p.validate(err)) return false;         // should have been invalid
  return err.find(substr) != std::string::npos;
}

} // namespace

// ===========================================================================
// VR-01 — Patient ID
// ===========================================================================
static void testPatientId() {
  std::cout << "\n[VR-01] Patient ID\n";

  // Length boundaries
  check("ID len 1 accepted",  valid(make("A")));
  check("ID len 10 accepted", valid(make("ABCDEFGH12")));
  check("ID len 11 rejected", invalid(make("ABCDEFGH123")));
  check("ID empty rejected",  invalid(make("")));

  // Alphanumeric-only
  check("ID with space rejected",        invalid(make("P 1001")));
  check("ID with hyphen rejected",       invalid(make("P-1001")));
  check("ID with underscore rejected",   invalid(make("P_1001")));
  check("ID with punctuation rejected",  invalid(make("P1001!")));

  // Uppercase normalisation
  {
    hms::Patient p("abc123", "Alice", 30, "M", "5551234567", "123 Main St");
    check("ID normalised to uppercase", p.getId() == "ABC123");
    check("Normalised ID is valid",     valid(p));
  }

  // Digits-only and letters-only IDs are also valid (edge)
  check("ID all digits accepted",  valid(make("1234567890")));
  check("ID all letters accepted", valid(make("ABCDEFGHIJ")));

  // VR-14 on ID field (commas/newlines cannot appear after isalnum check, but test
  // formula-injection prefix — a leading '=' is rejected because isalnum('=') is false)
  check("ID beginning with = rejected", invalid(make("=ID1")));
}

// ===========================================================================
// VR-02 — Patient name
// ===========================================================================
static void testName() {
  std::cout << "\n[VR-02] Patient name\n";

  // Length boundaries
  check("Name len 1 accepted",  valid(make("P1", "A")));
  check("Name len 50 accepted", valid(make("P1", std::string(50, 'A'))));
  check("Name len 51 rejected", invalid(make("P1", std::string(51, 'A'))));
  check("Name empty rejected",  invalid(make("P1", "")));

  // Allowed characters
  check("Name with space accepted",     valid(make("P1", "Mary Jane")));
  check("Name with period accepted",    valid(make("P1", "Dr. Smith")));
  check("Name with hyphen accepted",    valid(make("P1", "Smith-Jones")));
  check("Name with apostrophe accepted",valid(make("P1", "O'Brien")));

  // Rejected characters
  check("Name with digit rejected",     invalid(make("P1", "Alice2")));
  check("Name with comma rejected",     invalid(make("P1", "Alice,Bob")));
  check("Name with exclamation rejected",invalid(make("P1","Alice!")));
  check("Name with at-sign rejected",   invalid(make("P1", "@Alice")));

  // VR-14 security on name
  check("Name with LF rejected",        invalid(make("P1", "Alice\nBob")));
  check("Name with CR rejected",        invalid(make("P1", "Alice\rBob")));
  check("Name beginning with = rejected",invalid(make("P1", "=Alice")));
  check("Name beginning with + rejected",invalid(make("P1", "+Alice")));
  check("Name beginning with - rejected",invalid(make("P1", "-Alice")));
  check("Name beginning with @ rejected",invalid(make("P1", "@Alice")));
}

// ===========================================================================
// VR-03 — Age
// ===========================================================================
static void testAge() {
  std::cout << "\n[VR-03] Age\n";

  check("Age 0 accepted",   valid(make("P1", "Alice", 0)));
  check("Age 1 accepted",   valid(make("P1", "Alice", 1)));
  check("Age 60 accepted",  valid(make("P1", "Alice", 60)));
  check("Age 120 accepted", valid(make("P1", "Alice", 120)));
  check("Age 121 rejected", invalid(make("P1", "Alice", 121)));
  check("Age -1 rejected",  invalid(make("P1", "Alice", -1)));
  check("Age 130 rejected", invalid(make("P1", "Alice", 130)));

  // Error message must mention the field and range
  check("Age error mentions field",
        errorContains(make("P1", "Alice", 121), "Age"));
  check("Age error mentions 0 to 120",
        errorContains(make("P1", "Alice", 121), "120"));
}

// ===========================================================================
// VR-04 — Gender
// ===========================================================================
static void testGender() {
  std::cout << "\n[VR-04] Gender\n";

  check("Gender M accepted",  valid(make("P1", "Alice", 30, "M")));
  check("Gender F accepted",  valid(make("P1", "Alice", 30, "F")));
  check("Gender O accepted",  valid(make("P1", "Alice", 30, "O")));

  // Case-insensitive input, normalised to uppercase
  {
    hms::Patient lower("P1", "Alice", 30, "m", "5551234567", "123 Main St");
    check("Gender m normalised to M",  lower.getGender() == "M");
    check("Normalised gender m valid", valid(lower));
  }
  {
    hms::Patient lf("P1", "Alice", 30, "f", "5551234567", "123 Main St");
    check("Gender f normalised to F", lf.getGender() == "F");
  }
  {
    hms::Patient lo("P1", "Alice", 30, "o", "5551234567", "123 Main St");
    check("Gender o normalised to O", lo.getGender() == "O");
  }

  // Invalid values
  check("Gender X rejected",        invalid(make("P1", "Alice", 30, "X")));
  check("Gender empty rejected",    invalid(make("P1", "Alice", 30, "")));
  check("Gender Male rejected",     invalid(make("P1", "Alice", 30, "Male")));
  check("Gender 1 rejected",        invalid(make("P1", "Alice", 30, "1")));
}

// ===========================================================================
// VR-05 — Contact number
// ===========================================================================
static void testContact() {
  std::cout << "\n[VR-05] Contact number\n";

  check("Contact 10 digits accepted", valid(make("P1","Alice",30,"M","5551234567")));
  check("Contact 9 digits rejected",  invalid(make("P1","Alice",30,"M","555123456")));
  check("Contact 11 digits rejected", invalid(make("P1","Alice",30,"M","55512345678")));
  check("Contact empty rejected",     invalid(make("P1","Alice",30,"M","")));

  check("Contact with letter rejected",    invalid(make("P1","Alice",30,"M","555123456A")));
  check("Contact with space rejected",     invalid(make("P1","Alice",30,"M","555 234567")));
  check("Contact with hyphen rejected",    invalid(make("P1","Alice",30,"M","555-234567")));
  check("Contact with plus rejected",      invalid(make("P1","Alice",30,"M","+551234567")));
  check("Contact with parens rejected",    invalid(make("P1","Alice",30,"M","(555)23456")));

  // VR-14 — commas and newlines can't fit in 10-char field, but test prefix injection
  // '+' triggers VR-14 prefix check — already covered above
}

// ===========================================================================
// VR-06 — Address
// ===========================================================================
static void testAddress() {
  std::cout << "\n[VR-06] Address\n";

  check("Address len 1 accepted",   valid(make("P1","Alice",30,"M","5551234567","X")));
  check("Address len 100 accepted", valid(make("P1","Alice",30,"M","5551234567",std::string(100,'A'))));
  check("Address len 101 rejected", invalid(make("P1","Alice",30,"M","5551234567",std::string(101,'A'))));
  check("Address empty rejected",   invalid(make("P1","Alice",30,"M","5551234567","")));

  // VR-14
  check("Address with comma rejected", invalid(make("P1","Alice",30,"M","5551234567","123,Main")));
  check("Address with LF rejected",    invalid(make("P1","Alice",30,"M","5551234567","123\nMain")));
  check("Address beginning with = rejected",
        invalid(make("P1","Alice",30,"M","5551234567","=formula")));
  check("Address beginning with @ rejected",
        invalid(make("P1","Alice",30,"M","5551234567","@bad")));
}

// ===========================================================================
// VR-07 — Medical history
// ===========================================================================
static void testMedicalHistory() {
  std::cout << "\n[VR-07] Medical history\n";

  check("MedHistory empty accepted",    valid(make("P1","Alice",30,"M","5551234567","Addr","")));
  check("MedHistory len 200 accepted",  valid(make("P1","Alice",30,"M","5551234567","Addr",std::string(200,'X'))));
  check("MedHistory len 201 rejected",  invalid(make("P1","Alice",30,"M","5551234567","Addr",std::string(201,'X'))));

  // VR-14
  check("MedHistory with comma rejected",invalid(make("P1","Alice",30,"M","5551234567","Addr","hyper,tension")));
  check("MedHistory with CR rejected",   invalid(make("P1","Alice",30,"M","5551234567","Addr","hyper\rtension")));
  check("MedHistory beginning with = rejected",
        invalid(make("P1","Alice",30,"M","5551234567","Addr","=FORMULA")));
  check("MedHistory beginning with + rejected",
        invalid(make("P1","Alice",30,"M","5551234567","Addr","+bad")));
}

// ===========================================================================
// VR-14 — Security checks (cross-field summary)
// ===========================================================================
static void testSecurity() {
  std::cout << "\n[VR-14] Security (injection prefixes / special chars)\n";

  // Formula-injection prefix in several fields
  check("ID  =prefix rejected",   invalid(make("=BAD")));
  // Name -prefix: '-' is allowed in names per VR-02 but VR-14 says reject leading '-'
  // The security check runs after the character-set check so a leading '-' is caught.
  check("Name -prefix rejected",  invalid(make("P1", "-Alice")));
  check("Addr +prefix rejected",  invalid(make("P1","Alice",30,"M","5551234567","+bad")));

  // Comma in several fields
  check("Addr comma rejected",    invalid(make("P1","Alice",30,"M","5551234567","1,2")));
  check("Hist comma rejected",    invalid(make("P1","Alice",30,"M","5551234567","A","a,b")));

  // Newlines
  check("Addr LF rejected",       invalid(make("P1","Alice",30,"M","5551234567","a\nb")));
  check("Hist CR rejected",       invalid(make("P1","Alice",30,"M","5551234567","A","a\rb")));
}

// ===========================================================================
// Persistence tests
// ===========================================================================
static void testPersistence(const std::string &csvPath) {
  std::cout << "\n[Persistence] FileManager integration\n";

  removeFile(csvPath);

  hms::FileManager fm(csvPath);

  // --- P1: Valid patient saves successfully ---
  {
    std::string err;
    bool ok = fm.savePatient(make(), err);
    check("P1: valid patient saved", ok);
    check("P1: error empty on success", err.empty());
    check("P1: 1 row in CSV", countDataRows(csvPath) == 1);
  }

  // --- P2: Invalid patient rejected ---
  {
    const std::string before = readFile(csvPath);
    hms::Patient bad("", "Alice", 30, "M", "5551234567", "123 Main St");
    std::string err;
    bool ok = fm.savePatient(bad, err);
    check("P2: invalid patient rejected", !ok);
    check("P2: error non-empty",          !err.empty());
    check("P2: CSV unchanged",            readFile(csvPath) == before);
    check("P2: row count unchanged",      countDataRows(csvPath) == 1);
  }

  // --- P3: VR-05 failure — invalid contact number not persisted ---
  {
    const std::string before = readFile(csvPath);
    hms::Patient badContact("P2","Alice",30,"M","555-1234","123 Main St");
    std::string err;
    bool ok = fm.savePatient(badContact, err);
    check("P3: bad contact rejected",    !ok);
    check("P3: error mentions contact",  err.find("Contact") != std::string::npos ||
                                         err.find("contact") != std::string::npos);
    check("P3: CSV unchanged",           readFile(csvPath) == before);
  }

  // --- P4: ID normalisation — stored as uppercase ---
  {
    std::string err;
    bool ok = fm.savePatient(make("p2"), err);   // lowercase input
    check("P4: lowercase-ID patient saved",    ok);
    check("P4: ID stored as P2 (uppercase)",   fm.patientIdExists("P2"));
    check("P4: p2 still detected as duplicate",fm.patientIdExists("p2"));
    check("P4: row count now 2",               countDataRows(csvPath) == 2);
  }

  // --- P5: Gender normalisation ---
  {
    hms::Patient lowerGender("P3","Alice",30,"f","5551234567","123 Main St");
    std::string err;
    bool ok = fm.savePatient(lowerGender, err);
    check("P5: lowercase gender patient saved", ok);
    check("P5: gender stored as F",
          lowerGender.getGender() == "F");
  }

  // --- P6: Age boundary — 0 and 120 are valid ---
  {
    std::string err;
    bool ok0 = fm.savePatient(
        hms::Patient("P4","Bob",0,"M","5551234568","456 Oak Ave"), err);
    check("P6: age 0 accepted and saved", ok0);

    err.clear();
    bool ok120 = fm.savePatient(
        hms::Patient("P5","Carol",120,"F","5551234569","789 Pine Rd"), err);
    check("P6: age 120 accepted and saved", ok120);
  }

  // --- P7: Age 121 rejected, CSV unchanged ---
  {
    const std::string before = readFile(csvPath);
    hms::Patient tooOld("P6","Dave",121,"M","5551234560","1 Oak St");
    std::string err;
    bool ok = fm.savePatient(tooOld, err);
    check("P7: age 121 rejected",        !ok);
    check("P7: error mentions age",       err.find("Age") != std::string::npos ||
                                          err.find("age") != std::string::npos);
    check("P7: CSV unchanged",            readFile(csvPath) == before);
  }

  // --- P8: Duplicate-ID check (Issue #2) still works ---
  {
    // At this point P2 is already stored (from P4).
    // Attempt to re-save P2 (uppercase) and verify the CSV is not modified.
    const std::string before = readFile(csvPath);
    std::string err;
    bool okDup = fm.savePatient(make("P2"), err);
    check("P8: duplicate P2 rejected",   !okDup);
    check("P8: error mentions duplicate",
          err.find("already") != std::string::npos ||
          err.find("exists")  != std::string::npos);
    check("P8: CSV unchanged on dup",    readFile(csvPath) == before);
  }

  // --- P9: VR-14 security — formula injection in address not persisted ---
  {
    const std::string before = readFile(csvPath);
    hms::Patient injection("P7","Alice",30,"M","5551234567","=SUM(A1:A10)");
    std::string err;
    bool ok = fm.savePatient(injection, err);
    check("P9: formula address rejected", !ok);
    check("P9: CSV unchanged",            readFile(csvPath) == before);
  }

  // --- P10: Medical history max boundary ---
  {
    std::string err;
    hms::Patient hist200("P8","Alice",30,"M","5551234561","123 Main St",
                         std::string(200,'H'));
    bool ok = fm.savePatient(hist200, err);
    check("P10: hist len 200 saved", ok);

    err.clear();
    const std::string before = readFile(csvPath);
    hms::Patient hist201("P9","Alice",30,"M","5551234562","123 Main St",
                         std::string(201,'H'));
    bool bad = fm.savePatient(hist201, err);
    check("P10: hist len 201 rejected", !bad);
    check("P10: CSV unchanged",         readFile(csvPath) == before);
  }
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  const std::string csvPath = "test_tc_pat_03_patients.csv";
  removeFile(csvPath);

  std::cout << "=== TC-PAT-03: Validate Patient Fields ===\n";

  testPatientId();
  testName();
  testAge();
  testGender();
  testContact();
  testAddress();
  testMedicalHistory();
  testSecurity();
  testPersistence(csvPath);

  removeFile(csvPath);

  std::cout << "\nResults: " << g_passed << " passed, " << g_failed
            << " failed.\n";

  return g_failed == 0 ? 0 : 1;
}
