#pragma once
// The 64-bit dispatch (README roadmap step 7, "Calls go to our code"): none of the game's x86
// code runs in a 64-bit process, so the game's code range (0x401000..0x8FB992, reserved at
// the start of bullyre_x64.exe's .text by src/x64/reserve64.asm) becomes a table of x64
// jumps, one at each of the game's function addresses. Every address of a game function our
// code calls (GAME_FN, casts, re::Ours), every one stored in the game's data (vtables, the
// static initializers' table, callback tables) then lands in our code, unchanged:
//   - a registered function: a jump to its entry (re::Body: the body, or the adapter of a
//     body with re::Edx, which takes the game's (self, args...));
//   - one of the game's jump thunks (jmp rel32 / rel8): a jump to its target's stub;
//   - an import thunk (jmp [IAT slot]): a jump to the 64-bit DLL's export of the same name;
//   - anything else: a trap that reports the address and its caller, and exits.

#include <cstdint>
#include <cstdio>

namespace x64 {

struct DispatchCounts {
    int functions = 0;   // function starts given a stub
    int ours = 0;        // ... to a registered body
    int adapters = 0;    // ... of which through an adapter (a body with re::Edx)
    int thunks = 0;      // ... aliasing a jump thunk's target
    int imports = 0;     // ... to a 64-bit DLL's export
    int traps = 0;       // ... with nothing behind them
    int tooClose = 0;    // starts closer than 5 bytes to the next (no room for a jump)
};

// The game's exe file (Bully_unpacked.exe) read into memory: its section table, its import
// table and the x86 bytes at each function start (to tell thunks apart).
bool LoadGameImage(const char* path);

// Writes the stubs. `functionList` is tools' functions_ghidra.txt (every function start the
// game's exe has; the registered addresses are added to it). Logs each import it can't find
// and the traps' count. Call after static initialisation (every registration is in).
// `namedStarts` (optional): more starts, the game addresses our source names that the list
// hasn't got (tools/x64census.py writes build64/x64_named_starts.txt)
DispatchCounts InstallDispatch(const char* functionList, FILE* log, const char* namedStarts = nullptr);

// The game's .rdata and .data at their addresses (the raw bytes; the rest of .data zero):
// what the game's static initialisers and code read and write.
bool MapGameData(FILE* log);

// The trap's report: the last missing function called (0 when none).
uint32_t LastMissing();

// With this on, a trap raises the SEH exception kMissingFunction (parameters: the game
// address, the caller) instead of exiting, for a test run that goes on past it.
constexpr unsigned long kMissingFunction = 0xE0B6A001;
void SetTrapsRecoverable(bool on);
// With this on, a trap logs the address (once) and returns 0 to its caller: a survey of what's
// missing along a path, at the risk of what a 0 does there.
void SetTrapsLenient(bool on);
long LenientCount();

} // namespace x64
