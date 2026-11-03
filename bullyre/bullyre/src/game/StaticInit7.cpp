// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8F8890 (exit)
void __cdecl Fn8F8890() {
    ReleaseObject(0x020E2D38);
}

// 0x8F88C0 (exit)
void __cdecl Fn8F88C0() {
    ReleaseFixedString(0x020E2DB8);
}

// 0x8F8900 (exit)
void __cdecl Fn8F8900() {
    ReleaseObject(0x020E2DF0);
}

// 0x8F8930 (exit)
void __cdecl Fn8F8930() {
    ReleaseObject(0x020E2F2C);
}

// 0x8F8960 (exit)
void __cdecl Fn8F8960() {
    ReleaseObject(0x020E2F64);
}

// 0x8F8990 (exit)
void __cdecl Fn8F8990() {
    ReleaseObject(0x020E2F9C);
}

// 0x8F89C0 (exit)
void __cdecl Fn8F89C0() {
    ReleaseObject(0x020E303C);
}

// 0x8F89F0 (exit)
void __cdecl Fn8F89F0() {
    Stdcall(Dword(0x008FC088), 0x020E30DCu);
}

// 0x8F8A50 (exit)
void __cdecl Fn8F8A50() {
    Stdcall(Dword(0x008FC088), 0x020E32F0u);
}

// 0x8F8A60 (exit)
void __cdecl Fn8F8A60() {
    ReleaseFixedString(0x020E35C4);
}

// 0x8F8AA0 (exit)
void __cdecl Fn8F8AA0() {
    ReleaseFixedString(0x020E393C);
}

// 0x8F8AE0 (exit)
void __cdecl Fn8F8AE0() {
    ReleaseObject(0x020E394C);
}

// 0x8F8B10 (exit)
void __cdecl Fn8F8B10() {
    Stdcall(Dword(0x008FC088), 0x020E398Cu);
}

// 0x8F8B20 (exit)
void __cdecl Fn8F8B20() {
    ReleaseObject(0x020E3AF8);
}

// 0x8F8B50 (exit)
void __cdecl Fn8F8B50() {
    ReleaseObject(0x020E3C9C);
}

// 0x8F8B80 (exit)
void __cdecl Fn8F8B80() {
    ReleaseObject(0x020E3DD8);
}

// 0x8F8BB0 (exit)
void __cdecl Fn8F8BB0() {
    ReleaseObject(0x020E3EB0);
}

// 0x8F8BE0 (exit)
void __cdecl Fn8F8BE0() {
    Stdcall(Dword(0x008FC088), 0x020E40CCu);
}

// 0x8F8C10 (exit)
void __cdecl Fn8F8C10() {
    Stdcall(Dword(0x008FC088), 0x020E4224u);
}

// 0x8F8C20 (exit)
void __cdecl Fn8F8C20() {
    Stdcall(Dword(0x008FC088), 0x020E4A88u);
}

// 0x8F8C30 (exit)
void __cdecl Fn8F8C30() {
    Stdcall(Dword(0x008FC088), 0x020E4AB0u);
}

// 0x8F8C40 (exit)
void __cdecl Fn8F8C40() {
    ReleaseFixedString(0x020E4AD0);
}

// 0x8F8C80 (exit)
void __cdecl Fn8F8C80() {
    ReleaseFixedString(0x020E4AD4);
}

// 0x8F8CC0 (exit)
void __cdecl Fn8F8CC0() {
    ReleaseObject(0x020E59C8);
}

// 0x8F8CF0 (exit)
void __cdecl Fn8F8CF0() {
    ReleaseFixedString(0x020E5BF4);
}

// 0x8F8D30 (exit)
void __cdecl Fn8F8D30() {
    ReleaseFixedString(0x020E5BF8);
}

// 0x8F8D70 (exit)
void __cdecl Fn8F8D70() {
    ReleaseFixedString(0x020E5BFC);
}

// 0x8F8DB0 (exit)
void __cdecl Fn8F8DB0() {
    ReleaseFixedString(0x020E5C00);
}

// 0x8F8DF0 (exit)
void __cdecl Fn8F8DF0() {
    ReleaseFixedString(0x020E5C04);
}

// 0x8F8E30 (exit)
void __cdecl Fn8F8E30() {
    ReleaseFixedString(0x020E5C08);
}

// 0x8F8E70 (exit)
void __cdecl Fn8F8E70() {
    ReleaseFixedString(0x020E5C0C);
}

// 0x8F8EB0 (exit)
void __cdecl Fn8F8EB0() {
    ReleaseFixedString(0x020E5C10);
}

// 0x8F8EF0 (exit)
void __cdecl Fn8F8EF0() {
    ReleaseFixedString(0x020E5C14);
}

// 0x8F8F30 (exit)
void __cdecl Fn8F8F30() {
    ReleaseFixedString(0x020E5C18);
}

// 0x8F8F70 (exit)
void __cdecl Fn8F8F70() {
    ReleaseFixedString(0x020E5C1C);
}

// 0x8F8FB0 (exit)
void __cdecl Fn8F8FB0() {
    ReleaseFixedString(0x020E5C20);
}

// 0x8F8FF0 (exit)
void __cdecl Fn8F8FF0() {
    ReleaseFixedString(0x020E5C24);
}

// 0x8F9030 (exit)
void __cdecl Fn8F9030() {
    ReleaseFixedString(0x020E5C28);
}

// 0x8F9070 (exit)
void __cdecl Fn8F9070() {
    ReleaseFixedString(0x020E5C2C);
}

// 0x8F90B0 (exit)
void __cdecl Fn8F90B0() {
    ReleaseFixedString(0x020E5C30);
}

// 0x8F90F0 (exit)
void __cdecl Fn8F90F0() {
    ReleaseFixedString(0x020E5C34);
}

// 0x8F9130 (exit)
void __cdecl Fn8F9130() {
    ReleaseFixedString(0x020E5C38);
}

// 0x8F9170 (exit)
void __cdecl Fn8F9170() {
    ReleaseFixedString(0x020E5C3C);
}

// 0x8F91B0 (exit)
void __cdecl Fn8F91B0() {
    ReleaseFixedString(0x020E5C40);
}

// 0x8F91F0 (exit)
void __cdecl Fn8F91F0() {
    ReleaseFixedString(0x020E5C44);
}

// 0x8F9230 (exit)
void __cdecl Fn8F9230() {
    ReleaseFixedString(0x020E5C48);
}

// 0x8F9270 (exit)
void __cdecl Fn8F9270() {
    ReleaseFixedString(0x020E5C4C);
}

// 0x8F92B0 (exit)
void __cdecl Fn8F92B0() {
    ReleaseFixedString(0x020E5C50);
}

// 0x8F92F0 (exit)
void __cdecl Fn8F92F0() {
    ReleaseFixedString(0x020E5C54);
}

// 0x8F9330 (exit)
void __cdecl Fn8F9330() {
    ReleaseFixedString(0x020E5C58);
}

// 0x8F9370 (exit)
void __cdecl Fn8F9370() {
    ReleaseFixedString(0x020E5C5C);
}

// 0x8F93B0 (exit)
void __cdecl Fn8F93B0() {
    ReleaseFixedString(0x020E5C60);
}

// 0x8F93F0 (exit)
void __cdecl Fn8F93F0() {
    ReleaseFixedString(0x020E5C78);
}

// 0x8F9430 (exit)
void __cdecl Fn8F9430() {
    ReleaseFixedString(0x020E5C7C);
}

// 0x8F9470 (exit)
void __cdecl Fn8F9470() {
    ReleaseFixedString(0x020E5C80);
}

// 0x8F94B0 (exit)
void __cdecl Fn8F94B0() {
    ReleaseFixedString(0x020E5C84);
}

// 0x8F94F0 (exit)
void __cdecl Fn8F94F0() {
    ReleaseFixedString(0x020E5C88);
}

// 0x8F9530 (exit)
void __cdecl Fn8F9530() {
    ReleaseFixedString(0x020E5C8C);
}

// 0x8F9570 (exit)
void __cdecl Fn8F9570() {
    ReleaseFixedString(0x020E5C90);
}

// 0x8F95B0 (exit)
void __cdecl Fn8F95B0() {
    ReleaseFixedString(0x020E5C94);
}

// 0x8F95F0 (exit)
void __cdecl Fn8F95F0() {
    ReleaseFixedString(0x020E5CAC);
}

// 0x8F9630 (exit)
void __cdecl Fn8F9630() {
    ReleaseFixedString(0x020E5CB0);
}

// 0x8F9670 (exit)
void __cdecl Fn8F9670() {
    ReleaseFixedString(0x020E5CB4);
}

// 0x8F96B0 (exit)
void __cdecl Fn8F96B0() {
    ReleaseFixedString(0x020E5CB8);
}

// 0x8F96F0 (exit)
void __cdecl Fn8F96F0() {
    ReleaseFixedString(0x020E5CBC);
}

// 0x8F9730 (exit)
void __cdecl Fn8F9730() {
    ReleaseFixedString(0x020E5CC0);
}

// 0x8F9770 (exit)
void __cdecl Fn8F9770() {
    ReleaseFixedString(0x020E5CC4);
}

// 0x8F97B0 (exit)
void __cdecl Fn8F97B0() {
    ReleaseFixedString(0x020E5CC8);
}

// 0x8F97F0 (exit)
void __cdecl Fn8F97F0() {
    ReleaseFixedString(0x020E5CCC);
}

// 0x8F9830 (exit)
void __cdecl Fn8F9830() {
    ReleaseFixedString(0x020E5CD0);
}

// 0x8F9870 (exit)
void __cdecl Fn8F9870() {
    ReleaseFixedString(0x020E5CD4);
}

// 0x8F98B0 (exit)
void __cdecl Fn8F98B0() {
    ReleaseFixedString(0x020E5CD8);
}

// 0x8F98F0 (exit)
void __cdecl Fn8F98F0() {
    ReleaseFixedString(0x020E5CDC);
}

// 0x8F9930 (exit)
void __cdecl Fn8F9930() {
    ReleaseFixedString(0x020E5CE0);
}

// 0x8F9970 (exit)
void __cdecl Fn8F9970() {
    ReleaseFixedString(0x020E5CE4);
}

// 0x8F99B0 (exit)
void __cdecl Fn8F99B0() {
    ReleaseFixedString(0x020E5CE8);
}

// 0x8F99F0 (exit)
void __cdecl Fn8F99F0() {
    ReleaseFixedString(0x020E5CEC);
}

// 0x8F9A30 (exit)
void __cdecl Fn8F9A30() {
    ReleaseFixedString(0x020E5CF0);
}

// 0x8F9A70 (exit)
void __cdecl Fn8F9A70() {
    ReleaseFixedString(0x020E5CF4);
}

// 0x8F9AB0 (exit)
void __cdecl Fn8F9AB0() {
    ReleaseFixedString(0x020E5CF8);
}

// 0x8F9AF0 (exit)
void __cdecl Fn8F9AF0() {
    ReleaseFixedString(0x020E5CFC);
}

// 0x8F9B30 (exit)
void __cdecl Fn8F9B30() {
    ReleaseFixedString(0x020E5D00);
}

// 0x8F9B70 (exit)
void __cdecl Fn8F9B70() {
    ReleaseFixedString(0x020E5D04);
}

// 0x8F9BB0 (exit)
void __cdecl Fn8F9BB0() {
    ReleaseFixedString(0x020E5D08);
}

// 0x8F9BF0 (exit)
void __cdecl Fn8F9BF0() {
    ReleaseFixedString(0x020E5D0C);
}

// 0x8F9C30 (exit)
void __cdecl Fn8F9C30() {
    ReleaseFixedString(0x020E5D10);
}

// 0x8F9C70 (exit)
void __cdecl Fn8F9C70() {
    ReleaseFixedString(0x020E5D14);
}

// 0x8F9CB0 (exit)
void __cdecl Fn8F9CB0() {
    ReleaseFixedString(0x020E5D18);
}

// 0x8F9CF0 (exit)
void __cdecl Fn8F9CF0() {
    ReleaseFixedString(0x020E5D1C);
}

// 0x8F9D30 (exit)
void __cdecl Fn8F9D30() {
    ReleaseFixedString(0x020E5D34);
}

// 0x8F9D70 (exit)
void __cdecl Fn8F9D70() {
    ReleaseFixedString(0x020E5D38);
}

// 0x8F9DB0 (exit)
void __cdecl Fn8F9DB0() {
    ReleaseFixedString(0x020E5D3C);
}

// 0x8F9DF0 (exit)
void __cdecl Fn8F9DF0() {
    ReleaseFixedString(0x020E5D40);
}

// 0x8F9E30 (exit)
void __cdecl Fn8F9E30() {
    ReleaseFixedString(0x020E5D44);
}

// 0x8F9E70 (exit)
void __cdecl Fn8F9E70() {
    ReleaseFixedString(0x020E5D48);
}

// 0x8F9EB0 (exit)
void __cdecl Fn8F9EB0() {
    ReleaseFixedString(0x020E5D4C);
}

// 0x8F9EF0 (exit)
void __cdecl Fn8F9EF0() {
    ReleaseFixedString(0x020E5D50);
}

// 0x8F9F30 (exit)
void __cdecl Fn8F9F30() {
    ReleaseFixedString(0x020E5D54);
}

// 0x8F9F70 (exit)
void __cdecl Fn8F9F70() {
    ReleaseFixedString(0x020E5D58);
}

// 0x8F9FB0 (exit)
void __cdecl Fn8F9FB0() {
    ReleaseFixedString(0x020E5D5C);
}

// 0x8F9FF0 (exit)
void __cdecl Fn8F9FF0() {
    ReleaseFixedString(0x020E5D60);
}

// 0x8FA030 (exit)
void __cdecl Fn8FA030() {
    ReleaseFixedString(0x020E5D64);
}

// 0x8FA070 (exit)
void __cdecl Fn8FA070() {
    ReleaseFixedString(0x020E5D7C);
}

// 0x8FA0B0 (exit)
void __cdecl Fn8FA0B0() {
    ReleaseFixedString(0x020E5D80);
}

// 0x8FA0F0 (exit)
void __cdecl Fn8FA0F0() {
    ReleaseFixedString(0x020E5D84);
}

// 0x8FA130 (exit)
void __cdecl Fn8FA130() {
    ReleaseFixedString(0x020E5D88);
}

// 0x8FA170 (exit)
void __cdecl Fn8FA170() {
    ReleaseFixedString(0x020E5D8C);
}

// 0x8FA1B0 (exit)
void __cdecl Fn8FA1B0() {
    ReleaseFixedString(0x020E5D90);
}

// 0x8FA1F0 (exit)
void __cdecl Fn8FA1F0() {
    ReleaseFixedString(0x020E5D94);
}

// 0x8FA230 (exit)
void __cdecl Fn8FA230() {
    ReleaseFixedString(0x020E5D98);
}

// 0x8FA270 (exit)
void __cdecl Fn8FA270() {
    ReleaseFixedString(0x020E5D9C);
}

// 0x8FA2B0 (exit)
void __cdecl Fn8FA2B0() {
    ReleaseFixedString(0x020E5DA0);
}

// 0x8FA2F0 (exit)
void __cdecl Fn8FA2F0() {
    ReleaseFixedString(0x020E5DA4);
}

// 0x8FA330 (exit)
void __cdecl Fn8FA330() {
    ReleaseFixedString(0x020E5DA8);
}

// 0x8FA370 (exit)
void __cdecl Fn8FA370() {
    ReleaseFixedString(0x020E5DAC);
}

// 0x8FA3B0 (exit)
void __cdecl Fn8FA3B0() {
    ReleaseFixedString(0x020E5DB0);
}

// 0x8FA3F0 (exit)
void __cdecl Fn8FA3F0() {
    ReleaseFixedString(0x020E5DB4);
}

// 0x8FA430 (exit)
void __cdecl Fn8FA430() {
    ReleaseFixedString(0x020E5DB8);
}

// 0x8FA470 (exit)
void __cdecl Fn8FA470() {
    ReleaseFixedString(0x020E5DBC);
}

// 0x8FA4B0 (exit)
void __cdecl Fn8FA4B0() {
    ReleaseFixedString(0x020E5DC0);
}

// 0x8FA4F0 (exit)
void __cdecl Fn8FA4F0() {
    ReleaseFixedString(0x020E5DC4);
}

// 0x8FA530 (exit)
void __cdecl Fn8FA530() {
    ReleaseFixedString(0x020E5DC8);
}

// 0x8FA570 (exit)
void __cdecl Fn8FA570() {
    ReleaseFixedString(0x020E5DCC);
}

// 0x8FA5B0 (exit)
void __cdecl Fn8FA5B0() {
    ReleaseFixedString(0x020E5DD0);
}

// 0x8FA5F0 (exit)
void __cdecl Fn8FA5F0() {
    ReleaseFixedString(0x020E5DD4);
}

// 0x8FA630 (exit)
void __cdecl Fn8FA630() {
    ReleaseFixedString(0x020E5DD8);
}

// 0x8FA670 (exit)
void __cdecl Fn8FA670() {
    ReleaseFixedString(0x020E5DDC);
}

// 0x8FA6B0 (exit)
void __cdecl Fn8FA6B0() {
    ReleaseFixedString(0x020E5DE0);
}

// 0x8FA6F0 (exit)
void __cdecl Fn8FA6F0() {
    ReleaseFixedString(0x020E5DE4);
}

// 0x8FA730 (exit)
void __cdecl Fn8FA730() {
    ReleaseFixedString(0x020E5DE8);
}

// 0x8FA770 (exit)
void __cdecl Fn8FA770() {
    ReleaseFixedString(0x020E5DEC);
}

// 0x8FA7B0 (exit)
void __cdecl Fn8FA7B0() {
    ReleaseFixedString(0x020E5DF0);
}

// 0x8FA7F0 (exit)
void __cdecl Fn8FA7F0() {
    ReleaseFixedString(0x020E5DF4);
}

// 0x8FA830 (exit)
void __cdecl Fn8FA830() {
    ReleaseFixedString(0x020E5DF8);
}

// 0x8FA870 (exit)
void __cdecl Fn8FA870() {
    ReleaseFixedString(0x020E5DFC);
}

// 0x8FA8B0 (exit)
void __cdecl Fn8FA8B0() {
    ReleaseFixedString(0x020E5E00);
}

// 0x8FA8F0 (exit)
void __cdecl Fn8FA8F0() {
    ReleaseFixedString(0x020E5E04);
}

// 0x8FA930 (exit)
void __cdecl Fn8FA930() {
    ReleaseFixedString(0x020E5E08);
}

// 0x8FA970 (exit)
void __cdecl Fn8FA970() {
    ReleaseFixedString(0x020E5E0C);
}

// 0x8FA9B0 (exit)
void __cdecl Fn8FA9B0() {
    ReleaseFixedString(0x020E5E10);
}

// 0x8FA9F0 (exit)
void __cdecl Fn8FA9F0() {
    ReleaseFixedString(0x020E5E14);
}

// 0x8FAA30 (exit)
void __cdecl Fn8FAA30() {
    ReleaseFixedString(0x020E5E18);
}

// 0x8FAA70 (exit)
void __cdecl Fn8FAA70() {
    ReleaseFixedString(0x020E5E1C);
}

// 0x8FAAB0 (exit)
void __cdecl Fn8FAAB0() {
    ReleaseFixedString(0x020E5E34);
}

// 0x8FAAF0 (exit)
void __cdecl Fn8FAAF0() {
    ReleaseFixedString(0x020E5E38);
}

// 0x8FAB30 (exit)
void __cdecl Fn8FAB30() {
    ReleaseFixedString(0x020E5E3C);
}

// 0x8FAB70 (exit)
void __cdecl Fn8FAB70() {
    ReleaseFixedString(0x020E5E40);
}

// 0x8FABB0 (exit)
void __cdecl Fn8FABB0() {
    ReleaseFixedString(0x020E5E44);
}

// 0x8FABF0 (exit)
void __cdecl Fn8FABF0() {
    ReleaseFixedString(0x020E5E48);
}

// 0x8FAC30 (exit)
void __cdecl Fn8FAC30() {
    ReleaseFixedString(0x020E5E4C);
}

// 0x8FAC70 (exit)
void __cdecl Fn8FAC70() {
    ReleaseFixedString(0x020E5E50);
}

// 0x8FACB0 (exit)
void __cdecl Fn8FACB0() {
    ReleaseFixedString(0x020E5E54);
}

// 0x8FACF0 (exit)
void __cdecl Fn8FACF0() {
    ReleaseFixedString(0x020E5E58);
}

// 0x8FAD30 (exit)
void __cdecl Fn8FAD30() {
    ReleaseFixedString(0x020E5E5C);
}

// 0x8FAD70 (exit)
void __cdecl Fn8FAD70() {
    ReleaseFixedString(0x020E5E60);
}

// 0x8FADB0 (exit)
void __cdecl Fn8FADB0() {
    ReleaseFixedString(0x020E5E64);
}

// 0x8FADF0 (exit)
void __cdecl Fn8FADF0() {
    ReleaseFixedString(0x020E5E68);
}

// 0x8FAE30 (exit)
void __cdecl Fn8FAE30() {
    ReleaseFixedString(0x020E5E6C);
}

// 0x8FAE70 (exit)
void __cdecl Fn8FAE70() {
    ReleaseFixedString(0x020E5E70);
}

// 0x8FAEB0 (exit)
void __cdecl Fn8FAEB0() {
    ReleaseFixedString(0x020E5E74);
}

// 0x8FAEF0 (exit)
void __cdecl Fn8FAEF0() {
    ReleaseFixedString(0x020E5E78);
}

// 0x8FAF30 (exit)
void __cdecl Fn8FAF30() {
    ReleaseFixedString(0x020E5E7C);
}

// 0x8FAF70 (exit)
void __cdecl Fn8FAF70() {
    ReleaseFixedString(0x020E5E80);
}

// 0x8FAFB0 (exit)
void __cdecl Fn8FAFB0() {
    ReleaseFixedString(0x020E5E98);
}

// 0x8FAFF0 (exit)
void __cdecl Fn8FAFF0() {
    ReleaseFixedString(0x020E5E9C);
}

// 0x8FB030 (exit)
void __cdecl Fn8FB030() {
    ReleaseFixedString(0x020E5EA0);
}

// 0x8FB070 (exit)
void __cdecl Fn8FB070() {
    ReleaseFixedString(0x020E5EA4);
}

// 0x8FB0B0 (exit)
void __cdecl Fn8FB0B0() {
    ReleaseFixedString(0x020E5EA8);
}

// 0x8FB0F0 (exit)
void __cdecl Fn8FB0F0() {
    ReleaseFixedString(0x020E5EAC);
}

// 0x8FB130 (exit)
void __cdecl Fn8FB130() {
    ReleaseFixedString(0x020E5EB0);
}

// 0x8FB170 (exit)
void __cdecl Fn8FB170() {
    ReleaseFixedString(0x020E5EB4);
}

// 0x8FB1B0 (exit)
void __cdecl Fn8FB1B0() {
    ReleaseFixedString(0x020E5EB8);
}

// 0x8FB1F0 (exit)
void __cdecl Fn8FB1F0() {
    ReleaseFixedString(0x020E5EBC);
}

// 0x8FB230 (exit)
void __cdecl Fn8FB230() {
    ReleaseFixedString(0x020E5EC0);
}

// 0x8FB270 (exit)
void __cdecl Fn8FB270() {
    ReleaseFixedString(0x020E5EC4);
}

// 0x8FB2B0 (exit)
void __cdecl Fn8FB2B0() {
    ReleaseFixedString(0x020E5EDC);
}

// 0x8FB2F0 (exit)
void __cdecl Fn8FB2F0() {
    ReleaseFixedString(0x020E5EE0);
}

// 0x8FB330 (exit)
void __cdecl Fn8FB330() {
    ReleaseFixedString(0x020E5EE4);
}

// 0x8FB370 (exit)
void __cdecl Fn8FB370() {
    ReleaseFixedString(0x020E5EE8);
}

// 0x8FB3B0 (exit)
void __cdecl Fn8FB3B0() {
    ReleaseFixedString(0x020E5EEC);
}

// 0x8FB3F0 (exit)
void __cdecl Fn8FB3F0() {
    ReleaseFixedString(0x020E5EF0);
}

// 0x8FB430 (exit)
void __cdecl Fn8FB430() {
    ReleaseFixedString(0x020E5EF4);
}

// 0x8FB470 (exit)
void __cdecl Fn8FB470() {
    ReleaseFixedString(0x020E5EF8);
}

// 0x8FB4B0 (exit)
void __cdecl Fn8FB4B0() {
    ReleaseFixedString(0x020E5EFC);
}

// 0x8FB4F0 (exit)
void __cdecl Fn8FB4F0() {
    ReleaseFixedString(0x020E5F00);
}

// 0x8FB530 (exit)
void __cdecl Fn8FB530() {
    ReleaseFixedString(0x020E5F04);
}

// 0x8FB570 (exit)
void __cdecl Fn8FB570() {
    ReleaseFixedString(0x020E5F08);
}

// 0x8FB5B0 (exit)
void __cdecl Fn8FB5B0() {
    ReleaseFixedString(0x020E5F0C);
}

// 0x8FB5F0 (exit)
void __cdecl Fn8FB5F0() {
    ReleaseFixedString(0x020E5F10);
}

// 0x8FB630 (exit)
void __cdecl Fn8FB630() {
    ReleaseFixedString(0x020E5F14);
}

// 0x8FB670 (exit)
void __cdecl Fn8FB670() {
    ReleaseFixedString(0x020E5F18);
}

// 0x8FB6B0 (exit)
void __cdecl Fn8FB6B0() {
    ReleaseFixedString(0x020E5F1C);
}

// 0x8FB6F0 (exit)
void __cdecl Fn8FB6F0() {
    ReleaseFixedString(0x020E5F20);
}

// 0x8FB730 (exit)
void __cdecl Fn8FB730() {
    ReleaseFixedString(0x020E5F24);
}

// 0x8FB770 (exit)
void __cdecl Fn8FB770() {
    ReleaseFixedString(0x020E5F28);
}

// 0x8FB7B0 (exit)
void __cdecl Fn8FB7B0() {
    ReleaseFixedString(0x020E5F2C);
}

// 0x8FB7F0 (exit)
void __cdecl Fn8FB7F0() {
    ReleaseFixedString(0x020E5F30);
}

// 0x8FB830 (exit)
void __cdecl Fn8FB830() {
    Stdcall(Dword(0x008FC088), 0x020E6D4Cu);
}

// 0x8FB840 (exit)
void __cdecl Fn8FB840() {
    Dword(0x00BCAE1C) = 0x00952E10u;
    Thiscall(0x0075DB60, 0x00BCAE1Cu);  // NiTPointerList_RemoveAll
    Dword(0x00BCAE1C) = 0x0099C748u;
}

// 0x8FB860 (exit)
void __cdecl Fn8FB860() {
    Stdcall(Dword(0x008FC088), 0x020E6D84u);
}

// 0x8FB870 (exit)
void __cdecl Fn8FB870() {
    ReleaseFixedString(0x020E6E54);
}

// 0x8FB8B0 (exit)
void __cdecl Fn8FB8B0() {
    Stdcall(Dword(0x008FC088), 0x020E7210u);
}

// 0x8FB8C0 (exit)
void __cdecl Fn8FB8C0() {
    Dword(0x00BCB930) = 0x00952E10u;
    Thiscall(0x0075DB60, 0x00BCB930u);  // NiTPointerList_RemoveAll
    Dword(0x00BCB930) = 0x0099C748u;
}

// 0x8FB8E0 (exit)
void __cdecl Fn8FB8E0() {
    Stdcall(Dword(0x008FC088), 0x020E7410u);
}

// 0x8FB8F0 (exit)
void __cdecl Fn8FB8F0() {
    Stdcall(Dword(0x008FC088), 0x020E75C0u);
}

// 0x8FB900 (exit)
void __cdecl Fn8FB900() {
    Stdcall(Dword(0x008FC088), 0x020E8F0Cu);
}

// 0x8FB910 (exit)
void __cdecl Fn8FB910() {
    ReleaseFixedString(0x020E9260);
}

// 0x8FB950 (exit)
void __cdecl Fn8FB950() {
    ReleaseObject(0x020E9340);
}

} // namespace
} // namespace staticinit

#define STATICINIT7_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT7_REVERSED("StaticExit::8F8890", 0x008F8890, &staticinit::Fn8F8890);
STATICINIT7_REVERSED("StaticExit::8F88C0", 0x008F88C0, &staticinit::Fn8F88C0);
STATICINIT7_REVERSED("StaticExit::8F8900", 0x008F8900, &staticinit::Fn8F8900);
STATICINIT7_REVERSED("StaticExit::8F8930", 0x008F8930, &staticinit::Fn8F8930);
STATICINIT7_REVERSED("StaticExit::8F8960", 0x008F8960, &staticinit::Fn8F8960);
STATICINIT7_REVERSED("StaticExit::8F8990", 0x008F8990, &staticinit::Fn8F8990);
STATICINIT7_REVERSED("StaticExit::8F89C0", 0x008F89C0, &staticinit::Fn8F89C0);
STATICINIT7_REVERSED("StaticExit::8F89F0", 0x008F89F0, &staticinit::Fn8F89F0);
STATICINIT7_REVERSED("StaticExit::8F8A50", 0x008F8A50, &staticinit::Fn8F8A50);
STATICINIT7_REVERSED("StaticExit::8F8A60", 0x008F8A60, &staticinit::Fn8F8A60);
STATICINIT7_REVERSED("StaticExit::8F8AA0", 0x008F8AA0, &staticinit::Fn8F8AA0);
STATICINIT7_REVERSED("StaticExit::8F8AE0", 0x008F8AE0, &staticinit::Fn8F8AE0);
STATICINIT7_REVERSED("StaticExit::8F8B10", 0x008F8B10, &staticinit::Fn8F8B10);
STATICINIT7_REVERSED("StaticExit::8F8B20", 0x008F8B20, &staticinit::Fn8F8B20);
STATICINIT7_REVERSED("StaticExit::8F8B50", 0x008F8B50, &staticinit::Fn8F8B50);
STATICINIT7_REVERSED("StaticExit::8F8B80", 0x008F8B80, &staticinit::Fn8F8B80);
STATICINIT7_REVERSED("StaticExit::8F8BB0", 0x008F8BB0, &staticinit::Fn8F8BB0);
STATICINIT7_REVERSED("StaticExit::8F8BE0", 0x008F8BE0, &staticinit::Fn8F8BE0);
STATICINIT7_REVERSED("StaticExit::8F8C10", 0x008F8C10, &staticinit::Fn8F8C10);
STATICINIT7_REVERSED("StaticExit::8F8C20", 0x008F8C20, &staticinit::Fn8F8C20);
STATICINIT7_REVERSED("StaticExit::8F8C30", 0x008F8C30, &staticinit::Fn8F8C30);
STATICINIT7_REVERSED("StaticExit::8F8C40", 0x008F8C40, &staticinit::Fn8F8C40);
STATICINIT7_REVERSED("StaticExit::8F8C80", 0x008F8C80, &staticinit::Fn8F8C80);
STATICINIT7_REVERSED("StaticExit::8F8CC0", 0x008F8CC0, &staticinit::Fn8F8CC0);
STATICINIT7_REVERSED("StaticExit::8F8CF0", 0x008F8CF0, &staticinit::Fn8F8CF0);
STATICINIT7_REVERSED("StaticExit::8F8D30", 0x008F8D30, &staticinit::Fn8F8D30);
STATICINIT7_REVERSED("StaticExit::8F8D70", 0x008F8D70, &staticinit::Fn8F8D70);
STATICINIT7_REVERSED("StaticExit::8F8DB0", 0x008F8DB0, &staticinit::Fn8F8DB0);
STATICINIT7_REVERSED("StaticExit::8F8DF0", 0x008F8DF0, &staticinit::Fn8F8DF0);
STATICINIT7_REVERSED("StaticExit::8F8E30", 0x008F8E30, &staticinit::Fn8F8E30);
STATICINIT7_REVERSED("StaticExit::8F8E70", 0x008F8E70, &staticinit::Fn8F8E70);
STATICINIT7_REVERSED("StaticExit::8F8EB0", 0x008F8EB0, &staticinit::Fn8F8EB0);
STATICINIT7_REVERSED("StaticExit::8F8EF0", 0x008F8EF0, &staticinit::Fn8F8EF0);
STATICINIT7_REVERSED("StaticExit::8F8F30", 0x008F8F30, &staticinit::Fn8F8F30);
STATICINIT7_REVERSED("StaticExit::8F8F70", 0x008F8F70, &staticinit::Fn8F8F70);
STATICINIT7_REVERSED("StaticExit::8F8FB0", 0x008F8FB0, &staticinit::Fn8F8FB0);
STATICINIT7_REVERSED("StaticExit::8F8FF0", 0x008F8FF0, &staticinit::Fn8F8FF0);
STATICINIT7_REVERSED("StaticExit::8F9030", 0x008F9030, &staticinit::Fn8F9030);
STATICINIT7_REVERSED("StaticExit::8F9070", 0x008F9070, &staticinit::Fn8F9070);
STATICINIT7_REVERSED("StaticExit::8F90B0", 0x008F90B0, &staticinit::Fn8F90B0);
STATICINIT7_REVERSED("StaticExit::8F90F0", 0x008F90F0, &staticinit::Fn8F90F0);
STATICINIT7_REVERSED("StaticExit::8F9130", 0x008F9130, &staticinit::Fn8F9130);
STATICINIT7_REVERSED("StaticExit::8F9170", 0x008F9170, &staticinit::Fn8F9170);
STATICINIT7_REVERSED("StaticExit::8F91B0", 0x008F91B0, &staticinit::Fn8F91B0);
STATICINIT7_REVERSED("StaticExit::8F91F0", 0x008F91F0, &staticinit::Fn8F91F0);
STATICINIT7_REVERSED("StaticExit::8F9230", 0x008F9230, &staticinit::Fn8F9230);
STATICINIT7_REVERSED("StaticExit::8F9270", 0x008F9270, &staticinit::Fn8F9270);
STATICINIT7_REVERSED("StaticExit::8F92B0", 0x008F92B0, &staticinit::Fn8F92B0);
STATICINIT7_REVERSED("StaticExit::8F92F0", 0x008F92F0, &staticinit::Fn8F92F0);
STATICINIT7_REVERSED("StaticExit::8F9330", 0x008F9330, &staticinit::Fn8F9330);
STATICINIT7_REVERSED("StaticExit::8F9370", 0x008F9370, &staticinit::Fn8F9370);
STATICINIT7_REVERSED("StaticExit::8F93B0", 0x008F93B0, &staticinit::Fn8F93B0);
STATICINIT7_REVERSED("StaticExit::8F93F0", 0x008F93F0, &staticinit::Fn8F93F0);
STATICINIT7_REVERSED("StaticExit::8F9430", 0x008F9430, &staticinit::Fn8F9430);
STATICINIT7_REVERSED("StaticExit::8F9470", 0x008F9470, &staticinit::Fn8F9470);
STATICINIT7_REVERSED("StaticExit::8F94B0", 0x008F94B0, &staticinit::Fn8F94B0);
STATICINIT7_REVERSED("StaticExit::8F94F0", 0x008F94F0, &staticinit::Fn8F94F0);
STATICINIT7_REVERSED("StaticExit::8F9530", 0x008F9530, &staticinit::Fn8F9530);
STATICINIT7_REVERSED("StaticExit::8F9570", 0x008F9570, &staticinit::Fn8F9570);
STATICINIT7_REVERSED("StaticExit::8F95B0", 0x008F95B0, &staticinit::Fn8F95B0);
STATICINIT7_REVERSED("StaticExit::8F95F0", 0x008F95F0, &staticinit::Fn8F95F0);
STATICINIT7_REVERSED("StaticExit::8F9630", 0x008F9630, &staticinit::Fn8F9630);
STATICINIT7_REVERSED("StaticExit::8F9670", 0x008F9670, &staticinit::Fn8F9670);
STATICINIT7_REVERSED("StaticExit::8F96B0", 0x008F96B0, &staticinit::Fn8F96B0);
STATICINIT7_REVERSED("StaticExit::8F96F0", 0x008F96F0, &staticinit::Fn8F96F0);
STATICINIT7_REVERSED("StaticExit::8F9730", 0x008F9730, &staticinit::Fn8F9730);
STATICINIT7_REVERSED("StaticExit::8F9770", 0x008F9770, &staticinit::Fn8F9770);
STATICINIT7_REVERSED("StaticExit::8F97B0", 0x008F97B0, &staticinit::Fn8F97B0);
STATICINIT7_REVERSED("StaticExit::8F97F0", 0x008F97F0, &staticinit::Fn8F97F0);
STATICINIT7_REVERSED("StaticExit::8F9830", 0x008F9830, &staticinit::Fn8F9830);
STATICINIT7_REVERSED("StaticExit::8F9870", 0x008F9870, &staticinit::Fn8F9870);
STATICINIT7_REVERSED("StaticExit::8F98B0", 0x008F98B0, &staticinit::Fn8F98B0);
STATICINIT7_REVERSED("StaticExit::8F98F0", 0x008F98F0, &staticinit::Fn8F98F0);
STATICINIT7_REVERSED("StaticExit::8F9930", 0x008F9930, &staticinit::Fn8F9930);
STATICINIT7_REVERSED("StaticExit::8F9970", 0x008F9970, &staticinit::Fn8F9970);
STATICINIT7_REVERSED("StaticExit::8F99B0", 0x008F99B0, &staticinit::Fn8F99B0);
STATICINIT7_REVERSED("StaticExit::8F99F0", 0x008F99F0, &staticinit::Fn8F99F0);
STATICINIT7_REVERSED("StaticExit::8F9A30", 0x008F9A30, &staticinit::Fn8F9A30);
STATICINIT7_REVERSED("StaticExit::8F9A70", 0x008F9A70, &staticinit::Fn8F9A70);
STATICINIT7_REVERSED("StaticExit::8F9AB0", 0x008F9AB0, &staticinit::Fn8F9AB0);
STATICINIT7_REVERSED("StaticExit::8F9AF0", 0x008F9AF0, &staticinit::Fn8F9AF0);
STATICINIT7_REVERSED("StaticExit::8F9B30", 0x008F9B30, &staticinit::Fn8F9B30);
STATICINIT7_REVERSED("StaticExit::8F9B70", 0x008F9B70, &staticinit::Fn8F9B70);
STATICINIT7_REVERSED("StaticExit::8F9BB0", 0x008F9BB0, &staticinit::Fn8F9BB0);
STATICINIT7_REVERSED("StaticExit::8F9BF0", 0x008F9BF0, &staticinit::Fn8F9BF0);
STATICINIT7_REVERSED("StaticExit::8F9C30", 0x008F9C30, &staticinit::Fn8F9C30);
STATICINIT7_REVERSED("StaticExit::8F9C70", 0x008F9C70, &staticinit::Fn8F9C70);
STATICINIT7_REVERSED("StaticExit::8F9CB0", 0x008F9CB0, &staticinit::Fn8F9CB0);
STATICINIT7_REVERSED("StaticExit::8F9CF0", 0x008F9CF0, &staticinit::Fn8F9CF0);
STATICINIT7_REVERSED("StaticExit::8F9D30", 0x008F9D30, &staticinit::Fn8F9D30);
STATICINIT7_REVERSED("StaticExit::8F9D70", 0x008F9D70, &staticinit::Fn8F9D70);
STATICINIT7_REVERSED("StaticExit::8F9DB0", 0x008F9DB0, &staticinit::Fn8F9DB0);
STATICINIT7_REVERSED("StaticExit::8F9DF0", 0x008F9DF0, &staticinit::Fn8F9DF0);
STATICINIT7_REVERSED("StaticExit::8F9E30", 0x008F9E30, &staticinit::Fn8F9E30);
STATICINIT7_REVERSED("StaticExit::8F9E70", 0x008F9E70, &staticinit::Fn8F9E70);
STATICINIT7_REVERSED("StaticExit::8F9EB0", 0x008F9EB0, &staticinit::Fn8F9EB0);
STATICINIT7_REVERSED("StaticExit::8F9EF0", 0x008F9EF0, &staticinit::Fn8F9EF0);
STATICINIT7_REVERSED("StaticExit::8F9F30", 0x008F9F30, &staticinit::Fn8F9F30);
STATICINIT7_REVERSED("StaticExit::8F9F70", 0x008F9F70, &staticinit::Fn8F9F70);
STATICINIT7_REVERSED("StaticExit::8F9FB0", 0x008F9FB0, &staticinit::Fn8F9FB0);
STATICINIT7_REVERSED("StaticExit::8F9FF0", 0x008F9FF0, &staticinit::Fn8F9FF0);
STATICINIT7_REVERSED("StaticExit::8FA030", 0x008FA030, &staticinit::Fn8FA030);
STATICINIT7_REVERSED("StaticExit::8FA070", 0x008FA070, &staticinit::Fn8FA070);
STATICINIT7_REVERSED("StaticExit::8FA0B0", 0x008FA0B0, &staticinit::Fn8FA0B0);
STATICINIT7_REVERSED("StaticExit::8FA0F0", 0x008FA0F0, &staticinit::Fn8FA0F0);
STATICINIT7_REVERSED("StaticExit::8FA130", 0x008FA130, &staticinit::Fn8FA130);
STATICINIT7_REVERSED("StaticExit::8FA170", 0x008FA170, &staticinit::Fn8FA170);
STATICINIT7_REVERSED("StaticExit::8FA1B0", 0x008FA1B0, &staticinit::Fn8FA1B0);
STATICINIT7_REVERSED("StaticExit::8FA1F0", 0x008FA1F0, &staticinit::Fn8FA1F0);
STATICINIT7_REVERSED("StaticExit::8FA230", 0x008FA230, &staticinit::Fn8FA230);
STATICINIT7_REVERSED("StaticExit::8FA270", 0x008FA270, &staticinit::Fn8FA270);
STATICINIT7_REVERSED("StaticExit::8FA2B0", 0x008FA2B0, &staticinit::Fn8FA2B0);
STATICINIT7_REVERSED("StaticExit::8FA2F0", 0x008FA2F0, &staticinit::Fn8FA2F0);
STATICINIT7_REVERSED("StaticExit::8FA330", 0x008FA330, &staticinit::Fn8FA330);
STATICINIT7_REVERSED("StaticExit::8FA370", 0x008FA370, &staticinit::Fn8FA370);
STATICINIT7_REVERSED("StaticExit::8FA3B0", 0x008FA3B0, &staticinit::Fn8FA3B0);
STATICINIT7_REVERSED("StaticExit::8FA3F0", 0x008FA3F0, &staticinit::Fn8FA3F0);
STATICINIT7_REVERSED("StaticExit::8FA430", 0x008FA430, &staticinit::Fn8FA430);
STATICINIT7_REVERSED("StaticExit::8FA470", 0x008FA470, &staticinit::Fn8FA470);
STATICINIT7_REVERSED("StaticExit::8FA4B0", 0x008FA4B0, &staticinit::Fn8FA4B0);
STATICINIT7_REVERSED("StaticExit::8FA4F0", 0x008FA4F0, &staticinit::Fn8FA4F0);
STATICINIT7_REVERSED("StaticExit::8FA530", 0x008FA530, &staticinit::Fn8FA530);
STATICINIT7_REVERSED("StaticExit::8FA570", 0x008FA570, &staticinit::Fn8FA570);
STATICINIT7_REVERSED("StaticExit::8FA5B0", 0x008FA5B0, &staticinit::Fn8FA5B0);
STATICINIT7_REVERSED("StaticExit::8FA5F0", 0x008FA5F0, &staticinit::Fn8FA5F0);
STATICINIT7_REVERSED("StaticExit::8FA630", 0x008FA630, &staticinit::Fn8FA630);
STATICINIT7_REVERSED("StaticExit::8FA670", 0x008FA670, &staticinit::Fn8FA670);
STATICINIT7_REVERSED("StaticExit::8FA6B0", 0x008FA6B0, &staticinit::Fn8FA6B0);
STATICINIT7_REVERSED("StaticExit::8FA6F0", 0x008FA6F0, &staticinit::Fn8FA6F0);
STATICINIT7_REVERSED("StaticExit::8FA730", 0x008FA730, &staticinit::Fn8FA730);
STATICINIT7_REVERSED("StaticExit::8FA770", 0x008FA770, &staticinit::Fn8FA770);
STATICINIT7_REVERSED("StaticExit::8FA7B0", 0x008FA7B0, &staticinit::Fn8FA7B0);
STATICINIT7_REVERSED("StaticExit::8FA7F0", 0x008FA7F0, &staticinit::Fn8FA7F0);
STATICINIT7_REVERSED("StaticExit::8FA830", 0x008FA830, &staticinit::Fn8FA830);
STATICINIT7_REVERSED("StaticExit::8FA870", 0x008FA870, &staticinit::Fn8FA870);
STATICINIT7_REVERSED("StaticExit::8FA8B0", 0x008FA8B0, &staticinit::Fn8FA8B0);
STATICINIT7_REVERSED("StaticExit::8FA8F0", 0x008FA8F0, &staticinit::Fn8FA8F0);
STATICINIT7_REVERSED("StaticExit::8FA930", 0x008FA930, &staticinit::Fn8FA930);
STATICINIT7_REVERSED("StaticExit::8FA970", 0x008FA970, &staticinit::Fn8FA970);
STATICINIT7_REVERSED("StaticExit::8FA9B0", 0x008FA9B0, &staticinit::Fn8FA9B0);
STATICINIT7_REVERSED("StaticExit::8FA9F0", 0x008FA9F0, &staticinit::Fn8FA9F0);
STATICINIT7_REVERSED("StaticExit::8FAA30", 0x008FAA30, &staticinit::Fn8FAA30);
STATICINIT7_REVERSED("StaticExit::8FAA70", 0x008FAA70, &staticinit::Fn8FAA70);
STATICINIT7_REVERSED("StaticExit::8FAAB0", 0x008FAAB0, &staticinit::Fn8FAAB0);
STATICINIT7_REVERSED("StaticExit::8FAAF0", 0x008FAAF0, &staticinit::Fn8FAAF0);
STATICINIT7_REVERSED("StaticExit::8FAB30", 0x008FAB30, &staticinit::Fn8FAB30);
STATICINIT7_REVERSED("StaticExit::8FAB70", 0x008FAB70, &staticinit::Fn8FAB70);
STATICINIT7_REVERSED("StaticExit::8FABB0", 0x008FABB0, &staticinit::Fn8FABB0);
STATICINIT7_REVERSED("StaticExit::8FABF0", 0x008FABF0, &staticinit::Fn8FABF0);
STATICINIT7_REVERSED("StaticExit::8FAC30", 0x008FAC30, &staticinit::Fn8FAC30);
STATICINIT7_REVERSED("StaticExit::8FAC70", 0x008FAC70, &staticinit::Fn8FAC70);
STATICINIT7_REVERSED("StaticExit::8FACB0", 0x008FACB0, &staticinit::Fn8FACB0);
STATICINIT7_REVERSED("StaticExit::8FACF0", 0x008FACF0, &staticinit::Fn8FACF0);
STATICINIT7_REVERSED("StaticExit::8FAD30", 0x008FAD30, &staticinit::Fn8FAD30);
STATICINIT7_REVERSED("StaticExit::8FAD70", 0x008FAD70, &staticinit::Fn8FAD70);
STATICINIT7_REVERSED("StaticExit::8FADB0", 0x008FADB0, &staticinit::Fn8FADB0);
STATICINIT7_REVERSED("StaticExit::8FADF0", 0x008FADF0, &staticinit::Fn8FADF0);
STATICINIT7_REVERSED("StaticExit::8FAE30", 0x008FAE30, &staticinit::Fn8FAE30);
STATICINIT7_REVERSED("StaticExit::8FAE70", 0x008FAE70, &staticinit::Fn8FAE70);
STATICINIT7_REVERSED("StaticExit::8FAEB0", 0x008FAEB0, &staticinit::Fn8FAEB0);
STATICINIT7_REVERSED("StaticExit::8FAEF0", 0x008FAEF0, &staticinit::Fn8FAEF0);
STATICINIT7_REVERSED("StaticExit::8FAF30", 0x008FAF30, &staticinit::Fn8FAF30);
STATICINIT7_REVERSED("StaticExit::8FAF70", 0x008FAF70, &staticinit::Fn8FAF70);
STATICINIT7_REVERSED("StaticExit::8FAFB0", 0x008FAFB0, &staticinit::Fn8FAFB0);
STATICINIT7_REVERSED("StaticExit::8FAFF0", 0x008FAFF0, &staticinit::Fn8FAFF0);
STATICINIT7_REVERSED("StaticExit::8FB030", 0x008FB030, &staticinit::Fn8FB030);
STATICINIT7_REVERSED("StaticExit::8FB070", 0x008FB070, &staticinit::Fn8FB070);
STATICINIT7_REVERSED("StaticExit::8FB0B0", 0x008FB0B0, &staticinit::Fn8FB0B0);
STATICINIT7_REVERSED("StaticExit::8FB0F0", 0x008FB0F0, &staticinit::Fn8FB0F0);
STATICINIT7_REVERSED("StaticExit::8FB130", 0x008FB130, &staticinit::Fn8FB130);
STATICINIT7_REVERSED("StaticExit::8FB170", 0x008FB170, &staticinit::Fn8FB170);
STATICINIT7_REVERSED("StaticExit::8FB1B0", 0x008FB1B0, &staticinit::Fn8FB1B0);
STATICINIT7_REVERSED("StaticExit::8FB1F0", 0x008FB1F0, &staticinit::Fn8FB1F0);
STATICINIT7_REVERSED("StaticExit::8FB230", 0x008FB230, &staticinit::Fn8FB230);
STATICINIT7_REVERSED("StaticExit::8FB270", 0x008FB270, &staticinit::Fn8FB270);
STATICINIT7_REVERSED("StaticExit::8FB2B0", 0x008FB2B0, &staticinit::Fn8FB2B0);
STATICINIT7_REVERSED("StaticExit::8FB2F0", 0x008FB2F0, &staticinit::Fn8FB2F0);
STATICINIT7_REVERSED("StaticExit::8FB330", 0x008FB330, &staticinit::Fn8FB330);
STATICINIT7_REVERSED("StaticExit::8FB370", 0x008FB370, &staticinit::Fn8FB370);
STATICINIT7_REVERSED("StaticExit::8FB3B0", 0x008FB3B0, &staticinit::Fn8FB3B0);
STATICINIT7_REVERSED("StaticExit::8FB3F0", 0x008FB3F0, &staticinit::Fn8FB3F0);
STATICINIT7_REVERSED("StaticExit::8FB430", 0x008FB430, &staticinit::Fn8FB430);
STATICINIT7_REVERSED("StaticExit::8FB470", 0x008FB470, &staticinit::Fn8FB470);
STATICINIT7_REVERSED("StaticExit::8FB4B0", 0x008FB4B0, &staticinit::Fn8FB4B0);
STATICINIT7_REVERSED("StaticExit::8FB4F0", 0x008FB4F0, &staticinit::Fn8FB4F0);
STATICINIT7_REVERSED("StaticExit::8FB530", 0x008FB530, &staticinit::Fn8FB530);
STATICINIT7_REVERSED("StaticExit::8FB570", 0x008FB570, &staticinit::Fn8FB570);
STATICINIT7_REVERSED("StaticExit::8FB5B0", 0x008FB5B0, &staticinit::Fn8FB5B0);
STATICINIT7_REVERSED("StaticExit::8FB5F0", 0x008FB5F0, &staticinit::Fn8FB5F0);
STATICINIT7_REVERSED("StaticExit::8FB630", 0x008FB630, &staticinit::Fn8FB630);
STATICINIT7_REVERSED("StaticExit::8FB670", 0x008FB670, &staticinit::Fn8FB670);
STATICINIT7_REVERSED("StaticExit::8FB6B0", 0x008FB6B0, &staticinit::Fn8FB6B0);
STATICINIT7_REVERSED("StaticExit::8FB6F0", 0x008FB6F0, &staticinit::Fn8FB6F0);
STATICINIT7_REVERSED("StaticExit::8FB730", 0x008FB730, &staticinit::Fn8FB730);
STATICINIT7_REVERSED("StaticExit::8FB770", 0x008FB770, &staticinit::Fn8FB770);
STATICINIT7_REVERSED("StaticExit::8FB7B0", 0x008FB7B0, &staticinit::Fn8FB7B0);
STATICINIT7_REVERSED("StaticExit::8FB7F0", 0x008FB7F0, &staticinit::Fn8FB7F0);
STATICINIT7_REVERSED("StaticExit::8FB830", 0x008FB830, &staticinit::Fn8FB830);
STATICINIT7_REVERSED("StaticExit::8FB840", 0x008FB840, &staticinit::Fn8FB840);
STATICINIT7_REVERSED("StaticExit::8FB860", 0x008FB860, &staticinit::Fn8FB860);
STATICINIT7_REVERSED("StaticExit::8FB870", 0x008FB870, &staticinit::Fn8FB870);
STATICINIT7_REVERSED("StaticExit::8FB8B0", 0x008FB8B0, &staticinit::Fn8FB8B0);
STATICINIT7_REVERSED("StaticExit::8FB8C0", 0x008FB8C0, &staticinit::Fn8FB8C0);
STATICINIT7_REVERSED("StaticExit::8FB8E0", 0x008FB8E0, &staticinit::Fn8FB8E0);
STATICINIT7_REVERSED("StaticExit::8FB8F0", 0x008FB8F0, &staticinit::Fn8FB8F0);
STATICINIT7_REVERSED("StaticExit::8FB900", 0x008FB900, &staticinit::Fn8FB900);
STATICINIT7_REVERSED("StaticExit::8FB910", 0x008FB910, &staticinit::Fn8FB910);
STATICINIT7_REVERSED("StaticExit::8FB950", 0x008FB950, &staticinit::Fn8FB950);
