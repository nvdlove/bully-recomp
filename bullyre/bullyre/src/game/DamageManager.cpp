// A vehicle's damage state (GTA's CDamageManager: wheels, doors, lights, panels) and
// its doors' angles (GTA's CDoor).

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace c11damage {

struct CDamageManager {
    float wheelDamageEffect;      // +0x0
    uint8_t engineStatus;         // +0x4
    uint8_t wheelStatus[4];       // +0x5 up to 2
    uint8_t doorStatus[6];        // +0x9 up to 3
    uint8_t unknownF;
    uint32_t lightStatus;         // +0x10 2 bits a light
    uint32_t panelStatus;         // +0x14 4 bits a panel (up to 3)

    // 0x4C7CB0: which group a component is in (0 panels 16+, 1 wheels 1-4, 2 doors,
    // 3 the bonnet (5), 4 (6), 5 lights 11-15, 6 the engine (0)) and its index there
    uint8_t GetComponentGroup(int component, int* group, int8_t* subComponent);
    // 0x4C7D50: everything undamaged (not the wheel effect)
    void ResetDamageStatus();
    // 0x4C7D70 / 0x4C7DA0
    void SetLightStatus(uint8_t light, uint32_t status);
    uint32_t GetLightStatus(uint8_t light);
    // 0x4C7DC0
    uint32_t GetPanelStatus(uint8_t panel);
    // 0x4C7DE0
    uint32_t GetWheelStatus(int wheel);   // (a byte, zero-extended: callers read all of eax)
    // 0x4C7DF0 / 0x4C7E00
    void SetDoorStatus(int door, uint8_t status);
    uint32_t GetDoorStatus(int door);
    // 0x4C7E10 / 0x4C7E30 / 0x4C7E70: one step worse (0: already at the worst)
    uint8_t ProgressDoorDamage(uint8_t door);
    uint8_t ProgressPanelDamage(uint8_t panel);
    uint8_t ProgressWheelDamage(uint8_t wheel);
    // 0x4C7E90: damage over 70 (after the group's multiplier; the last light 0.6 of
    // it) makes the component one step worse
    uint8_t ApplyDamage(int component, float damage, int unused);
    // 0x4C7F70: how many wheels, doors, lights and panels are damaged at all
    int32_t CountDamagedComponents();
};
static_assert(offsetof(CDamageManager, engineStatus) == 0x4);
static_assert(offsetof(CDamageManager, wheelStatus) == 0x5);
static_assert(offsetof(CDamageManager, doorStatus) == 0x9);
static_assert(offsetof(CDamageManager, lightStatus) == 0x10);
static_assert(offsetof(CDamageManager, panelStatus) == 0x14);
static_assert(sizeof(CDamageManager) == 0x18);

struct CDoor {
    float maxAngle;     // +0x0
    float minAngle;     // +0x4
    int8_t dirn;        // +0x8
    int8_t axis;        // +0x9
    uint8_t state;      // +0xA 1: open
    uint8_t unknownB;
    float angle;        // +0xC
    float prevAngle;    // +0x10
    float angVel;       // +0x14

    // 0x4C80B0: opened to the ratio of its open angle (fully: its state open)
    void Open(float ratio);
    // 0x4C8130: the smaller of its angles (in size)
    float RetAngleWhenClosed();
    // 0x4C8160
    uint8_t IsClosed();
};
static_assert(offsetof(CDoor, state) == 0xA);
static_assert(offsetof(CDoor, angle) == 0xC);
static_assert(offsetof(CDoor, angVel) == 0x14);
static_assert(sizeof(CDoor) == 0x18);

} // namespace c11damage

namespace {

using namespace c11damage;

GAME_FN(uint8_t, __thiscall, DM_GetComponentGroup, 0x004C7CB0, CDamageManager* dm, int component, int* group,
        int8_t* subComponent);
GAME_FN(void, __thiscall, DM_SetLightStatus, 0x004C7D70, CDamageManager* dm, uint8_t light, uint32_t status);
GAME_FN(uint8_t, __thiscall, DM_ProgressDoorDamage, 0x004C7E10, CDamageManager* dm, uint8_t door);
GAME_FN(uint8_t, __thiscall, DM_ProgressPanelDamage, 0x004C7E30, CDamageManager* dm, uint8_t panel);
GAME_FN(uint8_t, __thiscall, DM_ProgressWheelDamage, 0x004C7E70, CDamageManager* dm, uint8_t wheel);

// damage multipliers by group
GAME_ARRAY(float, groupDamageMultipliers, 0x00A3577C);

// (x86 shifts by the count's low 5 bits)
uint32_t Shift(uint8_t count) {
    return count & 31u;
}

// (the game: min(status, 1) through floats, then truncated)
int DamagedCount(uint32_t status) {
    float value = float(status);
    if (!(value <= 1.0f)) {
        value = 1.0f;
    }
    return int(value);
}

} // namespace

namespace c11damage {

uint8_t CDamageManager::GetComponentGroup(int component, int* group, int8_t* subComponent) {
    *subComponent = -2;
    if (component >= 11) {
        *group = component >= 16 ? 0 : 5;
        *subComponent = int8_t(component - 11);
        return 1;
    }
    if (component >= 5) {
        if (component == 5) {
            *group = 3;
            *subComponent = 0;
            return 1;
        }
        *group = component == 6 ? 4 : 2;
        *subComponent = int8_t(component - 5);
        return 1;
    }
    if (component >= 1) {
        *group = 1;
        *subComponent = int8_t(component - 1);
        return 1;
    }
    if (component >= 0) {
        *group = 6;
        *subComponent = 0;
        return 1;
    }
    return 0;
}

void CDamageManager::ResetDamageStatus() {
    engineStatus = 0;
    for (uint8_t& status : wheelStatus) {
        status = 0;
    }
    for (uint8_t& status : doorStatus) {
        status = 0;
    }
    lightStatus = 0;
    panelStatus = 0;
}

void CDamageManager::SetLightStatus(uint8_t light, uint32_t status) {
    uint32_t shift = Shift(uint8_t(light * 2));
    lightStatus = (lightStatus & ~(3u << shift)) | (status << shift);
}

uint32_t CDamageManager::GetLightStatus(uint8_t light) {
    return (lightStatus >> Shift(uint8_t(light * 2))) & 3;
}

uint32_t CDamageManager::GetPanelStatus(uint8_t panel) {
    return (panelStatus >> Shift(uint8_t(panel * 4))) & 0xF;
}

uint32_t CDamageManager::GetWheelStatus(int wheel) {
    return wheelStatus[wheel];
}

void CDamageManager::SetDoorStatus(int door, uint8_t status) {
    doorStatus[door] = status;
}

uint32_t CDamageManager::GetDoorStatus(int door) {
    return doorStatus[door];
}

uint8_t CDamageManager::ProgressDoorDamage(uint8_t door) {
    if (doorStatus[door] == 3) {
        return 0;
    }
    doorStatus[door]++;
    return 1;
}

uint8_t CDamageManager::ProgressPanelDamage(uint8_t panel) {
    uint32_t shift = Shift(uint8_t(panel * 4));
    uint8_t status = uint8_t((panelStatus >> shift) & 0xF);
    if (status == 3) {
        return 0;
    }
    panelStatus = (panelStatus & ~(0xFu << shift)) | (uint32_t(uint8_t(status + 1)) << shift);
    return 1;
}

uint8_t CDamageManager::ProgressWheelDamage(uint8_t wheel) {
    if (wheelStatus[wheel] == 2) {
        return 0;
    }
    wheelStatus[wheel]++;
    return 1;
}

uint8_t CDamageManager::ApplyDamage(int component, float damage, int) {
    // (for a negative component the group is never written: its slot holds `this`)
    int group = int(reinterpret_cast<uintptr_t>(this));
    // the index is written over the component's low byte (the rest is the component's)
    int8_t subComponent;
    DM_GetComponentGroup(this, component, &group, &subComponent);
    damage = groupDamageMultipliers[group] * damage;
    if (component == 15) {
        damage = float(damage * 0.6000000238418579);
    }
    if (!(70.0f < damage)) {
        return 0;
    }
    uint8_t sub = uint8_t(subComponent);
    switch (uint32_t(group)) {
    case 0:
        DM_ProgressPanelDamage(this, sub);
        break;
    case 1:
        DM_ProgressWheelDamage(this, sub);
        break;
    case 2:
    case 3:
    case 4:
        DM_ProgressDoorDamage(this, sub);
        break;
    case 5:
        DM_SetLightStatus(this, sub, 1);
        DM_ProgressPanelDamage(this, sub);
        break;
    default:
        break;
    }
    return 1;
}

int32_t CDamageManager::CountDamagedComponents() {
    int32_t count = 0;
    for (int i = 0; i < 4; i++) {
        count += DamagedCount(wheelStatus[i]);
    }
    for (int i = 0; i < 6; i++) {
        count += DamagedCount(doorStatus[i]);
    }
    for (int i = 0; i < 4; i++) {
        count += DamagedCount((lightStatus >> Shift(uint8_t(i * 2))) & 3);
    }
    for (int i = 0; i < 7; i++) {
        count += DamagedCount((panelStatus >> Shift(uint8_t(i * 4))) & 0xF);
    }
    return count;
}

void CDoor::Open(float ratio) {
    prevAngle = angle;
    float open = std::fabs(maxAngle) < std::fabs(minAngle) ? minAngle : maxAngle;
    if (ratio < 1.0f) {
        angle = ratio * open;
        if (angle == 0.0f) {
            angVel = 0.0f;
        }
        return;
    }
    state = 1;
    angle = open;
}

float CDoor::RetAngleWhenClosed() {
    return std::fabs(maxAngle) < std::fabs(minAngle) ? maxAngle : minAngle;
}

uint8_t CDoor::IsClosed() {
    float closed = std::fabs(maxAngle) < std::fabs(minAngle) ? maxAngle : minAngle;
    return angle == closed ? 1 : 0;
}

} // namespace c11damage

// ---- registration ---------------------------------------------------------------------

#define DAMAGE_REVERSED(name, address, member)                                               \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, re::MemberAddress(member), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

DAMAGE_REVERSED("CDamageManager::GetComponentGroup", 0x004C7CB0, &c11damage::CDamageManager::GetComponentGroup);
DAMAGE_REVERSED("CDamageManager::ResetDamageStatus", 0x004C7D50, &c11damage::CDamageManager::ResetDamageStatus);
DAMAGE_REVERSED("CDamageManager::SetLightStatus", 0x004C7D70, &c11damage::CDamageManager::SetLightStatus);
DAMAGE_REVERSED("CDamageManager::GetLightStatus", 0x004C7DA0, &c11damage::CDamageManager::GetLightStatus);
DAMAGE_REVERSED("CDamageManager::GetPanelStatus", 0x004C7DC0, &c11damage::CDamageManager::GetPanelStatus);
DAMAGE_REVERSED("CDamageManager::GetWheelStatus", 0x004C7DE0, &c11damage::CDamageManager::GetWheelStatus);
DAMAGE_REVERSED("CDamageManager::SetDoorStatus", 0x004C7DF0, &c11damage::CDamageManager::SetDoorStatus);
DAMAGE_REVERSED("CDamageManager::GetDoorStatus", 0x004C7E00, &c11damage::CDamageManager::GetDoorStatus);
DAMAGE_REVERSED("CDamageManager::ProgressDoorDamage", 0x004C7E10, &c11damage::CDamageManager::ProgressDoorDamage);
DAMAGE_REVERSED("CDamageManager::ProgressPanelDamage", 0x004C7E30, &c11damage::CDamageManager::ProgressPanelDamage);
DAMAGE_REVERSED("CDamageManager::ProgressWheelDamage", 0x004C7E70, &c11damage::CDamageManager::ProgressWheelDamage);
DAMAGE_REVERSED("CDamageManager::ApplyDamage", 0x004C7E90, &c11damage::CDamageManager::ApplyDamage);
DAMAGE_REVERSED("CDamageManager::CountDamagedComponents", 0x004C7F70, &c11damage::CDamageManager::CountDamagedComponents);
DAMAGE_REVERSED("CDoor::Open", 0x004C80B0, &c11damage::CDoor::Open);
DAMAGE_REVERSED("CDoor::RetAngleWhenClosed", 0x004C8130, &c11damage::CDoor::RetAngleWhenClosed);
DAMAGE_REVERSED("CDoor::IsClosed", 0x004C8160, &c11damage::CDoor::IsClosed);
