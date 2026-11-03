#include "reversed.h"
#include "sysabi.h"

#include <windows.h>

#include <MinHook.h>

#include <atomic>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "log.h"
#include "runflags.h"

namespace re {

namespace {
bool g_forceWindowed = false;
bool g_keepActive = false;
std::atomic<bool> g_frameGrab{false};
struct ScriptedKey {
    uint16_t code;   // (a DirectInput key; 0x100+ a mouse button)
    uint32_t at, until;
};
std::vector<ScriptedKey> g_scriptedKeys;
ULONGLONG g_scriptStart = 0;
char g_frameGrabPath[MAX_PATH] = {};
}
bool ForceWindowed() {
    return g_forceWindowed;
}
void SetForceWindowed(bool on) {
    g_forceWindowed = on;
}
bool g_playWindow = false;
bool PlayWindow() {
    return g_playWindow && g_forceWindowed;
}
void SetPlayWindow(bool on) {
    g_playWindow = on;
}
bool g_background = false;
bool Background() {
    return g_background;
}
void SetBackground(bool on) {
    g_background = on;
}
bool g_twoAreas = true;
bool TwoAreas() {
    return g_twoAreas;
}
void SetTwoAreas(bool on) {
    g_twoAreas = on;
}
bool g_seamlessDoors = false;
bool SeamlessDoors() {
    return g_seamlessDoors;
}
void SetSeamlessDoors(bool on) {
    g_seamlessDoors = on;
}
const float* g_lightsFrom = nullptr;
const float* LightsFrom() {
    return g_lightsFrom;
}
void SetLightsFrom(const float* position) {
    g_lightsFrom = position;
}
bool g_portalScan = false;
bool PortalScan() {
    return g_portalScan;
}
void SetPortalScan(bool on) {
    g_portalScan = on;
}
bool g_portals = true;
bool Portals() {
    return g_portals;
}
void SetPortals(bool on) {
    g_portals = on;
}
bool g_portalFull = false;
bool PortalFull() {
    return g_portalFull;
}
void SetPortalFull(bool on) {
    g_portalFull = on;
}
bool g_portalTest = false;
bool PortalTest() {
    return g_portalTest;
}
void SetPortalTest(bool on) {
    g_portalTest = on;
}
bool g_doorReport = false;
bool DoorReport() {
    return g_doorReport;
}
void SetDoorReport(bool on) {
    g_doorReport = on;
}
struct ScriptedDoorUse {
    uint32_t at;
    int door;
    bool done;
};
std::vector<ScriptedDoorUse> g_scriptedDoorUses;
void AddScriptedDoorUse(uint32_t atMs, int door) {
    if (g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
    g_scriptedDoorUses.push_back({atMs, door, false});
}
bool g_ultraWide = false;
bool UltraWide() {
    return g_ultraWide;
}
void SetUltraWide(bool on) {
    g_ultraWide = on;
}
uint16_t g_shadowMapSize = 0x400;
uint16_t ShadowMapSize() {
    return g_shadowMapSize;
}
void SetShadowMapSize(uint16_t size) {
    g_shadowMapSize = size;
}
bool KeepActive() {
    return g_keepActive;
}
void SetKeepActive(bool on) {
    g_keepActive = on;
}
void RequestFrameGrab(const char* path) {
    if (!g_frameGrab.load()) {
        strncpy_s(g_frameGrabPath, path, _TRUNCATE);
        g_frameGrab.store(true);
    }
}
void AddScriptedKey(uint16_t code, uint32_t atMs, uint32_t forMs) {
    if (g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
    g_scriptedKeys.push_back({code, atMs, atMs + forMs});
}
std::atomic<uint32_t> g_drawCalls{0}, g_drawFailed{0};
std::atomic<uint32_t> g_drawSites[4];
long CountDraw(long hr, int site) {
    g_drawSites[site & 3]++;
    g_drawCalls++;
    if (hr < 0) {
        g_drawFailed++;
    }
    return hr;
}
uint32_t DrawSiteCount(int site) {
    return g_drawSites[site & 3].load();
}
void DrawCounts(uint32_t* calls, uint32_t* failed) {
    *calls = g_drawCalls.load();
    *failed = g_drawFailed.load();
}
#if defined(_M_X64)
namespace {
struct WatchRequest {
    HANDLE thread;
    const void* address;
};
DWORD WINAPI SetWatch(void* p) {
    auto* r = static_cast<WatchRequest*>(p);
    SuspendThread(r->thread);
    CONTEXT c = {};
    c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(r->thread, &c)) {
        c.Dr0 = reinterpret_cast<DWORD64>(r->address);
        // L0 on, R/W0 = 01 (writes), LEN0 = 11 (4 bytes)
        c.Dr7 = r->address ? (c.Dr7 & ~0xF0003ull) | 0x1 | (0x1ull << 16) | (0x3ull << 18) : (c.Dr7 & ~0x3ull);
        SetThreadContext(r->thread, &c);
    }
    ResumeThread(r->thread);
    return 0;
}
}
void WatchWrites(const void* address) {
    WatchRequest r;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &r.thread, THREAD_ALL_ACCESS, FALSE, 0);
    r.address = address;
    HANDLE helper = CreateThread(nullptr, 0, &SetWatch, &r, 0, nullptr);
    WaitForSingleObject(helper, INFINITE);
    CloseHandle(helper);
    CloseHandle(r.thread);
}
#else
void WatchWrites(const void*) {}
#endif
struct ScriptedTeleport {
    uint32_t at;
    float x, y, z, heading;
    int area;
    bool done;
};
std::vector<ScriptedTeleport> g_scriptedTeleports;
void AddScriptedTeleport(uint32_t atMs, float x, float y, float z, int area, float heading) {
    if (g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
    g_scriptedTeleports.push_back({atMs, x, y, z, heading, area, false});
}
uint32_t g_scriptedQuit = 0;   // (ms on the keys' clock; 0: none)
void AddScriptedQuit(uint32_t atMs) {
    if (g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
    g_scriptedQuit = atMs;
}
struct ScriptedMissions {
    uint32_t at = 0, every = 0;
    uint32_t passAfter = 0;
    ULONGLONG passAt = 0;   // (0: none due)
    int slotState = -2;     // (the main slot's state when last seen)
    bool ending = false;    // (the running one asked to end before the next starts)
    std::vector<int> queue;
    size_t next = 0;
    bool listed = false;
    bool Pending() const { return next < queue.size(); }
};
ScriptedMissions g_missions;
char g_currentMission[96] = "";
void AddScriptedMissions(uint32_t atMs, uint32_t everyMs, const int* missions, int count) {
    if (g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
    g_missions.at = atMs;
    g_missions.every = everyMs;
    g_missions.queue.assign(missions, missions + count);
    g_missions.next = 0;
}
void SetScriptedMissionPass(uint32_t afterMs) {
    g_missions.passAfter = afterMs;
}
const char* CurrentScriptedMission() {
    return g_currentMission;
}
char g_outputTag[32] = "";
void SetOutputTag(const char* tag) {
    std::snprintf(g_outputTag, sizeof g_outputTag, "%s", tag ? tag : "");
}
const char* OutputName(const char* what) {
    // (a few names at once: the crash report's and the minidump's)
    static thread_local char names[4][MAX_PATH];
    static thread_local int turn = 0;
    char* name = names[turn++ & 3];
    std::snprintf(name, MAX_PATH, *g_outputTag ? "bullyre_x64_%s_%s" : "bullyre_x64_%s%s", g_outputTag, what);
    return name;
}
namespace {
void* const kMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
int MissionCount() {
    return *reinterpret_cast<const int32_t*>(0x020C3CA0 + 0x744);
}
// a mission's name (+0x18) or script (+0x1C) from its data (CMissionMgr +0x4B8, 0x4C each)
const char* MissionText(int index, int offset) {
    uint32_t data = *reinterpret_cast<const uint32_t*>(0x020C3CA0 + 0x4B8);
    if (data == 0) {
        return "";
    }
    uint32_t text = *reinterpret_cast<const uint32_t*>(uintptr_t(data) + uintptr_t(index) * 0x4C + offset);
    return text ? reinterpret_cast<const char*>(uintptr_t(text)) : "";
}
bool MissionIsDebug(int index) {
    uint32_t debug = reinterpret_cast<uint32_t(__cdecl*)(const char*)>(0x00576ED0)("Debug");   // (name hash)
    return reinterpret_cast<uint8_t(__thiscall*)(void*, int32_t, uint32_t)>(0x006AB290)(kMissionMgr, index, debug) != 0;
}
// the script function ForceStartMissionIndex: a running mission ended, the mission made
// available and started
void ForceStartMission(int index) {
    uint8_t* current = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x006AA680)(kMissionMgr);
    if (reinterpret_cast<uint8_t(__thiscall*)(void*)>(0x006AA3F0)(current) != 0) {
        reinterpret_cast<uint8_t(__thiscall*)(void*, int, int, int, int, int, int, int)>(0x006AA300)(current, 1, 0, 0, 1, 0, 0, 0);
    }
    reinterpret_cast<uint8_t*(__thiscall*)(void*, int)>(0x006AA7C0)(kMissionMgr, index)[4] |= 2;
    reinterpret_cast<void(__thiscall*)(void*, int)>(0x006AD290)(kMissionMgr, 1);
    reinterpret_cast<void(__thiscall*)(void*, int, int)>(0x006AD100)(kMissionMgr, index, 0);
}
void ListMissions() {
    FILE* f = std::fopen(OutputName("missions.txt"), "w");
    if (!f) {
        return;
    }
    for (int i = 0; i < MissionCount(); i++) {
        std::fprintf(f, "%d\t%s\t%s\t%s\n", i, MissionIsDebug(i) ? "debug" : "-", MissionText(i, 0x18), MissionText(i, 0x1C));
    }
    std::fclose(f);
}
void RunScriptedMissions(ULONGLONG now) {
    ScriptedMissions& m = g_missions;
    if (m.listed) {
        // (the main slot's state changes said: 4 running; whether a mission ended by itself)
        uint8_t* slot = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x006AA680)(kMissionMgr);
        int state = slot ? *reinterpret_cast<const int32_t*>(slot + 8) : -1;
        if (state != m.slotState) {
            std::fprintf(stderr, "x64: mission slot state %d -> %d (%s)\n", m.slotState, state, g_currentMission);
            m.slotState = state;
        }
        // (the second slot's too: minigames run there)
        static int slot2State = -2;
        uint8_t* slot2 = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x006AA690)(kMissionMgr);
        int state2 = slot2 ? *reinterpret_cast<const int32_t*>(slot2 + 8) : -1;
        if (state2 != slot2State) {
            std::fprintf(stderr, "x64: mission slot2 state %d -> %d (%s)\n", slot2State, state2, g_currentMission);
            slot2State = state2;
        }
        // (passed once it runs (CMissionSlot::Pass only passes state 4): still starting at its
        // time, it's tried again until its turn ends)
        if (m.passAt != 0 && now >= m.passAt) {
            if (state == 4) {
                m.passAt = 0;
                reinterpret_cast<void(__thiscall*)(void*, int, int, int)>(0x006ABF40)(slot, 0, 1, 1);
                Log("mission passed: %s", g_currentMission);
                std::fprintf(stderr, "x64: mission passed: %s\n", g_currentMission);
            } else if (state <= 0 || now >= m.at) {
                m.passAt = 0;
                std::fprintf(stderr, "x64: mission not running at its pass: %s\n", g_currentMission);
            }
        }
    }
    if (m.listed) {
        // (the confirmation the game waits on, "This will end your current task. Are you sure?"
        // (CMissionMgr's popup, +0x4DC; open while its +0x50), No chosen: Yes is picked: Up, then
        // the popup's accept, button 7: Left Alt by the default keys (0x454E00))
        // (a menu a mission opens and waits on, the pause menu's map or status for a tutorial: the
        // game's state 1 (0xBA6894) with its clock (CTimer's ms, 0xC1A9B4) standing still for 5
        // seconds: Back (button 8: Space) and Start (button 5: Escape, the pause menu's toggle) by turns)
        static uint32_t lastClock = 0;
        static ULONGLONG clockSince = 0, backed = 0;
        static int presses = 0;
        uint32_t clock = *reinterpret_cast<const volatile uint32_t*>(0x00C1A9B4);
        if (clock != lastClock) {
            lastClock = clock;
            clockSince = now;
        } else if (*reinterpret_cast<const volatile uint32_t*>(0x00BA6894) == 1 && now > clockSince + 5000 && now > backed + 4000) {
            backed = now;
            AddScriptedKey(presses++ % 2 == 0 ? 0x39 : 0x01, uint32_t(now), 250);   // (DIK_SPACE, DIK_ESCAPE)
            std::fprintf(stderr, "x64: the game's menu is up: Back pressed (%s)\n", g_currentMission);
        }
        static ULONGLONG answered = 0;
        const volatile uint8_t* popup = reinterpret_cast<const volatile uint8_t*>(0x020C3CA0 + 0x4DC);
        if (popup[0x50] != 0 && now > answered + 3000) {
            answered = now;
            AddScriptedKey(0xC8, uint32_t(now), 250);         // (DIK_UP)
            AddScriptedKey(0x38, uint32_t(now) + 700, 250);   // (DIK_LMENU)
            std::fprintf(stderr, "x64: the game's popup waits: Yes picked (%s)\n", g_currentMission);
        }
    }
    if (!m.Pending() || now < m.at) {
        return;
    }
    // (one running when the next's turn comes is ended first, and the next waits for the slot to
    // be idle (0), 30 seconds at most: a start while it's still ending is dropped by the game)
    if (m.slotState == 4 && !m.ending) {
        uint8_t* slot = reinterpret_cast<uint8_t*(__thiscall*)(void*)>(0x006AA680)(kMissionMgr);
        reinterpret_cast<uint8_t(__thiscall*)(void*, int, int, int, int, int, int, int)>(0x006AA300)(slot, 1, 0, 0, 1, 0, 0, 0);
        m.ending = true;
        return;
    }
    if (m.slotState > 0 && now < m.at + 30000ull) {
        return;
    }
    if (m.slotState > 0) {
        // (the slot kept by the game (a pass that waits on a scene, a fade): no mission starts in
        // this game any more; the test run ends, the next ones go to a new game)
        std::fprintf(stderr, "x64: mission slot stuck in state %d: the run ends here (%s)\n", m.slotState, g_currentMission);
        std::fflush(nullptr);
        TerminateProcess(GetCurrentProcess(), 5);
    }
    m.ending = false;
    // (not before the game has started (CGame's flag 0xBCBC3B) and run for 8 seconds: the player
    // exists while it's still loading, the missions' tracks don't yet)
    static ULONGLONG readySince = 0;
    if (*reinterpret_cast<const volatile uint8_t*>(0x00BCBC3B) == 0 || *reinterpret_cast<const uint32_t*>(0x00C1AEA8) == 0 ||
        MissionCount() <= 0) {
        readySince = 0;
        return;
    }
    if (readySince == 0) {
        readySince = now;
    }
    if (now < readySince + 8000) {
        return;
    }
    if (!m.listed) {
        m.listed = true;
        ListMissions();
    }
    while (m.Pending() && (m.queue[m.next] < 0 || m.queue[m.next] >= MissionCount())) {
        m.next++;
    }
    if (!m.Pending()) {
        std::snprintf(g_currentMission, sizeof g_currentMission, "(after the last)");
        std::fprintf(stderr, "x64: missions: the last one started\n");
        return;
    }
    int index = m.queue[m.next++];
    std::snprintf(g_currentMission, sizeof g_currentMission, "%d %s (%s)", index, MissionText(index, 0x18), MissionText(index, 0x1C));
    Log("mission: %s", g_currentMission);
    std::fprintf(stderr, "x64: mission %s\n", g_currentMission);
    ForceStartMission(index);
    m.at = uint32_t(now) + m.every;
    m.passAt = m.passAfter != 0 ? now + m.passAfter : 0;
}
}
void RunScheduled() {
    PlayWindowFrame();   // (bullyre_x64 played: the cursor held while playing)
    if (g_doorReport) {
        ReportDoors();
    }
    for (ScriptedDoorUse& d : g_scriptedDoorUses) {
        if (!d.done && GetTickCount64() - g_scriptStart >= d.at) {
            d.done = true;
            UseDoor(d.door);
        }
    }
    if (g_scriptedTeleports.empty() && g_scriptedQuit == 0 && !g_missions.Pending() && !g_missions.listed) {
        return;
    }
    ULONGLONG now = GetTickCount64() - g_scriptStart;
    RunScriptedMissions(now);
    if (g_scriptedQuit != 0 && now >= g_scriptedQuit) {
        g_scriptedQuit = 0;
        std::fprintf(stderr, "x64: quit (the front end's Quit)\n");
        reinterpret_cast<void(__thiscall*)(void*)>(0x006998B0)(nullptr);
    }
    for (ScriptedTeleport& t : g_scriptedTeleports) {
        if (t.done || now < t.at) {
            continue;
        }
        t.done = true;
        uint32_t player = *reinterpret_cast<const uint32_t*>(0x00C1AEA8);
        uint32_t flags = *reinterpret_cast<const uint32_t*>(0x009E4F4C);
        if (player == 0 || flags == 0) {
            Log("teleport: no player yet");
            continue;
        }
        *reinterpret_cast<uint8_t*>(uintptr_t(flags) + 0x8DC) = 1;
        float pos[3] = {t.x, t.y, t.z};
        int area = t.area;
        // (AreaTransitionManager::RequestTransition, thiscall on 0xBCC580)
        reinterpret_cast<void(__thiscall*)(void*, const float*, float, const int*)>(0x00415880)(
            reinterpret_cast<void*>(0x00BCC580), pos, t.heading, &area);
        Log("teleport: to %.2f %.2f %.2f area %d", t.x, t.y, t.z, t.area);
        std::fprintf(stderr, "x64: teleport to %.2f %.2f %.2f area %d\n", t.x, t.y, t.z, t.area);
    }
}
#if defined(_M_X64)
void NoteInput(const char* where, long hr, const uint8_t* state, uint32_t size) {
    static std::mutex lock;
    static std::unordered_map<std::string, std::string> last;
    static int lines = 0;
    static FILE* f = nullptr;
    std::string now = std::to_string(hr) + ":";
    if (hr >= 0 && state) {
        if (size == 0x100) {
            for (uint32_t k = 0; k < 0x100; k++) {
                if (state[k] & 0x80) {
                    now += " " + std::to_string(k);
                }
            }
        } else if (size >= 0x10) {
            for (uint32_t b = 0xC; b < size; b++) {
                if (state[b] & 0x80) {
                    now += " b" + std::to_string(b - 0xC);
                }
            }
        }
    }
    std::lock_guard<std::mutex> guard(lock);
    std::string& before = last[where];
    if (before == now || lines >= 400) {
        return;
    }
    before = now;
    if (!f) {
        f = std::fopen(OutputName("input.txt"), "w");
        if (!f) {
            return;
        }
    }
    lines++;
    std::fprintf(f, "%8llu %s hr %08lX keys%s\n", static_cast<unsigned long long>(GetTickCount64()), where,
                 static_cast<unsigned long>(hr), now.substr(now.find(':') + 1).c_str());
    std::fflush(f);
}
#else
void NoteInput(const char*, long, const uint8_t*, uint32_t) {}
#endif
bool g_enterUntilStarted = false;
void SetEnterUntilStarted(bool on) {
    g_enterUntilStarted = on;
    if (on && g_scriptStart == 0) {
        g_scriptStart = GetTickCount64();
    }
}
bool ScriptedKeysActive() {
    return !g_scriptedKeys.empty() || g_enterUntilStarted;
}
void ApplyScriptedKeys(uint8_t* state) {
    ULONGLONG now = GetTickCount64() - g_scriptStart;
    // (--start: ENTER for 0.3 s of every 2 from 20 s on until the game has started (CGame's flag
    // 0xBCBC3B): the movies skipped and STORY picked however long the loading takes)
    if (g_enterUntilStarted && now >= 20000 && *reinterpret_cast<const volatile uint8_t*>(0x00BCBC3B) == 0 && now % 2000 < 300) {
        state[0x1C] = 0x80;
    }
    for (const ScriptedKey& k : g_scriptedKeys) {
        if (k.code < 0x100 && now >= k.at && now < k.until) {
            state[k.code] = 0x80;
        }
    }
}
void ApplyScriptedMouse(uint8_t* state) {
    ULONGLONG now = GetTickCount64() - g_scriptStart;
    for (const ScriptedKey& k : g_scriptedKeys) {
        if (k.code >= 0x100 && k.code < 0x104 && now >= k.at && now < k.until) {
            state[0xC + (k.code - 0x100)] = 0x80;
        }
    }
}
bool TakeFrameGrab() {
    return g_frameGrab.exchange(false);
}
const char* FrameGrabPath() {
    return g_frameGrabPath;
}

// Registration happens during static initialisation, in any order: a function-local
// list is safe to use from there.
static std::vector<Function>& Functions() {
    static std::vector<Function> list;
    return list;
}

// each function's source file (for "file:" rules), by its index in Functions()
static std::vector<const char*>& Files() {
    static std::vector<const char*> list;
    return list;
}

bool Register(const Function& f, const char* file) {
    Functions().push_back(f);
    Files().push_back(file ? file : "");
    return true;
}

size_t RegisteredCount() {
    return Functions().size();
}

const Function& Registered(size_t index) {
    return Functions()[index];
}

#if defined(_WIN64)
_RTL_CRITICAL_SECTION* GameSection(void* section) {
    auto* slot = static_cast<volatile long*>(section);
    if (uint32_t v = uint32_t(*slot)) {
        return reinterpret_cast<CRITICAL_SECTION*>(uintptr_t(v));
    }
    auto* mine = new CRITICAL_SECTION;
    InitializeCriticalSection(mine);
    long value = long(uint32_t(reinterpret_cast<uintptr_t>(mine)));
    if (long already = _InterlockedCompareExchange(slot, value, 0); already != 0) {
        DeleteCriticalSection(mine);
        delete mine;
        return reinterpret_cast<CRITICAL_SECTION*>(uintptr_t(uint32_t(already)));
    }
    return mine;
}
void DeleteGameSection(void* section) {
    auto* slot = static_cast<volatile long*>(section);
    if (uint32_t v = uint32_t(_InterlockedExchange(slot, 0))) {
        auto* mine = reinterpret_cast<CRITICAL_SECTION*>(uintptr_t(v));
        DeleteCriticalSection(mine);
        delete mine;
    }
}
// re::NativeField's slots: one per field address, made on first use and kept (an
// unordered_map's elements stay where they are as it grows)
void*& NativeFieldSlot(const void* field) {
    static SRWLOCK lock = SRWLOCK_INIT;
    static auto* slots = new std::unordered_map<uintptr_t, void*>();
    AcquireSRWLockExclusive(&lock);
    void*& slot = (*slots)[reinterpret_cast<uintptr_t>(field)];
    ReleaseSRWLockExclusive(&lock);
    return slot;
}
#else
void DeleteGameSection(void* section) {
    DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(section));
}
#endif

void* EntryAt(uintptr_t address) {
    static std::unordered_map<uintptr_t, void*>* byAddress = nullptr;
    if (!byAddress) {
        auto* map = new std::unordered_map<uintptr_t, void*>();
        map->reserve(Functions().size());
        for (const Function& f : Functions()) {
            (*map)[f.address] = f.ours.entry;
        }
        byAddress = map;
    }
    auto it = byAddress->find(address);
    return it == byAddress->end() ? nullptr : it->second;
}

void* OursAt(uintptr_t address) {
    // (built on the first lookup: every registration is in by then)
    static std::unordered_map<uintptr_t, void*>* byAddress = nullptr;
    if (!byAddress) {
        auto* map = new std::unordered_map<uintptr_t, void*>();
        map->reserve(Functions().size());
        for (const Function& f : Functions()) {
            (*map)[f.address] = f.ours.ours;
        }
        byAddress = map;
    }
    auto it = byAddress->find(address);
    if (it == byAddress->end()) {
        Log("re::OursAt: nothing registered at 0x%08X", unsigned(address));
        return nullptr;
    }
    return it->second;
}

static std::string Trim(std::string s) {
    auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && space(s.back())) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && space(s[i])) {
        i++;
    }
    return s.substr(i);
}

static bool IniString(const char* key, std::string& value);

int IniInt(const char* key, int fallback) {
    std::string v;
    if (!IniString(key, v)) {
        return fallback;
    }
    return int(strtol(v.c_str(), nullptr, 0));
}

// "key = value" from bullyre.ini
static bool IniString(const char* key, std::string& value) {
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    while (n > 0 && path[n - 1] != '\\') {
        n--;
    }
    strcpy_s(path + n, MAX_PATH - n, "bullyre.ini");
    FILE* f = fopen(path, "r");
    if (!f) {
        return false;
    }
    char line[512];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        std::string l = line;
        size_t eq = l.find('=');
        if (l.empty() || l[0] == '#' || eq == std::string::npos || Trim(l.substr(0, eq)) != key) {
            continue;
        }
        value = Trim(l.substr(eq + 1));
        found = true;
    }
    fclose(f);
    return found;
}

// bullyre.ini rules, read once, in order; the last one that matches a function wins:
//   CStreaming::GetInfoIndex = shadow     a name
//   CPed::* = ours                        a pattern ('*' matches anything)
//   file:Ped*.cpp = ours                  the source file it's in (its name only)
//   0x470000-0x49FFFF = ours              its address in a range
//   * = ours                              everything
// Values: original, shadow, ours.
struct Rule {
    enum Kind { Name, File, Range } kind;
    std::string pattern;
    uintptr_t low = 0, high = 0;
    Mode mode;
};

static bool Glob(const char* pattern, const char* text) {
    // '*' matches any run of characters; everything else literally
    const char* star = nullptr;
    const char* resume = nullptr;
    while (*text) {
        if (*pattern == '*') {
            star = pattern++;
            resume = text;
        } else if (*pattern == *text) {
            pattern++;
            text++;
        } else if (star) {
            pattern = star + 1;
            text = ++resume;
        } else {
            return false;
        }
    }
    while (*pattern == '*') {
        pattern++;
    }
    return *pattern == 0;
}

static std::vector<Rule> ReadRules() {
    std::vector<Rule> rules;
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    while (n > 0 && path[n - 1] != '\\') {
        n--;
    }
    strcpy_s(path + n, MAX_PATH - n, "bullyre.ini");
    FILE* f = fopen(path, "r");
    if (!f) {
        return rules;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        std::string l = line;
        size_t eq = l.find('=');
        if (l.empty() || l[0] == '#' || eq == std::string::npos) {
            continue;
        }
        std::string key = Trim(l.substr(0, eq));
        std::string v = Trim(l.substr(eq + 1));
        Rule r;
        if (v == "original") {
            r.mode = Mode::Original;
        } else if (v == "shadow") {
            r.mode = Mode::Shadow;
        } else if (v == "ours") {
            r.mode = Mode::Ours;
        } else {
            continue;  // (other settings, e.g. "bullyre.off")
        }
        unsigned long low = 0, high = 0;
        if (key.rfind("file:", 0) == 0) {
            r.kind = Rule::File;
            r.pattern = key.substr(5);
        } else if (key.rfind("0x", 0) == 0 && sscanf_s(key.c_str(), "%lx-%lx", &low, &high) == 2) {
            r.kind = Rule::Range;
            r.low = low;
            r.high = high;
        } else {
            r.kind = Rule::Name;
            r.pattern = key;
        }
        rules.push_back(r);
    }
    fclose(f);
    return rules;
}

static const char* BaseName(const char* path) {
    const char* base = path;
    for (const char* p = path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }
    return base;
}

static void ApplyRules(const std::vector<Rule>& rules, const Function& f, const char* file, Mode& mode) {
    for (const Rule& r : rules) {
        bool match = r.kind == Rule::Name    ? Glob(r.pattern.c_str(), f.name)
                     : r.kind == Rule::File  ? Glob(r.pattern.c_str(), BaseName(file))
                                             : f.address >= r.low && f.address <= r.high;
        if (match) {
            mode = r.mode;
        }
    }
}

// bullyre_hooks.txt next to the game: every function, its mode this run, its file (for
// the test tools: bisecting, picking batches)
static void WriteManifest() {
    char path[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    while (n > 0 && path[n - 1] != '\\') {
        n--;
    }
    strcpy_s(path + n, MAX_PATH - n, "bullyre_hooks.txt");
    FILE* f = fopen(path, "w");
    if (!f) {
        return;
    }
    static const char* const names[] = {"original", "shadow", "ours"};
    for (size_t i = 0; i < Functions().size(); i++) {
        const Function& fn = Functions()[i];
        fprintf(f, "0x%08X %s %s %s\n", unsigned(fn.address), fn.unhooked ? "unhooked" : names[int(fn.mode)], BaseName(Files()[i]), fn.name);
    }
    fclose(f);
    // (each one's game address and our function's offset in bullyre.dll, for tools/cleanupcheck.py)
    HMODULE self = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&WriteManifest), &self);
    if (self == GetModuleHandleA(nullptr)) {
        return;   // (the host exe: these offsets would be the exe's, not bullyre.dll's)
    }
    strcpy_s(path + n, MAX_PATH - n, "bullyre_ours.txt");
    f = fopen(path, "w");
    if (!f) {
        return;
    }
    for (const Function& fn : Functions()) {
        fprintf(f, "0x%08X 0x%08X\n", unsigned(fn.address), unsigned(uintptr_t(fn.ours.ours) - uintptr_t(self)));
    }
    fclose(f);
}

// Functions whose game callers keep a value in ecx / edx across the call (the original never
// touches it; ours, compiled, may): {address, bytes of arguments popped, 1 ecx | 2 edx}
struct Preserve {
    uintptr_t address;
    uint32_t pops;
    uint32_t regs;
};
static const Preserve kPreserve[] = {
#include "preserve_regs.inc"
};

// A thunk in front of `ours` keeping ecx / edx: it saves them, calls ours with a copy of the
// arguments (all `pops` of them; 64 bytes for a caller-pops function), puts them back and
// returns as the original does. eax, edx:eax results aside, and st(0) pass through.
static void* PreservingThunk(void* ours, const Preserve& p) {
    static uint8_t* block = nullptr;
    static size_t used = 0;
    const uint32_t copy = p.pops ? p.pops : 64;
    const size_t size = 2 + (copy / 4) * 7 + 7 + 7 + 5 + 6 + 8 + 3;   // (the most it takes)
    if (!block || used + size > 0x10000) {
        block = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        used = 0;
        if (!block) {
            return ours;
        }
    }
    uint8_t* code = block + used;
    uint8_t* c = code;
    auto u32 = [&c](uint32_t v) {
        std::memcpy(c, &v, 4);
        c += 4;
    };
    *c++ = 0x51;   // push ecx
    *c++ = 0x52;   // push edx
    for (uint32_t i = 0; i < copy / 4; i++) {
        *c++ = 0xFF, *c++ = 0xB4, *c++ = 0x24;   // push dword [esp + copy + 8]
        u32(copy + 8);
    }
    *c++ = 0x8B, *c++ = 0x8C, *c++ = 0x24;   // mov ecx, [esp + copy + 4]
    u32(copy + 4);
    *c++ = 0x8B, *c++ = 0x94, *c++ = 0x24;   // mov edx, [esp + copy]
    u32(copy);
    *c++ = 0xE8;   // call ours
    u32(uint32_t(reinterpret_cast<uintptr_t>(ours) - reinterpret_cast<uintptr_t>(c + 4)));
    if (!p.pops) {
        *c++ = 0x81, *c++ = 0xC4;   // add esp, copy
        u32(copy);
    }
    if (p.regs & 2) {
        *c++ = 0x5A;   // pop edx
    } else {
        *c++ = 0x8D, *c++ = 0x64, *c++ = 0x24, *c++ = 0x04;   // lea esp, [esp + 4]
    }
    if (p.regs & 1) {
        *c++ = 0x59;   // pop ecx
    } else {
        *c++ = 0x8D, *c++ = 0x64, *c++ = 0x24, *c++ = 0x04;
    }
    if (p.pops) {
        *c++ = 0xC2;   // ret pops
        *c++ = uint8_t(p.pops), *c++ = uint8_t(p.pops >> 8);
    } else {
        *c++ = 0xC3;
    }
    used += size_t(c - code);
    return code;
}

static const Preserve* FindPreserve(uintptr_t address) {
    for (const Preserve& p : kPreserve) {
        if (p.address == address) {
            return &p;
        }
    }
    return nullptr;
}

void InstallAll() {
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        Log("MinHook: %s", MH_StatusToString(status));
        return;
    }
    int ours = 0, shadow = 0, original = 0, unhooked = 0;
    std::vector<Rule> rules = ReadRules();
    for (size_t i = 0; i < Functions().size(); i++) {
        Function& f = Functions()[i];
        if (f.unhooked) {
            unhooked++;   // (RE_UNHOOKED_FN: the body is for OursAt only)
            continue;
        }
        ApplyRules(rules, f, Files()[i], f.mode);
        if (f.mode == Mode::Shadow && !f.shadow) {
            f.mode = Mode::Ours;
        }
        void* target = reinterpret_cast<void*>(f.address);
        void* detour = f.mode == Mode::Shadow ? f.shadow : f.ours.ours;
        if (const Preserve* p = FindPreserve(f.address)) {
            detour = PreservingThunk(detour, *p);
        }
        // the trampoline is made in every mode, so shadows and callers of the
        // original always have it; the hook is only switched on when ours runs
        status = MH_CreateHook(target, detour, f.original);
        if (status != MH_OK) {
            Log("%s at 0x%08X: %s", f.name, unsigned(f.address), MH_StatusToString(status));
            continue;
        }
        if (f.mode == Mode::Original) {
            original++;
            continue;
        }
        // (queued: one MH_ApplyQueued below instead of a thread freeze per hook)
        status = MH_QueueEnableHook(target);
        if (status != MH_OK) {
            Log("%s at 0x%08X: %s", f.name, unsigned(f.address), MH_StatusToString(status));
            continue;
        }
        (f.mode == Mode::Shadow ? shadow : ours)++;
    }
    status = MH_ApplyQueued();
    if (status != MH_OK) {
        Log("MinHook ApplyQueued: %s", MH_StatusToString(status));
    }
    Log("bullyre: %d functions reversed: %d ours, %d in shadow mode, %d original, %d written but not hooked", int(Functions().size()), ours,
        shadow, original, unhooked);
    WriteManifest();
}

void Compared(const char* name, uint32_t& count) {
    uint32_t n = ++count;
    for (uint32_t p = 1; p != 0 && p <= n; p = p <= 429496729 ? p * 10 : 0) {
        if (p == n) {
            Log("%s: %u calls compared", name, n);
        }
    }
}

#if defined(_M_X64)
namespace {
struct NativeObject {
    void* const* vtable;
    void* game;
};
long __stdcall NativeQueryInterface(NativeObject* self, const void*, void** out) {
    if (!out) {
        return E_POINTER;
    }
    uint32_t addRef = (*static_cast<const Ptr32<const uint32_t>*>(self->game))[1];
    reinterpret_cast<unsigned long(__stdcall*)(void*)>(uintptr_t(addRef))(self->game);
    *out = self;
    return S_OK;
}
} // namespace

void* NativeInterface(void* game, int slots) {
    if (!game) {
        return nullptr;
    }
    static SRWLOCK lock = SRWLOCK_INIT;
    static std::unordered_map<void*, void*> made;
    AcquireSRWLockExclusive(&lock);
    auto found = made.find(game);
    if (found != made.end()) {
        void* native = found->second;
        ReleaseSRWLockExclusive(&lock);
        return native;
    }
    // per slot 16 bytes of code: mov rcx, [rcx+8]; mov rax, <the game's method>; jmp rax
    const uint32_t* methods = *static_cast<const Ptr32<const uint32_t>*>(game);
    size_t codeSize = size_t(slots) * 16;
    auto* block = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, codeSize + size_t(slots) * 8 + sizeof(NativeObject), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!block) {
        ReleaseSRWLockExclusive(&lock);
        return nullptr;
    }
    auto* vtable = reinterpret_cast<void**>(block + codeSize);
    for (int i = 0; i < slots; i++) {
        uint8_t* code = block + size_t(i) * 16;
        uint64_t target = methods[i];
        code[0] = 0x48;
        code[1] = 0x8B;
        code[2] = 0x49;
        code[3] = 0x08;
        code[4] = 0x48;
        code[5] = 0xB8;
        std::memcpy(code + 6, &target, 8);
        code[14] = 0xFF;
        code[15] = 0xE0;
        vtable[i] = code;
    }
    vtable[0] = reinterpret_cast<void*>(&NativeQueryInterface);
    auto* native = reinterpret_cast<NativeObject*>(block + codeSize + size_t(slots) * 8);
    native->vtable = vtable;
    native->game = game;
    DWORD old;
    VirtualProtect(block, codeSize + size_t(slots) * 8 + sizeof(NativeObject), PAGE_EXECUTE_READWRITE, &old);
    FlushInstructionCache(GetCurrentProcess(), block, codeSize);
    made[game] = native;
    ReleaseSRWLockExclusive(&lock);
    return native;
}
#else
void* NativeInterface(void* game, int) {
    return game;
}
#endif

#if defined(_M_X64)
namespace {
SRWLOCK g_overlappedLock = SRWLOCK_INIT;
std::unordered_map<void*, OVERLAPPED*> g_overlapped;
OVERLAPPED* NativeOverlappedOf(void* game) {
    AcquireSRWLockExclusive(&g_overlappedLock);
    OVERLAPPED*& o = g_overlapped[game];
    if (o == nullptr) {
        o = new OVERLAPPED{};
    }
    OVERLAPPED* result = o;
    ReleaseSRWLockExclusive(&g_overlappedLock);
    return result;
}
}
void* SystemOverlapped(void* game) {
    if (game == nullptr) {
        return nullptr;
    }
    OVERLAPPED* o = NativeOverlappedOf(game);
    const uint32_t* g = static_cast<const uint32_t*>(game);
    o->Offset = g[2];
    o->OffsetHigh = g[3];
    o->hEvent = reinterpret_cast<HANDLE>(uintptr_t(g[4]));
    return o;
}
void SyncOverlapped(void* game) {
    if (game == nullptr) {
        return;
    }
    OVERLAPPED* o = NativeOverlappedOf(game);
    uint32_t* g = static_cast<uint32_t*>(game);
    g[0] = uint32_t(o->Internal);
    g[1] = uint32_t(o->InternalHigh);
}
#else
void* SystemOverlapped(void* game) {
    return game;
}
void SyncOverlapped(void*) {}
#endif

void Mismatch(const char* name, const char* format, ...) {
    // the first few of each run: enough to see what's wrong without flooding
    static std::atomic<int> reported{0};
    if (reported.fetch_add(1) >= 200) {
        return;
    }
    char text[768];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    Log("MISMATCH %s: %s", name, text);
}

} // namespace re
