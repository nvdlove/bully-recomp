// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8F58D0 (exit)
uint32_t __cdecl Fn8F58D0() {
    return Thiscall(0x0049A650, 0x01B654E0u);  // NameKey_Destruct
}

// 0x8F58E0 (exit)
uint32_t __cdecl Fn8F58E0() {
    return Thiscall(0x0049A650, 0x01B654E4u);  // NameKey_Destruct
}

// 0x8F58F0 (exit)
uint32_t __cdecl Fn8F58F0() {
    return Thiscall(0x0049A650, 0x01B654E8u);  // NameKey_Destruct
}

// 0x8F5900 (exit)
uint32_t __cdecl Fn8F5900() {
    return Thiscall(0x0049A650, 0x01B654ECu);  // NameKey_Destruct
}

// 0x8F5910 (exit)
uint32_t __cdecl Fn8F5910() {
    return Thiscall(0x0049A650, 0x01B654F0u);  // NameKey_Destruct
}

// 0x8F5920 (exit)
uint32_t __cdecl Fn8F5920() {
    return Thiscall(0x0049A650, 0x01B654F4u);  // NameKey_Destruct
}

// 0x8F5930 (exit)
uint32_t __cdecl Fn8F5930() {
    return Thiscall(0x0049A650, 0x01B654F8u);  // NameKey_Destruct
}

// 0x8F5940 (exit)
uint32_t __cdecl Fn8F5940() {
    return Thiscall(0x0049A650, 0x01B654FCu);  // NameKey_Destruct
}

// 0x8F5950 (exit)
uint32_t __cdecl Fn8F5950() {
    return Thiscall(0x0049A650, 0x01B65500u);  // NameKey_Destruct
}

// 0x8F5960 (exit)
uint32_t __cdecl Fn8F5960() {
    return Thiscall(0x0049A650, 0x01B65504u);  // NameKey_Destruct
}

// 0x8F5970 (exit)
uint32_t __cdecl Fn8F5970() {
    return Thiscall(0x0049A650, 0x01B65508u);  // NameKey_Destruct
}

// 0x8F5980 (exit)
uint32_t __cdecl Fn8F5980() {
    return Thiscall(0x0049A650, 0x01B6550Cu);  // NameKey_Destruct
}

// 0x8F5990 (exit)
uint32_t __cdecl Fn8F5990() {
    return Thiscall(0x0049A650, 0x01B65510u);  // NameKey_Destruct
}

// 0x8F59A0 (exit)
uint32_t __cdecl Fn8F59A0() {
    return Thiscall(0x0049A650, 0x01B65514u);  // NameKey_Destruct
}

// 0x8F59B0 (exit)
uint32_t __cdecl Fn8F59B0() {
    return Thiscall(0x0049A650, 0x01B65518u);  // NameKey_Destruct
}

// 0x8F59C0 (exit)
uint32_t __cdecl Fn8F59C0() {
    return Thiscall(0x0049A650, 0x01B6551Cu);  // NameKey_Destruct
}

// 0x8F59D0 (exit)
uint32_t __cdecl Fn8F59D0() {
    return Thiscall(0x0049A650, 0x01B65520u);  // NameKey_Destruct
}

// 0x8F59E0 (exit)
uint32_t __cdecl Fn8F59E0() {
    return Thiscall(0x0049A650, 0x01B65524u);  // NameKey_Destruct
}

// 0x8F59F0 (exit)
uint32_t __cdecl Fn8F59F0() {
    return Thiscall(0x0049A650, 0x01B65528u);  // NameKey_Destruct
}

// 0x8F5A00 (exit)
uint32_t __cdecl Fn8F5A00() {
    return Thiscall(0x0049A650, 0x01B6552Cu);  // NameKey_Destruct
}

// 0x8F5A10 (exit)
uint32_t __cdecl Fn8F5A10() {
    return Thiscall(0x0049A650, 0x01B65530u);  // NameKey_Destruct
}

// 0x8F5A20 (exit)
uint32_t __cdecl Fn8F5A20() {
    return Thiscall(0x0049A650, 0x01B65534u);  // NameKey_Destruct
}

// 0x8F5A30 (exit)
uint32_t __cdecl Fn8F5A30() {
    return Thiscall(0x0049A650, 0x01B65538u);  // NameKey_Destruct
}

// 0x8F5A40 (exit)
uint32_t __cdecl Fn8F5A40() {
    return Thiscall(0x0049A650, 0x01B6553Cu);  // NameKey_Destruct
}

// 0x8F5A50 (exit)
uint32_t __cdecl Fn8F5A50() {
    return Thiscall(0x0049A650, 0x01B65540u);  // NameKey_Destruct
}

// 0x8F5A60 (exit)
uint32_t __cdecl Fn8F5A60() {
    return Thiscall(0x0049A650, 0x01B65544u);  // NameKey_Destruct
}

// 0x8F5A70 (exit)
uint32_t __cdecl Fn8F5A70() {
    return Thiscall(0x0049A650, 0x01B65548u);  // NameKey_Destruct
}

// 0x8F5A80 (exit)
uint32_t __cdecl Fn8F5A80() {
    return Thiscall(0x0049A650, 0x01B6554Cu);  // NameKey_Destruct
}

// 0x8F5A90 (exit)
uint32_t __cdecl Fn8F5A90() {
    return Thiscall(0x0049A650, 0x01B65550u);  // NameKey_Destruct
}

// 0x8F5AA0 (exit)
uint32_t __cdecl Fn8F5AA0() {
    return Thiscall(0x0049A650, 0x01B65554u);  // NameKey_Destruct
}

// 0x8F5AB0 (exit)
uint32_t __cdecl Fn8F5AB0() {
    return Thiscall(0x0049A650, 0x01B65558u);  // NameKey_Destruct
}

// 0x8F5AC0 (exit)
uint32_t __cdecl Fn8F5AC0() {
    return Thiscall(0x0049A650, 0x01B6555Cu);  // NameKey_Destruct
}

// 0x8F5AD0 (exit)
uint32_t __cdecl Fn8F5AD0() {
    return Thiscall(0x0049A650, 0x01B65560u);  // NameKey_Destruct
}

// 0x8F5AE0 (exit)
uint32_t __cdecl Fn8F5AE0() {
    return Thiscall(0x0049A650, 0x01B65564u);  // NameKey_Destruct
}

// 0x8F5AF0 (exit)
uint32_t __cdecl Fn8F5AF0() {
    return Thiscall(0x0049A650, 0x01B65580u);  // NameKey_Destruct
}

// 0x8F5B00 (exit)
uint32_t __cdecl Fn8F5B00() {
    return Thiscall(0x0049A650, 0x01B65584u);  // NameKey_Destruct
}

// 0x8F5B10 (exit)
uint32_t __cdecl Fn8F5B10() {
    return Thiscall(0x0049A650, 0x01B65588u);  // NameKey_Destruct
}

// 0x8F5B20 (exit)
uint32_t __cdecl Fn8F5B20() {
    return Thiscall(0x0049A650, 0x01B6558Cu);  // NameKey_Destruct
}

// 0x8F5B30 (exit)
uint32_t __cdecl Fn8F5B30() {
    return Thiscall(0x0049A650, 0x01B65590u);  // NameKey_Destruct
}

// 0x8F5B40 (exit)
uint32_t __cdecl Fn8F5B40() {
    return Thiscall(0x0049A650, 0x01B65594u);  // NameKey_Destruct
}

// 0x8F5B50 (exit)
uint32_t __cdecl Fn8F5B50() {
    return Thiscall(0x0049A650, 0x01B65598u);  // NameKey_Destruct
}

// 0x8F5B60 (exit)
uint32_t __cdecl Fn8F5B60() {
    return Thiscall(0x0049A650, 0x01B6559Cu);  // NameKey_Destruct
}

// 0x8F5B70 (exit)
uint32_t __cdecl Fn8F5B70() {
    return Thiscall(0x0049A650, 0x01B655A0u);  // NameKey_Destruct
}

// 0x8F5B80 (exit)
uint32_t __cdecl Fn8F5B80() {
    return Thiscall(0x0049A650, 0x01B655A4u);  // NameKey_Destruct
}

// 0x8F5B90 (exit)
uint32_t __cdecl Fn8F5B90() {
    return Thiscall(0x0049A650, 0x01B655A8u);  // NameKey_Destruct
}

// 0x8F5BA0 (exit)
uint32_t __cdecl Fn8F5BA0() {
    return Thiscall(0x0049A650, 0x01B655ACu);  // NameKey_Destruct
}

// 0x8F5BB0 (exit)
uint32_t __cdecl Fn8F5BB0() {
    return Thiscall(0x0049A650, 0x01B655B0u);  // NameKey_Destruct
}

// 0x8F5BC0 (exit)
uint32_t __cdecl Fn8F5BC0() {
    return Thiscall(0x0049A650, 0x01B655B4u);  // NameKey_Destruct
}

// 0x8F5BD0 (exit)
uint32_t __cdecl Fn8F5BD0() {
    return Thiscall(0x0049A650, 0x01B655B8u);  // NameKey_Destruct
}

// 0x8F5BE0 (exit)
uint32_t __cdecl Fn8F5BE0() {
    return Thiscall(0x0049A650, 0x01B655BCu);  // NameKey_Destruct
}

// 0x8F5BF0 (exit)
uint32_t __cdecl Fn8F5BF0() {
    return Thiscall(0x0049A650, 0x01B655C0u);  // NameKey_Destruct
}

// 0x8F5C00 (exit)
uint32_t __cdecl Fn8F5C00() {
    return Thiscall(0x0049A650, 0x01B655C4u);  // NameKey_Destruct
}

// 0x8F5C10 (exit)
uint32_t __cdecl Fn8F5C10() {
    return Thiscall(0x0049A650, 0x01B655C8u);  // NameKey_Destruct
}

// 0x8F5C20 (exit)
uint32_t __cdecl Fn8F5C20() {
    return Thiscall(0x0049A650, 0x01B655CCu);  // NameKey_Destruct
}

// 0x8F5C30 (exit)
uint32_t __cdecl Fn8F5C30() {
    return Thiscall(0x0049A650, 0x01B65620u);  // NameKey_Destruct
}

// 0x8F5C40 (exit)
uint32_t __cdecl Fn8F5C40() {
    return Thiscall(0x0049A650, 0x01B65624u);  // NameKey_Destruct
}

// 0x8F5C50 (exit)
uint32_t __cdecl Fn8F5C50() {
    return Thiscall(0x0049A650, 0x01B65628u);  // NameKey_Destruct
}

// 0x8F5C60 (exit)
uint32_t __cdecl Fn8F5C60() {
    return Thiscall(0x0049A650, 0x01B6562Cu);  // NameKey_Destruct
}

// 0x8F5C70 (exit)
uint32_t __cdecl Fn8F5C70() {
    return Thiscall(0x0049A650, 0x01B65630u);  // NameKey_Destruct
}

// 0x8F5C80 (exit)
uint32_t __cdecl Fn8F5C80() {
    return Thiscall(0x0049A650, 0x01B65634u);  // NameKey_Destruct
}

// 0x8F5C90 (exit)
uint32_t __cdecl Fn8F5C90() {
    return Thiscall(0x0049A650, 0x01B65638u);  // NameKey_Destruct
}

// 0x8F5CA0 (exit)
uint32_t __cdecl Fn8F5CA0() {
    return Thiscall(0x0049A650, 0x01B6563Cu);  // NameKey_Destruct
}

// 0x8F5CB0 (exit)
uint32_t __cdecl Fn8F5CB0() {
    return Thiscall(0x0049A650, 0x01B65640u);  // NameKey_Destruct
}

// 0x8F5CC0 (exit)
uint32_t __cdecl Fn8F5CC0() {
    return Thiscall(0x0049A650, 0x01B65644u);  // NameKey_Destruct
}

// 0x8F5CD0 (exit)
uint32_t __cdecl Fn8F5CD0() {
    return Thiscall(0x0049A650, 0x01B65648u);  // NameKey_Destruct
}

// 0x8F5D10 (exit)
uint32_t __cdecl Fn8F5D10() {
    return Thiscall(0x0061A800, 0x01B6565Cu);  // TrackRegistry::Destruct
}

// 0x8F5D20 (exit)
uint32_t __cdecl Fn8F5D20() {
    return Thiscall(0x0049A650, 0x01B6566Cu);  // NameKey_Destruct
}

// 0x8F5D30 (exit)
uint32_t __cdecl Fn8F5D30() {
    return Thiscall(0x0049A650, 0x01B6573Cu);  // NameKey_Destruct
}

// 0x8F5D50 (exit)
uint32_t __cdecl Fn8F5D50() {
    return Thiscall(0x0049A650, 0x01B65740u);  // NameKey_Destruct
}

// 0x8F5D60 (exit)
uint32_t __cdecl Fn8F5D60() {
    return Thiscall(0x0049A650, 0x01B65744u);  // NameKey_Destruct
}

// 0x8F5D70 (exit)
uint32_t __cdecl Fn8F5D70() {
    return Thiscall(0x0049A650, 0x01B65748u);  // NameKey_Destruct
}

// 0x8F5D80 (exit)
uint32_t __cdecl Fn8F5D80() {
    return Thiscall(0x0049A650, 0x01B6574Cu);  // NameKey_Destruct
}

// 0x8F5D90 (exit)
uint32_t __cdecl Fn8F5D90() {
    return Thiscall(0x0049A650, 0x01B65750u);  // NameKey_Destruct
}

// 0x8F5DA0 (exit)
uint32_t __cdecl Fn8F5DA0() {
    return Thiscall(0x0049A650, 0x01B65754u);  // NameKey_Destruct
}

// 0x8F5DB0 (exit)
uint32_t __cdecl Fn8F5DB0() {
    return Thiscall(0x0049A650, 0x01B65758u);  // NameKey_Destruct
}

// 0x8F5DC0 (exit)
uint32_t __cdecl Fn8F5DC0() {
    return Thiscall(0x0049A650, 0x01B6575Cu);  // NameKey_Destruct
}

// 0x8F5DD0 (exit)
uint32_t __cdecl Fn8F5DD0() {
    return Thiscall(0x0049A650, 0x01B65760u);  // NameKey_Destruct
}

// 0x8F5DE0 (exit)
uint32_t __cdecl Fn8F5DE0() {
    return Thiscall(0x0049A650, 0x01B65764u);  // NameKey_Destruct
}

// 0x8F5DF0 (exit)
uint32_t __cdecl Fn8F5DF0() {
    return Thiscall(0x0049A650, 0x01B65768u);  // NameKey_Destruct
}

// 0x8F5E00 (exit)
uint32_t __cdecl Fn8F5E00() {
    return Thiscall(0x0049A650, 0x01B6576Cu);  // NameKey_Destruct
}

// 0x8F5E10 (exit)
uint32_t __cdecl Fn8F5E10() {
    return Thiscall(0x0049A650, 0x01B65770u);  // NameKey_Destruct
}

// 0x8F5E20 (exit)
uint32_t __cdecl Fn8F5E20() {
    return Thiscall(0x0049A650, 0x01B65774u);  // NameKey_Destruct
}

// 0x8F5E30 (exit)
uint32_t __cdecl Fn8F5E30() {
    return Thiscall(0x0049A650, 0x01B65778u);  // NameKey_Destruct
}

// 0x8F5E40 (exit)
uint32_t __cdecl Fn8F5E40() {
    return Thiscall(0x0049A650, 0x01B6577Cu);  // NameKey_Destruct
}

// 0x8F5E50 (exit)
uint32_t __cdecl Fn8F5E50() {
    return Thiscall(0x0049A650, 0x01B65780u);  // NameKey_Destruct
}

// 0x8F5E60 (exit)
uint32_t __cdecl Fn8F5E60() {
    return Thiscall(0x0049A650, 0x01B65784u);  // NameKey_Destruct
}

// 0x8F5E70 (exit)
uint32_t __cdecl Fn8F5E70() {
    return Thiscall(0x0049A650, 0x01B65788u);  // NameKey_Destruct
}

// 0x8F5E80 (exit)
uint32_t __cdecl Fn8F5E80() {
    return Thiscall(0x0049A650, 0x01B6578Cu);  // NameKey_Destruct
}

// 0x8F5E90 (exit)
uint32_t __cdecl Fn8F5E90() {
    return Thiscall(0x0049A650, 0x01B65790u);  // NameKey_Destruct
}

// 0x8F5EA0 (exit)
uint32_t __cdecl Fn8F5EA0() {
    return Thiscall(0x0049A650, 0x01B65794u);  // NameKey_Destruct
}

// 0x8F5EB0 (exit)
uint32_t __cdecl Fn8F5EB0() {
    return Thiscall(0x0049A650, 0x01B65798u);  // NameKey_Destruct
}

// 0x8F5EC0 (exit)
uint32_t __cdecl Fn8F5EC0() {
    return Thiscall(0x0049A650, 0x01B6579Cu);  // NameKey_Destruct
}

// 0x8F5ED0 (exit)
uint32_t __cdecl Fn8F5ED0() {
    return Thiscall(0x0049A650, 0x01B657A0u);  // NameKey_Destruct
}

// 0x8F5EE0 (exit)
uint32_t __cdecl Fn8F5EE0() {
    return Thiscall(0x0049A650, 0x01B657A4u);  // NameKey_Destruct
}

// 0x8F5EF0 (exit)
uint32_t __cdecl Fn8F5EF0() {
    return Thiscall(0x0049A650, 0x01B657A8u);  // NameKey_Destruct
}

// 0x8F5F00 (exit)
uint32_t __cdecl Fn8F5F00() {
    return Thiscall(0x0049A650, 0x01B657ACu);  // NameKey_Destruct
}

// 0x8F5F10 (exit)
uint32_t __cdecl Fn8F5F10() {
    return Thiscall(0x0049A650, 0x01B657B0u);  // NameKey_Destruct
}

// 0x8F5F20 (exit)
uint32_t __cdecl Fn8F5F20() {
    return Thiscall(0x0049A650, 0x01B657B4u);  // NameKey_Destruct
}

// 0x8F5F30 (exit)
uint32_t __cdecl Fn8F5F30() {
    return Thiscall(0x0049A650, 0x01B657B8u);  // NameKey_Destruct
}

// 0x8F5F40 (exit)
uint32_t __cdecl Fn8F5F40() {
    return Thiscall(0x0049A650, 0x01B657BCu);  // NameKey_Destruct
}

// 0x8F5F50 (exit)
uint32_t __cdecl Fn8F5F50() {
    return Thiscall(0x0049A650, 0x01B657C0u);  // NameKey_Destruct
}

// 0x8F5F60 (exit)
uint32_t __cdecl Fn8F5F60() {
    return Thiscall(0x0049A650, 0x01B657C4u);  // NameKey_Destruct
}

// 0x8F5F70 (exit)
uint32_t __cdecl Fn8F5F70() {
    return Thiscall(0x0049A650, 0x01B657C8u);  // NameKey_Destruct
}

// 0x8F5F80 (exit)
uint32_t __cdecl Fn8F5F80() {
    return Thiscall(0x0049A650, 0x01B657CCu);  // NameKey_Destruct
}

// 0x8F5F90 (exit)
uint32_t __cdecl Fn8F5F90() {
    return Thiscall(0x0049A650, 0x01B657D0u);  // NameKey_Destruct
}

// 0x8F5FA0 (exit)
uint32_t __cdecl Fn8F5FA0() {
    return Thiscall(0x0049A650, 0x01B657D4u);  // NameKey_Destruct
}

// 0x8F5FB0 (exit)
uint32_t __cdecl Fn8F5FB0() {
    return Thiscall(0x0049A650, 0x01B657D8u);  // NameKey_Destruct
}

// 0x8F5FC0 (exit)
uint32_t __cdecl Fn8F5FC0() {
    return Thiscall(0x0049A650, 0x01B657DCu);  // NameKey_Destruct
}

// 0x8F5FD0 (exit)
uint32_t __cdecl Fn8F5FD0() {
    return Thiscall(0x0049A650, 0x01B657E0u);  // NameKey_Destruct
}

// 0x8F5FE0 (exit)
uint32_t __cdecl Fn8F5FE0() {
    return Thiscall(0x0049A650, 0x01B657E4u);  // NameKey_Destruct
}

// 0x8F5FF0 (exit)
uint32_t __cdecl Fn8F5FF0() {
    return Thiscall(0x0049A650, 0x01B657E8u);  // NameKey_Destruct
}

// 0x8F6000 (exit)
uint32_t __cdecl Fn8F6000() {
    return Thiscall(0x0049A650, 0x01B657ECu);  // NameKey_Destruct
}

// 0x8F6010 (exit)
uint32_t __cdecl Fn8F6010() {
    return Thiscall(0x0049A650, 0x01B657F0u);  // NameKey_Destruct
}

// 0x8F6020 (exit)
uint32_t __cdecl Fn8F6020() {
    return Thiscall(0x0049A650, 0x01B657F4u);  // NameKey_Destruct
}

// 0x8F6030 (exit)
uint32_t __cdecl Fn8F6030() {
    return Thiscall(0x0049A650, 0x01B657F8u);  // NameKey_Destruct
}

// 0x8F6040 (exit)
uint32_t __cdecl Fn8F6040() {
    return Thiscall(0x0049A650, 0x01B657FCu);  // NameKey_Destruct
}

// 0x8F6050 (exit)
uint32_t __cdecl Fn8F6050() {
    return Thiscall(0x0049A650, 0x01B65800u);  // NameKey_Destruct
}

// 0x8F6060 (exit)
uint32_t __cdecl Fn8F6060() {
    return Thiscall(0x0049A650, 0x01B65804u);  // NameKey_Destruct
}

// 0x8F6070 (exit)
uint32_t __cdecl Fn8F6070() {
    return Thiscall(0x0049A650, 0x01B65808u);  // NameKey_Destruct
}

// 0x8F6080 (exit)
uint32_t __cdecl Fn8F6080() {
    return Thiscall(0x0049A650, 0x01B6580Cu);  // NameKey_Destruct
}

// 0x8F6090 (exit)
uint32_t __cdecl Fn8F6090() {
    return Thiscall(0x0049A650, 0x01B65810u);  // NameKey_Destruct
}

// 0x8F60A0 (exit)
uint32_t __cdecl Fn8F60A0() {
    return Thiscall(0x0049A650, 0x01B65814u);  // NameKey_Destruct
}

// 0x8F60B0 (exit)
uint32_t __cdecl Fn8F60B0() {
    return Thiscall(0x0049A650, 0x01B65818u);  // NameKey_Destruct
}

// 0x8F60C0 (exit)
uint32_t __cdecl Fn8F60C0() {
    return Thiscall(0x0049A650, 0x01B6581Cu);  // NameKey_Destruct
}

// 0x8F60D0 (exit)
uint32_t __cdecl Fn8F60D0() {
    return Thiscall(0x0049A650, 0x01B65820u);  // NameKey_Destruct
}

// 0x8F60E0 (exit)
uint32_t __cdecl Fn8F60E0() {
    return Thiscall(0x0049A650, 0x01B65824u);  // NameKey_Destruct
}

// 0x8F60F0 (exit)
uint32_t __cdecl Fn8F60F0() {
    return Thiscall(0x0049A650, 0x01B65828u);  // NameKey_Destruct
}

// 0x8F6100 (exit)
uint32_t __cdecl Fn8F6100() {
    return Thiscall(0x0049A650, 0x01B6582Cu);  // NameKey_Destruct
}

// 0x8F6110 (exit)
uint32_t __cdecl Fn8F6110() {
    return Thiscall(0x0049A650, 0x01B65830u);  // NameKey_Destruct
}

// 0x8F6120 (exit)
uint32_t __cdecl Fn8F6120() {
    return Thiscall(0x0049A650, 0x01B65834u);  // NameKey_Destruct
}

// 0x8F6130 (exit)
uint32_t __cdecl Fn8F6130() {
    return Thiscall(0x0049A650, 0x01B65838u);  // NameKey_Destruct
}

// 0x8F6140 (exit)
uint32_t __cdecl Fn8F6140() {
    return Thiscall(0x0049A650, 0x01B6583Cu);  // NameKey_Destruct
}

// 0x8F6150 (exit)
uint32_t __cdecl Fn8F6150() {
    return Thiscall(0x0049A650, 0x01B65840u);  // NameKey_Destruct
}

// 0x8F6160 (exit)
uint32_t __cdecl Fn8F6160() {
    return Thiscall(0x0049A650, 0x01B65844u);  // NameKey_Destruct
}

// 0x8F6170 (exit)
uint32_t __cdecl Fn8F6170() {
    return Thiscall(0x0049A650, 0x01B65848u);  // NameKey_Destruct
}

// 0x8F6180 (exit)
uint32_t __cdecl Fn8F6180() {
    return Thiscall(0x0049A650, 0x01B6584Cu);  // NameKey_Destruct
}

// 0x8F6190 (exit)
uint32_t __cdecl Fn8F6190() {
    return Thiscall(0x0049A650, 0x01B65850u);  // NameKey_Destruct
}

// 0x8F61A0 (exit)
uint32_t __cdecl Fn8F61A0() {
    return Thiscall(0x0049A650, 0x01B65854u);  // NameKey_Destruct
}

// 0x8F61B0 (exit)
uint32_t __cdecl Fn8F61B0() {
    return Thiscall(0x0049A650, 0x01B65858u);  // NameKey_Destruct
}

// 0x8F61C0 (exit)
uint32_t __cdecl Fn8F61C0() {
    return Thiscall(0x0049A650, 0x01B6585Cu);  // NameKey_Destruct
}

// 0x8F61D0 (exit)
uint32_t __cdecl Fn8F61D0() {
    return Thiscall(0x0049A650, 0x01B65860u);  // NameKey_Destruct
}

// 0x8F61E0 (exit)
uint32_t __cdecl Fn8F61E0() {
    return Thiscall(0x0049A650, 0x01B65864u);  // NameKey_Destruct
}

// 0x8F61F0 (exit)
uint32_t __cdecl Fn8F61F0() {
    return Thiscall(0x0049A650, 0x01B65868u);  // NameKey_Destruct
}

// 0x8F6200 (exit)
uint32_t __cdecl Fn8F6200() {
    return Thiscall(0x0049A650, 0x01B6586Cu);  // NameKey_Destruct
}

// 0x8F6210 (exit)
uint32_t __cdecl Fn8F6210() {
    return Thiscall(0x0049A650, 0x01B65870u);  // NameKey_Destruct
}

// 0x8F6220 (exit)
uint32_t __cdecl Fn8F6220() {
    return Thiscall(0x0049A650, 0x01B65874u);  // NameKey_Destruct
}

// 0x8F6230 (exit)
uint32_t __cdecl Fn8F6230() {
    return Thiscall(0x0049A650, 0x01B65878u);  // NameKey_Destruct
}

// 0x8F6240 (exit)
uint32_t __cdecl Fn8F6240() {
    return Thiscall(0x0049A650, 0x01B6587Cu);  // NameKey_Destruct
}

// 0x8F6250 (exit)
uint32_t __cdecl Fn8F6250() {
    return Thiscall(0x0049A650, 0x01B65880u);  // NameKey_Destruct
}

// 0x8F6260 (exit)
uint32_t __cdecl Fn8F6260() {
    return Thiscall(0x0049A650, 0x01B65884u);  // NameKey_Destruct
}

// 0x8F6270 (exit)
uint32_t __cdecl Fn8F6270() {
    return Thiscall(0x0049A650, 0x01B65888u);  // NameKey_Destruct
}

// 0x8F6280 (exit)
uint32_t __cdecl Fn8F6280() {
    return Thiscall(0x0049A650, 0x01B6588Cu);  // NameKey_Destruct
}

// 0x8F62A0 (exit)
uint32_t __cdecl Fn8F62A0() {
    return Thiscall(0x0049A650, 0x01B65890u);  // NameKey_Destruct
}

// 0x8F62B0 (exit)
uint32_t __cdecl Fn8F62B0() {
    return Thiscall(0x0049A650, 0x01B65894u);  // NameKey_Destruct
}

// 0x8F62C0 (exit)
uint32_t __cdecl Fn8F62C0() {
    return Thiscall(0x0049A650, 0x01B65898u);  // NameKey_Destruct
}

// 0x8F62D0 (exit)
uint32_t __cdecl Fn8F62D0() {
    return Thiscall(0x0049A650, 0x01B6589Cu);  // NameKey_Destruct
}

// 0x8F62E0 (exit)
uint32_t __cdecl Fn8F62E0() {
    return Thiscall(0x0049A650, 0x01B658A0u);  // NameKey_Destruct
}

// 0x8F62F0 (exit)
uint32_t __cdecl Fn8F62F0() {
    return Thiscall(0x0049A650, 0x01B658A4u);  // NameKey_Destruct
}

// 0x8F6300 (exit)
uint32_t __cdecl Fn8F6300() {
    return Thiscall(0x0049A650, 0x01B658A8u);  // NameKey_Destruct
}

// 0x8F6310 (exit)
uint32_t __cdecl Fn8F6310() {
    return Thiscall(0x0049A650, 0x01B658ACu);  // NameKey_Destruct
}

// 0x8F6320 (exit)
uint32_t __cdecl Fn8F6320() {
    return Thiscall(0x0049A650, 0x01B658B0u);  // NameKey_Destruct
}

// 0x8F6330 (exit)
uint32_t __cdecl Fn8F6330() {
    return Thiscall(0x0049A650, 0x01B658B4u);  // NameKey_Destruct
}

// 0x8F6340 (exit)
uint32_t __cdecl Fn8F6340() {
    return Thiscall(0x0049A650, 0x01B658B8u);  // NameKey_Destruct
}

// 0x8F6350 (exit)
uint32_t __cdecl Fn8F6350() {
    return Thiscall(0x0049A650, 0x01B658BCu);  // NameKey_Destruct
}

// 0x8F6360 (exit)
uint32_t __cdecl Fn8F6360() {
    return Thiscall(0x0049A650, 0x01B658C0u);  // NameKey_Destruct
}

// 0x8F6370 (exit)
uint32_t __cdecl Fn8F6370() {
    return Thiscall(0x0049A650, 0x01B658C4u);  // NameKey_Destruct
}

// 0x8F6380 (exit)
uint32_t __cdecl Fn8F6380() {
    return Thiscall(0x0049A650, 0x01B658C8u);  // NameKey_Destruct
}

// 0x8F6390 (exit)
uint32_t __cdecl Fn8F6390() {
    return Thiscall(0x0049A650, 0x01B658CCu);  // NameKey_Destruct
}

// 0x8F63A0 (exit)
uint32_t __cdecl Fn8F63A0() {
    return Thiscall(0x0049A650, 0x01B658D0u);  // NameKey_Destruct
}

// 0x8F63B0 (exit)
uint32_t __cdecl Fn8F63B0() {
    return Thiscall(0x0049A650, 0x01B658D4u);  // NameKey_Destruct
}

// 0x8F63C0 (exit)
uint32_t __cdecl Fn8F63C0() {
    return Thiscall(0x0049A650, 0x01B658D8u);  // NameKey_Destruct
}

// 0x8F63D0 (exit)
uint32_t __cdecl Fn8F63D0() {
    return Thiscall(0x0049A650, 0x01B658DCu);  // NameKey_Destruct
}

// 0x8F63E0 (exit)
uint32_t __cdecl Fn8F63E0() {
    return Thiscall(0x0049A650, 0x01B658E0u);  // NameKey_Destruct
}

// 0x8F63F0 (exit)
uint32_t __cdecl Fn8F63F0() {
    return Thiscall(0x0049A650, 0x01B658E4u);  // NameKey_Destruct
}

// 0x8F6400 (exit)
uint32_t __cdecl Fn8F6400() {
    return Thiscall(0x0049A650, 0x01B658E8u);  // NameKey_Destruct
}

// 0x8F6410 (exit)
uint32_t __cdecl Fn8F6410() {
    return Thiscall(0x0049A650, 0x01B658ECu);  // NameKey_Destruct
}

// 0x8F6420 (exit)
uint32_t __cdecl Fn8F6420() {
    return Thiscall(0x0049A650, 0x01B658F0u);  // NameKey_Destruct
}

// 0x8F6430 (exit)
uint32_t __cdecl Fn8F6430() {
    return Thiscall(0x0049A650, 0x01B658F4u);  // NameKey_Destruct
}

// 0x8F6440 (exit)
uint32_t __cdecl Fn8F6440() {
    return Thiscall(0x0049A650, 0x01B658F8u);  // NameKey_Destruct
}

// 0x8F6450 (exit)
uint32_t __cdecl Fn8F6450() {
    return Thiscall(0x0049A650, 0x01B658FCu);  // NameKey_Destruct
}

// 0x8F6460 (exit)
uint32_t __cdecl Fn8F6460() {
    return Thiscall(0x0049A650, 0x01B65900u);  // NameKey_Destruct
}

// 0x8F6470 (exit)
uint32_t __cdecl Fn8F6470() {
    return Thiscall(0x0049A650, 0x01B65904u);  // NameKey_Destruct
}

// 0x8F6480 (exit)
uint32_t __cdecl Fn8F6480() {
    return Thiscall(0x0049A650, 0x01B65908u);  // NameKey_Destruct
}

// 0x8F6490 (exit)
uint32_t __cdecl Fn8F6490() {
    return Thiscall(0x0049A650, 0x01B6590Cu);  // NameKey_Destruct
}

// 0x8F64A0 (exit)
uint32_t __cdecl Fn8F64A0() {
    return Thiscall(0x0049A650, 0x01B65910u);  // NameKey_Destruct
}

// 0x8F64B0 (exit)
uint32_t __cdecl Fn8F64B0() {
    return Thiscall(0x0049A650, 0x01B65914u);  // NameKey_Destruct
}

// 0x8F64C0 (exit)
uint32_t __cdecl Fn8F64C0() {
    return Thiscall(0x0049A650, 0x01B65918u);  // NameKey_Destruct
}

// 0x8F64D0 (exit)
uint32_t __cdecl Fn8F64D0() {
    return Thiscall(0x0049A650, 0x01B6591Cu);  // NameKey_Destruct
}

// 0x8F64E0 (exit)
uint32_t __cdecl Fn8F64E0() {
    return Thiscall(0x0049A650, 0x01B65920u);  // NameKey_Destruct
}

// 0x8F64F0 (exit)
uint32_t __cdecl Fn8F64F0() {
    return Thiscall(0x0049A650, 0x01B65924u);  // NameKey_Destruct
}

// 0x8F6500 (exit)
uint32_t __cdecl Fn8F6500() {
    return Thiscall(0x0049A650, 0x01B65928u);  // NameKey_Destruct
}

// 0x8F6510 (exit)
uint32_t __cdecl Fn8F6510() {
    return Thiscall(0x0049A650, 0x01B6592Cu);  // NameKey_Destruct
}

// 0x8F6520 (exit)
uint32_t __cdecl Fn8F6520() {
    return Thiscall(0x0049A650, 0x01B65930u);  // NameKey_Destruct
}

// 0x8F6530 (exit)
uint32_t __cdecl Fn8F6530() {
    return Thiscall(0x0049A650, 0x01B65934u);  // NameKey_Destruct
}

// 0x8F6540 (exit)
uint32_t __cdecl Fn8F6540() {
    return Thiscall(0x0049A650, 0x01B65938u);  // NameKey_Destruct
}

// 0x8F6550 (exit)
uint32_t __cdecl Fn8F6550() {
    return Thiscall(0x0049A650, 0x01B6593Cu);  // NameKey_Destruct
}

// 0x8F6560 (exit)
uint32_t __cdecl Fn8F6560() {
    return Thiscall(0x0049A650, 0x01B65940u);  // NameKey_Destruct
}

// 0x8F6570 (exit)
uint32_t __cdecl Fn8F6570() {
    return Thiscall(0x0049A650, 0x01B65944u);  // NameKey_Destruct
}

// 0x8F6580 (exit)
uint32_t __cdecl Fn8F6580() {
    return Thiscall(0x0049A650, 0x01B65948u);  // NameKey_Destruct
}

// 0x8F6590 (exit)
uint32_t __cdecl Fn8F6590() {
    return Thiscall(0x0049A650, 0x01B6594Cu);  // NameKey_Destruct
}

// 0x8F65A0 (exit)
uint32_t __cdecl Fn8F65A0() {
    return Thiscall(0x0049A650, 0x01B65950u);  // NameKey_Destruct
}

// 0x8F65B0 (exit)
uint32_t __cdecl Fn8F65B0() {
    return Thiscall(0x0049A650, 0x01B65954u);  // NameKey_Destruct
}

// 0x8F65C0 (exit)
uint32_t __cdecl Fn8F65C0() {
    return Thiscall(0x0049A650, 0x01B65958u);  // NameKey_Destruct
}

// 0x8F65D0 (exit)
uint32_t __cdecl Fn8F65D0() {
    return Thiscall(0x0049A650, 0x01B6595Cu);  // NameKey_Destruct
}

// 0x8F65E0 (exit)
uint32_t __cdecl Fn8F65E0() {
    return Thiscall(0x0049A650, 0x01B65960u);  // NameKey_Destruct
}

// 0x8F65F0 (exit)
uint32_t __cdecl Fn8F65F0() {
    return Thiscall(0x0049A650, 0x01B65964u);  // NameKey_Destruct
}

// 0x8F6600 (exit)
uint32_t __cdecl Fn8F6600() {
    return Thiscall(0x0049A650, 0x01B65968u);  // NameKey_Destruct
}

// 0x8F6610 (exit)
uint32_t __cdecl Fn8F6610() {
    return Thiscall(0x0049A650, 0x01B6596Cu);  // NameKey_Destruct
}

// 0x8F6620 (exit)
uint32_t __cdecl Fn8F6620() {
    return Thiscall(0x0049A650, 0x01B65970u);  // NameKey_Destruct
}

// 0x8F6630 (exit)
uint32_t __cdecl Fn8F6630() {
    return Thiscall(0x0049A650, 0x01B65974u);  // NameKey_Destruct
}

// 0x8F6640 (exit)
uint32_t __cdecl Fn8F6640() {
    return Thiscall(0x0049A650, 0x01B65978u);  // NameKey_Destruct
}

// 0x8F6650 (exit)
uint32_t __cdecl Fn8F6650() {
    return Thiscall(0x0049A650, 0x01B6597Cu);  // NameKey_Destruct
}

// 0x8F6660 (exit)
uint32_t __cdecl Fn8F6660() {
    return Thiscall(0x0049A650, 0x01B65980u);  // NameKey_Destruct
}

// 0x8F6670 (exit)
uint32_t __cdecl Fn8F6670() {
    return Thiscall(0x0049A650, 0x01B65984u);  // NameKey_Destruct
}

// 0x8F6680 (exit)
uint32_t __cdecl Fn8F6680() {
    return Thiscall(0x0049A650, 0x01B65988u);  // NameKey_Destruct
}

// 0x8F6690 (exit)
uint32_t __cdecl Fn8F6690() {
    return Thiscall(0x0049A650, 0x01B6598Cu);  // NameKey_Destruct
}

// 0x8F66A0 (exit)
uint32_t __cdecl Fn8F66A0() {
    return Thiscall(0x0049A650, 0x01B65990u);  // NameKey_Destruct
}

// 0x8F66B0 (exit)
uint32_t __cdecl Fn8F66B0() {
    return Thiscall(0x0049A650, 0x01B659FCu);  // NameKey_Destruct
}

// 0x8F66C0 (exit)
uint32_t __cdecl Fn8F66C0() {
    return Thiscall(0x0049A650, 0x01B65A00u);  // NameKey_Destruct
}

// 0x8F66D0 (exit)
uint32_t __cdecl Fn8F66D0() {
    return Thiscall(0x0049A650, 0x01B65A04u);  // NameKey_Destruct
}

// 0x8F66E0 (exit)
uint32_t __cdecl Fn8F66E0() {
    return Thiscall(0x0049A650, 0x01B65A08u);  // NameKey_Destruct
}

// 0x8F66F0 (exit)
uint32_t __cdecl Fn8F66F0() {
    return Thiscall(0x0049A650, 0x01B65A0Cu);  // NameKey_Destruct
}

// 0x8F6700 (exit)
uint32_t __cdecl Fn8F6700() {
    return Thiscall(0x0049A650, 0x01B65A10u);  // NameKey_Destruct
}

// 0x8F6710 (exit)
uint32_t __cdecl Fn8F6710() {
    return Thiscall(0x0049A650, 0x01B65A14u);  // NameKey_Destruct
}

// 0x8F6720 (exit)
uint32_t __cdecl Fn8F6720() {
    return Thiscall(0x0049A650, 0x01B65A18u);  // NameKey_Destruct
}

// 0x8F6730 (exit)
uint32_t __cdecl Fn8F6730() {
    return Thiscall(0x0049A650, 0x01B65A1Cu);  // NameKey_Destruct
}

// 0x8F6740 (exit)
uint32_t __cdecl Fn8F6740() {
    return Thiscall(0x0049A650, 0x01B65A20u);  // NameKey_Destruct
}

// 0x8F6750 (exit)
uint32_t __cdecl Fn8F6750() {
    return Thiscall(0x0049A650, 0x01B65A24u);  // NameKey_Destruct
}

// 0x8F6760 (exit)
uint32_t __cdecl Fn8F6760() {
    return Thiscall(0x0049A650, 0x01B65A28u);  // NameKey_Destruct
}

// 0x8F6770 (exit)
uint32_t __cdecl Fn8F6770() {
    return Thiscall(0x0049A650, 0x01B65A2Cu);  // NameKey_Destruct
}

// 0x8F6780 (exit)
uint32_t __cdecl Fn8F6780() {
    return Thiscall(0x0049A650, 0x01B65A30u);  // NameKey_Destruct
}

// 0x8F6790 (exit)
uint32_t __cdecl Fn8F6790() {
    return Thiscall(0x0049A650, 0x01B65A34u);  // NameKey_Destruct
}

// 0x8F67A0 (exit)
uint32_t __cdecl Fn8F67A0() {
    return Thiscall(0x0049A650, 0x01B65A38u);  // NameKey_Destruct
}

// 0x8F67B0 (exit)
uint32_t __cdecl Fn8F67B0() {
    return Thiscall(0x0049A650, 0x01B65A3Cu);  // NameKey_Destruct
}

// 0x8F67C0 (exit)
uint32_t __cdecl Fn8F67C0() {
    return Thiscall(0x0049A650, 0x01B65A40u);  // NameKey_Destruct
}

// 0x8F67D0 (exit)
uint32_t __cdecl Fn8F67D0() {
    return Thiscall(0x0049A650, 0x01B65A44u);  // NameKey_Destruct
}

// 0x8F67E0 (exit)
uint32_t __cdecl Fn8F67E0() {
    return Thiscall(0x0049A650, 0x01B65A48u);  // NameKey_Destruct
}

// 0x8F67F0 (exit)
uint32_t __cdecl Fn8F67F0() {
    return Thiscall(0x0049A650, 0x01B65A4Cu);  // NameKey_Destruct
}

// 0x8F6800 (exit)
uint32_t __cdecl Fn8F6800() {
    return Thiscall(0x0049A650, 0x01B65A50u);  // NameKey_Destruct
}

// 0x8F6810 (exit)
uint32_t __cdecl Fn8F6810() {
    return Thiscall(0x0049A650, 0x01B65AACu);  // NameKey_Destruct
}

// 0x8F6820 (exit)
uint32_t __cdecl Fn8F6820() {
    return Thiscall(0x0049A650, 0x01B65AB0u);  // NameKey_Destruct
}

// 0x8F6830 (exit)
uint32_t __cdecl Fn8F6830() {
    return Thiscall(0x0049A650, 0x01B65AB4u);  // NameKey_Destruct
}

// 0x8F6840 (exit)
uint32_t __cdecl Fn8F6840() {
    return Thiscall(0x0049A650, 0x01B65AB8u);  // NameKey_Destruct
}

// 0x8F6850 (exit)
uint32_t __cdecl Fn8F6850() {
    return Thiscall(0x0049A650, 0x01B65ABCu);  // NameKey_Destruct
}

// 0x8F6860 (exit)
uint32_t __cdecl Fn8F6860() {
    return Thiscall(0x0049A650, 0x01B65AC0u);  // NameKey_Destruct
}

// 0x8F6870 (exit)
uint32_t __cdecl Fn8F6870() {
    return Thiscall(0x0049A650, 0x01B65AC4u);  // NameKey_Destruct
}

// 0x8F6880 (exit)
uint32_t __cdecl Fn8F6880() {
    return Thiscall(0x0049A650, 0x01B65AC8u);  // NameKey_Destruct
}

// 0x8F6890 (exit)
uint32_t __cdecl Fn8F6890() {
    return Thiscall(0x0049A650, 0x01B65ACCu);  // NameKey_Destruct
}

// 0x8F68A0 (exit)
uint32_t __cdecl Fn8F68A0() {
    return Thiscall(0x0049A650, 0x01B65AD0u);  // NameKey_Destruct
}

// 0x8F68B0 (exit)
uint32_t __cdecl Fn8F68B0() {
    return Thiscall(0x0049A650, 0x01B65AD4u);  // NameKey_Destruct
}

// 0x8F68C0 (exit)
uint32_t __cdecl Fn8F68C0() {
    return Thiscall(0x0049A650, 0x01B65AD8u);  // NameKey_Destruct
}

// 0x8F68D0 (exit)
uint32_t __cdecl Fn8F68D0() {
    return Thiscall(0x0049A650, 0x01B65ADCu);  // NameKey_Destruct
}

// 0x8F68E0 (exit)
void __cdecl Fn8F68E0() {
}

// 0x8F68F0 (exit)
void __cdecl Fn8F68F0() {
}

// 0x8F6900 (exit)
void __cdecl Fn8F6900() {
}

// 0x8F6910 (exit)
void __cdecl Fn8F6910() {
}

// 0x8F6920 (exit)
void __cdecl Fn8F6920() {
}

// 0x8F6930 (exit)
void __cdecl Fn8F6930() {
}

// 0x8F6940 (exit)
uint32_t __cdecl Fn8F6940() {
    return Thiscall(0x0049A650, 0x01B65AF4u);  // NameKey_Destruct
}

// 0x8F6950 (exit)
uint32_t __cdecl Fn8F6950() {
    return Thiscall(0x0049A650, 0x01B65AF8u);  // NameKey_Destruct
}

// 0x8F6960 (exit)
uint32_t __cdecl Fn8F6960() {
    return Thiscall(0x0049A650, 0x01B65AFCu);  // NameKey_Destruct
}

// 0x8F6970 (exit)
uint32_t __cdecl Fn8F6970() {
    return Thiscall(0x0049A650, 0x01B65B00u);  // NameKey_Destruct
}

// 0x8F6980 (exit)
uint32_t __cdecl Fn8F6980() {
    return Thiscall(0x0049A650, 0x01B65B04u);  // NameKey_Destruct
}

// 0x8F6990 (exit)
uint32_t __cdecl Fn8F6990() {
    return Thiscall(0x0049A650, 0x01B65B08u);  // NameKey_Destruct
}

// 0x8F69A0 (exit)
uint32_t __cdecl Fn8F69A0() {
    return Thiscall(0x0049A650, 0x01B65B0Cu);  // NameKey_Destruct
}

// 0x8F69B0 (exit)
uint32_t __cdecl Fn8F69B0() {
    return Thiscall(0x0049A650, 0x01B65B10u);  // NameKey_Destruct
}

// 0x8F69C0 (exit)
uint32_t __cdecl Fn8F69C0() {
    return Thiscall(0x0049A650, 0x01B65B14u);  // NameKey_Destruct
}

// 0x8F69D0 (exit)
uint32_t __cdecl Fn8F69D0() {
    return Thiscall(0x0049A650, 0x01B65B18u);  // NameKey_Destruct
}

// 0x8F69E0 (exit)
uint32_t __cdecl Fn8F69E0() {
    return Thiscall(0x0049A650, 0x01B65B1Cu);  // NameKey_Destruct
}

// 0x8F69F0 (exit)
uint32_t __cdecl Fn8F69F0() {
    return Thiscall(0x0049A650, 0x01B65B20u);  // NameKey_Destruct
}

// 0x8F6A00 (exit)
uint32_t __cdecl Fn8F6A00() {
    return Thiscall(0x0049A650, 0x01B65B24u);  // NameKey_Destruct
}

// 0x8F6A10 (exit)
uint32_t __cdecl Fn8F6A10() {
    return Thiscall(0x0049A650, 0x01B65B28u);  // NameKey_Destruct
}

// 0x8F6A20 (exit)
uint32_t __cdecl Fn8F6A20() {
    return Thiscall(0x0049A650, 0x01B65B2Cu);  // NameKey_Destruct
}

// 0x8F6A30 (exit)
uint32_t __cdecl Fn8F6A30() {
    return Thiscall(0x0049A650, 0x01B65B30u);  // NameKey_Destruct
}

// 0x8F6A40 (exit)
uint32_t __cdecl Fn8F6A40() {
    return Thiscall(0x0049A650, 0x01B65B34u);  // NameKey_Destruct
}

// 0x8F6A50 (exit)
uint32_t __cdecl Fn8F6A50() {
    return Thiscall(0x0049A650, 0x01B65B38u);  // NameKey_Destruct
}

// 0x8F6A60 (exit)
uint32_t __cdecl Fn8F6A60() {
    return Thiscall(0x0049A650, 0x01B65B3Cu);  // NameKey_Destruct
}

// 0x8F6A70 (exit)
uint32_t __cdecl Fn8F6A70() {
    return Thiscall(0x0049A650, 0x01B65B40u);  // NameKey_Destruct
}

// 0x8F6A80 (exit)
uint32_t __cdecl Fn8F6A80() {
    return Thiscall(0x0049A650, 0x01B65B44u);  // NameKey_Destruct
}

// 0x8F6A90 (exit)
uint32_t __cdecl Fn8F6A90() {
    return Thiscall(0x0049A650, 0x01B65B48u);  // NameKey_Destruct
}

// 0x8F6AA0 (exit)
uint32_t __cdecl Fn8F6AA0() {
    return Thiscall(0x0049A650, 0x01B65B4Cu);  // NameKey_Destruct
}

// 0x8F6AB0 (exit)
uint32_t __cdecl Fn8F6AB0() {
    return Thiscall(0x0049A650, 0x01B65B50u);  // NameKey_Destruct
}

// 0x8F6AC0 (exit)
uint32_t __cdecl Fn8F6AC0() {
    return Thiscall(0x0049A650, 0x01B65B54u);  // NameKey_Destruct
}

// 0x8F6AD0 (exit)
uint32_t __cdecl Fn8F6AD0() {
    return Thiscall(0x0049A650, 0x01B65B58u);  // NameKey_Destruct
}

// 0x8F6AE0 (exit)
uint32_t __cdecl Fn8F6AE0() {
    return Thiscall(0x0049A650, 0x01B65B5Cu);  // NameKey_Destruct
}

// 0x8F6AF0 (exit)
uint32_t __cdecl Fn8F6AF0() {
    return Thiscall(0x0049A650, 0x01B65B60u);  // NameKey_Destruct
}

// 0x8F6B00 (exit)
uint32_t __cdecl Fn8F6B00() {
    return Thiscall(0x0049A650, 0x01B65B64u);  // NameKey_Destruct
}

// 0x8F6B10 (exit)
uint32_t __cdecl Fn8F6B10() {
    return Thiscall(0x0049A650, 0x01B65B68u);  // NameKey_Destruct
}

// 0x8F6B20 (exit)
uint32_t __cdecl Fn8F6B20() {
    return Thiscall(0x0049A650, 0x01B65B6Cu);  // NameKey_Destruct
}

// 0x8F6B30 (exit)
uint32_t __cdecl Fn8F6B30() {
    return Thiscall(0x0049A650, 0x01B65B70u);  // NameKey_Destruct
}

// 0x8F6B40 (exit)
uint32_t __cdecl Fn8F6B40() {
    return Thiscall(0x0049A650, 0x01B65B74u);  // NameKey_Destruct
}

// 0x8F6B50 (exit)
uint32_t __cdecl Fn8F6B50() {
    return Thiscall(0x0049A650, 0x01B65B78u);  // NameKey_Destruct
}

// 0x8F6B60 (exit)
uint32_t __cdecl Fn8F6B60() {
    return Thiscall(0x0049A650, 0x01B65B7Cu);  // NameKey_Destruct
}

// 0x8F6B70 (exit)
uint32_t __cdecl Fn8F6B70() {
    return Thiscall(0x0049A650, 0x01B65B80u);  // NameKey_Destruct
}

// 0x8F6B80 (exit)
uint32_t __cdecl Fn8F6B80() {
    return Thiscall(0x0049A650, 0x01B65B84u);  // NameKey_Destruct
}

// 0x8F6B90 (exit)
uint32_t __cdecl Fn8F6B90() {
    return Thiscall(0x0049A650, 0x01B65B88u);  // NameKey_Destruct
}

// 0x8F6BA0 (exit)
uint32_t __cdecl Fn8F6BA0() {
    return Thiscall(0x0049A650, 0x01B65B8Cu);  // NameKey_Destruct
}

// 0x8F6BB0 (exit)
uint32_t __cdecl Fn8F6BB0() {
    return Thiscall(0x0049A650, 0x01B65B90u);  // NameKey_Destruct
}

// 0x8F6BC0 (exit)
uint32_t __cdecl Fn8F6BC0() {
    return Thiscall(0x0049A650, 0x01B65B94u);  // NameKey_Destruct
}

// 0x8F6BD0 (exit)
uint32_t __cdecl Fn8F6BD0() {
    return Thiscall(0x0049A650, 0x01B65B98u);  // NameKey_Destruct
}

// 0x8F6BE0 (exit)
uint32_t __cdecl Fn8F6BE0() {
    return Thiscall(0x0049A650, 0x01B65B9Cu);  // NameKey_Destruct
}

// 0x8F6BF0 (exit)
uint32_t __cdecl Fn8F6BF0() {
    return Thiscall(0x0049A650, 0x01B65BA0u);  // NameKey_Destruct
}

// 0x8F6C00 (exit)
uint32_t __cdecl Fn8F6C00() {
    return Thiscall(0x0049A650, 0x01B65BA4u);  // NameKey_Destruct
}

// 0x8F6C10 (exit)
uint32_t __cdecl Fn8F6C10() {
    return Thiscall(0x0049A650, 0x01B65BA8u);  // NameKey_Destruct
}

// 0x8F6C20 (exit)
uint32_t __cdecl Fn8F6C20() {
    return Thiscall(0x0049A650, 0x01B65BACu);  // NameKey_Destruct
}

// 0x8F6C30 (exit)
uint32_t __cdecl Fn8F6C30() {
    return Thiscall(0x0049A650, 0x01B65BB0u);  // NameKey_Destruct
}

// 0x8F6C40 (exit)
uint32_t __cdecl Fn8F6C40() {
    return Thiscall(0x0049A650, 0x01B65BB4u);  // NameKey_Destruct
}

// 0x8F6C50 (exit)
uint32_t __cdecl Fn8F6C50() {
    return Thiscall(0x0049A650, 0x01B65BB8u);  // NameKey_Destruct
}

// 0x8F6C60 (exit)
uint32_t __cdecl Fn8F6C60() {
    return Thiscall(0x0049A650, 0x01B65BBCu);  // NameKey_Destruct
}

// 0x8F6C70 (exit)
uint32_t __cdecl Fn8F6C70() {
    return Thiscall(0x0049A650, 0x01B65BC0u);  // NameKey_Destruct
}

// 0x8F6C80 (exit)
uint32_t __cdecl Fn8F6C80() {
    return Thiscall(0x0049A650, 0x01B65BC4u);  // NameKey_Destruct
}

// 0x8F6C90 (exit)
uint32_t __cdecl Fn8F6C90() {
    return Thiscall(0x0049A650, 0x01B65BC8u);  // NameKey_Destruct
}

// 0x8F6CA0 (exit)
uint32_t __cdecl Fn8F6CA0() {
    return Thiscall(0x0049A650, 0x01B65BCCu);  // NameKey_Destruct
}

// 0x8F6CB0 (exit)
uint32_t __cdecl Fn8F6CB0() {
    return Thiscall(0x0049A650, 0x01B65BD0u);  // NameKey_Destruct
}

// 0x8F6CC0 (exit)
uint32_t __cdecl Fn8F6CC0() {
    return Thiscall(0x0049A650, 0x01B65BD4u);  // NameKey_Destruct
}

// 0x8F6CD0 (exit)
uint32_t __cdecl Fn8F6CD0() {
    return Thiscall(0x0049A650, 0x01B65BD8u);  // NameKey_Destruct
}

// 0x8F6CE0 (exit)
uint32_t __cdecl Fn8F6CE0() {
    return Thiscall(0x0049A650, 0x01B65BDCu);  // NameKey_Destruct
}

// 0x8F6CF0 (exit)
uint32_t __cdecl Fn8F6CF0() {
    return Thiscall(0x0049A650, 0x01B65BE0u);  // NameKey_Destruct
}

// 0x8F6D00 (exit)
uint32_t __cdecl Fn8F6D00() {
    return Thiscall(0x0049A650, 0x01B65BE4u);  // NameKey_Destruct
}

// 0x8F6D10 (exit)
uint32_t __cdecl Fn8F6D10() {
    return Thiscall(0x0049A650, 0x01B65BE8u);  // NameKey_Destruct
}

// 0x8F6D20 (exit)
uint32_t __cdecl Fn8F6D20() {
    return Thiscall(0x0049A650, 0x01B65BECu);  // NameKey_Destruct
}

// 0x8F6D30 (exit)
uint32_t __cdecl Fn8F6D30() {
    return Thiscall(0x0049A650, 0x01B65BF0u);  // NameKey_Destruct
}

// 0x8F6D40 (exit)
uint32_t __cdecl Fn8F6D40() {
    return Thiscall(0x0049A650, 0x01B65BF4u);  // NameKey_Destruct
}

// 0x8F6D50 (exit)
uint32_t __cdecl Fn8F6D50() {
    return Thiscall(0x0049A650, 0x01B65BF8u);  // NameKey_Destruct
}

// 0x8F6D60 (exit)
uint32_t __cdecl Fn8F6D60() {
    return Thiscall(0x0049A650, 0x01B65BFCu);  // NameKey_Destruct
}

// 0x8F6D70 (exit)
uint32_t __cdecl Fn8F6D70() {
    return Thiscall(0x0049A650, 0x01B65C00u);  // NameKey_Destruct
}

// 0x8F6D80 (exit)
uint32_t __cdecl Fn8F6D80() {
    return Thiscall(0x0049A650, 0x01B65C04u);  // NameKey_Destruct
}

// 0x8F6D90 (exit)
uint32_t __cdecl Fn8F6D90() {
    return Thiscall(0x0049A650, 0x01B65C08u);  // NameKey_Destruct
}

// 0x8F6DA0 (exit)
uint32_t __cdecl Fn8F6DA0() {
    return Thiscall(0x0049A650, 0x01B65C64u);  // NameKey_Destruct
}

// 0x8F6DB0 (exit)
uint32_t __cdecl Fn8F6DB0() {
    return Thiscall(0x0049A650, 0x01B65C68u);  // NameKey_Destruct
}

// 0x8F6DC0 (exit)
uint32_t __cdecl Fn8F6DC0() {
    return Thiscall(0x0049A650, 0x01B65C6Cu);  // NameKey_Destruct
}

// 0x8F6DD0 (exit)
uint32_t __cdecl Fn8F6DD0() {
    return Thiscall(0x0049A650, 0x01B65C70u);  // NameKey_Destruct
}

// 0x8F6DE0 (exit)
uint32_t __cdecl Fn8F6DE0() {
    return Thiscall(0x0049A650, 0x01B65C74u);  // NameKey_Destruct
}

// 0x8F6DF0 (exit)
uint32_t __cdecl Fn8F6DF0() {
    return Thiscall(0x0049A650, 0x01B65C78u);  // NameKey_Destruct
}

// 0x8F6E00 (exit)
uint32_t __cdecl Fn8F6E00() {
    return Thiscall(0x0049A650, 0x01B65C7Cu);  // NameKey_Destruct
}

// 0x8F6E10 (exit)
uint32_t __cdecl Fn8F6E10() {
    return Thiscall(0x0049A650, 0x01B65C80u);  // NameKey_Destruct
}

// 0x8F6E20 (exit)
uint32_t __cdecl Fn8F6E20() {
    return Thiscall(0x0049A650, 0x01B65C84u);  // NameKey_Destruct
}

// 0x8F6E30 (exit)
uint32_t __cdecl Fn8F6E30() {
    return Thiscall(0x0049A650, 0x01B65C88u);  // NameKey_Destruct
}

// 0x8F6E40 (exit)
uint32_t __cdecl Fn8F6E40() {
    return Thiscall(0x0049A650, 0x01B65C8Cu);  // NameKey_Destruct
}

// 0x8F6E50 (exit)
uint32_t __cdecl Fn8F6E50() {
    return Thiscall(0x0049A650, 0x01B65C90u);  // NameKey_Destruct
}

// 0x8F6E60 (exit)
uint32_t __cdecl Fn8F6E60() {
    return Thiscall(0x0049A650, 0x01B65C94u);  // NameKey_Destruct
}

// 0x8F6E70 (exit)
uint32_t __cdecl Fn8F6E70() {
    return Thiscall(0x0049A650, 0x01B65C98u);  // NameKey_Destruct
}

// 0x8F6E80 (exit)
uint32_t __cdecl Fn8F6E80() {
    return Thiscall(0x0049A650, 0x01B65C9Cu);  // NameKey_Destruct
}

// 0x8F6E90 (exit)
uint32_t __cdecl Fn8F6E90() {
    return Thiscall(0x0049A650, 0x01B65CA0u);  // NameKey_Destruct
}

// 0x8F6EA0 (exit)
uint32_t __cdecl Fn8F6EA0() {
    return Thiscall(0x0049A650, 0x01B65CA4u);  // NameKey_Destruct
}

// 0x8F6EB0 (exit)
uint32_t __cdecl Fn8F6EB0() {
    return Thiscall(0x0049A650, 0x01B65CA8u);  // NameKey_Destruct
}

// 0x8F6EC0 (exit)
uint32_t __cdecl Fn8F6EC0() {
    return Thiscall(0x0049A650, 0x01B65CACu);  // NameKey_Destruct
}

// 0x8F6ED0 (exit)
uint32_t __cdecl Fn8F6ED0() {
    return Thiscall(0x0049A650, 0x01B65CB0u);  // NameKey_Destruct
}

// 0x8F6EE0 (exit)
uint32_t __cdecl Fn8F6EE0() {
    return Thiscall(0x0049A650, 0x01B65CB4u);  // NameKey_Destruct
}

// 0x8F6EF0 (exit)
uint32_t __cdecl Fn8F6EF0() {
    return Thiscall(0x0049A650, 0x01B65CB8u);  // NameKey_Destruct
}

// 0x8F6F00 (exit)
uint32_t __cdecl Fn8F6F00() {
    return Thiscall(0x0049A650, 0x01B65CBCu);  // NameKey_Destruct
}

// 0x8F6F10 (exit)
uint32_t __cdecl Fn8F6F10() {
    return Thiscall(0x0049A650, 0x01B65CC0u);  // NameKey_Destruct
}

// 0x8F6F20 (exit)
uint32_t __cdecl Fn8F6F20() {
    return Thiscall(0x0049A650, 0x01B65CC4u);  // NameKey_Destruct
}

// 0x8F6F30 (exit)
uint32_t __cdecl Fn8F6F30() {
    return Thiscall(0x0049A650, 0x01B65CC8u);  // NameKey_Destruct
}

// 0x8F6F40 (exit)
uint32_t __cdecl Fn8F6F40() {
    return Thiscall(0x0049A650, 0x01B65CCCu);  // NameKey_Destruct
}

// 0x8F6F50 (exit)
uint32_t __cdecl Fn8F6F50() {
    return Thiscall(0x0049A650, 0x01B65CD0u);  // NameKey_Destruct
}

// 0x8F6F60 (exit)
uint32_t __cdecl Fn8F6F60() {
    return Thiscall(0x0049A650, 0x01B65CD4u);  // NameKey_Destruct
}

// 0x8F6F70 (exit)
uint32_t __cdecl Fn8F6F70() {
    return Thiscall(0x0049A650, 0x01B65CD8u);  // NameKey_Destruct
}

// 0x8F6F80 (exit)
uint32_t __cdecl Fn8F6F80() {
    return Thiscall(0x0049A650, 0x01B65CDCu);  // NameKey_Destruct
}

// 0x8F6F90 (exit)
uint32_t __cdecl Fn8F6F90() {
    return Thiscall(0x0049A650, 0x01B65CE0u);  // NameKey_Destruct
}

// 0x8F6FA0 (exit)
uint32_t __cdecl Fn8F6FA0() {
    return Thiscall(0x0049A650, 0x01B65CE4u);  // NameKey_Destruct
}

// 0x8F6FB0 (exit)
uint32_t __cdecl Fn8F6FB0() {
    return Thiscall(0x0049A650, 0x01B65CE8u);  // NameKey_Destruct
}

// 0x8F6FC0 (exit)
uint32_t __cdecl Fn8F6FC0() {
    return Thiscall(0x0049A650, 0x01B65CECu);  // NameKey_Destruct
}

// 0x8F6FD0 (exit)
uint32_t __cdecl Fn8F6FD0() {
    return Thiscall(0x0049A650, 0x01B65CF0u);  // NameKey_Destruct
}

// 0x8F6FE0 (exit)
uint32_t __cdecl Fn8F6FE0() {
    return Thiscall(0x0049A650, 0x01B65CF4u);  // NameKey_Destruct
}

// 0x8F6FF0 (exit)
uint32_t __cdecl Fn8F6FF0() {
    return Thiscall(0x0049A650, 0x01B65CF8u);  // NameKey_Destruct
}

// 0x8F7000 (exit)
uint32_t __cdecl Fn8F7000() {
    return Thiscall(0x0049A650, 0x01B65CFCu);  // NameKey_Destruct
}

// 0x8F7010 (exit)
uint32_t __cdecl Fn8F7010() {
    return Thiscall(0x0049A650, 0x01B65D00u);  // NameKey_Destruct
}

// 0x8F7020 (exit)
uint32_t __cdecl Fn8F7020() {
    return Thiscall(0x0049A650, 0x01B65D04u);  // NameKey_Destruct
}

// 0x8F7030 (exit)
uint32_t __cdecl Fn8F7030() {
    return Thiscall(0x0049A650, 0x01B65D08u);  // NameKey_Destruct
}

// 0x8F7040 (exit)
uint32_t __cdecl Fn8F7040() {
    return Thiscall(0x0049A650, 0x01B65D0Cu);  // NameKey_Destruct
}

// 0x8F7050 (exit)
uint32_t __cdecl Fn8F7050() {
    return Thiscall(0x0049A650, 0x01B65D10u);  // NameKey_Destruct
}

// 0x8F7060 (exit)
uint32_t __cdecl Fn8F7060() {
    return Thiscall(0x0049A650, 0x01B65D14u);  // NameKey_Destruct
}

// 0x8F7070 (exit)
uint32_t __cdecl Fn8F7070() {
    return Thiscall(0x0049A650, 0x01B65D18u);  // NameKey_Destruct
}

// 0x8F7080 (exit)
uint32_t __cdecl Fn8F7080() {
    return Thiscall(0x0049A650, 0x01B65D1Cu);  // NameKey_Destruct
}

// 0x8F7090 (exit)
uint32_t __cdecl Fn8F7090() {
    return Thiscall(0x0049A650, 0x01B65D20u);  // NameKey_Destruct
}

// 0x8F70A0 (exit)
uint32_t __cdecl Fn8F70A0() {
    return Thiscall(0x0049A650, 0x01B65D24u);  // NameKey_Destruct
}

// 0x8F70B0 (exit)
uint32_t __cdecl Fn8F70B0() {
    return Thiscall(0x0049A650, 0x01B65D28u);  // NameKey_Destruct
}

// 0x8F70C0 (exit)
uint32_t __cdecl Fn8F70C0() {
    return Thiscall(0x0049A650, 0x01B65D2Cu);  // NameKey_Destruct
}

// 0x8F70D0 (exit)
uint32_t __cdecl Fn8F70D0() {
    return Thiscall(0x0049A650, 0x01B65D30u);  // NameKey_Destruct
}

// 0x8F70E0 (exit)
uint32_t __cdecl Fn8F70E0() {
    return Thiscall(0x0049A650, 0x01B65D34u);  // NameKey_Destruct
}

// 0x8F70F0 (exit)
uint32_t __cdecl Fn8F70F0() {
    return Thiscall(0x0049A650, 0x01B65D38u);  // NameKey_Destruct
}

// 0x8F7100 (exit)
uint32_t __cdecl Fn8F7100() {
    return Thiscall(0x0049A650, 0x01B65D3Cu);  // NameKey_Destruct
}

// 0x8F7110 (exit)
uint32_t __cdecl Fn8F7110() {
    return Thiscall(0x0049A650, 0x01B65D40u);  // NameKey_Destruct
}

// 0x8F7120 (exit)
uint32_t __cdecl Fn8F7120() {
    return Thiscall(0x0049A650, 0x01B65D44u);  // NameKey_Destruct
}

// 0x8F7130 (exit)
void __cdecl Fn8F7130() {
}

// 0x8F7140 (exit)
uint32_t __cdecl Fn8F7140() {
    return Thiscall(0x00669A30, 0x01B65D50u);  // EffectPoolStorage::Destruct_669A30
}

// 0x8F7150 (exit)
uint32_t __cdecl Fn8F7150() {
    return Thiscall(0x00667E70, 0x01B782C8u);  // EffectManager::Destruct_667E70
}

// 0x8F7160 (exit)
uint32_t __cdecl Fn8F7160() {
    return Thiscall(0x0066B560, 0x01B782ECu);  // InterpolatorElementStore::Destruct_66B560
}

// 0x8F7170 (exit)
uint32_t __cdecl Fn8F7170() {
    return Thiscall(0x0049A650, 0x01B78318u);  // NameKey_Destruct
}

// 0x8F7180 (exit)
uint32_t __cdecl Fn8F7180() {
    return Thiscall(0x00669D90, 0x01B78408u);  // EffectPools::Destruct_669D90
}

// 0x8F7190 (exit)
uint32_t __cdecl Fn8F7190() {
    return Thiscall(0x0049A650, 0x01B786A8u);  // NameKey_Destruct
}

// 0x8F71A0 (exit)
uint32_t __cdecl Fn8F71A0() {
    return Thiscall(0x00667990, 0x01B82AC0u);  // EffectDefinitionManager::Destruct_667990
}

// 0x8F71B0 (exit)
uint32_t __cdecl Fn8F71B0() {
    return Thiscall(0x0049A650, 0x01B83278u);  // NameKey_Destruct
}

// 0x8F71C0 (exit)
uint32_t __cdecl Fn8F71C0() {
    return Thiscall(0x00667BE0, 0x01B782FCu);  // EffectLightManager::Destruct_667BE0
}

// 0x8F71D0 (exit)
void __cdecl Fn8F71D0() {
    Dword(0x00B6F1D0) = 0x0093D718u;
}

// 0x8F71E0 (exit)
void __cdecl Fn8F71E0() {
}

// 0x8F71F0 (exit)
uint32_t __cdecl Fn8F71F0() {
    return Thiscall(0x0049A650, 0x01B890CCu);  // NameKey_Destruct
}

// 0x8F7200 (exit)
uint32_t __cdecl Fn8F7200() {
    return Thiscall(0x0049A650, 0x01B890D0u);  // NameKey_Destruct
}

// 0x8F7210 (exit)
uint32_t __cdecl Fn8F7210() {
    return Thiscall(0x0049A650, 0x01B890D4u);  // NameKey_Destruct
}

// 0x8F7220 (exit)
uint32_t __cdecl Fn8F7220() {
    return Thiscall(0x0049A650, 0x01B890D8u);  // NameKey_Destruct
}

// 0x8F7230 (exit)
uint32_t __cdecl Fn8F7230() {
    return Thiscall(0x0049A650, 0x01B890DCu);  // NameKey_Destruct
}

// 0x8F7240 (exit)
uint32_t __cdecl Fn8F7240() {
    return Thiscall(0x0049A650, 0x01B890E0u);  // NameKey_Destruct
}

// 0x8F7250 (exit)
uint32_t __cdecl Fn8F7250() {
    return Thiscall(0x0049A650, 0x01B890E4u);  // NameKey_Destruct
}

// 0x8F7260 (exit)
uint32_t __cdecl Fn8F7260() {
    return Thiscall(0x0049A650, 0x01B890E8u);  // NameKey_Destruct
}

// 0x8F7270 (exit)
uint32_t __cdecl Fn8F7270() {
    return Thiscall(0x0049A650, 0x01B890ECu);  // NameKey_Destruct
}

// 0x8F7280 (exit)
uint32_t __cdecl Fn8F7280() {
    return Thiscall(0x0049A650, 0x01B890F0u);  // NameKey_Destruct
}

// 0x8F7290 (exit)
uint32_t __cdecl Fn8F7290() {
    return Thiscall(0x0049A650, 0x01B890F4u);  // NameKey_Destruct
}

// 0x8F72A0 (exit)
uint32_t __cdecl Fn8F72A0() {
    return Thiscall(0x0049A650, 0x01B890F8u);  // NameKey_Destruct
}

// 0x8F72B0 (exit)
uint32_t __cdecl Fn8F72B0() {
    return Thiscall(0x0049A650, 0x01B890FCu);  // NameKey_Destruct
}

// 0x8F72C0 (exit)
uint32_t __cdecl Fn8F72C0() {
    return Thiscall(0x0049A650, 0x01B89100u);  // NameKey_Destruct
}

// 0x8F72D0 (exit)
uint32_t __cdecl Fn8F72D0() {
    return Thiscall(0x0049A650, 0x01B89104u);  // NameKey_Destruct
}

// 0x8F72E0 (exit)
uint32_t __cdecl Fn8F72E0() {
    return Thiscall(0x0049A650, 0x01B89108u);  // NameKey_Destruct
}

// 0x8F72F0 (exit)
uint32_t __cdecl Fn8F72F0() {
    return Thiscall(0x0049A650, 0x01B8910Cu);  // NameKey_Destruct
}

// 0x8F7300 (exit)
uint32_t __cdecl Fn8F7300() {
    return Thiscall(0x0049A650, 0x01B89110u);  // NameKey_Destruct
}

// 0x8F7310 (exit)
uint32_t __cdecl Fn8F7310() {
    return Thiscall(0x0049A650, 0x01B89114u);  // NameKey_Destruct
}

// 0x8F7320 (exit)
uint32_t __cdecl Fn8F7320() {
    return Thiscall(0x0049A650, 0x01B89118u);  // NameKey_Destruct
}

// 0x8F7330 (exit)
uint32_t __cdecl Fn8F7330() {
    return Thiscall(0x0049A650, 0x01B8911Cu);  // NameKey_Destruct
}

// 0x8F7340 (exit)
uint32_t __cdecl Fn8F7340() {
    return Thiscall(0x0049A650, 0x01B89120u);  // NameKey_Destruct
}

// 0x8F7350 (exit)
uint32_t __cdecl Fn8F7350() {
    return Thiscall(0x0049A650, 0x01B89124u);  // NameKey_Destruct
}

// 0x8F7360 (exit)
uint32_t __cdecl Fn8F7360() {
    return Thiscall(0x0049A650, 0x01B89128u);  // NameKey_Destruct
}

// 0x8F7370 (exit)
uint32_t __cdecl Fn8F7370() {
    return Thiscall(0x0049A650, 0x01B8912Cu);  // NameKey_Destruct
}

// 0x8F7380 (exit)
uint32_t __cdecl Fn8F7380() {
    return Thiscall(0x0049A650, 0x01B89130u);  // NameKey_Destruct
}

// 0x8F7390 (exit)
uint32_t __cdecl Fn8F7390() {
    return Thiscall(0x0049A650, 0x01B89134u);  // NameKey_Destruct
}

// 0x8F73A0 (exit)
uint32_t __cdecl Fn8F73A0() {
    return Thiscall(0x0049A650, 0x01B89138u);  // NameKey_Destruct
}

// 0x8F73B0 (exit)
uint32_t __cdecl Fn8F73B0() {
    return Thiscall(0x0049A650, 0x01B8913Cu);  // NameKey_Destruct
}

// 0x8F73C0 (exit)
uint32_t __cdecl Fn8F73C0() {
    return Thiscall(0x0049A650, 0x01B89140u);  // NameKey_Destruct
}

// 0x8F73D0 (exit)
uint32_t __cdecl Fn8F73D0() {
    return Thiscall(0x0049A650, 0x01B89144u);  // NameKey_Destruct
}

// 0x8F73E0 (exit)
uint32_t __cdecl Fn8F73E0() {
    return Thiscall(0x0049A650, 0x01B89148u);  // NameKey_Destruct
}

// 0x8F73F0 (exit)
uint32_t __cdecl Fn8F73F0() {
    return Thiscall(0x0049A650, 0x01B8914Cu);  // NameKey_Destruct
}

// 0x8F7400 (exit)
uint32_t __cdecl Fn8F7400() {
    return Thiscall(0x0049A650, 0x01B89164u);  // NameKey_Destruct
}

// 0x8F7410 (exit)
uint32_t __cdecl Fn8F7410() {
    return Thiscall(0x0049A650, 0x01B89168u);  // NameKey_Destruct
}

// 0x8F7420 (exit)
uint32_t __cdecl Fn8F7420() {
    return Thiscall(0x005D9C50, 0x01B89178u);  // ScriptRef_Clear
}

// 0x8F7430 (exit)
uint32_t __cdecl Fn8F7430() {
    return Thiscall(0x0049A650, 0x01B8917Cu);  // NameKey_Destruct
}

// 0x8F7440 (exit)
uint32_t __cdecl Fn8F7440() {
    return Thiscall(0x0049A650, 0x01B89180u);  // NameKey_Destruct
}

// 0x8F7450 (exit)
uint32_t __cdecl Fn8F7450() {
    return Thiscall(0x0049A650, 0x01B89184u);  // NameKey_Destruct
}

// 0x8F7460 (exit)
uint32_t __cdecl Fn8F7460() {
    return Thiscall(0x0049A650, 0x01B89188u);  // NameKey_Destruct
}

// 0x8F7470 (exit)
uint32_t __cdecl Fn8F7470() {
    return Thiscall(0x0049A650, 0x01B8918Cu);  // NameKey_Destruct
}

// 0x8F7480 (exit)
uint32_t __cdecl Fn8F7480() {
    return Thiscall(0x0049A650, 0x01B89190u);  // NameKey_Destruct
}

// 0x8F7490 (exit)
uint32_t __cdecl Fn8F7490() {
    return Thiscall(0x0049A650, 0x01B89194u);  // NameKey_Destruct
}

// 0x8F74A0 (exit)
uint32_t __cdecl Fn8F74A0() {
    return Thiscall(0x0049A650, 0x01B891E0u);  // NameKey_Destruct
}

// 0x8F74B0 (exit)
uint32_t __cdecl Fn8F74B0() {
    return Thiscall(0x0049A650, 0x01B891E4u);  // NameKey_Destruct
}

// 0x8F74C0 (exit)
uint32_t __cdecl Fn8F74C0() {
    return Thiscall(0x0049A650, 0x01B891E8u);  // NameKey_Destruct
}

// 0x8F74D0 (exit)
uint32_t __cdecl Fn8F74D0() {
    return Thiscall(0x0049A650, 0x01B891ECu);  // NameKey_Destruct
}

// 0x8F74E0 (exit)
uint32_t __cdecl Fn8F74E0() {
    return Thiscall(0x0049A650, 0x01B891F0u);  // NameKey_Destruct
}

// 0x8F74F0 (exit)
uint32_t __cdecl Fn8F74F0() {
    return Thiscall(0x0049A650, 0x01B891F4u);  // NameKey_Destruct
}

// 0x8F7500 (exit)
uint32_t __cdecl Fn8F7500() {
    return Thiscall(0x0049A650, 0x01B891F8u);  // NameKey_Destruct
}

// 0x8F7510 (exit)
uint32_t __cdecl Fn8F7510() {
    return Thiscall(0x0049A650, 0x01B891FCu);  // NameKey_Destruct
}

// 0x8F7520 (exit)
uint32_t __cdecl Fn8F7520() {
    return Thiscall(0x0049A650, 0x01B89200u);  // NameKey_Destruct
}

// 0x8F7530 (exit)
uint32_t __cdecl Fn8F7530() {
    return Thiscall(0x0049A650, 0x01B89204u);  // NameKey_Destruct
}

// 0x8F7540 (exit)
uint32_t __cdecl Fn8F7540() {
    return Thiscall(0x0049A650, 0x01B89208u);  // NameKey_Destruct
}

// 0x8F7550 (exit)
uint32_t __cdecl Fn8F7550() {
    return Thiscall(0x0049A650, 0x01B8920Cu);  // NameKey_Destruct
}

// 0x8F7560 (exit)
uint32_t __cdecl Fn8F7560() {
    return Thiscall(0x0049A650, 0x01B89210u);  // NameKey_Destruct
}

// 0x8F7570 (exit)
uint32_t __cdecl Fn8F7570() {
    return Thiscall(0x0049A650, 0x01B89214u);  // NameKey_Destruct
}

// 0x8F7580 (exit)
uint32_t __cdecl Fn8F7580() {
    return Thiscall(0x0049A650, 0x01B89218u);  // NameKey_Destruct
}

// 0x8F7590 (exit)
uint32_t __cdecl Fn8F7590() {
    return Thiscall(0x0049A650, 0x01B8921Cu);  // NameKey_Destruct
}

// 0x8F75A0 (exit)
uint32_t __cdecl Fn8F75A0() {
    return Thiscall(0x0049A650, 0x01B89220u);  // NameKey_Destruct
}

// 0x8F75B0 (exit)
uint32_t __cdecl Fn8F75B0() {
    return Thiscall(0x0049A650, 0x01B89224u);  // NameKey_Destruct
}

// 0x8F75C0 (exit)
uint32_t __cdecl Fn8F75C0() {
    return Thiscall(0x0049A650, 0x01B89228u);  // NameKey_Destruct
}

// 0x8F75D0 (exit)
uint32_t __cdecl Fn8F75D0() {
    return Thiscall(0x0049A650, 0x01B8922Cu);  // NameKey_Destruct
}

// 0x8F75E0 (exit)
uint32_t __cdecl Fn8F75E0() {
    return Thiscall(0x00684CF0, 0x00B9DAB8u /* "Act/dialog.act" */);  // Conversation::End
}

// 0x8F75F0 (exit)
void __cdecl Fn8F75F0() {
}

// 0x8F7600 (exit)
void __cdecl Fn8F7600() {
}

// 0x8F7610 (exit)
void __cdecl Fn8F7610() {
}

// 0x8F7620 (exit)
void __cdecl Fn8F7620() {
}

// 0x8F7650 (exit)
void __cdecl Fn8F7650() {
}

// 0x8F7660 (exit)
void __cdecl Fn8F7660() {
}

// 0x8F7670 (exit)
void __cdecl Fn8F7670() {
}

// 0x8F7680 (exit)
uint32_t __cdecl Fn8F7680() {
    return Thiscall(0x0068FB40, 0x020C2FA4u);  // CText::Unload
}

// 0x8F7690 (exit)
void __cdecl Fn8F7690() {
}

// 0x8F76A0 (exit)
void __cdecl Fn8F76A0() {
}

// 0x8F76B0 (exit)
void __cdecl Fn8F76B0() {
}

// 0x8F76C0 (exit)
void __cdecl Fn8F76C0() {
}

// 0x8F76D0 (exit)
void __cdecl Fn8F76D0() {
}

// 0x8F76E0 (exit)
void __cdecl Fn8F76E0() {
}

// 0x8F76F0 (exit)
void __cdecl Fn8F76F0() {
}

// 0x8F7700 (exit)
void __cdecl Fn8F7700() {
}

// 0x8F7710 (exit)
void __cdecl Fn8F7710() {
}

// 0x8F7720 (exit)
void __cdecl Fn8F7720() {
}

// 0x8F7730 (exit)
void __cdecl Fn8F7730() {
}

// 0x8F7740 (exit)
void __cdecl Fn8F7740() {
}

// 0x8F7750 (exit)
void __cdecl Fn8F7750() {
}

// 0x8F7760 (exit)
void __cdecl Fn8F7760() {
}

// 0x8F7770 (exit)
uint32_t __cdecl Fn8F7770() {
    Dword(0x020C33F8) = 0x00941004u;
    return Thiscall(0x0069AB40, 0x020C33F8u);  // FEScrollingSelectionListWindow::~FEScrollingSelectionListWindow
}

// 0x8F7790 (exit)
uint32_t __cdecl Fn8F7790() {
    Dword(0x020C3488) = 0x0093FCFCu /* "Pti" */;
    return Thiscall(0x0069B8E0, 0x020C3488u);  // FESelectionListWindow::~FESelectionListWindow
}

// 0x8F77B0 (exit)
uint32_t __cdecl Fn8F77B0() {
    Dword(0x020C3508) = 0x0093FCFCu /* "Pti" */;
    return Thiscall(0x0069B8E0, 0x020C3508u);  // FESelectionListWindow::~FESelectionListWindow
}

// 0x8F77D0 (exit)
void __cdecl Fn8F77D0() {
}

// 0x8F77E0 (exit)
void __cdecl Fn8F77E0() {
}

// 0x8F77F0 (exit)
uint32_t __cdecl Fn8F77F0() {
    return Thiscall(0x0049A650, 0x020C359Cu);  // NameKey_Destruct
}

// 0x8F7800 (exit)
uint32_t __cdecl Fn8F7800() {
    return Thiscall(0x0049A650, 0x020C35A0u);  // NameKey_Destruct
}

// 0x8F7830 (exit)
uint32_t __cdecl Fn8F7830() {
    return Thiscall(0x0049A650, 0x020C35A4u);  // NameKey_Destruct
}

// 0x8F7840 (exit)
uint32_t __cdecl Fn8F7840() {
    return Thiscall(0x0049A650, 0x020C35A8u);  // NameKey_Destruct
}

// 0x8F7850 (exit)
uint32_t __cdecl Fn8F7850() {
    return Thiscall(0x0049A650, 0x020C35ACu);  // NameKey_Destruct
}

// 0x8F7860 (exit)
uint32_t __cdecl Fn8F7860() {
    return Thiscall(0x0049A650, 0x020C35B0u);  // NameKey_Destruct
}

// 0x8F7870 (exit)
uint32_t __cdecl Fn8F7870() {
    return Thiscall(0x0049A650, 0x020C35B4u);  // NameKey_Destruct
}

// 0x8F7880 (exit)
uint32_t __cdecl Fn8F7880() {
    return Thiscall(0x0049A650, 0x020C35B8u);  // NameKey_Destruct
}

// 0x8F7890 (exit)
uint32_t __cdecl Fn8F7890() {
    return Thiscall(0x0049A650, 0x020C35BCu);  // NameKey_Destruct
}

// 0x8F78A0 (exit)
uint32_t __cdecl Fn8F78A0() {
    return Thiscall(0x0049A650, 0x020C35C0u);  // NameKey_Destruct
}

// 0x8F78B0 (exit)
uint32_t __cdecl Fn8F78B0() {
    return Thiscall(0x0049A650, 0x020C35C4u);  // NameKey_Destruct
}

// 0x8F78C0 (exit)
uint32_t __cdecl Fn8F78C0() {
    return Thiscall(0x0049A650, 0x020C35C8u);  // NameKey_Destruct
}

// 0x8F78D0 (exit)
uint32_t __cdecl Fn8F78D0() {
    return Thiscall(0x0049A650, 0x020C35CCu);  // NameKey_Destruct
}

// 0x8F78E0 (exit)
uint32_t __cdecl Fn8F78E0() {
    return Thiscall(0x0049A650, 0x020C35D0u);  // NameKey_Destruct
}

// 0x8F78F0 (exit)
uint32_t __cdecl Fn8F78F0() {
    return Thiscall(0x0049A650, 0x020C35D4u);  // NameKey_Destruct
}

// 0x8F7900 (exit)
uint32_t __cdecl Fn8F7900() {
    return Thiscall(0x0049A650, 0x020C35D8u);  // NameKey_Destruct
}

// 0x8F7910 (exit)
uint32_t __cdecl Fn8F7910() {
    return Thiscall(0x0049A650, 0x020C35DCu);  // NameKey_Destruct
}

// 0x8F7920 (exit)
uint32_t __cdecl Fn8F7920() {
    return Thiscall(0x0049A650, 0x020C35E0u);  // NameKey_Destruct
}

// 0x8F7930 (exit)
uint32_t __cdecl Fn8F7930() {
    return Thiscall(0x0049A650, 0x020C35E4u);  // NameKey_Destruct
}

// 0x8F7940 (exit)
void __cdecl Fn8F7940() {
}

// 0x8F7950 (exit)
void __cdecl Fn8F7950() {
}

// 0x8F7960 (exit)
uint32_t __cdecl Fn8F7960() {
    return Thiscall(0x006AB6F0, 0x020C3CA0u);  // CMissionMgr::~CMissionMgr
}

// 0x8F7970 (exit)
uint32_t __cdecl Fn8F7970() {
    return Thiscall(0x006AE220, 0x020C4420u);  // CMissionObjectiveMgr::~CMissionObjectiveMgr
}

// 0x8F7980 (exit)
void __cdecl Fn8F7980() {
}

// 0x8F7990 (exit)
void __cdecl Fn8F7990() {
    Dword(0x00BB6130) = 0x0092D0ACu;
}

// 0x8F79A0 (exit)
void __cdecl Fn8F79A0() {
    Dword(0x00BB615C) = 0x0092D0ACu;
}

// 0x8F79B0 (exit)
void __cdecl Fn8F79B0() {
    Dword(0x020C4A28) = 0x0092D0ACu;
    Dword(0x020C49F8) = 0x0092D0ACu;
    Dword(0x020C49C8) = 0x0092D0ACu;
    Dword(0x020C4998) = 0x0092D0ACu;
    Dword(0x020C496C) = 0x0092D0ACu;
    Dword(0x020C493C) = 0x0092D0ACu;
    Dword(0x020C4910) = 0x0092D0ACu;
}

// 0x8F79E0 (exit)
void __cdecl Fn8F79E0() {
}

// 0x8F7A20 (exit)
void __cdecl Fn8F7A20() {
}

// 0x8F7A30 (exit)
void __cdecl Fn8F7A30() {
}

// 0x8F7A40 (exit)
uint32_t __cdecl Fn8F7A40() {
    return Thiscall(0x006D2150, 0x020C7BECu);  // COctTree::~COctTree
}

// 0x8F7A50 (exit)
void __cdecl Fn8F7A50() {
}

// 0x8F7B10 (exit)
void __cdecl Fn8F7B10() {
}

// 0x8F7B20 (exit)
void __cdecl Fn8F7B20() {
    Dword(0x00BC5E7C) = 0x009437F8u;
}

// 0x8F7B30 (exit)
void __cdecl Fn8F7B30() {
    Dword(0x020C7E98) = 0x00943A44u /* " wm" */;
}

// 0x8F7B40 (exit)
void __cdecl Fn8F7B40() {
}

// 0x8F7B50 (exit)
void __cdecl Fn8F7B50() {
}

// 0x8F7B60 (exit)
void __cdecl Fn8F7B60() {
}

// 0x8F7B70 (exit)
void __cdecl Fn8F7B70() {
}

// 0x8F7B80 (exit)
void __cdecl Fn8F7B80() {
}

// 0x8F7B90 (exit)
void __cdecl Fn8F7B90() {
}

// 0x8F7BA0 (exit)
void __cdecl Fn8F7BA0() {
}

// 0x8F7BB0 (exit)
void __cdecl Fn8F7BB0() {
}

// 0x8F7BC0 (exit)
void __cdecl Fn8F7BC0() {
}

// 0x8F7BD0 (exit)
void __cdecl Fn8F7BD0() {
}

// 0x8F7BE0 (exit)
void __cdecl Fn8F7BE0() {
}

// 0x8F7BF0 (exit)
void __cdecl Fn8F7BF0() {
}

// 0x8F7C00 (exit)
void __cdecl Fn8F7C00() {
}

// 0x8F7C10 (exit)
void __cdecl Fn8F7C10() {
}

// 0x8F7C20 (exit)
void __cdecl Fn8F7C20() {
}

// 0x8F7C30 (exit)
void __cdecl Fn8F7C30() {
}

// 0x8F7C40 (exit)
void __cdecl Fn8F7C40() {
}

// 0x8F7C50 (exit)
void __cdecl Fn8F7C50() {
}

// 0x8F7C60 (exit)
void __cdecl Fn8F7C60() {
}

// 0x8F7C70 (exit)
void __cdecl Fn8F7C70() {
}

// 0x8F7C80 (exit)
void __cdecl Fn8F7C80() {
}

// 0x8F7C90 (exit)
void __cdecl Fn8F7C90() {
}

// 0x8F7CA0 (exit)
void __cdecl Fn8F7CA0() {
}

// 0x8F7CB0 (exit)
void __cdecl Fn8F7CB0() {
}

// 0x8F7CC0 (exit)
void __cdecl Fn8F7CC0() {
}

// 0x8F7CD0 (exit)
void __cdecl Fn8F7CD0() {
}

// 0x8F7CE0 (exit)
void __cdecl Fn8F7CE0() {
}

// 0x8F7CF0 (exit)
void __cdecl Fn8F7CF0() {
}

// 0x8F7D00 (exit)
void __cdecl Fn8F7D00() {
}

// 0x8F7D10 (exit)
void __cdecl Fn8F7D10() {
}

// 0x8F7D20 (exit)
void __cdecl Fn8F7D20() {
}

// 0x8F7D30 (exit)
void __cdecl Fn8F7D30() {
}

// 0x8F7D40 (exit)
void __cdecl Fn8F7D40() {
}

// 0x8F7D50 (exit)
void __cdecl Fn8F7D50() {
}

// 0x8F7D60 (exit)
void __cdecl Fn8F7D60() {
}

// 0x8F7D70 (exit)
void __cdecl Fn8F7D70() {
}

// 0x8F7D80 (exit)
void __cdecl Fn8F7D80() {
}

// 0x8F7D90 (exit)
void __cdecl Fn8F7D90() {
}

// 0x8F7DA0 (exit)
void __cdecl Fn8F7DA0() {
}

// 0x8F7DB0 (exit)
void __cdecl Fn8F7DB0() {
}

// 0x8F7DC0 (exit)
void __cdecl Fn8F7DC0() {
}

// 0x8F7DD0 (exit)
void __cdecl Fn8F7DD0() {
}

// 0x8F7DE0 (exit)
void __cdecl Fn8F7DE0() {
}

// 0x8F7DF0 (exit)
void __cdecl Fn8F7DF0() {
}

// 0x8F7E00 (exit)
void __cdecl Fn8F7E00() {
}

// 0x8F7E10 (exit)
void __cdecl Fn8F7E10() {
}

// 0x8F7E20 (exit)
void __cdecl Fn8F7E20() {
}

// 0x8F7E30 (exit)
void __cdecl Fn8F7E30() {
}

// 0x8F7E40 (exit)
void __cdecl Fn8F7E40() {
}

// 0x8F7E50 (exit)
void __cdecl Fn8F7E50() {
}

// 0x8F7E60 (exit)
void __cdecl Fn8F7E60() {
}

// 0x8F7E70 (exit)
void __cdecl Fn8F7E70() {
}

// 0x8F7E80 (exit)
void __cdecl Fn8F7E80() {
}

// 0x8F7E90 (exit)
void __cdecl Fn8F7E90() {
}

// 0x8F7EA0 (exit)
void __cdecl Fn8F7EA0() {
}

// 0x8F7EB0 (exit)
void __cdecl Fn8F7EB0() {
}

// 0x8F7EC0 (exit)
void __cdecl Fn8F7EC0() {
}

// 0x8F7ED0 (exit)
void __cdecl Fn8F7ED0() {
}

// 0x8F7EE0 (exit)
void __cdecl Fn8F7EE0() {
}

// 0x8F7EF0 (exit)
void __cdecl Fn8F7EF0() {
}

// 0x8F7F00 (exit)
void __cdecl Fn8F7F00() {
}

// 0x8F7F10 (exit)
void __cdecl Fn8F7F10() {
}

// 0x8F7F20 (exit)
void __cdecl Fn8F7F20() {
}

// 0x8F7F30 (exit)
void __cdecl Fn8F7F30() {
}

// 0x8F7F40 (exit)
void __cdecl Fn8F7F40() {
}

// 0x8F7F50 (exit)
void __cdecl Fn8F7F50() {
}

// 0x8F7F60 (exit)
void __cdecl Fn8F7F60() {
}

// 0x8F7F70 (exit)
void __cdecl Fn8F7F70() {
}

// 0x8F7F80 (exit)
void __cdecl Fn8F7F80() {
}

// 0x8F7F90 (exit)
void __cdecl Fn8F7F90() {
}

// 0x8F7FA0 (exit)
void __cdecl Fn8F7FA0() {
}

// 0x8F7FB0 (exit)
void __cdecl Fn8F7FB0() {
}

// 0x8F7FC0 (exit)
void __cdecl Fn8F7FC0() {
}

// 0x8F7FD0 (exit)
void __cdecl Fn8F7FD0() {
}

// 0x8F7FE0 (exit)
void __cdecl Fn8F7FE0() {
}

// 0x8F7FF0 (exit)
void __cdecl Fn8F7FF0() {
}

// 0x8F8000 (exit)
void __cdecl Fn8F8000() {
}

// 0x8F8010 (exit)
void __cdecl Fn8F8010() {
}

// 0x8F8020 (exit)
void __cdecl Fn8F8020() {
}

// 0x8F8030 (exit)
void __cdecl Fn8F8030() {
}

// 0x8F8040 (exit)
void __cdecl Fn8F8040() {
}

// 0x8F8050 (exit)
void __cdecl Fn8F8050() {
}

// 0x8F8060 (exit)
void __cdecl Fn8F8060() {
}

// 0x8F8070 (exit)
void __cdecl Fn8F8070() {
}

// 0x8F8080 (exit)
void __cdecl Fn8F8080() {
}

// 0x8F8090 (exit)
void __cdecl Fn8F8090() {
}

// 0x8F80A0 (exit)
void __cdecl Fn8F80A0() {
}

// 0x8F80B0 (exit)
void __cdecl Fn8F80B0() {
}

// 0x8F80C0 (exit)
void __cdecl Fn8F80C0() {
}

// 0x8F80D0 (exit)
void __cdecl Fn8F80D0() {
}

// 0x8F80E0 (exit)
void __cdecl Fn8F80E0() {
}

// 0x8F80F0 (exit)
void __cdecl Fn8F80F0() {
}

// 0x8F8100 (exit)
void __cdecl Fn8F8100() {
}

// 0x8F8110 (exit)
void __cdecl Fn8F8110() {
}

// 0x8F8120 (exit)
void __cdecl Fn8F8120() {
}

// 0x8F8130 (exit)
void __cdecl Fn8F8130() {
}

// 0x8F8140 (exit)
void __cdecl Fn8F8140() {
}

// 0x8F8180 (exit)
void __cdecl Fn8F8180() {
}

// 0x8F8190 (exit)
void __cdecl Fn8F8190() {
}

// 0x8F81A0 (exit)
void __cdecl Fn8F81A0() {
}

// 0x8F81B0 (exit)
void __cdecl Fn8F81B0() {
}

// 0x8F81C0 (exit)
void __cdecl Fn8F81C0() {
}

// 0x8F81D0 (exit)
void __cdecl Fn8F81D0() {
}

// 0x8F81E0 (exit)
void __cdecl Fn8F81E0() {
}

// 0x8F81F0 (exit)
void __cdecl Fn8F81F0() {
}

// 0x8F8200 (exit)
void __cdecl Fn8F8200() {
}

// 0x8F8210 (exit)
void __cdecl Fn8F8210() {
}

// 0x8F8220 (exit)
void __cdecl Fn8F8220() {
}

// 0x8F8230 (exit)
void __cdecl Fn8F8230() {
}

// 0x8F8240 (exit)
void __cdecl Fn8F8240() {
}

// 0x8F8250 (exit)
void __cdecl Fn8F8250() {
}

// 0x8F8260 (exit)
void __cdecl Fn8F8260() {
}

// 0x8F8270 (exit)
void __cdecl Fn8F8270() {
}

// 0x8F8280 (exit)
void __cdecl Fn8F8280() {
}

// 0x8F8290 (exit)
void __cdecl Fn8F8290() {
}

// 0x8F82A0 (exit)
uint32_t __cdecl Fn8F82A0() {
    return Thiscall(0x0049A650, 0x020C893Cu);  // NameKey_Destruct
}

// 0x8F82B0 (exit)
uint32_t __cdecl Fn8F82B0() {
    return Thiscall(0x0049A650, 0x020C8940u);  // NameKey_Destruct
}

// 0x8F82C0 (exit)
uint32_t __cdecl Fn8F82C0() {
    return Thiscall(0x0049A650, 0x020C8944u);  // NameKey_Destruct
}

// 0x8F82D0 (exit)
uint32_t __cdecl Fn8F82D0() {
    return Thiscall(0x0049A650, 0x020C8948u);  // NameKey_Destruct
}

// 0x8F82E0 (exit)
uint32_t __cdecl Fn8F82E0() {
    return Thiscall(0x0049A650, 0x020C894Cu);  // NameKey_Destruct
}

// 0x8F82F0 (exit)
uint32_t __cdecl Fn8F82F0() {
    return Thiscall(0x0049A650, 0x020C8950u);  // NameKey_Destruct
}

// 0x8F8300 (exit)
uint32_t __cdecl Fn8F8300() {
    return Thiscall(0x0049A650, 0x020C8954u);  // NameKey_Destruct
}

// 0x8F8310 (exit)
uint32_t __cdecl Fn8F8310() {
    return Thiscall(0x0049A650, 0x020C8958u);  // NameKey_Destruct
}

// 0x8F8320 (exit)
uint32_t __cdecl Fn8F8320() {
    return Thiscall(0x0049A650, 0x020C895Cu);  // NameKey_Destruct
}

// 0x8F8330 (exit)
uint32_t __cdecl Fn8F8330() {
    return Thiscall(0x0049A650, 0x020C8960u);  // NameKey_Destruct
}

// 0x8F8340 (exit)
uint32_t __cdecl Fn8F8340() {
    return Thiscall(0x0049A650, 0x020C8964u);  // NameKey_Destruct
}

// 0x8F8350 (exit)
uint32_t __cdecl Fn8F8350() {
    return Thiscall(0x0049A650, 0x020C8968u);  // NameKey_Destruct
}

// 0x8F8360 (exit)
uint32_t __cdecl Fn8F8360() {
    return Thiscall(0x0049A650, 0x020C896Cu);  // NameKey_Destruct
}

// 0x8F8370 (exit)
uint32_t __cdecl Fn8F8370() {
    return Thiscall(0x0049A650, 0x020C8970u);  // NameKey_Destruct
}

// 0x8F8380 (exit)
uint32_t __cdecl Fn8F8380() {
    return Thiscall(0x0049A650, 0x020C8974u);  // NameKey_Destruct
}

// 0x8F8390 (exit)
uint32_t __cdecl Fn8F8390() {
    return Thiscall(0x0049A650, 0x020C8978u);  // NameKey_Destruct
}

// 0x8F83A0 (exit)
uint32_t __cdecl Fn8F83A0() {
    return Thiscall(0x0049A650, 0x020C897Cu);  // NameKey_Destruct
}

// 0x8F83B0 (exit)
uint32_t __cdecl Fn8F83B0() {
    return Thiscall(0x0049A650, 0x020C8980u);  // NameKey_Destruct
}

// 0x8F83C0 (exit)
uint32_t __cdecl Fn8F83C0() {
    return Thiscall(0x0049A650, 0x020C8984u);  // NameKey_Destruct
}

// 0x8F83D0 (exit)
uint32_t __cdecl Fn8F83D0() {
    return Thiscall(0x0049A650, 0x020C8988u);  // NameKey_Destruct
}

// 0x8F83E0 (exit)
uint32_t __cdecl Fn8F83E0() {
    return Thiscall(0x0049A650, 0x020C898Cu);  // NameKey_Destruct
}

// 0x8F83F0 (exit)
uint32_t __cdecl Fn8F83F0() {
    return Thiscall(0x0049A650, 0x020C8990u);  // NameKey_Destruct
}

// 0x8F8400 (exit)
uint32_t __cdecl Fn8F8400() {
    return Thiscall(0x0049A650, 0x020C8994u);  // NameKey_Destruct
}

// 0x8F8410 (exit)
uint32_t __cdecl Fn8F8410() {
    return Thiscall(0x0049A650, 0x020C8998u);  // NameKey_Destruct
}

// 0x8F8420 (exit)
uint32_t __cdecl Fn8F8420() {
    return Thiscall(0x0049A650, 0x020C899Cu);  // NameKey_Destruct
}

// 0x8F8430 (exit)
uint32_t __cdecl Fn8F8430() {
    return Thiscall(0x0049A650, 0x020C89A0u);  // NameKey_Destruct
}

// 0x8F8440 (exit)
uint32_t __cdecl Fn8F8440() {
    return Thiscall(0x0049A650, 0x020C89A4u);  // NameKey_Destruct
}

// 0x8F8450 (exit)
uint32_t __cdecl Fn8F8450() {
    return Thiscall(0x0049A650, 0x020C89A8u);  // NameKey_Destruct
}

// 0x8F8460 (exit)
uint32_t __cdecl Fn8F8460() {
    return Thiscall(0x0049A650, 0x020C89ACu);  // NameKey_Destruct
}

// 0x8F8470 (exit)
uint32_t __cdecl Fn8F8470() {
    return Thiscall(0x0049A650, 0x020C89B0u);  // NameKey_Destruct
}

// 0x8F8480 (exit)
uint32_t __cdecl Fn8F8480() {
    return Thiscall(0x0049A650, 0x020C89B4u);  // NameKey_Destruct
}

// 0x8F8490 (exit)
uint32_t __cdecl Fn8F8490() {
    return Thiscall(0x0049A650, 0x020C89B8u);  // NameKey_Destruct
}

// 0x8F84A0 (exit)
uint32_t __cdecl Fn8F84A0() {
    return Thiscall(0x0049A650, 0x020C89BCu);  // NameKey_Destruct
}

// 0x8F84B0 (exit)
uint32_t __cdecl Fn8F84B0() {
    return Thiscall(0x0049A650, 0x020C89C0u);  // NameKey_Destruct
}

// 0x8F84C0 (exit)
uint32_t __cdecl Fn8F84C0() {
    return Thiscall(0x0049A650, 0x020C89C4u);  // NameKey_Destruct
}

// 0x8F84D0 (exit)
uint32_t __cdecl Fn8F84D0() {
    return Thiscall(0x0049A650, 0x020C89C8u);  // NameKey_Destruct
}

// 0x8F84E0 (exit)
uint32_t __cdecl Fn8F84E0() {
    return Thiscall(0x0049A650, 0x020C89CCu);  // NameKey_Destruct
}

// 0x8F84F0 (exit)
uint32_t __cdecl Fn8F84F0() {
    return Thiscall(0x0049A650, 0x020C89D0u);  // NameKey_Destruct
}

// 0x8F8500 (exit)
uint32_t __cdecl Fn8F8500() {
    return Thiscall(0x0049A650, 0x020C89D4u);  // NameKey_Destruct
}

// 0x8F8510 (exit)
uint32_t __cdecl Fn8F8510() {
    return Thiscall(0x0049A650, 0x020C89D8u);  // NameKey_Destruct
}

// 0x8F8520 (exit)
uint32_t __cdecl Fn8F8520() {
    return Thiscall(0x0049A650, 0x020C89DCu);  // NameKey_Destruct
}

// 0x8F8530 (exit)
uint32_t __cdecl Fn8F8530() {
    return Thiscall(0x0049A650, 0x020C89E0u);  // NameKey_Destruct
}

// 0x8F8540 (exit)
uint32_t __cdecl Fn8F8540() {
    return Thiscall(0x0049A650, 0x020C89E4u);  // NameKey_Destruct
}

// 0x8F8550 (exit)
uint32_t __cdecl Fn8F8550() {
    return Thiscall(0x0049A650, 0x020C89E8u);  // NameKey_Destruct
}

// 0x8F8560 (exit)
uint32_t __cdecl Fn8F8560() {
    return Thiscall(0x0049A650, 0x020C89ECu);  // NameKey_Destruct
}

// 0x8F8570 (exit)
uint32_t __cdecl Fn8F8570() {
    return Thiscall(0x0049A650, 0x020C89F0u);  // NameKey_Destruct
}

// 0x8F8580 (exit)
uint32_t __cdecl Fn8F8580() {
    return Thiscall(0x0049A650, 0x020C89F4u);  // NameKey_Destruct
}

// 0x8F8590 (exit)
uint32_t __cdecl Fn8F8590() {
    return Thiscall(0x0049A650, 0x020C89F8u);  // NameKey_Destruct
}

// 0x8F85A0 (exit)
uint32_t __cdecl Fn8F85A0() {
    return Thiscall(0x0049A650, 0x020C89FCu);  // NameKey_Destruct
}

// 0x8F85B0 (exit)
uint32_t __cdecl Fn8F85B0() {
    return Thiscall(0x0072C060, 0x020C8A00u);  // Tagging::Destruct
}

// 0x8F85C0 (exit)
void __cdecl Fn8F85C0() {
}

// 0x8F85D0 (exit)
void __cdecl Fn8F85D0() {
}

// 0x8F85F0 (exit)
void __cdecl Fn8F85F0() {
    Stdcall(Dword(0x008FC088), 0x020E21B8u);
}

// 0x8F8690 (exit)
void __cdecl Fn8F8690() {
    Stdcall(Dword(0x008FC088), 0x020E2434u);
}

// 0x8F86A0 (exit)
void __cdecl Fn8F86A0() {
    Stdcall(Dword(0x008FC088), 0x020E2414u);
}

// 0x8F86B0 (exit)
void __cdecl Fn8F86B0() {
    Stdcall(Dword(0x008FC088), 0x020E245Cu);
}

// 0x8F86C0 (exit)
void __cdecl Fn8F86C0() {
    Stdcall(Dword(0x008FC088), 0x020E248Cu);
}

// 0x8F86D0 (exit)
void __cdecl Fn8F86D0() {
    ReleaseFixedString(0x020E25B0);
}

// 0x8F8720 (exit)
void __cdecl Fn8F8720() {
    ReleaseFixedString(0x020E2810);
}

// 0x8F8760 (exit)
void __cdecl Fn8F8760() {
    ReleaseObject(0x020E28F0);
}

// 0x8F8830 (exit)
void __cdecl Fn8F8830() {
    ReleaseObject(0x020E2B24);
}

// 0x8F8860 (exit)
void __cdecl Fn8F8860() {
    ReleaseObject(0x020E2B5C);
}

} // namespace
} // namespace staticinit

#define STATICINIT6_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT6_REVERSED("StaticExit::8F58D0", 0x008F58D0, &staticinit::Fn8F58D0);
STATICINIT6_REVERSED("StaticExit::8F58E0", 0x008F58E0, &staticinit::Fn8F58E0);
STATICINIT6_REVERSED("StaticExit::8F58F0", 0x008F58F0, &staticinit::Fn8F58F0);
STATICINIT6_REVERSED("StaticExit::8F5900", 0x008F5900, &staticinit::Fn8F5900);
STATICINIT6_REVERSED("StaticExit::8F5910", 0x008F5910, &staticinit::Fn8F5910);
STATICINIT6_REVERSED("StaticExit::8F5920", 0x008F5920, &staticinit::Fn8F5920);
STATICINIT6_REVERSED("StaticExit::8F5930", 0x008F5930, &staticinit::Fn8F5930);
STATICINIT6_REVERSED("StaticExit::8F5940", 0x008F5940, &staticinit::Fn8F5940);
STATICINIT6_REVERSED("StaticExit::8F5950", 0x008F5950, &staticinit::Fn8F5950);
STATICINIT6_REVERSED("StaticExit::8F5960", 0x008F5960, &staticinit::Fn8F5960);
STATICINIT6_REVERSED("StaticExit::8F5970", 0x008F5970, &staticinit::Fn8F5970);
STATICINIT6_REVERSED("StaticExit::8F5980", 0x008F5980, &staticinit::Fn8F5980);
STATICINIT6_REVERSED("StaticExit::8F5990", 0x008F5990, &staticinit::Fn8F5990);
STATICINIT6_REVERSED("StaticExit::8F59A0", 0x008F59A0, &staticinit::Fn8F59A0);
STATICINIT6_REVERSED("StaticExit::8F59B0", 0x008F59B0, &staticinit::Fn8F59B0);
STATICINIT6_REVERSED("StaticExit::8F59C0", 0x008F59C0, &staticinit::Fn8F59C0);
STATICINIT6_REVERSED("StaticExit::8F59D0", 0x008F59D0, &staticinit::Fn8F59D0);
STATICINIT6_REVERSED("StaticExit::8F59E0", 0x008F59E0, &staticinit::Fn8F59E0);
STATICINIT6_REVERSED("StaticExit::8F59F0", 0x008F59F0, &staticinit::Fn8F59F0);
STATICINIT6_REVERSED("StaticExit::8F5A00", 0x008F5A00, &staticinit::Fn8F5A00);
STATICINIT6_REVERSED("StaticExit::8F5A10", 0x008F5A10, &staticinit::Fn8F5A10);
STATICINIT6_REVERSED("StaticExit::8F5A20", 0x008F5A20, &staticinit::Fn8F5A20);
STATICINIT6_REVERSED("StaticExit::8F5A30", 0x008F5A30, &staticinit::Fn8F5A30);
STATICINIT6_REVERSED("StaticExit::8F5A40", 0x008F5A40, &staticinit::Fn8F5A40);
STATICINIT6_REVERSED("StaticExit::8F5A50", 0x008F5A50, &staticinit::Fn8F5A50);
STATICINIT6_REVERSED("StaticExit::8F5A60", 0x008F5A60, &staticinit::Fn8F5A60);
STATICINIT6_REVERSED("StaticExit::8F5A70", 0x008F5A70, &staticinit::Fn8F5A70);
STATICINIT6_REVERSED("StaticExit::8F5A80", 0x008F5A80, &staticinit::Fn8F5A80);
STATICINIT6_REVERSED("StaticExit::8F5A90", 0x008F5A90, &staticinit::Fn8F5A90);
STATICINIT6_REVERSED("StaticExit::8F5AA0", 0x008F5AA0, &staticinit::Fn8F5AA0);
STATICINIT6_REVERSED("StaticExit::8F5AB0", 0x008F5AB0, &staticinit::Fn8F5AB0);
STATICINIT6_REVERSED("StaticExit::8F5AC0", 0x008F5AC0, &staticinit::Fn8F5AC0);
STATICINIT6_REVERSED("StaticExit::8F5AD0", 0x008F5AD0, &staticinit::Fn8F5AD0);
STATICINIT6_REVERSED("StaticExit::8F5AE0", 0x008F5AE0, &staticinit::Fn8F5AE0);
STATICINIT6_REVERSED("StaticExit::8F5AF0", 0x008F5AF0, &staticinit::Fn8F5AF0);
STATICINIT6_REVERSED("StaticExit::8F5B00", 0x008F5B00, &staticinit::Fn8F5B00);
STATICINIT6_REVERSED("StaticExit::8F5B10", 0x008F5B10, &staticinit::Fn8F5B10);
STATICINIT6_REVERSED("StaticExit::8F5B20", 0x008F5B20, &staticinit::Fn8F5B20);
STATICINIT6_REVERSED("StaticExit::8F5B30", 0x008F5B30, &staticinit::Fn8F5B30);
STATICINIT6_REVERSED("StaticExit::8F5B40", 0x008F5B40, &staticinit::Fn8F5B40);
STATICINIT6_REVERSED("StaticExit::8F5B50", 0x008F5B50, &staticinit::Fn8F5B50);
STATICINIT6_REVERSED("StaticExit::8F5B60", 0x008F5B60, &staticinit::Fn8F5B60);
STATICINIT6_REVERSED("StaticExit::8F5B70", 0x008F5B70, &staticinit::Fn8F5B70);
STATICINIT6_REVERSED("StaticExit::8F5B80", 0x008F5B80, &staticinit::Fn8F5B80);
STATICINIT6_REVERSED("StaticExit::8F5B90", 0x008F5B90, &staticinit::Fn8F5B90);
STATICINIT6_REVERSED("StaticExit::8F5BA0", 0x008F5BA0, &staticinit::Fn8F5BA0);
STATICINIT6_REVERSED("StaticExit::8F5BB0", 0x008F5BB0, &staticinit::Fn8F5BB0);
STATICINIT6_REVERSED("StaticExit::8F5BC0", 0x008F5BC0, &staticinit::Fn8F5BC0);
STATICINIT6_REVERSED("StaticExit::8F5BD0", 0x008F5BD0, &staticinit::Fn8F5BD0);
STATICINIT6_REVERSED("StaticExit::8F5BE0", 0x008F5BE0, &staticinit::Fn8F5BE0);
STATICINIT6_REVERSED("StaticExit::8F5BF0", 0x008F5BF0, &staticinit::Fn8F5BF0);
STATICINIT6_REVERSED("StaticExit::8F5C00", 0x008F5C00, &staticinit::Fn8F5C00);
STATICINIT6_REVERSED("StaticExit::8F5C10", 0x008F5C10, &staticinit::Fn8F5C10);
STATICINIT6_REVERSED("StaticExit::8F5C20", 0x008F5C20, &staticinit::Fn8F5C20);
STATICINIT6_REVERSED("StaticExit::8F5C30", 0x008F5C30, &staticinit::Fn8F5C30);
STATICINIT6_REVERSED("StaticExit::8F5C40", 0x008F5C40, &staticinit::Fn8F5C40);
STATICINIT6_REVERSED("StaticExit::8F5C50", 0x008F5C50, &staticinit::Fn8F5C50);
STATICINIT6_REVERSED("StaticExit::8F5C60", 0x008F5C60, &staticinit::Fn8F5C60);
STATICINIT6_REVERSED("StaticExit::8F5C70", 0x008F5C70, &staticinit::Fn8F5C70);
STATICINIT6_REVERSED("StaticExit::8F5C80", 0x008F5C80, &staticinit::Fn8F5C80);
STATICINIT6_REVERSED("StaticExit::8F5C90", 0x008F5C90, &staticinit::Fn8F5C90);
STATICINIT6_REVERSED("StaticExit::8F5CA0", 0x008F5CA0, &staticinit::Fn8F5CA0);
STATICINIT6_REVERSED("StaticExit::8F5CB0", 0x008F5CB0, &staticinit::Fn8F5CB0);
STATICINIT6_REVERSED("StaticExit::8F5CC0", 0x008F5CC0, &staticinit::Fn8F5CC0);
STATICINIT6_REVERSED("StaticExit::8F5CD0", 0x008F5CD0, &staticinit::Fn8F5CD0);
STATICINIT6_REVERSED("StaticExit::8F5D10", 0x008F5D10, &staticinit::Fn8F5D10);
STATICINIT6_REVERSED("StaticExit::8F5D20", 0x008F5D20, &staticinit::Fn8F5D20);
STATICINIT6_REVERSED("StaticExit::8F5D30", 0x008F5D30, &staticinit::Fn8F5D30);
STATICINIT6_REVERSED("StaticExit::8F5D50", 0x008F5D50, &staticinit::Fn8F5D50);
STATICINIT6_REVERSED("StaticExit::8F5D60", 0x008F5D60, &staticinit::Fn8F5D60);
STATICINIT6_REVERSED("StaticExit::8F5D70", 0x008F5D70, &staticinit::Fn8F5D70);
STATICINIT6_REVERSED("StaticExit::8F5D80", 0x008F5D80, &staticinit::Fn8F5D80);
STATICINIT6_REVERSED("StaticExit::8F5D90", 0x008F5D90, &staticinit::Fn8F5D90);
STATICINIT6_REVERSED("StaticExit::8F5DA0", 0x008F5DA0, &staticinit::Fn8F5DA0);
STATICINIT6_REVERSED("StaticExit::8F5DB0", 0x008F5DB0, &staticinit::Fn8F5DB0);
STATICINIT6_REVERSED("StaticExit::8F5DC0", 0x008F5DC0, &staticinit::Fn8F5DC0);
STATICINIT6_REVERSED("StaticExit::8F5DD0", 0x008F5DD0, &staticinit::Fn8F5DD0);
STATICINIT6_REVERSED("StaticExit::8F5DE0", 0x008F5DE0, &staticinit::Fn8F5DE0);
STATICINIT6_REVERSED("StaticExit::8F5DF0", 0x008F5DF0, &staticinit::Fn8F5DF0);
STATICINIT6_REVERSED("StaticExit::8F5E00", 0x008F5E00, &staticinit::Fn8F5E00);
STATICINIT6_REVERSED("StaticExit::8F5E10", 0x008F5E10, &staticinit::Fn8F5E10);
STATICINIT6_REVERSED("StaticExit::8F5E20", 0x008F5E20, &staticinit::Fn8F5E20);
STATICINIT6_REVERSED("StaticExit::8F5E30", 0x008F5E30, &staticinit::Fn8F5E30);
STATICINIT6_REVERSED("StaticExit::8F5E40", 0x008F5E40, &staticinit::Fn8F5E40);
STATICINIT6_REVERSED("StaticExit::8F5E50", 0x008F5E50, &staticinit::Fn8F5E50);
STATICINIT6_REVERSED("StaticExit::8F5E60", 0x008F5E60, &staticinit::Fn8F5E60);
STATICINIT6_REVERSED("StaticExit::8F5E70", 0x008F5E70, &staticinit::Fn8F5E70);
STATICINIT6_REVERSED("StaticExit::8F5E80", 0x008F5E80, &staticinit::Fn8F5E80);
STATICINIT6_REVERSED("StaticExit::8F5E90", 0x008F5E90, &staticinit::Fn8F5E90);
STATICINIT6_REVERSED("StaticExit::8F5EA0", 0x008F5EA0, &staticinit::Fn8F5EA0);
STATICINIT6_REVERSED("StaticExit::8F5EB0", 0x008F5EB0, &staticinit::Fn8F5EB0);
STATICINIT6_REVERSED("StaticExit::8F5EC0", 0x008F5EC0, &staticinit::Fn8F5EC0);
STATICINIT6_REVERSED("StaticExit::8F5ED0", 0x008F5ED0, &staticinit::Fn8F5ED0);
STATICINIT6_REVERSED("StaticExit::8F5EE0", 0x008F5EE0, &staticinit::Fn8F5EE0);
STATICINIT6_REVERSED("StaticExit::8F5EF0", 0x008F5EF0, &staticinit::Fn8F5EF0);
STATICINIT6_REVERSED("StaticExit::8F5F00", 0x008F5F00, &staticinit::Fn8F5F00);
STATICINIT6_REVERSED("StaticExit::8F5F10", 0x008F5F10, &staticinit::Fn8F5F10);
STATICINIT6_REVERSED("StaticExit::8F5F20", 0x008F5F20, &staticinit::Fn8F5F20);
STATICINIT6_REVERSED("StaticExit::8F5F30", 0x008F5F30, &staticinit::Fn8F5F30);
STATICINIT6_REVERSED("StaticExit::8F5F40", 0x008F5F40, &staticinit::Fn8F5F40);
STATICINIT6_REVERSED("StaticExit::8F5F50", 0x008F5F50, &staticinit::Fn8F5F50);
STATICINIT6_REVERSED("StaticExit::8F5F60", 0x008F5F60, &staticinit::Fn8F5F60);
STATICINIT6_REVERSED("StaticExit::8F5F70", 0x008F5F70, &staticinit::Fn8F5F70);
STATICINIT6_REVERSED("StaticExit::8F5F80", 0x008F5F80, &staticinit::Fn8F5F80);
STATICINIT6_REVERSED("StaticExit::8F5F90", 0x008F5F90, &staticinit::Fn8F5F90);
STATICINIT6_REVERSED("StaticExit::8F5FA0", 0x008F5FA0, &staticinit::Fn8F5FA0);
STATICINIT6_REVERSED("StaticExit::8F5FB0", 0x008F5FB0, &staticinit::Fn8F5FB0);
STATICINIT6_REVERSED("StaticExit::8F5FC0", 0x008F5FC0, &staticinit::Fn8F5FC0);
STATICINIT6_REVERSED("StaticExit::8F5FD0", 0x008F5FD0, &staticinit::Fn8F5FD0);
STATICINIT6_REVERSED("StaticExit::8F5FE0", 0x008F5FE0, &staticinit::Fn8F5FE0);
STATICINIT6_REVERSED("StaticExit::8F5FF0", 0x008F5FF0, &staticinit::Fn8F5FF0);
STATICINIT6_REVERSED("StaticExit::8F6000", 0x008F6000, &staticinit::Fn8F6000);
STATICINIT6_REVERSED("StaticExit::8F6010", 0x008F6010, &staticinit::Fn8F6010);
STATICINIT6_REVERSED("StaticExit::8F6020", 0x008F6020, &staticinit::Fn8F6020);
STATICINIT6_REVERSED("StaticExit::8F6030", 0x008F6030, &staticinit::Fn8F6030);
STATICINIT6_REVERSED("StaticExit::8F6040", 0x008F6040, &staticinit::Fn8F6040);
STATICINIT6_REVERSED("StaticExit::8F6050", 0x008F6050, &staticinit::Fn8F6050);
STATICINIT6_REVERSED("StaticExit::8F6060", 0x008F6060, &staticinit::Fn8F6060);
STATICINIT6_REVERSED("StaticExit::8F6070", 0x008F6070, &staticinit::Fn8F6070);
STATICINIT6_REVERSED("StaticExit::8F6080", 0x008F6080, &staticinit::Fn8F6080);
STATICINIT6_REVERSED("StaticExit::8F6090", 0x008F6090, &staticinit::Fn8F6090);
STATICINIT6_REVERSED("StaticExit::8F60A0", 0x008F60A0, &staticinit::Fn8F60A0);
STATICINIT6_REVERSED("StaticExit::8F60B0", 0x008F60B0, &staticinit::Fn8F60B0);
STATICINIT6_REVERSED("StaticExit::8F60C0", 0x008F60C0, &staticinit::Fn8F60C0);
STATICINIT6_REVERSED("StaticExit::8F60D0", 0x008F60D0, &staticinit::Fn8F60D0);
STATICINIT6_REVERSED("StaticExit::8F60E0", 0x008F60E0, &staticinit::Fn8F60E0);
STATICINIT6_REVERSED("StaticExit::8F60F0", 0x008F60F0, &staticinit::Fn8F60F0);
STATICINIT6_REVERSED("StaticExit::8F6100", 0x008F6100, &staticinit::Fn8F6100);
STATICINIT6_REVERSED("StaticExit::8F6110", 0x008F6110, &staticinit::Fn8F6110);
STATICINIT6_REVERSED("StaticExit::8F6120", 0x008F6120, &staticinit::Fn8F6120);
STATICINIT6_REVERSED("StaticExit::8F6130", 0x008F6130, &staticinit::Fn8F6130);
STATICINIT6_REVERSED("StaticExit::8F6140", 0x008F6140, &staticinit::Fn8F6140);
STATICINIT6_REVERSED("StaticExit::8F6150", 0x008F6150, &staticinit::Fn8F6150);
STATICINIT6_REVERSED("StaticExit::8F6160", 0x008F6160, &staticinit::Fn8F6160);
STATICINIT6_REVERSED("StaticExit::8F6170", 0x008F6170, &staticinit::Fn8F6170);
STATICINIT6_REVERSED("StaticExit::8F6180", 0x008F6180, &staticinit::Fn8F6180);
STATICINIT6_REVERSED("StaticExit::8F6190", 0x008F6190, &staticinit::Fn8F6190);
STATICINIT6_REVERSED("StaticExit::8F61A0", 0x008F61A0, &staticinit::Fn8F61A0);
STATICINIT6_REVERSED("StaticExit::8F61B0", 0x008F61B0, &staticinit::Fn8F61B0);
STATICINIT6_REVERSED("StaticExit::8F61C0", 0x008F61C0, &staticinit::Fn8F61C0);
STATICINIT6_REVERSED("StaticExit::8F61D0", 0x008F61D0, &staticinit::Fn8F61D0);
STATICINIT6_REVERSED("StaticExit::8F61E0", 0x008F61E0, &staticinit::Fn8F61E0);
STATICINIT6_REVERSED("StaticExit::8F61F0", 0x008F61F0, &staticinit::Fn8F61F0);
STATICINIT6_REVERSED("StaticExit::8F6200", 0x008F6200, &staticinit::Fn8F6200);
STATICINIT6_REVERSED("StaticExit::8F6210", 0x008F6210, &staticinit::Fn8F6210);
STATICINIT6_REVERSED("StaticExit::8F6220", 0x008F6220, &staticinit::Fn8F6220);
STATICINIT6_REVERSED("StaticExit::8F6230", 0x008F6230, &staticinit::Fn8F6230);
STATICINIT6_REVERSED("StaticExit::8F6240", 0x008F6240, &staticinit::Fn8F6240);
STATICINIT6_REVERSED("StaticExit::8F6250", 0x008F6250, &staticinit::Fn8F6250);
STATICINIT6_REVERSED("StaticExit::8F6260", 0x008F6260, &staticinit::Fn8F6260);
STATICINIT6_REVERSED("StaticExit::8F6270", 0x008F6270, &staticinit::Fn8F6270);
STATICINIT6_REVERSED("StaticExit::8F6280", 0x008F6280, &staticinit::Fn8F6280);
STATICINIT6_REVERSED("StaticExit::8F62A0", 0x008F62A0, &staticinit::Fn8F62A0);
STATICINIT6_REVERSED("StaticExit::8F62B0", 0x008F62B0, &staticinit::Fn8F62B0);
STATICINIT6_REVERSED("StaticExit::8F62C0", 0x008F62C0, &staticinit::Fn8F62C0);
STATICINIT6_REVERSED("StaticExit::8F62D0", 0x008F62D0, &staticinit::Fn8F62D0);
STATICINIT6_REVERSED("StaticExit::8F62E0", 0x008F62E0, &staticinit::Fn8F62E0);
STATICINIT6_REVERSED("StaticExit::8F62F0", 0x008F62F0, &staticinit::Fn8F62F0);
STATICINIT6_REVERSED("StaticExit::8F6300", 0x008F6300, &staticinit::Fn8F6300);
STATICINIT6_REVERSED("StaticExit::8F6310", 0x008F6310, &staticinit::Fn8F6310);
STATICINIT6_REVERSED("StaticExit::8F6320", 0x008F6320, &staticinit::Fn8F6320);
STATICINIT6_REVERSED("StaticExit::8F6330", 0x008F6330, &staticinit::Fn8F6330);
STATICINIT6_REVERSED("StaticExit::8F6340", 0x008F6340, &staticinit::Fn8F6340);
STATICINIT6_REVERSED("StaticExit::8F6350", 0x008F6350, &staticinit::Fn8F6350);
STATICINIT6_REVERSED("StaticExit::8F6360", 0x008F6360, &staticinit::Fn8F6360);
STATICINIT6_REVERSED("StaticExit::8F6370", 0x008F6370, &staticinit::Fn8F6370);
STATICINIT6_REVERSED("StaticExit::8F6380", 0x008F6380, &staticinit::Fn8F6380);
STATICINIT6_REVERSED("StaticExit::8F6390", 0x008F6390, &staticinit::Fn8F6390);
STATICINIT6_REVERSED("StaticExit::8F63A0", 0x008F63A0, &staticinit::Fn8F63A0);
STATICINIT6_REVERSED("StaticExit::8F63B0", 0x008F63B0, &staticinit::Fn8F63B0);
STATICINIT6_REVERSED("StaticExit::8F63C0", 0x008F63C0, &staticinit::Fn8F63C0);
STATICINIT6_REVERSED("StaticExit::8F63D0", 0x008F63D0, &staticinit::Fn8F63D0);
STATICINIT6_REVERSED("StaticExit::8F63E0", 0x008F63E0, &staticinit::Fn8F63E0);
STATICINIT6_REVERSED("StaticExit::8F63F0", 0x008F63F0, &staticinit::Fn8F63F0);
STATICINIT6_REVERSED("StaticExit::8F6400", 0x008F6400, &staticinit::Fn8F6400);
STATICINIT6_REVERSED("StaticExit::8F6410", 0x008F6410, &staticinit::Fn8F6410);
STATICINIT6_REVERSED("StaticExit::8F6420", 0x008F6420, &staticinit::Fn8F6420);
STATICINIT6_REVERSED("StaticExit::8F6430", 0x008F6430, &staticinit::Fn8F6430);
STATICINIT6_REVERSED("StaticExit::8F6440", 0x008F6440, &staticinit::Fn8F6440);
STATICINIT6_REVERSED("StaticExit::8F6450", 0x008F6450, &staticinit::Fn8F6450);
STATICINIT6_REVERSED("StaticExit::8F6460", 0x008F6460, &staticinit::Fn8F6460);
STATICINIT6_REVERSED("StaticExit::8F6470", 0x008F6470, &staticinit::Fn8F6470);
STATICINIT6_REVERSED("StaticExit::8F6480", 0x008F6480, &staticinit::Fn8F6480);
STATICINIT6_REVERSED("StaticExit::8F6490", 0x008F6490, &staticinit::Fn8F6490);
STATICINIT6_REVERSED("StaticExit::8F64A0", 0x008F64A0, &staticinit::Fn8F64A0);
STATICINIT6_REVERSED("StaticExit::8F64B0", 0x008F64B0, &staticinit::Fn8F64B0);
STATICINIT6_REVERSED("StaticExit::8F64C0", 0x008F64C0, &staticinit::Fn8F64C0);
STATICINIT6_REVERSED("StaticExit::8F64D0", 0x008F64D0, &staticinit::Fn8F64D0);
STATICINIT6_REVERSED("StaticExit::8F64E0", 0x008F64E0, &staticinit::Fn8F64E0);
STATICINIT6_REVERSED("StaticExit::8F64F0", 0x008F64F0, &staticinit::Fn8F64F0);
STATICINIT6_REVERSED("StaticExit::8F6500", 0x008F6500, &staticinit::Fn8F6500);
STATICINIT6_REVERSED("StaticExit::8F6510", 0x008F6510, &staticinit::Fn8F6510);
STATICINIT6_REVERSED("StaticExit::8F6520", 0x008F6520, &staticinit::Fn8F6520);
STATICINIT6_REVERSED("StaticExit::8F6530", 0x008F6530, &staticinit::Fn8F6530);
STATICINIT6_REVERSED("StaticExit::8F6540", 0x008F6540, &staticinit::Fn8F6540);
STATICINIT6_REVERSED("StaticExit::8F6550", 0x008F6550, &staticinit::Fn8F6550);
STATICINIT6_REVERSED("StaticExit::8F6560", 0x008F6560, &staticinit::Fn8F6560);
STATICINIT6_REVERSED("StaticExit::8F6570", 0x008F6570, &staticinit::Fn8F6570);
STATICINIT6_REVERSED("StaticExit::8F6580", 0x008F6580, &staticinit::Fn8F6580);
STATICINIT6_REVERSED("StaticExit::8F6590", 0x008F6590, &staticinit::Fn8F6590);
STATICINIT6_REVERSED("StaticExit::8F65A0", 0x008F65A0, &staticinit::Fn8F65A0);
STATICINIT6_REVERSED("StaticExit::8F65B0", 0x008F65B0, &staticinit::Fn8F65B0);
STATICINIT6_REVERSED("StaticExit::8F65C0", 0x008F65C0, &staticinit::Fn8F65C0);
STATICINIT6_REVERSED("StaticExit::8F65D0", 0x008F65D0, &staticinit::Fn8F65D0);
STATICINIT6_REVERSED("StaticExit::8F65E0", 0x008F65E0, &staticinit::Fn8F65E0);
STATICINIT6_REVERSED("StaticExit::8F65F0", 0x008F65F0, &staticinit::Fn8F65F0);
STATICINIT6_REVERSED("StaticExit::8F6600", 0x008F6600, &staticinit::Fn8F6600);
STATICINIT6_REVERSED("StaticExit::8F6610", 0x008F6610, &staticinit::Fn8F6610);
STATICINIT6_REVERSED("StaticExit::8F6620", 0x008F6620, &staticinit::Fn8F6620);
STATICINIT6_REVERSED("StaticExit::8F6630", 0x008F6630, &staticinit::Fn8F6630);
STATICINIT6_REVERSED("StaticExit::8F6640", 0x008F6640, &staticinit::Fn8F6640);
STATICINIT6_REVERSED("StaticExit::8F6650", 0x008F6650, &staticinit::Fn8F6650);
STATICINIT6_REVERSED("StaticExit::8F6660", 0x008F6660, &staticinit::Fn8F6660);
STATICINIT6_REVERSED("StaticExit::8F6670", 0x008F6670, &staticinit::Fn8F6670);
STATICINIT6_REVERSED("StaticExit::8F6680", 0x008F6680, &staticinit::Fn8F6680);
STATICINIT6_REVERSED("StaticExit::8F6690", 0x008F6690, &staticinit::Fn8F6690);
STATICINIT6_REVERSED("StaticExit::8F66A0", 0x008F66A0, &staticinit::Fn8F66A0);
STATICINIT6_REVERSED("StaticExit::8F66B0", 0x008F66B0, &staticinit::Fn8F66B0);
STATICINIT6_REVERSED("StaticExit::8F66C0", 0x008F66C0, &staticinit::Fn8F66C0);
STATICINIT6_REVERSED("StaticExit::8F66D0", 0x008F66D0, &staticinit::Fn8F66D0);
STATICINIT6_REVERSED("StaticExit::8F66E0", 0x008F66E0, &staticinit::Fn8F66E0);
STATICINIT6_REVERSED("StaticExit::8F66F0", 0x008F66F0, &staticinit::Fn8F66F0);
STATICINIT6_REVERSED("StaticExit::8F6700", 0x008F6700, &staticinit::Fn8F6700);
STATICINIT6_REVERSED("StaticExit::8F6710", 0x008F6710, &staticinit::Fn8F6710);
STATICINIT6_REVERSED("StaticExit::8F6720", 0x008F6720, &staticinit::Fn8F6720);
STATICINIT6_REVERSED("StaticExit::8F6730", 0x008F6730, &staticinit::Fn8F6730);
STATICINIT6_REVERSED("StaticExit::8F6740", 0x008F6740, &staticinit::Fn8F6740);
STATICINIT6_REVERSED("StaticExit::8F6750", 0x008F6750, &staticinit::Fn8F6750);
STATICINIT6_REVERSED("StaticExit::8F6760", 0x008F6760, &staticinit::Fn8F6760);
STATICINIT6_REVERSED("StaticExit::8F6770", 0x008F6770, &staticinit::Fn8F6770);
STATICINIT6_REVERSED("StaticExit::8F6780", 0x008F6780, &staticinit::Fn8F6780);
STATICINIT6_REVERSED("StaticExit::8F6790", 0x008F6790, &staticinit::Fn8F6790);
STATICINIT6_REVERSED("StaticExit::8F67A0", 0x008F67A0, &staticinit::Fn8F67A0);
STATICINIT6_REVERSED("StaticExit::8F67B0", 0x008F67B0, &staticinit::Fn8F67B0);
STATICINIT6_REVERSED("StaticExit::8F67C0", 0x008F67C0, &staticinit::Fn8F67C0);
STATICINIT6_REVERSED("StaticExit::8F67D0", 0x008F67D0, &staticinit::Fn8F67D0);
STATICINIT6_REVERSED("StaticExit::8F67E0", 0x008F67E0, &staticinit::Fn8F67E0);
STATICINIT6_REVERSED("StaticExit::8F67F0", 0x008F67F0, &staticinit::Fn8F67F0);
STATICINIT6_REVERSED("StaticExit::8F6800", 0x008F6800, &staticinit::Fn8F6800);
STATICINIT6_REVERSED("StaticExit::8F6810", 0x008F6810, &staticinit::Fn8F6810);
STATICINIT6_REVERSED("StaticExit::8F6820", 0x008F6820, &staticinit::Fn8F6820);
STATICINIT6_REVERSED("StaticExit::8F6830", 0x008F6830, &staticinit::Fn8F6830);
STATICINIT6_REVERSED("StaticExit::8F6840", 0x008F6840, &staticinit::Fn8F6840);
STATICINIT6_REVERSED("StaticExit::8F6850", 0x008F6850, &staticinit::Fn8F6850);
STATICINIT6_REVERSED("StaticExit::8F6860", 0x008F6860, &staticinit::Fn8F6860);
STATICINIT6_REVERSED("StaticExit::8F6870", 0x008F6870, &staticinit::Fn8F6870);
STATICINIT6_REVERSED("StaticExit::8F6880", 0x008F6880, &staticinit::Fn8F6880);
STATICINIT6_REVERSED("StaticExit::8F6890", 0x008F6890, &staticinit::Fn8F6890);
STATICINIT6_REVERSED("StaticExit::8F68A0", 0x008F68A0, &staticinit::Fn8F68A0);
STATICINIT6_REVERSED("StaticExit::8F68B0", 0x008F68B0, &staticinit::Fn8F68B0);
STATICINIT6_REVERSED("StaticExit::8F68C0", 0x008F68C0, &staticinit::Fn8F68C0);
STATICINIT6_REVERSED("StaticExit::8F68D0", 0x008F68D0, &staticinit::Fn8F68D0);
STATICINIT6_REVERSED("StaticExit::8F68E0", 0x008F68E0, &staticinit::Fn8F68E0);
STATICINIT6_REVERSED("StaticExit::8F68F0", 0x008F68F0, &staticinit::Fn8F68F0);
STATICINIT6_REVERSED("StaticExit::8F6900", 0x008F6900, &staticinit::Fn8F6900);
STATICINIT6_REVERSED("StaticExit::8F6910", 0x008F6910, &staticinit::Fn8F6910);
STATICINIT6_REVERSED("StaticExit::8F6920", 0x008F6920, &staticinit::Fn8F6920);
STATICINIT6_REVERSED("StaticExit::8F6930", 0x008F6930, &staticinit::Fn8F6930);
STATICINIT6_REVERSED("StaticExit::8F6940", 0x008F6940, &staticinit::Fn8F6940);
STATICINIT6_REVERSED("StaticExit::8F6950", 0x008F6950, &staticinit::Fn8F6950);
STATICINIT6_REVERSED("StaticExit::8F6960", 0x008F6960, &staticinit::Fn8F6960);
STATICINIT6_REVERSED("StaticExit::8F6970", 0x008F6970, &staticinit::Fn8F6970);
STATICINIT6_REVERSED("StaticExit::8F6980", 0x008F6980, &staticinit::Fn8F6980);
STATICINIT6_REVERSED("StaticExit::8F6990", 0x008F6990, &staticinit::Fn8F6990);
STATICINIT6_REVERSED("StaticExit::8F69A0", 0x008F69A0, &staticinit::Fn8F69A0);
STATICINIT6_REVERSED("StaticExit::8F69B0", 0x008F69B0, &staticinit::Fn8F69B0);
STATICINIT6_REVERSED("StaticExit::8F69C0", 0x008F69C0, &staticinit::Fn8F69C0);
STATICINIT6_REVERSED("StaticExit::8F69D0", 0x008F69D0, &staticinit::Fn8F69D0);
STATICINIT6_REVERSED("StaticExit::8F69E0", 0x008F69E0, &staticinit::Fn8F69E0);
STATICINIT6_REVERSED("StaticExit::8F69F0", 0x008F69F0, &staticinit::Fn8F69F0);
STATICINIT6_REVERSED("StaticExit::8F6A00", 0x008F6A00, &staticinit::Fn8F6A00);
STATICINIT6_REVERSED("StaticExit::8F6A10", 0x008F6A10, &staticinit::Fn8F6A10);
STATICINIT6_REVERSED("StaticExit::8F6A20", 0x008F6A20, &staticinit::Fn8F6A20);
STATICINIT6_REVERSED("StaticExit::8F6A30", 0x008F6A30, &staticinit::Fn8F6A30);
STATICINIT6_REVERSED("StaticExit::8F6A40", 0x008F6A40, &staticinit::Fn8F6A40);
STATICINIT6_REVERSED("StaticExit::8F6A50", 0x008F6A50, &staticinit::Fn8F6A50);
STATICINIT6_REVERSED("StaticExit::8F6A60", 0x008F6A60, &staticinit::Fn8F6A60);
STATICINIT6_REVERSED("StaticExit::8F6A70", 0x008F6A70, &staticinit::Fn8F6A70);
STATICINIT6_REVERSED("StaticExit::8F6A80", 0x008F6A80, &staticinit::Fn8F6A80);
STATICINIT6_REVERSED("StaticExit::8F6A90", 0x008F6A90, &staticinit::Fn8F6A90);
STATICINIT6_REVERSED("StaticExit::8F6AA0", 0x008F6AA0, &staticinit::Fn8F6AA0);
STATICINIT6_REVERSED("StaticExit::8F6AB0", 0x008F6AB0, &staticinit::Fn8F6AB0);
STATICINIT6_REVERSED("StaticExit::8F6AC0", 0x008F6AC0, &staticinit::Fn8F6AC0);
STATICINIT6_REVERSED("StaticExit::8F6AD0", 0x008F6AD0, &staticinit::Fn8F6AD0);
STATICINIT6_REVERSED("StaticExit::8F6AE0", 0x008F6AE0, &staticinit::Fn8F6AE0);
STATICINIT6_REVERSED("StaticExit::8F6AF0", 0x008F6AF0, &staticinit::Fn8F6AF0);
STATICINIT6_REVERSED("StaticExit::8F6B00", 0x008F6B00, &staticinit::Fn8F6B00);
STATICINIT6_REVERSED("StaticExit::8F6B10", 0x008F6B10, &staticinit::Fn8F6B10);
STATICINIT6_REVERSED("StaticExit::8F6B20", 0x008F6B20, &staticinit::Fn8F6B20);
STATICINIT6_REVERSED("StaticExit::8F6B30", 0x008F6B30, &staticinit::Fn8F6B30);
STATICINIT6_REVERSED("StaticExit::8F6B40", 0x008F6B40, &staticinit::Fn8F6B40);
STATICINIT6_REVERSED("StaticExit::8F6B50", 0x008F6B50, &staticinit::Fn8F6B50);
STATICINIT6_REVERSED("StaticExit::8F6B60", 0x008F6B60, &staticinit::Fn8F6B60);
STATICINIT6_REVERSED("StaticExit::8F6B70", 0x008F6B70, &staticinit::Fn8F6B70);
STATICINIT6_REVERSED("StaticExit::8F6B80", 0x008F6B80, &staticinit::Fn8F6B80);
STATICINIT6_REVERSED("StaticExit::8F6B90", 0x008F6B90, &staticinit::Fn8F6B90);
STATICINIT6_REVERSED("StaticExit::8F6BA0", 0x008F6BA0, &staticinit::Fn8F6BA0);
STATICINIT6_REVERSED("StaticExit::8F6BB0", 0x008F6BB0, &staticinit::Fn8F6BB0);
STATICINIT6_REVERSED("StaticExit::8F6BC0", 0x008F6BC0, &staticinit::Fn8F6BC0);
STATICINIT6_REVERSED("StaticExit::8F6BD0", 0x008F6BD0, &staticinit::Fn8F6BD0);
STATICINIT6_REVERSED("StaticExit::8F6BE0", 0x008F6BE0, &staticinit::Fn8F6BE0);
STATICINIT6_REVERSED("StaticExit::8F6BF0", 0x008F6BF0, &staticinit::Fn8F6BF0);
STATICINIT6_REVERSED("StaticExit::8F6C00", 0x008F6C00, &staticinit::Fn8F6C00);
STATICINIT6_REVERSED("StaticExit::8F6C10", 0x008F6C10, &staticinit::Fn8F6C10);
STATICINIT6_REVERSED("StaticExit::8F6C20", 0x008F6C20, &staticinit::Fn8F6C20);
STATICINIT6_REVERSED("StaticExit::8F6C30", 0x008F6C30, &staticinit::Fn8F6C30);
STATICINIT6_REVERSED("StaticExit::8F6C40", 0x008F6C40, &staticinit::Fn8F6C40);
STATICINIT6_REVERSED("StaticExit::8F6C50", 0x008F6C50, &staticinit::Fn8F6C50);
STATICINIT6_REVERSED("StaticExit::8F6C60", 0x008F6C60, &staticinit::Fn8F6C60);
STATICINIT6_REVERSED("StaticExit::8F6C70", 0x008F6C70, &staticinit::Fn8F6C70);
STATICINIT6_REVERSED("StaticExit::8F6C80", 0x008F6C80, &staticinit::Fn8F6C80);
STATICINIT6_REVERSED("StaticExit::8F6C90", 0x008F6C90, &staticinit::Fn8F6C90);
STATICINIT6_REVERSED("StaticExit::8F6CA0", 0x008F6CA0, &staticinit::Fn8F6CA0);
STATICINIT6_REVERSED("StaticExit::8F6CB0", 0x008F6CB0, &staticinit::Fn8F6CB0);
STATICINIT6_REVERSED("StaticExit::8F6CC0", 0x008F6CC0, &staticinit::Fn8F6CC0);
STATICINIT6_REVERSED("StaticExit::8F6CD0", 0x008F6CD0, &staticinit::Fn8F6CD0);
STATICINIT6_REVERSED("StaticExit::8F6CE0", 0x008F6CE0, &staticinit::Fn8F6CE0);
STATICINIT6_REVERSED("StaticExit::8F6CF0", 0x008F6CF0, &staticinit::Fn8F6CF0);
STATICINIT6_REVERSED("StaticExit::8F6D00", 0x008F6D00, &staticinit::Fn8F6D00);
STATICINIT6_REVERSED("StaticExit::8F6D10", 0x008F6D10, &staticinit::Fn8F6D10);
STATICINIT6_REVERSED("StaticExit::8F6D20", 0x008F6D20, &staticinit::Fn8F6D20);
STATICINIT6_REVERSED("StaticExit::8F6D30", 0x008F6D30, &staticinit::Fn8F6D30);
STATICINIT6_REVERSED("StaticExit::8F6D40", 0x008F6D40, &staticinit::Fn8F6D40);
STATICINIT6_REVERSED("StaticExit::8F6D50", 0x008F6D50, &staticinit::Fn8F6D50);
STATICINIT6_REVERSED("StaticExit::8F6D60", 0x008F6D60, &staticinit::Fn8F6D60);
STATICINIT6_REVERSED("StaticExit::8F6D70", 0x008F6D70, &staticinit::Fn8F6D70);
STATICINIT6_REVERSED("StaticExit::8F6D80", 0x008F6D80, &staticinit::Fn8F6D80);
STATICINIT6_REVERSED("StaticExit::8F6D90", 0x008F6D90, &staticinit::Fn8F6D90);
STATICINIT6_REVERSED("StaticExit::8F6DA0", 0x008F6DA0, &staticinit::Fn8F6DA0);
STATICINIT6_REVERSED("StaticExit::8F6DB0", 0x008F6DB0, &staticinit::Fn8F6DB0);
STATICINIT6_REVERSED("StaticExit::8F6DC0", 0x008F6DC0, &staticinit::Fn8F6DC0);
STATICINIT6_REVERSED("StaticExit::8F6DD0", 0x008F6DD0, &staticinit::Fn8F6DD0);
STATICINIT6_REVERSED("StaticExit::8F6DE0", 0x008F6DE0, &staticinit::Fn8F6DE0);
STATICINIT6_REVERSED("StaticExit::8F6DF0", 0x008F6DF0, &staticinit::Fn8F6DF0);
STATICINIT6_REVERSED("StaticExit::8F6E00", 0x008F6E00, &staticinit::Fn8F6E00);
STATICINIT6_REVERSED("StaticExit::8F6E10", 0x008F6E10, &staticinit::Fn8F6E10);
STATICINIT6_REVERSED("StaticExit::8F6E20", 0x008F6E20, &staticinit::Fn8F6E20);
STATICINIT6_REVERSED("StaticExit::8F6E30", 0x008F6E30, &staticinit::Fn8F6E30);
STATICINIT6_REVERSED("StaticExit::8F6E40", 0x008F6E40, &staticinit::Fn8F6E40);
STATICINIT6_REVERSED("StaticExit::8F6E50", 0x008F6E50, &staticinit::Fn8F6E50);
STATICINIT6_REVERSED("StaticExit::8F6E60", 0x008F6E60, &staticinit::Fn8F6E60);
STATICINIT6_REVERSED("StaticExit::8F6E70", 0x008F6E70, &staticinit::Fn8F6E70);
STATICINIT6_REVERSED("StaticExit::8F6E80", 0x008F6E80, &staticinit::Fn8F6E80);
STATICINIT6_REVERSED("StaticExit::8F6E90", 0x008F6E90, &staticinit::Fn8F6E90);
STATICINIT6_REVERSED("StaticExit::8F6EA0", 0x008F6EA0, &staticinit::Fn8F6EA0);
STATICINIT6_REVERSED("StaticExit::8F6EB0", 0x008F6EB0, &staticinit::Fn8F6EB0);
STATICINIT6_REVERSED("StaticExit::8F6EC0", 0x008F6EC0, &staticinit::Fn8F6EC0);
STATICINIT6_REVERSED("StaticExit::8F6ED0", 0x008F6ED0, &staticinit::Fn8F6ED0);
STATICINIT6_REVERSED("StaticExit::8F6EE0", 0x008F6EE0, &staticinit::Fn8F6EE0);
STATICINIT6_REVERSED("StaticExit::8F6EF0", 0x008F6EF0, &staticinit::Fn8F6EF0);
STATICINIT6_REVERSED("StaticExit::8F6F00", 0x008F6F00, &staticinit::Fn8F6F00);
STATICINIT6_REVERSED("StaticExit::8F6F10", 0x008F6F10, &staticinit::Fn8F6F10);
STATICINIT6_REVERSED("StaticExit::8F6F20", 0x008F6F20, &staticinit::Fn8F6F20);
STATICINIT6_REVERSED("StaticExit::8F6F30", 0x008F6F30, &staticinit::Fn8F6F30);
STATICINIT6_REVERSED("StaticExit::8F6F40", 0x008F6F40, &staticinit::Fn8F6F40);
STATICINIT6_REVERSED("StaticExit::8F6F50", 0x008F6F50, &staticinit::Fn8F6F50);
STATICINIT6_REVERSED("StaticExit::8F6F60", 0x008F6F60, &staticinit::Fn8F6F60);
STATICINIT6_REVERSED("StaticExit::8F6F70", 0x008F6F70, &staticinit::Fn8F6F70);
STATICINIT6_REVERSED("StaticExit::8F6F80", 0x008F6F80, &staticinit::Fn8F6F80);
STATICINIT6_REVERSED("StaticExit::8F6F90", 0x008F6F90, &staticinit::Fn8F6F90);
STATICINIT6_REVERSED("StaticExit::8F6FA0", 0x008F6FA0, &staticinit::Fn8F6FA0);
STATICINIT6_REVERSED("StaticExit::8F6FB0", 0x008F6FB0, &staticinit::Fn8F6FB0);
STATICINIT6_REVERSED("StaticExit::8F6FC0", 0x008F6FC0, &staticinit::Fn8F6FC0);
STATICINIT6_REVERSED("StaticExit::8F6FD0", 0x008F6FD0, &staticinit::Fn8F6FD0);
STATICINIT6_REVERSED("StaticExit::8F6FE0", 0x008F6FE0, &staticinit::Fn8F6FE0);
STATICINIT6_REVERSED("StaticExit::8F6FF0", 0x008F6FF0, &staticinit::Fn8F6FF0);
STATICINIT6_REVERSED("StaticExit::8F7000", 0x008F7000, &staticinit::Fn8F7000);
STATICINIT6_REVERSED("StaticExit::8F7010", 0x008F7010, &staticinit::Fn8F7010);
STATICINIT6_REVERSED("StaticExit::8F7020", 0x008F7020, &staticinit::Fn8F7020);
STATICINIT6_REVERSED("StaticExit::8F7030", 0x008F7030, &staticinit::Fn8F7030);
STATICINIT6_REVERSED("StaticExit::8F7040", 0x008F7040, &staticinit::Fn8F7040);
STATICINIT6_REVERSED("StaticExit::8F7050", 0x008F7050, &staticinit::Fn8F7050);
STATICINIT6_REVERSED("StaticExit::8F7060", 0x008F7060, &staticinit::Fn8F7060);
STATICINIT6_REVERSED("StaticExit::8F7070", 0x008F7070, &staticinit::Fn8F7070);
STATICINIT6_REVERSED("StaticExit::8F7080", 0x008F7080, &staticinit::Fn8F7080);
STATICINIT6_REVERSED("StaticExit::8F7090", 0x008F7090, &staticinit::Fn8F7090);
STATICINIT6_REVERSED("StaticExit::8F70A0", 0x008F70A0, &staticinit::Fn8F70A0);
STATICINIT6_REVERSED("StaticExit::8F70B0", 0x008F70B0, &staticinit::Fn8F70B0);
STATICINIT6_REVERSED("StaticExit::8F70C0", 0x008F70C0, &staticinit::Fn8F70C0);
STATICINIT6_REVERSED("StaticExit::8F70D0", 0x008F70D0, &staticinit::Fn8F70D0);
STATICINIT6_REVERSED("StaticExit::8F70E0", 0x008F70E0, &staticinit::Fn8F70E0);
STATICINIT6_REVERSED("StaticExit::8F70F0", 0x008F70F0, &staticinit::Fn8F70F0);
STATICINIT6_REVERSED("StaticExit::8F7100", 0x008F7100, &staticinit::Fn8F7100);
STATICINIT6_REVERSED("StaticExit::8F7110", 0x008F7110, &staticinit::Fn8F7110);
STATICINIT6_REVERSED("StaticExit::8F7120", 0x008F7120, &staticinit::Fn8F7120);
STATICINIT6_REVERSED("StaticExit::8F7130", 0x008F7130, &staticinit::Fn8F7130);
STATICINIT6_REVERSED("StaticExit::8F7140", 0x008F7140, &staticinit::Fn8F7140);
STATICINIT6_REVERSED("StaticExit::8F7150", 0x008F7150, &staticinit::Fn8F7150);
STATICINIT6_REVERSED("StaticExit::8F7160", 0x008F7160, &staticinit::Fn8F7160);
STATICINIT6_REVERSED("StaticExit::8F7170", 0x008F7170, &staticinit::Fn8F7170);
STATICINIT6_REVERSED("StaticExit::8F7180", 0x008F7180, &staticinit::Fn8F7180);
STATICINIT6_REVERSED("StaticExit::8F7190", 0x008F7190, &staticinit::Fn8F7190);
STATICINIT6_REVERSED("StaticExit::8F71A0", 0x008F71A0, &staticinit::Fn8F71A0);
STATICINIT6_REVERSED("StaticExit::8F71B0", 0x008F71B0, &staticinit::Fn8F71B0);
STATICINIT6_REVERSED("StaticExit::8F71C0", 0x008F71C0, &staticinit::Fn8F71C0);
STATICINIT6_REVERSED("StaticExit::8F71D0", 0x008F71D0, &staticinit::Fn8F71D0);
STATICINIT6_REVERSED("StaticExit::8F71E0", 0x008F71E0, &staticinit::Fn8F71E0);
STATICINIT6_REVERSED("StaticExit::8F71F0", 0x008F71F0, &staticinit::Fn8F71F0);
STATICINIT6_REVERSED("StaticExit::8F7200", 0x008F7200, &staticinit::Fn8F7200);
STATICINIT6_REVERSED("StaticExit::8F7210", 0x008F7210, &staticinit::Fn8F7210);
STATICINIT6_REVERSED("StaticExit::8F7220", 0x008F7220, &staticinit::Fn8F7220);
STATICINIT6_REVERSED("StaticExit::8F7230", 0x008F7230, &staticinit::Fn8F7230);
STATICINIT6_REVERSED("StaticExit::8F7240", 0x008F7240, &staticinit::Fn8F7240);
STATICINIT6_REVERSED("StaticExit::8F7250", 0x008F7250, &staticinit::Fn8F7250);
STATICINIT6_REVERSED("StaticExit::8F7260", 0x008F7260, &staticinit::Fn8F7260);
STATICINIT6_REVERSED("StaticExit::8F7270", 0x008F7270, &staticinit::Fn8F7270);
STATICINIT6_REVERSED("StaticExit::8F7280", 0x008F7280, &staticinit::Fn8F7280);
STATICINIT6_REVERSED("StaticExit::8F7290", 0x008F7290, &staticinit::Fn8F7290);
STATICINIT6_REVERSED("StaticExit::8F72A0", 0x008F72A0, &staticinit::Fn8F72A0);
STATICINIT6_REVERSED("StaticExit::8F72B0", 0x008F72B0, &staticinit::Fn8F72B0);
STATICINIT6_REVERSED("StaticExit::8F72C0", 0x008F72C0, &staticinit::Fn8F72C0);
STATICINIT6_REVERSED("StaticExit::8F72D0", 0x008F72D0, &staticinit::Fn8F72D0);
STATICINIT6_REVERSED("StaticExit::8F72E0", 0x008F72E0, &staticinit::Fn8F72E0);
STATICINIT6_REVERSED("StaticExit::8F72F0", 0x008F72F0, &staticinit::Fn8F72F0);
STATICINIT6_REVERSED("StaticExit::8F7300", 0x008F7300, &staticinit::Fn8F7300);
STATICINIT6_REVERSED("StaticExit::8F7310", 0x008F7310, &staticinit::Fn8F7310);
STATICINIT6_REVERSED("StaticExit::8F7320", 0x008F7320, &staticinit::Fn8F7320);
STATICINIT6_REVERSED("StaticExit::8F7330", 0x008F7330, &staticinit::Fn8F7330);
STATICINIT6_REVERSED("StaticExit::8F7340", 0x008F7340, &staticinit::Fn8F7340);
STATICINIT6_REVERSED("StaticExit::8F7350", 0x008F7350, &staticinit::Fn8F7350);
STATICINIT6_REVERSED("StaticExit::8F7360", 0x008F7360, &staticinit::Fn8F7360);
STATICINIT6_REVERSED("StaticExit::8F7370", 0x008F7370, &staticinit::Fn8F7370);
STATICINIT6_REVERSED("StaticExit::8F7380", 0x008F7380, &staticinit::Fn8F7380);
STATICINIT6_REVERSED("StaticExit::8F7390", 0x008F7390, &staticinit::Fn8F7390);
STATICINIT6_REVERSED("StaticExit::8F73A0", 0x008F73A0, &staticinit::Fn8F73A0);
STATICINIT6_REVERSED("StaticExit::8F73B0", 0x008F73B0, &staticinit::Fn8F73B0);
STATICINIT6_REVERSED("StaticExit::8F73C0", 0x008F73C0, &staticinit::Fn8F73C0);
STATICINIT6_REVERSED("StaticExit::8F73D0", 0x008F73D0, &staticinit::Fn8F73D0);
STATICINIT6_REVERSED("StaticExit::8F73E0", 0x008F73E0, &staticinit::Fn8F73E0);
STATICINIT6_REVERSED("StaticExit::8F73F0", 0x008F73F0, &staticinit::Fn8F73F0);
STATICINIT6_REVERSED("StaticExit::8F7400", 0x008F7400, &staticinit::Fn8F7400);
STATICINIT6_REVERSED("StaticExit::8F7410", 0x008F7410, &staticinit::Fn8F7410);
STATICINIT6_REVERSED("StaticExit::8F7420", 0x008F7420, &staticinit::Fn8F7420);
STATICINIT6_REVERSED("StaticExit::8F7430", 0x008F7430, &staticinit::Fn8F7430);
STATICINIT6_REVERSED("StaticExit::8F7440", 0x008F7440, &staticinit::Fn8F7440);
STATICINIT6_REVERSED("StaticExit::8F7450", 0x008F7450, &staticinit::Fn8F7450);
STATICINIT6_REVERSED("StaticExit::8F7460", 0x008F7460, &staticinit::Fn8F7460);
STATICINIT6_REVERSED("StaticExit::8F7470", 0x008F7470, &staticinit::Fn8F7470);
STATICINIT6_REVERSED("StaticExit::8F7480", 0x008F7480, &staticinit::Fn8F7480);
STATICINIT6_REVERSED("StaticExit::8F7490", 0x008F7490, &staticinit::Fn8F7490);
STATICINIT6_REVERSED("StaticExit::8F74A0", 0x008F74A0, &staticinit::Fn8F74A0);
STATICINIT6_REVERSED("StaticExit::8F74B0", 0x008F74B0, &staticinit::Fn8F74B0);
STATICINIT6_REVERSED("StaticExit::8F74C0", 0x008F74C0, &staticinit::Fn8F74C0);
STATICINIT6_REVERSED("StaticExit::8F74D0", 0x008F74D0, &staticinit::Fn8F74D0);
STATICINIT6_REVERSED("StaticExit::8F74E0", 0x008F74E0, &staticinit::Fn8F74E0);
STATICINIT6_REVERSED("StaticExit::8F74F0", 0x008F74F0, &staticinit::Fn8F74F0);
STATICINIT6_REVERSED("StaticExit::8F7500", 0x008F7500, &staticinit::Fn8F7500);
STATICINIT6_REVERSED("StaticExit::8F7510", 0x008F7510, &staticinit::Fn8F7510);
STATICINIT6_REVERSED("StaticExit::8F7520", 0x008F7520, &staticinit::Fn8F7520);
STATICINIT6_REVERSED("StaticExit::8F7530", 0x008F7530, &staticinit::Fn8F7530);
STATICINIT6_REVERSED("StaticExit::8F7540", 0x008F7540, &staticinit::Fn8F7540);
STATICINIT6_REVERSED("StaticExit::8F7550", 0x008F7550, &staticinit::Fn8F7550);
STATICINIT6_REVERSED("StaticExit::8F7560", 0x008F7560, &staticinit::Fn8F7560);
STATICINIT6_REVERSED("StaticExit::8F7570", 0x008F7570, &staticinit::Fn8F7570);
STATICINIT6_REVERSED("StaticExit::8F7580", 0x008F7580, &staticinit::Fn8F7580);
STATICINIT6_REVERSED("StaticExit::8F7590", 0x008F7590, &staticinit::Fn8F7590);
STATICINIT6_REVERSED("StaticExit::8F75A0", 0x008F75A0, &staticinit::Fn8F75A0);
STATICINIT6_REVERSED("StaticExit::8F75B0", 0x008F75B0, &staticinit::Fn8F75B0);
STATICINIT6_REVERSED("StaticExit::8F75C0", 0x008F75C0, &staticinit::Fn8F75C0);
STATICINIT6_REVERSED("StaticExit::8F75D0", 0x008F75D0, &staticinit::Fn8F75D0);
STATICINIT6_REVERSED("StaticExit::8F75E0", 0x008F75E0, &staticinit::Fn8F75E0);
STATICINIT6_REVERSED("StaticExit::8F75F0", 0x008F75F0, &staticinit::Fn8F75F0);
STATICINIT6_REVERSED("StaticExit::8F7600", 0x008F7600, &staticinit::Fn8F7600);
STATICINIT6_REVERSED("StaticExit::8F7610", 0x008F7610, &staticinit::Fn8F7610);
STATICINIT6_REVERSED("StaticExit::8F7620", 0x008F7620, &staticinit::Fn8F7620);
STATICINIT6_REVERSED("StaticExit::8F7650", 0x008F7650, &staticinit::Fn8F7650);
STATICINIT6_REVERSED("StaticExit::8F7660", 0x008F7660, &staticinit::Fn8F7660);
STATICINIT6_REVERSED("StaticExit::8F7670", 0x008F7670, &staticinit::Fn8F7670);
STATICINIT6_REVERSED("StaticExit::8F7680", 0x008F7680, &staticinit::Fn8F7680);
STATICINIT6_REVERSED("StaticExit::8F7690", 0x008F7690, &staticinit::Fn8F7690);
STATICINIT6_REVERSED("StaticExit::8F76A0", 0x008F76A0, &staticinit::Fn8F76A0);
STATICINIT6_REVERSED("StaticExit::8F76B0", 0x008F76B0, &staticinit::Fn8F76B0);
STATICINIT6_REVERSED("StaticExit::8F76C0", 0x008F76C0, &staticinit::Fn8F76C0);
STATICINIT6_REVERSED("StaticExit::8F76D0", 0x008F76D0, &staticinit::Fn8F76D0);
STATICINIT6_REVERSED("StaticExit::8F76E0", 0x008F76E0, &staticinit::Fn8F76E0);
STATICINIT6_REVERSED("StaticExit::8F76F0", 0x008F76F0, &staticinit::Fn8F76F0);
STATICINIT6_REVERSED("StaticExit::8F7700", 0x008F7700, &staticinit::Fn8F7700);
STATICINIT6_REVERSED("StaticExit::8F7710", 0x008F7710, &staticinit::Fn8F7710);
STATICINIT6_REVERSED("StaticExit::8F7720", 0x008F7720, &staticinit::Fn8F7720);
STATICINIT6_REVERSED("StaticExit::8F7730", 0x008F7730, &staticinit::Fn8F7730);
STATICINIT6_REVERSED("StaticExit::8F7740", 0x008F7740, &staticinit::Fn8F7740);
STATICINIT6_REVERSED("StaticExit::8F7750", 0x008F7750, &staticinit::Fn8F7750);
STATICINIT6_REVERSED("StaticExit::8F7760", 0x008F7760, &staticinit::Fn8F7760);
STATICINIT6_REVERSED("StaticExit::8F7770", 0x008F7770, &staticinit::Fn8F7770);
STATICINIT6_REVERSED("StaticExit::8F7790", 0x008F7790, &staticinit::Fn8F7790);
STATICINIT6_REVERSED("StaticExit::8F77B0", 0x008F77B0, &staticinit::Fn8F77B0);
STATICINIT6_REVERSED("StaticExit::8F77D0", 0x008F77D0, &staticinit::Fn8F77D0);
STATICINIT6_REVERSED("StaticExit::8F77E0", 0x008F77E0, &staticinit::Fn8F77E0);
STATICINIT6_REVERSED("StaticExit::8F77F0", 0x008F77F0, &staticinit::Fn8F77F0);
STATICINIT6_REVERSED("StaticExit::8F7800", 0x008F7800, &staticinit::Fn8F7800);
STATICINIT6_REVERSED("StaticExit::8F7830", 0x008F7830, &staticinit::Fn8F7830);
STATICINIT6_REVERSED("StaticExit::8F7840", 0x008F7840, &staticinit::Fn8F7840);
STATICINIT6_REVERSED("StaticExit::8F7850", 0x008F7850, &staticinit::Fn8F7850);
STATICINIT6_REVERSED("StaticExit::8F7860", 0x008F7860, &staticinit::Fn8F7860);
STATICINIT6_REVERSED("StaticExit::8F7870", 0x008F7870, &staticinit::Fn8F7870);
STATICINIT6_REVERSED("StaticExit::8F7880", 0x008F7880, &staticinit::Fn8F7880);
STATICINIT6_REVERSED("StaticExit::8F7890", 0x008F7890, &staticinit::Fn8F7890);
STATICINIT6_REVERSED("StaticExit::8F78A0", 0x008F78A0, &staticinit::Fn8F78A0);
STATICINIT6_REVERSED("StaticExit::8F78B0", 0x008F78B0, &staticinit::Fn8F78B0);
STATICINIT6_REVERSED("StaticExit::8F78C0", 0x008F78C0, &staticinit::Fn8F78C0);
STATICINIT6_REVERSED("StaticExit::8F78D0", 0x008F78D0, &staticinit::Fn8F78D0);
STATICINIT6_REVERSED("StaticExit::8F78E0", 0x008F78E0, &staticinit::Fn8F78E0);
STATICINIT6_REVERSED("StaticExit::8F78F0", 0x008F78F0, &staticinit::Fn8F78F0);
STATICINIT6_REVERSED("StaticExit::8F7900", 0x008F7900, &staticinit::Fn8F7900);
STATICINIT6_REVERSED("StaticExit::8F7910", 0x008F7910, &staticinit::Fn8F7910);
STATICINIT6_REVERSED("StaticExit::8F7920", 0x008F7920, &staticinit::Fn8F7920);
STATICINIT6_REVERSED("StaticExit::8F7930", 0x008F7930, &staticinit::Fn8F7930);
STATICINIT6_REVERSED("StaticExit::8F7940", 0x008F7940, &staticinit::Fn8F7940);
STATICINIT6_REVERSED("StaticExit::8F7950", 0x008F7950, &staticinit::Fn8F7950);
STATICINIT6_REVERSED("StaticExit::8F7960", 0x008F7960, &staticinit::Fn8F7960);
STATICINIT6_REVERSED("StaticExit::8F7970", 0x008F7970, &staticinit::Fn8F7970);
STATICINIT6_REVERSED("StaticExit::8F7980", 0x008F7980, &staticinit::Fn8F7980);
STATICINIT6_REVERSED("StaticExit::8F7990", 0x008F7990, &staticinit::Fn8F7990);
STATICINIT6_REVERSED("StaticExit::8F79A0", 0x008F79A0, &staticinit::Fn8F79A0);
STATICINIT6_REVERSED("StaticExit::8F79B0", 0x008F79B0, &staticinit::Fn8F79B0);
STATICINIT6_REVERSED("StaticExit::8F79E0", 0x008F79E0, &staticinit::Fn8F79E0);
STATICINIT6_REVERSED("StaticExit::8F7A20", 0x008F7A20, &staticinit::Fn8F7A20);
STATICINIT6_REVERSED("StaticExit::8F7A30", 0x008F7A30, &staticinit::Fn8F7A30);
STATICINIT6_REVERSED("StaticExit::8F7A40", 0x008F7A40, &staticinit::Fn8F7A40);
STATICINIT6_REVERSED("StaticExit::8F7A50", 0x008F7A50, &staticinit::Fn8F7A50);
STATICINIT6_REVERSED("StaticExit::8F7B10", 0x008F7B10, &staticinit::Fn8F7B10);
STATICINIT6_REVERSED("StaticExit::8F7B20", 0x008F7B20, &staticinit::Fn8F7B20);
STATICINIT6_REVERSED("StaticExit::8F7B30", 0x008F7B30, &staticinit::Fn8F7B30);
STATICINIT6_REVERSED("StaticExit::8F7B40", 0x008F7B40, &staticinit::Fn8F7B40);
STATICINIT6_REVERSED("StaticExit::8F7B50", 0x008F7B50, &staticinit::Fn8F7B50);
STATICINIT6_REVERSED("StaticExit::8F7B60", 0x008F7B60, &staticinit::Fn8F7B60);
STATICINIT6_REVERSED("StaticExit::8F7B70", 0x008F7B70, &staticinit::Fn8F7B70);
STATICINIT6_REVERSED("StaticExit::8F7B80", 0x008F7B80, &staticinit::Fn8F7B80);
STATICINIT6_REVERSED("StaticExit::8F7B90", 0x008F7B90, &staticinit::Fn8F7B90);
STATICINIT6_REVERSED("StaticExit::8F7BA0", 0x008F7BA0, &staticinit::Fn8F7BA0);
STATICINIT6_REVERSED("StaticExit::8F7BB0", 0x008F7BB0, &staticinit::Fn8F7BB0);
STATICINIT6_REVERSED("StaticExit::8F7BC0", 0x008F7BC0, &staticinit::Fn8F7BC0);
STATICINIT6_REVERSED("StaticExit::8F7BD0", 0x008F7BD0, &staticinit::Fn8F7BD0);
STATICINIT6_REVERSED("StaticExit::8F7BE0", 0x008F7BE0, &staticinit::Fn8F7BE0);
STATICINIT6_REVERSED("StaticExit::8F7BF0", 0x008F7BF0, &staticinit::Fn8F7BF0);
STATICINIT6_REVERSED("StaticExit::8F7C00", 0x008F7C00, &staticinit::Fn8F7C00);
STATICINIT6_REVERSED("StaticExit::8F7C10", 0x008F7C10, &staticinit::Fn8F7C10);
STATICINIT6_REVERSED("StaticExit::8F7C20", 0x008F7C20, &staticinit::Fn8F7C20);
STATICINIT6_REVERSED("StaticExit::8F7C30", 0x008F7C30, &staticinit::Fn8F7C30);
STATICINIT6_REVERSED("StaticExit::8F7C40", 0x008F7C40, &staticinit::Fn8F7C40);
STATICINIT6_REVERSED("StaticExit::8F7C50", 0x008F7C50, &staticinit::Fn8F7C50);
STATICINIT6_REVERSED("StaticExit::8F7C60", 0x008F7C60, &staticinit::Fn8F7C60);
STATICINIT6_REVERSED("StaticExit::8F7C70", 0x008F7C70, &staticinit::Fn8F7C70);
STATICINIT6_REVERSED("StaticExit::8F7C80", 0x008F7C80, &staticinit::Fn8F7C80);
STATICINIT6_REVERSED("StaticExit::8F7C90", 0x008F7C90, &staticinit::Fn8F7C90);
STATICINIT6_REVERSED("StaticExit::8F7CA0", 0x008F7CA0, &staticinit::Fn8F7CA0);
STATICINIT6_REVERSED("StaticExit::8F7CB0", 0x008F7CB0, &staticinit::Fn8F7CB0);
STATICINIT6_REVERSED("StaticExit::8F7CC0", 0x008F7CC0, &staticinit::Fn8F7CC0);
STATICINIT6_REVERSED("StaticExit::8F7CD0", 0x008F7CD0, &staticinit::Fn8F7CD0);
STATICINIT6_REVERSED("StaticExit::8F7CE0", 0x008F7CE0, &staticinit::Fn8F7CE0);
STATICINIT6_REVERSED("StaticExit::8F7CF0", 0x008F7CF0, &staticinit::Fn8F7CF0);
STATICINIT6_REVERSED("StaticExit::8F7D00", 0x008F7D00, &staticinit::Fn8F7D00);
STATICINIT6_REVERSED("StaticExit::8F7D10", 0x008F7D10, &staticinit::Fn8F7D10);
STATICINIT6_REVERSED("StaticExit::8F7D20", 0x008F7D20, &staticinit::Fn8F7D20);
STATICINIT6_REVERSED("StaticExit::8F7D30", 0x008F7D30, &staticinit::Fn8F7D30);
STATICINIT6_REVERSED("StaticExit::8F7D40", 0x008F7D40, &staticinit::Fn8F7D40);
STATICINIT6_REVERSED("StaticExit::8F7D50", 0x008F7D50, &staticinit::Fn8F7D50);
STATICINIT6_REVERSED("StaticExit::8F7D60", 0x008F7D60, &staticinit::Fn8F7D60);
STATICINIT6_REVERSED("StaticExit::8F7D70", 0x008F7D70, &staticinit::Fn8F7D70);
STATICINIT6_REVERSED("StaticExit::8F7D80", 0x008F7D80, &staticinit::Fn8F7D80);
STATICINIT6_REVERSED("StaticExit::8F7D90", 0x008F7D90, &staticinit::Fn8F7D90);
STATICINIT6_REVERSED("StaticExit::8F7DA0", 0x008F7DA0, &staticinit::Fn8F7DA0);
STATICINIT6_REVERSED("StaticExit::8F7DB0", 0x008F7DB0, &staticinit::Fn8F7DB0);
STATICINIT6_REVERSED("StaticExit::8F7DC0", 0x008F7DC0, &staticinit::Fn8F7DC0);
STATICINIT6_REVERSED("StaticExit::8F7DD0", 0x008F7DD0, &staticinit::Fn8F7DD0);
STATICINIT6_REVERSED("StaticExit::8F7DE0", 0x008F7DE0, &staticinit::Fn8F7DE0);
STATICINIT6_REVERSED("StaticExit::8F7DF0", 0x008F7DF0, &staticinit::Fn8F7DF0);
STATICINIT6_REVERSED("StaticExit::8F7E00", 0x008F7E00, &staticinit::Fn8F7E00);
STATICINIT6_REVERSED("StaticExit::8F7E10", 0x008F7E10, &staticinit::Fn8F7E10);
STATICINIT6_REVERSED("StaticExit::8F7E20", 0x008F7E20, &staticinit::Fn8F7E20);
STATICINIT6_REVERSED("StaticExit::8F7E30", 0x008F7E30, &staticinit::Fn8F7E30);
STATICINIT6_REVERSED("StaticExit::8F7E40", 0x008F7E40, &staticinit::Fn8F7E40);
STATICINIT6_REVERSED("StaticExit::8F7E50", 0x008F7E50, &staticinit::Fn8F7E50);
STATICINIT6_REVERSED("StaticExit::8F7E60", 0x008F7E60, &staticinit::Fn8F7E60);
STATICINIT6_REVERSED("StaticExit::8F7E70", 0x008F7E70, &staticinit::Fn8F7E70);
STATICINIT6_REVERSED("StaticExit::8F7E80", 0x008F7E80, &staticinit::Fn8F7E80);
STATICINIT6_REVERSED("StaticExit::8F7E90", 0x008F7E90, &staticinit::Fn8F7E90);
STATICINIT6_REVERSED("StaticExit::8F7EA0", 0x008F7EA0, &staticinit::Fn8F7EA0);
STATICINIT6_REVERSED("StaticExit::8F7EB0", 0x008F7EB0, &staticinit::Fn8F7EB0);
STATICINIT6_REVERSED("StaticExit::8F7EC0", 0x008F7EC0, &staticinit::Fn8F7EC0);
STATICINIT6_REVERSED("StaticExit::8F7ED0", 0x008F7ED0, &staticinit::Fn8F7ED0);
STATICINIT6_REVERSED("StaticExit::8F7EE0", 0x008F7EE0, &staticinit::Fn8F7EE0);
STATICINIT6_REVERSED("StaticExit::8F7EF0", 0x008F7EF0, &staticinit::Fn8F7EF0);
STATICINIT6_REVERSED("StaticExit::8F7F00", 0x008F7F00, &staticinit::Fn8F7F00);
STATICINIT6_REVERSED("StaticExit::8F7F10", 0x008F7F10, &staticinit::Fn8F7F10);
STATICINIT6_REVERSED("StaticExit::8F7F20", 0x008F7F20, &staticinit::Fn8F7F20);
STATICINIT6_REVERSED("StaticExit::8F7F30", 0x008F7F30, &staticinit::Fn8F7F30);
STATICINIT6_REVERSED("StaticExit::8F7F40", 0x008F7F40, &staticinit::Fn8F7F40);
STATICINIT6_REVERSED("StaticExit::8F7F50", 0x008F7F50, &staticinit::Fn8F7F50);
STATICINIT6_REVERSED("StaticExit::8F7F60", 0x008F7F60, &staticinit::Fn8F7F60);
STATICINIT6_REVERSED("StaticExit::8F7F70", 0x008F7F70, &staticinit::Fn8F7F70);
STATICINIT6_REVERSED("StaticExit::8F7F80", 0x008F7F80, &staticinit::Fn8F7F80);
STATICINIT6_REVERSED("StaticExit::8F7F90", 0x008F7F90, &staticinit::Fn8F7F90);
STATICINIT6_REVERSED("StaticExit::8F7FA0", 0x008F7FA0, &staticinit::Fn8F7FA0);
STATICINIT6_REVERSED("StaticExit::8F7FB0", 0x008F7FB0, &staticinit::Fn8F7FB0);
STATICINIT6_REVERSED("StaticExit::8F7FC0", 0x008F7FC0, &staticinit::Fn8F7FC0);
STATICINIT6_REVERSED("StaticExit::8F7FD0", 0x008F7FD0, &staticinit::Fn8F7FD0);
STATICINIT6_REVERSED("StaticExit::8F7FE0", 0x008F7FE0, &staticinit::Fn8F7FE0);
STATICINIT6_REVERSED("StaticExit::8F7FF0", 0x008F7FF0, &staticinit::Fn8F7FF0);
STATICINIT6_REVERSED("StaticExit::8F8000", 0x008F8000, &staticinit::Fn8F8000);
STATICINIT6_REVERSED("StaticExit::8F8010", 0x008F8010, &staticinit::Fn8F8010);
STATICINIT6_REVERSED("StaticExit::8F8020", 0x008F8020, &staticinit::Fn8F8020);
STATICINIT6_REVERSED("StaticExit::8F8030", 0x008F8030, &staticinit::Fn8F8030);
STATICINIT6_REVERSED("StaticExit::8F8040", 0x008F8040, &staticinit::Fn8F8040);
STATICINIT6_REVERSED("StaticExit::8F8050", 0x008F8050, &staticinit::Fn8F8050);
STATICINIT6_REVERSED("StaticExit::8F8060", 0x008F8060, &staticinit::Fn8F8060);
STATICINIT6_REVERSED("StaticExit::8F8070", 0x008F8070, &staticinit::Fn8F8070);
STATICINIT6_REVERSED("StaticExit::8F8080", 0x008F8080, &staticinit::Fn8F8080);
STATICINIT6_REVERSED("StaticExit::8F8090", 0x008F8090, &staticinit::Fn8F8090);
STATICINIT6_REVERSED("StaticExit::8F80A0", 0x008F80A0, &staticinit::Fn8F80A0);
STATICINIT6_REVERSED("StaticExit::8F80B0", 0x008F80B0, &staticinit::Fn8F80B0);
STATICINIT6_REVERSED("StaticExit::8F80C0", 0x008F80C0, &staticinit::Fn8F80C0);
STATICINIT6_REVERSED("StaticExit::8F80D0", 0x008F80D0, &staticinit::Fn8F80D0);
STATICINIT6_REVERSED("StaticExit::8F80E0", 0x008F80E0, &staticinit::Fn8F80E0);
STATICINIT6_REVERSED("StaticExit::8F80F0", 0x008F80F0, &staticinit::Fn8F80F0);
STATICINIT6_REVERSED("StaticExit::8F8100", 0x008F8100, &staticinit::Fn8F8100);
STATICINIT6_REVERSED("StaticExit::8F8110", 0x008F8110, &staticinit::Fn8F8110);
STATICINIT6_REVERSED("StaticExit::8F8120", 0x008F8120, &staticinit::Fn8F8120);
STATICINIT6_REVERSED("StaticExit::8F8130", 0x008F8130, &staticinit::Fn8F8130);
STATICINIT6_REVERSED("StaticExit::8F8140", 0x008F8140, &staticinit::Fn8F8140);
STATICINIT6_REVERSED("StaticExit::8F8180", 0x008F8180, &staticinit::Fn8F8180);
STATICINIT6_REVERSED("StaticExit::8F8190", 0x008F8190, &staticinit::Fn8F8190);
STATICINIT6_REVERSED("StaticExit::8F81A0", 0x008F81A0, &staticinit::Fn8F81A0);
STATICINIT6_REVERSED("StaticExit::8F81B0", 0x008F81B0, &staticinit::Fn8F81B0);
STATICINIT6_REVERSED("StaticExit::8F81C0", 0x008F81C0, &staticinit::Fn8F81C0);
STATICINIT6_REVERSED("StaticExit::8F81D0", 0x008F81D0, &staticinit::Fn8F81D0);
STATICINIT6_REVERSED("StaticExit::8F81E0", 0x008F81E0, &staticinit::Fn8F81E0);
STATICINIT6_REVERSED("StaticExit::8F81F0", 0x008F81F0, &staticinit::Fn8F81F0);
STATICINIT6_REVERSED("StaticExit::8F8200", 0x008F8200, &staticinit::Fn8F8200);
STATICINIT6_REVERSED("StaticExit::8F8210", 0x008F8210, &staticinit::Fn8F8210);
STATICINIT6_REVERSED("StaticExit::8F8220", 0x008F8220, &staticinit::Fn8F8220);
STATICINIT6_REVERSED("StaticExit::8F8230", 0x008F8230, &staticinit::Fn8F8230);
STATICINIT6_REVERSED("StaticExit::8F8240", 0x008F8240, &staticinit::Fn8F8240);
STATICINIT6_REVERSED("StaticExit::8F8250", 0x008F8250, &staticinit::Fn8F8250);
STATICINIT6_REVERSED("StaticExit::8F8260", 0x008F8260, &staticinit::Fn8F8260);
STATICINIT6_REVERSED("StaticExit::8F8270", 0x008F8270, &staticinit::Fn8F8270);
STATICINIT6_REVERSED("StaticExit::8F8280", 0x008F8280, &staticinit::Fn8F8280);
STATICINIT6_REVERSED("StaticExit::8F8290", 0x008F8290, &staticinit::Fn8F8290);
STATICINIT6_REVERSED("StaticExit::8F82A0", 0x008F82A0, &staticinit::Fn8F82A0);
STATICINIT6_REVERSED("StaticExit::8F82B0", 0x008F82B0, &staticinit::Fn8F82B0);
STATICINIT6_REVERSED("StaticExit::8F82C0", 0x008F82C0, &staticinit::Fn8F82C0);
STATICINIT6_REVERSED("StaticExit::8F82D0", 0x008F82D0, &staticinit::Fn8F82D0);
STATICINIT6_REVERSED("StaticExit::8F82E0", 0x008F82E0, &staticinit::Fn8F82E0);
STATICINIT6_REVERSED("StaticExit::8F82F0", 0x008F82F0, &staticinit::Fn8F82F0);
STATICINIT6_REVERSED("StaticExit::8F8300", 0x008F8300, &staticinit::Fn8F8300);
STATICINIT6_REVERSED("StaticExit::8F8310", 0x008F8310, &staticinit::Fn8F8310);
STATICINIT6_REVERSED("StaticExit::8F8320", 0x008F8320, &staticinit::Fn8F8320);
STATICINIT6_REVERSED("StaticExit::8F8330", 0x008F8330, &staticinit::Fn8F8330);
STATICINIT6_REVERSED("StaticExit::8F8340", 0x008F8340, &staticinit::Fn8F8340);
STATICINIT6_REVERSED("StaticExit::8F8350", 0x008F8350, &staticinit::Fn8F8350);
STATICINIT6_REVERSED("StaticExit::8F8360", 0x008F8360, &staticinit::Fn8F8360);
STATICINIT6_REVERSED("StaticExit::8F8370", 0x008F8370, &staticinit::Fn8F8370);
STATICINIT6_REVERSED("StaticExit::8F8380", 0x008F8380, &staticinit::Fn8F8380);
STATICINIT6_REVERSED("StaticExit::8F8390", 0x008F8390, &staticinit::Fn8F8390);
STATICINIT6_REVERSED("StaticExit::8F83A0", 0x008F83A0, &staticinit::Fn8F83A0);
STATICINIT6_REVERSED("StaticExit::8F83B0", 0x008F83B0, &staticinit::Fn8F83B0);
STATICINIT6_REVERSED("StaticExit::8F83C0", 0x008F83C0, &staticinit::Fn8F83C0);
STATICINIT6_REVERSED("StaticExit::8F83D0", 0x008F83D0, &staticinit::Fn8F83D0);
STATICINIT6_REVERSED("StaticExit::8F83E0", 0x008F83E0, &staticinit::Fn8F83E0);
STATICINIT6_REVERSED("StaticExit::8F83F0", 0x008F83F0, &staticinit::Fn8F83F0);
STATICINIT6_REVERSED("StaticExit::8F8400", 0x008F8400, &staticinit::Fn8F8400);
STATICINIT6_REVERSED("StaticExit::8F8410", 0x008F8410, &staticinit::Fn8F8410);
STATICINIT6_REVERSED("StaticExit::8F8420", 0x008F8420, &staticinit::Fn8F8420);
STATICINIT6_REVERSED("StaticExit::8F8430", 0x008F8430, &staticinit::Fn8F8430);
STATICINIT6_REVERSED("StaticExit::8F8440", 0x008F8440, &staticinit::Fn8F8440);
STATICINIT6_REVERSED("StaticExit::8F8450", 0x008F8450, &staticinit::Fn8F8450);
STATICINIT6_REVERSED("StaticExit::8F8460", 0x008F8460, &staticinit::Fn8F8460);
STATICINIT6_REVERSED("StaticExit::8F8470", 0x008F8470, &staticinit::Fn8F8470);
STATICINIT6_REVERSED("StaticExit::8F8480", 0x008F8480, &staticinit::Fn8F8480);
STATICINIT6_REVERSED("StaticExit::8F8490", 0x008F8490, &staticinit::Fn8F8490);
STATICINIT6_REVERSED("StaticExit::8F84A0", 0x008F84A0, &staticinit::Fn8F84A0);
STATICINIT6_REVERSED("StaticExit::8F84B0", 0x008F84B0, &staticinit::Fn8F84B0);
STATICINIT6_REVERSED("StaticExit::8F84C0", 0x008F84C0, &staticinit::Fn8F84C0);
STATICINIT6_REVERSED("StaticExit::8F84D0", 0x008F84D0, &staticinit::Fn8F84D0);
STATICINIT6_REVERSED("StaticExit::8F84E0", 0x008F84E0, &staticinit::Fn8F84E0);
STATICINIT6_REVERSED("StaticExit::8F84F0", 0x008F84F0, &staticinit::Fn8F84F0);
STATICINIT6_REVERSED("StaticExit::8F8500", 0x008F8500, &staticinit::Fn8F8500);
STATICINIT6_REVERSED("StaticExit::8F8510", 0x008F8510, &staticinit::Fn8F8510);
STATICINIT6_REVERSED("StaticExit::8F8520", 0x008F8520, &staticinit::Fn8F8520);
STATICINIT6_REVERSED("StaticExit::8F8530", 0x008F8530, &staticinit::Fn8F8530);
STATICINIT6_REVERSED("StaticExit::8F8540", 0x008F8540, &staticinit::Fn8F8540);
STATICINIT6_REVERSED("StaticExit::8F8550", 0x008F8550, &staticinit::Fn8F8550);
STATICINIT6_REVERSED("StaticExit::8F8560", 0x008F8560, &staticinit::Fn8F8560);
STATICINIT6_REVERSED("StaticExit::8F8570", 0x008F8570, &staticinit::Fn8F8570);
STATICINIT6_REVERSED("StaticExit::8F8580", 0x008F8580, &staticinit::Fn8F8580);
STATICINIT6_REVERSED("StaticExit::8F8590", 0x008F8590, &staticinit::Fn8F8590);
STATICINIT6_REVERSED("StaticExit::8F85A0", 0x008F85A0, &staticinit::Fn8F85A0);
STATICINIT6_REVERSED("StaticExit::8F85B0", 0x008F85B0, &staticinit::Fn8F85B0);
STATICINIT6_REVERSED("StaticExit::8F85C0", 0x008F85C0, &staticinit::Fn8F85C0);
STATICINIT6_REVERSED("StaticExit::8F85D0", 0x008F85D0, &staticinit::Fn8F85D0);
STATICINIT6_REVERSED("StaticExit::8F85F0", 0x008F85F0, &staticinit::Fn8F85F0);
STATICINIT6_REVERSED("StaticExit::8F8690", 0x008F8690, &staticinit::Fn8F8690);
STATICINIT6_REVERSED("StaticExit::8F86A0", 0x008F86A0, &staticinit::Fn8F86A0);
STATICINIT6_REVERSED("StaticExit::8F86B0", 0x008F86B0, &staticinit::Fn8F86B0);
STATICINIT6_REVERSED("StaticExit::8F86C0", 0x008F86C0, &staticinit::Fn8F86C0);
STATICINIT6_REVERSED("StaticExit::8F86D0", 0x008F86D0, &staticinit::Fn8F86D0);
STATICINIT6_REVERSED("StaticExit::8F8720", 0x008F8720, &staticinit::Fn8F8720);
STATICINIT6_REVERSED("StaticExit::8F8760", 0x008F8760, &staticinit::Fn8F8760);
STATICINIT6_REVERSED("StaticExit::8F8830", 0x008F8830, &staticinit::Fn8F8830);
STATICINIT6_REVERSED("StaticExit::8F8860", 0x008F8860, &staticinit::Fn8F8860);
