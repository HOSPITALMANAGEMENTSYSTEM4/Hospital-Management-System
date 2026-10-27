# TC-NFR-09 — Standalone Offline Operation

Requirement: HMS-NF-009
Issue: #43
Test case: TC-NFR-09

## Acceptance criteria

- Application operates as a standalone, single-user process.
- Application starts and exits normally without a network connection.
- No database server is required for startup and normal menu exit.
- No additional GCC runtime DLLs are imported by the executable.

## Test results

| Check | Result | Evidence |
|---|---|---|
| C++ build | PASS | CMake/Ninja build succeeded |
| Existing regression suite | PASS | 39/39 tests passed before adding this test |
| Offline startup and exit | PASS | Manually verified with Wi-Fi disconnected |
| GCC runtime DLL imports | PASS | libwinpthread-1.dll no longer imported |
| Dedicated CTest verification | Pending | Run TC-NFR-09_StandaloneOffline |
| Final regression suite | Pending | Run all CTest tests after the change |

## Limitations

The offline test was manually performed on the development computer.
A separate clean-machine test is still needed to establish behavior on a
machine without the development environment installed.

The runtime import inspection checks GCC runtime DLLs. It does not by
itself prove the absence of every possible third-party dependency.
