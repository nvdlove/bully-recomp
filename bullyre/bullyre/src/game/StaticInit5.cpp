// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8F29F0 (exit)
void __cdecl Fn8F29F0() {
}

// 0x8F2A00 (exit)
void __cdecl Fn8F2A00() {
}

// 0x8F2A10 (exit)
void __cdecl Fn8F2A10() {
}

// 0x8F2A20 (exit)
void __cdecl Fn8F2A20() {
}

// 0x8F2A30 (exit)
void __cdecl Fn8F2A30() {
}

// 0x8F2A40 (exit)
void __cdecl Fn8F2A40() {
}

// 0x8F2A50 (exit)
void __cdecl Fn8F2A50() {
}

// 0x8F2A60 (exit)
void __cdecl Fn8F2A60() {
}

// 0x8F2A70 (exit)
void __cdecl Fn8F2A70() {
}

// 0x8F2A80 (exit)
void __cdecl Fn8F2A80() {
}

// 0x8F2A90 (exit)
void __cdecl Fn8F2A90() {
}

// 0x8F2AA0 (exit)
void __cdecl Fn8F2AA0() {
    ReleaseObject(0x00CF12EC);
}

// 0x8F2AD0 (exit)
void __cdecl Fn8F2AD0() {
    ReleaseObject(0x00CF12F0);
}

// 0x8F2B00 (exit)
void __cdecl Fn8F2B00() {
    ReleaseObject(0x00CF12F4);
}

// 0x8F2B30 (exit)
void __cdecl Fn8F2B30() {
    ReleaseObject(0x00CF12F8);
}

// 0x8F2B60 (exit)
void __cdecl Fn8F2B60() {
    ReleaseObject(0x00CF12FC);
}

// 0x8F2B90 (exit)
void __cdecl Fn8F2B90() {
    ReleaseObject(0x00CF1300);
}

// 0x8F2BC0 (exit)
void __cdecl Fn8F2BC0() {
    ReleaseObject(0x00CF1304);
}

// 0x8F2BF0 (exit)
void __cdecl Fn8F2BF0() {
    ReleaseObject(0x00CF1308);
}

// 0x8F2C20 (exit)
void __cdecl Fn8F2C20() {
    ReleaseObject(0x00CF130C);
}

// 0x8F2C50 (exit)
void __cdecl Fn8F2C50() {
}

// 0x8F2C60 (exit)
void __cdecl Fn8F2C60() {
}

// 0x8F2C70 (exit)
void __cdecl Fn8F2C70() {
}

// 0x8F2C80 (exit)
void __cdecl Fn8F2C80() {
}

// 0x8F2C90 (exit)
uint32_t __cdecl Fn8F2C90() {
    return Thiscall(0x00594E00, 0x00CF3DA0u);  // TheSounds::ReleaseAll
}

// 0x8F2CD0 (exit)
void __cdecl Fn8F2CD0() {
    Thiscall(0x0059D110, 0x00CF3D94u, 0x00000000u);  // Sound_Set
    Thiscall(0x0059D110, 0x00CF3D90u, 0x00000000u);  // Sound_Set
    Thiscall(0x005D7F10, 0x00CF3D88u, 0x00000000u);  // EntityRef_Set
}

// 0x8F2D00 (exit)
void __cdecl Fn8F2D00() {
    Stdcall(Dword(0x008FC088), 0x00CF7148u);
}

// 0x8F2D10 (exit)
uint32_t __cdecl Fn8F2D10() {
    return Thiscall(0x005A9B50, 0x00CF7268u);  // CueEngine::FreeGainsReverse
}

// 0x8F2D20 (exit)
void __cdecl Fn8F2D20() {
    Stdcall(Dword(0x008FC088), 0x00CFAD10u);
}

// 0x8F2D30 (exit)
void __cdecl Fn8F2D30() {
    Stdcall(Dword(0x008FC088), 0x00CFAD28u);
}

// 0x8F2D40 (exit)
uint32_t __cdecl Fn8F2D40() {
    return Thiscall(0x005B1EC0, 0x00CFAD58u);  // ScriptEvents::GlobalDestruct_5B1EC0
}

// 0x8F2D70 (exit)
uint32_t __cdecl Fn8F2D70() {
    return Thiscall(0x005B4D50, 0x00CFAF90u);  // ScriptEvents::PropDestruct_5B4D50
}

// 0x8F2DA0 (exit)
void __cdecl Fn8F2DA0() {
    Thiscall(0x005D7F10, 0x00CFB3C0u, 0x00000000u);  // EntityRef_Set
}

// 0x8F2DB0 (exit)
void __cdecl Fn8F2DB0() {
}

// 0x8F2DC0 (exit)
uint32_t __cdecl Fn8F2DC0() {
    return Thiscall(0x005EEE00, 0x00CFC9B8u);
}

// 0x8F2E30 (exit)
void __cdecl Fn8F2E30() {
}

// 0x8F2E50 (exit)
void __cdecl Fn8F2E50() {
}

// 0x8F2E60 (exit)
uint32_t __cdecl Fn8F2E60() {
    return Thiscall(0x005E60A0, 0x00D12AF8u);  // NavObstacleList::FreeAll
}

// 0x8F2E70 (exit)
void __cdecl Fn8F2E70() {
}

// 0x8F2EB0 (exit)
void __cdecl Fn8F2EB0() {
}

// 0x8F2EC0 (exit)
uint32_t __cdecl Fn8F2EC0() {
    Stdcall(Dword(0x008FC088), 0x00D142E4u);
    return Thiscall(0x005EEE00, 0x00D142CCu);
}

// 0x8F2EE0 (exit)
uint32_t __cdecl Fn8F2EE0() {
    Stdcall(Dword(0x008FC088), 0x00D143B4u);
    return Thiscall(0x005EEE00, 0x00D1439Cu);
}

// 0x8F2F00 (exit)
uint32_t __cdecl Fn8F2F00() {
    Stdcall(Dword(0x008FC088), 0x00D14484u);
    return Thiscall(0x005EEE00, 0x00D1446Cu);
}

// 0x8F2F20 (exit)
uint32_t __cdecl Fn8F2F20() {
    Stdcall(Dword(0x008FC088), 0x00D14214u);
    return Thiscall(0x005EEE00, 0x00D141FCu);
}

// 0x8F2F40 (exit)
void __cdecl Fn8F2F40() {
    Dword(0x00B0A284) = 0x0092D0ACu;
}

// 0x8F2FD0 (exit)
uint32_t __cdecl Fn8F2FD0() {
    return Thiscall(0x0049A650, 0x01B649E8u);  // NameKey_Destruct
}

// 0x8F2FF0 (exit)
uint32_t __cdecl Fn8F2FF0() {
    return Thiscall(0x0049A650, 0x01B64A30u);  // NameKey_Destruct
}

// 0x8F3000 (exit)
uint32_t __cdecl Fn8F3000() {
    return Thiscall(0x0049A650, 0x01B64A34u);  // NameKey_Destruct
}

// 0x8F3010 (exit)
uint32_t __cdecl Fn8F3010() {
    return Thiscall(0x0049A650, 0x01B64A38u);  // NameKey_Destruct
}

// 0x8F3020 (exit)
uint32_t __cdecl Fn8F3020() {
    return Thiscall(0x0049A650, 0x01B64A3Cu);  // NameKey_Destruct
}

// 0x8F3030 (exit)
uint32_t __cdecl Fn8F3030() {
    return Thiscall(0x0049A650, 0x01B64A40u);  // NameKey_Destruct
}

// 0x8F3040 (exit)
uint32_t __cdecl Fn8F3040() {
    return Thiscall(0x0049A650, 0x01B64A44u);  // NameKey_Destruct
}

// 0x8F3050 (exit)
uint32_t __cdecl Fn8F3050() {
    return Thiscall(0x0049A650, 0x01B64A48u);  // NameKey_Destruct
}

// 0x8F3060 (exit)
uint32_t __cdecl Fn8F3060() {
    return Thiscall(0x0049A650, 0x01B64A4Cu);  // NameKey_Destruct
}

// 0x8F3070 (exit)
uint32_t __cdecl Fn8F3070() {
    return Thiscall(0x0049A650, 0x01B64A50u);  // NameKey_Destruct
}

// 0x8F3080 (exit)
uint32_t __cdecl Fn8F3080() {
    return Thiscall(0x0049A650, 0x01B64A54u);  // NameKey_Destruct
}

// 0x8F3090 (exit)
uint32_t __cdecl Fn8F3090() {
    return Thiscall(0x0049A650, 0x01B64A58u);  // NameKey_Destruct
}

// 0x8F30A0 (exit)
uint32_t __cdecl Fn8F30A0() {
    return Thiscall(0x0049A650, 0x01B64A5Cu);  // NameKey_Destruct
}

// 0x8F30B0 (exit)
uint32_t __cdecl Fn8F30B0() {
    return Thiscall(0x0049A650, 0x01B64A60u);  // NameKey_Destruct
}

// 0x8F30C0 (exit)
uint32_t __cdecl Fn8F30C0() {
    return Thiscall(0x0049A650, 0x01B64A64u);  // NameKey_Destruct
}

// 0x8F30D0 (exit)
void __cdecl Fn8F30D0() {
}

// 0x8F30E0 (exit)
void __cdecl Fn8F30E0() {
}

// 0x8F30F0 (exit)
uint32_t __cdecl Fn8F30F0() {
    return Thiscall(0x0061A800, 0x01B64A94u);  // TrackRegistry::Destruct
}

// 0x8F3100 (exit)
uint32_t __cdecl Fn8F3100() {
    return Thiscall(0x0049A650, 0x01B64AA4u);  // NameKey_Destruct
}

// 0x8F3110 (exit)
uint32_t __cdecl Fn8F3110() {
    return Thiscall(0x0049A650, 0x01B64AA8u);  // NameKey_Destruct
}

// 0x8F3120 (exit)
uint32_t __cdecl Fn8F3120() {
    return Thiscall(0x0049A650, 0x01B64AACu);  // NameKey_Destruct
}

// 0x8F3130 (exit)
uint32_t __cdecl Fn8F3130() {
    return Thiscall(0x0049A650, 0x01B64AB0u);  // NameKey_Destruct
}

// 0x8F3140 (exit)
uint32_t __cdecl Fn8F3140() {
    return Thiscall(0x0049A650, 0x01B64AB4u);  // NameKey_Destruct
}

// 0x8F3150 (exit)
uint32_t __cdecl Fn8F3150() {
    return Thiscall(0x0049A650, 0x01B64AB8u);  // NameKey_Destruct
}

// 0x8F3160 (exit)
uint32_t __cdecl Fn8F3160() {
    return Thiscall(0x0049A650, 0x01B64ACCu);  // NameKey_Destruct
}

// 0x8F3170 (exit)
uint32_t __cdecl Fn8F3170() {
    return Thiscall(0x0049A650, 0x01B64AD0u);  // NameKey_Destruct
}

// 0x8F3180 (exit)
uint32_t __cdecl Fn8F3180() {
    return Thiscall(0x0049A650, 0x01B64AD4u);  // NameKey_Destruct
}

// 0x8F3190 (exit)
uint32_t __cdecl Fn8F3190() {
    return Thiscall(0x0049A650, 0x01B64AD8u);  // NameKey_Destruct
}

// 0x8F31A0 (exit)
uint32_t __cdecl Fn8F31A0() {
    return Thiscall(0x0049A650, 0x01B64ADCu);  // NameKey_Destruct
}

// 0x8F31B0 (exit)
uint32_t __cdecl Fn8F31B0() {
    return Thiscall(0x0049A650, 0x01B64AE0u);  // NameKey_Destruct
}

// 0x8F31C0 (exit)
uint32_t __cdecl Fn8F31C0() {
    return Thiscall(0x0049A650, 0x01B64AE4u);  // NameKey_Destruct
}

// 0x8F31D0 (exit)
uint32_t __cdecl Fn8F31D0() {
    return Thiscall(0x0049A650, 0x01B64AE8u);  // NameKey_Destruct
}

// 0x8F31E0 (exit)
uint32_t __cdecl Fn8F31E0() {
    return Thiscall(0x0049A650, 0x01B64AECu);  // NameKey_Destruct
}

// 0x8F31F0 (exit)
uint32_t __cdecl Fn8F31F0() {
    return Thiscall(0x0049A650, 0x01B64AF0u);  // NameKey_Destruct
}

// 0x8F3200 (exit)
uint32_t __cdecl Fn8F3200() {
    return Thiscall(0x0049A650, 0x01B64AF4u);  // NameKey_Destruct
}

// 0x8F3210 (exit)
uint32_t __cdecl Fn8F3210() {
    return Thiscall(0x0049A650, 0x01B64AF8u);  // NameKey_Destruct
}

// 0x8F3220 (exit)
uint32_t __cdecl Fn8F3220() {
    return Thiscall(0x0049A650, 0x01B64AFCu);  // NameKey_Destruct
}

// 0x8F3230 (exit)
uint32_t __cdecl Fn8F3230() {
    return Thiscall(0x0049A650, 0x01B64B00u);  // NameKey_Destruct
}

// 0x8F3240 (exit)
uint32_t __cdecl Fn8F3240() {
    return Thiscall(0x0049A650, 0x01B64B04u);  // NameKey_Destruct
}

// 0x8F3250 (exit)
uint32_t __cdecl Fn8F3250() {
    return Thiscall(0x0049A650, 0x01B64B08u);  // NameKey_Destruct
}

// 0x8F3260 (exit)
uint32_t __cdecl Fn8F3260() {
    return Thiscall(0x0049A650, 0x01B64B0Cu);  // NameKey_Destruct
}

// 0x8F3270 (exit)
uint32_t __cdecl Fn8F3270() {
    return Thiscall(0x0049A650, 0x01B64B10u);  // NameKey_Destruct
}

// 0x8F3280 (exit)
uint32_t __cdecl Fn8F3280() {
    return Thiscall(0x0049A650, 0x01B64B14u);  // NameKey_Destruct
}

// 0x8F3290 (exit)
uint32_t __cdecl Fn8F3290() {
    return Thiscall(0x0049A650, 0x01B64B18u);  // NameKey_Destruct
}

// 0x8F32A0 (exit)
uint32_t __cdecl Fn8F32A0() {
    return Thiscall(0x0049A650, 0x01B64B1Cu);  // NameKey_Destruct
}

// 0x8F32B0 (exit)
uint32_t __cdecl Fn8F32B0() {
    return Thiscall(0x0049A650, 0x01B64B20u);  // NameKey_Destruct
}

// 0x8F32C0 (exit)
uint32_t __cdecl Fn8F32C0() {
    return Thiscall(0x0049A650, 0x01B64B24u);  // NameKey_Destruct
}

// 0x8F32D0 (exit)
uint32_t __cdecl Fn8F32D0() {
    return Thiscall(0x0049A650, 0x01B64B28u);  // NameKey_Destruct
}

// 0x8F32E0 (exit)
uint32_t __cdecl Fn8F32E0() {
    return Thiscall(0x0049A650, 0x01B64B2Cu);  // NameKey_Destruct
}

// 0x8F32F0 (exit)
uint32_t __cdecl Fn8F32F0() {
    return Thiscall(0x0049A650, 0x01B64B30u);  // NameKey_Destruct
}

// 0x8F3300 (exit)
uint32_t __cdecl Fn8F3300() {
    return Thiscall(0x0049A650, 0x01B64B34u);  // NameKey_Destruct
}

// 0x8F3310 (exit)
uint32_t __cdecl Fn8F3310() {
    return Thiscall(0x0049A650, 0x01B64B38u);  // NameKey_Destruct
}

// 0x8F3320 (exit)
uint32_t __cdecl Fn8F3320() {
    return Thiscall(0x0049A650, 0x01B64B3Cu);  // NameKey_Destruct
}

// 0x8F3330 (exit)
uint32_t __cdecl Fn8F3330() {
    return Thiscall(0x0049A650, 0x01B64B40u);  // NameKey_Destruct
}

// 0x8F3340 (exit)
uint32_t __cdecl Fn8F3340() {
    return Thiscall(0x0049A650, 0x01B64B44u);  // NameKey_Destruct
}

// 0x8F3350 (exit)
uint32_t __cdecl Fn8F3350() {
    return Thiscall(0x0049A650, 0x01B64B48u);  // NameKey_Destruct
}

// 0x8F3360 (exit)
uint32_t __cdecl Fn8F3360() {
    return Thiscall(0x0049A650, 0x01B64B4Cu);  // NameKey_Destruct
}

// 0x8F3370 (exit)
uint32_t __cdecl Fn8F3370() {
    return Thiscall(0x0049A650, 0x01B64B50u);  // NameKey_Destruct
}

// 0x8F3380 (exit)
uint32_t __cdecl Fn8F3380() {
    return Thiscall(0x0049A650, 0x01B64B54u);  // NameKey_Destruct
}

// 0x8F3390 (exit)
uint32_t __cdecl Fn8F3390() {
    return Thiscall(0x0049A650, 0x01B64B58u);  // NameKey_Destruct
}

// 0x8F33A0 (exit)
uint32_t __cdecl Fn8F33A0() {
    return Thiscall(0x0049A650, 0x01B64B5Cu);  // NameKey_Destruct
}

// 0x8F33B0 (exit)
uint32_t __cdecl Fn8F33B0() {
    return Thiscall(0x0049A650, 0x01B64B60u);  // NameKey_Destruct
}

// 0x8F33C0 (exit)
uint32_t __cdecl Fn8F33C0() {
    return Thiscall(0x0049A650, 0x01B64B64u);  // NameKey_Destruct
}

// 0x8F33D0 (exit)
uint32_t __cdecl Fn8F33D0() {
    return Thiscall(0x0049A650, 0x01B64B68u);  // NameKey_Destruct
}

// 0x8F33E0 (exit)
uint32_t __cdecl Fn8F33E0() {
    return Thiscall(0x0049A650, 0x01B64B6Cu);  // NameKey_Destruct
}

// 0x8F33F0 (exit)
uint32_t __cdecl Fn8F33F0() {
    return Thiscall(0x0049A650, 0x01B64B70u);  // NameKey_Destruct
}

// 0x8F3400 (exit)
uint32_t __cdecl Fn8F3400() {
    return Thiscall(0x0049A650, 0x01B64B74u);  // NameKey_Destruct
}

// 0x8F3410 (exit)
uint32_t __cdecl Fn8F3410() {
    return Thiscall(0x0049A650, 0x01B64B78u);  // NameKey_Destruct
}

// 0x8F3420 (exit)
uint32_t __cdecl Fn8F3420() {
    return Thiscall(0x0049A650, 0x01B64B7Cu);  // NameKey_Destruct
}

// 0x8F3430 (exit)
uint32_t __cdecl Fn8F3430() {
    return Thiscall(0x0049A650, 0x01B64B80u);  // NameKey_Destruct
}

// 0x8F3440 (exit)
uint32_t __cdecl Fn8F3440() {
    return Thiscall(0x0049A650, 0x01B64B84u);  // NameKey_Destruct
}

// 0x8F3450 (exit)
uint32_t __cdecl Fn8F3450() {
    return Thiscall(0x0049A650, 0x01B64B88u);  // NameKey_Destruct
}

// 0x8F3460 (exit)
uint32_t __cdecl Fn8F3460() {
    return Thiscall(0x0049A650, 0x01B64B8Cu);  // NameKey_Destruct
}

// 0x8F3470 (exit)
uint32_t __cdecl Fn8F3470() {
    return Thiscall(0x0049A650, 0x01B64B90u);  // NameKey_Destruct
}

// 0x8F3480 (exit)
uint32_t __cdecl Fn8F3480() {
    return Thiscall(0x0049A650, 0x01B64B94u);  // NameKey_Destruct
}

// 0x8F3490 (exit)
uint32_t __cdecl Fn8F3490() {
    return Thiscall(0x0049A650, 0x01B64B98u);  // NameKey_Destruct
}

// 0x8F34A0 (exit)
uint32_t __cdecl Fn8F34A0() {
    return Thiscall(0x0049A650, 0x01B64B9Cu);  // NameKey_Destruct
}

// 0x8F34B0 (exit)
uint32_t __cdecl Fn8F34B0() {
    return Thiscall(0x0049A650, 0x01B64BA0u);  // NameKey_Destruct
}

// 0x8F34C0 (exit)
uint32_t __cdecl Fn8F34C0() {
    return Thiscall(0x0049A650, 0x01B64BA4u);  // NameKey_Destruct
}

// 0x8F34D0 (exit)
uint32_t __cdecl Fn8F34D0() {
    return Thiscall(0x0049A650, 0x01B64BA8u);  // NameKey_Destruct
}

// 0x8F34E0 (exit)
uint32_t __cdecl Fn8F34E0() {
    return Thiscall(0x0049A650, 0x01B64BACu);  // NameKey_Destruct
}

// 0x8F34F0 (exit)
uint32_t __cdecl Fn8F34F0() {
    return Thiscall(0x0049A650, 0x01B64BB0u);  // NameKey_Destruct
}

// 0x8F3500 (exit)
uint32_t __cdecl Fn8F3500() {
    return Thiscall(0x0049A650, 0x01B64BB4u);  // NameKey_Destruct
}

// 0x8F3510 (exit)
uint32_t __cdecl Fn8F3510() {
    return Thiscall(0x0049A650, 0x01B64BB8u);  // NameKey_Destruct
}

// 0x8F3520 (exit)
uint32_t __cdecl Fn8F3520() {
    return Thiscall(0x0049A650, 0x01B64BBCu);  // NameKey_Destruct
}

// 0x8F3530 (exit)
uint32_t __cdecl Fn8F3530() {
    return Thiscall(0x0049A650, 0x01B64BC0u);  // NameKey_Destruct
}

// 0x8F3540 (exit)
uint32_t __cdecl Fn8F3540() {
    return Thiscall(0x0049A650, 0x01B64BC4u);  // NameKey_Destruct
}

// 0x8F3550 (exit)
uint32_t __cdecl Fn8F3550() {
    return Thiscall(0x0049A650, 0x01B64BC8u);  // NameKey_Destruct
}

// 0x8F3560 (exit)
uint32_t __cdecl Fn8F3560() {
    return Thiscall(0x0049A650, 0x01B64BCCu);  // NameKey_Destruct
}

// 0x8F3570 (exit)
uint32_t __cdecl Fn8F3570() {
    return Thiscall(0x0049A650, 0x01B64BD0u);  // NameKey_Destruct
}

// 0x8F3580 (exit)
uint32_t __cdecl Fn8F3580() {
    return Thiscall(0x0049A650, 0x01B64BD4u);  // NameKey_Destruct
}

// 0x8F3590 (exit)
uint32_t __cdecl Fn8F3590() {
    return Thiscall(0x0049A650, 0x01B64BD8u);  // NameKey_Destruct
}

// 0x8F35A0 (exit)
uint32_t __cdecl Fn8F35A0() {
    return Thiscall(0x0049A650, 0x01B64BDCu);  // NameKey_Destruct
}

// 0x8F35B0 (exit)
uint32_t __cdecl Fn8F35B0() {
    return Thiscall(0x0049A650, 0x01B64BE0u);  // NameKey_Destruct
}

// 0x8F35C0 (exit)
uint32_t __cdecl Fn8F35C0() {
    return Thiscall(0x0049A650, 0x01B64BE4u);  // NameKey_Destruct
}

// 0x8F35D0 (exit)
uint32_t __cdecl Fn8F35D0() {
    return Thiscall(0x0049A650, 0x01B64BE8u);  // NameKey_Destruct
}

// 0x8F35E0 (exit)
uint32_t __cdecl Fn8F35E0() {
    return Thiscall(0x0049A650, 0x01B64BECu);  // NameKey_Destruct
}

// 0x8F35F0 (exit)
uint32_t __cdecl Fn8F35F0() {
    return Thiscall(0x0049A650, 0x01B64BF0u);  // NameKey_Destruct
}

// 0x8F3600 (exit)
uint32_t __cdecl Fn8F3600() {
    return Thiscall(0x0049A650, 0x01B64BF4u);  // NameKey_Destruct
}

// 0x8F3610 (exit)
uint32_t __cdecl Fn8F3610() {
    return Thiscall(0x0049A650, 0x01B64BF8u);  // NameKey_Destruct
}

// 0x8F3620 (exit)
uint32_t __cdecl Fn8F3620() {
    return Thiscall(0x0049A650, 0x01B64BFCu);  // NameKey_Destruct
}

// 0x8F3630 (exit)
uint32_t __cdecl Fn8F3630() {
    return Thiscall(0x0049A650, 0x01B64C00u);  // NameKey_Destruct
}

// 0x8F3640 (exit)
uint32_t __cdecl Fn8F3640() {
    return Thiscall(0x0049A650, 0x01B64C04u);  // NameKey_Destruct
}

// 0x8F3650 (exit)
uint32_t __cdecl Fn8F3650() {
    return Thiscall(0x0049A650, 0x01B64C08u);  // NameKey_Destruct
}

// 0x8F3660 (exit)
uint32_t __cdecl Fn8F3660() {
    return Thiscall(0x0049A650, 0x01B64C0Cu);  // NameKey_Destruct
}

// 0x8F3670 (exit)
uint32_t __cdecl Fn8F3670() {
    return Thiscall(0x0049A650, 0x01B64C10u);  // NameKey_Destruct
}

// 0x8F3680 (exit)
uint32_t __cdecl Fn8F3680() {
    return Thiscall(0x0049A650, 0x01B64C14u);  // NameKey_Destruct
}

// 0x8F3690 (exit)
uint32_t __cdecl Fn8F3690() {
    return Thiscall(0x0049A650, 0x01B64C18u);  // NameKey_Destruct
}

// 0x8F36A0 (exit)
uint32_t __cdecl Fn8F36A0() {
    return Thiscall(0x0049A650, 0x01B64C1Cu);  // NameKey_Destruct
}

// 0x8F36B0 (exit)
uint32_t __cdecl Fn8F36B0() {
    return Thiscall(0x0049A650, 0x01B64C20u);  // NameKey_Destruct
}

// 0x8F36C0 (exit)
uint32_t __cdecl Fn8F36C0() {
    return Thiscall(0x0049A650, 0x01B64C24u);  // NameKey_Destruct
}

// 0x8F36D0 (exit)
uint32_t __cdecl Fn8F36D0() {
    return Thiscall(0x0049A650, 0x01B64C28u);  // NameKey_Destruct
}

// 0x8F36E0 (exit)
uint32_t __cdecl Fn8F36E0() {
    return Thiscall(0x0049A650, 0x01B64C2Cu);  // NameKey_Destruct
}

// 0x8F36F0 (exit)
uint32_t __cdecl Fn8F36F0() {
    return Thiscall(0x0049A650, 0x01B64C30u);  // NameKey_Destruct
}

// 0x8F3700 (exit)
uint32_t __cdecl Fn8F3700() {
    return Thiscall(0x0049A650, 0x01B64C34u);  // NameKey_Destruct
}

// 0x8F3710 (exit)
uint32_t __cdecl Fn8F3710() {
    return Thiscall(0x0049A650, 0x01B64C38u);  // NameKey_Destruct
}

// 0x8F3720 (exit)
uint32_t __cdecl Fn8F3720() {
    return Thiscall(0x0049A650, 0x01B64C3Cu);  // NameKey_Destruct
}

// 0x8F3730 (exit)
uint32_t __cdecl Fn8F3730() {
    return Thiscall(0x0049A650, 0x01B64C40u);  // NameKey_Destruct
}

// 0x8F3740 (exit)
uint32_t __cdecl Fn8F3740() {
    return Thiscall(0x0049A650, 0x01B64C44u);  // NameKey_Destruct
}

// 0x8F3750 (exit)
uint32_t __cdecl Fn8F3750() {
    return Thiscall(0x0049A650, 0x01B64C48u);  // NameKey_Destruct
}

// 0x8F3760 (exit)
uint32_t __cdecl Fn8F3760() {
    return Thiscall(0x0049A650, 0x01B64C4Cu);  // NameKey_Destruct
}

// 0x8F3770 (exit)
uint32_t __cdecl Fn8F3770() {
    return Thiscall(0x0049A650, 0x01B64C50u);  // NameKey_Destruct
}

// 0x8F3780 (exit)
uint32_t __cdecl Fn8F3780() {
    return Thiscall(0x0049A650, 0x01B64C54u);  // NameKey_Destruct
}

// 0x8F3790 (exit)
uint32_t __cdecl Fn8F3790() {
    return Thiscall(0x0049A650, 0x01B64C58u);  // NameKey_Destruct
}

// 0x8F37A0 (exit)
uint32_t __cdecl Fn8F37A0() {
    return Thiscall(0x0049A650, 0x01B64C5Cu);  // NameKey_Destruct
}

// 0x8F37B0 (exit)
uint32_t __cdecl Fn8F37B0() {
    return Thiscall(0x0049A650, 0x01B64C60u);  // NameKey_Destruct
}

// 0x8F37C0 (exit)
uint32_t __cdecl Fn8F37C0() {
    return Thiscall(0x0049A650, 0x01B64C64u);  // NameKey_Destruct
}

// 0x8F37D0 (exit)
uint32_t __cdecl Fn8F37D0() {
    return Thiscall(0x0049A650, 0x01B64C68u);  // NameKey_Destruct
}

// 0x8F37E0 (exit)
uint32_t __cdecl Fn8F37E0() {
    return Thiscall(0x0049A650, 0x01B64C6Cu);  // NameKey_Destruct
}

// 0x8F37F0 (exit)
uint32_t __cdecl Fn8F37F0() {
    return Thiscall(0x0049A650, 0x01B64C70u);  // NameKey_Destruct
}

// 0x8F3800 (exit)
uint32_t __cdecl Fn8F3800() {
    return Thiscall(0x0049A650, 0x01B64C74u);  // NameKey_Destruct
}

// 0x8F3810 (exit)
uint32_t __cdecl Fn8F3810() {
    return Thiscall(0x0049A650, 0x01B64C78u);  // NameKey_Destruct
}

// 0x8F3820 (exit)
uint32_t __cdecl Fn8F3820() {
    return Thiscall(0x0049A650, 0x01B64C7Cu);  // NameKey_Destruct
}

// 0x8F3830 (exit)
uint32_t __cdecl Fn8F3830() {
    return Thiscall(0x0049A650, 0x01B64C80u);  // NameKey_Destruct
}

// 0x8F3840 (exit)
uint32_t __cdecl Fn8F3840() {
    return Thiscall(0x0049A650, 0x01B64C84u);  // NameKey_Destruct
}

// 0x8F3850 (exit)
uint32_t __cdecl Fn8F3850() {
    return Thiscall(0x0049A650, 0x01B64C88u);  // NameKey_Destruct
}

// 0x8F3860 (exit)
uint32_t __cdecl Fn8F3860() {
    return Thiscall(0x0049A650, 0x01B64C8Cu);  // NameKey_Destruct
}

// 0x8F3870 (exit)
uint32_t __cdecl Fn8F3870() {
    return Thiscall(0x0049A650, 0x01B64C90u);  // NameKey_Destruct
}

// 0x8F3880 (exit)
uint32_t __cdecl Fn8F3880() {
    return Thiscall(0x0049A650, 0x01B64C94u);  // NameKey_Destruct
}

// 0x8F3890 (exit)
uint32_t __cdecl Fn8F3890() {
    return Thiscall(0x0049A650, 0x01B64C98u);  // NameKey_Destruct
}

// 0x8F38A0 (exit)
uint32_t __cdecl Fn8F38A0() {
    return Thiscall(0x0049A650, 0x01B64C9Cu);  // NameKey_Destruct
}

// 0x8F38B0 (exit)
uint32_t __cdecl Fn8F38B0() {
    return Thiscall(0x0049A650, 0x01B64CA0u);  // NameKey_Destruct
}

// 0x8F38C0 (exit)
uint32_t __cdecl Fn8F38C0() {
    return Thiscall(0x0049A650, 0x01B64CA4u);  // NameKey_Destruct
}

// 0x8F38D0 (exit)
uint32_t __cdecl Fn8F38D0() {
    return Thiscall(0x0049A650, 0x01B64CA8u);  // NameKey_Destruct
}

// 0x8F38E0 (exit)
uint32_t __cdecl Fn8F38E0() {
    return Thiscall(0x0049A650, 0x01B64CACu);  // NameKey_Destruct
}

// 0x8F38F0 (exit)
uint32_t __cdecl Fn8F38F0() {
    return Thiscall(0x0049A650, 0x01B64CB0u);  // NameKey_Destruct
}

// 0x8F3900 (exit)
uint32_t __cdecl Fn8F3900() {
    return Thiscall(0x0049A650, 0x01B64CB4u);  // NameKey_Destruct
}

// 0x8F3910 (exit)
uint32_t __cdecl Fn8F3910() {
    return Thiscall(0x0049A650, 0x01B64CB8u);  // NameKey_Destruct
}

// 0x8F3920 (exit)
uint32_t __cdecl Fn8F3920() {
    return Thiscall(0x0049A650, 0x01B64CBCu);  // NameKey_Destruct
}

// 0x8F3930 (exit)
uint32_t __cdecl Fn8F3930() {
    return Thiscall(0x0049A650, 0x01B64CC0u);  // NameKey_Destruct
}

// 0x8F3940 (exit)
uint32_t __cdecl Fn8F3940() {
    return Thiscall(0x0049A650, 0x01B64CC4u);  // NameKey_Destruct
}

// 0x8F3950 (exit)
uint32_t __cdecl Fn8F3950() {
    return Thiscall(0x0049A650, 0x01B64CC8u);  // NameKey_Destruct
}

// 0x8F3960 (exit)
uint32_t __cdecl Fn8F3960() {
    return Thiscall(0x0049A650, 0x01B64CCCu);  // NameKey_Destruct
}

// 0x8F3970 (exit)
uint32_t __cdecl Fn8F3970() {
    return Thiscall(0x0049A650, 0x01B64CD0u);  // NameKey_Destruct
}

// 0x8F3980 (exit)
uint32_t __cdecl Fn8F3980() {
    return Thiscall(0x0049A650, 0x01B64CD4u);  // NameKey_Destruct
}

// 0x8F3990 (exit)
uint32_t __cdecl Fn8F3990() {
    return Thiscall(0x0049A650, 0x01B64CD8u);  // NameKey_Destruct
}

// 0x8F39A0 (exit)
uint32_t __cdecl Fn8F39A0() {
    return Thiscall(0x0049A650, 0x01B64CDCu);  // NameKey_Destruct
}

// 0x8F39B0 (exit)
uint32_t __cdecl Fn8F39B0() {
    return Thiscall(0x0049A650, 0x01B64CE0u);  // NameKey_Destruct
}

// 0x8F39C0 (exit)
uint32_t __cdecl Fn8F39C0() {
    return Thiscall(0x0049A650, 0x01B64CE4u);  // NameKey_Destruct
}

// 0x8F39D0 (exit)
uint32_t __cdecl Fn8F39D0() {
    return Thiscall(0x0049A650, 0x01B64CE8u);  // NameKey_Destruct
}

// 0x8F39E0 (exit)
uint32_t __cdecl Fn8F39E0() {
    return Thiscall(0x0049A650, 0x01B64CECu);  // NameKey_Destruct
}

// 0x8F39F0 (exit)
uint32_t __cdecl Fn8F39F0() {
    return Thiscall(0x0049A650, 0x01B64CF0u);  // NameKey_Destruct
}

// 0x8F3A00 (exit)
uint32_t __cdecl Fn8F3A00() {
    return Thiscall(0x0049A650, 0x01B64CF4u);  // NameKey_Destruct
}

// 0x8F3A10 (exit)
uint32_t __cdecl Fn8F3A10() {
    return Thiscall(0x0049A650, 0x01B64CF8u);  // NameKey_Destruct
}

// 0x8F3A20 (exit)
uint32_t __cdecl Fn8F3A20() {
    return Thiscall(0x0049A650, 0x01B64CFCu);  // NameKey_Destruct
}

// 0x8F3A30 (exit)
uint32_t __cdecl Fn8F3A30() {
    return Thiscall(0x0049A650, 0x01B64D00u);  // NameKey_Destruct
}

// 0x8F3A40 (exit)
uint32_t __cdecl Fn8F3A40() {
    return Thiscall(0x0049A650, 0x01B64D04u);  // NameKey_Destruct
}

// 0x8F3A50 (exit)
uint32_t __cdecl Fn8F3A50() {
    return Thiscall(0x0049A650, 0x01B64D08u);  // NameKey_Destruct
}

// 0x8F3A60 (exit)
uint32_t __cdecl Fn8F3A60() {
    return Thiscall(0x0049A650, 0x01B64D0Cu);  // NameKey_Destruct
}

// 0x8F3A70 (exit)
uint32_t __cdecl Fn8F3A70() {
    return Thiscall(0x0049A650, 0x01B64D10u);  // NameKey_Destruct
}

// 0x8F3A80 (exit)
uint32_t __cdecl Fn8F3A80() {
    return Thiscall(0x0049A650, 0x01B64D14u);  // NameKey_Destruct
}

// 0x8F3A90 (exit)
uint32_t __cdecl Fn8F3A90() {
    return Thiscall(0x0049A650, 0x01B64D18u);  // NameKey_Destruct
}

// 0x8F3AA0 (exit)
uint32_t __cdecl Fn8F3AA0() {
    return Thiscall(0x0049A650, 0x01B64D1Cu);  // NameKey_Destruct
}

// 0x8F3AB0 (exit)
uint32_t __cdecl Fn8F3AB0() {
    return Thiscall(0x0049A650, 0x01B64D20u);  // NameKey_Destruct
}

// 0x8F3AC0 (exit)
uint32_t __cdecl Fn8F3AC0() {
    return Thiscall(0x0049A650, 0x01B64D24u);  // NameKey_Destruct
}

// 0x8F3AD0 (exit)
uint32_t __cdecl Fn8F3AD0() {
    return Thiscall(0x0049A650, 0x01B64D28u);  // NameKey_Destruct
}

// 0x8F3AE0 (exit)
uint32_t __cdecl Fn8F3AE0() {
    return Thiscall(0x0049A650, 0x01B64D2Cu);  // NameKey_Destruct
}

// 0x8F3AF0 (exit)
uint32_t __cdecl Fn8F3AF0() {
    return Thiscall(0x0049A650, 0x01B64D30u);  // NameKey_Destruct
}

// 0x8F3B00 (exit)
uint32_t __cdecl Fn8F3B00() {
    return Thiscall(0x0049A650, 0x01B64D34u);  // NameKey_Destruct
}

// 0x8F3B10 (exit)
uint32_t __cdecl Fn8F3B10() {
    return Thiscall(0x0049A650, 0x01B64D38u);  // NameKey_Destruct
}

// 0x8F3B20 (exit)
uint32_t __cdecl Fn8F3B20() {
    return Thiscall(0x0049A650, 0x01B64D3Cu);  // NameKey_Destruct
}

// 0x8F3B30 (exit)
uint32_t __cdecl Fn8F3B30() {
    return Thiscall(0x0049A650, 0x01B64D40u);  // NameKey_Destruct
}

// 0x8F3B40 (exit)
uint32_t __cdecl Fn8F3B40() {
    return Thiscall(0x0049A650, 0x01B64D44u);  // NameKey_Destruct
}

// 0x8F3B50 (exit)
uint32_t __cdecl Fn8F3B50() {
    return Thiscall(0x0049A650, 0x01B64D48u);  // NameKey_Destruct
}

// 0x8F3B60 (exit)
uint32_t __cdecl Fn8F3B60() {
    return Thiscall(0x0049A650, 0x01B64D4Cu);  // NameKey_Destruct
}

// 0x8F3B70 (exit)
uint32_t __cdecl Fn8F3B70() {
    return Thiscall(0x0049A650, 0x01B64D50u);  // NameKey_Destruct
}

// 0x8F3B80 (exit)
uint32_t __cdecl Fn8F3B80() {
    return Thiscall(0x0049A650, 0x01B64D54u);  // NameKey_Destruct
}

// 0x8F3B90 (exit)
uint32_t __cdecl Fn8F3B90() {
    return Thiscall(0x0049A650, 0x01B64D58u);  // NameKey_Destruct
}

// 0x8F3BA0 (exit)
uint32_t __cdecl Fn8F3BA0() {
    return Thiscall(0x0049A650, 0x01B64D5Cu);  // NameKey_Destruct
}

// 0x8F3BB0 (exit)
uint32_t __cdecl Fn8F3BB0() {
    return Thiscall(0x0049A650, 0x01B64D60u);  // NameKey_Destruct
}

// 0x8F3BC0 (exit)
uint32_t __cdecl Fn8F3BC0() {
    return Thiscall(0x0049A650, 0x01B64D68u);  // NameKey_Destruct
}

// 0x8F3BD0 (exit)
uint32_t __cdecl Fn8F3BD0() {
    return Thiscall(0x0049A650, 0x01B64D6Cu);  // NameKey_Destruct
}

// 0x8F3BE0 (exit)
uint32_t __cdecl Fn8F3BE0() {
    return Thiscall(0x0049A650, 0x01B64D70u);  // NameKey_Destruct
}

// 0x8F3BF0 (exit)
uint32_t __cdecl Fn8F3BF0() {
    return Thiscall(0x0049A650, 0x01B64D74u);  // NameKey_Destruct
}

// 0x8F3C00 (exit)
uint32_t __cdecl Fn8F3C00() {
    return Thiscall(0x0049A650, 0x01B64D78u);  // NameKey_Destruct
}

// 0x8F3C10 (exit)
uint32_t __cdecl Fn8F3C10() {
    return Thiscall(0x0049A650, 0x01B64D7Cu);  // NameKey_Destruct
}

// 0x8F3C20 (exit)
uint32_t __cdecl Fn8F3C20() {
    return Thiscall(0x0049A650, 0x01B64D80u);  // NameKey_Destruct
}

// 0x8F3C30 (exit)
uint32_t __cdecl Fn8F3C30() {
    return Thiscall(0x0049A650, 0x01B64D84u);  // NameKey_Destruct
}

// 0x8F3C40 (exit)
uint32_t __cdecl Fn8F3C40() {
    return Thiscall(0x0049A650, 0x01B64D88u);  // NameKey_Destruct
}

// 0x8F3C50 (exit)
uint32_t __cdecl Fn8F3C50() {
    return Thiscall(0x0049A650, 0x01B64D8Cu);  // NameKey_Destruct
}

// 0x8F3C60 (exit)
uint32_t __cdecl Fn8F3C60() {
    return Thiscall(0x0049A650, 0x01B64D90u);  // NameKey_Destruct
}

// 0x8F3C70 (exit)
uint32_t __cdecl Fn8F3C70() {
    return Thiscall(0x0049A650, 0x01B64D94u);  // NameKey_Destruct
}

// 0x8F3C80 (exit)
uint32_t __cdecl Fn8F3C80() {
    return Thiscall(0x0049A650, 0x01B64D98u);  // NameKey_Destruct
}

// 0x8F3C90 (exit)
uint32_t __cdecl Fn8F3C90() {
    return Thiscall(0x0049A650, 0x01B64D9Cu);  // NameKey_Destruct
}

// 0x8F3CA0 (exit)
uint32_t __cdecl Fn8F3CA0() {
    return Thiscall(0x0049A650, 0x01B64DA0u);  // NameKey_Destruct
}

// 0x8F3CB0 (exit)
uint32_t __cdecl Fn8F3CB0() {
    return Thiscall(0x0049A650, 0x01B64DA4u);  // NameKey_Destruct
}

// 0x8F3CC0 (exit)
uint32_t __cdecl Fn8F3CC0() {
    return Thiscall(0x0049A650, 0x01B64DA8u);  // NameKey_Destruct
}

// 0x8F3CD0 (exit)
uint32_t __cdecl Fn8F3CD0() {
    return Thiscall(0x0049A650, 0x01B64DACu);  // NameKey_Destruct
}

// 0x8F3CE0 (exit)
uint32_t __cdecl Fn8F3CE0() {
    return Thiscall(0x0049A650, 0x01B64DB0u);  // NameKey_Destruct
}

// 0x8F3CF0 (exit)
uint32_t __cdecl Fn8F3CF0() {
    return Thiscall(0x0049A650, 0x01B64DB4u);  // NameKey_Destruct
}

// 0x8F3D00 (exit)
uint32_t __cdecl Fn8F3D00() {
    return Thiscall(0x0049A650, 0x01B64DB8u);  // NameKey_Destruct
}

// 0x8F3D10 (exit)
uint32_t __cdecl Fn8F3D10() {
    return Thiscall(0x0049A650, 0x01B64DBCu);  // NameKey_Destruct
}

// 0x8F3D20 (exit)
uint32_t __cdecl Fn8F3D20() {
    return Thiscall(0x0049A650, 0x01B64DC0u);  // NameKey_Destruct
}

// 0x8F3D30 (exit)
uint32_t __cdecl Fn8F3D30() {
    return Thiscall(0x0049A650, 0x01B64DC4u);  // NameKey_Destruct
}

// 0x8F3D40 (exit)
uint32_t __cdecl Fn8F3D40() {
    return Thiscall(0x0049A650, 0x01B64DC8u);  // NameKey_Destruct
}

// 0x8F3D50 (exit)
uint32_t __cdecl Fn8F3D50() {
    return Thiscall(0x0049A650, 0x01B64DCCu);  // NameKey_Destruct
}

// 0x8F3D60 (exit)
uint32_t __cdecl Fn8F3D60() {
    return Thiscall(0x0049A650, 0x01B64DD0u);  // NameKey_Destruct
}

// 0x8F3D70 (exit)
uint32_t __cdecl Fn8F3D70() {
    return Thiscall(0x0049A650, 0x01B64DD4u);  // NameKey_Destruct
}

// 0x8F3D80 (exit)
uint32_t __cdecl Fn8F3D80() {
    return Thiscall(0x0049A650, 0x01B64DD8u);  // NameKey_Destruct
}

// 0x8F3D90 (exit)
uint32_t __cdecl Fn8F3D90() {
    return Thiscall(0x0049A650, 0x01B64DDCu);  // NameKey_Destruct
}

// 0x8F3DA0 (exit)
uint32_t __cdecl Fn8F3DA0() {
    return Thiscall(0x0049A650, 0x01B64DE0u);  // NameKey_Destruct
}

// 0x8F3DB0 (exit)
uint32_t __cdecl Fn8F3DB0() {
    return Thiscall(0x0049A650, 0x01B64DE4u);  // NameKey_Destruct
}

// 0x8F3DC0 (exit)
uint32_t __cdecl Fn8F3DC0() {
    return Thiscall(0x0049A650, 0x01B64DE8u);  // NameKey_Destruct
}

// 0x8F3DD0 (exit)
uint32_t __cdecl Fn8F3DD0() {
    return Thiscall(0x0049A650, 0x01B64DECu);  // NameKey_Destruct
}

// 0x8F3DE0 (exit)
uint32_t __cdecl Fn8F3DE0() {
    return Thiscall(0x0049A650, 0x01B64DF0u);  // NameKey_Destruct
}

// 0x8F3DF0 (exit)
uint32_t __cdecl Fn8F3DF0() {
    return Thiscall(0x0049A650, 0x01B64DF4u);  // NameKey_Destruct
}

// 0x8F3E00 (exit)
uint32_t __cdecl Fn8F3E00() {
    return Thiscall(0x0049A650, 0x01B64DF8u);  // NameKey_Destruct
}

// 0x8F3E10 (exit)
uint32_t __cdecl Fn8F3E10() {
    return Thiscall(0x0049A650, 0x01B64DFCu);  // NameKey_Destruct
}

// 0x8F3E20 (exit)
uint32_t __cdecl Fn8F3E20() {
    return Thiscall(0x0049A650, 0x01B64E00u);  // NameKey_Destruct
}

// 0x8F3E30 (exit)
uint32_t __cdecl Fn8F3E30() {
    return Thiscall(0x0049A650, 0x01B64E04u);  // NameKey_Destruct
}

// 0x8F3E40 (exit)
uint32_t __cdecl Fn8F3E40() {
    return Thiscall(0x0049A650, 0x01B64E08u);  // NameKey_Destruct
}

// 0x8F3E50 (exit)
uint32_t __cdecl Fn8F3E50() {
    return Thiscall(0x0049A650, 0x01B64E0Cu);  // NameKey_Destruct
}

// 0x8F3E60 (exit)
uint32_t __cdecl Fn8F3E60() {
    return Thiscall(0x0049A650, 0x01B64E10u);  // NameKey_Destruct
}

// 0x8F3E70 (exit)
uint32_t __cdecl Fn8F3E70() {
    return Thiscall(0x0049A650, 0x01B64E14u);  // NameKey_Destruct
}

// 0x8F3E80 (exit)
uint32_t __cdecl Fn8F3E80() {
    return Thiscall(0x0049A650, 0x01B64E18u);  // NameKey_Destruct
}

// 0x8F3E90 (exit)
uint32_t __cdecl Fn8F3E90() {
    return Thiscall(0x0049A650, 0x01B64E1Cu);  // NameKey_Destruct
}

// 0x8F3EA0 (exit)
uint32_t __cdecl Fn8F3EA0() {
    return Thiscall(0x0049A650, 0x01B64E20u);  // NameKey_Destruct
}

// 0x8F3EB0 (exit)
uint32_t __cdecl Fn8F3EB0() {
    return Thiscall(0x0049A650, 0x01B64E24u);  // NameKey_Destruct
}

// 0x8F3EC0 (exit)
uint32_t __cdecl Fn8F3EC0() {
    return Thiscall(0x0049A650, 0x01B64E28u);  // NameKey_Destruct
}

// 0x8F3ED0 (exit)
uint32_t __cdecl Fn8F3ED0() {
    return Thiscall(0x0049A650, 0x01B64E2Cu);  // NameKey_Destruct
}

// 0x8F3EE0 (exit)
uint32_t __cdecl Fn8F3EE0() {
    return Thiscall(0x0049A650, 0x01B64E30u);  // NameKey_Destruct
}

// 0x8F3EF0 (exit)
uint32_t __cdecl Fn8F3EF0() {
    return Thiscall(0x0049A650, 0x01B64E34u);  // NameKey_Destruct
}

// 0x8F3F00 (exit)
uint32_t __cdecl Fn8F3F00() {
    return Thiscall(0x0049A650, 0x01B64E38u);  // NameKey_Destruct
}

// 0x8F3F10 (exit)
uint32_t __cdecl Fn8F3F10() {
    return Thiscall(0x0049A650, 0x01B64E3Cu);  // NameKey_Destruct
}

// 0x8F3F20 (exit)
uint32_t __cdecl Fn8F3F20() {
    return Thiscall(0x0049A650, 0x01B64E40u);  // NameKey_Destruct
}

// 0x8F3F30 (exit)
uint32_t __cdecl Fn8F3F30() {
    return Thiscall(0x0049A650, 0x01B64E44u);  // NameKey_Destruct
}

// 0x8F3F40 (exit)
uint32_t __cdecl Fn8F3F40() {
    return Thiscall(0x0049A650, 0x01B64E48u);  // NameKey_Destruct
}

// 0x8F3F50 (exit)
uint32_t __cdecl Fn8F3F50() {
    return Thiscall(0x0049A650, 0x01B64E4Cu);  // NameKey_Destruct
}

// 0x8F3F60 (exit)
uint32_t __cdecl Fn8F3F60() {
    return Thiscall(0x0049A650, 0x01B64E50u);  // NameKey_Destruct
}

// 0x8F3F70 (exit)
uint32_t __cdecl Fn8F3F70() {
    return Thiscall(0x0049A650, 0x01B64E54u);  // NameKey_Destruct
}

// 0x8F3F80 (exit)
uint32_t __cdecl Fn8F3F80() {
    return Thiscall(0x0049A650, 0x01B64E58u);  // NameKey_Destruct
}

// 0x8F3F90 (exit)
uint32_t __cdecl Fn8F3F90() {
    return Thiscall(0x0049A650, 0x01B64E5Cu);  // NameKey_Destruct
}

// 0x8F3FA0 (exit)
uint32_t __cdecl Fn8F3FA0() {
    return Thiscall(0x0049A650, 0x01B64E60u);  // NameKey_Destruct
}

// 0x8F3FB0 (exit)
uint32_t __cdecl Fn8F3FB0() {
    return Thiscall(0x0049A650, 0x01B64E64u);  // NameKey_Destruct
}

// 0x8F3FC0 (exit)
uint32_t __cdecl Fn8F3FC0() {
    return Thiscall(0x0049A650, 0x01B64E68u);  // NameKey_Destruct
}

// 0x8F3FD0 (exit)
uint32_t __cdecl Fn8F3FD0() {
    return Thiscall(0x0049A650, 0x01B64E6Cu);  // NameKey_Destruct
}

// 0x8F3FE0 (exit)
uint32_t __cdecl Fn8F3FE0() {
    return Thiscall(0x0049A650, 0x01B64E70u);  // NameKey_Destruct
}

// 0x8F3FF0 (exit)
uint32_t __cdecl Fn8F3FF0() {
    return Thiscall(0x0049A650, 0x01B64E74u);  // NameKey_Destruct
}

// 0x8F4000 (exit)
uint32_t __cdecl Fn8F4000() {
    return Thiscall(0x0049A650, 0x01B64E78u);  // NameKey_Destruct
}

// 0x8F4010 (exit)
uint32_t __cdecl Fn8F4010() {
    return Thiscall(0x0049A650, 0x01B64E7Cu);  // NameKey_Destruct
}

// 0x8F4020 (exit)
uint32_t __cdecl Fn8F4020() {
    return Thiscall(0x0049A650, 0x01B64E80u);  // NameKey_Destruct
}

// 0x8F4030 (exit)
uint32_t __cdecl Fn8F4030() {
    return Thiscall(0x0049A650, 0x01B64E84u);  // NameKey_Destruct
}

// 0x8F4040 (exit)
uint32_t __cdecl Fn8F4040() {
    return Thiscall(0x0049A650, 0x01B64E88u);  // NameKey_Destruct
}

// 0x8F4050 (exit)
uint32_t __cdecl Fn8F4050() {
    return Thiscall(0x0049A650, 0x01B64E8Cu);  // NameKey_Destruct
}

// 0x8F4060 (exit)
uint32_t __cdecl Fn8F4060() {
    return Thiscall(0x0049A650, 0x01B64E90u);  // NameKey_Destruct
}

// 0x8F4070 (exit)
uint32_t __cdecl Fn8F4070() {
    return Thiscall(0x0049A650, 0x01B64E94u);  // NameKey_Destruct
}

// 0x8F4080 (exit)
uint32_t __cdecl Fn8F4080() {
    return Thiscall(0x0049A650, 0x01B64E98u);  // NameKey_Destruct
}

// 0x8F4090 (exit)
uint32_t __cdecl Fn8F4090() {
    return Thiscall(0x0049A650, 0x01B64E9Cu);  // NameKey_Destruct
}

// 0x8F40A0 (exit)
uint32_t __cdecl Fn8F40A0() {
    return Thiscall(0x0049A650, 0x01B64EA0u);  // NameKey_Destruct
}

// 0x8F40B0 (exit)
uint32_t __cdecl Fn8F40B0() {
    return Thiscall(0x0049A650, 0x01B64EA4u);  // NameKey_Destruct
}

// 0x8F40C0 (exit)
uint32_t __cdecl Fn8F40C0() {
    return Thiscall(0x0049A650, 0x01B64EA8u);  // NameKey_Destruct
}

// 0x8F40D0 (exit)
uint32_t __cdecl Fn8F40D0() {
    return Thiscall(0x0049A650, 0x01B64EACu);  // NameKey_Destruct
}

// 0x8F40E0 (exit)
uint32_t __cdecl Fn8F40E0() {
    return Thiscall(0x0049A650, 0x01B64EB0u);  // NameKey_Destruct
}

// 0x8F40F0 (exit)
uint32_t __cdecl Fn8F40F0() {
    return Thiscall(0x0049A650, 0x01B64EB4u);  // NameKey_Destruct
}

// 0x8F4100 (exit)
uint32_t __cdecl Fn8F4100() {
    return Thiscall(0x0049A650, 0x01B64EB8u);  // NameKey_Destruct
}

// 0x8F4110 (exit)
uint32_t __cdecl Fn8F4110() {
    return Thiscall(0x0049A650, 0x01B64EBCu);  // NameKey_Destruct
}

// 0x8F4120 (exit)
uint32_t __cdecl Fn8F4120() {
    return Thiscall(0x0049A650, 0x01B64EC0u);  // NameKey_Destruct
}

// 0x8F4130 (exit)
uint32_t __cdecl Fn8F4130() {
    return Thiscall(0x0049A650, 0x01B64EC4u);  // NameKey_Destruct
}

// 0x8F4140 (exit)
uint32_t __cdecl Fn8F4140() {
    return Thiscall(0x0049A650, 0x01B64EC8u);  // NameKey_Destruct
}

// 0x8F4150 (exit)
uint32_t __cdecl Fn8F4150() {
    return Thiscall(0x0049A650, 0x01B64ECCu);  // NameKey_Destruct
}

// 0x8F4160 (exit)
uint32_t __cdecl Fn8F4160() {
    return Thiscall(0x0049A650, 0x01B64ED0u);  // NameKey_Destruct
}

// 0x8F4170 (exit)
uint32_t __cdecl Fn8F4170() {
    return Thiscall(0x0049A650, 0x01B64ED4u);  // NameKey_Destruct
}

// 0x8F4180 (exit)
uint32_t __cdecl Fn8F4180() {
    return Thiscall(0x0049A650, 0x01B64ED8u);  // NameKey_Destruct
}

// 0x8F4190 (exit)
uint32_t __cdecl Fn8F4190() {
    return Thiscall(0x0049A650, 0x01B64EDCu);  // NameKey_Destruct
}

// 0x8F41A0 (exit)
uint32_t __cdecl Fn8F41A0() {
    return Thiscall(0x0049A650, 0x01B64EE0u);  // NameKey_Destruct
}

// 0x8F41B0 (exit)
uint32_t __cdecl Fn8F41B0() {
    return Thiscall(0x0049A650, 0x01B64EE4u);  // NameKey_Destruct
}

// 0x8F41C0 (exit)
uint32_t __cdecl Fn8F41C0() {
    return Thiscall(0x0049A650, 0x01B64EE8u);  // NameKey_Destruct
}

// 0x8F41D0 (exit)
uint32_t __cdecl Fn8F41D0() {
    return Thiscall(0x0049A650, 0x01B64EECu);  // NameKey_Destruct
}

// 0x8F41E0 (exit)
uint32_t __cdecl Fn8F41E0() {
    return Thiscall(0x0049A650, 0x01B64EF0u);  // NameKey_Destruct
}

// 0x8F41F0 (exit)
uint32_t __cdecl Fn8F41F0() {
    return Thiscall(0x0049A650, 0x01B64EF4u);  // NameKey_Destruct
}

// 0x8F4200 (exit)
uint32_t __cdecl Fn8F4200() {
    return Thiscall(0x0049A650, 0x01B64EF8u);  // NameKey_Destruct
}

// 0x8F4210 (exit)
uint32_t __cdecl Fn8F4210() {
    return Thiscall(0x0049A650, 0x01B64EFCu);  // NameKey_Destruct
}

// 0x8F4220 (exit)
uint32_t __cdecl Fn8F4220() {
    return Thiscall(0x0049A650, 0x01B64F00u);  // NameKey_Destruct
}

// 0x8F4230 (exit)
uint32_t __cdecl Fn8F4230() {
    return Thiscall(0x0049A650, 0x01B64F04u);  // NameKey_Destruct
}

// 0x8F4240 (exit)
uint32_t __cdecl Fn8F4240() {
    return Thiscall(0x0049A650, 0x01B64F08u);  // NameKey_Destruct
}

// 0x8F4250 (exit)
uint32_t __cdecl Fn8F4250() {
    return Thiscall(0x0049A650, 0x01B64F0Cu);  // NameKey_Destruct
}

// 0x8F4260 (exit)
uint32_t __cdecl Fn8F4260() {
    return Thiscall(0x0049A650, 0x01B64F10u);  // NameKey_Destruct
}

// 0x8F4270 (exit)
uint32_t __cdecl Fn8F4270() {
    return Thiscall(0x0049A650, 0x01B64F14u);  // NameKey_Destruct
}

// 0x8F4280 (exit)
uint32_t __cdecl Fn8F4280() {
    return Thiscall(0x0049A650, 0x01B64F18u);  // NameKey_Destruct
}

// 0x8F4290 (exit)
uint32_t __cdecl Fn8F4290() {
    return Thiscall(0x0049A650, 0x01B64F1Cu);  // NameKey_Destruct
}

// 0x8F42A0 (exit)
uint32_t __cdecl Fn8F42A0() {
    return Thiscall(0x0049A650, 0x01B64F20u);  // NameKey_Destruct
}

// 0x8F42B0 (exit)
uint32_t __cdecl Fn8F42B0() {
    return Thiscall(0x0049A650, 0x01B64F24u);  // NameKey_Destruct
}

// 0x8F42C0 (exit)
uint32_t __cdecl Fn8F42C0() {
    return Thiscall(0x0049A650, 0x01B64F28u);  // NameKey_Destruct
}

// 0x8F42D0 (exit)
uint32_t __cdecl Fn8F42D0() {
    return Thiscall(0x0049A650, 0x01B64F2Cu);  // NameKey_Destruct
}

// 0x8F42E0 (exit)
uint32_t __cdecl Fn8F42E0() {
    return Thiscall(0x0049A650, 0x01B64F30u);  // NameKey_Destruct
}

// 0x8F42F0 (exit)
uint32_t __cdecl Fn8F42F0() {
    return Thiscall(0x0049A650, 0x01B64F34u);  // NameKey_Destruct
}

// 0x8F4300 (exit)
uint32_t __cdecl Fn8F4300() {
    return Thiscall(0x0049A650, 0x01B64F38u);  // NameKey_Destruct
}

// 0x8F4310 (exit)
uint32_t __cdecl Fn8F4310() {
    return Thiscall(0x0049A650, 0x01B64F3Cu);  // NameKey_Destruct
}

// 0x8F4320 (exit)
uint32_t __cdecl Fn8F4320() {
    return Thiscall(0x0049A650, 0x01B64F40u);  // NameKey_Destruct
}

// 0x8F4330 (exit)
uint32_t __cdecl Fn8F4330() {
    return Thiscall(0x0049A650, 0x01B64F44u);  // NameKey_Destruct
}

// 0x8F4340 (exit)
uint32_t __cdecl Fn8F4340() {
    return Thiscall(0x0049A650, 0x01B64F48u);  // NameKey_Destruct
}

// 0x8F4350 (exit)
uint32_t __cdecl Fn8F4350() {
    return Thiscall(0x0049A650, 0x01B64F4Cu);  // NameKey_Destruct
}

// 0x8F4360 (exit)
uint32_t __cdecl Fn8F4360() {
    return Thiscall(0x0049A650, 0x01B64F50u);  // NameKey_Destruct
}

// 0x8F4370 (exit)
uint32_t __cdecl Fn8F4370() {
    return Thiscall(0x0049A650, 0x01B64F54u);  // NameKey_Destruct
}

// 0x8F4380 (exit)
uint32_t __cdecl Fn8F4380() {
    return Thiscall(0x0049A650, 0x01B64F58u);  // NameKey_Destruct
}

// 0x8F4390 (exit)
uint32_t __cdecl Fn8F4390() {
    return Thiscall(0x0049A650, 0x01B64F5Cu);  // NameKey_Destruct
}

// 0x8F43A0 (exit)
uint32_t __cdecl Fn8F43A0() {
    return Thiscall(0x0049A650, 0x01B64F60u);  // NameKey_Destruct
}

// 0x8F43B0 (exit)
uint32_t __cdecl Fn8F43B0() {
    return Thiscall(0x0049A650, 0x01B64F64u);  // NameKey_Destruct
}

// 0x8F43C0 (exit)
uint32_t __cdecl Fn8F43C0() {
    return Thiscall(0x0049A650, 0x01B64F68u);  // NameKey_Destruct
}

// 0x8F43D0 (exit)
uint32_t __cdecl Fn8F43D0() {
    return Thiscall(0x0049A650, 0x01B64F6Cu);  // NameKey_Destruct
}

// 0x8F43E0 (exit)
uint32_t __cdecl Fn8F43E0() {
    return Thiscall(0x0049A650, 0x01B64F70u);  // NameKey_Destruct
}

// 0x8F43F0 (exit)
uint32_t __cdecl Fn8F43F0() {
    return Thiscall(0x0049A650, 0x01B64F74u);  // NameKey_Destruct
}

// 0x8F4400 (exit)
uint32_t __cdecl Fn8F4400() {
    return Thiscall(0x0049A650, 0x01B64F78u);  // NameKey_Destruct
}

// 0x8F4410 (exit)
uint32_t __cdecl Fn8F4410() {
    return Thiscall(0x0049A650, 0x01B64F7Cu);  // NameKey_Destruct
}

// 0x8F4420 (exit)
uint32_t __cdecl Fn8F4420() {
    return Thiscall(0x0049A650, 0x01B64F80u);  // NameKey_Destruct
}

// 0x8F4430 (exit)
uint32_t __cdecl Fn8F4430() {
    return Thiscall(0x0049A650, 0x01B64F84u);  // NameKey_Destruct
}

// 0x8F4440 (exit)
uint32_t __cdecl Fn8F4440() {
    return Thiscall(0x0049A650, 0x01B64F88u);  // NameKey_Destruct
}

// 0x8F4450 (exit)
uint32_t __cdecl Fn8F4450() {
    return Thiscall(0x0049A650, 0x01B64F8Cu);  // NameKey_Destruct
}

// 0x8F4460 (exit)
uint32_t __cdecl Fn8F4460() {
    return Thiscall(0x0049A650, 0x01B64F90u);  // NameKey_Destruct
}

// 0x8F4470 (exit)
uint32_t __cdecl Fn8F4470() {
    return Thiscall(0x0049A650, 0x01B64F94u);  // NameKey_Destruct
}

// 0x8F4480 (exit)
uint32_t __cdecl Fn8F4480() {
    return Thiscall(0x0049A650, 0x01B64F98u);  // NameKey_Destruct
}

// 0x8F4490 (exit)
uint32_t __cdecl Fn8F4490() {
    return Thiscall(0x0049A650, 0x01B64F9Cu);  // NameKey_Destruct
}

// 0x8F44A0 (exit)
uint32_t __cdecl Fn8F44A0() {
    return Thiscall(0x0049A650, 0x01B64FA0u);  // NameKey_Destruct
}

// 0x8F44B0 (exit)
uint32_t __cdecl Fn8F44B0() {
    return Thiscall(0x0049A650, 0x01B64FA4u);  // NameKey_Destruct
}

// 0x8F44C0 (exit)
uint32_t __cdecl Fn8F44C0() {
    return Thiscall(0x0049A650, 0x01B64FA8u);  // NameKey_Destruct
}

// 0x8F44D0 (exit)
uint32_t __cdecl Fn8F44D0() {
    return Thiscall(0x0049A650, 0x01B64FACu);  // NameKey_Destruct
}

// 0x8F44E0 (exit)
uint32_t __cdecl Fn8F44E0() {
    return Thiscall(0x0049A650, 0x01B64FB0u);  // NameKey_Destruct
}

// 0x8F44F0 (exit)
uint32_t __cdecl Fn8F44F0() {
    return Thiscall(0x0049A650, 0x01B64FB4u);  // NameKey_Destruct
}

// 0x8F4500 (exit)
uint32_t __cdecl Fn8F4500() {
    return Thiscall(0x0049A650, 0x01B64FB8u);  // NameKey_Destruct
}

// 0x8F4510 (exit)
uint32_t __cdecl Fn8F4510() {
    return Thiscall(0x0049A650, 0x01B64FBCu);  // NameKey_Destruct
}

// 0x8F4520 (exit)
uint32_t __cdecl Fn8F4520() {
    return Thiscall(0x0049A650, 0x01B64FC0u);  // NameKey_Destruct
}

// 0x8F4530 (exit)
uint32_t __cdecl Fn8F4530() {
    return Thiscall(0x0049A650, 0x01B64FC4u);  // NameKey_Destruct
}

// 0x8F4540 (exit)
uint32_t __cdecl Fn8F4540() {
    return Thiscall(0x0049A650, 0x01B64FC8u);  // NameKey_Destruct
}

// 0x8F4550 (exit)
uint32_t __cdecl Fn8F4550() {
    return Thiscall(0x0049A650, 0x01B64FCCu);  // NameKey_Destruct
}

// 0x8F4560 (exit)
uint32_t __cdecl Fn8F4560() {
    return Thiscall(0x0049A650, 0x01B64FD0u);  // NameKey_Destruct
}

// 0x8F4570 (exit)
uint32_t __cdecl Fn8F4570() {
    return Thiscall(0x0049A650, 0x01B64FD4u);  // NameKey_Destruct
}

// 0x8F4580 (exit)
uint32_t __cdecl Fn8F4580() {
    return Thiscall(0x0049A650, 0x01B64FD8u);  // NameKey_Destruct
}

// 0x8F4590 (exit)
uint32_t __cdecl Fn8F4590() {
    return Thiscall(0x0049A650, 0x01B64FDCu);  // NameKey_Destruct
}

// 0x8F45A0 (exit)
uint32_t __cdecl Fn8F45A0() {
    return Thiscall(0x0049A650, 0x01B64FE0u);  // NameKey_Destruct
}

// 0x8F45B0 (exit)
uint32_t __cdecl Fn8F45B0() {
    return Thiscall(0x0049A650, 0x01B64FE4u);  // NameKey_Destruct
}

// 0x8F45C0 (exit)
uint32_t __cdecl Fn8F45C0() {
    return Thiscall(0x0049A650, 0x01B64FE8u);  // NameKey_Destruct
}

// 0x8F45D0 (exit)
uint32_t __cdecl Fn8F45D0() {
    return Thiscall(0x0049A650, 0x01B64FECu);  // NameKey_Destruct
}

// 0x8F45E0 (exit)
uint32_t __cdecl Fn8F45E0() {
    return Thiscall(0x0049A650, 0x01B64FF0u);  // NameKey_Destruct
}

// 0x8F45F0 (exit)
uint32_t __cdecl Fn8F45F0() {
    return Thiscall(0x0049A650, 0x01B64FF4u);  // NameKey_Destruct
}

// 0x8F4600 (exit)
uint32_t __cdecl Fn8F4600() {
    return Thiscall(0x0049A650, 0x01B64FF8u);  // NameKey_Destruct
}

// 0x8F4610 (exit)
uint32_t __cdecl Fn8F4610() {
    return Thiscall(0x0049A650, 0x01B64FFCu);  // NameKey_Destruct
}

// 0x8F4620 (exit)
uint32_t __cdecl Fn8F4620() {
    return Thiscall(0x0049A650, 0x01B65000u);  // NameKey_Destruct
}

// 0x8F4630 (exit)
uint32_t __cdecl Fn8F4630() {
    return Thiscall(0x0049A650, 0x01B65004u);  // NameKey_Destruct
}

// 0x8F4640 (exit)
uint32_t __cdecl Fn8F4640() {
    return Thiscall(0x0049A650, 0x01B65008u);  // NameKey_Destruct
}

// 0x8F4650 (exit)
uint32_t __cdecl Fn8F4650() {
    return Thiscall(0x0049A650, 0x01B6500Cu);  // NameKey_Destruct
}

// 0x8F4660 (exit)
uint32_t __cdecl Fn8F4660() {
    return Thiscall(0x0049A650, 0x01B65010u);  // NameKey_Destruct
}

// 0x8F4670 (exit)
uint32_t __cdecl Fn8F4670() {
    return Thiscall(0x0049A650, 0x01B65014u);  // NameKey_Destruct
}

// 0x8F4680 (exit)
uint32_t __cdecl Fn8F4680() {
    return Thiscall(0x0049A650, 0x01B65018u);  // NameKey_Destruct
}

// 0x8F4690 (exit)
uint32_t __cdecl Fn8F4690() {
    return Thiscall(0x0049A650, 0x01B6501Cu);  // NameKey_Destruct
}

// 0x8F46A0 (exit)
uint32_t __cdecl Fn8F46A0() {
    return Thiscall(0x0049A650, 0x01B65020u);  // NameKey_Destruct
}

// 0x8F46B0 (exit)
uint32_t __cdecl Fn8F46B0() {
    return Thiscall(0x0049A650, 0x01B65024u);  // NameKey_Destruct
}

// 0x8F46C0 (exit)
uint32_t __cdecl Fn8F46C0() {
    return Thiscall(0x0049A650, 0x01B65028u);  // NameKey_Destruct
}

// 0x8F46D0 (exit)
uint32_t __cdecl Fn8F46D0() {
    return Thiscall(0x0049A650, 0x01B65050u);  // NameKey_Destruct
}

// 0x8F46E0 (exit)
uint32_t __cdecl Fn8F46E0() {
    return Thiscall(0x0049A650, 0x01B65054u);  // NameKey_Destruct
}

// 0x8F46F0 (exit)
uint32_t __cdecl Fn8F46F0() {
    return Thiscall(0x0049A650, 0x01B65058u);  // NameKey_Destruct
}

// 0x8F4700 (exit)
uint32_t __cdecl Fn8F4700() {
    return Thiscall(0x0049A650, 0x01B6505Cu);  // NameKey_Destruct
}

// 0x8F4710 (exit)
uint32_t __cdecl Fn8F4710() {
    return Thiscall(0x0049A650, 0x01B65060u);  // NameKey_Destruct
}

// 0x8F4720 (exit)
uint32_t __cdecl Fn8F4720() {
    return Thiscall(0x0049A650, 0x01B65064u);  // NameKey_Destruct
}

// 0x8F4730 (exit)
uint32_t __cdecl Fn8F4730() {
    return Thiscall(0x0049A650, 0x01B65068u);  // NameKey_Destruct
}

// 0x8F4740 (exit)
uint32_t __cdecl Fn8F4740() {
    return Thiscall(0x0049A650, 0x01B6506Cu);  // NameKey_Destruct
}

// 0x8F4750 (exit)
uint32_t __cdecl Fn8F4750() {
    return Thiscall(0x0049A650, 0x01B65070u);  // NameKey_Destruct
}

// 0x8F4760 (exit)
uint32_t __cdecl Fn8F4760() {
    return Thiscall(0x0049A650, 0x01B65074u);  // NameKey_Destruct
}

// 0x8F4770 (exit)
uint32_t __cdecl Fn8F4770() {
    return Thiscall(0x0049A650, 0x01B65078u);  // NameKey_Destruct
}

// 0x8F4780 (exit)
uint32_t __cdecl Fn8F4780() {
    return Thiscall(0x0049A650, 0x01B6507Cu);  // NameKey_Destruct
}

// 0x8F4790 (exit)
uint32_t __cdecl Fn8F4790() {
    return Thiscall(0x0049A650, 0x01B65080u);  // NameKey_Destruct
}

// 0x8F47A0 (exit)
uint32_t __cdecl Fn8F47A0() {
    return Thiscall(0x0049A650, 0x01B65084u);  // NameKey_Destruct
}

// 0x8F47B0 (exit)
uint32_t __cdecl Fn8F47B0() {
    return Thiscall(0x0049A650, 0x01B65088u);  // NameKey_Destruct
}

// 0x8F47C0 (exit)
uint32_t __cdecl Fn8F47C0() {
    return Thiscall(0x0049A650, 0x01B6508Cu);  // NameKey_Destruct
}

// 0x8F47D0 (exit)
uint32_t __cdecl Fn8F47D0() {
    return Thiscall(0x0049A650, 0x01B65090u);  // NameKey_Destruct
}

// 0x8F47E0 (exit)
uint32_t __cdecl Fn8F47E0() {
    return Thiscall(0x0049A650, 0x01B65094u);  // NameKey_Destruct
}

// 0x8F47F0 (exit)
uint32_t __cdecl Fn8F47F0() {
    return Thiscall(0x0049A650, 0x01B65098u);  // NameKey_Destruct
}

// 0x8F4800 (exit)
uint32_t __cdecl Fn8F4800() {
    return Thiscall(0x0049A650, 0x01B6509Cu);  // NameKey_Destruct
}

// 0x8F4810 (exit)
uint32_t __cdecl Fn8F4810() {
    return Thiscall(0x0049A650, 0x01B650A0u);  // NameKey_Destruct
}

// 0x8F4820 (exit)
uint32_t __cdecl Fn8F4820() {
    return Thiscall(0x0049A650, 0x01B650A4u);  // NameKey_Destruct
}

// 0x8F4830 (exit)
uint32_t __cdecl Fn8F4830() {
    return Thiscall(0x0049A650, 0x01B650A8u);  // NameKey_Destruct
}

// 0x8F4840 (exit)
uint32_t __cdecl Fn8F4840() {
    return Thiscall(0x0049A650, 0x01B650ACu);  // NameKey_Destruct
}

// 0x8F4850 (exit)
uint32_t __cdecl Fn8F4850() {
    return Thiscall(0x0049A650, 0x01B650B0u);  // NameKey_Destruct
}

// 0x8F4860 (exit)
uint32_t __cdecl Fn8F4860() {
    return Thiscall(0x0049A650, 0x01B650B4u);  // NameKey_Destruct
}

// 0x8F4870 (exit)
uint32_t __cdecl Fn8F4870() {
    return Thiscall(0x0049A650, 0x01B650B8u);  // NameKey_Destruct
}

// 0x8F4880 (exit)
uint32_t __cdecl Fn8F4880() {
    return Thiscall(0x0049A650, 0x01B650BCu);  // NameKey_Destruct
}

// 0x8F4890 (exit)
uint32_t __cdecl Fn8F4890() {
    return Thiscall(0x0049A650, 0x01B650C0u);  // NameKey_Destruct
}

// 0x8F48A0 (exit)
uint32_t __cdecl Fn8F48A0() {
    return Thiscall(0x0049A650, 0x01B650C4u);  // NameKey_Destruct
}

// 0x8F48B0 (exit)
uint32_t __cdecl Fn8F48B0() {
    return Thiscall(0x0049A650, 0x01B650C8u);  // NameKey_Destruct
}

// 0x8F48C0 (exit)
uint32_t __cdecl Fn8F48C0() {
    return Thiscall(0x0049A650, 0x01B650CCu);  // NameKey_Destruct
}

// 0x8F48D0 (exit)
uint32_t __cdecl Fn8F48D0() {
    return Thiscall(0x0049A650, 0x01B650D0u);  // NameKey_Destruct
}

// 0x8F48E0 (exit)
uint32_t __cdecl Fn8F48E0() {
    return Thiscall(0x0049A650, 0x01B650D4u);  // NameKey_Destruct
}

// 0x8F48F0 (exit)
uint32_t __cdecl Fn8F48F0() {
    return Thiscall(0x0049A650, 0x01B650D8u);  // NameKey_Destruct
}

// 0x8F4900 (exit)
uint32_t __cdecl Fn8F4900() {
    return Thiscall(0x0049A650, 0x01B650DCu);  // NameKey_Destruct
}

// 0x8F4910 (exit)
uint32_t __cdecl Fn8F4910() {
    return Thiscall(0x0049A650, 0x01B650E0u);  // NameKey_Destruct
}

// 0x8F4920 (exit)
uint32_t __cdecl Fn8F4920() {
    return Thiscall(0x0049A650, 0x01B650E4u);  // NameKey_Destruct
}

// 0x8F4930 (exit)
uint32_t __cdecl Fn8F4930() {
    return Thiscall(0x0049A650, 0x01B650E8u);  // NameKey_Destruct
}

// 0x8F4940 (exit)
uint32_t __cdecl Fn8F4940() {
    return Thiscall(0x0049A650, 0x01B650ECu);  // NameKey_Destruct
}

// 0x8F4950 (exit)
uint32_t __cdecl Fn8F4950() {
    return Thiscall(0x0049A650, 0x01B650F0u);  // NameKey_Destruct
}

// 0x8F4960 (exit)
uint32_t __cdecl Fn8F4960() {
    return Thiscall(0x0049A650, 0x01B650F4u);  // NameKey_Destruct
}

// 0x8F4970 (exit)
uint32_t __cdecl Fn8F4970() {
    return Thiscall(0x0049A650, 0x01B650F8u);  // NameKey_Destruct
}

// 0x8F4980 (exit)
uint32_t __cdecl Fn8F4980() {
    return Thiscall(0x0049A650, 0x01B650FCu);  // NameKey_Destruct
}

// 0x8F4990 (exit)
uint32_t __cdecl Fn8F4990() {
    return Thiscall(0x0049A650, 0x01B65100u);  // NameKey_Destruct
}

// 0x8F49A0 (exit)
uint32_t __cdecl Fn8F49A0() {
    return Thiscall(0x0049A650, 0x01B65104u);  // NameKey_Destruct
}

// 0x8F49B0 (exit)
uint32_t __cdecl Fn8F49B0() {
    return Thiscall(0x0049A650, 0x01B65108u);  // NameKey_Destruct
}

// 0x8F49C0 (exit)
uint32_t __cdecl Fn8F49C0() {
    return Thiscall(0x0049A650, 0x01B6510Cu);  // NameKey_Destruct
}

// 0x8F49D0 (exit)
uint32_t __cdecl Fn8F49D0() {
    return Thiscall(0x0049A650, 0x01B65110u);  // NameKey_Destruct
}

// 0x8F49E0 (exit)
uint32_t __cdecl Fn8F49E0() {
    return Thiscall(0x0049A650, 0x01B65114u);  // NameKey_Destruct
}

// 0x8F49F0 (exit)
uint32_t __cdecl Fn8F49F0() {
    return Thiscall(0x0049A650, 0x01B65118u);  // NameKey_Destruct
}

// 0x8F4A00 (exit)
uint32_t __cdecl Fn8F4A00() {
    return Thiscall(0x0049A650, 0x01B6511Cu);  // NameKey_Destruct
}

// 0x8F4A10 (exit)
uint32_t __cdecl Fn8F4A10() {
    return Thiscall(0x0049A650, 0x01B65120u);  // NameKey_Destruct
}

// 0x8F4A20 (exit)
uint32_t __cdecl Fn8F4A20() {
    return Thiscall(0x0049A650, 0x01B65124u);  // NameKey_Destruct
}

// 0x8F4A30 (exit)
uint32_t __cdecl Fn8F4A30() {
    return Thiscall(0x0049A650, 0x01B65128u);  // NameKey_Destruct
}

// 0x8F4A40 (exit)
uint32_t __cdecl Fn8F4A40() {
    return Thiscall(0x0049A650, 0x01B6512Cu);  // NameKey_Destruct
}

// 0x8F4A50 (exit)
uint32_t __cdecl Fn8F4A50() {
    return Thiscall(0x0049A650, 0x01B65130u);  // NameKey_Destruct
}

// 0x8F4A60 (exit)
uint32_t __cdecl Fn8F4A60() {
    return Thiscall(0x0049A650, 0x01B65134u);  // NameKey_Destruct
}

// 0x8F4A70 (exit)
uint32_t __cdecl Fn8F4A70() {
    return Thiscall(0x0049A650, 0x01B65138u);  // NameKey_Destruct
}

// 0x8F4A80 (exit)
uint32_t __cdecl Fn8F4A80() {
    return Thiscall(0x0049A650, 0x01B6513Cu);  // NameKey_Destruct
}

// 0x8F4A90 (exit)
uint32_t __cdecl Fn8F4A90() {
    return Thiscall(0x0049A650, 0x01B65140u);  // NameKey_Destruct
}

// 0x8F4AA0 (exit)
uint32_t __cdecl Fn8F4AA0() {
    return Thiscall(0x0049A650, 0x01B65144u);  // NameKey_Destruct
}

// 0x8F4AB0 (exit)
uint32_t __cdecl Fn8F4AB0() {
    return Thiscall(0x0049A650, 0x01B65148u);  // NameKey_Destruct
}

// 0x8F4AC0 (exit)
uint32_t __cdecl Fn8F4AC0() {
    return Thiscall(0x0049A650, 0x01B6514Cu);  // NameKey_Destruct
}

// 0x8F4AD0 (exit)
uint32_t __cdecl Fn8F4AD0() {
    return Thiscall(0x0049A650, 0x01B65150u);  // NameKey_Destruct
}

// 0x8F4AE0 (exit)
uint32_t __cdecl Fn8F4AE0() {
    return Thiscall(0x0049A650, 0x01B65154u);  // NameKey_Destruct
}

// 0x8F4AF0 (exit)
uint32_t __cdecl Fn8F4AF0() {
    return Thiscall(0x0049A650, 0x01B65158u);  // NameKey_Destruct
}

// 0x8F4B00 (exit)
uint32_t __cdecl Fn8F4B00() {
    return Thiscall(0x0049A650, 0x01B6515Cu);  // NameKey_Destruct
}

// 0x8F4B10 (exit)
uint32_t __cdecl Fn8F4B10() {
    return Thiscall(0x0049A650, 0x01B65160u);  // NameKey_Destruct
}

// 0x8F4B20 (exit)
uint32_t __cdecl Fn8F4B20() {
    return Thiscall(0x0049A650, 0x01B65164u);  // NameKey_Destruct
}

// 0x8F4B30 (exit)
uint32_t __cdecl Fn8F4B30() {
    return Thiscall(0x0049A650, 0x01B65168u);  // NameKey_Destruct
}

// 0x8F4B40 (exit)
uint32_t __cdecl Fn8F4B40() {
    return Thiscall(0x0049A650, 0x01B6516Cu);  // NameKey_Destruct
}

// 0x8F4B50 (exit)
uint32_t __cdecl Fn8F4B50() {
    return Thiscall(0x0049A650, 0x01B65170u);  // NameKey_Destruct
}

// 0x8F4B60 (exit)
uint32_t __cdecl Fn8F4B60() {
    return Thiscall(0x0049A650, 0x01B65174u);  // NameKey_Destruct
}

// 0x8F4B70 (exit)
uint32_t __cdecl Fn8F4B70() {
    return Thiscall(0x0049A650, 0x01B65178u);  // NameKey_Destruct
}

// 0x8F4B80 (exit)
uint32_t __cdecl Fn8F4B80() {
    return Thiscall(0x0049A650, 0x01B6517Cu);  // NameKey_Destruct
}

// 0x8F4B90 (exit)
uint32_t __cdecl Fn8F4B90() {
    return Thiscall(0x0049A650, 0x01B65180u);  // NameKey_Destruct
}

// 0x8F4BA0 (exit)
uint32_t __cdecl Fn8F4BA0() {
    return Thiscall(0x0049A650, 0x01B65184u);  // NameKey_Destruct
}

// 0x8F4BB0 (exit)
uint32_t __cdecl Fn8F4BB0() {
    return Thiscall(0x0049A650, 0x01B65188u);  // NameKey_Destruct
}

// 0x8F4BC0 (exit)
uint32_t __cdecl Fn8F4BC0() {
    return Thiscall(0x0049A650, 0x01B6518Cu);  // NameKey_Destruct
}

// 0x8F4BD0 (exit)
uint32_t __cdecl Fn8F4BD0() {
    return Thiscall(0x0049A650, 0x01B65190u);  // NameKey_Destruct
}

// 0x8F4BE0 (exit)
uint32_t __cdecl Fn8F4BE0() {
    return Thiscall(0x0049A650, 0x01B65194u);  // NameKey_Destruct
}

// 0x8F4BF0 (exit)
uint32_t __cdecl Fn8F4BF0() {
    return Thiscall(0x0049A650, 0x01B65198u);  // NameKey_Destruct
}

// 0x8F4C00 (exit)
uint32_t __cdecl Fn8F4C00() {
    return Thiscall(0x0049A650, 0x01B6519Cu);  // NameKey_Destruct
}

// 0x8F4C10 (exit)
uint32_t __cdecl Fn8F4C10() {
    return Thiscall(0x0049A650, 0x01B651A0u);  // NameKey_Destruct
}

// 0x8F4C20 (exit)
uint32_t __cdecl Fn8F4C20() {
    return Thiscall(0x0049A650, 0x01B651A4u);  // NameKey_Destruct
}

// 0x8F4C30 (exit)
uint32_t __cdecl Fn8F4C30() {
    return Thiscall(0x0049A650, 0x01B651A8u);  // NameKey_Destruct
}

// 0x8F4C40 (exit)
uint32_t __cdecl Fn8F4C40() {
    return Thiscall(0x0049A650, 0x01B651ACu);  // NameKey_Destruct
}

// 0x8F4C50 (exit)
uint32_t __cdecl Fn8F4C50() {
    return Thiscall(0x0049A650, 0x01B651B0u);  // NameKey_Destruct
}

// 0x8F4C60 (exit)
uint32_t __cdecl Fn8F4C60() {
    return Thiscall(0x0049A650, 0x01B651B4u);  // NameKey_Destruct
}

// 0x8F4C70 (exit)
uint32_t __cdecl Fn8F4C70() {
    return Thiscall(0x0049A650, 0x01B651B8u);  // NameKey_Destruct
}

// 0x8F4C80 (exit)
uint32_t __cdecl Fn8F4C80() {
    return Thiscall(0x0049A650, 0x01B651BCu);  // NameKey_Destruct
}

// 0x8F4C90 (exit)
uint32_t __cdecl Fn8F4C90() {
    return Thiscall(0x0049A650, 0x01B651C0u);  // NameKey_Destruct
}

// 0x8F4CA0 (exit)
uint32_t __cdecl Fn8F4CA0() {
    return Thiscall(0x0049A650, 0x01B651C4u);  // NameKey_Destruct
}

// 0x8F4CB0 (exit)
uint32_t __cdecl Fn8F4CB0() {
    return Thiscall(0x0049A650, 0x01B651C8u);  // NameKey_Destruct
}

// 0x8F4CC0 (exit)
uint32_t __cdecl Fn8F4CC0() {
    return Thiscall(0x0049A650, 0x01B651CCu);  // NameKey_Destruct
}

// 0x8F4CD0 (exit)
uint32_t __cdecl Fn8F4CD0() {
    return Thiscall(0x0049A650, 0x01B651D0u);  // NameKey_Destruct
}

// 0x8F4CE0 (exit)
uint32_t __cdecl Fn8F4CE0() {
    return Thiscall(0x0049A650, 0x01B651D4u);  // NameKey_Destruct
}

// 0x8F4CF0 (exit)
uint32_t __cdecl Fn8F4CF0() {
    return Thiscall(0x0049A650, 0x01B651D8u);  // NameKey_Destruct
}

// 0x8F4D00 (exit)
uint32_t __cdecl Fn8F4D00() {
    return Thiscall(0x0049A650, 0x01B651DCu);  // NameKey_Destruct
}

// 0x8F4D10 (exit)
uint32_t __cdecl Fn8F4D10() {
    return Thiscall(0x0049A650, 0x01B651E0u);  // NameKey_Destruct
}

// 0x8F4D20 (exit)
uint32_t __cdecl Fn8F4D20() {
    return Thiscall(0x0049A650, 0x01B651E4u);  // NameKey_Destruct
}

// 0x8F4D30 (exit)
uint32_t __cdecl Fn8F4D30() {
    return Thiscall(0x0049A650, 0x01B651E8u);  // NameKey_Destruct
}

// 0x8F4D40 (exit)
uint32_t __cdecl Fn8F4D40() {
    return Thiscall(0x0049A650, 0x01B651ECu);  // NameKey_Destruct
}

// 0x8F4D50 (exit)
uint32_t __cdecl Fn8F4D50() {
    return Thiscall(0x0049A650, 0x01B651F0u);  // NameKey_Destruct
}

// 0x8F4D60 (exit)
uint32_t __cdecl Fn8F4D60() {
    return Thiscall(0x0049A650, 0x01B651F4u);  // NameKey_Destruct
}

// 0x8F4D70 (exit)
uint32_t __cdecl Fn8F4D70() {
    return Thiscall(0x0049A650, 0x01B651F8u);  // NameKey_Destruct
}

// 0x8F4D80 (exit)
uint32_t __cdecl Fn8F4D80() {
    return Thiscall(0x0049A650, 0x01B651FCu);  // NameKey_Destruct
}

// 0x8F4D90 (exit)
uint32_t __cdecl Fn8F4D90() {
    return Thiscall(0x0049A650, 0x01B65200u);  // NameKey_Destruct
}

// 0x8F4DA0 (exit)
uint32_t __cdecl Fn8F4DA0() {
    return Thiscall(0x0049A650, 0x01B65204u);  // NameKey_Destruct
}

// 0x8F4DB0 (exit)
uint32_t __cdecl Fn8F4DB0() {
    return Thiscall(0x0049A650, 0x01B65208u);  // NameKey_Destruct
}

// 0x8F4DC0 (exit)
uint32_t __cdecl Fn8F4DC0() {
    return Thiscall(0x0049A650, 0x01B6520Cu);  // NameKey_Destruct
}

// 0x8F4DD0 (exit)
uint32_t __cdecl Fn8F4DD0() {
    return Thiscall(0x0049A650, 0x01B65210u);  // NameKey_Destruct
}

// 0x8F4DE0 (exit)
uint32_t __cdecl Fn8F4DE0() {
    return Thiscall(0x0049A650, 0x01B65214u);  // NameKey_Destruct
}

// 0x8F4DF0 (exit)
uint32_t __cdecl Fn8F4DF0() {
    return Thiscall(0x0049A650, 0x01B65218u);  // NameKey_Destruct
}

// 0x8F4E00 (exit)
uint32_t __cdecl Fn8F4E00() {
    return Thiscall(0x0049A650, 0x01B6521Cu);  // NameKey_Destruct
}

// 0x8F4E10 (exit)
uint32_t __cdecl Fn8F4E10() {
    return Thiscall(0x0049A650, 0x01B65220u);  // NameKey_Destruct
}

// 0x8F4E20 (exit)
uint32_t __cdecl Fn8F4E20() {
    return Thiscall(0x0049A650, 0x01B65224u);  // NameKey_Destruct
}

// 0x8F4E30 (exit)
uint32_t __cdecl Fn8F4E30() {
    return Thiscall(0x0049A650, 0x01B65228u);  // NameKey_Destruct
}

// 0x8F4E40 (exit)
uint32_t __cdecl Fn8F4E40() {
    return Thiscall(0x0049A650, 0x01B6522Cu);  // NameKey_Destruct
}

// 0x8F4E50 (exit)
uint32_t __cdecl Fn8F4E50() {
    return Thiscall(0x0049A650, 0x01B65230u);  // NameKey_Destruct
}

// 0x8F4E60 (exit)
uint32_t __cdecl Fn8F4E60() {
    return Thiscall(0x0049A650, 0x01B65234u);  // NameKey_Destruct
}

// 0x8F4E70 (exit)
uint32_t __cdecl Fn8F4E70() {
    return Thiscall(0x0049A650, 0x01B65238u);  // NameKey_Destruct
}

// 0x8F4E80 (exit)
uint32_t __cdecl Fn8F4E80() {
    return Thiscall(0x0049A650, 0x01B6523Cu);  // NameKey_Destruct
}

// 0x8F4E90 (exit)
uint32_t __cdecl Fn8F4E90() {
    return Thiscall(0x0049A650, 0x01B65240u);  // NameKey_Destruct
}

// 0x8F4EA0 (exit)
uint32_t __cdecl Fn8F4EA0() {
    return Thiscall(0x0049A650, 0x01B65244u);  // NameKey_Destruct
}

// 0x8F4EB0 (exit)
uint32_t __cdecl Fn8F4EB0() {
    return Thiscall(0x0049A650, 0x01B65248u);  // NameKey_Destruct
}

// 0x8F4EC0 (exit)
uint32_t __cdecl Fn8F4EC0() {
    return Thiscall(0x0049A650, 0x01B6524Cu);  // NameKey_Destruct
}

// 0x8F4ED0 (exit)
uint32_t __cdecl Fn8F4ED0() {
    return Thiscall(0x0049A650, 0x01B65260u);  // NameKey_Destruct
}

// 0x8F4EE0 (exit)
uint32_t __cdecl Fn8F4EE0() {
    return Thiscall(0x0049A650, 0x01B65264u);  // NameKey_Destruct
}

// 0x8F4EF0 (exit)
uint32_t __cdecl Fn8F4EF0() {
    return Thiscall(0x0049A650, 0x01B65268u);  // NameKey_Destruct
}

// 0x8F4F00 (exit)
uint32_t __cdecl Fn8F4F00() {
    return Thiscall(0x0049A650, 0x01B6526Cu);  // NameKey_Destruct
}

// 0x8F4F10 (exit)
uint32_t __cdecl Fn8F4F10() {
    return Thiscall(0x0049A650, 0x01B65270u);  // NameKey_Destruct
}

// 0x8F4F20 (exit)
uint32_t __cdecl Fn8F4F20() {
    return Thiscall(0x0049A650, 0x01B65274u);  // NameKey_Destruct
}

// 0x8F4F30 (exit)
uint32_t __cdecl Fn8F4F30() {
    return Thiscall(0x0049A650, 0x01B65278u);  // NameKey_Destruct
}

// 0x8F4F40 (exit)
uint32_t __cdecl Fn8F4F40() {
    return Thiscall(0x0049A650, 0x01B6527Cu);  // NameKey_Destruct
}

// 0x8F4F50 (exit)
uint32_t __cdecl Fn8F4F50() {
    return Thiscall(0x0049A650, 0x01B65280u);  // NameKey_Destruct
}

// 0x8F4F60 (exit)
uint32_t __cdecl Fn8F4F60() {
    return Thiscall(0x0049A650, 0x01B65284u);  // NameKey_Destruct
}

// 0x8F4F70 (exit)
uint32_t __cdecl Fn8F4F70() {
    return Thiscall(0x0049A650, 0x01B65288u);  // NameKey_Destruct
}

// 0x8F4F80 (exit)
uint32_t __cdecl Fn8F4F80() {
    return Thiscall(0x0049A650, 0x01B6528Cu);  // NameKey_Destruct
}

// 0x8F4F90 (exit)
uint32_t __cdecl Fn8F4F90() {
    return Thiscall(0x0049A650, 0x01B65290u);  // NameKey_Destruct
}

// 0x8F4FA0 (exit)
uint32_t __cdecl Fn8F4FA0() {
    return Thiscall(0x0049A650, 0x01B65294u);  // NameKey_Destruct
}

// 0x8F4FB0 (exit)
uint32_t __cdecl Fn8F4FB0() {
    return Thiscall(0x0049A650, 0x01B65298u);  // NameKey_Destruct
}

// 0x8F4FC0 (exit)
uint32_t __cdecl Fn8F4FC0() {
    return Thiscall(0x0049A650, 0x01B6529Cu);  // NameKey_Destruct
}

// 0x8F4FD0 (exit)
uint32_t __cdecl Fn8F4FD0() {
    return Thiscall(0x0049A650, 0x01B652A0u);  // NameKey_Destruct
}

// 0x8F4FE0 (exit)
uint32_t __cdecl Fn8F4FE0() {
    return Thiscall(0x0049A650, 0x01B652A4u);  // NameKey_Destruct
}

// 0x8F4FF0 (exit)
uint32_t __cdecl Fn8F4FF0() {
    return Thiscall(0x0049A650, 0x01B652A8u);  // NameKey_Destruct
}

// 0x8F5000 (exit)
uint32_t __cdecl Fn8F5000() {
    return Thiscall(0x0049A650, 0x01B652ACu);  // NameKey_Destruct
}

// 0x8F5010 (exit)
uint32_t __cdecl Fn8F5010() {
    return Thiscall(0x0049A650, 0x01B652B0u);  // NameKey_Destruct
}

// 0x8F5020 (exit)
uint32_t __cdecl Fn8F5020() {
    return Thiscall(0x0049A650, 0x01B652B4u);  // NameKey_Destruct
}

// 0x8F5030 (exit)
uint32_t __cdecl Fn8F5030() {
    return Thiscall(0x0049A650, 0x01B652B8u);  // NameKey_Destruct
}

// 0x8F5040 (exit)
uint32_t __cdecl Fn8F5040() {
    return Thiscall(0x0049A650, 0x01B652BCu);  // NameKey_Destruct
}

// 0x8F5050 (exit)
uint32_t __cdecl Fn8F5050() {
    return Thiscall(0x0049A650, 0x01B652C0u);  // NameKey_Destruct
}

// 0x8F5060 (exit)
uint32_t __cdecl Fn8F5060() {
    return Thiscall(0x0049A650, 0x01B652C4u);  // NameKey_Destruct
}

// 0x8F5070 (exit)
uint32_t __cdecl Fn8F5070() {
    return Thiscall(0x0049A650, 0x01B652C8u);  // NameKey_Destruct
}

// 0x8F5080 (exit)
uint32_t __cdecl Fn8F5080() {
    return Thiscall(0x0049A650, 0x01B652CCu);  // NameKey_Destruct
}

// 0x8F5090 (exit)
uint32_t __cdecl Fn8F5090() {
    return Thiscall(0x0049A650, 0x01B652D0u);  // NameKey_Destruct
}

// 0x8F50A0 (exit)
uint32_t __cdecl Fn8F50A0() {
    return Thiscall(0x0049A650, 0x01B652D4u);  // NameKey_Destruct
}

// 0x8F50B0 (exit)
uint32_t __cdecl Fn8F50B0() {
    return Thiscall(0x0049A650, 0x01B652D8u);  // NameKey_Destruct
}

// 0x8F50C0 (exit)
uint32_t __cdecl Fn8F50C0() {
    return Thiscall(0x0049A650, 0x01B652DCu);  // NameKey_Destruct
}

// 0x8F50D0 (exit)
uint32_t __cdecl Fn8F50D0() {
    return Thiscall(0x0049A650, 0x01B652E0u);  // NameKey_Destruct
}

// 0x8F50E0 (exit)
uint32_t __cdecl Fn8F50E0() {
    return Thiscall(0x0049A650, 0x01B652E4u);  // NameKey_Destruct
}

// 0x8F50F0 (exit)
uint32_t __cdecl Fn8F50F0() {
    return Thiscall(0x0049A650, 0x01B652E8u);  // NameKey_Destruct
}

// 0x8F5100 (exit)
uint32_t __cdecl Fn8F5100() {
    return Thiscall(0x0049A650, 0x01B652ECu);  // NameKey_Destruct
}

// 0x8F5110 (exit)
uint32_t __cdecl Fn8F5110() {
    return Thiscall(0x0049A650, 0x01B652F0u);  // NameKey_Destruct
}

// 0x8F5120 (exit)
uint32_t __cdecl Fn8F5120() {
    return Thiscall(0x0049A650, 0x01B652F4u);  // NameKey_Destruct
}

// 0x8F5130 (exit)
uint32_t __cdecl Fn8F5130() {
    return Thiscall(0x0049A650, 0x01B652F8u);  // NameKey_Destruct
}

// 0x8F5140 (exit)
uint32_t __cdecl Fn8F5140() {
    return Thiscall(0x0049A650, 0x01B652FCu);  // NameKey_Destruct
}

// 0x8F5150 (exit)
uint32_t __cdecl Fn8F5150() {
    return Thiscall(0x0049A650, 0x01B65300u);  // NameKey_Destruct
}

// 0x8F5160 (exit)
uint32_t __cdecl Fn8F5160() {
    return Thiscall(0x0049A650, 0x01B65304u);  // NameKey_Destruct
}

// 0x8F5170 (exit)
uint32_t __cdecl Fn8F5170() {
    return Thiscall(0x0049A650, 0x01B65308u);  // NameKey_Destruct
}

// 0x8F5180 (exit)
uint32_t __cdecl Fn8F5180() {
    return Thiscall(0x0049A650, 0x01B6530Cu);  // NameKey_Destruct
}

// 0x8F5190 (exit)
uint32_t __cdecl Fn8F5190() {
    return Thiscall(0x0049A650, 0x01B65310u);  // NameKey_Destruct
}

// 0x8F51A0 (exit)
uint32_t __cdecl Fn8F51A0() {
    return Thiscall(0x0049A650, 0x01B65314u);  // NameKey_Destruct
}

// 0x8F51B0 (exit)
uint32_t __cdecl Fn8F51B0() {
    return Thiscall(0x0049A650, 0x01B65318u);  // NameKey_Destruct
}

// 0x8F51C0 (exit)
uint32_t __cdecl Fn8F51C0() {
    return Thiscall(0x0049A650, 0x01B6531Cu);  // NameKey_Destruct
}

// 0x8F51D0 (exit)
uint32_t __cdecl Fn8F51D0() {
    return Thiscall(0x0049A650, 0x01B65320u);  // NameKey_Destruct
}

// 0x8F51E0 (exit)
uint32_t __cdecl Fn8F51E0() {
    return Thiscall(0x0049A650, 0x01B65324u);  // NameKey_Destruct
}

// 0x8F51F0 (exit)
uint32_t __cdecl Fn8F51F0() {
    return Thiscall(0x0049A650, 0x01B65328u);  // NameKey_Destruct
}

// 0x8F5200 (exit)
uint32_t __cdecl Fn8F5200() {
    return Thiscall(0x0049A650, 0x01B6532Cu);  // NameKey_Destruct
}

// 0x8F5210 (exit)
uint32_t __cdecl Fn8F5210() {
    return Thiscall(0x0049A650, 0x01B65330u);  // NameKey_Destruct
}

// 0x8F5220 (exit)
uint32_t __cdecl Fn8F5220() {
    return Thiscall(0x0049A650, 0x01B65334u);  // NameKey_Destruct
}

// 0x8F5230 (exit)
uint32_t __cdecl Fn8F5230() {
    return Thiscall(0x0049A650, 0x01B65338u);  // NameKey_Destruct
}

// 0x8F5240 (exit)
uint32_t __cdecl Fn8F5240() {
    return Thiscall(0x0049A650, 0x01B6533Cu);  // NameKey_Destruct
}

// 0x8F5250 (exit)
uint32_t __cdecl Fn8F5250() {
    return Thiscall(0x0049A650, 0x01B65340u);  // NameKey_Destruct
}

// 0x8F5260 (exit)
uint32_t __cdecl Fn8F5260() {
    return Thiscall(0x0049A650, 0x01B65344u);  // NameKey_Destruct
}

// 0x8F5270 (exit)
uint32_t __cdecl Fn8F5270() {
    return Thiscall(0x0049A650, 0x01B65348u);  // NameKey_Destruct
}

// 0x8F5280 (exit)
uint32_t __cdecl Fn8F5280() {
    return Thiscall(0x0049A650, 0x01B6534Cu);  // NameKey_Destruct
}

// 0x8F5290 (exit)
uint32_t __cdecl Fn8F5290() {
    return Thiscall(0x0049A650, 0x01B65350u);  // NameKey_Destruct
}

// 0x8F52A0 (exit)
uint32_t __cdecl Fn8F52A0() {
    return Thiscall(0x0049A650, 0x01B65354u);  // NameKey_Destruct
}

// 0x8F52B0 (exit)
uint32_t __cdecl Fn8F52B0() {
    return Thiscall(0x0049A650, 0x01B65358u);  // NameKey_Destruct
}

// 0x8F52C0 (exit)
uint32_t __cdecl Fn8F52C0() {
    return Thiscall(0x0049A650, 0x01B6535Cu);  // NameKey_Destruct
}

// 0x8F52D0 (exit)
uint32_t __cdecl Fn8F52D0() {
    return Thiscall(0x0049A650, 0x01B65360u);  // NameKey_Destruct
}

// 0x8F52E0 (exit)
uint32_t __cdecl Fn8F52E0() {
    return Thiscall(0x0049A650, 0x01B65364u);  // NameKey_Destruct
}

// 0x8F52F0 (exit)
uint32_t __cdecl Fn8F52F0() {
    return Thiscall(0x0049A650, 0x01B65368u);  // NameKey_Destruct
}

// 0x8F5300 (exit)
uint32_t __cdecl Fn8F5300() {
    return Thiscall(0x0049A650, 0x01B6536Cu);  // NameKey_Destruct
}

// 0x8F5310 (exit)
uint32_t __cdecl Fn8F5310() {
    return Thiscall(0x0049A650, 0x01B65370u);  // NameKey_Destruct
}

// 0x8F5320 (exit)
uint32_t __cdecl Fn8F5320() {
    return Thiscall(0x0049A650, 0x01B65374u);  // NameKey_Destruct
}

// 0x8F5330 (exit)
uint32_t __cdecl Fn8F5330() {
    return Thiscall(0x0049A650, 0x01B65378u);  // NameKey_Destruct
}

// 0x8F5340 (exit)
uint32_t __cdecl Fn8F5340() {
    return Thiscall(0x0049A650, 0x01B6537Cu);  // NameKey_Destruct
}

// 0x8F5350 (exit)
uint32_t __cdecl Fn8F5350() {
    return Thiscall(0x0049A650, 0x01B65380u);  // NameKey_Destruct
}

// 0x8F5360 (exit)
uint32_t __cdecl Fn8F5360() {
    return Thiscall(0x0049A650, 0x01B65384u);  // NameKey_Destruct
}

// 0x8F5370 (exit)
uint32_t __cdecl Fn8F5370() {
    return Thiscall(0x0049A650, 0x01B65388u);  // NameKey_Destruct
}

// 0x8F5380 (exit)
uint32_t __cdecl Fn8F5380() {
    return Thiscall(0x0049A650, 0x01B6538Cu);  // NameKey_Destruct
}

// 0x8F5390 (exit)
uint32_t __cdecl Fn8F5390() {
    return Thiscall(0x0049A650, 0x01B65390u);  // NameKey_Destruct
}

// 0x8F53A0 (exit)
uint32_t __cdecl Fn8F53A0() {
    return Thiscall(0x0049A650, 0x01B65394u);  // NameKey_Destruct
}

// 0x8F53B0 (exit)
uint32_t __cdecl Fn8F53B0() {
    return Thiscall(0x0049A650, 0x01B65398u);  // NameKey_Destruct
}

// 0x8F53C0 (exit)
uint32_t __cdecl Fn8F53C0() {
    return Thiscall(0x0049A650, 0x01B6539Cu);  // NameKey_Destruct
}

// 0x8F53D0 (exit)
uint32_t __cdecl Fn8F53D0() {
    return Thiscall(0x0049A650, 0x01B653A0u);  // NameKey_Destruct
}

// 0x8F53E0 (exit)
uint32_t __cdecl Fn8F53E0() {
    return Thiscall(0x0049A650, 0x01B653A4u);  // NameKey_Destruct
}

// 0x8F53F0 (exit)
uint32_t __cdecl Fn8F53F0() {
    return Thiscall(0x0049A650, 0x01B653A8u);  // NameKey_Destruct
}

// 0x8F5400 (exit)
uint32_t __cdecl Fn8F5400() {
    return Thiscall(0x0049A650, 0x01B653ACu);  // NameKey_Destruct
}

// 0x8F5410 (exit)
uint32_t __cdecl Fn8F5410() {
    return Thiscall(0x0049A650, 0x01B653B0u);  // NameKey_Destruct
}

// 0x8F5420 (exit)
uint32_t __cdecl Fn8F5420() {
    return Thiscall(0x0049A650, 0x01B653B4u);  // NameKey_Destruct
}

// 0x8F5430 (exit)
uint32_t __cdecl Fn8F5430() {
    return Thiscall(0x0049A650, 0x01B653B8u);  // NameKey_Destruct
}

// 0x8F5440 (exit)
uint32_t __cdecl Fn8F5440() {
    return Thiscall(0x0049A650, 0x01B653BCu);  // NameKey_Destruct
}

// 0x8F5450 (exit)
uint32_t __cdecl Fn8F5450() {
    return Thiscall(0x0049A650, 0x01B653C0u);  // NameKey_Destruct
}

// 0x8F5460 (exit)
uint32_t __cdecl Fn8F5460() {
    return Thiscall(0x0049A650, 0x01B653C4u);  // NameKey_Destruct
}

// 0x8F5470 (exit)
uint32_t __cdecl Fn8F5470() {
    return Thiscall(0x0049A650, 0x01B653C8u);  // NameKey_Destruct
}

// 0x8F5480 (exit)
uint32_t __cdecl Fn8F5480() {
    return Thiscall(0x0049A650, 0x01B653CCu);  // NameKey_Destruct
}

// 0x8F5490 (exit)
uint32_t __cdecl Fn8F5490() {
    return Thiscall(0x0049A650, 0x01B653D0u);  // NameKey_Destruct
}

// 0x8F54A0 (exit)
uint32_t __cdecl Fn8F54A0() {
    return Thiscall(0x0049A650, 0x01B653D4u);  // NameKey_Destruct
}

// 0x8F54B0 (exit)
uint32_t __cdecl Fn8F54B0() {
    return Thiscall(0x0049A650, 0x01B653D8u);  // NameKey_Destruct
}

// 0x8F54C0 (exit)
uint32_t __cdecl Fn8F54C0() {
    return Thiscall(0x0049A650, 0x01B653DCu);  // NameKey_Destruct
}

// 0x8F54D0 (exit)
uint32_t __cdecl Fn8F54D0() {
    return Thiscall(0x0049A650, 0x01B653E0u);  // NameKey_Destruct
}

// 0x8F54E0 (exit)
uint32_t __cdecl Fn8F54E0() {
    return Thiscall(0x0049A650, 0x01B653E4u);  // NameKey_Destruct
}

// 0x8F54F0 (exit)
uint32_t __cdecl Fn8F54F0() {
    return Thiscall(0x0049A650, 0x01B653E8u);  // NameKey_Destruct
}

// 0x8F5500 (exit)
uint32_t __cdecl Fn8F5500() {
    return Thiscall(0x0049A650, 0x01B653ECu);  // NameKey_Destruct
}

// 0x8F5510 (exit)
uint32_t __cdecl Fn8F5510() {
    return Thiscall(0x0049A650, 0x01B653F0u);  // NameKey_Destruct
}

// 0x8F5520 (exit)
uint32_t __cdecl Fn8F5520() {
    return Thiscall(0x0049A650, 0x01B653F4u);  // NameKey_Destruct
}

// 0x8F5530 (exit)
uint32_t __cdecl Fn8F5530() {
    return Thiscall(0x0049A650, 0x01B653F8u);  // NameKey_Destruct
}

// 0x8F5540 (exit)
uint32_t __cdecl Fn8F5540() {
    return Thiscall(0x0049A650, 0x01B653FCu);  // NameKey_Destruct
}

// 0x8F5550 (exit)
uint32_t __cdecl Fn8F5550() {
    return Thiscall(0x0049A650, 0x01B65400u);  // NameKey_Destruct
}

// 0x8F5560 (exit)
uint32_t __cdecl Fn8F5560() {
    return Thiscall(0x0049A650, 0x01B65404u);  // NameKey_Destruct
}

// 0x8F5570 (exit)
uint32_t __cdecl Fn8F5570() {
    return Thiscall(0x0049A650, 0x01B65408u);  // NameKey_Destruct
}

// 0x8F5580 (exit)
uint32_t __cdecl Fn8F5580() {
    return Thiscall(0x0049A650, 0x01B6540Cu);  // NameKey_Destruct
}

// 0x8F5590 (exit)
uint32_t __cdecl Fn8F5590() {
    return Thiscall(0x0049A650, 0x01B65410u);  // NameKey_Destruct
}

// 0x8F55A0 (exit)
uint32_t __cdecl Fn8F55A0() {
    return Thiscall(0x0049A650, 0x01B65414u);  // NameKey_Destruct
}

// 0x8F55B0 (exit)
uint32_t __cdecl Fn8F55B0() {
    return Thiscall(0x0049A650, 0x01B65418u);  // NameKey_Destruct
}

// 0x8F55C0 (exit)
uint32_t __cdecl Fn8F55C0() {
    return Thiscall(0x0049A650, 0x01B6541Cu);  // NameKey_Destruct
}

// 0x8F55D0 (exit)
uint32_t __cdecl Fn8F55D0() {
    return Thiscall(0x0049A650, 0x01B65420u);  // NameKey_Destruct
}

// 0x8F55E0 (exit)
uint32_t __cdecl Fn8F55E0() {
    return Thiscall(0x0049A650, 0x01B65424u);  // NameKey_Destruct
}

// 0x8F55F0 (exit)
uint32_t __cdecl Fn8F55F0() {
    return Thiscall(0x0049A650, 0x01B65428u);  // NameKey_Destruct
}

// 0x8F5600 (exit)
uint32_t __cdecl Fn8F5600() {
    return Thiscall(0x0049A650, 0x01B6542Cu);  // NameKey_Destruct
}

// 0x8F5610 (exit)
uint32_t __cdecl Fn8F5610() {
    return Thiscall(0x0049A650, 0x01B65430u);  // NameKey_Destruct
}

// 0x8F5620 (exit)
uint32_t __cdecl Fn8F5620() {
    return Thiscall(0x0049A650, 0x01B65434u);  // NameKey_Destruct
}

// 0x8F5630 (exit)
uint32_t __cdecl Fn8F5630() {
    return Thiscall(0x0049A650, 0x01B65438u);  // NameKey_Destruct
}

// 0x8F5640 (exit)
uint32_t __cdecl Fn8F5640() {
    return Thiscall(0x0049A650, 0x01B6543Cu);  // NameKey_Destruct
}

// 0x8F5650 (exit)
uint32_t __cdecl Fn8F5650() {
    return Thiscall(0x0049A650, 0x01B65440u);  // NameKey_Destruct
}

// 0x8F5660 (exit)
uint32_t __cdecl Fn8F5660() {
    return Thiscall(0x0049A650, 0x01B65444u);  // NameKey_Destruct
}

// 0x8F5670 (exit)
uint32_t __cdecl Fn8F5670() {
    return Thiscall(0x0049A650, 0x01B65448u);  // NameKey_Destruct
}

// 0x8F5680 (exit)
uint32_t __cdecl Fn8F5680() {
    return Thiscall(0x0049A650, 0x01B6544Cu);  // NameKey_Destruct
}

// 0x8F5690 (exit)
uint32_t __cdecl Fn8F5690() {
    return Thiscall(0x0049A650, 0x01B65450u);  // NameKey_Destruct
}

// 0x8F56A0 (exit)
uint32_t __cdecl Fn8F56A0() {
    return Thiscall(0x0049A650, 0x01B65454u);  // NameKey_Destruct
}

// 0x8F56B0 (exit)
uint32_t __cdecl Fn8F56B0() {
    return Thiscall(0x0049A650, 0x01B65458u);  // NameKey_Destruct
}

// 0x8F56C0 (exit)
uint32_t __cdecl Fn8F56C0() {
    return Thiscall(0x0049A650, 0x01B6545Cu);  // NameKey_Destruct
}

// 0x8F56D0 (exit)
uint32_t __cdecl Fn8F56D0() {
    return Thiscall(0x0049A650, 0x01B65460u);  // NameKey_Destruct
}

// 0x8F56E0 (exit)
uint32_t __cdecl Fn8F56E0() {
    return Thiscall(0x0049A650, 0x01B65464u);  // NameKey_Destruct
}

// 0x8F56F0 (exit)
uint32_t __cdecl Fn8F56F0() {
    return Thiscall(0x0049A650, 0x01B65468u);  // NameKey_Destruct
}

// 0x8F5700 (exit)
uint32_t __cdecl Fn8F5700() {
    return Thiscall(0x0049A650, 0x01B6546Cu);  // NameKey_Destruct
}

// 0x8F5710 (exit)
uint32_t __cdecl Fn8F5710() {
    return Thiscall(0x0049A650, 0x01B65470u);  // NameKey_Destruct
}

// 0x8F5720 (exit)
uint32_t __cdecl Fn8F5720() {
    return Thiscall(0x0049A650, 0x01B65474u);  // NameKey_Destruct
}

// 0x8F5730 (exit)
uint32_t __cdecl Fn8F5730() {
    return Thiscall(0x0049A650, 0x01B65478u);  // NameKey_Destruct
}

// 0x8F5740 (exit)
uint32_t __cdecl Fn8F5740() {
    return Thiscall(0x0049A650, 0x01B6547Cu);  // NameKey_Destruct
}

// 0x8F5750 (exit)
uint32_t __cdecl Fn8F5750() {
    return Thiscall(0x0049A650, 0x01B65480u);  // NameKey_Destruct
}

// 0x8F5760 (exit)
uint32_t __cdecl Fn8F5760() {
    return Thiscall(0x0049A650, 0x01B65484u);  // NameKey_Destruct
}

// 0x8F5770 (exit)
uint32_t __cdecl Fn8F5770() {
    return Thiscall(0x0049A650, 0x01B65488u);  // NameKey_Destruct
}

// 0x8F5780 (exit)
uint32_t __cdecl Fn8F5780() {
    return Thiscall(0x0049A650, 0x01B6548Cu);  // NameKey_Destruct
}

// 0x8F5790 (exit)
uint32_t __cdecl Fn8F5790() {
    return Thiscall(0x0049A650, 0x01B65490u);  // NameKey_Destruct
}

// 0x8F57A0 (exit)
uint32_t __cdecl Fn8F57A0() {
    return Thiscall(0x0049A650, 0x01B65494u);  // NameKey_Destruct
}

// 0x8F57B0 (exit)
uint32_t __cdecl Fn8F57B0() {
    return Thiscall(0x0049A650, 0x01B65498u);  // NameKey_Destruct
}

// 0x8F57C0 (exit)
uint32_t __cdecl Fn8F57C0() {
    return Thiscall(0x0049A650, 0x01B6549Cu);  // NameKey_Destruct
}

// 0x8F57D0 (exit)
uint32_t __cdecl Fn8F57D0() {
    return Thiscall(0x0049A650, 0x01B654A0u);  // NameKey_Destruct
}

// 0x8F57E0 (exit)
uint32_t __cdecl Fn8F57E0() {
    return Thiscall(0x0049A650, 0x01B654A4u);  // NameKey_Destruct
}

// 0x8F57F0 (exit)
uint32_t __cdecl Fn8F57F0() {
    return Thiscall(0x0049A650, 0x01B654A8u);  // NameKey_Destruct
}

// 0x8F5800 (exit)
uint32_t __cdecl Fn8F5800() {
    return Thiscall(0x0049A650, 0x01B654ACu);  // NameKey_Destruct
}

// 0x8F5810 (exit)
uint32_t __cdecl Fn8F5810() {
    return Thiscall(0x0049A650, 0x01B654B0u);  // NameKey_Destruct
}

// 0x8F5820 (exit)
uint32_t __cdecl Fn8F5820() {
    return Thiscall(0x0049A650, 0x01B654B4u);  // NameKey_Destruct
}

// 0x8F5830 (exit)
uint32_t __cdecl Fn8F5830() {
    return Thiscall(0x0049A650, 0x01B654B8u);  // NameKey_Destruct
}

// 0x8F5840 (exit)
uint32_t __cdecl Fn8F5840() {
    return Thiscall(0x0049A650, 0x01B654BCu);  // NameKey_Destruct
}

// 0x8F5850 (exit)
uint32_t __cdecl Fn8F5850() {
    return Thiscall(0x0049A650, 0x01B654C0u);  // NameKey_Destruct
}

// 0x8F5860 (exit)
uint32_t __cdecl Fn8F5860() {
    return Thiscall(0x0049A650, 0x01B654C4u);  // NameKey_Destruct
}

// 0x8F5870 (exit)
uint32_t __cdecl Fn8F5870() {
    return Thiscall(0x0049A650, 0x01B654C8u);  // NameKey_Destruct
}

// 0x8F5880 (exit)
uint32_t __cdecl Fn8F5880() {
    return Thiscall(0x0049A650, 0x01B654CCu);  // NameKey_Destruct
}

// 0x8F5890 (exit)
uint32_t __cdecl Fn8F5890() {
    return Thiscall(0x0049A650, 0x01B654D0u);  // NameKey_Destruct
}

// 0x8F58A0 (exit)
uint32_t __cdecl Fn8F58A0() {
    return Thiscall(0x0049A650, 0x01B654D4u);  // NameKey_Destruct
}

// 0x8F58B0 (exit)
uint32_t __cdecl Fn8F58B0() {
    return Thiscall(0x0049A650, 0x01B654D8u);  // NameKey_Destruct
}

// 0x8F58C0 (exit)
uint32_t __cdecl Fn8F58C0() {
    return Thiscall(0x0049A650, 0x01B654DCu);  // NameKey_Destruct
}

} // namespace
} // namespace staticinit

#define STATICINIT5_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT5_REVERSED("StaticExit::8F29F0", 0x008F29F0, &staticinit::Fn8F29F0);
STATICINIT5_REVERSED("StaticExit::8F2A00", 0x008F2A00, &staticinit::Fn8F2A00);
STATICINIT5_REVERSED("StaticExit::8F2A10", 0x008F2A10, &staticinit::Fn8F2A10);
STATICINIT5_REVERSED("StaticExit::8F2A20", 0x008F2A20, &staticinit::Fn8F2A20);
STATICINIT5_REVERSED("StaticExit::8F2A30", 0x008F2A30, &staticinit::Fn8F2A30);
STATICINIT5_REVERSED("StaticExit::8F2A40", 0x008F2A40, &staticinit::Fn8F2A40);
STATICINIT5_REVERSED("StaticExit::8F2A50", 0x008F2A50, &staticinit::Fn8F2A50);
STATICINIT5_REVERSED("StaticExit::8F2A60", 0x008F2A60, &staticinit::Fn8F2A60);
STATICINIT5_REVERSED("StaticExit::8F2A70", 0x008F2A70, &staticinit::Fn8F2A70);
STATICINIT5_REVERSED("StaticExit::8F2A80", 0x008F2A80, &staticinit::Fn8F2A80);
STATICINIT5_REVERSED("StaticExit::8F2A90", 0x008F2A90, &staticinit::Fn8F2A90);
STATICINIT5_REVERSED("StaticExit::8F2AA0", 0x008F2AA0, &staticinit::Fn8F2AA0);
STATICINIT5_REVERSED("StaticExit::8F2AD0", 0x008F2AD0, &staticinit::Fn8F2AD0);
STATICINIT5_REVERSED("StaticExit::8F2B00", 0x008F2B00, &staticinit::Fn8F2B00);
STATICINIT5_REVERSED("StaticExit::8F2B30", 0x008F2B30, &staticinit::Fn8F2B30);
STATICINIT5_REVERSED("StaticExit::8F2B60", 0x008F2B60, &staticinit::Fn8F2B60);
STATICINIT5_REVERSED("StaticExit::8F2B90", 0x008F2B90, &staticinit::Fn8F2B90);
STATICINIT5_REVERSED("StaticExit::8F2BC0", 0x008F2BC0, &staticinit::Fn8F2BC0);
STATICINIT5_REVERSED("StaticExit::8F2BF0", 0x008F2BF0, &staticinit::Fn8F2BF0);
STATICINIT5_REVERSED("StaticExit::8F2C20", 0x008F2C20, &staticinit::Fn8F2C20);
STATICINIT5_REVERSED("StaticExit::8F2C50", 0x008F2C50, &staticinit::Fn8F2C50);
STATICINIT5_REVERSED("StaticExit::8F2C60", 0x008F2C60, &staticinit::Fn8F2C60);
STATICINIT5_REVERSED("StaticExit::8F2C70", 0x008F2C70, &staticinit::Fn8F2C70);
STATICINIT5_REVERSED("StaticExit::8F2C80", 0x008F2C80, &staticinit::Fn8F2C80);
STATICINIT5_REVERSED("StaticExit::8F2C90", 0x008F2C90, &staticinit::Fn8F2C90);
STATICINIT5_REVERSED("StaticExit::8F2CD0", 0x008F2CD0, &staticinit::Fn8F2CD0);
STATICINIT5_REVERSED("StaticExit::8F2D00", 0x008F2D00, &staticinit::Fn8F2D00);
STATICINIT5_REVERSED("StaticExit::8F2D10", 0x008F2D10, &staticinit::Fn8F2D10);
STATICINIT5_REVERSED("StaticExit::8F2D20", 0x008F2D20, &staticinit::Fn8F2D20);
STATICINIT5_REVERSED("StaticExit::8F2D30", 0x008F2D30, &staticinit::Fn8F2D30);
STATICINIT5_REVERSED("StaticExit::8F2D40", 0x008F2D40, &staticinit::Fn8F2D40);
STATICINIT5_REVERSED("StaticExit::8F2D70", 0x008F2D70, &staticinit::Fn8F2D70);
STATICINIT5_REVERSED("StaticExit::8F2DA0", 0x008F2DA0, &staticinit::Fn8F2DA0);
STATICINIT5_REVERSED("StaticExit::8F2DB0", 0x008F2DB0, &staticinit::Fn8F2DB0);
STATICINIT5_REVERSED("StaticExit::8F2DC0", 0x008F2DC0, &staticinit::Fn8F2DC0);
STATICINIT5_REVERSED("StaticExit::8F2E30", 0x008F2E30, &staticinit::Fn8F2E30);
STATICINIT5_REVERSED("StaticExit::8F2E50", 0x008F2E50, &staticinit::Fn8F2E50);
STATICINIT5_REVERSED("StaticExit::8F2E60", 0x008F2E60, &staticinit::Fn8F2E60);
STATICINIT5_REVERSED("StaticExit::8F2E70", 0x008F2E70, &staticinit::Fn8F2E70);
STATICINIT5_REVERSED("StaticExit::8F2EB0", 0x008F2EB0, &staticinit::Fn8F2EB0);
STATICINIT5_REVERSED("StaticExit::8F2EC0", 0x008F2EC0, &staticinit::Fn8F2EC0);
STATICINIT5_REVERSED("StaticExit::8F2EE0", 0x008F2EE0, &staticinit::Fn8F2EE0);
STATICINIT5_REVERSED("StaticExit::8F2F00", 0x008F2F00, &staticinit::Fn8F2F00);
STATICINIT5_REVERSED("StaticExit::8F2F20", 0x008F2F20, &staticinit::Fn8F2F20);
STATICINIT5_REVERSED("StaticExit::8F2F40", 0x008F2F40, &staticinit::Fn8F2F40);
STATICINIT5_REVERSED("StaticExit::8F2FD0", 0x008F2FD0, &staticinit::Fn8F2FD0);
STATICINIT5_REVERSED("StaticExit::8F2FF0", 0x008F2FF0, &staticinit::Fn8F2FF0);
STATICINIT5_REVERSED("StaticExit::8F3000", 0x008F3000, &staticinit::Fn8F3000);
STATICINIT5_REVERSED("StaticExit::8F3010", 0x008F3010, &staticinit::Fn8F3010);
STATICINIT5_REVERSED("StaticExit::8F3020", 0x008F3020, &staticinit::Fn8F3020);
STATICINIT5_REVERSED("StaticExit::8F3030", 0x008F3030, &staticinit::Fn8F3030);
STATICINIT5_REVERSED("StaticExit::8F3040", 0x008F3040, &staticinit::Fn8F3040);
STATICINIT5_REVERSED("StaticExit::8F3050", 0x008F3050, &staticinit::Fn8F3050);
STATICINIT5_REVERSED("StaticExit::8F3060", 0x008F3060, &staticinit::Fn8F3060);
STATICINIT5_REVERSED("StaticExit::8F3070", 0x008F3070, &staticinit::Fn8F3070);
STATICINIT5_REVERSED("StaticExit::8F3080", 0x008F3080, &staticinit::Fn8F3080);
STATICINIT5_REVERSED("StaticExit::8F3090", 0x008F3090, &staticinit::Fn8F3090);
STATICINIT5_REVERSED("StaticExit::8F30A0", 0x008F30A0, &staticinit::Fn8F30A0);
STATICINIT5_REVERSED("StaticExit::8F30B0", 0x008F30B0, &staticinit::Fn8F30B0);
STATICINIT5_REVERSED("StaticExit::8F30C0", 0x008F30C0, &staticinit::Fn8F30C0);
STATICINIT5_REVERSED("StaticExit::8F30D0", 0x008F30D0, &staticinit::Fn8F30D0);
STATICINIT5_REVERSED("StaticExit::8F30E0", 0x008F30E0, &staticinit::Fn8F30E0);
STATICINIT5_REVERSED("StaticExit::8F30F0", 0x008F30F0, &staticinit::Fn8F30F0);
STATICINIT5_REVERSED("StaticExit::8F3100", 0x008F3100, &staticinit::Fn8F3100);
STATICINIT5_REVERSED("StaticExit::8F3110", 0x008F3110, &staticinit::Fn8F3110);
STATICINIT5_REVERSED("StaticExit::8F3120", 0x008F3120, &staticinit::Fn8F3120);
STATICINIT5_REVERSED("StaticExit::8F3130", 0x008F3130, &staticinit::Fn8F3130);
STATICINIT5_REVERSED("StaticExit::8F3140", 0x008F3140, &staticinit::Fn8F3140);
STATICINIT5_REVERSED("StaticExit::8F3150", 0x008F3150, &staticinit::Fn8F3150);
STATICINIT5_REVERSED("StaticExit::8F3160", 0x008F3160, &staticinit::Fn8F3160);
STATICINIT5_REVERSED("StaticExit::8F3170", 0x008F3170, &staticinit::Fn8F3170);
STATICINIT5_REVERSED("StaticExit::8F3180", 0x008F3180, &staticinit::Fn8F3180);
STATICINIT5_REVERSED("StaticExit::8F3190", 0x008F3190, &staticinit::Fn8F3190);
STATICINIT5_REVERSED("StaticExit::8F31A0", 0x008F31A0, &staticinit::Fn8F31A0);
STATICINIT5_REVERSED("StaticExit::8F31B0", 0x008F31B0, &staticinit::Fn8F31B0);
STATICINIT5_REVERSED("StaticExit::8F31C0", 0x008F31C0, &staticinit::Fn8F31C0);
STATICINIT5_REVERSED("StaticExit::8F31D0", 0x008F31D0, &staticinit::Fn8F31D0);
STATICINIT5_REVERSED("StaticExit::8F31E0", 0x008F31E0, &staticinit::Fn8F31E0);
STATICINIT5_REVERSED("StaticExit::8F31F0", 0x008F31F0, &staticinit::Fn8F31F0);
STATICINIT5_REVERSED("StaticExit::8F3200", 0x008F3200, &staticinit::Fn8F3200);
STATICINIT5_REVERSED("StaticExit::8F3210", 0x008F3210, &staticinit::Fn8F3210);
STATICINIT5_REVERSED("StaticExit::8F3220", 0x008F3220, &staticinit::Fn8F3220);
STATICINIT5_REVERSED("StaticExit::8F3230", 0x008F3230, &staticinit::Fn8F3230);
STATICINIT5_REVERSED("StaticExit::8F3240", 0x008F3240, &staticinit::Fn8F3240);
STATICINIT5_REVERSED("StaticExit::8F3250", 0x008F3250, &staticinit::Fn8F3250);
STATICINIT5_REVERSED("StaticExit::8F3260", 0x008F3260, &staticinit::Fn8F3260);
STATICINIT5_REVERSED("StaticExit::8F3270", 0x008F3270, &staticinit::Fn8F3270);
STATICINIT5_REVERSED("StaticExit::8F3280", 0x008F3280, &staticinit::Fn8F3280);
STATICINIT5_REVERSED("StaticExit::8F3290", 0x008F3290, &staticinit::Fn8F3290);
STATICINIT5_REVERSED("StaticExit::8F32A0", 0x008F32A0, &staticinit::Fn8F32A0);
STATICINIT5_REVERSED("StaticExit::8F32B0", 0x008F32B0, &staticinit::Fn8F32B0);
STATICINIT5_REVERSED("StaticExit::8F32C0", 0x008F32C0, &staticinit::Fn8F32C0);
STATICINIT5_REVERSED("StaticExit::8F32D0", 0x008F32D0, &staticinit::Fn8F32D0);
STATICINIT5_REVERSED("StaticExit::8F32E0", 0x008F32E0, &staticinit::Fn8F32E0);
STATICINIT5_REVERSED("StaticExit::8F32F0", 0x008F32F0, &staticinit::Fn8F32F0);
STATICINIT5_REVERSED("StaticExit::8F3300", 0x008F3300, &staticinit::Fn8F3300);
STATICINIT5_REVERSED("StaticExit::8F3310", 0x008F3310, &staticinit::Fn8F3310);
STATICINIT5_REVERSED("StaticExit::8F3320", 0x008F3320, &staticinit::Fn8F3320);
STATICINIT5_REVERSED("StaticExit::8F3330", 0x008F3330, &staticinit::Fn8F3330);
STATICINIT5_REVERSED("StaticExit::8F3340", 0x008F3340, &staticinit::Fn8F3340);
STATICINIT5_REVERSED("StaticExit::8F3350", 0x008F3350, &staticinit::Fn8F3350);
STATICINIT5_REVERSED("StaticExit::8F3360", 0x008F3360, &staticinit::Fn8F3360);
STATICINIT5_REVERSED("StaticExit::8F3370", 0x008F3370, &staticinit::Fn8F3370);
STATICINIT5_REVERSED("StaticExit::8F3380", 0x008F3380, &staticinit::Fn8F3380);
STATICINIT5_REVERSED("StaticExit::8F3390", 0x008F3390, &staticinit::Fn8F3390);
STATICINIT5_REVERSED("StaticExit::8F33A0", 0x008F33A0, &staticinit::Fn8F33A0);
STATICINIT5_REVERSED("StaticExit::8F33B0", 0x008F33B0, &staticinit::Fn8F33B0);
STATICINIT5_REVERSED("StaticExit::8F33C0", 0x008F33C0, &staticinit::Fn8F33C0);
STATICINIT5_REVERSED("StaticExit::8F33D0", 0x008F33D0, &staticinit::Fn8F33D0);
STATICINIT5_REVERSED("StaticExit::8F33E0", 0x008F33E0, &staticinit::Fn8F33E0);
STATICINIT5_REVERSED("StaticExit::8F33F0", 0x008F33F0, &staticinit::Fn8F33F0);
STATICINIT5_REVERSED("StaticExit::8F3400", 0x008F3400, &staticinit::Fn8F3400);
STATICINIT5_REVERSED("StaticExit::8F3410", 0x008F3410, &staticinit::Fn8F3410);
STATICINIT5_REVERSED("StaticExit::8F3420", 0x008F3420, &staticinit::Fn8F3420);
STATICINIT5_REVERSED("StaticExit::8F3430", 0x008F3430, &staticinit::Fn8F3430);
STATICINIT5_REVERSED("StaticExit::8F3440", 0x008F3440, &staticinit::Fn8F3440);
STATICINIT5_REVERSED("StaticExit::8F3450", 0x008F3450, &staticinit::Fn8F3450);
STATICINIT5_REVERSED("StaticExit::8F3460", 0x008F3460, &staticinit::Fn8F3460);
STATICINIT5_REVERSED("StaticExit::8F3470", 0x008F3470, &staticinit::Fn8F3470);
STATICINIT5_REVERSED("StaticExit::8F3480", 0x008F3480, &staticinit::Fn8F3480);
STATICINIT5_REVERSED("StaticExit::8F3490", 0x008F3490, &staticinit::Fn8F3490);
STATICINIT5_REVERSED("StaticExit::8F34A0", 0x008F34A0, &staticinit::Fn8F34A0);
STATICINIT5_REVERSED("StaticExit::8F34B0", 0x008F34B0, &staticinit::Fn8F34B0);
STATICINIT5_REVERSED("StaticExit::8F34C0", 0x008F34C0, &staticinit::Fn8F34C0);
STATICINIT5_REVERSED("StaticExit::8F34D0", 0x008F34D0, &staticinit::Fn8F34D0);
STATICINIT5_REVERSED("StaticExit::8F34E0", 0x008F34E0, &staticinit::Fn8F34E0);
STATICINIT5_REVERSED("StaticExit::8F34F0", 0x008F34F0, &staticinit::Fn8F34F0);
STATICINIT5_REVERSED("StaticExit::8F3500", 0x008F3500, &staticinit::Fn8F3500);
STATICINIT5_REVERSED("StaticExit::8F3510", 0x008F3510, &staticinit::Fn8F3510);
STATICINIT5_REVERSED("StaticExit::8F3520", 0x008F3520, &staticinit::Fn8F3520);
STATICINIT5_REVERSED("StaticExit::8F3530", 0x008F3530, &staticinit::Fn8F3530);
STATICINIT5_REVERSED("StaticExit::8F3540", 0x008F3540, &staticinit::Fn8F3540);
STATICINIT5_REVERSED("StaticExit::8F3550", 0x008F3550, &staticinit::Fn8F3550);
STATICINIT5_REVERSED("StaticExit::8F3560", 0x008F3560, &staticinit::Fn8F3560);
STATICINIT5_REVERSED("StaticExit::8F3570", 0x008F3570, &staticinit::Fn8F3570);
STATICINIT5_REVERSED("StaticExit::8F3580", 0x008F3580, &staticinit::Fn8F3580);
STATICINIT5_REVERSED("StaticExit::8F3590", 0x008F3590, &staticinit::Fn8F3590);
STATICINIT5_REVERSED("StaticExit::8F35A0", 0x008F35A0, &staticinit::Fn8F35A0);
STATICINIT5_REVERSED("StaticExit::8F35B0", 0x008F35B0, &staticinit::Fn8F35B0);
STATICINIT5_REVERSED("StaticExit::8F35C0", 0x008F35C0, &staticinit::Fn8F35C0);
STATICINIT5_REVERSED("StaticExit::8F35D0", 0x008F35D0, &staticinit::Fn8F35D0);
STATICINIT5_REVERSED("StaticExit::8F35E0", 0x008F35E0, &staticinit::Fn8F35E0);
STATICINIT5_REVERSED("StaticExit::8F35F0", 0x008F35F0, &staticinit::Fn8F35F0);
STATICINIT5_REVERSED("StaticExit::8F3600", 0x008F3600, &staticinit::Fn8F3600);
STATICINIT5_REVERSED("StaticExit::8F3610", 0x008F3610, &staticinit::Fn8F3610);
STATICINIT5_REVERSED("StaticExit::8F3620", 0x008F3620, &staticinit::Fn8F3620);
STATICINIT5_REVERSED("StaticExit::8F3630", 0x008F3630, &staticinit::Fn8F3630);
STATICINIT5_REVERSED("StaticExit::8F3640", 0x008F3640, &staticinit::Fn8F3640);
STATICINIT5_REVERSED("StaticExit::8F3650", 0x008F3650, &staticinit::Fn8F3650);
STATICINIT5_REVERSED("StaticExit::8F3660", 0x008F3660, &staticinit::Fn8F3660);
STATICINIT5_REVERSED("StaticExit::8F3670", 0x008F3670, &staticinit::Fn8F3670);
STATICINIT5_REVERSED("StaticExit::8F3680", 0x008F3680, &staticinit::Fn8F3680);
STATICINIT5_REVERSED("StaticExit::8F3690", 0x008F3690, &staticinit::Fn8F3690);
STATICINIT5_REVERSED("StaticExit::8F36A0", 0x008F36A0, &staticinit::Fn8F36A0);
STATICINIT5_REVERSED("StaticExit::8F36B0", 0x008F36B0, &staticinit::Fn8F36B0);
STATICINIT5_REVERSED("StaticExit::8F36C0", 0x008F36C0, &staticinit::Fn8F36C0);
STATICINIT5_REVERSED("StaticExit::8F36D0", 0x008F36D0, &staticinit::Fn8F36D0);
STATICINIT5_REVERSED("StaticExit::8F36E0", 0x008F36E0, &staticinit::Fn8F36E0);
STATICINIT5_REVERSED("StaticExit::8F36F0", 0x008F36F0, &staticinit::Fn8F36F0);
STATICINIT5_REVERSED("StaticExit::8F3700", 0x008F3700, &staticinit::Fn8F3700);
STATICINIT5_REVERSED("StaticExit::8F3710", 0x008F3710, &staticinit::Fn8F3710);
STATICINIT5_REVERSED("StaticExit::8F3720", 0x008F3720, &staticinit::Fn8F3720);
STATICINIT5_REVERSED("StaticExit::8F3730", 0x008F3730, &staticinit::Fn8F3730);
STATICINIT5_REVERSED("StaticExit::8F3740", 0x008F3740, &staticinit::Fn8F3740);
STATICINIT5_REVERSED("StaticExit::8F3750", 0x008F3750, &staticinit::Fn8F3750);
STATICINIT5_REVERSED("StaticExit::8F3760", 0x008F3760, &staticinit::Fn8F3760);
STATICINIT5_REVERSED("StaticExit::8F3770", 0x008F3770, &staticinit::Fn8F3770);
STATICINIT5_REVERSED("StaticExit::8F3780", 0x008F3780, &staticinit::Fn8F3780);
STATICINIT5_REVERSED("StaticExit::8F3790", 0x008F3790, &staticinit::Fn8F3790);
STATICINIT5_REVERSED("StaticExit::8F37A0", 0x008F37A0, &staticinit::Fn8F37A0);
STATICINIT5_REVERSED("StaticExit::8F37B0", 0x008F37B0, &staticinit::Fn8F37B0);
STATICINIT5_REVERSED("StaticExit::8F37C0", 0x008F37C0, &staticinit::Fn8F37C0);
STATICINIT5_REVERSED("StaticExit::8F37D0", 0x008F37D0, &staticinit::Fn8F37D0);
STATICINIT5_REVERSED("StaticExit::8F37E0", 0x008F37E0, &staticinit::Fn8F37E0);
STATICINIT5_REVERSED("StaticExit::8F37F0", 0x008F37F0, &staticinit::Fn8F37F0);
STATICINIT5_REVERSED("StaticExit::8F3800", 0x008F3800, &staticinit::Fn8F3800);
STATICINIT5_REVERSED("StaticExit::8F3810", 0x008F3810, &staticinit::Fn8F3810);
STATICINIT5_REVERSED("StaticExit::8F3820", 0x008F3820, &staticinit::Fn8F3820);
STATICINIT5_REVERSED("StaticExit::8F3830", 0x008F3830, &staticinit::Fn8F3830);
STATICINIT5_REVERSED("StaticExit::8F3840", 0x008F3840, &staticinit::Fn8F3840);
STATICINIT5_REVERSED("StaticExit::8F3850", 0x008F3850, &staticinit::Fn8F3850);
STATICINIT5_REVERSED("StaticExit::8F3860", 0x008F3860, &staticinit::Fn8F3860);
STATICINIT5_REVERSED("StaticExit::8F3870", 0x008F3870, &staticinit::Fn8F3870);
STATICINIT5_REVERSED("StaticExit::8F3880", 0x008F3880, &staticinit::Fn8F3880);
STATICINIT5_REVERSED("StaticExit::8F3890", 0x008F3890, &staticinit::Fn8F3890);
STATICINIT5_REVERSED("StaticExit::8F38A0", 0x008F38A0, &staticinit::Fn8F38A0);
STATICINIT5_REVERSED("StaticExit::8F38B0", 0x008F38B0, &staticinit::Fn8F38B0);
STATICINIT5_REVERSED("StaticExit::8F38C0", 0x008F38C0, &staticinit::Fn8F38C0);
STATICINIT5_REVERSED("StaticExit::8F38D0", 0x008F38D0, &staticinit::Fn8F38D0);
STATICINIT5_REVERSED("StaticExit::8F38E0", 0x008F38E0, &staticinit::Fn8F38E0);
STATICINIT5_REVERSED("StaticExit::8F38F0", 0x008F38F0, &staticinit::Fn8F38F0);
STATICINIT5_REVERSED("StaticExit::8F3900", 0x008F3900, &staticinit::Fn8F3900);
STATICINIT5_REVERSED("StaticExit::8F3910", 0x008F3910, &staticinit::Fn8F3910);
STATICINIT5_REVERSED("StaticExit::8F3920", 0x008F3920, &staticinit::Fn8F3920);
STATICINIT5_REVERSED("StaticExit::8F3930", 0x008F3930, &staticinit::Fn8F3930);
STATICINIT5_REVERSED("StaticExit::8F3940", 0x008F3940, &staticinit::Fn8F3940);
STATICINIT5_REVERSED("StaticExit::8F3950", 0x008F3950, &staticinit::Fn8F3950);
STATICINIT5_REVERSED("StaticExit::8F3960", 0x008F3960, &staticinit::Fn8F3960);
STATICINIT5_REVERSED("StaticExit::8F3970", 0x008F3970, &staticinit::Fn8F3970);
STATICINIT5_REVERSED("StaticExit::8F3980", 0x008F3980, &staticinit::Fn8F3980);
STATICINIT5_REVERSED("StaticExit::8F3990", 0x008F3990, &staticinit::Fn8F3990);
STATICINIT5_REVERSED("StaticExit::8F39A0", 0x008F39A0, &staticinit::Fn8F39A0);
STATICINIT5_REVERSED("StaticExit::8F39B0", 0x008F39B0, &staticinit::Fn8F39B0);
STATICINIT5_REVERSED("StaticExit::8F39C0", 0x008F39C0, &staticinit::Fn8F39C0);
STATICINIT5_REVERSED("StaticExit::8F39D0", 0x008F39D0, &staticinit::Fn8F39D0);
STATICINIT5_REVERSED("StaticExit::8F39E0", 0x008F39E0, &staticinit::Fn8F39E0);
STATICINIT5_REVERSED("StaticExit::8F39F0", 0x008F39F0, &staticinit::Fn8F39F0);
STATICINIT5_REVERSED("StaticExit::8F3A00", 0x008F3A00, &staticinit::Fn8F3A00);
STATICINIT5_REVERSED("StaticExit::8F3A10", 0x008F3A10, &staticinit::Fn8F3A10);
STATICINIT5_REVERSED("StaticExit::8F3A20", 0x008F3A20, &staticinit::Fn8F3A20);
STATICINIT5_REVERSED("StaticExit::8F3A30", 0x008F3A30, &staticinit::Fn8F3A30);
STATICINIT5_REVERSED("StaticExit::8F3A40", 0x008F3A40, &staticinit::Fn8F3A40);
STATICINIT5_REVERSED("StaticExit::8F3A50", 0x008F3A50, &staticinit::Fn8F3A50);
STATICINIT5_REVERSED("StaticExit::8F3A60", 0x008F3A60, &staticinit::Fn8F3A60);
STATICINIT5_REVERSED("StaticExit::8F3A70", 0x008F3A70, &staticinit::Fn8F3A70);
STATICINIT5_REVERSED("StaticExit::8F3A80", 0x008F3A80, &staticinit::Fn8F3A80);
STATICINIT5_REVERSED("StaticExit::8F3A90", 0x008F3A90, &staticinit::Fn8F3A90);
STATICINIT5_REVERSED("StaticExit::8F3AA0", 0x008F3AA0, &staticinit::Fn8F3AA0);
STATICINIT5_REVERSED("StaticExit::8F3AB0", 0x008F3AB0, &staticinit::Fn8F3AB0);
STATICINIT5_REVERSED("StaticExit::8F3AC0", 0x008F3AC0, &staticinit::Fn8F3AC0);
STATICINIT5_REVERSED("StaticExit::8F3AD0", 0x008F3AD0, &staticinit::Fn8F3AD0);
STATICINIT5_REVERSED("StaticExit::8F3AE0", 0x008F3AE0, &staticinit::Fn8F3AE0);
STATICINIT5_REVERSED("StaticExit::8F3AF0", 0x008F3AF0, &staticinit::Fn8F3AF0);
STATICINIT5_REVERSED("StaticExit::8F3B00", 0x008F3B00, &staticinit::Fn8F3B00);
STATICINIT5_REVERSED("StaticExit::8F3B10", 0x008F3B10, &staticinit::Fn8F3B10);
STATICINIT5_REVERSED("StaticExit::8F3B20", 0x008F3B20, &staticinit::Fn8F3B20);
STATICINIT5_REVERSED("StaticExit::8F3B30", 0x008F3B30, &staticinit::Fn8F3B30);
STATICINIT5_REVERSED("StaticExit::8F3B40", 0x008F3B40, &staticinit::Fn8F3B40);
STATICINIT5_REVERSED("StaticExit::8F3B50", 0x008F3B50, &staticinit::Fn8F3B50);
STATICINIT5_REVERSED("StaticExit::8F3B60", 0x008F3B60, &staticinit::Fn8F3B60);
STATICINIT5_REVERSED("StaticExit::8F3B70", 0x008F3B70, &staticinit::Fn8F3B70);
STATICINIT5_REVERSED("StaticExit::8F3B80", 0x008F3B80, &staticinit::Fn8F3B80);
STATICINIT5_REVERSED("StaticExit::8F3B90", 0x008F3B90, &staticinit::Fn8F3B90);
STATICINIT5_REVERSED("StaticExit::8F3BA0", 0x008F3BA0, &staticinit::Fn8F3BA0);
STATICINIT5_REVERSED("StaticExit::8F3BB0", 0x008F3BB0, &staticinit::Fn8F3BB0);
STATICINIT5_REVERSED("StaticExit::8F3BC0", 0x008F3BC0, &staticinit::Fn8F3BC0);
STATICINIT5_REVERSED("StaticExit::8F3BD0", 0x008F3BD0, &staticinit::Fn8F3BD0);
STATICINIT5_REVERSED("StaticExit::8F3BE0", 0x008F3BE0, &staticinit::Fn8F3BE0);
STATICINIT5_REVERSED("StaticExit::8F3BF0", 0x008F3BF0, &staticinit::Fn8F3BF0);
STATICINIT5_REVERSED("StaticExit::8F3C00", 0x008F3C00, &staticinit::Fn8F3C00);
STATICINIT5_REVERSED("StaticExit::8F3C10", 0x008F3C10, &staticinit::Fn8F3C10);
STATICINIT5_REVERSED("StaticExit::8F3C20", 0x008F3C20, &staticinit::Fn8F3C20);
STATICINIT5_REVERSED("StaticExit::8F3C30", 0x008F3C30, &staticinit::Fn8F3C30);
STATICINIT5_REVERSED("StaticExit::8F3C40", 0x008F3C40, &staticinit::Fn8F3C40);
STATICINIT5_REVERSED("StaticExit::8F3C50", 0x008F3C50, &staticinit::Fn8F3C50);
STATICINIT5_REVERSED("StaticExit::8F3C60", 0x008F3C60, &staticinit::Fn8F3C60);
STATICINIT5_REVERSED("StaticExit::8F3C70", 0x008F3C70, &staticinit::Fn8F3C70);
STATICINIT5_REVERSED("StaticExit::8F3C80", 0x008F3C80, &staticinit::Fn8F3C80);
STATICINIT5_REVERSED("StaticExit::8F3C90", 0x008F3C90, &staticinit::Fn8F3C90);
STATICINIT5_REVERSED("StaticExit::8F3CA0", 0x008F3CA0, &staticinit::Fn8F3CA0);
STATICINIT5_REVERSED("StaticExit::8F3CB0", 0x008F3CB0, &staticinit::Fn8F3CB0);
STATICINIT5_REVERSED("StaticExit::8F3CC0", 0x008F3CC0, &staticinit::Fn8F3CC0);
STATICINIT5_REVERSED("StaticExit::8F3CD0", 0x008F3CD0, &staticinit::Fn8F3CD0);
STATICINIT5_REVERSED("StaticExit::8F3CE0", 0x008F3CE0, &staticinit::Fn8F3CE0);
STATICINIT5_REVERSED("StaticExit::8F3CF0", 0x008F3CF0, &staticinit::Fn8F3CF0);
STATICINIT5_REVERSED("StaticExit::8F3D00", 0x008F3D00, &staticinit::Fn8F3D00);
STATICINIT5_REVERSED("StaticExit::8F3D10", 0x008F3D10, &staticinit::Fn8F3D10);
STATICINIT5_REVERSED("StaticExit::8F3D20", 0x008F3D20, &staticinit::Fn8F3D20);
STATICINIT5_REVERSED("StaticExit::8F3D30", 0x008F3D30, &staticinit::Fn8F3D30);
STATICINIT5_REVERSED("StaticExit::8F3D40", 0x008F3D40, &staticinit::Fn8F3D40);
STATICINIT5_REVERSED("StaticExit::8F3D50", 0x008F3D50, &staticinit::Fn8F3D50);
STATICINIT5_REVERSED("StaticExit::8F3D60", 0x008F3D60, &staticinit::Fn8F3D60);
STATICINIT5_REVERSED("StaticExit::8F3D70", 0x008F3D70, &staticinit::Fn8F3D70);
STATICINIT5_REVERSED("StaticExit::8F3D80", 0x008F3D80, &staticinit::Fn8F3D80);
STATICINIT5_REVERSED("StaticExit::8F3D90", 0x008F3D90, &staticinit::Fn8F3D90);
STATICINIT5_REVERSED("StaticExit::8F3DA0", 0x008F3DA0, &staticinit::Fn8F3DA0);
STATICINIT5_REVERSED("StaticExit::8F3DB0", 0x008F3DB0, &staticinit::Fn8F3DB0);
STATICINIT5_REVERSED("StaticExit::8F3DC0", 0x008F3DC0, &staticinit::Fn8F3DC0);
STATICINIT5_REVERSED("StaticExit::8F3DD0", 0x008F3DD0, &staticinit::Fn8F3DD0);
STATICINIT5_REVERSED("StaticExit::8F3DE0", 0x008F3DE0, &staticinit::Fn8F3DE0);
STATICINIT5_REVERSED("StaticExit::8F3DF0", 0x008F3DF0, &staticinit::Fn8F3DF0);
STATICINIT5_REVERSED("StaticExit::8F3E00", 0x008F3E00, &staticinit::Fn8F3E00);
STATICINIT5_REVERSED("StaticExit::8F3E10", 0x008F3E10, &staticinit::Fn8F3E10);
STATICINIT5_REVERSED("StaticExit::8F3E20", 0x008F3E20, &staticinit::Fn8F3E20);
STATICINIT5_REVERSED("StaticExit::8F3E30", 0x008F3E30, &staticinit::Fn8F3E30);
STATICINIT5_REVERSED("StaticExit::8F3E40", 0x008F3E40, &staticinit::Fn8F3E40);
STATICINIT5_REVERSED("StaticExit::8F3E50", 0x008F3E50, &staticinit::Fn8F3E50);
STATICINIT5_REVERSED("StaticExit::8F3E60", 0x008F3E60, &staticinit::Fn8F3E60);
STATICINIT5_REVERSED("StaticExit::8F3E70", 0x008F3E70, &staticinit::Fn8F3E70);
STATICINIT5_REVERSED("StaticExit::8F3E80", 0x008F3E80, &staticinit::Fn8F3E80);
STATICINIT5_REVERSED("StaticExit::8F3E90", 0x008F3E90, &staticinit::Fn8F3E90);
STATICINIT5_REVERSED("StaticExit::8F3EA0", 0x008F3EA0, &staticinit::Fn8F3EA0);
STATICINIT5_REVERSED("StaticExit::8F3EB0", 0x008F3EB0, &staticinit::Fn8F3EB0);
STATICINIT5_REVERSED("StaticExit::8F3EC0", 0x008F3EC0, &staticinit::Fn8F3EC0);
STATICINIT5_REVERSED("StaticExit::8F3ED0", 0x008F3ED0, &staticinit::Fn8F3ED0);
STATICINIT5_REVERSED("StaticExit::8F3EE0", 0x008F3EE0, &staticinit::Fn8F3EE0);
STATICINIT5_REVERSED("StaticExit::8F3EF0", 0x008F3EF0, &staticinit::Fn8F3EF0);
STATICINIT5_REVERSED("StaticExit::8F3F00", 0x008F3F00, &staticinit::Fn8F3F00);
STATICINIT5_REVERSED("StaticExit::8F3F10", 0x008F3F10, &staticinit::Fn8F3F10);
STATICINIT5_REVERSED("StaticExit::8F3F20", 0x008F3F20, &staticinit::Fn8F3F20);
STATICINIT5_REVERSED("StaticExit::8F3F30", 0x008F3F30, &staticinit::Fn8F3F30);
STATICINIT5_REVERSED("StaticExit::8F3F40", 0x008F3F40, &staticinit::Fn8F3F40);
STATICINIT5_REVERSED("StaticExit::8F3F50", 0x008F3F50, &staticinit::Fn8F3F50);
STATICINIT5_REVERSED("StaticExit::8F3F60", 0x008F3F60, &staticinit::Fn8F3F60);
STATICINIT5_REVERSED("StaticExit::8F3F70", 0x008F3F70, &staticinit::Fn8F3F70);
STATICINIT5_REVERSED("StaticExit::8F3F80", 0x008F3F80, &staticinit::Fn8F3F80);
STATICINIT5_REVERSED("StaticExit::8F3F90", 0x008F3F90, &staticinit::Fn8F3F90);
STATICINIT5_REVERSED("StaticExit::8F3FA0", 0x008F3FA0, &staticinit::Fn8F3FA0);
STATICINIT5_REVERSED("StaticExit::8F3FB0", 0x008F3FB0, &staticinit::Fn8F3FB0);
STATICINIT5_REVERSED("StaticExit::8F3FC0", 0x008F3FC0, &staticinit::Fn8F3FC0);
STATICINIT5_REVERSED("StaticExit::8F3FD0", 0x008F3FD0, &staticinit::Fn8F3FD0);
STATICINIT5_REVERSED("StaticExit::8F3FE0", 0x008F3FE0, &staticinit::Fn8F3FE0);
STATICINIT5_REVERSED("StaticExit::8F3FF0", 0x008F3FF0, &staticinit::Fn8F3FF0);
STATICINIT5_REVERSED("StaticExit::8F4000", 0x008F4000, &staticinit::Fn8F4000);
STATICINIT5_REVERSED("StaticExit::8F4010", 0x008F4010, &staticinit::Fn8F4010);
STATICINIT5_REVERSED("StaticExit::8F4020", 0x008F4020, &staticinit::Fn8F4020);
STATICINIT5_REVERSED("StaticExit::8F4030", 0x008F4030, &staticinit::Fn8F4030);
STATICINIT5_REVERSED("StaticExit::8F4040", 0x008F4040, &staticinit::Fn8F4040);
STATICINIT5_REVERSED("StaticExit::8F4050", 0x008F4050, &staticinit::Fn8F4050);
STATICINIT5_REVERSED("StaticExit::8F4060", 0x008F4060, &staticinit::Fn8F4060);
STATICINIT5_REVERSED("StaticExit::8F4070", 0x008F4070, &staticinit::Fn8F4070);
STATICINIT5_REVERSED("StaticExit::8F4080", 0x008F4080, &staticinit::Fn8F4080);
STATICINIT5_REVERSED("StaticExit::8F4090", 0x008F4090, &staticinit::Fn8F4090);
STATICINIT5_REVERSED("StaticExit::8F40A0", 0x008F40A0, &staticinit::Fn8F40A0);
STATICINIT5_REVERSED("StaticExit::8F40B0", 0x008F40B0, &staticinit::Fn8F40B0);
STATICINIT5_REVERSED("StaticExit::8F40C0", 0x008F40C0, &staticinit::Fn8F40C0);
STATICINIT5_REVERSED("StaticExit::8F40D0", 0x008F40D0, &staticinit::Fn8F40D0);
STATICINIT5_REVERSED("StaticExit::8F40E0", 0x008F40E0, &staticinit::Fn8F40E0);
STATICINIT5_REVERSED("StaticExit::8F40F0", 0x008F40F0, &staticinit::Fn8F40F0);
STATICINIT5_REVERSED("StaticExit::8F4100", 0x008F4100, &staticinit::Fn8F4100);
STATICINIT5_REVERSED("StaticExit::8F4110", 0x008F4110, &staticinit::Fn8F4110);
STATICINIT5_REVERSED("StaticExit::8F4120", 0x008F4120, &staticinit::Fn8F4120);
STATICINIT5_REVERSED("StaticExit::8F4130", 0x008F4130, &staticinit::Fn8F4130);
STATICINIT5_REVERSED("StaticExit::8F4140", 0x008F4140, &staticinit::Fn8F4140);
STATICINIT5_REVERSED("StaticExit::8F4150", 0x008F4150, &staticinit::Fn8F4150);
STATICINIT5_REVERSED("StaticExit::8F4160", 0x008F4160, &staticinit::Fn8F4160);
STATICINIT5_REVERSED("StaticExit::8F4170", 0x008F4170, &staticinit::Fn8F4170);
STATICINIT5_REVERSED("StaticExit::8F4180", 0x008F4180, &staticinit::Fn8F4180);
STATICINIT5_REVERSED("StaticExit::8F4190", 0x008F4190, &staticinit::Fn8F4190);
STATICINIT5_REVERSED("StaticExit::8F41A0", 0x008F41A0, &staticinit::Fn8F41A0);
STATICINIT5_REVERSED("StaticExit::8F41B0", 0x008F41B0, &staticinit::Fn8F41B0);
STATICINIT5_REVERSED("StaticExit::8F41C0", 0x008F41C0, &staticinit::Fn8F41C0);
STATICINIT5_REVERSED("StaticExit::8F41D0", 0x008F41D0, &staticinit::Fn8F41D0);
STATICINIT5_REVERSED("StaticExit::8F41E0", 0x008F41E0, &staticinit::Fn8F41E0);
STATICINIT5_REVERSED("StaticExit::8F41F0", 0x008F41F0, &staticinit::Fn8F41F0);
STATICINIT5_REVERSED("StaticExit::8F4200", 0x008F4200, &staticinit::Fn8F4200);
STATICINIT5_REVERSED("StaticExit::8F4210", 0x008F4210, &staticinit::Fn8F4210);
STATICINIT5_REVERSED("StaticExit::8F4220", 0x008F4220, &staticinit::Fn8F4220);
STATICINIT5_REVERSED("StaticExit::8F4230", 0x008F4230, &staticinit::Fn8F4230);
STATICINIT5_REVERSED("StaticExit::8F4240", 0x008F4240, &staticinit::Fn8F4240);
STATICINIT5_REVERSED("StaticExit::8F4250", 0x008F4250, &staticinit::Fn8F4250);
STATICINIT5_REVERSED("StaticExit::8F4260", 0x008F4260, &staticinit::Fn8F4260);
STATICINIT5_REVERSED("StaticExit::8F4270", 0x008F4270, &staticinit::Fn8F4270);
STATICINIT5_REVERSED("StaticExit::8F4280", 0x008F4280, &staticinit::Fn8F4280);
STATICINIT5_REVERSED("StaticExit::8F4290", 0x008F4290, &staticinit::Fn8F4290);
STATICINIT5_REVERSED("StaticExit::8F42A0", 0x008F42A0, &staticinit::Fn8F42A0);
STATICINIT5_REVERSED("StaticExit::8F42B0", 0x008F42B0, &staticinit::Fn8F42B0);
STATICINIT5_REVERSED("StaticExit::8F42C0", 0x008F42C0, &staticinit::Fn8F42C0);
STATICINIT5_REVERSED("StaticExit::8F42D0", 0x008F42D0, &staticinit::Fn8F42D0);
STATICINIT5_REVERSED("StaticExit::8F42E0", 0x008F42E0, &staticinit::Fn8F42E0);
STATICINIT5_REVERSED("StaticExit::8F42F0", 0x008F42F0, &staticinit::Fn8F42F0);
STATICINIT5_REVERSED("StaticExit::8F4300", 0x008F4300, &staticinit::Fn8F4300);
STATICINIT5_REVERSED("StaticExit::8F4310", 0x008F4310, &staticinit::Fn8F4310);
STATICINIT5_REVERSED("StaticExit::8F4320", 0x008F4320, &staticinit::Fn8F4320);
STATICINIT5_REVERSED("StaticExit::8F4330", 0x008F4330, &staticinit::Fn8F4330);
STATICINIT5_REVERSED("StaticExit::8F4340", 0x008F4340, &staticinit::Fn8F4340);
STATICINIT5_REVERSED("StaticExit::8F4350", 0x008F4350, &staticinit::Fn8F4350);
STATICINIT5_REVERSED("StaticExit::8F4360", 0x008F4360, &staticinit::Fn8F4360);
STATICINIT5_REVERSED("StaticExit::8F4370", 0x008F4370, &staticinit::Fn8F4370);
STATICINIT5_REVERSED("StaticExit::8F4380", 0x008F4380, &staticinit::Fn8F4380);
STATICINIT5_REVERSED("StaticExit::8F4390", 0x008F4390, &staticinit::Fn8F4390);
STATICINIT5_REVERSED("StaticExit::8F43A0", 0x008F43A0, &staticinit::Fn8F43A0);
STATICINIT5_REVERSED("StaticExit::8F43B0", 0x008F43B0, &staticinit::Fn8F43B0);
STATICINIT5_REVERSED("StaticExit::8F43C0", 0x008F43C0, &staticinit::Fn8F43C0);
STATICINIT5_REVERSED("StaticExit::8F43D0", 0x008F43D0, &staticinit::Fn8F43D0);
STATICINIT5_REVERSED("StaticExit::8F43E0", 0x008F43E0, &staticinit::Fn8F43E0);
STATICINIT5_REVERSED("StaticExit::8F43F0", 0x008F43F0, &staticinit::Fn8F43F0);
STATICINIT5_REVERSED("StaticExit::8F4400", 0x008F4400, &staticinit::Fn8F4400);
STATICINIT5_REVERSED("StaticExit::8F4410", 0x008F4410, &staticinit::Fn8F4410);
STATICINIT5_REVERSED("StaticExit::8F4420", 0x008F4420, &staticinit::Fn8F4420);
STATICINIT5_REVERSED("StaticExit::8F4430", 0x008F4430, &staticinit::Fn8F4430);
STATICINIT5_REVERSED("StaticExit::8F4440", 0x008F4440, &staticinit::Fn8F4440);
STATICINIT5_REVERSED("StaticExit::8F4450", 0x008F4450, &staticinit::Fn8F4450);
STATICINIT5_REVERSED("StaticExit::8F4460", 0x008F4460, &staticinit::Fn8F4460);
STATICINIT5_REVERSED("StaticExit::8F4470", 0x008F4470, &staticinit::Fn8F4470);
STATICINIT5_REVERSED("StaticExit::8F4480", 0x008F4480, &staticinit::Fn8F4480);
STATICINIT5_REVERSED("StaticExit::8F4490", 0x008F4490, &staticinit::Fn8F4490);
STATICINIT5_REVERSED("StaticExit::8F44A0", 0x008F44A0, &staticinit::Fn8F44A0);
STATICINIT5_REVERSED("StaticExit::8F44B0", 0x008F44B0, &staticinit::Fn8F44B0);
STATICINIT5_REVERSED("StaticExit::8F44C0", 0x008F44C0, &staticinit::Fn8F44C0);
STATICINIT5_REVERSED("StaticExit::8F44D0", 0x008F44D0, &staticinit::Fn8F44D0);
STATICINIT5_REVERSED("StaticExit::8F44E0", 0x008F44E0, &staticinit::Fn8F44E0);
STATICINIT5_REVERSED("StaticExit::8F44F0", 0x008F44F0, &staticinit::Fn8F44F0);
STATICINIT5_REVERSED("StaticExit::8F4500", 0x008F4500, &staticinit::Fn8F4500);
STATICINIT5_REVERSED("StaticExit::8F4510", 0x008F4510, &staticinit::Fn8F4510);
STATICINIT5_REVERSED("StaticExit::8F4520", 0x008F4520, &staticinit::Fn8F4520);
STATICINIT5_REVERSED("StaticExit::8F4530", 0x008F4530, &staticinit::Fn8F4530);
STATICINIT5_REVERSED("StaticExit::8F4540", 0x008F4540, &staticinit::Fn8F4540);
STATICINIT5_REVERSED("StaticExit::8F4550", 0x008F4550, &staticinit::Fn8F4550);
STATICINIT5_REVERSED("StaticExit::8F4560", 0x008F4560, &staticinit::Fn8F4560);
STATICINIT5_REVERSED("StaticExit::8F4570", 0x008F4570, &staticinit::Fn8F4570);
STATICINIT5_REVERSED("StaticExit::8F4580", 0x008F4580, &staticinit::Fn8F4580);
STATICINIT5_REVERSED("StaticExit::8F4590", 0x008F4590, &staticinit::Fn8F4590);
STATICINIT5_REVERSED("StaticExit::8F45A0", 0x008F45A0, &staticinit::Fn8F45A0);
STATICINIT5_REVERSED("StaticExit::8F45B0", 0x008F45B0, &staticinit::Fn8F45B0);
STATICINIT5_REVERSED("StaticExit::8F45C0", 0x008F45C0, &staticinit::Fn8F45C0);
STATICINIT5_REVERSED("StaticExit::8F45D0", 0x008F45D0, &staticinit::Fn8F45D0);
STATICINIT5_REVERSED("StaticExit::8F45E0", 0x008F45E0, &staticinit::Fn8F45E0);
STATICINIT5_REVERSED("StaticExit::8F45F0", 0x008F45F0, &staticinit::Fn8F45F0);
STATICINIT5_REVERSED("StaticExit::8F4600", 0x008F4600, &staticinit::Fn8F4600);
STATICINIT5_REVERSED("StaticExit::8F4610", 0x008F4610, &staticinit::Fn8F4610);
STATICINIT5_REVERSED("StaticExit::8F4620", 0x008F4620, &staticinit::Fn8F4620);
STATICINIT5_REVERSED("StaticExit::8F4630", 0x008F4630, &staticinit::Fn8F4630);
STATICINIT5_REVERSED("StaticExit::8F4640", 0x008F4640, &staticinit::Fn8F4640);
STATICINIT5_REVERSED("StaticExit::8F4650", 0x008F4650, &staticinit::Fn8F4650);
STATICINIT5_REVERSED("StaticExit::8F4660", 0x008F4660, &staticinit::Fn8F4660);
STATICINIT5_REVERSED("StaticExit::8F4670", 0x008F4670, &staticinit::Fn8F4670);
STATICINIT5_REVERSED("StaticExit::8F4680", 0x008F4680, &staticinit::Fn8F4680);
STATICINIT5_REVERSED("StaticExit::8F4690", 0x008F4690, &staticinit::Fn8F4690);
STATICINIT5_REVERSED("StaticExit::8F46A0", 0x008F46A0, &staticinit::Fn8F46A0);
STATICINIT5_REVERSED("StaticExit::8F46B0", 0x008F46B0, &staticinit::Fn8F46B0);
STATICINIT5_REVERSED("StaticExit::8F46C0", 0x008F46C0, &staticinit::Fn8F46C0);
STATICINIT5_REVERSED("StaticExit::8F46D0", 0x008F46D0, &staticinit::Fn8F46D0);
STATICINIT5_REVERSED("StaticExit::8F46E0", 0x008F46E0, &staticinit::Fn8F46E0);
STATICINIT5_REVERSED("StaticExit::8F46F0", 0x008F46F0, &staticinit::Fn8F46F0);
STATICINIT5_REVERSED("StaticExit::8F4700", 0x008F4700, &staticinit::Fn8F4700);
STATICINIT5_REVERSED("StaticExit::8F4710", 0x008F4710, &staticinit::Fn8F4710);
STATICINIT5_REVERSED("StaticExit::8F4720", 0x008F4720, &staticinit::Fn8F4720);
STATICINIT5_REVERSED("StaticExit::8F4730", 0x008F4730, &staticinit::Fn8F4730);
STATICINIT5_REVERSED("StaticExit::8F4740", 0x008F4740, &staticinit::Fn8F4740);
STATICINIT5_REVERSED("StaticExit::8F4750", 0x008F4750, &staticinit::Fn8F4750);
STATICINIT5_REVERSED("StaticExit::8F4760", 0x008F4760, &staticinit::Fn8F4760);
STATICINIT5_REVERSED("StaticExit::8F4770", 0x008F4770, &staticinit::Fn8F4770);
STATICINIT5_REVERSED("StaticExit::8F4780", 0x008F4780, &staticinit::Fn8F4780);
STATICINIT5_REVERSED("StaticExit::8F4790", 0x008F4790, &staticinit::Fn8F4790);
STATICINIT5_REVERSED("StaticExit::8F47A0", 0x008F47A0, &staticinit::Fn8F47A0);
STATICINIT5_REVERSED("StaticExit::8F47B0", 0x008F47B0, &staticinit::Fn8F47B0);
STATICINIT5_REVERSED("StaticExit::8F47C0", 0x008F47C0, &staticinit::Fn8F47C0);
STATICINIT5_REVERSED("StaticExit::8F47D0", 0x008F47D0, &staticinit::Fn8F47D0);
STATICINIT5_REVERSED("StaticExit::8F47E0", 0x008F47E0, &staticinit::Fn8F47E0);
STATICINIT5_REVERSED("StaticExit::8F47F0", 0x008F47F0, &staticinit::Fn8F47F0);
STATICINIT5_REVERSED("StaticExit::8F4800", 0x008F4800, &staticinit::Fn8F4800);
STATICINIT5_REVERSED("StaticExit::8F4810", 0x008F4810, &staticinit::Fn8F4810);
STATICINIT5_REVERSED("StaticExit::8F4820", 0x008F4820, &staticinit::Fn8F4820);
STATICINIT5_REVERSED("StaticExit::8F4830", 0x008F4830, &staticinit::Fn8F4830);
STATICINIT5_REVERSED("StaticExit::8F4840", 0x008F4840, &staticinit::Fn8F4840);
STATICINIT5_REVERSED("StaticExit::8F4850", 0x008F4850, &staticinit::Fn8F4850);
STATICINIT5_REVERSED("StaticExit::8F4860", 0x008F4860, &staticinit::Fn8F4860);
STATICINIT5_REVERSED("StaticExit::8F4870", 0x008F4870, &staticinit::Fn8F4870);
STATICINIT5_REVERSED("StaticExit::8F4880", 0x008F4880, &staticinit::Fn8F4880);
STATICINIT5_REVERSED("StaticExit::8F4890", 0x008F4890, &staticinit::Fn8F4890);
STATICINIT5_REVERSED("StaticExit::8F48A0", 0x008F48A0, &staticinit::Fn8F48A0);
STATICINIT5_REVERSED("StaticExit::8F48B0", 0x008F48B0, &staticinit::Fn8F48B0);
STATICINIT5_REVERSED("StaticExit::8F48C0", 0x008F48C0, &staticinit::Fn8F48C0);
STATICINIT5_REVERSED("StaticExit::8F48D0", 0x008F48D0, &staticinit::Fn8F48D0);
STATICINIT5_REVERSED("StaticExit::8F48E0", 0x008F48E0, &staticinit::Fn8F48E0);
STATICINIT5_REVERSED("StaticExit::8F48F0", 0x008F48F0, &staticinit::Fn8F48F0);
STATICINIT5_REVERSED("StaticExit::8F4900", 0x008F4900, &staticinit::Fn8F4900);
STATICINIT5_REVERSED("StaticExit::8F4910", 0x008F4910, &staticinit::Fn8F4910);
STATICINIT5_REVERSED("StaticExit::8F4920", 0x008F4920, &staticinit::Fn8F4920);
STATICINIT5_REVERSED("StaticExit::8F4930", 0x008F4930, &staticinit::Fn8F4930);
STATICINIT5_REVERSED("StaticExit::8F4940", 0x008F4940, &staticinit::Fn8F4940);
STATICINIT5_REVERSED("StaticExit::8F4950", 0x008F4950, &staticinit::Fn8F4950);
STATICINIT5_REVERSED("StaticExit::8F4960", 0x008F4960, &staticinit::Fn8F4960);
STATICINIT5_REVERSED("StaticExit::8F4970", 0x008F4970, &staticinit::Fn8F4970);
STATICINIT5_REVERSED("StaticExit::8F4980", 0x008F4980, &staticinit::Fn8F4980);
STATICINIT5_REVERSED("StaticExit::8F4990", 0x008F4990, &staticinit::Fn8F4990);
STATICINIT5_REVERSED("StaticExit::8F49A0", 0x008F49A0, &staticinit::Fn8F49A0);
STATICINIT5_REVERSED("StaticExit::8F49B0", 0x008F49B0, &staticinit::Fn8F49B0);
STATICINIT5_REVERSED("StaticExit::8F49C0", 0x008F49C0, &staticinit::Fn8F49C0);
STATICINIT5_REVERSED("StaticExit::8F49D0", 0x008F49D0, &staticinit::Fn8F49D0);
STATICINIT5_REVERSED("StaticExit::8F49E0", 0x008F49E0, &staticinit::Fn8F49E0);
STATICINIT5_REVERSED("StaticExit::8F49F0", 0x008F49F0, &staticinit::Fn8F49F0);
STATICINIT5_REVERSED("StaticExit::8F4A00", 0x008F4A00, &staticinit::Fn8F4A00);
STATICINIT5_REVERSED("StaticExit::8F4A10", 0x008F4A10, &staticinit::Fn8F4A10);
STATICINIT5_REVERSED("StaticExit::8F4A20", 0x008F4A20, &staticinit::Fn8F4A20);
STATICINIT5_REVERSED("StaticExit::8F4A30", 0x008F4A30, &staticinit::Fn8F4A30);
STATICINIT5_REVERSED("StaticExit::8F4A40", 0x008F4A40, &staticinit::Fn8F4A40);
STATICINIT5_REVERSED("StaticExit::8F4A50", 0x008F4A50, &staticinit::Fn8F4A50);
STATICINIT5_REVERSED("StaticExit::8F4A60", 0x008F4A60, &staticinit::Fn8F4A60);
STATICINIT5_REVERSED("StaticExit::8F4A70", 0x008F4A70, &staticinit::Fn8F4A70);
STATICINIT5_REVERSED("StaticExit::8F4A80", 0x008F4A80, &staticinit::Fn8F4A80);
STATICINIT5_REVERSED("StaticExit::8F4A90", 0x008F4A90, &staticinit::Fn8F4A90);
STATICINIT5_REVERSED("StaticExit::8F4AA0", 0x008F4AA0, &staticinit::Fn8F4AA0);
STATICINIT5_REVERSED("StaticExit::8F4AB0", 0x008F4AB0, &staticinit::Fn8F4AB0);
STATICINIT5_REVERSED("StaticExit::8F4AC0", 0x008F4AC0, &staticinit::Fn8F4AC0);
STATICINIT5_REVERSED("StaticExit::8F4AD0", 0x008F4AD0, &staticinit::Fn8F4AD0);
STATICINIT5_REVERSED("StaticExit::8F4AE0", 0x008F4AE0, &staticinit::Fn8F4AE0);
STATICINIT5_REVERSED("StaticExit::8F4AF0", 0x008F4AF0, &staticinit::Fn8F4AF0);
STATICINIT5_REVERSED("StaticExit::8F4B00", 0x008F4B00, &staticinit::Fn8F4B00);
STATICINIT5_REVERSED("StaticExit::8F4B10", 0x008F4B10, &staticinit::Fn8F4B10);
STATICINIT5_REVERSED("StaticExit::8F4B20", 0x008F4B20, &staticinit::Fn8F4B20);
STATICINIT5_REVERSED("StaticExit::8F4B30", 0x008F4B30, &staticinit::Fn8F4B30);
STATICINIT5_REVERSED("StaticExit::8F4B40", 0x008F4B40, &staticinit::Fn8F4B40);
STATICINIT5_REVERSED("StaticExit::8F4B50", 0x008F4B50, &staticinit::Fn8F4B50);
STATICINIT5_REVERSED("StaticExit::8F4B60", 0x008F4B60, &staticinit::Fn8F4B60);
STATICINIT5_REVERSED("StaticExit::8F4B70", 0x008F4B70, &staticinit::Fn8F4B70);
STATICINIT5_REVERSED("StaticExit::8F4B80", 0x008F4B80, &staticinit::Fn8F4B80);
STATICINIT5_REVERSED("StaticExit::8F4B90", 0x008F4B90, &staticinit::Fn8F4B90);
STATICINIT5_REVERSED("StaticExit::8F4BA0", 0x008F4BA0, &staticinit::Fn8F4BA0);
STATICINIT5_REVERSED("StaticExit::8F4BB0", 0x008F4BB0, &staticinit::Fn8F4BB0);
STATICINIT5_REVERSED("StaticExit::8F4BC0", 0x008F4BC0, &staticinit::Fn8F4BC0);
STATICINIT5_REVERSED("StaticExit::8F4BD0", 0x008F4BD0, &staticinit::Fn8F4BD0);
STATICINIT5_REVERSED("StaticExit::8F4BE0", 0x008F4BE0, &staticinit::Fn8F4BE0);
STATICINIT5_REVERSED("StaticExit::8F4BF0", 0x008F4BF0, &staticinit::Fn8F4BF0);
STATICINIT5_REVERSED("StaticExit::8F4C00", 0x008F4C00, &staticinit::Fn8F4C00);
STATICINIT5_REVERSED("StaticExit::8F4C10", 0x008F4C10, &staticinit::Fn8F4C10);
STATICINIT5_REVERSED("StaticExit::8F4C20", 0x008F4C20, &staticinit::Fn8F4C20);
STATICINIT5_REVERSED("StaticExit::8F4C30", 0x008F4C30, &staticinit::Fn8F4C30);
STATICINIT5_REVERSED("StaticExit::8F4C40", 0x008F4C40, &staticinit::Fn8F4C40);
STATICINIT5_REVERSED("StaticExit::8F4C50", 0x008F4C50, &staticinit::Fn8F4C50);
STATICINIT5_REVERSED("StaticExit::8F4C60", 0x008F4C60, &staticinit::Fn8F4C60);
STATICINIT5_REVERSED("StaticExit::8F4C70", 0x008F4C70, &staticinit::Fn8F4C70);
STATICINIT5_REVERSED("StaticExit::8F4C80", 0x008F4C80, &staticinit::Fn8F4C80);
STATICINIT5_REVERSED("StaticExit::8F4C90", 0x008F4C90, &staticinit::Fn8F4C90);
STATICINIT5_REVERSED("StaticExit::8F4CA0", 0x008F4CA0, &staticinit::Fn8F4CA0);
STATICINIT5_REVERSED("StaticExit::8F4CB0", 0x008F4CB0, &staticinit::Fn8F4CB0);
STATICINIT5_REVERSED("StaticExit::8F4CC0", 0x008F4CC0, &staticinit::Fn8F4CC0);
STATICINIT5_REVERSED("StaticExit::8F4CD0", 0x008F4CD0, &staticinit::Fn8F4CD0);
STATICINIT5_REVERSED("StaticExit::8F4CE0", 0x008F4CE0, &staticinit::Fn8F4CE0);
STATICINIT5_REVERSED("StaticExit::8F4CF0", 0x008F4CF0, &staticinit::Fn8F4CF0);
STATICINIT5_REVERSED("StaticExit::8F4D00", 0x008F4D00, &staticinit::Fn8F4D00);
STATICINIT5_REVERSED("StaticExit::8F4D10", 0x008F4D10, &staticinit::Fn8F4D10);
STATICINIT5_REVERSED("StaticExit::8F4D20", 0x008F4D20, &staticinit::Fn8F4D20);
STATICINIT5_REVERSED("StaticExit::8F4D30", 0x008F4D30, &staticinit::Fn8F4D30);
STATICINIT5_REVERSED("StaticExit::8F4D40", 0x008F4D40, &staticinit::Fn8F4D40);
STATICINIT5_REVERSED("StaticExit::8F4D50", 0x008F4D50, &staticinit::Fn8F4D50);
STATICINIT5_REVERSED("StaticExit::8F4D60", 0x008F4D60, &staticinit::Fn8F4D60);
STATICINIT5_REVERSED("StaticExit::8F4D70", 0x008F4D70, &staticinit::Fn8F4D70);
STATICINIT5_REVERSED("StaticExit::8F4D80", 0x008F4D80, &staticinit::Fn8F4D80);
STATICINIT5_REVERSED("StaticExit::8F4D90", 0x008F4D90, &staticinit::Fn8F4D90);
STATICINIT5_REVERSED("StaticExit::8F4DA0", 0x008F4DA0, &staticinit::Fn8F4DA0);
STATICINIT5_REVERSED("StaticExit::8F4DB0", 0x008F4DB0, &staticinit::Fn8F4DB0);
STATICINIT5_REVERSED("StaticExit::8F4DC0", 0x008F4DC0, &staticinit::Fn8F4DC0);
STATICINIT5_REVERSED("StaticExit::8F4DD0", 0x008F4DD0, &staticinit::Fn8F4DD0);
STATICINIT5_REVERSED("StaticExit::8F4DE0", 0x008F4DE0, &staticinit::Fn8F4DE0);
STATICINIT5_REVERSED("StaticExit::8F4DF0", 0x008F4DF0, &staticinit::Fn8F4DF0);
STATICINIT5_REVERSED("StaticExit::8F4E00", 0x008F4E00, &staticinit::Fn8F4E00);
STATICINIT5_REVERSED("StaticExit::8F4E10", 0x008F4E10, &staticinit::Fn8F4E10);
STATICINIT5_REVERSED("StaticExit::8F4E20", 0x008F4E20, &staticinit::Fn8F4E20);
STATICINIT5_REVERSED("StaticExit::8F4E30", 0x008F4E30, &staticinit::Fn8F4E30);
STATICINIT5_REVERSED("StaticExit::8F4E40", 0x008F4E40, &staticinit::Fn8F4E40);
STATICINIT5_REVERSED("StaticExit::8F4E50", 0x008F4E50, &staticinit::Fn8F4E50);
STATICINIT5_REVERSED("StaticExit::8F4E60", 0x008F4E60, &staticinit::Fn8F4E60);
STATICINIT5_REVERSED("StaticExit::8F4E70", 0x008F4E70, &staticinit::Fn8F4E70);
STATICINIT5_REVERSED("StaticExit::8F4E80", 0x008F4E80, &staticinit::Fn8F4E80);
STATICINIT5_REVERSED("StaticExit::8F4E90", 0x008F4E90, &staticinit::Fn8F4E90);
STATICINIT5_REVERSED("StaticExit::8F4EA0", 0x008F4EA0, &staticinit::Fn8F4EA0);
STATICINIT5_REVERSED("StaticExit::8F4EB0", 0x008F4EB0, &staticinit::Fn8F4EB0);
STATICINIT5_REVERSED("StaticExit::8F4EC0", 0x008F4EC0, &staticinit::Fn8F4EC0);
STATICINIT5_REVERSED("StaticExit::8F4ED0", 0x008F4ED0, &staticinit::Fn8F4ED0);
STATICINIT5_REVERSED("StaticExit::8F4EE0", 0x008F4EE0, &staticinit::Fn8F4EE0);
STATICINIT5_REVERSED("StaticExit::8F4EF0", 0x008F4EF0, &staticinit::Fn8F4EF0);
STATICINIT5_REVERSED("StaticExit::8F4F00", 0x008F4F00, &staticinit::Fn8F4F00);
STATICINIT5_REVERSED("StaticExit::8F4F10", 0x008F4F10, &staticinit::Fn8F4F10);
STATICINIT5_REVERSED("StaticExit::8F4F20", 0x008F4F20, &staticinit::Fn8F4F20);
STATICINIT5_REVERSED("StaticExit::8F4F30", 0x008F4F30, &staticinit::Fn8F4F30);
STATICINIT5_REVERSED("StaticExit::8F4F40", 0x008F4F40, &staticinit::Fn8F4F40);
STATICINIT5_REVERSED("StaticExit::8F4F50", 0x008F4F50, &staticinit::Fn8F4F50);
STATICINIT5_REVERSED("StaticExit::8F4F60", 0x008F4F60, &staticinit::Fn8F4F60);
STATICINIT5_REVERSED("StaticExit::8F4F70", 0x008F4F70, &staticinit::Fn8F4F70);
STATICINIT5_REVERSED("StaticExit::8F4F80", 0x008F4F80, &staticinit::Fn8F4F80);
STATICINIT5_REVERSED("StaticExit::8F4F90", 0x008F4F90, &staticinit::Fn8F4F90);
STATICINIT5_REVERSED("StaticExit::8F4FA0", 0x008F4FA0, &staticinit::Fn8F4FA0);
STATICINIT5_REVERSED("StaticExit::8F4FB0", 0x008F4FB0, &staticinit::Fn8F4FB0);
STATICINIT5_REVERSED("StaticExit::8F4FC0", 0x008F4FC0, &staticinit::Fn8F4FC0);
STATICINIT5_REVERSED("StaticExit::8F4FD0", 0x008F4FD0, &staticinit::Fn8F4FD0);
STATICINIT5_REVERSED("StaticExit::8F4FE0", 0x008F4FE0, &staticinit::Fn8F4FE0);
STATICINIT5_REVERSED("StaticExit::8F4FF0", 0x008F4FF0, &staticinit::Fn8F4FF0);
STATICINIT5_REVERSED("StaticExit::8F5000", 0x008F5000, &staticinit::Fn8F5000);
STATICINIT5_REVERSED("StaticExit::8F5010", 0x008F5010, &staticinit::Fn8F5010);
STATICINIT5_REVERSED("StaticExit::8F5020", 0x008F5020, &staticinit::Fn8F5020);
STATICINIT5_REVERSED("StaticExit::8F5030", 0x008F5030, &staticinit::Fn8F5030);
STATICINIT5_REVERSED("StaticExit::8F5040", 0x008F5040, &staticinit::Fn8F5040);
STATICINIT5_REVERSED("StaticExit::8F5050", 0x008F5050, &staticinit::Fn8F5050);
STATICINIT5_REVERSED("StaticExit::8F5060", 0x008F5060, &staticinit::Fn8F5060);
STATICINIT5_REVERSED("StaticExit::8F5070", 0x008F5070, &staticinit::Fn8F5070);
STATICINIT5_REVERSED("StaticExit::8F5080", 0x008F5080, &staticinit::Fn8F5080);
STATICINIT5_REVERSED("StaticExit::8F5090", 0x008F5090, &staticinit::Fn8F5090);
STATICINIT5_REVERSED("StaticExit::8F50A0", 0x008F50A0, &staticinit::Fn8F50A0);
STATICINIT5_REVERSED("StaticExit::8F50B0", 0x008F50B0, &staticinit::Fn8F50B0);
STATICINIT5_REVERSED("StaticExit::8F50C0", 0x008F50C0, &staticinit::Fn8F50C0);
STATICINIT5_REVERSED("StaticExit::8F50D0", 0x008F50D0, &staticinit::Fn8F50D0);
STATICINIT5_REVERSED("StaticExit::8F50E0", 0x008F50E0, &staticinit::Fn8F50E0);
STATICINIT5_REVERSED("StaticExit::8F50F0", 0x008F50F0, &staticinit::Fn8F50F0);
STATICINIT5_REVERSED("StaticExit::8F5100", 0x008F5100, &staticinit::Fn8F5100);
STATICINIT5_REVERSED("StaticExit::8F5110", 0x008F5110, &staticinit::Fn8F5110);
STATICINIT5_REVERSED("StaticExit::8F5120", 0x008F5120, &staticinit::Fn8F5120);
STATICINIT5_REVERSED("StaticExit::8F5130", 0x008F5130, &staticinit::Fn8F5130);
STATICINIT5_REVERSED("StaticExit::8F5140", 0x008F5140, &staticinit::Fn8F5140);
STATICINIT5_REVERSED("StaticExit::8F5150", 0x008F5150, &staticinit::Fn8F5150);
STATICINIT5_REVERSED("StaticExit::8F5160", 0x008F5160, &staticinit::Fn8F5160);
STATICINIT5_REVERSED("StaticExit::8F5170", 0x008F5170, &staticinit::Fn8F5170);
STATICINIT5_REVERSED("StaticExit::8F5180", 0x008F5180, &staticinit::Fn8F5180);
STATICINIT5_REVERSED("StaticExit::8F5190", 0x008F5190, &staticinit::Fn8F5190);
STATICINIT5_REVERSED("StaticExit::8F51A0", 0x008F51A0, &staticinit::Fn8F51A0);
STATICINIT5_REVERSED("StaticExit::8F51B0", 0x008F51B0, &staticinit::Fn8F51B0);
STATICINIT5_REVERSED("StaticExit::8F51C0", 0x008F51C0, &staticinit::Fn8F51C0);
STATICINIT5_REVERSED("StaticExit::8F51D0", 0x008F51D0, &staticinit::Fn8F51D0);
STATICINIT5_REVERSED("StaticExit::8F51E0", 0x008F51E0, &staticinit::Fn8F51E0);
STATICINIT5_REVERSED("StaticExit::8F51F0", 0x008F51F0, &staticinit::Fn8F51F0);
STATICINIT5_REVERSED("StaticExit::8F5200", 0x008F5200, &staticinit::Fn8F5200);
STATICINIT5_REVERSED("StaticExit::8F5210", 0x008F5210, &staticinit::Fn8F5210);
STATICINIT5_REVERSED("StaticExit::8F5220", 0x008F5220, &staticinit::Fn8F5220);
STATICINIT5_REVERSED("StaticExit::8F5230", 0x008F5230, &staticinit::Fn8F5230);
STATICINIT5_REVERSED("StaticExit::8F5240", 0x008F5240, &staticinit::Fn8F5240);
STATICINIT5_REVERSED("StaticExit::8F5250", 0x008F5250, &staticinit::Fn8F5250);
STATICINIT5_REVERSED("StaticExit::8F5260", 0x008F5260, &staticinit::Fn8F5260);
STATICINIT5_REVERSED("StaticExit::8F5270", 0x008F5270, &staticinit::Fn8F5270);
STATICINIT5_REVERSED("StaticExit::8F5280", 0x008F5280, &staticinit::Fn8F5280);
STATICINIT5_REVERSED("StaticExit::8F5290", 0x008F5290, &staticinit::Fn8F5290);
STATICINIT5_REVERSED("StaticExit::8F52A0", 0x008F52A0, &staticinit::Fn8F52A0);
STATICINIT5_REVERSED("StaticExit::8F52B0", 0x008F52B0, &staticinit::Fn8F52B0);
STATICINIT5_REVERSED("StaticExit::8F52C0", 0x008F52C0, &staticinit::Fn8F52C0);
STATICINIT5_REVERSED("StaticExit::8F52D0", 0x008F52D0, &staticinit::Fn8F52D0);
STATICINIT5_REVERSED("StaticExit::8F52E0", 0x008F52E0, &staticinit::Fn8F52E0);
STATICINIT5_REVERSED("StaticExit::8F52F0", 0x008F52F0, &staticinit::Fn8F52F0);
STATICINIT5_REVERSED("StaticExit::8F5300", 0x008F5300, &staticinit::Fn8F5300);
STATICINIT5_REVERSED("StaticExit::8F5310", 0x008F5310, &staticinit::Fn8F5310);
STATICINIT5_REVERSED("StaticExit::8F5320", 0x008F5320, &staticinit::Fn8F5320);
STATICINIT5_REVERSED("StaticExit::8F5330", 0x008F5330, &staticinit::Fn8F5330);
STATICINIT5_REVERSED("StaticExit::8F5340", 0x008F5340, &staticinit::Fn8F5340);
STATICINIT5_REVERSED("StaticExit::8F5350", 0x008F5350, &staticinit::Fn8F5350);
STATICINIT5_REVERSED("StaticExit::8F5360", 0x008F5360, &staticinit::Fn8F5360);
STATICINIT5_REVERSED("StaticExit::8F5370", 0x008F5370, &staticinit::Fn8F5370);
STATICINIT5_REVERSED("StaticExit::8F5380", 0x008F5380, &staticinit::Fn8F5380);
STATICINIT5_REVERSED("StaticExit::8F5390", 0x008F5390, &staticinit::Fn8F5390);
STATICINIT5_REVERSED("StaticExit::8F53A0", 0x008F53A0, &staticinit::Fn8F53A0);
STATICINIT5_REVERSED("StaticExit::8F53B0", 0x008F53B0, &staticinit::Fn8F53B0);
STATICINIT5_REVERSED("StaticExit::8F53C0", 0x008F53C0, &staticinit::Fn8F53C0);
STATICINIT5_REVERSED("StaticExit::8F53D0", 0x008F53D0, &staticinit::Fn8F53D0);
STATICINIT5_REVERSED("StaticExit::8F53E0", 0x008F53E0, &staticinit::Fn8F53E0);
STATICINIT5_REVERSED("StaticExit::8F53F0", 0x008F53F0, &staticinit::Fn8F53F0);
STATICINIT5_REVERSED("StaticExit::8F5400", 0x008F5400, &staticinit::Fn8F5400);
STATICINIT5_REVERSED("StaticExit::8F5410", 0x008F5410, &staticinit::Fn8F5410);
STATICINIT5_REVERSED("StaticExit::8F5420", 0x008F5420, &staticinit::Fn8F5420);
STATICINIT5_REVERSED("StaticExit::8F5430", 0x008F5430, &staticinit::Fn8F5430);
STATICINIT5_REVERSED("StaticExit::8F5440", 0x008F5440, &staticinit::Fn8F5440);
STATICINIT5_REVERSED("StaticExit::8F5450", 0x008F5450, &staticinit::Fn8F5450);
STATICINIT5_REVERSED("StaticExit::8F5460", 0x008F5460, &staticinit::Fn8F5460);
STATICINIT5_REVERSED("StaticExit::8F5470", 0x008F5470, &staticinit::Fn8F5470);
STATICINIT5_REVERSED("StaticExit::8F5480", 0x008F5480, &staticinit::Fn8F5480);
STATICINIT5_REVERSED("StaticExit::8F5490", 0x008F5490, &staticinit::Fn8F5490);
STATICINIT5_REVERSED("StaticExit::8F54A0", 0x008F54A0, &staticinit::Fn8F54A0);
STATICINIT5_REVERSED("StaticExit::8F54B0", 0x008F54B0, &staticinit::Fn8F54B0);
STATICINIT5_REVERSED("StaticExit::8F54C0", 0x008F54C0, &staticinit::Fn8F54C0);
STATICINIT5_REVERSED("StaticExit::8F54D0", 0x008F54D0, &staticinit::Fn8F54D0);
STATICINIT5_REVERSED("StaticExit::8F54E0", 0x008F54E0, &staticinit::Fn8F54E0);
STATICINIT5_REVERSED("StaticExit::8F54F0", 0x008F54F0, &staticinit::Fn8F54F0);
STATICINIT5_REVERSED("StaticExit::8F5500", 0x008F5500, &staticinit::Fn8F5500);
STATICINIT5_REVERSED("StaticExit::8F5510", 0x008F5510, &staticinit::Fn8F5510);
STATICINIT5_REVERSED("StaticExit::8F5520", 0x008F5520, &staticinit::Fn8F5520);
STATICINIT5_REVERSED("StaticExit::8F5530", 0x008F5530, &staticinit::Fn8F5530);
STATICINIT5_REVERSED("StaticExit::8F5540", 0x008F5540, &staticinit::Fn8F5540);
STATICINIT5_REVERSED("StaticExit::8F5550", 0x008F5550, &staticinit::Fn8F5550);
STATICINIT5_REVERSED("StaticExit::8F5560", 0x008F5560, &staticinit::Fn8F5560);
STATICINIT5_REVERSED("StaticExit::8F5570", 0x008F5570, &staticinit::Fn8F5570);
STATICINIT5_REVERSED("StaticExit::8F5580", 0x008F5580, &staticinit::Fn8F5580);
STATICINIT5_REVERSED("StaticExit::8F5590", 0x008F5590, &staticinit::Fn8F5590);
STATICINIT5_REVERSED("StaticExit::8F55A0", 0x008F55A0, &staticinit::Fn8F55A0);
STATICINIT5_REVERSED("StaticExit::8F55B0", 0x008F55B0, &staticinit::Fn8F55B0);
STATICINIT5_REVERSED("StaticExit::8F55C0", 0x008F55C0, &staticinit::Fn8F55C0);
STATICINIT5_REVERSED("StaticExit::8F55D0", 0x008F55D0, &staticinit::Fn8F55D0);
STATICINIT5_REVERSED("StaticExit::8F55E0", 0x008F55E0, &staticinit::Fn8F55E0);
STATICINIT5_REVERSED("StaticExit::8F55F0", 0x008F55F0, &staticinit::Fn8F55F0);
STATICINIT5_REVERSED("StaticExit::8F5600", 0x008F5600, &staticinit::Fn8F5600);
STATICINIT5_REVERSED("StaticExit::8F5610", 0x008F5610, &staticinit::Fn8F5610);
STATICINIT5_REVERSED("StaticExit::8F5620", 0x008F5620, &staticinit::Fn8F5620);
STATICINIT5_REVERSED("StaticExit::8F5630", 0x008F5630, &staticinit::Fn8F5630);
STATICINIT5_REVERSED("StaticExit::8F5640", 0x008F5640, &staticinit::Fn8F5640);
STATICINIT5_REVERSED("StaticExit::8F5650", 0x008F5650, &staticinit::Fn8F5650);
STATICINIT5_REVERSED("StaticExit::8F5660", 0x008F5660, &staticinit::Fn8F5660);
STATICINIT5_REVERSED("StaticExit::8F5670", 0x008F5670, &staticinit::Fn8F5670);
STATICINIT5_REVERSED("StaticExit::8F5680", 0x008F5680, &staticinit::Fn8F5680);
STATICINIT5_REVERSED("StaticExit::8F5690", 0x008F5690, &staticinit::Fn8F5690);
STATICINIT5_REVERSED("StaticExit::8F56A0", 0x008F56A0, &staticinit::Fn8F56A0);
STATICINIT5_REVERSED("StaticExit::8F56B0", 0x008F56B0, &staticinit::Fn8F56B0);
STATICINIT5_REVERSED("StaticExit::8F56C0", 0x008F56C0, &staticinit::Fn8F56C0);
STATICINIT5_REVERSED("StaticExit::8F56D0", 0x008F56D0, &staticinit::Fn8F56D0);
STATICINIT5_REVERSED("StaticExit::8F56E0", 0x008F56E0, &staticinit::Fn8F56E0);
STATICINIT5_REVERSED("StaticExit::8F56F0", 0x008F56F0, &staticinit::Fn8F56F0);
STATICINIT5_REVERSED("StaticExit::8F5700", 0x008F5700, &staticinit::Fn8F5700);
STATICINIT5_REVERSED("StaticExit::8F5710", 0x008F5710, &staticinit::Fn8F5710);
STATICINIT5_REVERSED("StaticExit::8F5720", 0x008F5720, &staticinit::Fn8F5720);
STATICINIT5_REVERSED("StaticExit::8F5730", 0x008F5730, &staticinit::Fn8F5730);
STATICINIT5_REVERSED("StaticExit::8F5740", 0x008F5740, &staticinit::Fn8F5740);
STATICINIT5_REVERSED("StaticExit::8F5750", 0x008F5750, &staticinit::Fn8F5750);
STATICINIT5_REVERSED("StaticExit::8F5760", 0x008F5760, &staticinit::Fn8F5760);
STATICINIT5_REVERSED("StaticExit::8F5770", 0x008F5770, &staticinit::Fn8F5770);
STATICINIT5_REVERSED("StaticExit::8F5780", 0x008F5780, &staticinit::Fn8F5780);
STATICINIT5_REVERSED("StaticExit::8F5790", 0x008F5790, &staticinit::Fn8F5790);
STATICINIT5_REVERSED("StaticExit::8F57A0", 0x008F57A0, &staticinit::Fn8F57A0);
STATICINIT5_REVERSED("StaticExit::8F57B0", 0x008F57B0, &staticinit::Fn8F57B0);
STATICINIT5_REVERSED("StaticExit::8F57C0", 0x008F57C0, &staticinit::Fn8F57C0);
STATICINIT5_REVERSED("StaticExit::8F57D0", 0x008F57D0, &staticinit::Fn8F57D0);
STATICINIT5_REVERSED("StaticExit::8F57E0", 0x008F57E0, &staticinit::Fn8F57E0);
STATICINIT5_REVERSED("StaticExit::8F57F0", 0x008F57F0, &staticinit::Fn8F57F0);
STATICINIT5_REVERSED("StaticExit::8F5800", 0x008F5800, &staticinit::Fn8F5800);
STATICINIT5_REVERSED("StaticExit::8F5810", 0x008F5810, &staticinit::Fn8F5810);
STATICINIT5_REVERSED("StaticExit::8F5820", 0x008F5820, &staticinit::Fn8F5820);
STATICINIT5_REVERSED("StaticExit::8F5830", 0x008F5830, &staticinit::Fn8F5830);
STATICINIT5_REVERSED("StaticExit::8F5840", 0x008F5840, &staticinit::Fn8F5840);
STATICINIT5_REVERSED("StaticExit::8F5850", 0x008F5850, &staticinit::Fn8F5850);
STATICINIT5_REVERSED("StaticExit::8F5860", 0x008F5860, &staticinit::Fn8F5860);
STATICINIT5_REVERSED("StaticExit::8F5870", 0x008F5870, &staticinit::Fn8F5870);
STATICINIT5_REVERSED("StaticExit::8F5880", 0x008F5880, &staticinit::Fn8F5880);
STATICINIT5_REVERSED("StaticExit::8F5890", 0x008F5890, &staticinit::Fn8F5890);
STATICINIT5_REVERSED("StaticExit::8F58A0", 0x008F58A0, &staticinit::Fn8F58A0);
STATICINIT5_REVERSED("StaticExit::8F58B0", 0x008F58B0, &staticinit::Fn8F58B0);
STATICINIT5_REVERSED("StaticExit::8F58C0", 0x008F58C0, &staticinit::Fn8F58C0);
