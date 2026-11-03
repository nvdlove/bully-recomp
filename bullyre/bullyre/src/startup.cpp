// What bullyre does before any of the game's own code runs, whichever way it got in:
// bullyre.dll's DllMain (bullymenu loads it at the game's first instruction) or the
// standalone host exe (src/host), once it has mapped the game.

#include <windows.h>
#include <xmmintrin.h>

#include <cstdint>
#include <cstring>

#include "game/HeapCheck.h"
#include "log.h"
#include "reversed.h"

// trap_nan: each place an invalid operation (a NaN made) traps is logged once (module and
// offset), then that instruction is let through masked; Game::Idle unmasks again next frame
// trap_nan = 3: a hardware write watch (DR0) on one float (TheCamera's position z), armed by
// Game::Idle raising 0xE0BADCA7 with the address; each writer is logged once, and flagged
// when what it wrote is a NaN or an infinity
#if defined(_M_IX86)
static uintptr_t watched = 0;

static void LogWriter(CONTEXT* c, uintptr_t at) {
    static uintptr_t seen[128];
    static bool badSeen[128];
    static int count = 0;
    uint32_t bits = *reinterpret_cast<const uint32_t*>(watched);
    bool bad = (bits & 0x7F800000u) == 0x7F800000u;
    int i = 0;
    while (i < count && seen[i] != at) {
        i++;
    }
    if (i < count && (badSeen[i] || !bad)) {
        return;
    }
    if (i == count) {
        if (count == 128) {
            return;
        }
        seen[count++] = at;
    }
    badSeen[i] = badSeen[i] || bad;
    HMODULE module = nullptr;
    char name[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(at), &module)) {
        GetModuleFileNameA(module, name, MAX_PATH);
    }
    const char* base = strrchr(name, '\\');
    const uint32_t* sp = reinterpret_cast<const uint32_t*>(c->Esp);
    re::Log("watch: %s+0x%X wrote 0x%08X%s  stack %08X %08X %08X %08X %08X %08X %08X %08X", base ? base + 1 : name,
            unsigned(at - uintptr_t(module)), bits, bad ? " (NaN/inf)" : "", sp[0], sp[1], sp[2], sp[3], sp[4], sp[5],
            sp[6], sp[7]);
}

static LONG CALLBACK NanTrap(EXCEPTION_POINTERS* info) {
    DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code == 0xE0BADCA7 && info->ExceptionRecord->NumberParameters == 1) {
        CONTEXT* c = info->ContextRecord;
        watched = uintptr_t(info->ExceptionRecord->ExceptionInformation[0]);
        c->Dr0 = watched;
        c->Dr7 = (c->Dr7 & ~0xF0003u) | 1u | (1u << 16) | (3u << 18);   // local, write, 4 bytes
        c->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        re::Log("watch: armed on 0x%08X", unsigned(watched));
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code == EXCEPTION_SINGLE_STEP && watched && (info->ContextRecord->Dr6 & 1)) {
        LogWriter(info->ContextRecord, uintptr_t(info->ExceptionRecord->ExceptionAddress));
        info->ContextRecord->Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (code != STATUS_FLOAT_INVALID_OPERATION && code != STATUS_FLOAT_MULTIPLE_TRAPS &&
        code != STATUS_FLOAT_DIVIDE_BY_ZERO && code != STATUS_FLOAT_OVERFLOW) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* c = info->ContextRecord;
    // (x87's are reported at the next x87 instruction: the faulting one is in ErrorOffset)
    uintptr_t at = code == STATUS_FLOAT_MULTIPLE_TRAPS ? uintptr_t(info->ExceptionRecord->ExceptionAddress)
                                                       : uintptr_t(c->FloatSave.ErrorOffset);
    static uintptr_t seen[256];
    static int count = 0;
    bool known = false;
    for (int i = 0; i < count; i++) {
        known |= seen[i] == at;
    }
    if (!known && count < 256) {
        seen[count++] = at;
        HMODULE module = nullptr;
        char name[MAX_PATH] = "?";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(at), &module)) {
            GetModuleFileNameA(module, name, MAX_PATH);
        }
        const char* base = strrchr(name, '\\');
        uint32_t flags = code == STATUS_FLOAT_MULTIPLE_TRAPS ? *reinterpret_cast<uint32_t*>(c->ExtendedRegisters + 24) & 0x3F
                                                              : c->FloatSave.StatusWord & 0x3F;
        re::Log("nan site: %s+0x%X (0x%08X) code 0x%08X flags 0x%02X (1 invalid, 4 /0, 8 overflow)",
                base ? base + 1 : name, unsigned(at - uintptr_t(module)), unsigned(at), unsigned(code), unsigned(flags));
        // (the stack: return addresses among it say who called)
        const uint32_t* sp = reinterpret_cast<const uint32_t*>(c->Esp);
        re::Log("  stack %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X", sp[0], sp[1], sp[2], sp[3],
                sp[4], sp[5], sp[6], sp[7], sp[8], sp[9], sp[10], sp[11]);
    }
    // masked again for this instruction (SSE: MXCSR at +24 of the FXSAVE area; x87: IM)
    uint32_t* mxcsr = reinterpret_cast<uint32_t*>(c->ExtendedRegisters + 24);
    *mxcsr = (*mxcsr | 0x680u) & ~0x3Fu;   // (invalid, divide by zero, overflow)
    c->FloatSave.ControlWord |= 0xD;
    c->FloatSave.StatusWord &= ~0x80FFu;
    return EXCEPTION_CONTINUE_EXECUTION;
}
#else
// (the traps read the x87 state and the 32-bit registers of the game's threads: nothing of
// that in the 64-bit build, where the game is this code)
static LONG CALLBACK NanTrap(EXCEPTION_POINTERS*) {
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

namespace re {

void Startup() {
    re::Log("---- bullyre loaded");
    // (testing: "bullyre.off = 1" in bullyre.ini loads the dll and does nothing)
    if (re::IniInt("bullyre.off", 0) == 0) {
        re::InstallAll();
        // (testing: "trap_nan = 1" unmasks SSE's invalid operation on this, the main,
        // thread: the first NaN our code makes (0/0, sqrt(-x), inf - inf) stops the game
        // there, with a crash report; the game's own code is x87 and unaffected)
        // ("= 4": only the handler, for the write watch: nothing unmasked, timing as usual)
        int trap = re::IniInt("trap_nan", 0);
        if (trap != 0) {
            AddVectoredExceptionHandler(1, NanTrap);
        }
        // (testing: "heap_check = 250" walks the memory manager's heaps every 250 ms and logs
        // the first inconsistent block header: src/game/HeapCheck.cpp)
        heapcheck::Start(re::IniInt("heap_check", 0));
        if (trap >= 1 && trap <= 3) {
            _mm_setcsr(_mm_getcsr() & ~0x680u);
            re::Log("bullyre: trapping SSE invalid operations");
        }
    } else {
        re::Log("bullyre: off (bullyre.ini)");
    }
}

} // namespace re
