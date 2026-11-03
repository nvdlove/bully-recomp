// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8D9C00 (initializer)
void __cdecl Fn8D9C00() {
    uint32_t r1 = Stdcall(0x007F8580);
    Dword(0x00BCBB58) = r1;
}

// 0x8D9C10 (initializer)
void __cdecl Fn8D9C10() {
    uint32_t r1 = Stdcall(0x007F8A80);
    Dword(0x00BCBB5C) = r1;
}

// 0x8D9C20 (initializer)
void __cdecl Fn8D9C20() {
    uint32_t r1 = Stdcall(0x007F91E0);
    Dword(0x00BCBB60) = r1;
}

// 0x8D9C30 (initializer)
void __cdecl Fn8D9C30() {
    uint32_t r1 = Stdcall(0x007F9D60);
    Dword(0x00BCBB64) = r1;
}

// 0x8D9C40 (initializer)
void __cdecl Fn8D9C40() {
    uint32_t r1 = Stdcall(0x007FA3F0);
    Dword(0x00BCBB68) = r1;
}

// 0x8D9C50 (initializer)
void __cdecl Fn8D9C50() {
    uint32_t r1 = Stdcall(0x007FA8A0);
    Dword(0x00BCBB6C) = r1;
}

// 0x8D9C60 (initializer)
void __cdecl Fn8D9C60() {
    uint32_t r1 = Stdcall(0x007FAC50);
    Dword(0x00BCBB70) = r1;
}

// 0x8D9C70 (initializer)
void __cdecl Fn8D9C70() {
    uint32_t r1 = Stdcall(0x007FB110);
    Dword(0x00BCBB74) = r1;
}

// 0x8D9C80 (initializer)
void __cdecl Fn8D9C80() {
    uint32_t r1 = Stdcall(0x007FB850);
    Dword(0x00BCBB78) = r1;
}

// 0x8D9C90 (initializer)
void __cdecl Fn8D9C90() {
    uint32_t r1 = Stdcall(0x007FBC70);
    Dword(0x00BCBB7C) = r1;
}

// 0x8D9CA0 (initializer)
void __cdecl Fn8D9CA0() {
    uint32_t r1 = Stdcall(0x007FBF60);
    Dword(0x00BCBB80) = r1;
}

// 0x8D9CB0 (initializer)
void __cdecl Fn8D9CB0() {
    uint32_t r1 = Stdcall(0x007FC2F0);
    Dword(0x00BCBB84) = r1;
}

// 0x8D9CC0 (initializer)
void __cdecl Fn8D9CC0() {
    uint32_t r1 = Stdcall(0x007FC5B0);
    Dword(0x00BCBB88) = r1;
}

// 0x8D9CD0 (initializer)
void __cdecl Fn8D9CD0() {
    uint32_t r1 = Stdcall(0x007FD3F0);
    Dword(0x00BCBB8C) = r1;
}

// 0x8D9CE0 (initializer)
void __cdecl Fn8D9CE0() {
    uint32_t r1 = Stdcall(0x007FDC10);
    Dword(0x00BCBB90) = r1;
}

// 0x8D9CF0 (initializer)
void __cdecl Fn8D9CF0() {
    uint32_t r1 = Stdcall(0x007FE300);
    Dword(0x00BCBB94) = r1;
}

// 0x8D9D00 (initializer)
void __cdecl Fn8D9D00() {
    uint32_t r1 = Stdcall(0x007FE580);
    Dword(0x00BCBB98) = r1;
}

// 0x8D9D10 (initializer)
void __cdecl Fn8D9D10() {
    uint32_t r1 = Stdcall(0x007FE790);
    Dword(0x00BCBB9C) = r1;
}

// 0x8D9D20 (initializer)
void __cdecl Fn8D9D20() {
    uint32_t r1 = Stdcall(0x007FE970);
    Dword(0x00BCBBA0) = r1;
}

// 0x8D9D30 (initializer)
void __cdecl Fn8D9D30() {
    uint32_t r1 = Stdcall(0x007FECC0);
    Dword(0x00BCBBA4) = r1;
}

// 0x8D9D40 (initializer)
void __cdecl Fn8D9D40() {
    uint32_t r1 = Stdcall(0x007FEF60);
    Dword(0x00BCBBA8) = r1;
}

// 0x8D9D50 (initializer)
uint32_t __cdecl Fn8D9D50() {
    return Thiscall(0x007FF690, 0x00BCBB56u);
}

// 0x8D9D60 (initializer)
void __cdecl Fn8D9D60() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1170));
}

// 0x8D9D70 (initializer)
void __cdecl Fn8D9D70() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F11A0));
}

// 0x8D9DB0 (initializer)
void __cdecl Fn8D9DB0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F11D0));
}

// 0x8D9DC0 (initializer)
void __cdecl Fn8D9DC0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1200));
}

// 0x8D9DD0 (initializer)
void __cdecl Fn8D9DD0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1230));
}

// 0x8D9DE0 (initializer)
void __cdecl Fn8D9DE0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1260));
}

// 0x8D9DF0 (initializer)
void __cdecl Fn8D9DF0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1290));
}

// 0x8D9E00 (initializer)
void __cdecl Fn8D9E00() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F12C0));
}

// 0x8D9E10 (initializer)
void __cdecl Fn8D9E10() {
    Thiscall(0x00765410, 0x00BCBBCCu, 0x008FF7C4u /* "BullyNiDX9Renderer" */, 0x020E6D18u);  // NiRTTI::NiRTTI
}

// 0x8D9E40 (initializer)
void __cdecl Fn8D9E40() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F12F0));
}

// 0x8D9E50 (initializer)
void __cdecl Fn8D9E50() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1300));
}

// 0x8D9E60 (initializer)
void __cdecl Fn8D9E60() {
    Dword(0x00BCC490) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1330));
}

// 0x8D9E80 (initializer)
void __cdecl Fn8D9E80() {
    Cdecl(0x00411620, 0x00BCC490u, 0x009005BCu /* "BullySprite2d" */);
}

// 0x8D9EA0 (initializer)
void __cdecl Fn8D9EA0() {
    Dword(0x00BCC498) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1370));
}

// 0x8D9EC0 (initializer)
void __cdecl Fn8D9EC0() {
    Cdecl(0x00411620, 0x00BCC498u, 0x009005CCu /* "BullySprite2dNoTex" */);
}

// 0x8D9EE0 (initializer)
void __cdecl Fn8D9EE0() {
    Dword(0x00BCC49C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F13B0));
}

// 0x8D9F00 (initializer)
void __cdecl Fn8D9F00() {
    Cdecl(0x00411620, 0x00BCC49Cu, 0x009005E0u /* "RwTexRaster" */);
}

// 0x8D9F20 (initializer)
void __cdecl Fn8D9F20() {
    Dword(0x00BCC4A0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F13F0));
}

// 0x8D9F40 (initializer)
void __cdecl Fn8D9F40() {
    Cdecl(0x00411620, 0x00BCC4A0u, 0x009005ECu /* "BullySky" */);
}

// 0x8D9F60 (initializer)
void __cdecl Fn8D9F60() {
    Dword(0x00BCC4A4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1430));
}

// 0x8D9F80 (initializer)
void __cdecl Fn8D9F80() {
    Cdecl(0x00411620, 0x00BCC4A4u, 0x009005F8u /* "BullySkyNoTex" */);
}

// 0x8D9FA0 (initializer)
void __cdecl Fn8D9FA0() {
    Dword(0x00BCC4AC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1470));
}

// 0x8D9FC0 (initializer)
void __cdecl Fn8D9FC0() {
    Cdecl(0x00411620, 0x00BCC4ACu, 0x00900608u /* "cMaterial" */);
}

// 0x8D9FE0 (initializer)
void __cdecl Fn8D9FE0() {
    Dword(0x00BCC4B0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F14B0));
}

// 0x8DA000 (initializer)
void __cdecl Fn8DA000() {
    Cdecl(0x00411620, 0x00BCC4B0u, 0x00900614u /* "BullyRwIm3D" */);
}

// 0x8DA020 (initializer)
void __cdecl Fn8DA020() {
    Dword(0x00BCC4B4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F14F0));
}

// 0x8DA040 (initializer)
void __cdecl Fn8DA040() {
    Cdecl(0x00411620, 0x00BCC4B4u, 0x00900620u /* "BullyRwIm3DNoTex" */);
}

// 0x8DA060 (initializer)
void __cdecl Fn8DA060() {
    Dword(0x00BCC4B8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1530));
}

// 0x8DA080 (initializer)
void __cdecl Fn8DA080() {
    Cdecl(0x00411620, 0x00BCC4B8u, 0x00900634u /* "MdMaterialDescriptor" */);
}

// 0x8DA0A0 (initializer)
void __cdecl Fn8DA0A0() {
    Dword(0x00BCC4C0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1570));
}

// 0x8DA0C0 (initializer)
void __cdecl Fn8DA0C0() {
    Cdecl(0x00411620, 0x00BCC4C0u, 0x0090064Cu /* "cFog" */);
}

// 0x8DA0E0 (initializer)
void __cdecl Fn8DA0E0() {
    Dword(0x00BCC4C4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F15B0));
}

// 0x8DA100 (initializer)
void __cdecl Fn8DA100() {
    Cdecl(0x00411620, 0x00BCC4C4u, 0x00900654u /* "vFogNearFar" */);
}

// 0x8DA120 (initializer)
void __cdecl Fn8DA120() {
    Dword(0x00BCC4C8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F15F0));
}

// 0x8DA140 (initializer)
void __cdecl Fn8DA140() {
    Cdecl(0x00411620, 0x00BCC4C8u, 0x00900660u /* "BoneIdx" */);
}

// 0x8DA160 (initializer)
void __cdecl Fn8DA160() {
    Dword(0x00BCC4CC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1630));
}

// 0x8DA180 (initializer)
void __cdecl Fn8DA180() {
    Cdecl(0x00411620, 0x00BCC4CCu, 0x00900668u /* "fAlpha_Material" */);
}

// 0x8DA1A0 (initializer)
void __cdecl Fn8DA1A0() {
    Dword(0x00BCC4D4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1670));
}

// 0x8DA1C0 (initializer)
void __cdecl Fn8DA1C0() {
    Cdecl(0x00411620, 0x00BCC4D4u, 0x00900678u /* "bHadAlpha" */);
}

// 0x8DA1E0 (initializer)
void __cdecl Fn8DA1E0() {
    Dword(0x00BCC4D8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F16B0));
}

// 0x8DA200 (initializer)
void __cdecl Fn8DA200() {
    Cdecl(0x00411620, 0x00BCC4D8u, 0x00900684u /* "bAlphaBlend" */);
}

// 0x8DA220 (initializer)
void __cdecl Fn8DA220() {
    Dword(0x00BCC4DC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F16F0));
}

// 0x8DA240 (initializer)
void __cdecl Fn8DA240() {
    Cdecl(0x00411620, 0x00BCC4DCu, 0x00900690u /* "iSrcBlend" */);
}

// 0x8DA260 (initializer)
void __cdecl Fn8DA260() {
    Dword(0x00BCC4E0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1730));
}

// 0x8DA280 (initializer)
void __cdecl Fn8DA280() {
    Cdecl(0x00411620, 0x00BCC4E0u, 0x0090069Cu /* "iDestBlend" */);
}

// 0x8DA2A0 (initializer)
void __cdecl Fn8DA2A0() {
    Dword(0x00BCC4E8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1770));
}

// 0x8DA2C0 (initializer)
void __cdecl Fn8DA2C0() {
    Cdecl(0x00411620, 0x00BCC4E8u, 0x009006A8u /* "bAlphaTest" */);
}

// 0x8DA2E0 (initializer)
void __cdecl Fn8DA2E0() {
    Dword(0x00BCC4EC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F17B0));
}

// 0x8DA300 (initializer)
void __cdecl Fn8DA300() {
    Cdecl(0x00411620, 0x00BCC4ECu, 0x009006B4u /* "iTestMode" */);
}

// 0x8DA320 (initializer)
void __cdecl Fn8DA320() {
    Dword(0x00BCC4F0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F17F0));
}

// 0x8DA340 (initializer)
void __cdecl Fn8DA340() {
    Cdecl(0x00411620, 0x00BCC4F0u, 0x009006C0u /* "iTestRef" */);
}

// 0x8DA360 (initializer)
void __cdecl Fn8DA360() {
    Dword(0x00BCC4F4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1830));
}

// 0x8DA380 (initializer)
void __cdecl Fn8DA380() {
    Cdecl(0x00411620, 0x00BCC4F4u, 0x009006CCu /* "Zbias" */);
}

// 0x8DA3A0 (initializer)
void __cdecl Fn8DA3A0() {
    Dword(0x00BCC4FC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1870));
}

// 0x8DA3C0 (initializer)
void __cdecl Fn8DA3C0() {
    Cdecl(0x00411620, 0x00BCC4FCu, 0x009006D4u /* "fNBias" */);
}

// 0x8DA3E0 (initializer)
void __cdecl Fn8DA3E0() {
    Dword(0x00BCC500) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F18B0));
}

// 0x8DA400 (initializer)
void __cdecl Fn8DA400() {
    Cdecl(0x00411620, 0x00BCC500u, 0x009006DCu /* "UserPropBuffer" */);
}

// 0x8DA420 (initializer)
void __cdecl Fn8DA420() {
    Dword(0x00BCC504) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F18F0));
}

// 0x8DA440 (initializer)
void __cdecl Fn8DA440() {
    Cdecl(0x00411620, 0x00BCC504u, 0x009006ECu /* "cBeamColor" */);
}

// 0x8DA460 (initializer)
void __cdecl Fn8DA460() {
    Dword(0x00BCC508) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1930));
}

// 0x8DA480 (initializer)
void __cdecl Fn8DA480() {
    Cdecl(0x00411620, 0x00BCC508u, 0x009006F8u /* "BullyBeam" */);
}

// 0x8DA4A0 (initializer)
void __cdecl Fn8DA4A0() {
    Dword(0x00BCC510) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1970));
}

// 0x8DA4C0 (initializer)
void __cdecl Fn8DA4C0() {
    Cdecl(0x00411620, 0x00BCC510u, 0x00900704u /* "BullyMirror" */);
}

// 0x8DA4E0 (initializer)
void __cdecl Fn8DA4E0() {
    Dword(0x00BCC514) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F19B0));
}

// 0x8DA500 (initializer)
void __cdecl Fn8DA500() {
    Cdecl(0x00411620, 0x00BCC514u, 0x00900710u /* "vUVAnim" */);
}

// 0x8DA520 (initializer)
void __cdecl Fn8DA520() {
    Dword(0x00BCC518) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F19F0));
}

// 0x8DA540 (initializer)
void __cdecl Fn8DA540() {
    Cdecl(0x00411620, 0x00BCC518u, 0x00900718u /* "vShadowParams" */);
}

// 0x8DA560 (initializer)
void __cdecl Fn8DA560() {
    Dword(0x00BCC51C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1A30));
}

// 0x8DA580 (initializer)
void __cdecl Fn8DA580() {
    Cdecl(0x00411620, 0x00BCC51Cu, 0x00900728u /* "NoShadowCast" */);
}

// 0x8DA5A0 (initializer)
void __cdecl Fn8DA5A0() {
    Dword(0x00BCC524) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1A70));
}

// 0x8DA5C0 (initializer)
void __cdecl Fn8DA5C0() {
    Cdecl(0x00411620, 0x00BCC524u, 0x00900738u /* "NoCast" */);
}

// 0x8DA5E0 (initializer)
void __cdecl Fn8DA5E0() {
    Dword(0x00BCC528) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1AB0));
}

// 0x8DA600 (initializer)
void __cdecl Fn8DA600() {
    Cdecl(0x00411620, 0x00BCC528u, 0x00900740u /* "RenderClose" */);
}

// 0x8DA620 (initializer)
void __cdecl Fn8DA620() {
    Dword(0x00BCC52C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1AF0));
}

// 0x8DA640 (initializer)
void __cdecl Fn8DA640() {
    Cdecl(0x00411620, 0x00BCC52Cu, 0x0090074Cu /* "bAlphaBlending" */);
}

// 0x8DA660 (initializer)
void __cdecl Fn8DA660() {
    Dword(0x00BCC530) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1B30));
}

// 0x8DA680 (initializer)
void __cdecl Fn8DA680() {
    Cdecl(0x00411620, 0x00BCC530u, 0x0090075Cu /* "iColorWriteEnable" */);
}

// 0x8DA6A0 (initializer)
void __cdecl Fn8DA6A0() {
    Dword(0x00BCC538) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1B70));
}

// 0x8DA6C0 (initializer)
void __cdecl Fn8DA6C0() {
    Cdecl(0x00411620, 0x00BCC538u, 0x00900770u /* "vTime" */);
}

// 0x8DA6E0 (initializer)
void __cdecl Fn8DA6E0() {
    Dword(0x00BCC53C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1BB0));
}

// 0x8DA700 (initializer)
void __cdecl Fn8DA700() {
    Cdecl(0x00411620, 0x00BCC53Cu, 0x00900778u /* "vTimeSinCos" */);
}

// 0x8DA720 (initializer)
void __cdecl Fn8DA720() {
    Dword(0x00BCC540) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1BF0));
}

// 0x8DA740 (initializer)
void __cdecl Fn8DA740() {
    Cdecl(0x00411620, 0x00BCC540u, 0x00900784u /* "cSkyTop" */);
}

// 0x8DA760 (initializer)
void __cdecl Fn8DA760() {
    Dword(0x00BCC544) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1C30));
}

// 0x8DA780 (initializer)
void __cdecl Fn8DA780() {
    Cdecl(0x00411620, 0x00BCC544u, 0x0090078Cu /* "cSkyBottom" */);
}

// 0x8DA7D0 (initializer)
void __cdecl Fn8DA7D0() {
    Float(0x00BCC570) = float((1.0 / double(Float(0x009E4F44))));
}

// 0x8DA7F0 (initializer)
void __cdecl Fn8DA7F0() {
    Float(0x00BCC574) = float((1.0 / (double(Float(0x009E4F44)) * double(Float(0x009E4F44)))));
}

// 0x8DA810 (initializer)
void __cdecl Fn8DA810() {
    Float(0x00BCC578) = float((double(Float(0x00BCC574)) * Double(0x00900140)));
}

// 0x8DA830 (initializer)
void __cdecl Fn8DA830() {
    Float(0x00BCC57C) = float((Double(0x00900AD8) / double(Float(0x009E4F44))));
}

// 0x8DA850 (initializer)
void __cdecl Fn8DA850() {
    Thiscall(0x00415520, 0x00BCC580u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1CA0));
}

// 0x8DA870 (initializer)
void __cdecl Fn8DA870() {
    Float(0x00BCCE70) = float((1.0 / double(Float(0x009E4F74))));
}

// 0x8DA890 (initializer)
void __cdecl Fn8DA890() {
    Float(0x00BCCE74) = float((1.0 / (double(Float(0x009E4F74)) * double(Float(0x009E4F74)))));
}

// 0x8DA8B0 (initializer)
void __cdecl Fn8DA8B0() {
    Float(0x00BCCE78) = float((double(Float(0x00BCCE74)) * Double(0x00900140)));
}

// 0x8DA8D0 (initializer)
void __cdecl Fn8DA8D0() {
    Float(0x00BCCE7C) = float((Double(0x00900AD8) / double(Float(0x009E4F74))));
}

// 0x8DA8F0 (initializer)
void __cdecl Fn8DA8F0() {
    Float(0x00BCD068) = float((1.0 / double(Float(0x009E4F7C))));
}

// 0x8DA910 (initializer)
void __cdecl Fn8DA910() {
    Float(0x00BCD06C) = float((1.0 / (double(Float(0x009E4F7C)) * double(Float(0x009E4F7C)))));
}

// 0x8DA930 (initializer)
void __cdecl Fn8DA930() {
    Float(0x00BCD070) = float((double(Float(0x00BCD06C)) * Double(0x00900140)));
}

// 0x8DA950 (initializer)
void __cdecl Fn8DA950() {
    Float(0x00BCD074) = float((Double(0x00900AD8) / double(Float(0x009E4F7C))));
}

// 0x8DA970 (initializer)
void __cdecl Fn8DA970() {
    Float(0x00BCD084) = float((1.0 / double(Float(0x009E4F94))));
}

// 0x8DA990 (initializer)
void __cdecl Fn8DA990() {
    Float(0x00BCD088) = float((1.0 / (double(Float(0x009E4F94)) * double(Float(0x009E4F94)))));
}

// 0x8DA9B0 (initializer)
void __cdecl Fn8DA9B0() {
    Float(0x00BCD08C) = float((double(Float(0x00BCD088)) * Double(0x00900140)));
}

// 0x8DA9D0 (initializer)
void __cdecl Fn8DA9D0() {
    Float(0x00BCD090) = float((Double(0x00900AD8) / double(Float(0x009E4F94))));
}

// 0x8DA9F0 (initializer)
void __cdecl Fn8DA9F0() {
    Thiscall(0x00424380, 0x00BCD09Cu);  // CharTriggers::ClearList
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1CB0));
}

// 0x8DAA10 (initializer)
void __cdecl Fn8DAA10() {
    Float(0x00BCEB28) = float((1.0 / double(Float(0x009E5150))));
}

// 0x8DAA30 (initializer)
void __cdecl Fn8DAA30() {
    Float(0x00BCEB2C) = float((1.0 / (double(Float(0x009E5150)) * double(Float(0x009E5150)))));
}

// 0x8DAA50 (initializer)
void __cdecl Fn8DAA50() {
    Float(0x00BCEB30) = float((double(Float(0x00BCEB2C)) * Double(0x00900140)));
}

// 0x8DAA70 (initializer)
void __cdecl Fn8DAA70() {
    Float(0x00BCEB34) = float((Double(0x00900AD8) / double(Float(0x009E5150))));
}

// 0x8DAA90 (initializer)
void __cdecl Fn8DAA90() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1CC0));
}

// 0x8DAAA0 (initializer)
void __cdecl Fn8DAAA0() {
    Thiscall(0x004266B0, 0x00BCEB50u);  // CCollectiblesMgr::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1CD0));
}

// 0x8DAAC0 (initializer)
void __cdecl Fn8DAAC0() {
    Float(0x00BD0B68) = float((1.0 / double(Float(0x009E52AC))));
}

// 0x8DAAE0 (initializer)
void __cdecl Fn8DAAE0() {
    Float(0x00BD0B6C) = float((1.0 / (double(Float(0x009E52AC)) * double(Float(0x009E52AC)))));
}

// 0x8DAB00 (initializer)
void __cdecl Fn8DAB00() {
    Float(0x00BD0B70) = float((double(Float(0x00BD0B6C)) * Double(0x00900140)));
}

// 0x8DAB20 (initializer)
void __cdecl Fn8DAB20() {
    Float(0x00BD0B74) = float((Double(0x00900AD8) / double(Float(0x009E52AC))));
}

// 0x8DAB40 (initializer)
void __cdecl Fn8DAB40() {
    Thiscall(0x0042D750, 0x00BD0CF8u);  // CFireManager::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1CF0));
}

// 0x8DAB60 (initializer)
void __cdecl Fn8DAB60() {
    Float(0x00BD0FF8) = float((1.0 / double(Float(0x009F4764))));
}

// 0x8DAB80 (initializer)
void __cdecl Fn8DAB80() {
    Float(0x00BD0FFC) = float((1.0 / (double(Float(0x009F4764)) * double(Float(0x009F4764)))));
}

// 0x8DABA0 (initializer)
void __cdecl Fn8DABA0() {
    Float(0x00BD1000) = float((double(Float(0x00BD0FFC)) * Double(0x00900140)));
}

// 0x8DABC0 (initializer)
void __cdecl Fn8DABC0() {
    Float(0x00BD1004) = float((Double(0x00900AD8) / double(Float(0x009F4764))));
}

// 0x8DAC10 (initializer)
void __cdecl Fn8DAC10() {
    Float(0x00BD1068) = float((1.0 / double(Float(0x009F4788))));
}

// 0x8DAC30 (initializer)
void __cdecl Fn8DAC30() {
    Float(0x00BD106C) = float((1.0 / (double(Float(0x009F4788)) * double(Float(0x009F4788)))));
}

// 0x8DAC50 (initializer)
void __cdecl Fn8DAC50() {
    Float(0x00BD1070) = float((double(Float(0x00BD106C)) * Double(0x00900140)));
}

// 0x8DAC70 (initializer)
void __cdecl Fn8DAC70() {
    Float(0x00BD1074) = float((Double(0x00900AD8) / double(Float(0x009F4788))));
}

// 0x8DAC90 (initializer)
void __cdecl Fn8DAC90() {
    Thiscall(0x004315C0, 0x00BD1078u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1D50));
}

// 0x8DACC0 (initializer)
void __cdecl Fn8DACC0() {
    Float(0x00BD5C68) = float((1.0 / double(Float(0x009F47A4))));
}

// 0x8DACE0 (initializer)
void __cdecl Fn8DACE0() {
    Float(0x00BD5C6C) = float((1.0 / (double(Float(0x009F47A4)) * double(Float(0x009F47A4)))));
}

// 0x8DAD00 (initializer)
void __cdecl Fn8DAD00() {
    Float(0x00BD5C70) = float((double(Float(0x00BD5C6C)) * Double(0x00900140)));
}

// 0x8DAD20 (initializer)
void __cdecl Fn8DAD20() {
    Float(0x00BD5C74) = float((Double(0x00900AD8) / double(Float(0x009F47A4))));
}

// 0x8DAD40 (initializer)
void __cdecl Fn8DAD40() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1D60));
}

// 0x8DAD50 (initializer)
void __cdecl Fn8DAD50() {
    Thiscall(0x00439FB0, 0x00BD5C90u);  // CLeafEffects::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1D80));
}

// 0x8DAD70 (initializer)
void __cdecl Fn8DAD70() {
    Float(0x00BD77A0) = float((1.0 / double(Float(0x00A041A0))));
}

// 0x8DAD90 (initializer)
void __cdecl Fn8DAD90() {
    Float(0x00BD77A4) = float((1.0 / (double(Float(0x00A041A0)) * double(Float(0x00A041A0)))));
}

// 0x8DADB0 (initializer)
void __cdecl Fn8DADB0() {
    Float(0x00BD77A8) = float((double(Float(0x00BD77A4)) * Double(0x00900140)));
}

// 0x8DADD0 (initializer)
void __cdecl Fn8DADD0() {
    Float(0x00BD77AC) = float((Double(0x00900AD8) / double(Float(0x00A041A0))));
}

// 0x8DADF0 (initializer)
void __cdecl Fn8DADF0() {
    Float(0x00BF3850) = float((1.0 / double(Float(0x00A13648))));
}

// 0x8DAE10 (initializer)
void __cdecl Fn8DAE10() {
    Float(0x00BF3854) = float((1.0 / (double(Float(0x00A13648)) * double(Float(0x00A13648)))));
}

// 0x8DAE30 (initializer)
void __cdecl Fn8DAE30() {
    Float(0x00BF3858) = float((double(Float(0x00BF3854)) * Double(0x00900140)));
}

// 0x8DAE50 (initializer)
void __cdecl Fn8DAE50() {
    Float(0x00BF385C) = float((Double(0x00900AD8) / double(Float(0x00A13648))));
}

// 0x8DAE70 (initializer)
void __cdecl Fn8DAE70() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E00));
}

// 0x8DAE80 (initializer)
void __cdecl Fn8DAE80() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1DB0));
}

// 0x8DAE90 (initializer)
void __cdecl Fn8DAE90() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1DC0));
}

// 0x8DAEA0 (initializer)
void __cdecl Fn8DAEA0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1DD0));
}

// 0x8DAEB0 (initializer)
void __cdecl Fn8DAEB0() {
    Thiscall(0x00667DA0, 0x00BF3E08u);  // List_Init
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1DF0));
}

// 0x8DAED0 (initializer)
void __cdecl Fn8DAED0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E10));
}

// 0x8DAEE0 (initializer)
void __cdecl Fn8DAEE0() {
    Float(0x00BF6214) = float((1.0 / double(Float(0x00A144AC))));
}

// 0x8DAF00 (initializer)
void __cdecl Fn8DAF00() {
    Float(0x00BF6218) = float((1.0 / (double(Float(0x00A144AC)) * double(Float(0x00A144AC)))));
}

// 0x8DAF20 (initializer)
void __cdecl Fn8DAF20() {
    Float(0x00BF621C) = float((double(Float(0x00BF6218)) * Double(0x00900140)));
}

// 0x8DAF40 (initializer)
void __cdecl Fn8DAF40() {
    Float(0x00BF6220) = float((Double(0x00900AD8) / double(Float(0x00A144AC))));
}

// 0x8DAF60 (initializer)
void __cdecl Fn8DAF60() {
    Thiscall(0x00440450, 0x00BF6228u);  // CPaths::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E30));
}

// 0x8DAF80 (initializer)
void __cdecl Fn8DAF80() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E20));
}

// 0x8DAFD0 (initializer)
void __cdecl Fn8DAFD0() {
    Thiscall(0x0061A880, 0x00C0F560u);  // Track_Construct
    Dword(0x00C0F560) = 0x009053B4u /* " 8D" */;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E40));
}

// 0x8DAFF0 (initializer)
void __cdecl Fn8DAFF0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1E80));
}

// 0x8DB000 (initializer)
void __cdecl Fn8DB000() {
    Float(0x00C0F5CC) = float((1.0 / double(Float(0x00A14554))));
}

// 0x8DB020 (initializer)
void __cdecl Fn8DB020() {
    Float(0x00C0F5D0) = float((1.0 / (double(Float(0x00A14554)) * double(Float(0x00A14554)))));
}

// 0x8DB040 (initializer)
void __cdecl Fn8DB040() {
    Float(0x00C0F5D4) = float((double(Float(0x00C0F5D0)) * Double(0x00900140)));
}

// 0x8DB060 (initializer)
void __cdecl Fn8DB060() {
    Float(0x00C0F5D8) = float((Double(0x00900AD8) / double(Float(0x00A14554))));
}

// 0x8DB080 (initializer)
void __cdecl Fn8DB080() {
    Float(0x00C0F658) = float((1.0 / double(Float(0x00A145E8))));
}

// 0x8DB0A0 (initializer)
void __cdecl Fn8DB0A0() {
    Float(0x00C0F65C) = float((1.0 / (double(Float(0x00A145E8)) * double(Float(0x00A145E8)))));
}

// 0x8DB0C0 (initializer)
void __cdecl Fn8DB0C0() {
    Float(0x00C0F660) = float((double(Float(0x00C0F65C)) * Double(0x00900140)));
}

// 0x8DB0E0 (initializer)
void __cdecl Fn8DB0E0() {
    Float(0x00C0F664) = float((Double(0x00900AD8) / double(Float(0x00A145E8))));
}

// 0x8DB130 (initializer)
void __cdecl Fn8DB130() {
    Float(0x00C0F790) = float((1.0 / double(Float(0x00A147A8))));
}

// 0x8DB150 (initializer)
void __cdecl Fn8DB150() {
    Float(0x00C0F794) = float((1.0 / (double(Float(0x00A147A8)) * double(Float(0x00A147A8)))));
}

// 0x8DB170 (initializer)
void __cdecl Fn8DB170() {
    Float(0x00C0F798) = float((double(Float(0x00C0F794)) * Double(0x00900140)));
}

// 0x8DB190 (initializer)
void __cdecl Fn8DB190() {
    Float(0x00C0F79C) = float((Double(0x00900AD8) / double(Float(0x00A147A8))));
}

// 0x8DB1B0 (initializer)
void __cdecl Fn8DB1B0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1EA0));
}

// 0x8DB1C0 (initializer)
void __cdecl Fn8DB1C0() {
    Float(0x00C11D44) = float((1.0 / double(Float(0x00A147B0))));
}

// 0x8DB1E0 (initializer)
void __cdecl Fn8DB1E0() {
    Float(0x00C11D48) = float((1.0 / (double(Float(0x00A147B0)) * double(Float(0x00A147B0)))));
}

// 0x8DB200 (initializer)
void __cdecl Fn8DB200() {
    Float(0x00C11D4C) = float((double(Float(0x00C11D48)) * Double(0x00900140)));
}

// 0x8DB220 (initializer)
void __cdecl Fn8DB220() {
    Float(0x00C11D50) = float((Double(0x00900AD8) / double(Float(0x00A147B0))));
}

// 0x8DB240 (initializer)
void __cdecl Fn8DB240() {
    Float(0x00C13F18) = float((1.0 / double(Float(0x00A147D8))));
}

// 0x8DB260 (initializer)
void __cdecl Fn8DB260() {
    Float(0x00C13F1C) = float((1.0 / (double(Float(0x00A147D8)) * double(Float(0x00A147D8)))));
}

// 0x8DB280 (initializer)
void __cdecl Fn8DB280() {
    Float(0x00C13F20) = float((double(Float(0x00C13F1C)) * Double(0x00900140)));
}

// 0x8DB2A0 (initializer)
void __cdecl Fn8DB2A0() {
    Float(0x00C13F24) = float((Double(0x00900AD8) / double(Float(0x00A147D8))));
}

// 0x8DB2C0 (initializer)
void __cdecl Fn8DB2C0() {
    Thiscall(0x00453A60, 0x00C13F70u);  // EffectProxyBank::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1EB0));
}

// 0x8DB2E0 (initializer)
void __cdecl Fn8DB2E0() {
    Thiscall(0x00453C70, 0x00C13FD8u);  // CAreaEffects::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1EC0));
}

// 0x8DB300 (initializer)
void __cdecl Fn8DB300() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1ED0));
}

// 0x8DB310 (initializer)
void __cdecl Fn8DB310() {
    Thiscall(0x004546B0, 0x00C18700u);  // Fn_4546B0
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1EE0));
}

// 0x8DB370 (initializer)
void __cdecl Fn8DB370() {
    Thiscall(0x00456CD0, 0x00C18748u);  // CShop::CShop
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F60));
}

// 0x8DB390 (initializer)
void __cdecl Fn8DB390() {
    Thiscall(0x00510DE0, 0x00C18744u, 0x0000000Fu, 0x0000000Fu, 0x0000000Fu, 0x000000DCu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1EF0));
}

// 0x8DB3C0 (initializer)
void __cdecl Fn8DB3C0() {
    Thiscall(0x00510DE0, 0x00C19224u, 0x000000F9u, 0x000000CFu, 0x0000005Eu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F00));
}

// 0x8DB3F0 (initializer)
void __cdecl Fn8DB3F0() {
    Thiscall(0x00510DE0, 0x00C19228u, 0x000000F9u, 0x000000CFu, 0x0000005Eu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F10));
}

// 0x8DB420 (initializer)
void __cdecl Fn8DB420() {
    Thiscall(0x00510DE0, 0x00C1922Cu, 0x000000A2u, 0x0000007Fu, 0x00000016u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F20));
}

// 0x8DB450 (initializer)
void __cdecl Fn8DB450() {
    Thiscall(0x00510DE0, 0x00C19230u, 0x0000004Bu, 0x0000004Bu, 0x0000004Bu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F30));
}

// 0x8DB480 (initializer)
void __cdecl Fn8DB480() {
    Thiscall(0x00510DE0, 0x00C19234u, 0x00000024u, 0x00000036u, 0x00000088u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F40));
}

// 0x8DB4B0 (initializer)
void __cdecl Fn8DB4B0() {
    Thiscall(0x00510DE0, 0x00C19238u, 0x0000004Bu, 0x0000004Bu, 0x0000004Bu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F50));
}

// 0x8DB4E0 (initializer)
void __cdecl Fn8DB4E0() {
    Float(0x00C1A750) = float((1.0 / double(Float(0x00A14A3C))));
}

// 0x8DB500 (initializer)
void __cdecl Fn8DB500() {
    Float(0x00C1A754) = float((1.0 / (double(Float(0x00A14A3C)) * double(Float(0x00A14A3C)))));
}

// 0x8DB520 (initializer)
void __cdecl Fn8DB520() {
    Float(0x00C1A758) = float((double(Float(0x00C1A754)) * Double(0x00900140)));
}

// 0x8DB540 (initializer)
void __cdecl Fn8DB540() {
    Float(0x00C1A75C) = float((Double(0x00900AD8) / double(Float(0x00A14A3C))));
}

// 0x8DB560 (initializer)
void __cdecl Fn8DB560() {
    Thiscall(0x0066BEA0, 0x00C1A760u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F70));
}

// 0x8DB580 (initializer)
void __cdecl Fn8DB580() {
    Thiscall(0x0066BEA0, 0x00C1A764u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F80));
}

// 0x8DB5A0 (initializer)
void __cdecl Fn8DB5A0() {
    Thiscall(0x0066BEA0, 0x00C1A768u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1F90));
}

// 0x8DB5C0 (initializer)
void __cdecl Fn8DB5C0() {
    Thiscall(0x0066BEA0, 0x00C1A76Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FA0));
}

// 0x8DB5E0 (initializer)
void __cdecl Fn8DB5E0() {
    Thiscall(0x0066BEA0, 0x00C1A770u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FB0));
}

// 0x8DB600 (initializer)
void __cdecl Fn8DB600() {
    Thiscall(0x0066BEA0, 0x00C1A774u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FC0));
}

// 0x8DB620 (initializer)
void __cdecl Fn8DB620() {
    Thiscall(0x0066BEA0, 0x00C1A778u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FD0));
}

// 0x8DB640 (initializer)
void __cdecl Fn8DB640() {
    Thiscall(0x0066BEA0, 0x00C1A77Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FE0));
}

// 0x8DB660 (initializer)
void __cdecl Fn8DB660() {
    Thiscall(0x0066BEA0, 0x00C1A780u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F1FF0));
}

// 0x8DB680 (initializer)
void __cdecl Fn8DB680() {
    Thiscall(0x0066BEA0, 0x00C1A784u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2000));
}

// 0x8DB6A0 (initializer)
void __cdecl Fn8DB6A0() {
    Thiscall(0x0066BEA0, 0x00C1A788u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2010));
}

// 0x8DB6C0 (initializer)
void __cdecl Fn8DB6C0() {
    Thiscall(0x0066BEA0, 0x00C1A78Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2020));
}

// 0x8DB6E0 (initializer)
void __cdecl Fn8DB6E0() {
    Thiscall(0x0066BEA0, 0x00C1A790u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2030));
}

// 0x8DB700 (initializer)
void __cdecl Fn8DB700() {
    Thiscall(0x0066BEA0, 0x00C1A794u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2040));
}

// 0x8DB720 (initializer)
void __cdecl Fn8DB720() {
    Thiscall(0x0066BEA0, 0x00C1A798u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2050));
}

// 0x8DB740 (initializer)
void __cdecl Fn8DB740() {
    Thiscall(0x0066BEA0, 0x00C1A79Cu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2060));
}

// 0x8DB760 (initializer)
void __cdecl Fn8DB760() {
    Thiscall(0x0066BEA0, 0x00C1A7A0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2070));
}

// 0x8DB780 (initializer)
void __cdecl Fn8DB780() {
    Thiscall(0x0066BEA0, 0x00C1A7A4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2080));
}

// 0x8DB7A0 (initializer)
void __cdecl Fn8DB7A0() {
    Thiscall(0x0066BEA0, 0x00C1A7A8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2090));
}

// 0x8DB7C0 (initializer)
void __cdecl Fn8DB7C0() {
    Thiscall(0x0066BEA0, 0x00C1A7ACu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20A0));
}

// 0x8DB7E0 (initializer)
void __cdecl Fn8DB7E0() {
    Thiscall(0x0066BEA0, 0x00C1A7B0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20B0));
}

// 0x8DB800 (initializer)
void __cdecl Fn8DB800() {
    Thiscall(0x0066BEA0, 0x00C1A7B4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20C0));
}

// 0x8DB820 (initializer)
void __cdecl Fn8DB820() {
    Thiscall(0x0066BEA0, 0x00C1A7B8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20D0));
}

// 0x8DB840 (initializer)
void __cdecl Fn8DB840() {
    Thiscall(0x0066BEA0, 0x00C1A7BCu);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20E0));
}

// 0x8DB860 (initializer)
void __cdecl Fn8DB860() {
    Thiscall(0x0066BEA0, 0x00C1A7C0u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F20F0));
}

// 0x8DB880 (initializer)
void __cdecl Fn8DB880() {
    Thiscall(0x0066BEA0, 0x00C1A7C4u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2100));
}

// 0x8DB8A0 (initializer)
void __cdecl Fn8DB8A0() {
    Thiscall(0x0066BEA0, 0x00C1A7C8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2110));
}

// 0x8DB8C0 (initializer)
void __cdecl Fn8DB8C0() {
    Float(0x00C1A7CC) = float((1.0 / double(Float(0x00A14A44))));
}

// 0x8DB8E0 (initializer)
void __cdecl Fn8DB8E0() {
    Float(0x00C1A7D0) = float((1.0 / (double(Float(0x00A14A44)) * double(Float(0x00A14A44)))));
}

// 0x8DB900 (initializer)
void __cdecl Fn8DB900() {
    Float(0x00C1A7D4) = float((double(Float(0x00C1A7D0)) * Double(0x00900140)));
}

// 0x8DB920 (initializer)
void __cdecl Fn8DB920() {
    Float(0x00C1A7D8) = float((Double(0x00900AD8) / double(Float(0x00A14A44))));
}

// 0x8DB940 (initializer)
void __cdecl Fn8DB940() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2120));
}

// 0x8DB970 (initializer)
void __cdecl Fn8DB970() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2140));
}

// 0x8DB980 (initializer)
void __cdecl Fn8DB980() {
    Float(0x00C1AA18) = float((1.0 / double(Float(0x00A14A64))));
}

// 0x8DB9A0 (initializer)
void __cdecl Fn8DB9A0() {
    Float(0x00C1AA1C) = float((1.0 / (double(Float(0x00A14A64)) * double(Float(0x00A14A64)))));
}

// 0x8DB9C0 (initializer)
void __cdecl Fn8DB9C0() {
    Float(0x00C1AA20) = float((double(Float(0x00C1AA1C)) * Double(0x00900140)));
}

// 0x8DB9E0 (initializer)
void __cdecl Fn8DB9E0() {
    Float(0x00C1AA24) = float((Double(0x00900AD8) / double(Float(0x00A14A64))));
}

// 0x8DBA00 (initializer)
void __cdecl Fn8DBA00() {
    Float(0x00C1AE94) = float((1.0 / double(Float(0x00A14A98))));
}

// 0x8DBA20 (initializer)
void __cdecl Fn8DBA20() {
    Float(0x00C1AE98) = float((1.0 / (double(Float(0x00A14A98)) * double(Float(0x00A14A98)))));
}

// 0x8DBA40 (initializer)
void __cdecl Fn8DBA40() {
    Float(0x00C1AE9C) = float((double(Float(0x00C1AE98)) * Double(0x00900140)));
}

// 0x8DBA60 (initializer)
void __cdecl Fn8DBA60() {
    Float(0x00C1AEA0) = float((Double(0x00900AD8) / double(Float(0x00A14A98))));
}

// 0x8DBB10 (initializer)
void __cdecl Fn8DBB10() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2190));
}

// 0x8DBB20 (initializer)
void __cdecl Fn8DBB20() {
    Dword(0x00C216B8) = 0x00000000u;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F21A0));
}

// 0x8DBBD0 (initializer)
void __cdecl Fn8DBBD0() {
    Float(0x00C222AC) = float((1.0 / double(Float(0x00A14AA8))));
}

// 0x8DBBF0 (initializer)
void __cdecl Fn8DBBF0() {
    Float(0x00C222B0) = float((1.0 / (double(Float(0x00A14AA8)) * double(Float(0x00A14AA8)))));
}

// 0x8DBC10 (initializer)
void __cdecl Fn8DBC10() {
    Float(0x00C222B4) = float((double(Float(0x00C222B0)) * Double(0x00900140)));
}

// 0x8DBC30 (initializer)
void __cdecl Fn8DBC30() {
    Float(0x00C222B8) = float((Double(0x00900AD8) / double(Float(0x00A14AA8))));
}

// 0x8DBC50 (initializer)
void __cdecl Fn8DBC50() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2230));
}

// 0x8DBC60 (initializer)
void __cdecl Fn8DBC60() {
    Float(0x00C2A8D8) = float((1.0 / double(Float(0x00A14AB8))));
}

// 0x8DBC80 (initializer)
void __cdecl Fn8DBC80() {
    Float(0x00C2A8DC) = float((1.0 / (double(Float(0x00A14AB8)) * double(Float(0x00A14AB8)))));
}

// 0x8DBCA0 (initializer)
void __cdecl Fn8DBCA0() {
    Float(0x00C2A8E0) = float((double(Float(0x00C2A8DC)) * Double(0x00900140)));
}

// 0x8DBCC0 (initializer)
void __cdecl Fn8DBCC0() {
    Float(0x00C2A8E4) = float((Double(0x00900AD8) / double(Float(0x00A14AB8))));
}

// 0x8DBCE0 (initializer)
void __cdecl Fn8DBCE0() {
    Float(0x00C2A910) = float((1.0 / double(Float(0x00A24014))));
}

// 0x8DBD00 (initializer)
void __cdecl Fn8DBD00() {
    Float(0x00C2A914) = float((1.0 / (double(Float(0x00A24014)) * double(Float(0x00A24014)))));
}

// 0x8DBD20 (initializer)
void __cdecl Fn8DBD20() {
    Float(0x00C2A918) = float((double(Float(0x00C2A914)) * Double(0x00900140)));
}

// 0x8DBD40 (initializer)
void __cdecl Fn8DBD40() {
    Float(0x00C2A91C) = float((Double(0x00900AD8) / double(Float(0x00A24014))));
}

// 0x8DBD60 (initializer)
void __cdecl Fn8DBD60() {
    Float(0x00C2A920) = float((1.0 / double(Float(0x00A24028))));
}

// 0x8DBD80 (initializer)
void __cdecl Fn8DBD80() {
    Float(0x00C2A924) = float((1.0 / (double(Float(0x00A24028)) * double(Float(0x00A24028)))));
}

// 0x8DBDA0 (initializer)
void __cdecl Fn8DBDA0() {
    Float(0x00C2A928) = float((double(Float(0x00C2A924)) * Double(0x00900140)));
}

// 0x8DBDC0 (initializer)
void __cdecl Fn8DBDC0() {
    Float(0x00C2A92C) = float((Double(0x00900AD8) / double(Float(0x00A24028))));
}

// 0x8DBDE0 (initializer)
void __cdecl Fn8DBDE0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2250));
}

// 0x8DBDF0 (initializer)
void __cdecl Fn8DBDF0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2240));
}

// 0x8DBE00 (initializer)
void __cdecl Fn8DBE00() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2260));
}

// 0x8DBE10 (initializer)
void __cdecl Fn8DBE10() {
    Float(0x00C2B134) = float((1.0 / double(Float(0x00A3397C))));
}

// 0x8DBE30 (initializer)
void __cdecl Fn8DBE30() {
    Float(0x00C2B138) = float((1.0 / (double(Float(0x00A3397C)) * double(Float(0x00A3397C)))));
}

// 0x8DBE50 (initializer)
void __cdecl Fn8DBE50() {
    Float(0x00C2B13C) = float((double(Float(0x00C2B138)) * Double(0x00900140)));
}

// 0x8DBE70 (initializer)
void __cdecl Fn8DBE70() {
    Float(0x00C2B140) = float((Double(0x00900AD8) / double(Float(0x00A3397C))));
}

// 0x8DBFE0 (initializer)
void __cdecl Fn8DBFE0() {
    Float(0x00C2B200) = float((1.0 / double(Float(0x00A339D8))));
}

// 0x8DC000 (initializer)
void __cdecl Fn8DC000() {
    Float(0x00C2B204) = float((1.0 / (double(Float(0x00A339D8)) * double(Float(0x00A339D8)))));
}

// 0x8DC020 (initializer)
void __cdecl Fn8DC020() {
    Float(0x00C2B208) = float((double(Float(0x00C2B204)) * Double(0x00900140)));
}

// 0x8DC040 (initializer)
void __cdecl Fn8DC040() {
    Float(0x00C2B20C) = float((Double(0x00900AD8) / double(Float(0x00A339D8))));
}

// 0x8DC060 (initializer)
void __cdecl Fn8DC060() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F22E0));
}

// 0x8DC070 (initializer)
void __cdecl Fn8DC070() {
    Float(0x00C2B3A4) = float((1.0 / double(Float(0x00A33A30))));
}

// 0x8DC090 (initializer)
void __cdecl Fn8DC090() {
    Float(0x00C2B3A8) = float((1.0 / (double(Float(0x00A33A30)) * double(Float(0x00A33A30)))));
}

// 0x8DC0B0 (initializer)
void __cdecl Fn8DC0B0() {
    Float(0x00C2B3AC) = float((double(Float(0x00C2B3A8)) * Double(0x00900140)));
}

// 0x8DC0D0 (initializer)
void __cdecl Fn8DC0D0() {
    Float(0x00C2B3B0) = float((Double(0x00900AD8) / double(Float(0x00A33A30))));
}

// 0x8DC0F0 (initializer)
void __cdecl Fn8DC0F0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2300));
}

// 0x8DC120 (initializer)
void __cdecl Fn8DC120() {
    Float(0x00C2C088) = float((1.0 / double(Float(0x00A35044))));
}

// 0x8DC140 (initializer)
void __cdecl Fn8DC140() {
    Float(0x00C2C08C) = float((1.0 / (double(Float(0x00A35044)) * double(Float(0x00A35044)))));
}

// 0x8DC160 (initializer)
void __cdecl Fn8DC160() {
    Float(0x00C2C090) = float((double(Float(0x00C2C08C)) * Double(0x00900140)));
}

// 0x8DC180 (initializer)
void __cdecl Fn8DC180() {
    Float(0x00C2C094) = float((Double(0x00900AD8) / double(Float(0x00A35044))));
}

// 0x8DC1A0 (initializer)
void __cdecl Fn8DC1A0() {
    Float(0x00C2C098) = float((1.0 / double(Float(0x00A3504C))));
}

// 0x8DC1C0 (initializer)
void __cdecl Fn8DC1C0() {
    Float(0x00C2C09C) = float((1.0 / (double(Float(0x00A3504C)) * double(Float(0x00A3504C)))));
}

// 0x8DC1E0 (initializer)
void __cdecl Fn8DC1E0() {
    Float(0x00C2C0A0) = float((double(Float(0x00C2C09C)) * Double(0x00900140)));
}

// 0x8DC200 (initializer)
void __cdecl Fn8DC200() {
    Float(0x00C2C0A4) = float((Double(0x00900AD8) / double(Float(0x00A3504C))));
}

// 0x8DC220 (initializer)
void __cdecl Fn8DC220() {
    Float(0x00C2C0B0) = float((1.0 / double(Float(0x00A35058))));
}

// 0x8DC240 (initializer)
void __cdecl Fn8DC240() {
    Float(0x00C2C0B4) = float((1.0 / (double(Float(0x00A35058)) * double(Float(0x00A35058)))));
}

// 0x8DC260 (initializer)
void __cdecl Fn8DC260() {
    Float(0x00C2C0B8) = float((double(Float(0x00C2C0B4)) * Double(0x00900140)));
}

// 0x8DC280 (initializer)
void __cdecl Fn8DC280() {
    Float(0x00C2C0BC) = float((Double(0x00900AD8) / double(Float(0x00A35058))));
}

// 0x8DC380 (initializer)
void __cdecl Fn8DC380() {
    Float(0x00C2C110) = float((1.0 / double(Float(0x00A35080))));
}

// 0x8DC3A0 (initializer)
void __cdecl Fn8DC3A0() {
    Float(0x00C2C114) = float((1.0 / (double(Float(0x00A35080)) * double(Float(0x00A35080)))));
}

// 0x8DC3C0 (initializer)
void __cdecl Fn8DC3C0() {
    Float(0x00C2C118) = float((double(Float(0x00C2C114)) * Double(0x00900140)));
}

// 0x8DC3E0 (initializer)
void __cdecl Fn8DC3E0() {
    Float(0x00C2C11C) = float((Double(0x00900AD8) / double(Float(0x00A35080))));
}

// 0x8DC400 (initializer)
void __cdecl Fn8DC400() {
    Float(0x00C2C190) = float((1.0 / double(Float(0x00A35200))));
}

// 0x8DC420 (initializer)
void __cdecl Fn8DC420() {
    Float(0x00C2C194) = float((1.0 / (double(Float(0x00A35200)) * double(Float(0x00A35200)))));
}

// 0x8DC440 (initializer)
void __cdecl Fn8DC440() {
    Float(0x00C2C198) = float((double(Float(0x00C2C194)) * Double(0x00900140)));
}

// 0x8DC460 (initializer)
void __cdecl Fn8DC460() {
    Float(0x00C2C19C) = float((Double(0x00900AD8) / double(Float(0x00A35200))));
}

// 0x8DC4C0 (initializer)
void __cdecl Fn8DC4C0() {
    Thiscall(0x004A7820, 0x00C2C1B0u);  // CAccessorySocketNames::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2340));
}

// 0x8DC4E0 (initializer)
void __cdecl Fn8DC4E0() {
    Thiscall(0x004A7880, 0x00C2C730u);  // CAccessoryManager::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2350));
}

// 0x8DC500 (initializer)
void __cdecl Fn8DC500() {
    Float(0x00C2D164) = float((1.0 / double(Float(0x00A356E0))));
}

// 0x8DC520 (initializer)
void __cdecl Fn8DC520() {
    Float(0x00C2D168) = float((1.0 / (double(Float(0x00A356E0)) * double(Float(0x00A356E0)))));
}

// 0x8DC540 (initializer)
void __cdecl Fn8DC540() {
    Float(0x00C2D16C) = float((double(Float(0x00C2D168)) * Double(0x00900140)));
}

// 0x8DC560 (initializer)
void __cdecl Fn8DC560() {
    Float(0x00C2D170) = float((Double(0x00900AD8) / double(Float(0x00A356E0))));
}

// 0x8DC580 (initializer)
void __cdecl Fn8DC580() {
    Float(0x00C2D188) = float((1.0 / double(Float(0x00A35748))));
}

// 0x8DC5A0 (initializer)
void __cdecl Fn8DC5A0() {
    Float(0x00C2D18C) = float((1.0 / (double(Float(0x00A35748)) * double(Float(0x00A35748)))));
}

// 0x8DC5C0 (initializer)
void __cdecl Fn8DC5C0() {
    Float(0x00C2D190) = float((double(Float(0x00C2D18C)) * Double(0x00900140)));
}

// 0x8DC5E0 (initializer)
void __cdecl Fn8DC5E0() {
    Float(0x00C2D194) = float((Double(0x00900AD8) / double(Float(0x00A35748))));
}

// 0x8DC600 (initializer)
void __cdecl Fn8DC600() {
    Float(0x00C2D1B8) = float((1.0 / double(Float(0x00A35774))));
}

// 0x8DC620 (initializer)
void __cdecl Fn8DC620() {
    Float(0x00C2D1BC) = float((1.0 / (double(Float(0x00A35774)) * double(Float(0x00A35774)))));
}

// 0x8DC640 (initializer)
void __cdecl Fn8DC640() {
    Float(0x00C2D1C0) = float((double(Float(0x00C2D1BC)) * Double(0x00900140)));
}

// 0x8DC660 (initializer)
void __cdecl Fn8DC660() {
    Float(0x00C2D1C4) = float((Double(0x00900AD8) / double(Float(0x00A35774))));
}

// 0x8DC6B0 (initializer)
void __cdecl Fn8DC6B0() {
    Float(0x00C2DB28) = float((1.0 / double(Float(0x00A35798))));
}

// 0x8DC6D0 (initializer)
void __cdecl Fn8DC6D0() {
    Float(0x00C2DB2C) = float((1.0 / (double(Float(0x00A35798)) * double(Float(0x00A35798)))));
}

// 0x8DC6F0 (initializer)
void __cdecl Fn8DC6F0() {
    Float(0x00C2DB30) = float((double(Float(0x00C2DB2C)) * Double(0x00900140)));
}

// 0x8DC710 (initializer)
void __cdecl Fn8DC710() {
    Float(0x00C2DB34) = float((Double(0x00900AD8) / double(Float(0x00A35798))));
}

// 0x8DC730 (initializer)
void __cdecl Fn8DC730() {
    Float(0x00C2DC20) = float((1.0 / double(Float(0x00A357B0))));
}

// 0x8DC750 (initializer)
void __cdecl Fn8DC750() {
    Float(0x00C2DC24) = float((1.0 / (double(Float(0x00A357B0)) * double(Float(0x00A357B0)))));
}

// 0x8DC770 (initializer)
void __cdecl Fn8DC770() {
    Float(0x00C2DC28) = float((double(Float(0x00C2DC24)) * Double(0x00900140)));
}

// 0x8DC790 (initializer)
void __cdecl Fn8DC790() {
    Float(0x00C2DC2C) = float((Double(0x00900AD8) / double(Float(0x00A357B0))));
}

// 0x8DC7B0 (initializer)
void __cdecl Fn8DC7B0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2390));
}

// 0x8DC7C0 (initializer)
void __cdecl Fn8DC7C0() {
    Float(0x00C2DC34) = float((1.0 / double(Float(0x00A357B8))));
}

// 0x8DC7E0 (initializer)
void __cdecl Fn8DC7E0() {
    Float(0x00C2DC38) = float((1.0 / (double(Float(0x00A357B8)) * double(Float(0x00A357B8)))));
}

// 0x8DC800 (initializer)
void __cdecl Fn8DC800() {
    Float(0x00C2DC3C) = float((double(Float(0x00C2DC38)) * Double(0x00900140)));
}

// 0x8DC820 (initializer)
void __cdecl Fn8DC820() {
    Float(0x00C2DC40) = float((Double(0x00900AD8) / double(Float(0x00A357B8))));
}

// 0x8DC8E0 (initializer)
void __cdecl Fn8DC8E0() {
    Float(0x00C2DEBC) = float((1.0 / double(Float(0x00A35E58))));
}

// 0x8DC900 (initializer)
void __cdecl Fn8DC900() {
    Float(0x00C2DEC0) = float((1.0 / (double(Float(0x00A35E58)) * double(Float(0x00A35E58)))));
}

// 0x8DC920 (initializer)
void __cdecl Fn8DC920() {
    Float(0x00C2DEC4) = float((double(Float(0x00C2DEC0)) * Double(0x00900140)));
}

// 0x8DC940 (initializer)
void __cdecl Fn8DC940() {
    Float(0x00C2DEC8) = float((Double(0x00900AD8) / double(Float(0x00A35E58))));
}

// 0x8DC960 (initializer)
uint32_t __cdecl Fn8DC960() {
    return Thiscall(0x004CA680, 0x00C2DED0u);  // cHandlingDataMgr::Construct
}

// 0x8DC970 (initializer)
void __cdecl Fn8DC970() {
    Float(0x00C34E08) = float((1.0 / double(Float(0x00A35E60))));
}

// 0x8DC990 (initializer)
void __cdecl Fn8DC990() {
    Float(0x00C34E0C) = float((1.0 / (double(Float(0x00A35E60)) * double(Float(0x00A35E60)))));
}

// 0x8DC9B0 (initializer)
void __cdecl Fn8DC9B0() {
    Float(0x00C34E10) = float((double(Float(0x00C34E0C)) * Double(0x00900140)));
}

// 0x8DC9D0 (initializer)
void __cdecl Fn8DC9D0() {
    Float(0x00C34E14) = float((Double(0x00900AD8) / double(Float(0x00A35E60))));
}

// 0x8DC9F0 (initializer)
void __cdecl Fn8DC9F0() {
    Float(0x00C34E1C) = float((1.0 / double(Float(0x00A45290))));
}

// 0x8DCA10 (initializer)
void __cdecl Fn8DCA10() {
    Float(0x00C34E20) = float((1.0 / (double(Float(0x00A45290)) * double(Float(0x00A45290)))));
}

// 0x8DCA30 (initializer)
void __cdecl Fn8DCA30() {
    Float(0x00C34E24) = float((double(Float(0x00C34E20)) * Double(0x00900140)));
}

// 0x8DCA50 (initializer)
void __cdecl Fn8DCA50() {
    Float(0x00C34E28) = float((Double(0x00900AD8) / double(Float(0x00A45290))));
}

// 0x8DCA70 (initializer)
void __cdecl Fn8DCA70() {
    Float(0x00C34E2C) = float((1.0 / double(Float(0x00A45298))));
}

// 0x8DCA90 (initializer)
void __cdecl Fn8DCA90() {
    Float(0x00C34E30) = float((1.0 / (double(Float(0x00A45298)) * double(Float(0x00A45298)))));
}

// 0x8DCAB0 (initializer)
void __cdecl Fn8DCAB0() {
    Float(0x00C34E34) = float((double(Float(0x00C34E30)) * Double(0x00900140)));
}

// 0x8DCAD0 (initializer)
void __cdecl Fn8DCAD0() {
    Float(0x00C34E38) = float((Double(0x00900AD8) / double(Float(0x00A45298))));
}

// 0x8DCAF0 (initializer)
void __cdecl Fn8DCAF0() {
    Float(0x00C34E3C) = float((1.0 / double(Float(0x00A452A0))));
}

// 0x8DCB10 (initializer)
void __cdecl Fn8DCB10() {
    Float(0x00C34E40) = float((1.0 / (double(Float(0x00A452A0)) * double(Float(0x00A452A0)))));
}

// 0x8DCB30 (initializer)
void __cdecl Fn8DCB30() {
    Float(0x00C34E44) = float((double(Float(0x00C34E40)) * Double(0x00900140)));
}

// 0x8DCB50 (initializer)
void __cdecl Fn8DCB50() {
    Float(0x00C34E48) = float((Double(0x00900AD8) / double(Float(0x00A452A0))));
}

// 0x8DCB70 (initializer)
void __cdecl Fn8DCB70() {
    Float(0x00C39B4C) = float((1.0 / double(Float(0x00A4532C))));
}

// 0x8DCB90 (initializer)
void __cdecl Fn8DCB90() {
    Float(0x00C39B50) = float((1.0 / (double(Float(0x00A4532C)) * double(Float(0x00A4532C)))));
}

// 0x8DCBB0 (initializer)
void __cdecl Fn8DCBB0() {
    Float(0x00C39B54) = float((double(Float(0x00C39B50)) * Double(0x00900140)));
}

// 0x8DCBD0 (initializer)
void __cdecl Fn8DCBD0() {
    Float(0x00C39B58) = float((Double(0x00900AD8) / double(Float(0x00A4532C))));
}

// 0x8DCC30 (initializer)
void __cdecl Fn8DCC30() {
    Float(0x00C39F0C) = float((1.0 / double(Float(0x00A45334))));
}

// 0x8DCC50 (initializer)
void __cdecl Fn8DCC50() {
    Float(0x00C39F10) = float((1.0 / (double(Float(0x00A45334)) * double(Float(0x00A45334)))));
}

// 0x8DCC70 (initializer)
void __cdecl Fn8DCC70() {
    Float(0x00C39F14) = float((double(Float(0x00C39F10)) * Double(0x00900140)));
}

// 0x8DCC90 (initializer)
void __cdecl Fn8DCC90() {
    Float(0x00C39F18) = float((Double(0x00900AD8) / double(Float(0x00A45334))));
}

// 0x8DCCB0 (initializer)
void __cdecl Fn8DCCB0() {
    Float(0x00C3A028) = float((1.0 / double(Float(0x00A45348))));
}

// 0x8DCCD0 (initializer)
void __cdecl Fn8DCCD0() {
    Float(0x00C3A02C) = float((1.0 / (double(Float(0x00A45348)) * double(Float(0x00A45348)))));
}

// 0x8DCCF0 (initializer)
void __cdecl Fn8DCCF0() {
    Float(0x00C3A030) = float((double(Float(0x00C3A02C)) * Double(0x00900140)));
}

// 0x8DCD10 (initializer)
void __cdecl Fn8DCD10() {
    Float(0x00C3A034) = float((Double(0x00900AD8) / double(Float(0x00A45348))));
}

// 0x8DCE20 (initializer)
void __cdecl Fn8DCE20() {
    Float(0x00C3A93C) = float((1.0 / double(Float(0x00A45378))));
}

// 0x8DCE40 (initializer)
void __cdecl Fn8DCE40() {
    Float(0x00C3A940) = float((1.0 / (double(Float(0x00A45378)) * double(Float(0x00A45378)))));
}

// 0x8DCE60 (initializer)
void __cdecl Fn8DCE60() {
    Float(0x00C3A944) = float((double(Float(0x00C3A940)) * Double(0x00900140)));
}

// 0x8DCE80 (initializer)
void __cdecl Fn8DCE80() {
    Float(0x00C3A948) = float((Double(0x00900AD8) / double(Float(0x00A45378))));
}

// 0x8DCEA0 (initializer)
void __cdecl Fn8DCEA0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F24A0));
}

// 0x8DCEB0 (initializer)
void __cdecl Fn8DCEB0() {
    Thiscall(0x004A7820, 0x00C3A978u);  // CAccessorySocketNames::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F24B0));
}

// 0x8DCED0 (initializer)
void __cdecl Fn8DCED0() {
    Thiscall(0x004DB100, 0x00C3AEF8u);  // CWeaponManager::CWeaponManager
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F24C0));
}

// 0x8DCEF0 (initializer)
void __cdecl Fn8DCEF0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F24D0));
}

// 0x8DCF70 (initializer)
void __cdecl Fn8DCF70() {
    Thiscall(0x004E9890, 0x00C3B808u);  // CDoorManager::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F24F0));
}

// 0x8DCF90 (initializer)
void __cdecl Fn8DCF90() {
    Float(0x00C3CC40) = float((1.0 / double(Float(0x00A453EC))));
}

// 0x8DCFB0 (initializer)
void __cdecl Fn8DCFB0() {
    Float(0x00C3CC44) = float((1.0 / (double(Float(0x00A453EC)) * double(Float(0x00A453EC)))));
}

// 0x8DCFD0 (initializer)
void __cdecl Fn8DCFD0() {
    Float(0x00C3CC48) = float((double(Float(0x00C3CC44)) * Double(0x00900140)));
}

// 0x8DCFF0 (initializer)
void __cdecl Fn8DCFF0() {
    Float(0x00C3CC4C) = float((Double(0x00900AD8) / double(Float(0x00A453EC))));
}

// 0x8DD010 (initializer)
void __cdecl Fn8DD010() {
    Thiscall(0x00510DE0, 0x00C3CC50u, 0x000000C4u, 0x000000C4u, 0x000000C4u, 0x00000064u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2500));
}

// 0x8DD040 (initializer)
void __cdecl Fn8DD040() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2510));
}

// 0x8DD050 (initializer)
void __cdecl Fn8DD050() {
    Thiscall(0x004F2F10, 0x00C3CC68u);  // CCamera::CCamera
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2540));
}

// 0x8DD070 (initializer)
void __cdecl Fn8DD070() {
    Float(0x00C3D0DC) = float((1.0 / double(Float(0x00A45770))));
}

// 0x8DD090 (initializer)
void __cdecl Fn8DD090() {
    Float(0x00C3D0E0) = float((1.0 / (double(Float(0x00A45770)) * double(Float(0x00A45770)))));
}

// 0x8DD0B0 (initializer)
void __cdecl Fn8DD0B0() {
    Float(0x00C3D0E4) = float((double(Float(0x00C3D0E0)) * Double(0x00900140)));
}

// 0x8DD0D0 (initializer)
void __cdecl Fn8DD0D0() {
    Float(0x00C3D0E8) = float((Double(0x00900AD8) / double(Float(0x00A45770))));
}

// 0x8DD110 (initializer)
void __cdecl Fn8DD110() {
    Float(0x00C3D2A8) = float((1.0 / double(Float(0x00A459F4))));
}

// 0x8DD130 (initializer)
void __cdecl Fn8DD130() {
    Float(0x00C3D2AC) = float((1.0 / (double(Float(0x00A459F4)) * double(Float(0x00A459F4)))));
}

// 0x8DD150 (initializer)
void __cdecl Fn8DD150() {
    Float(0x00C3D2B0) = float((double(Float(0x00C3D2AC)) * Double(0x00900140)));
}

// 0x8DD170 (initializer)
void __cdecl Fn8DD170() {
    Float(0x00C3D2B4) = float((Double(0x00900AD8) / double(Float(0x00A459F4))));
}

// 0x8DD190 (initializer)
void __cdecl Fn8DD190() {
    Float(0x00C3D2BC) = float((1.0 / double(Float(0x00A45A08))));
}

// 0x8DD1B0 (initializer)
void __cdecl Fn8DD1B0() {
    Float(0x00C3D2C0) = float((1.0 / (double(Float(0x00A45A08)) * double(Float(0x00A45A08)))));
}

// 0x8DD1D0 (initializer)
void __cdecl Fn8DD1D0() {
    Float(0x00C3D2C4) = float((double(Float(0x00C3D2C0)) * Double(0x00900140)));
}

// 0x8DD1F0 (initializer)
void __cdecl Fn8DD1F0() {
    Float(0x00C3D2C8) = float((Double(0x00900AD8) / double(Float(0x00A45A08))));
}

// 0x8DD210 (initializer)
void __cdecl Fn8DD210() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2550));
}

// 0x8DD220 (initializer)
void __cdecl Fn8DD220() {
    Float(0x00C3D2CC) = float((1.0 / double(Float(0x00A45B6C))));
}

// 0x8DD240 (initializer)
void __cdecl Fn8DD240() {
    Float(0x00C3D2D0) = float((1.0 / (double(Float(0x00A45B6C)) * double(Float(0x00A45B6C)))));
}

// 0x8DD260 (initializer)
void __cdecl Fn8DD260() {
    Float(0x00C3D2D4) = float((double(Float(0x00C3D2D0)) * Double(0x00900140)));
}

// 0x8DD280 (initializer)
void __cdecl Fn8DD280() {
    Float(0x00C3D2D8) = float((Double(0x00900AD8) / double(Float(0x00A45B6C))));
}

// 0x8DD410 (initializer)
void __cdecl Fn8DD410() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2560));
}

// 0x8DD420 (initializer)
void __cdecl Fn8DD420() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F25B0));
}

// 0x8DD430 (initializer)
void __cdecl Fn8DD430() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2600));
}

// 0x8DD440 (initializer)
void __cdecl Fn8DD440() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2620));
}

// 0x8DD450 (initializer)
void __cdecl Fn8DD450() {
    Stdcall(Dword(0x008FC084), 0x00A45CB0u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2670));
}

// 0x8DD470 (initializer)
void __cdecl Fn8DD470() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2680));
}

// 0x8DD480 (initializer)
void __cdecl Fn8DD480() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2690));
}

// 0x8DD490 (initializer)
void __cdecl Fn8DD490() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F26A0));
}

// 0x8DD510 (initializer)
uint32_t __cdecl Fn8DD510() {
    return Thiscall(0x005136E0, 0x00C66B78u);
}

// 0x8DD5A0 (initializer)
void __cdecl Fn8DD5A0() {
    Thiscall(0x00765410, 0x00C675BCu, 0x00917570u /* "MdBullyAccumulator" */, 0x020E2D50u);  // NiRTTI::NiRTTI
}

// 0x8DD5C0 (initializer)
void __cdecl Fn8DD5C0() {
    Thiscall(0x00765410, 0x00C675F4u, 0x009175E0u /* "MdBullyMainViewRenderClick" */, 0x020E2DBCu);  // NiRTTI::NiRTTI
}

// 0x8DD5E0 (initializer)
void __cdecl Fn8DD5E0() {
    Thiscall(0x00765410, 0x00C6762Cu, 0x00917814u /* "MdBullyMaterial" */, 0x020E2708u);  // NiRTTI::NiRTTI
}

// 0x8DD600 (initializer)
void __cdecl Fn8DD600() {
    Thiscall(0x00765410, 0x00C67670u, 0x009178B0u /* "MdBullyMaterialSwapProcessor" */, 0x020E2E5Cu);  // NiRTTI::NiRTTI
}

// 0x8DD620 (initializer)
void __cdecl Fn8DD620() {
    Thiscall(0x00765410, 0x00C676A4u, 0x009178E0u /* "MdBullyMirror" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8DD640 (initializer)
void __cdecl Fn8DD640() {
    Thiscall(0x00765410, 0x00C676D8u, 0x00917980u /* "MdBullyShadowRenderClick" */, 0x020E2E90u);  // NiRTTI::NiRTTI
}

// 0x8DD660 (initializer)
void __cdecl Fn8DD660() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F27D0));
}

// 0x8DD670 (initializer)
void __cdecl Fn8DD670() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F27E0));
}

// 0x8DD680 (initializer)
void __cdecl Fn8DD680() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F27F0));
}

// 0x8DD690 (initializer)
void __cdecl Fn8DD690() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2830));
}

// 0x8DD6A0 (initializer)
void __cdecl Fn8DD6A0() {
    Thiscall(0x0051A980, 0x00C735E0u, 0x00918028u /* "weaponStore" */);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F26E0));
}

// 0x8DD820 (initializer)
void __cdecl Fn8DD820() {
    Thiscall(0x005109E0, 0x00C89EECu, 0x0000000Bu);  // CMI_ClumpConstruct
    Dword(0x00C89EEC) = 0x00917BECu;
    Dword(0x00C89EE8) = 0x00000000u;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F27A0));
}

// 0x8DD850 (initializer)
void __cdecl Fn8DD850() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F27B0));
}

// 0x8DD910 (initializer)
void __cdecl Fn8DD910() {
    Thiscall(0x00523EB0, 0x00C92D70u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2840));
}

// 0x8DD930 (initializer)
void __cdecl Fn8DD930() {
    Float(0x00C9376C) = float((1.0 / double(Float(0x00A95554))));
}

// 0x8DD950 (initializer)
void __cdecl Fn8DD950() {
    Float(0x00C93770) = float((1.0 / (double(Float(0x00A95554)) * double(Float(0x00A95554)))));
}

// 0x8DD970 (initializer)
void __cdecl Fn8DD970() {
    Float(0x00C93774) = float((double(Float(0x00C93770)) * Double(0x00900140)));
}

// 0x8DD990 (initializer)
void __cdecl Fn8DD990() {
    Float(0x00C93778) = float((Double(0x00900AD8) / double(Float(0x00A95554))));
}

// 0x8DDB00 (initializer)
void __cdecl Fn8DDB00() {
    Float(0x00CA39A4) = float((1.0 / double(Float(0x00A957DC))));
}

// 0x8DDB20 (initializer)
void __cdecl Fn8DDB20() {
    Float(0x00CA39A8) = float((1.0 / (double(Float(0x00A957DC)) * double(Float(0x00A957DC)))));
}

// 0x8DDB40 (initializer)
void __cdecl Fn8DDB40() {
    Float(0x00CA39AC) = float((double(Float(0x00CA39A8)) * Double(0x00900140)));
}

// 0x8DDB60 (initializer)
void __cdecl Fn8DDB60() {
    Float(0x00CA39B0) = float((Double(0x00900AD8) / double(Float(0x00A957DC))));
}

// 0x8DDBB0 (initializer)
void __cdecl Fn8DDBB0() {
    Float(0x00CE7530) = float((1.0 / double(Float(0x00A95AE0))));
}

// 0x8DDBD0 (initializer)
void __cdecl Fn8DDBD0() {
    Float(0x00CE7534) = float((1.0 / (double(Float(0x00A95AE0)) * double(Float(0x00A95AE0)))));
}

// 0x8DDBF0 (initializer)
void __cdecl Fn8DDBF0() {
    Float(0x00CE7538) = float((double(Float(0x00CE7534)) * Double(0x00900140)));
}

// 0x8DDC10 (initializer)
void __cdecl Fn8DDC10() {
    Float(0x00CE753C) = float((Double(0x00900AD8) / double(Float(0x00A95AE0))));
}

// 0x8DDC30 (initializer)
void __cdecl Fn8DDC30() {
    Thiscall(0x00536F30, 0x00CE7580u);  // WindowGlow::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2890));
}

// 0x8DDC50 (initializer)
void __cdecl Fn8DDC50() {
    Float(0x00CEFCA4) = float((1.0 / double(Float(0x00AB456C))));
}

// 0x8DDC70 (initializer)
void __cdecl Fn8DDC70() {
    Float(0x00CEFCA8) = float((1.0 / (double(Float(0x00AB456C)) * double(Float(0x00AB456C)))));
}

// 0x8DDC90 (initializer)
void __cdecl Fn8DDC90() {
    Float(0x00CEFCAC) = float((double(Float(0x00CEFCA8)) * Double(0x00900140)));
}

// 0x8DDCB0 (initializer)
void __cdecl Fn8DDCB0() {
    Float(0x00CEFCB0) = float((Double(0x00900AD8) / double(Float(0x00AB456C))));
}

// 0x8DDCD0 (initializer)
void __cdecl Fn8DDCD0() {
    Thiscall(0x00510DE0, 0x00CEFCB4u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F28C0));
}

// 0x8DDD00 (initializer)
void __cdecl Fn8DDD00() {
    Thiscall(0x00510DE0, 0x00CEFCB8u, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000080u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F28D0));
}

// 0x8DDD30 (initializer)
void __cdecl Fn8DDD30() {
    Thiscall(0x00510DE0, 0x00CEFCBCu, 0x000000EBu, 0x000000EBu, 0x00000000u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F28E0));
}

// 0x8DDD60 (initializer)
void __cdecl Fn8DDD60() {
    Thiscall(0x00510DE0, 0x00CEFCC0u, 0x000000EBu, 0x00000080u, 0x00000000u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F28F0));
}

// 0x8DDD90 (initializer)
void __cdecl Fn8DDD90() {
    Thiscall(0x00510DE0, 0x00CEFCC4u, 0x000000EBu, 0x00000028u, 0x00000028u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2900));
}

// 0x8DDDC0 (initializer)
void __cdecl Fn8DDDC0() {
    Thiscall(0x00510DE0, 0x00CF01A0u, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2910));
}

// 0x8DDDF0 (initializer)
void __cdecl Fn8DDDF0() {
    Thiscall(0x00510DE0, 0x00CF01A4u, 0x00000073u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2920));
}

// 0x8DDE20 (initializer)
void __cdecl Fn8DDE20() {
    Thiscall(0x00510DE0, 0x00CF01A8u, 0x00000022u, 0x0000005Du, 0x00000042u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2930));
}

// 0x8DDE50 (initializer)
void __cdecl Fn8DDE50() {
    Float(0x00CF01BC) = float((1.0 / double(Float(0x00AB467C))));
}

// 0x8DDE70 (initializer)
void __cdecl Fn8DDE70() {
    Float(0x00CF01C0) = float((1.0 / (double(Float(0x00AB467C)) * double(Float(0x00AB467C)))));
}

// 0x8DDE90 (initializer)
void __cdecl Fn8DDE90() {
    Float(0x00CF01C4) = float((double(Float(0x00CF01C0)) * Double(0x00900140)));
}

// 0x8DDEB0 (initializer)
void __cdecl Fn8DDEB0() {
    Float(0x00CF01C8) = float((Double(0x00900AD8) / double(Float(0x00AB467C))));
}

// 0x8DDED0 (initializer)
void __cdecl Fn8DDED0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2940));
}

// 0x8DDEE0 (initializer)
void __cdecl Fn8DDEE0() {
    Thiscall(0x00510DE0, 0x00CF01CCu, 0x000000EBu, 0x000000EBu, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2950));
}

// 0x8DDF10 (initializer)
void __cdecl Fn8DDF10() {
    Thiscall(0x00510DE0, 0x00CF01D0u, 0x00000060u, 0x00000060u, 0x00000060u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2960));
}

// 0x8DDF40 (initializer)
void __cdecl Fn8DDF40() {
    Thiscall(0x00510DE0, 0x00CF01D4u, 0x000000B4u, 0x0000005Au, 0x0000005Au, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2970));
}

// 0x8DDF70 (initializer)
void __cdecl Fn8DDF70() {
    Thiscall(0x00510DE0, 0x00CF01D8u, 0x0000005Au, 0x0000005Au, 0x000000B4u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2980));
}

// 0x8DDFA0 (initializer)
void __cdecl Fn8DDFA0() {
    Thiscall(0x00510DE0, 0x00CF01DCu, 0x00000010u, 0x00000010u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2990));
}

// 0x8DDFD0 (initializer)
void __cdecl Fn8DDFD0() {
    Thiscall(0x00510DE0, 0x00CF01E0u, 0x00000010u, 0x00000010u, 0x00000010u, 0x00000060u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29A0));
}

// 0x8DDFF0 (initializer)
void __cdecl Fn8DDFF0() {
    Float(0x00CF09EC) = float((1.0 / double(Float(0x00AB7398))));
}

// 0x8DE010 (initializer)
void __cdecl Fn8DE010() {
    Float(0x00CF09F0) = float((1.0 / (double(Float(0x00AB7398)) * double(Float(0x00AB7398)))));
}

// 0x8DE030 (initializer)
void __cdecl Fn8DE030() {
    Float(0x00CF09F4) = float((double(Float(0x00CF09F0)) * Double(0x00900140)));
}

// 0x8DE050 (initializer)
void __cdecl Fn8DE050() {
    Float(0x00CF09F8) = float((Double(0x00900AD8) / double(Float(0x00AB7398))));
}

// 0x8DE070 (initializer)
void __cdecl Fn8DE070() {
    Thiscall(0x00510DE0, 0x00CF09FCu, 0x000000FFu, 0x000000FFu, 0x000000FFu, 0x00000078u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29B0));
}

// 0x8DE0A0 (initializer)
void __cdecl Fn8DE0A0() {
    Thiscall(0x00510DE0, 0x00CF0A10u, 0x00000010u, 0x00000010u, 0x00000010u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29C0));
}

// 0x8DE0D0 (initializer)
void __cdecl Fn8DE0D0() {
    Thiscall(0x00510DE0, 0x00CF0A14u, 0x00000080u, 0x00000080u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29D0));
}

// 0x8DE100 (initializer)
void __cdecl Fn8DE100() {
    Thiscall(0x00510DE0, 0x00CF0A18u, 0x000000C0u, 0x00000020u, 0x00000020u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29E0));
}

// 0x8DE130 (initializer)
void __cdecl Fn8DE130() {
    Thiscall(0x00510DE0, 0x00CF0A1Cu, 0x00000032u, 0x00000032u, 0x00000032u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F29F0));
}

// 0x8DE160 (initializer)
void __cdecl Fn8DE160() {
    Thiscall(0x00510DE0, 0x00CF0A20u, 0x00000080u, 0x00000080u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A00));
}

// 0x8DE190 (initializer)
void __cdecl Fn8DE190() {
    Thiscall(0x00510DE0, 0x00CF0A24u, 0x000000EBu, 0x00000080u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A10));
}

// 0x8DE1C0 (initializer)
void __cdecl Fn8DE1C0() {
    Thiscall(0x00510DE0, 0x00CF0A28u, 0x000000EBu, 0x00000080u, 0x00000080u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A20));
}

// 0x8DE1F0 (initializer)
void __cdecl Fn8DE1F0() {
    Thiscall(0x00510DE0, 0x00CF0A2Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A30));
}

// 0x8DE220 (initializer)
void __cdecl Fn8DE220() {
    Thiscall(0x00510DE0, 0x00CF0A38u, 0x000000C8u, 0x00000010u, 0x00000010u, 0x000000A0u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A40));
}

// 0x8DE250 (initializer)
void __cdecl Fn8DE250() {
    Thiscall(0x00510DE0, 0x00CF0A3Cu, 0x000000EBu, 0x000000EBu, 0x000000EBu, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A50));
}

// 0x8DE280 (initializer)
void __cdecl Fn8DE280() {
    Float(0x00CF0A48) = float((1.0 / double(Float(0x00AB81C4))));
}

// 0x8DE2A0 (initializer)
void __cdecl Fn8DE2A0() {
    Float(0x00CF0A4C) = float((1.0 / (double(Float(0x00AB81C4)) * double(Float(0x00AB81C4)))));
}

// 0x8DE2C0 (initializer)
void __cdecl Fn8DE2C0() {
    Float(0x00CF0A50) = float((double(Float(0x00CF0A4C)) * Double(0x00900140)));
}

// 0x8DE2E0 (initializer)
void __cdecl Fn8DE2E0() {
    Float(0x00CF0A54) = float((Double(0x00900AD8) / double(Float(0x00AB81C4))));
}

// 0x8DE300 (initializer)
void __cdecl Fn8DE300() {
    Thiscall(0x00510DE0, 0x00CF0A58u, 0x000000C8u, 0x000000C8u, 0x000000C8u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A60));
}

// 0x8DE330 (initializer)
void __cdecl Fn8DE330() {
    Thiscall(0x00510DE0, 0x00CF0A5Cu, 0x00000060u, 0x00000060u, 0x00000060u, 0x00000060u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A70));
}

// 0x8DE350 (initializer)
void __cdecl Fn8DE350() {
    Thiscall(0x00510DE0, 0x00CF0A60u, 0x000000C8u, 0x000000C8u, 0x000000C8u, 0x000000FFu);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A80));
}

// 0x8DE380 (initializer)
void __cdecl Fn8DE380() {
    Thiscall(0x00510DE0, 0x00CF0A64u, 0x00000060u, 0x00000060u, 0x00000060u, 0x00000060u);  // RGBA_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2A90));
}

// 0x8DE3A0 (initializer)
void __cdecl Fn8DE3A0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2AA0));
}

// 0x8DE3B0 (initializer)
void __cdecl Fn8DE3B0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2AD0));
}

// 0x8DE3C0 (initializer)
void __cdecl Fn8DE3C0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2B00));
}

// 0x8DE3D0 (initializer)
void __cdecl Fn8DE3D0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2B30));
}

// 0x8DE3E0 (initializer)
void __cdecl Fn8DE3E0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2B60));
}

// 0x8DE3F0 (initializer)
void __cdecl Fn8DE3F0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2B90));
}

// 0x8DE400 (initializer)
void __cdecl Fn8DE400() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2BC0));
}

// 0x8DE410 (initializer)
void __cdecl Fn8DE410() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2BF0));
}

// 0x8DE420 (initializer)
void __cdecl Fn8DE420() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C20));
}

// 0x8DE460 (initializer)
void __cdecl Fn8DE460() {
    Float(0x00CF26D0) = float((1.0 / double(Float(0x00AC7700))));
}

// 0x8DE480 (initializer)
void __cdecl Fn8DE480() {
    Float(0x00CF26D4) = float((1.0 / (double(Float(0x00AC7700)) * double(Float(0x00AC7700)))));
}

// 0x8DE4A0 (initializer)
void __cdecl Fn8DE4A0() {
    Float(0x00CF26D8) = float((double(Float(0x00CF26D4)) * Double(0x00900140)));
}

// 0x8DE4C0 (initializer)
void __cdecl Fn8DE4C0() {
    Float(0x00CF26DC) = float((Double(0x00900AD8) / double(Float(0x00AC7700))));
}

// 0x8DE4E0 (initializer)
void __cdecl Fn8DE4E0() {
    uint32_t r1 = Cdecl(0x00576ED0, 0x00902AF4u /* "generic" */);  // HashName
    Dword(0x00CF3078) = r1;
}

// 0x8DE500 (initializer)
void __cdecl Fn8DE500() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C50));
}

// 0x8DE510 (initializer)
void __cdecl Fn8DE510() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C60));
}

// 0x8DE520 (initializer)
void __cdecl Fn8DE520() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C70));
}

// 0x8DE530 (initializer)
void __cdecl Fn8DE530() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C80));
}

// 0x8DE570 (initializer)
void __cdecl Fn8DE570() {
    Float(0x00CF3998) = float((1.0 / double(Float(0x00AE6110))));
}

// 0x8DE590 (initializer)
void __cdecl Fn8DE590() {
    Float(0x00CF399C) = float((1.0 / (double(Float(0x00AE6110)) * double(Float(0x00AE6110)))));
}

// 0x8DE5B0 (initializer)
void __cdecl Fn8DE5B0() {
    Float(0x00CF39A0) = float((double(Float(0x00CF399C)) * Double(0x00900140)));
}

// 0x8DE5D0 (initializer)
void __cdecl Fn8DE5D0() {
    Float(0x00CF39A4) = float((Double(0x00900AD8) / double(Float(0x00AE6110))));
}

// 0x8DE5F0 (initializer)
void __cdecl Fn8DE5F0() {
    Float(0x00CF39A8) = float((1.0 / double(Float(0x00AE6254))));
}

// 0x8DE610 (initializer)
void __cdecl Fn8DE610() {
    Float(0x00CF39AC) = float((1.0 / (double(Float(0x00AE6254)) * double(Float(0x00AE6254)))));
}

// 0x8DE630 (initializer)
void __cdecl Fn8DE630() {
    Float(0x00CF39B0) = float((double(Float(0x00CF39AC)) * Double(0x00900140)));
}

// 0x8DE650 (initializer)
void __cdecl Fn8DE650() {
    Float(0x00CF39B4) = float((Double(0x00900AD8) / double(Float(0x00AE6254))));
}

// 0x8DE670 (initializer)
void __cdecl Fn8DE670() {
    Float(0x00CF39B8) = float((1.0 / double(Float(0x00AE626C))));
}

// 0x8DE690 (initializer)
void __cdecl Fn8DE690() {
    Float(0x00CF39BC) = float((1.0 / (double(Float(0x00AE626C)) * double(Float(0x00AE626C)))));
}

// 0x8DE6B0 (initializer)
void __cdecl Fn8DE6B0() {
    Float(0x00CF39C0) = float((double(Float(0x00CF39BC)) * Double(0x00900140)));
}

// 0x8DE6D0 (initializer)
void __cdecl Fn8DE6D0() {
    Float(0x00CF39C4) = float((Double(0x00900AD8) / double(Float(0x00AE626C))));
}

// 0x8DE6F0 (initializer)
void __cdecl Fn8DE6F0() {
    Float(0x00CF3B14) = float((1.0 / double(Float(0x00AE6308))));
}

// 0x8DE710 (initializer)
void __cdecl Fn8DE710() {
    Float(0x00CF3B18) = float((1.0 / (double(Float(0x00AE6308)) * double(Float(0x00AE6308)))));
}

// 0x8DE730 (initializer)
void __cdecl Fn8DE730() {
    Float(0x00CF3B1C) = float((double(Float(0x00CF3B18)) * Double(0x00900140)));
}

// 0x8DE750 (initializer)
void __cdecl Fn8DE750() {
    Float(0x00CF3B20) = float((Double(0x00900AD8) / double(Float(0x00AE6308))));
}

// 0x8DE790 (initializer)
void __cdecl Fn8DE790() {
    Float(0x00CF3BF4) = float((1.0 / double(Float(0x00AE6B80))));
}

// 0x8DE7B0 (initializer)
void __cdecl Fn8DE7B0() {
    Float(0x00CF3BF8) = float((1.0 / (double(Float(0x00AE6B80)) * double(Float(0x00AE6B80)))));
}

// 0x8DE7D0 (initializer)
void __cdecl Fn8DE7D0() {
    Float(0x00CF3BFC) = float((double(Float(0x00CF3BF8)) * Double(0x00900140)));
}

// 0x8DE7F0 (initializer)
void __cdecl Fn8DE7F0() {
    Float(0x00CF3C00) = float((Double(0x00900AD8) / double(Float(0x00AE6B80))));
}

// 0x8DE840 (initializer)
void __cdecl Fn8DE840() {
    Dword(0x00CF3D88) = 0x00000000u;
    Dword(0x00CF3D90) = 0x00000000u;
    Dword(0x00CF3D94) = 0x00000000u;
    Thiscall(0x005D7F10, 0x00CF3D88u, 0x00000000u);  // EntityRef_Set
    Thiscall(0x0059D110, 0x00CF3D90u, 0x00000000u);  // Sound_Set
    Thiscall(0x0059D110, 0x00CF3D94u, 0x00000000u);  // Sound_Set
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2CD0));
}

// 0x8DE890 (initializer)
void __cdecl Fn8DE890() {
    Thiscall(0x005961B0, 0x00CF3DA0u);  // TheSounds::TheSounds
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2C90));
}

// 0x8DE8B0 (initializer)
void __cdecl Fn8DE8B0() {
    Float(0x00CF68FC) = float((1.0 / double(Float(0x00AE6C10))));
}

// 0x8DE8D0 (initializer)
void __cdecl Fn8DE8D0() {
    Float(0x00CF6900) = float((1.0 / (double(Float(0x00AE6C10)) * double(Float(0x00AE6C10)))));
}

// 0x8DE8F0 (initializer)
void __cdecl Fn8DE8F0() {
    Float(0x00CF6904) = float((double(Float(0x00CF6900)) * Double(0x00900140)));
}

// 0x8DE910 (initializer)
void __cdecl Fn8DE910() {
    Float(0x00CF6908) = float((Double(0x00900AD8) / double(Float(0x00AE6C10))));
}

// 0x8DE970 (initializer)
void __cdecl Fn8DE970() {
    Float(0x00CF6CAC) = float((1.0 / double(Float(0x00AE80B0))));
}

// 0x8DE990 (initializer)
void __cdecl Fn8DE990() {
    Float(0x00CF6CB0) = float((1.0 / (double(Float(0x00AE80B0)) * double(Float(0x00AE80B0)))));
}

// 0x8DE9B0 (initializer)
void __cdecl Fn8DE9B0() {
    Float(0x00CF6CB4) = float((double(Float(0x00CF6CB0)) * Double(0x00900140)));
}

// 0x8DE9D0 (initializer)
void __cdecl Fn8DE9D0() {
    Float(0x00CF6CB8) = float((Double(0x00900AD8) / double(Float(0x00AE80B0))));
}

// 0x8DEA90 (initializer)
void __cdecl Fn8DEA90() {
    Stdcall(Dword(0x008FC084), 0x00CF7148u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D00));
}

// 0x8DEAB0 (initializer)
void __cdecl Fn8DEAB0() {
    Thiscall(0x005AA2B0, 0x00CF7268u);  // CueEngine::CueEngine
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D10));
}

// 0x8DEAD0 (initializer)
void __cdecl Fn8DEAD0() {
    Stdcall(Dword(0x008FC084), 0x00CFAD10u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D20));
}

// 0x8DEAF0 (initializer)
void __cdecl Fn8DEAF0() {
    Stdcall(Dword(0x008FC084), 0x00CFAD28u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D30));
}

// 0x8DEB10 (initializer)
void __cdecl Fn8DEB10() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D40));
}

// 0x8DEB80 (initializer)
void __cdecl Fn8DEB80() {
    Thiscall(0x005B4CF0, 0x00CFAF90u);  // ScriptEvents::PropConstruct_5B4CF0
    Dword(0x00CFB064) = 0xFFFFFFFFu;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2D70));
}

// 0x8DEBE0 (initializer)
void __cdecl Fn8DEBE0() {
    Float(0x00CFB2BC) = float((1.0 / double(Float(0x00AE9410))));
}

// 0x8DEC00 (initializer)
void __cdecl Fn8DEC00() {
    Float(0x00CFB2C0) = float((1.0 / (double(Float(0x00AE9410)) * double(Float(0x00AE9410)))));
}

// 0x8DEC20 (initializer)
void __cdecl Fn8DEC20() {
    Float(0x00CFB2C4) = float((double(Float(0x00CFB2C0)) * Double(0x00900140)));
}

// 0x8DEC40 (initializer)
void __cdecl Fn8DEC40() {
    Float(0x00CFB2C8) = float((Double(0x00900AD8) / double(Float(0x00AE9410))));
}

// 0x8DEC60 (initializer)
void __cdecl Fn8DEC60() {
    Float(0x00CFB2CC) = float((1.0 / double(Float(0x00AE94D8))));
}

// 0x8DEC80 (initializer)
void __cdecl Fn8DEC80() {
    Float(0x00CFB2D0) = float((1.0 / (double(Float(0x00AE94D8)) * double(Float(0x00AE94D8)))));
}

// 0x8DECA0 (initializer)
void __cdecl Fn8DECA0() {
    Float(0x00CFB2D4) = float((double(Float(0x00CFB2D0)) * Double(0x00900140)));
}

// 0x8DECC0 (initializer)
void __cdecl Fn8DECC0() {
    Float(0x00CFB2D8) = float((Double(0x00900AD8) / double(Float(0x00AE94D8))));
}

// 0x8DED10 (initializer)
void __cdecl Fn8DED10() {
    Float(0x00CFB344) = float((1.0 / double(Float(0x00AE9910))));
}

// 0x8DED30 (initializer)
void __cdecl Fn8DED30() {
    Float(0x00CFB348) = float((1.0 / (double(Float(0x00AE9910)) * double(Float(0x00AE9910)))));
}

// 0x8DED50 (initializer)
void __cdecl Fn8DED50() {
    Float(0x00CFB34C) = float((double(Float(0x00CFB348)) * Double(0x00900140)));
}

// 0x8DED70 (initializer)
void __cdecl Fn8DED70() {
    Float(0x00CFB350) = float((Double(0x00900AD8) / double(Float(0x00AE9910))));
}

// 0x8DED90 (initializer)
void __cdecl Fn8DED90() {
    Float(0x00CFB360) = float((1.0 / double(Float(0x00AEA254))));
}

// 0x8DEDB0 (initializer)
void __cdecl Fn8DEDB0() {
    Float(0x00CFB364) = float((1.0 / (double(Float(0x00AEA254)) * double(Float(0x00AEA254)))));
}

// 0x8DEDD0 (initializer)
void __cdecl Fn8DEDD0() {
    Float(0x00CFB368) = float((double(Float(0x00CFB364)) * Double(0x00900140)));
}

// 0x8DEDF0 (initializer)
void __cdecl Fn8DEDF0() {
    Float(0x00CFB36C) = float((Double(0x00900AD8) / double(Float(0x00AEA254))));
}

// 0x8DEE10 (initializer)
void __cdecl Fn8DEE10() {
    Float(0x00CFB37C) = float((1.0 / double(Float(0x00AEAF08))));
}

// 0x8DEE30 (initializer)
void __cdecl Fn8DEE30() {
    Float(0x00CFB380) = float((1.0 / (double(Float(0x00AEAF08)) * double(Float(0x00AEAF08)))));
}

// 0x8DEE50 (initializer)
void __cdecl Fn8DEE50() {
    Float(0x00CFB384) = float((double(Float(0x00CFB380)) * Double(0x00900140)));
}

// 0x8DEE70 (initializer)
void __cdecl Fn8DEE70() {
    Float(0x00CFB388) = float((Double(0x00900AD8) / double(Float(0x00AEAF08))));
}

// 0x8DEEC0 (initializer)
void __cdecl Fn8DEEC0() {
    Float(0x00CFB3A4) = float((1.0 / double(Float(0x00AEB1E0))));
}

// 0x8DEEE0 (initializer)
void __cdecl Fn8DEEE0() {
    Float(0x00CFB3A8) = float((1.0 / (double(Float(0x00AEB1E0)) * double(Float(0x00AEB1E0)))));
}

// 0x8DEF00 (initializer)
void __cdecl Fn8DEF00() {
    Float(0x00CFB3AC) = float((double(Float(0x00CFB3A8)) * Double(0x00900140)));
}

// 0x8DEF20 (initializer)
void __cdecl Fn8DEF20() {
    Float(0x00CFB3B0) = float((Double(0x00900AD8) / double(Float(0x00AEB1E0))));
}

// 0x8DEF40 (initializer)
void __cdecl Fn8DEF40() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2DA0));
}

// 0x8DEF50 (initializer)
void __cdecl Fn8DEF50() {
    Float(0x00CFB6D8) = float((1.0 / double(Float(0x00AEB908))));
}

// 0x8DEF70 (initializer)
void __cdecl Fn8DEF70() {
    Float(0x00CFB6DC) = float((1.0 / (double(Float(0x00AEB908)) * double(Float(0x00AEB908)))));
}

// 0x8DEF90 (initializer)
void __cdecl Fn8DEF90() {
    Float(0x00CFB6E0) = float((double(Float(0x00CFB6DC)) * Double(0x00900140)));
}

// 0x8DEFB0 (initializer)
void __cdecl Fn8DEFB0() {
    Float(0x00CFB6E4) = float((Double(0x00900AD8) / double(Float(0x00AEB908))));
}

// 0x8DEFD0 (initializer)
void __cdecl Fn8DEFD0() {
    Thiscall(0x005D7EC0, 0x00CFB6E8u);  // ScriptArray::Construct_5D7EC0
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2DB0));
}

// 0x8DEFF0 (initializer)
void __cdecl Fn8DEFF0() {
    Float(0x00CFC9A8) = float((1.0 / double(Float(0x00AEB910))));
}

// 0x8DF010 (initializer)
void __cdecl Fn8DF010() {
    Float(0x00CFC9AC) = float((1.0 / (double(Float(0x00AEB910)) * double(Float(0x00AEB910)))));
}

// 0x8DF030 (initializer)
void __cdecl Fn8DF030() {
    Float(0x00CFC9B0) = float((double(Float(0x00CFC9AC)) * Double(0x00900140)));
}

// 0x8DF050 (initializer)
void __cdecl Fn8DF050() {
    Float(0x00CFC9B4) = float((Double(0x00900AD8) / double(Float(0x00AEB910))));
}

// 0x8DF070 (initializer)
void __cdecl Fn8DF070() {
    Thiscall(0x005EEAD0, 0x00CFC9B8u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2DC0));
}

// 0x8DF090 (initializer)
void __cdecl Fn8DF090() {
    Float(0x00CFC9D0) = float((1.0 / double(Float(0x00AEB918))));
}

// 0x8DF0B0 (initializer)
void __cdecl Fn8DF0B0() {
    Float(0x00CFC9D4) = float((1.0 / (double(Float(0x00AEB918)) * double(Float(0x00AEB918)))));
}

// 0x8DF0D0 (initializer)
void __cdecl Fn8DF0D0() {
    Float(0x00CFC9D8) = float((double(Float(0x00CFC9D4)) * Double(0x00900140)));
}

// 0x8DF0F0 (initializer)
void __cdecl Fn8DF0F0() {
    Float(0x00CFC9DC) = float((Double(0x00900AD8) / double(Float(0x00AEB918))));
}

// 0x8DF1B0 (initializer)
void __cdecl Fn8DF1B0() {
    Float(0x00D09510) = float((1.0 / double(Float(0x00AEB924))));
}

// 0x8DF1D0 (initializer)
void __cdecl Fn8DF1D0() {
    Float(0x00D09514) = float((1.0 / (double(Float(0x00AEB924)) * double(Float(0x00AEB924)))));
}

// 0x8DF1F0 (initializer)
void __cdecl Fn8DF1F0() {
    Float(0x00D09518) = float((double(Float(0x00D09514)) * Double(0x00900140)));
}

// 0x8DF210 (initializer)
void __cdecl Fn8DF210() {
    Float(0x00D0951C) = float((Double(0x00900AD8) / double(Float(0x00AEB924))));
}

// 0x8DF2C0 (initializer)
void __cdecl Fn8DF2C0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2E30));
}

// 0x8DF2D0 (initializer)
void __cdecl Fn8DF2D0() {
    Float(0x00D12AD0) = float((1.0 / double(Float(0x00AEB938))));
}

// 0x8DF2F0 (initializer)
void __cdecl Fn8DF2F0() {
    Float(0x00D12AD4) = float((1.0 / (double(Float(0x00AEB938)) * double(Float(0x00AEB938)))));
}

// 0x8DF310 (initializer)
void __cdecl Fn8DF310() {
    Float(0x00D12AD8) = float((double(Float(0x00D12AD4)) * Double(0x00900140)));
}

// 0x8DF330 (initializer)
void __cdecl Fn8DF330() {
    Float(0x00D12ADC) = float((Double(0x00900AD8) / double(Float(0x00AEB938))));
}

// 0x8DF350 (initializer)
void __cdecl Fn8DF350() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2E50));
}

// 0x8DF360 (initializer)
void __cdecl Fn8DF360() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2E60));
}

// 0x8DF370 (initializer)
void __cdecl Fn8DF370() {
    Thiscall(0x005E8AB0, 0x00D13150u);  // WalkableMeshQueryService::Reset
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2E70));
}

// 0x8DF390 (initializer)
void __cdecl Fn8DF390() {
    Thiscall(0x005EB950, 0x00D13730u);  // CPathMovers::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2E80));
}

// 0x8DF3B0 (initializer)
void __cdecl Fn8DF3B0() {
    Thiscall(0x005EE330, 0x00D140B8u);  // LightAccumulator::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2EB0));
}

// 0x8DF3D0 (initializer)
void __cdecl Fn8DF3D0() {
    Thiscall(0x005EEE20, 0x00D14170u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2F20));
}

// 0x8DF3F0 (initializer)
void __cdecl Fn8DF3F0() {
    Thiscall(0x005EEE20, 0x00D14240u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2EC0));
}

// 0x8DF410 (initializer)
void __cdecl Fn8DF410() {
    Thiscall(0x005EEE20, 0x00D14310u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2EE0));
}

// 0x8DF430 (initializer)
void __cdecl Fn8DF430() {
    Thiscall(0x005EEE20, 0x00D143E0u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2F00));
}

// 0x8DF450 (initializer)
void __cdecl Fn8DF450() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2F40));
}

// 0x8DF4D0 (initializer)
void __cdecl Fn8DF4D0() {
    Float(0x01B64888) = float((1.0 / double(Float(0x00B0A4B4))));
}

// 0x8DF4F0 (initializer)
void __cdecl Fn8DF4F0() {
    Float(0x01B6488C) = float((1.0 / (double(Float(0x00B0A4B4)) * double(Float(0x00B0A4B4)))));
}

// 0x8DF510 (initializer)
void __cdecl Fn8DF510() {
    Float(0x01B64890) = float((double(Float(0x01B6488C)) * Double(0x00900140)));
}

// 0x8DF530 (initializer)
void __cdecl Fn8DF530() {
    Float(0x01B64894) = float((Double(0x00900AD8) / double(Float(0x00B0A4B4))));
}

// 0x8DF550 (initializer)
void __cdecl Fn8DF550() {
    Thiscall(0x0066BEA0, 0x01B649E8u);  // Member254_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2FD0));
}

// 0x8DF570 (initializer)
void __cdecl Fn8DF570() {
    Float(0x01B64A20) = float((1.0 / double(Float(0x00B0A740))));
}

// 0x8DF590 (initializer)
void __cdecl Fn8DF590() {
    Float(0x01B64A24) = float((1.0 / (double(Float(0x00B0A740)) * double(Float(0x00B0A740)))));
}

// 0x8DF5B0 (initializer)
void __cdecl Fn8DF5B0() {
    Float(0x01B64A28) = float((double(Float(0x01B64A24)) * Double(0x00900140)));
}

// 0x8DF5D0 (initializer)
void __cdecl Fn8DF5D0() {
    Float(0x01B64A2C) = float((Double(0x00900AD8) / double(Float(0x00B0A740))));
}

// 0x8DF5F0 (initializer)
void __cdecl Fn8DF5F0() {
    Thiscall(0x005FA910, 0x01B64A30u, 0x0092DF18u /* "SequenceTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F2FF0));
}

// 0x8DF610 (initializer)
void __cdecl Fn8DF610() {
    Thiscall(0x005FA910, 0x01B64A34u, 0x0092DF28u /* "ExecuteTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3000));
}

// 0x8DF630 (initializer)
void __cdecl Fn8DF630() {
    Thiscall(0x005FA910, 0x01B64A38u, 0x0092DF38u /* "ExecuteDoorTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3010));
}

// 0x8DF650 (initializer)
void __cdecl Fn8DF650() {
    Thiscall(0x005FA910, 0x01B64A3Cu, 0x0092DF4Cu /* "NISobjectTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3020));
}

// 0x8DF670 (initializer)
void __cdecl Fn8DF670() {
    Thiscall(0x005FA910, 0x01B64A40u, 0x0092DF5Cu /* "SpawnTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3030));
}

// 0x8DF690 (initializer)
void __cdecl Fn8DF690() {
    Thiscall(0x005FA910, 0x01B64A44u, 0x0092DF68u /* "PlayTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3040));
}

// 0x8DF6B0 (initializer)
void __cdecl Fn8DF6B0() {
    Thiscall(0x005FA910, 0x01B64A48u, 0x0092DF74u /* "TargetPlayTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3050));
}

// 0x8DF6D0 (initializer)
void __cdecl Fn8DF6D0() {
    Thiscall(0x005FA910, 0x01B64A4Cu, 0x0092DF84u /* "OpportunityTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3060));
}

// 0x8DF6F0 (initializer)
void __cdecl Fn8DF6F0() {
    Thiscall(0x005FA910, 0x01B64A50u, 0x0092DF98u /* "OpportunityStruggleTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3070));
}

// 0x8DF710 (initializer)
void __cdecl Fn8DF710() {
    Thiscall(0x005FA910, 0x01B64A54u, 0x0092DFB4u /* "OpportunityElseActionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3080));
}

// 0x8DF730 (initializer)
void __cdecl Fn8DF730() {
    Thiscall(0x005FA910, 0x01B64A58u, 0x0092DFD0u /* "OpportunityRandomLatchTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3090));
}

// 0x8DF750 (initializer)
void __cdecl Fn8DF750() {
    Thiscall(0x005FA910, 0x01B64A5Cu, 0x0092DFECu /* "CollisionTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30A0));
}

// 0x8DF770 (initializer)
void __cdecl Fn8DF770() {
    Thiscall(0x005FA910, 0x01B64A60u, 0x0092DFFCu /* "MotionBlurTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30B0));
}

// 0x8DF790 (initializer)
void __cdecl Fn8DF790() {
    Thiscall(0x005FA910, 0x01B64A64u, 0x0092E00Cu /* "BodyMotionBlurTrack" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30C0));
}

// 0x8DF7B0 (initializer)
void __cdecl Fn8DF7B0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30D0));
}

// 0x8DF7C0 (initializer)
void __cdecl Fn8DF7C0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30E0));
}

// 0x8DF7D0 (initializer)
void __cdecl Fn8DF7D0() {
    Thiscall(0x0061A7E0, 0x01B64A94u);  // TrackRegistry::Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F30F0));
}

// 0x8DF7F0 (initializer)
void __cdecl Fn8DF7F0() {
    Thiscall(0x005FA910, 0x01B64AA4u, 0x0092E15Cu /* "ConditionFalse" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3100));
}

// 0x8DF810 (initializer)
void __cdecl Fn8DF810() {
    Thiscall(0x005FA910, 0x01B64AA8u, 0x0092E16Cu /* "ConditionNOT" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3110));
}

// 0x8DF830 (initializer)
void __cdecl Fn8DF830() {
    Thiscall(0x005FA910, 0x01B64AACu, 0x0092E17Cu /* "ConditionOR" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3120));
}

// 0x8DF850 (initializer)
void __cdecl Fn8DF850() {
    Thiscall(0x005FA910, 0x01B64AB0u, 0x0092E188u /* "ConditionActionNodePlayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3130));
}

// 0x8DF870 (initializer)
void __cdecl Fn8DF870() {
    Thiscall(0x005FA910, 0x01B64AB4u, 0x0092E1A4u /* "ConditionPrint" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3140));
}

// 0x8DF890 (initializer)
void __cdecl Fn8DF890() {
    Thiscall(0x005FA910, 0x01B64AB8u, 0x0092E1B4u /* "ConditionGroupCondition" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3150));
}

// 0x8DF8B0 (initializer)
void __cdecl Fn8DF8B0() {
    Float(0x01B64ABC) = float((1.0 / double(Float(0x00B0AAC4))));
}

// 0x8DF8D0 (initializer)
void __cdecl Fn8DF8D0() {
    Float(0x01B64AC0) = float((1.0 / (double(Float(0x00B0AAC4)) * double(Float(0x00B0AAC4)))));
}

// 0x8DF8F0 (initializer)
void __cdecl Fn8DF8F0() {
    Float(0x01B64AC4) = float((double(Float(0x01B64AC0)) * Double(0x00900140)));
}

// 0x8DF910 (initializer)
void __cdecl Fn8DF910() {
    Float(0x01B64AC8) = float((Double(0x00900AD8) / double(Float(0x00B0AAC4))));
}

// 0x8DF930 (initializer)
void __cdecl Fn8DF930() {
    Thiscall(0x005FA910, 0x01B64ACCu, 0x0092E8B0u /* "ConditionActionIntentionActionRequest" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3160));
}

// 0x8DF950 (initializer)
void __cdecl Fn8DF950() {
    Thiscall(0x005FA910, 0x01B64AD0u, 0x0092E8D8u /* "ConditionIntentionActionNodeRequest" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3170));
}

// 0x8DF970 (initializer)
void __cdecl Fn8DF970() {
    Thiscall(0x005FA910, 0x01B64AD4u, 0x0092E8FCu /* "ConditionActionRequestButton" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3180));
}

// 0x8DF990 (initializer)
void __cdecl Fn8DF990() {
    Thiscall(0x005FA910, 0x01B64AD8u, 0x0092E91Cu /* "ConditionPedFlags" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3190));
}

// 0x8DF9B0 (initializer)
void __cdecl Fn8DF9B0() {
    Thiscall(0x005FA910, 0x01B64ADCu, 0x0092E930u /* "ConditionSetPedFlags" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31A0));
}

// 0x8DF9D0 (initializer)
void __cdecl Fn8DF9D0() {
    Thiscall(0x005FA910, 0x01B64AE0u, 0x0092E948u /* "ConditionPedFightingSystem" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31B0));
}

// 0x8DF9F0 (initializer)
void __cdecl Fn8DF9F0() {
    Thiscall(0x005FA910, 0x01B64AE4u, 0x0092E964u /* "ConditionMotionIntentionDirection" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31C0));
}

// 0x8DFA10 (initializer)
void __cdecl Fn8DFA10() {
    Thiscall(0x005FA910, 0x01B64AE8u, 0x0092E988u /* "ConditionMotionIntentionSpeed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31D0));
}

// 0x8DFA30 (initializer)
void __cdecl Fn8DFA30() {
    Thiscall(0x005FA910, 0x01B64AECu, 0x0092E9A8u /* "ConditionMotionCycle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31E0));
}

// 0x8DFA50 (initializer)
void __cdecl Fn8DFA50() {
    Thiscall(0x005FA910, 0x01B64AF0u, 0x0092E9C0u /* "ConditionAnimationParametric" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F31F0));
}

// 0x8DFA70 (initializer)
void __cdecl Fn8DFA70() {
    Thiscall(0x005FA910, 0x01B64AF4u, 0x0092E9E0u /* "ConditionHealth" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3200));
}

// 0x8DFA90 (initializer)
void __cdecl Fn8DFA90() {
    Thiscall(0x005FA910, 0x01B64AF8u, 0x0092E9F0u /* "ConditionPercentageHealth" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3210));
}

// 0x8DFAB0 (initializer)
void __cdecl Fn8DFAB0() {
    Thiscall(0x005FA910, 0x01B64AFCu, 0x0092EA0Cu /* "ConditionPercentageSpecial" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3220));
}

// 0x8DFAD0 (initializer)
void __cdecl Fn8DFAD0() {
    Thiscall(0x005FA910, 0x01B64B00u, 0x0092EA28u /* "ConditionDamage" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3230));
}

// 0x8DFAF0 (initializer)
void __cdecl Fn8DFAF0() {
    Thiscall(0x005FA910, 0x01B64B04u, 0x0092EA38u /* "ConditionDamagePending" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3240));
}

// 0x8DFB10 (initializer)
void __cdecl Fn8DFB10() {
    Thiscall(0x005FA910, 0x01B64B08u, 0x0092EA50u /* "ConditionSpecial" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3250));
}

// 0x8DFB30 (initializer)
void __cdecl Fn8DFB30() {
    Thiscall(0x005FA910, 0x01B64B0Cu, 0x0092EA64u /* "ConditionSprint" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3260));
}

// 0x8DFB50 (initializer)
void __cdecl Fn8DFB50() {
    Thiscall(0x005FA910, 0x01B64B10u, 0x0092EA74u /* "ConditionSpecialPoints" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3270));
}

// 0x8DFB70 (initializer)
void __cdecl Fn8DFB70() {
    Thiscall(0x005FA910, 0x01B64B14u, 0x0092EA8Cu /* "ConditionCollisionFlag" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3280));
}

// 0x8DFB90 (initializer)
void __cdecl Fn8DFB90() {
    Thiscall(0x005FA910, 0x01B64B18u, 0x0092EAA4u /* "ConditionHitByPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3290));
}

// 0x8DFBB0 (initializer)
void __cdecl Fn8DFBB0() {
    Thiscall(0x005FA910, 0x01B64B1Cu, 0x0092EAB8u /* "ConditionWallHitDirection" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32A0));
}

// 0x8DFBD0 (initializer)
void __cdecl Fn8DFBD0() {
    Thiscall(0x005FA910, 0x01B64B20u, 0x0092EAD4u /* "ConditionPedIsInOutfit" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32B0));
}

// 0x8DFBF0 (initializer)
void __cdecl Fn8DFBF0() {
    Thiscall(0x005FA910, 0x01B64B24u, 0x0092EAECu /* "ConditionHitFlag" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32C0));
}

// 0x8DFC10 (initializer)
void __cdecl Fn8DFC10() {
    Thiscall(0x005FA910, 0x01B64B28u, 0x0092EB00u /* "ConditionHitFlagClear" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32D0));
}

// 0x8DFC30 (initializer)
void __cdecl Fn8DFC30() {
    Thiscall(0x005FA910, 0x01B64B2Cu, 0x0092EB18u /* "ConditionActionNodePlayTimePast" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32E0));
}

// 0x8DFC50 (initializer)
void __cdecl Fn8DFC50() {
    Thiscall(0x005FA910, 0x01B64B30u, 0x0092EB38u /* "ConditionFirstPerson" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F32F0));
}

// 0x8DFC70 (initializer)
void __cdecl Fn8DFC70() {
    Thiscall(0x005FA910, 0x01B64B34u, 0x0092EB50u /* "CanPathToEntityCond" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3300));
}

// 0x8DFC90 (initializer)
void __cdecl Fn8DFC90() {
    Thiscall(0x005FA910, 0x01B64B38u, 0x0092EB64u /* "ConditionTargetVehicle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3310));
}

// 0x8DFCB0 (initializer)
void __cdecl Fn8DFCB0() {
    Thiscall(0x005FA910, 0x01B64B3Cu, 0x0092EB7Cu /* "ConditionTargetVehicleCar" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3320));
}

// 0x8DFCD0 (initializer)
void __cdecl Fn8DFCD0() {
    Thiscall(0x005FA910, 0x01B64B40u, 0x0092EB98u /* "ConditionTargetVehicleBike" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3330));
}

// 0x8DFCF0 (initializer)
void __cdecl Fn8DFCF0() {
    Thiscall(0x005FA910, 0x01B64B44u, 0x0092EBB4u /* "ConditionAttemptAnchorToVehicle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3340));
}

// 0x8DFD10 (initializer)
void __cdecl Fn8DFD10() {
    Thiscall(0x005FA910, 0x01B64B48u, 0x0092EBD4u /* "ConditionTargetVehicleIs" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3350));
}

// 0x8DFD30 (initializer)
void __cdecl Fn8DFD30() {
    Thiscall(0x005FA910, 0x01B64B4Cu, 0x0092EBF0u /* "ConditionTargetVehicleIsLastVehicleUsed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3360));
}

// 0x8DFD50 (initializer)
void __cdecl Fn8DFD50() {
    Thiscall(0x005FA910, 0x01B64B50u, 0x0092EC18u /* "ConditionTargetVehicleOwnerDifferentFaction" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3370));
}

// 0x8DFD70 (initializer)
void __cdecl Fn8DFD70() {
    Thiscall(0x005FA910, 0x01B64B54u, 0x0092EC44u /* "ConditionTargetVehicleIsAlreadyOccupied" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3380));
}

// 0x8DFD90 (initializer)
void __cdecl Fn8DFD90() {
    Thiscall(0x005FA910, 0x01B64B58u, 0x0092EC6Cu /* "ConditionTargetVehicleIsPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3390));
}

// 0x8DFDB0 (initializer)
void __cdecl Fn8DFDB0() {
    Thiscall(0x005FA910, 0x01B64B5Cu, 0x0092EC8Cu /* "ConditionTargetMyTargetsVehicle" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33A0));
}

// 0x8DFDD0 (initializer)
void __cdecl Fn8DFDD0() {
    Thiscall(0x005FA910, 0x01B64B60u, 0x0092ECACu /* "ConditionRelativePosition" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33B0));
}

// 0x8DFDF0 (initializer)
void __cdecl Fn8DFDF0() {
    Thiscall(0x005FA910, 0x01B64B64u, 0x0092ECC8u /* "ConditionVehicleWheelsOnGround" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33C0));
}

// 0x8DFE10 (initializer)
void __cdecl Fn8DFE10() {
    Thiscall(0x005FA910, 0x01B64B68u, 0x0092ECE8u /* "ConditionTargetRelativePosition" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33D0));
}

// 0x8DFE30 (initializer)
void __cdecl Fn8DFE30() {
    Thiscall(0x005FA910, 0x01B64B6Cu, 0x0092ED08u /* "ConditionTargetRelativePedCollisionSpheres" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33E0));
}

// 0x8DFE50 (initializer)
void __cdecl Fn8DFE50() {
    Thiscall(0x005FA910, 0x01B64B70u, 0x0092ED34u /* "ConditionTargetRelativeOrientation" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F33F0));
}

// 0x8DFE70 (initializer)
void __cdecl Fn8DFE70() {
    Thiscall(0x005FA910, 0x01B64B74u, 0x0092ED58u /* "ConditionTargetRelativePositionInsideBox" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3400));
}

// 0x8DFE90 (initializer)
void __cdecl Fn8DFE90() {
    Thiscall(0x005FA910, 0x01B64B78u, 0x0092ED84u /* "ConditionTargetHealth" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3410));
}

// 0x8DFEB0 (initializer)
void __cdecl Fn8DFEB0() {
    Thiscall(0x005FA910, 0x01B64B7Cu, 0x0092ED9Cu /* "ConditionTargetPlayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3420));
}

// 0x8DFED0 (initializer)
void __cdecl Fn8DFED0() {
    Thiscall(0x005FA910, 0x01B64B80u, 0x0092EDB4u /* "ConditionTargetPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3430));
}

// 0x8DFEF0 (initializer)
void __cdecl Fn8DFEF0() {
    Thiscall(0x005FA910, 0x01B64B84u, 0x0092EDCCu /* "ConditionMePlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3440));
}

// 0x8DFF10 (initializer)
void __cdecl Fn8DFF10() {
    Thiscall(0x005FA910, 0x01B64B88u, 0x0092EDE0u /* "ConditionMePlayingDefaultNeutralState" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3450));
}

// 0x8DFF30 (initializer)
void __cdecl Fn8DFF30() {
    Thiscall(0x005FA910, 0x01B64B8Cu, 0x0092EE08u /* "ConditionIsDayTime" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3460));
}

// 0x8DFF50 (initializer)
void __cdecl Fn8DFF50() {
    Thiscall(0x005FA910, 0x01B64B90u, 0x0092EE1Cu /* "ConditionAIIsPlaying" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3470));
}

// 0x8DFF70 (initializer)
void __cdecl Fn8DFF70() {
    Thiscall(0x005FA910, 0x01B64B94u, 0x0092EE34u /* "ConditionLiquidLevel" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3480));
}

// 0x8DFF90 (initializer)
void __cdecl Fn8DFF90() {
    Thiscall(0x005FA910, 0x01B64B98u, 0x0092EE4Cu /* "ConditionLiquidType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3490));
}

// 0x8DFFB0 (initializer)
void __cdecl Fn8DFFB0() {
    Thiscall(0x005FA910, 0x01B64B9Cu, 0x0092EE60u /* "ConditionSurfaceHardness" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34A0));
}

// 0x8DFFD0 (initializer)
void __cdecl Fn8DFFD0() {
    Thiscall(0x005FA910, 0x01B64BA0u, 0x0092EE7Cu /* "ConditionInTriggerVolume" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34B0));
}

// 0x8DFFF0 (initializer)
void __cdecl Fn8DFFF0() {
    Thiscall(0x005FA910, 0x01B64BA4u, 0x0092EE98u /* "ConditionPlayerInTriggerVolume" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34C0));
}

// 0x8E0010 (initializer)
void __cdecl Fn8E0010() {
    Thiscall(0x005FA910, 0x01B64BA8u, 0x0092EEB8u /* "ConditionInTriggerVolumeOfType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34D0));
}

// 0x8E0030 (initializer)
void __cdecl Fn8E0030() {
    Thiscall(0x005FA910, 0x01B64BACu, 0x0092EED8u /* "ConditionNearPoint" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34E0));
}

// 0x8E0050 (initializer)
void __cdecl Fn8E0050() {
    Thiscall(0x005FA910, 0x01B64BB0u, 0x0092EEECu /* "ConditionPedType" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F34F0));
}

// 0x8E0070 (initializer)
void __cdecl Fn8E0070() {
    Thiscall(0x005FA910, 0x01B64BB4u, 0x0092EF00u /* "ConditionPedIsDriver" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3500));
}

// 0x8E0090 (initializer)
void __cdecl Fn8E0090() {
    Thiscall(0x005FA910, 0x01B64BB8u, 0x0092EF18u /* "ConditionIsAuthority" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3510));
}

// 0x8E00B0 (initializer)
void __cdecl Fn8E00B0() {
    Thiscall(0x005FA910, 0x01B64BBCu, 0x0092EF30u /* "ConditionIsFemale" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3520));
}

// 0x8E00D0 (initializer)
void __cdecl Fn8E00D0() {
    Thiscall(0x005FA910, 0x01B64BC0u, 0x0092EF44u /* "ConditionIsMissionSpecific" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3530));
}

// 0x8E00F0 (initializer)
void __cdecl Fn8E00F0() {
    Thiscall(0x005FA910, 0x01B64BC4u, 0x0092EF60u /* "ConditionIsHangingOut" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3540));
}

// 0x8E0110 (initializer)
void __cdecl Fn8E0110() {
    Thiscall(0x005FA910, 0x01B64BC8u, 0x0092EF78u /* "ConditionIsSpectator" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3550));
}

// 0x8E0130 (initializer)
void __cdecl Fn8E0130() {
    Thiscall(0x005FA910, 0x01B64BCCu, 0x0092EF90u /* "ConditionIsScenarioPed" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3560));
}

// 0x8E0150 (initializer)
void __cdecl Fn8E0150() {
    Thiscall(0x005FA910, 0x01B64BD0u, 0x0092EFA8u /* "ConditionHasCompletedScenarioObj" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3570));
}

// 0x8E0170 (initializer)
void __cdecl Fn8E0170() {
    Thiscall(0x005FA910, 0x01B64BD4u, 0x0092EFCCu /* "ConditionIsScriptedAmbient" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3580));
}

// 0x8E0190 (initializer)
void __cdecl Fn8E0190() {
    Thiscall(0x005FA910, 0x01B64BD8u, 0x0092EFE8u /* "ConditionIsTargetAttackRightsTaken" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F3590));
}

// 0x8E01B0 (initializer)
void __cdecl Fn8E01B0() {
    Thiscall(0x005FA910, 0x01B64BDCu, 0x0092F00Cu /* "ConditionAcquirePOI" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35A0));
}

// 0x8E01D0 (initializer)
void __cdecl Fn8E01D0() {
    Thiscall(0x005FA910, 0x01B64BE0u, 0x0092F020u /* "ConditionHavePOI" */);  // NameKey_Construct
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F35B0));
}

} // namespace
} // namespace staticinit

#define STATICINIT1_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT1_REVERSED("StaticInit::8D9C00", 0x008D9C00, &staticinit::Fn8D9C00);
STATICINIT1_REVERSED("StaticInit::8D9C10", 0x008D9C10, &staticinit::Fn8D9C10);
STATICINIT1_REVERSED("StaticInit::8D9C20", 0x008D9C20, &staticinit::Fn8D9C20);
STATICINIT1_REVERSED("StaticInit::8D9C30", 0x008D9C30, &staticinit::Fn8D9C30);
STATICINIT1_REVERSED("StaticInit::8D9C40", 0x008D9C40, &staticinit::Fn8D9C40);
STATICINIT1_REVERSED("StaticInit::8D9C50", 0x008D9C50, &staticinit::Fn8D9C50);
STATICINIT1_REVERSED("StaticInit::8D9C60", 0x008D9C60, &staticinit::Fn8D9C60);
STATICINIT1_REVERSED("StaticInit::8D9C70", 0x008D9C70, &staticinit::Fn8D9C70);
STATICINIT1_REVERSED("StaticInit::8D9C80", 0x008D9C80, &staticinit::Fn8D9C80);
STATICINIT1_REVERSED("StaticInit::8D9C90", 0x008D9C90, &staticinit::Fn8D9C90);
STATICINIT1_REVERSED("StaticInit::8D9CA0", 0x008D9CA0, &staticinit::Fn8D9CA0);
STATICINIT1_REVERSED("StaticInit::8D9CB0", 0x008D9CB0, &staticinit::Fn8D9CB0);
STATICINIT1_REVERSED("StaticInit::8D9CC0", 0x008D9CC0, &staticinit::Fn8D9CC0);
STATICINIT1_REVERSED("StaticInit::8D9CD0", 0x008D9CD0, &staticinit::Fn8D9CD0);
STATICINIT1_REVERSED("StaticInit::8D9CE0", 0x008D9CE0, &staticinit::Fn8D9CE0);
STATICINIT1_REVERSED("StaticInit::8D9CF0", 0x008D9CF0, &staticinit::Fn8D9CF0);
STATICINIT1_REVERSED("StaticInit::8D9D00", 0x008D9D00, &staticinit::Fn8D9D00);
STATICINIT1_REVERSED("StaticInit::8D9D10", 0x008D9D10, &staticinit::Fn8D9D10);
STATICINIT1_REVERSED("StaticInit::8D9D20", 0x008D9D20, &staticinit::Fn8D9D20);
STATICINIT1_REVERSED("StaticInit::8D9D30", 0x008D9D30, &staticinit::Fn8D9D30);
STATICINIT1_REVERSED("StaticInit::8D9D40", 0x008D9D40, &staticinit::Fn8D9D40);
STATICINIT1_REVERSED("StaticInit::8D9D50", 0x008D9D50, &staticinit::Fn8D9D50);
STATICINIT1_REVERSED("StaticInit::8D9D60", 0x008D9D60, &staticinit::Fn8D9D60);
STATICINIT1_REVERSED("StaticInit::8D9D70", 0x008D9D70, &staticinit::Fn8D9D70);
STATICINIT1_REVERSED("StaticInit::8D9DB0", 0x008D9DB0, &staticinit::Fn8D9DB0);
STATICINIT1_REVERSED("StaticInit::8D9DC0", 0x008D9DC0, &staticinit::Fn8D9DC0);
STATICINIT1_REVERSED("StaticInit::8D9DD0", 0x008D9DD0, &staticinit::Fn8D9DD0);
STATICINIT1_REVERSED("StaticInit::8D9DE0", 0x008D9DE0, &staticinit::Fn8D9DE0);
STATICINIT1_REVERSED("StaticInit::8D9DF0", 0x008D9DF0, &staticinit::Fn8D9DF0);
STATICINIT1_REVERSED("StaticInit::8D9E00", 0x008D9E00, &staticinit::Fn8D9E00);
STATICINIT1_REVERSED("StaticInit::8D9E10", 0x008D9E10, &staticinit::Fn8D9E10);
STATICINIT1_REVERSED("StaticInit::8D9E40", 0x008D9E40, &staticinit::Fn8D9E40);
STATICINIT1_REVERSED("StaticInit::8D9E50", 0x008D9E50, &staticinit::Fn8D9E50);
STATICINIT1_REVERSED("StaticInit::8D9E60", 0x008D9E60, &staticinit::Fn8D9E60);
STATICINIT1_REVERSED("StaticInit::8D9E80", 0x008D9E80, &staticinit::Fn8D9E80);
STATICINIT1_REVERSED("StaticInit::8D9EA0", 0x008D9EA0, &staticinit::Fn8D9EA0);
STATICINIT1_REVERSED("StaticInit::8D9EC0", 0x008D9EC0, &staticinit::Fn8D9EC0);
STATICINIT1_REVERSED("StaticInit::8D9EE0", 0x008D9EE0, &staticinit::Fn8D9EE0);
STATICINIT1_REVERSED("StaticInit::8D9F00", 0x008D9F00, &staticinit::Fn8D9F00);
STATICINIT1_REVERSED("StaticInit::8D9F20", 0x008D9F20, &staticinit::Fn8D9F20);
STATICINIT1_REVERSED("StaticInit::8D9F40", 0x008D9F40, &staticinit::Fn8D9F40);
STATICINIT1_REVERSED("StaticInit::8D9F60", 0x008D9F60, &staticinit::Fn8D9F60);
STATICINIT1_REVERSED("StaticInit::8D9F80", 0x008D9F80, &staticinit::Fn8D9F80);
STATICINIT1_REVERSED("StaticInit::8D9FA0", 0x008D9FA0, &staticinit::Fn8D9FA0);
STATICINIT1_REVERSED("StaticInit::8D9FC0", 0x008D9FC0, &staticinit::Fn8D9FC0);
STATICINIT1_REVERSED("StaticInit::8D9FE0", 0x008D9FE0, &staticinit::Fn8D9FE0);
STATICINIT1_REVERSED("StaticInit::8DA000", 0x008DA000, &staticinit::Fn8DA000);
STATICINIT1_REVERSED("StaticInit::8DA020", 0x008DA020, &staticinit::Fn8DA020);
STATICINIT1_REVERSED("StaticInit::8DA040", 0x008DA040, &staticinit::Fn8DA040);
STATICINIT1_REVERSED("StaticInit::8DA060", 0x008DA060, &staticinit::Fn8DA060);
STATICINIT1_REVERSED("StaticInit::8DA080", 0x008DA080, &staticinit::Fn8DA080);
STATICINIT1_REVERSED("StaticInit::8DA0A0", 0x008DA0A0, &staticinit::Fn8DA0A0);
STATICINIT1_REVERSED("StaticInit::8DA0C0", 0x008DA0C0, &staticinit::Fn8DA0C0);
STATICINIT1_REVERSED("StaticInit::8DA0E0", 0x008DA0E0, &staticinit::Fn8DA0E0);
STATICINIT1_REVERSED("StaticInit::8DA100", 0x008DA100, &staticinit::Fn8DA100);
STATICINIT1_REVERSED("StaticInit::8DA120", 0x008DA120, &staticinit::Fn8DA120);
STATICINIT1_REVERSED("StaticInit::8DA140", 0x008DA140, &staticinit::Fn8DA140);
STATICINIT1_REVERSED("StaticInit::8DA160", 0x008DA160, &staticinit::Fn8DA160);
STATICINIT1_REVERSED("StaticInit::8DA180", 0x008DA180, &staticinit::Fn8DA180);
STATICINIT1_REVERSED("StaticInit::8DA1A0", 0x008DA1A0, &staticinit::Fn8DA1A0);
STATICINIT1_REVERSED("StaticInit::8DA1C0", 0x008DA1C0, &staticinit::Fn8DA1C0);
STATICINIT1_REVERSED("StaticInit::8DA1E0", 0x008DA1E0, &staticinit::Fn8DA1E0);
STATICINIT1_REVERSED("StaticInit::8DA200", 0x008DA200, &staticinit::Fn8DA200);
STATICINIT1_REVERSED("StaticInit::8DA220", 0x008DA220, &staticinit::Fn8DA220);
STATICINIT1_REVERSED("StaticInit::8DA240", 0x008DA240, &staticinit::Fn8DA240);
STATICINIT1_REVERSED("StaticInit::8DA260", 0x008DA260, &staticinit::Fn8DA260);
STATICINIT1_REVERSED("StaticInit::8DA280", 0x008DA280, &staticinit::Fn8DA280);
STATICINIT1_REVERSED("StaticInit::8DA2A0", 0x008DA2A0, &staticinit::Fn8DA2A0);
STATICINIT1_REVERSED("StaticInit::8DA2C0", 0x008DA2C0, &staticinit::Fn8DA2C0);
STATICINIT1_REVERSED("StaticInit::8DA2E0", 0x008DA2E0, &staticinit::Fn8DA2E0);
STATICINIT1_REVERSED("StaticInit::8DA300", 0x008DA300, &staticinit::Fn8DA300);
STATICINIT1_REVERSED("StaticInit::8DA320", 0x008DA320, &staticinit::Fn8DA320);
STATICINIT1_REVERSED("StaticInit::8DA340", 0x008DA340, &staticinit::Fn8DA340);
STATICINIT1_REVERSED("StaticInit::8DA360", 0x008DA360, &staticinit::Fn8DA360);
STATICINIT1_REVERSED("StaticInit::8DA380", 0x008DA380, &staticinit::Fn8DA380);
STATICINIT1_REVERSED("StaticInit::8DA3A0", 0x008DA3A0, &staticinit::Fn8DA3A0);
STATICINIT1_REVERSED("StaticInit::8DA3C0", 0x008DA3C0, &staticinit::Fn8DA3C0);
STATICINIT1_REVERSED("StaticInit::8DA3E0", 0x008DA3E0, &staticinit::Fn8DA3E0);
STATICINIT1_REVERSED("StaticInit::8DA400", 0x008DA400, &staticinit::Fn8DA400);
STATICINIT1_REVERSED("StaticInit::8DA420", 0x008DA420, &staticinit::Fn8DA420);
STATICINIT1_REVERSED("StaticInit::8DA440", 0x008DA440, &staticinit::Fn8DA440);
STATICINIT1_REVERSED("StaticInit::8DA460", 0x008DA460, &staticinit::Fn8DA460);
STATICINIT1_REVERSED("StaticInit::8DA480", 0x008DA480, &staticinit::Fn8DA480);
STATICINIT1_REVERSED("StaticInit::8DA4A0", 0x008DA4A0, &staticinit::Fn8DA4A0);
STATICINIT1_REVERSED("StaticInit::8DA4C0", 0x008DA4C0, &staticinit::Fn8DA4C0);
STATICINIT1_REVERSED("StaticInit::8DA4E0", 0x008DA4E0, &staticinit::Fn8DA4E0);
STATICINIT1_REVERSED("StaticInit::8DA500", 0x008DA500, &staticinit::Fn8DA500);
STATICINIT1_REVERSED("StaticInit::8DA520", 0x008DA520, &staticinit::Fn8DA520);
STATICINIT1_REVERSED("StaticInit::8DA540", 0x008DA540, &staticinit::Fn8DA540);
STATICINIT1_REVERSED("StaticInit::8DA560", 0x008DA560, &staticinit::Fn8DA560);
STATICINIT1_REVERSED("StaticInit::8DA580", 0x008DA580, &staticinit::Fn8DA580);
STATICINIT1_REVERSED("StaticInit::8DA5A0", 0x008DA5A0, &staticinit::Fn8DA5A0);
STATICINIT1_REVERSED("StaticInit::8DA5C0", 0x008DA5C0, &staticinit::Fn8DA5C0);
STATICINIT1_REVERSED("StaticInit::8DA5E0", 0x008DA5E0, &staticinit::Fn8DA5E0);
STATICINIT1_REVERSED("StaticInit::8DA600", 0x008DA600, &staticinit::Fn8DA600);
STATICINIT1_REVERSED("StaticInit::8DA620", 0x008DA620, &staticinit::Fn8DA620);
STATICINIT1_REVERSED("StaticInit::8DA640", 0x008DA640, &staticinit::Fn8DA640);
STATICINIT1_REVERSED("StaticInit::8DA660", 0x008DA660, &staticinit::Fn8DA660);
STATICINIT1_REVERSED("StaticInit::8DA680", 0x008DA680, &staticinit::Fn8DA680);
STATICINIT1_REVERSED("StaticInit::8DA6A0", 0x008DA6A0, &staticinit::Fn8DA6A0);
STATICINIT1_REVERSED("StaticInit::8DA6C0", 0x008DA6C0, &staticinit::Fn8DA6C0);
STATICINIT1_REVERSED("StaticInit::8DA6E0", 0x008DA6E0, &staticinit::Fn8DA6E0);
STATICINIT1_REVERSED("StaticInit::8DA700", 0x008DA700, &staticinit::Fn8DA700);
STATICINIT1_REVERSED("StaticInit::8DA720", 0x008DA720, &staticinit::Fn8DA720);
STATICINIT1_REVERSED("StaticInit::8DA740", 0x008DA740, &staticinit::Fn8DA740);
STATICINIT1_REVERSED("StaticInit::8DA760", 0x008DA760, &staticinit::Fn8DA760);
STATICINIT1_REVERSED("StaticInit::8DA780", 0x008DA780, &staticinit::Fn8DA780);
STATICINIT1_REVERSED("StaticInit::8DA7D0", 0x008DA7D0, &staticinit::Fn8DA7D0);
STATICINIT1_REVERSED("StaticInit::8DA7F0", 0x008DA7F0, &staticinit::Fn8DA7F0);
STATICINIT1_REVERSED("StaticInit::8DA810", 0x008DA810, &staticinit::Fn8DA810);
STATICINIT1_REVERSED("StaticInit::8DA830", 0x008DA830, &staticinit::Fn8DA830);
STATICINIT1_REVERSED("StaticInit::8DA850", 0x008DA850, &staticinit::Fn8DA850);
STATICINIT1_REVERSED("StaticInit::8DA870", 0x008DA870, &staticinit::Fn8DA870);
STATICINIT1_REVERSED("StaticInit::8DA890", 0x008DA890, &staticinit::Fn8DA890);
STATICINIT1_REVERSED("StaticInit::8DA8B0", 0x008DA8B0, &staticinit::Fn8DA8B0);
STATICINIT1_REVERSED("StaticInit::8DA8D0", 0x008DA8D0, &staticinit::Fn8DA8D0);
STATICINIT1_REVERSED("StaticInit::8DA8F0", 0x008DA8F0, &staticinit::Fn8DA8F0);
STATICINIT1_REVERSED("StaticInit::8DA910", 0x008DA910, &staticinit::Fn8DA910);
STATICINIT1_REVERSED("StaticInit::8DA930", 0x008DA930, &staticinit::Fn8DA930);
STATICINIT1_REVERSED("StaticInit::8DA950", 0x008DA950, &staticinit::Fn8DA950);
STATICINIT1_REVERSED("StaticInit::8DA970", 0x008DA970, &staticinit::Fn8DA970);
STATICINIT1_REVERSED("StaticInit::8DA990", 0x008DA990, &staticinit::Fn8DA990);
STATICINIT1_REVERSED("StaticInit::8DA9B0", 0x008DA9B0, &staticinit::Fn8DA9B0);
STATICINIT1_REVERSED("StaticInit::8DA9D0", 0x008DA9D0, &staticinit::Fn8DA9D0);
STATICINIT1_REVERSED("StaticInit::8DA9F0", 0x008DA9F0, &staticinit::Fn8DA9F0);
STATICINIT1_REVERSED("StaticInit::8DAA10", 0x008DAA10, &staticinit::Fn8DAA10);
STATICINIT1_REVERSED("StaticInit::8DAA30", 0x008DAA30, &staticinit::Fn8DAA30);
STATICINIT1_REVERSED("StaticInit::8DAA50", 0x008DAA50, &staticinit::Fn8DAA50);
STATICINIT1_REVERSED("StaticInit::8DAA70", 0x008DAA70, &staticinit::Fn8DAA70);
STATICINIT1_REVERSED("StaticInit::8DAA90", 0x008DAA90, &staticinit::Fn8DAA90);
STATICINIT1_REVERSED("StaticInit::8DAAA0", 0x008DAAA0, &staticinit::Fn8DAAA0);
STATICINIT1_REVERSED("StaticInit::8DAAC0", 0x008DAAC0, &staticinit::Fn8DAAC0);
STATICINIT1_REVERSED("StaticInit::8DAAE0", 0x008DAAE0, &staticinit::Fn8DAAE0);
STATICINIT1_REVERSED("StaticInit::8DAB00", 0x008DAB00, &staticinit::Fn8DAB00);
STATICINIT1_REVERSED("StaticInit::8DAB20", 0x008DAB20, &staticinit::Fn8DAB20);
STATICINIT1_REVERSED("StaticInit::8DAB40", 0x008DAB40, &staticinit::Fn8DAB40);
STATICINIT1_REVERSED("StaticInit::8DAB60", 0x008DAB60, &staticinit::Fn8DAB60);
STATICINIT1_REVERSED("StaticInit::8DAB80", 0x008DAB80, &staticinit::Fn8DAB80);
STATICINIT1_REVERSED("StaticInit::8DABA0", 0x008DABA0, &staticinit::Fn8DABA0);
STATICINIT1_REVERSED("StaticInit::8DABC0", 0x008DABC0, &staticinit::Fn8DABC0);
STATICINIT1_REVERSED("StaticInit::8DAC10", 0x008DAC10, &staticinit::Fn8DAC10);
STATICINIT1_REVERSED("StaticInit::8DAC30", 0x008DAC30, &staticinit::Fn8DAC30);
STATICINIT1_REVERSED("StaticInit::8DAC50", 0x008DAC50, &staticinit::Fn8DAC50);
STATICINIT1_REVERSED("StaticInit::8DAC70", 0x008DAC70, &staticinit::Fn8DAC70);
STATICINIT1_REVERSED("StaticInit::8DAC90", 0x008DAC90, &staticinit::Fn8DAC90);
STATICINIT1_REVERSED("StaticInit::8DACC0", 0x008DACC0, &staticinit::Fn8DACC0);
STATICINIT1_REVERSED("StaticInit::8DACE0", 0x008DACE0, &staticinit::Fn8DACE0);
STATICINIT1_REVERSED("StaticInit::8DAD00", 0x008DAD00, &staticinit::Fn8DAD00);
STATICINIT1_REVERSED("StaticInit::8DAD20", 0x008DAD20, &staticinit::Fn8DAD20);
STATICINIT1_REVERSED("StaticInit::8DAD40", 0x008DAD40, &staticinit::Fn8DAD40);
STATICINIT1_REVERSED("StaticInit::8DAD50", 0x008DAD50, &staticinit::Fn8DAD50);
STATICINIT1_REVERSED("StaticInit::8DAD70", 0x008DAD70, &staticinit::Fn8DAD70);
STATICINIT1_REVERSED("StaticInit::8DAD90", 0x008DAD90, &staticinit::Fn8DAD90);
STATICINIT1_REVERSED("StaticInit::8DADB0", 0x008DADB0, &staticinit::Fn8DADB0);
STATICINIT1_REVERSED("StaticInit::8DADD0", 0x008DADD0, &staticinit::Fn8DADD0);
STATICINIT1_REVERSED("StaticInit::8DADF0", 0x008DADF0, &staticinit::Fn8DADF0);
STATICINIT1_REVERSED("StaticInit::8DAE10", 0x008DAE10, &staticinit::Fn8DAE10);
STATICINIT1_REVERSED("StaticInit::8DAE30", 0x008DAE30, &staticinit::Fn8DAE30);
STATICINIT1_REVERSED("StaticInit::8DAE50", 0x008DAE50, &staticinit::Fn8DAE50);
STATICINIT1_REVERSED("StaticInit::8DAE70", 0x008DAE70, &staticinit::Fn8DAE70);
STATICINIT1_REVERSED("StaticInit::8DAE80", 0x008DAE80, &staticinit::Fn8DAE80);
STATICINIT1_REVERSED("StaticInit::8DAE90", 0x008DAE90, &staticinit::Fn8DAE90);
STATICINIT1_REVERSED("StaticInit::8DAEA0", 0x008DAEA0, &staticinit::Fn8DAEA0);
STATICINIT1_REVERSED("StaticInit::8DAEB0", 0x008DAEB0, &staticinit::Fn8DAEB0);
STATICINIT1_REVERSED("StaticInit::8DAED0", 0x008DAED0, &staticinit::Fn8DAED0);
STATICINIT1_REVERSED("StaticInit::8DAEE0", 0x008DAEE0, &staticinit::Fn8DAEE0);
STATICINIT1_REVERSED("StaticInit::8DAF00", 0x008DAF00, &staticinit::Fn8DAF00);
STATICINIT1_REVERSED("StaticInit::8DAF20", 0x008DAF20, &staticinit::Fn8DAF20);
STATICINIT1_REVERSED("StaticInit::8DAF40", 0x008DAF40, &staticinit::Fn8DAF40);
STATICINIT1_REVERSED("StaticInit::8DAF60", 0x008DAF60, &staticinit::Fn8DAF60);
STATICINIT1_REVERSED("StaticInit::8DAF80", 0x008DAF80, &staticinit::Fn8DAF80);
STATICINIT1_REVERSED("StaticInit::8DAFD0", 0x008DAFD0, &staticinit::Fn8DAFD0);
STATICINIT1_REVERSED("StaticInit::8DAFF0", 0x008DAFF0, &staticinit::Fn8DAFF0);
STATICINIT1_REVERSED("StaticInit::8DB000", 0x008DB000, &staticinit::Fn8DB000);
STATICINIT1_REVERSED("StaticInit::8DB020", 0x008DB020, &staticinit::Fn8DB020);
STATICINIT1_REVERSED("StaticInit::8DB040", 0x008DB040, &staticinit::Fn8DB040);
STATICINIT1_REVERSED("StaticInit::8DB060", 0x008DB060, &staticinit::Fn8DB060);
STATICINIT1_REVERSED("StaticInit::8DB080", 0x008DB080, &staticinit::Fn8DB080);
STATICINIT1_REVERSED("StaticInit::8DB0A0", 0x008DB0A0, &staticinit::Fn8DB0A0);
STATICINIT1_REVERSED("StaticInit::8DB0C0", 0x008DB0C0, &staticinit::Fn8DB0C0);
STATICINIT1_REVERSED("StaticInit::8DB0E0", 0x008DB0E0, &staticinit::Fn8DB0E0);
STATICINIT1_REVERSED("StaticInit::8DB130", 0x008DB130, &staticinit::Fn8DB130);
STATICINIT1_REVERSED("StaticInit::8DB150", 0x008DB150, &staticinit::Fn8DB150);
STATICINIT1_REVERSED("StaticInit::8DB170", 0x008DB170, &staticinit::Fn8DB170);
STATICINIT1_REVERSED("StaticInit::8DB190", 0x008DB190, &staticinit::Fn8DB190);
STATICINIT1_REVERSED("StaticInit::8DB1B0", 0x008DB1B0, &staticinit::Fn8DB1B0);
STATICINIT1_REVERSED("StaticInit::8DB1C0", 0x008DB1C0, &staticinit::Fn8DB1C0);
STATICINIT1_REVERSED("StaticInit::8DB1E0", 0x008DB1E0, &staticinit::Fn8DB1E0);
STATICINIT1_REVERSED("StaticInit::8DB200", 0x008DB200, &staticinit::Fn8DB200);
STATICINIT1_REVERSED("StaticInit::8DB220", 0x008DB220, &staticinit::Fn8DB220);
STATICINIT1_REVERSED("StaticInit::8DB240", 0x008DB240, &staticinit::Fn8DB240);
STATICINIT1_REVERSED("StaticInit::8DB260", 0x008DB260, &staticinit::Fn8DB260);
STATICINIT1_REVERSED("StaticInit::8DB280", 0x008DB280, &staticinit::Fn8DB280);
STATICINIT1_REVERSED("StaticInit::8DB2A0", 0x008DB2A0, &staticinit::Fn8DB2A0);
STATICINIT1_REVERSED("StaticInit::8DB2C0", 0x008DB2C0, &staticinit::Fn8DB2C0);
STATICINIT1_REVERSED("StaticInit::8DB2E0", 0x008DB2E0, &staticinit::Fn8DB2E0);
STATICINIT1_REVERSED("StaticInit::8DB300", 0x008DB300, &staticinit::Fn8DB300);
STATICINIT1_REVERSED("StaticInit::8DB310", 0x008DB310, &staticinit::Fn8DB310);
STATICINIT1_REVERSED("StaticInit::8DB370", 0x008DB370, &staticinit::Fn8DB370);
STATICINIT1_REVERSED("StaticInit::8DB390", 0x008DB390, &staticinit::Fn8DB390);
STATICINIT1_REVERSED("StaticInit::8DB3C0", 0x008DB3C0, &staticinit::Fn8DB3C0);
STATICINIT1_REVERSED("StaticInit::8DB3F0", 0x008DB3F0, &staticinit::Fn8DB3F0);
STATICINIT1_REVERSED("StaticInit::8DB420", 0x008DB420, &staticinit::Fn8DB420);
STATICINIT1_REVERSED("StaticInit::8DB450", 0x008DB450, &staticinit::Fn8DB450);
STATICINIT1_REVERSED("StaticInit::8DB480", 0x008DB480, &staticinit::Fn8DB480);
STATICINIT1_REVERSED("StaticInit::8DB4B0", 0x008DB4B0, &staticinit::Fn8DB4B0);
STATICINIT1_REVERSED("StaticInit::8DB4E0", 0x008DB4E0, &staticinit::Fn8DB4E0);
STATICINIT1_REVERSED("StaticInit::8DB500", 0x008DB500, &staticinit::Fn8DB500);
STATICINIT1_REVERSED("StaticInit::8DB520", 0x008DB520, &staticinit::Fn8DB520);
STATICINIT1_REVERSED("StaticInit::8DB540", 0x008DB540, &staticinit::Fn8DB540);
STATICINIT1_REVERSED("StaticInit::8DB560", 0x008DB560, &staticinit::Fn8DB560);
STATICINIT1_REVERSED("StaticInit::8DB580", 0x008DB580, &staticinit::Fn8DB580);
STATICINIT1_REVERSED("StaticInit::8DB5A0", 0x008DB5A0, &staticinit::Fn8DB5A0);
STATICINIT1_REVERSED("StaticInit::8DB5C0", 0x008DB5C0, &staticinit::Fn8DB5C0);
STATICINIT1_REVERSED("StaticInit::8DB5E0", 0x008DB5E0, &staticinit::Fn8DB5E0);
STATICINIT1_REVERSED("StaticInit::8DB600", 0x008DB600, &staticinit::Fn8DB600);
STATICINIT1_REVERSED("StaticInit::8DB620", 0x008DB620, &staticinit::Fn8DB620);
STATICINIT1_REVERSED("StaticInit::8DB640", 0x008DB640, &staticinit::Fn8DB640);
STATICINIT1_REVERSED("StaticInit::8DB660", 0x008DB660, &staticinit::Fn8DB660);
STATICINIT1_REVERSED("StaticInit::8DB680", 0x008DB680, &staticinit::Fn8DB680);
STATICINIT1_REVERSED("StaticInit::8DB6A0", 0x008DB6A0, &staticinit::Fn8DB6A0);
STATICINIT1_REVERSED("StaticInit::8DB6C0", 0x008DB6C0, &staticinit::Fn8DB6C0);
STATICINIT1_REVERSED("StaticInit::8DB6E0", 0x008DB6E0, &staticinit::Fn8DB6E0);
STATICINIT1_REVERSED("StaticInit::8DB700", 0x008DB700, &staticinit::Fn8DB700);
STATICINIT1_REVERSED("StaticInit::8DB720", 0x008DB720, &staticinit::Fn8DB720);
STATICINIT1_REVERSED("StaticInit::8DB740", 0x008DB740, &staticinit::Fn8DB740);
STATICINIT1_REVERSED("StaticInit::8DB760", 0x008DB760, &staticinit::Fn8DB760);
STATICINIT1_REVERSED("StaticInit::8DB780", 0x008DB780, &staticinit::Fn8DB780);
STATICINIT1_REVERSED("StaticInit::8DB7A0", 0x008DB7A0, &staticinit::Fn8DB7A0);
STATICINIT1_REVERSED("StaticInit::8DB7C0", 0x008DB7C0, &staticinit::Fn8DB7C0);
STATICINIT1_REVERSED("StaticInit::8DB7E0", 0x008DB7E0, &staticinit::Fn8DB7E0);
STATICINIT1_REVERSED("StaticInit::8DB800", 0x008DB800, &staticinit::Fn8DB800);
STATICINIT1_REVERSED("StaticInit::8DB820", 0x008DB820, &staticinit::Fn8DB820);
STATICINIT1_REVERSED("StaticInit::8DB840", 0x008DB840, &staticinit::Fn8DB840);
STATICINIT1_REVERSED("StaticInit::8DB860", 0x008DB860, &staticinit::Fn8DB860);
STATICINIT1_REVERSED("StaticInit::8DB880", 0x008DB880, &staticinit::Fn8DB880);
STATICINIT1_REVERSED("StaticInit::8DB8A0", 0x008DB8A0, &staticinit::Fn8DB8A0);
STATICINIT1_REVERSED("StaticInit::8DB8C0", 0x008DB8C0, &staticinit::Fn8DB8C0);
STATICINIT1_REVERSED("StaticInit::8DB8E0", 0x008DB8E0, &staticinit::Fn8DB8E0);
STATICINIT1_REVERSED("StaticInit::8DB900", 0x008DB900, &staticinit::Fn8DB900);
STATICINIT1_REVERSED("StaticInit::8DB920", 0x008DB920, &staticinit::Fn8DB920);
STATICINIT1_REVERSED("StaticInit::8DB940", 0x008DB940, &staticinit::Fn8DB940);
STATICINIT1_REVERSED("StaticInit::8DB970", 0x008DB970, &staticinit::Fn8DB970);
STATICINIT1_REVERSED("StaticInit::8DB980", 0x008DB980, &staticinit::Fn8DB980);
STATICINIT1_REVERSED("StaticInit::8DB9A0", 0x008DB9A0, &staticinit::Fn8DB9A0);
STATICINIT1_REVERSED("StaticInit::8DB9C0", 0x008DB9C0, &staticinit::Fn8DB9C0);
STATICINIT1_REVERSED("StaticInit::8DB9E0", 0x008DB9E0, &staticinit::Fn8DB9E0);
STATICINIT1_REVERSED("StaticInit::8DBA00", 0x008DBA00, &staticinit::Fn8DBA00);
STATICINIT1_REVERSED("StaticInit::8DBA20", 0x008DBA20, &staticinit::Fn8DBA20);
STATICINIT1_REVERSED("StaticInit::8DBA40", 0x008DBA40, &staticinit::Fn8DBA40);
STATICINIT1_REVERSED("StaticInit::8DBA60", 0x008DBA60, &staticinit::Fn8DBA60);
STATICINIT1_REVERSED("StaticInit::8DBB10", 0x008DBB10, &staticinit::Fn8DBB10);
STATICINIT1_REVERSED("StaticInit::8DBB20", 0x008DBB20, &staticinit::Fn8DBB20);
STATICINIT1_REVERSED("StaticInit::8DBBD0", 0x008DBBD0, &staticinit::Fn8DBBD0);
STATICINIT1_REVERSED("StaticInit::8DBBF0", 0x008DBBF0, &staticinit::Fn8DBBF0);
STATICINIT1_REVERSED("StaticInit::8DBC10", 0x008DBC10, &staticinit::Fn8DBC10);
STATICINIT1_REVERSED("StaticInit::8DBC30", 0x008DBC30, &staticinit::Fn8DBC30);
STATICINIT1_REVERSED("StaticInit::8DBC50", 0x008DBC50, &staticinit::Fn8DBC50);
STATICINIT1_REVERSED("StaticInit::8DBC60", 0x008DBC60, &staticinit::Fn8DBC60);
STATICINIT1_REVERSED("StaticInit::8DBC80", 0x008DBC80, &staticinit::Fn8DBC80);
STATICINIT1_REVERSED("StaticInit::8DBCA0", 0x008DBCA0, &staticinit::Fn8DBCA0);
STATICINIT1_REVERSED("StaticInit::8DBCC0", 0x008DBCC0, &staticinit::Fn8DBCC0);
STATICINIT1_REVERSED("StaticInit::8DBCE0", 0x008DBCE0, &staticinit::Fn8DBCE0);
STATICINIT1_REVERSED("StaticInit::8DBD00", 0x008DBD00, &staticinit::Fn8DBD00);
STATICINIT1_REVERSED("StaticInit::8DBD20", 0x008DBD20, &staticinit::Fn8DBD20);
STATICINIT1_REVERSED("StaticInit::8DBD40", 0x008DBD40, &staticinit::Fn8DBD40);
STATICINIT1_REVERSED("StaticInit::8DBD60", 0x008DBD60, &staticinit::Fn8DBD60);
STATICINIT1_REVERSED("StaticInit::8DBD80", 0x008DBD80, &staticinit::Fn8DBD80);
STATICINIT1_REVERSED("StaticInit::8DBDA0", 0x008DBDA0, &staticinit::Fn8DBDA0);
STATICINIT1_REVERSED("StaticInit::8DBDC0", 0x008DBDC0, &staticinit::Fn8DBDC0);
STATICINIT1_REVERSED("StaticInit::8DBDE0", 0x008DBDE0, &staticinit::Fn8DBDE0);
STATICINIT1_REVERSED("StaticInit::8DBDF0", 0x008DBDF0, &staticinit::Fn8DBDF0);
STATICINIT1_REVERSED("StaticInit::8DBE00", 0x008DBE00, &staticinit::Fn8DBE00);
STATICINIT1_REVERSED("StaticInit::8DBE10", 0x008DBE10, &staticinit::Fn8DBE10);
STATICINIT1_REVERSED("StaticInit::8DBE30", 0x008DBE30, &staticinit::Fn8DBE30);
STATICINIT1_REVERSED("StaticInit::8DBE50", 0x008DBE50, &staticinit::Fn8DBE50);
STATICINIT1_REVERSED("StaticInit::8DBE70", 0x008DBE70, &staticinit::Fn8DBE70);
STATICINIT1_REVERSED("StaticInit::8DBFE0", 0x008DBFE0, &staticinit::Fn8DBFE0);
STATICINIT1_REVERSED("StaticInit::8DC000", 0x008DC000, &staticinit::Fn8DC000);
STATICINIT1_REVERSED("StaticInit::8DC020", 0x008DC020, &staticinit::Fn8DC020);
STATICINIT1_REVERSED("StaticInit::8DC040", 0x008DC040, &staticinit::Fn8DC040);
STATICINIT1_REVERSED("StaticInit::8DC060", 0x008DC060, &staticinit::Fn8DC060);
STATICINIT1_REVERSED("StaticInit::8DC070", 0x008DC070, &staticinit::Fn8DC070);
STATICINIT1_REVERSED("StaticInit::8DC090", 0x008DC090, &staticinit::Fn8DC090);
STATICINIT1_REVERSED("StaticInit::8DC0B0", 0x008DC0B0, &staticinit::Fn8DC0B0);
STATICINIT1_REVERSED("StaticInit::8DC0D0", 0x008DC0D0, &staticinit::Fn8DC0D0);
STATICINIT1_REVERSED("StaticInit::8DC0F0", 0x008DC0F0, &staticinit::Fn8DC0F0);
STATICINIT1_REVERSED("StaticInit::8DC120", 0x008DC120, &staticinit::Fn8DC120);
STATICINIT1_REVERSED("StaticInit::8DC140", 0x008DC140, &staticinit::Fn8DC140);
STATICINIT1_REVERSED("StaticInit::8DC160", 0x008DC160, &staticinit::Fn8DC160);
STATICINIT1_REVERSED("StaticInit::8DC180", 0x008DC180, &staticinit::Fn8DC180);
STATICINIT1_REVERSED("StaticInit::8DC1A0", 0x008DC1A0, &staticinit::Fn8DC1A0);
STATICINIT1_REVERSED("StaticInit::8DC1C0", 0x008DC1C0, &staticinit::Fn8DC1C0);
STATICINIT1_REVERSED("StaticInit::8DC1E0", 0x008DC1E0, &staticinit::Fn8DC1E0);
STATICINIT1_REVERSED("StaticInit::8DC200", 0x008DC200, &staticinit::Fn8DC200);
STATICINIT1_REVERSED("StaticInit::8DC220", 0x008DC220, &staticinit::Fn8DC220);
STATICINIT1_REVERSED("StaticInit::8DC240", 0x008DC240, &staticinit::Fn8DC240);
STATICINIT1_REVERSED("StaticInit::8DC260", 0x008DC260, &staticinit::Fn8DC260);
STATICINIT1_REVERSED("StaticInit::8DC280", 0x008DC280, &staticinit::Fn8DC280);
STATICINIT1_REVERSED("StaticInit::8DC380", 0x008DC380, &staticinit::Fn8DC380);
STATICINIT1_REVERSED("StaticInit::8DC3A0", 0x008DC3A0, &staticinit::Fn8DC3A0);
STATICINIT1_REVERSED("StaticInit::8DC3C0", 0x008DC3C0, &staticinit::Fn8DC3C0);
STATICINIT1_REVERSED("StaticInit::8DC3E0", 0x008DC3E0, &staticinit::Fn8DC3E0);
STATICINIT1_REVERSED("StaticInit::8DC400", 0x008DC400, &staticinit::Fn8DC400);
STATICINIT1_REVERSED("StaticInit::8DC420", 0x008DC420, &staticinit::Fn8DC420);
STATICINIT1_REVERSED("StaticInit::8DC440", 0x008DC440, &staticinit::Fn8DC440);
STATICINIT1_REVERSED("StaticInit::8DC460", 0x008DC460, &staticinit::Fn8DC460);
STATICINIT1_REVERSED("StaticInit::8DC4C0", 0x008DC4C0, &staticinit::Fn8DC4C0);
STATICINIT1_REVERSED("StaticInit::8DC4E0", 0x008DC4E0, &staticinit::Fn8DC4E0);
STATICINIT1_REVERSED("StaticInit::8DC500", 0x008DC500, &staticinit::Fn8DC500);
STATICINIT1_REVERSED("StaticInit::8DC520", 0x008DC520, &staticinit::Fn8DC520);
STATICINIT1_REVERSED("StaticInit::8DC540", 0x008DC540, &staticinit::Fn8DC540);
STATICINIT1_REVERSED("StaticInit::8DC560", 0x008DC560, &staticinit::Fn8DC560);
STATICINIT1_REVERSED("StaticInit::8DC580", 0x008DC580, &staticinit::Fn8DC580);
STATICINIT1_REVERSED("StaticInit::8DC5A0", 0x008DC5A0, &staticinit::Fn8DC5A0);
STATICINIT1_REVERSED("StaticInit::8DC5C0", 0x008DC5C0, &staticinit::Fn8DC5C0);
STATICINIT1_REVERSED("StaticInit::8DC5E0", 0x008DC5E0, &staticinit::Fn8DC5E0);
STATICINIT1_REVERSED("StaticInit::8DC600", 0x008DC600, &staticinit::Fn8DC600);
STATICINIT1_REVERSED("StaticInit::8DC620", 0x008DC620, &staticinit::Fn8DC620);
STATICINIT1_REVERSED("StaticInit::8DC640", 0x008DC640, &staticinit::Fn8DC640);
STATICINIT1_REVERSED("StaticInit::8DC660", 0x008DC660, &staticinit::Fn8DC660);
STATICINIT1_REVERSED("StaticInit::8DC6B0", 0x008DC6B0, &staticinit::Fn8DC6B0);
STATICINIT1_REVERSED("StaticInit::8DC6D0", 0x008DC6D0, &staticinit::Fn8DC6D0);
STATICINIT1_REVERSED("StaticInit::8DC6F0", 0x008DC6F0, &staticinit::Fn8DC6F0);
STATICINIT1_REVERSED("StaticInit::8DC710", 0x008DC710, &staticinit::Fn8DC710);
STATICINIT1_REVERSED("StaticInit::8DC730", 0x008DC730, &staticinit::Fn8DC730);
STATICINIT1_REVERSED("StaticInit::8DC750", 0x008DC750, &staticinit::Fn8DC750);
STATICINIT1_REVERSED("StaticInit::8DC770", 0x008DC770, &staticinit::Fn8DC770);
STATICINIT1_REVERSED("StaticInit::8DC790", 0x008DC790, &staticinit::Fn8DC790);
STATICINIT1_REVERSED("StaticInit::8DC7B0", 0x008DC7B0, &staticinit::Fn8DC7B0);
STATICINIT1_REVERSED("StaticInit::8DC7C0", 0x008DC7C0, &staticinit::Fn8DC7C0);
STATICINIT1_REVERSED("StaticInit::8DC7E0", 0x008DC7E0, &staticinit::Fn8DC7E0);
STATICINIT1_REVERSED("StaticInit::8DC800", 0x008DC800, &staticinit::Fn8DC800);
STATICINIT1_REVERSED("StaticInit::8DC820", 0x008DC820, &staticinit::Fn8DC820);
STATICINIT1_REVERSED("StaticInit::8DC8E0", 0x008DC8E0, &staticinit::Fn8DC8E0);
STATICINIT1_REVERSED("StaticInit::8DC900", 0x008DC900, &staticinit::Fn8DC900);
STATICINIT1_REVERSED("StaticInit::8DC920", 0x008DC920, &staticinit::Fn8DC920);
STATICINIT1_REVERSED("StaticInit::8DC940", 0x008DC940, &staticinit::Fn8DC940);
STATICINIT1_REVERSED("StaticInit::8DC960", 0x008DC960, &staticinit::Fn8DC960);
STATICINIT1_REVERSED("StaticInit::8DC970", 0x008DC970, &staticinit::Fn8DC970);
STATICINIT1_REVERSED("StaticInit::8DC990", 0x008DC990, &staticinit::Fn8DC990);
STATICINIT1_REVERSED("StaticInit::8DC9B0", 0x008DC9B0, &staticinit::Fn8DC9B0);
STATICINIT1_REVERSED("StaticInit::8DC9D0", 0x008DC9D0, &staticinit::Fn8DC9D0);
STATICINIT1_REVERSED("StaticInit::8DC9F0", 0x008DC9F0, &staticinit::Fn8DC9F0);
STATICINIT1_REVERSED("StaticInit::8DCA10", 0x008DCA10, &staticinit::Fn8DCA10);
STATICINIT1_REVERSED("StaticInit::8DCA30", 0x008DCA30, &staticinit::Fn8DCA30);
STATICINIT1_REVERSED("StaticInit::8DCA50", 0x008DCA50, &staticinit::Fn8DCA50);
STATICINIT1_REVERSED("StaticInit::8DCA70", 0x008DCA70, &staticinit::Fn8DCA70);
STATICINIT1_REVERSED("StaticInit::8DCA90", 0x008DCA90, &staticinit::Fn8DCA90);
STATICINIT1_REVERSED("StaticInit::8DCAB0", 0x008DCAB0, &staticinit::Fn8DCAB0);
STATICINIT1_REVERSED("StaticInit::8DCAD0", 0x008DCAD0, &staticinit::Fn8DCAD0);
STATICINIT1_REVERSED("StaticInit::8DCAF0", 0x008DCAF0, &staticinit::Fn8DCAF0);
STATICINIT1_REVERSED("StaticInit::8DCB10", 0x008DCB10, &staticinit::Fn8DCB10);
STATICINIT1_REVERSED("StaticInit::8DCB30", 0x008DCB30, &staticinit::Fn8DCB30);
STATICINIT1_REVERSED("StaticInit::8DCB50", 0x008DCB50, &staticinit::Fn8DCB50);
STATICINIT1_REVERSED("StaticInit::8DCB70", 0x008DCB70, &staticinit::Fn8DCB70);
STATICINIT1_REVERSED("StaticInit::8DCB90", 0x008DCB90, &staticinit::Fn8DCB90);
STATICINIT1_REVERSED("StaticInit::8DCBB0", 0x008DCBB0, &staticinit::Fn8DCBB0);
STATICINIT1_REVERSED("StaticInit::8DCBD0", 0x008DCBD0, &staticinit::Fn8DCBD0);
STATICINIT1_REVERSED("StaticInit::8DCC30", 0x008DCC30, &staticinit::Fn8DCC30);
STATICINIT1_REVERSED("StaticInit::8DCC50", 0x008DCC50, &staticinit::Fn8DCC50);
STATICINIT1_REVERSED("StaticInit::8DCC70", 0x008DCC70, &staticinit::Fn8DCC70);
STATICINIT1_REVERSED("StaticInit::8DCC90", 0x008DCC90, &staticinit::Fn8DCC90);
STATICINIT1_REVERSED("StaticInit::8DCCB0", 0x008DCCB0, &staticinit::Fn8DCCB0);
STATICINIT1_REVERSED("StaticInit::8DCCD0", 0x008DCCD0, &staticinit::Fn8DCCD0);
STATICINIT1_REVERSED("StaticInit::8DCCF0", 0x008DCCF0, &staticinit::Fn8DCCF0);
STATICINIT1_REVERSED("StaticInit::8DCD10", 0x008DCD10, &staticinit::Fn8DCD10);
STATICINIT1_REVERSED("StaticInit::8DCE20", 0x008DCE20, &staticinit::Fn8DCE20);
STATICINIT1_REVERSED("StaticInit::8DCE40", 0x008DCE40, &staticinit::Fn8DCE40);
STATICINIT1_REVERSED("StaticInit::8DCE60", 0x008DCE60, &staticinit::Fn8DCE60);
STATICINIT1_REVERSED("StaticInit::8DCE80", 0x008DCE80, &staticinit::Fn8DCE80);
STATICINIT1_REVERSED("StaticInit::8DCEA0", 0x008DCEA0, &staticinit::Fn8DCEA0);
STATICINIT1_REVERSED("StaticInit::8DCEB0", 0x008DCEB0, &staticinit::Fn8DCEB0);
STATICINIT1_REVERSED("StaticInit::8DCED0", 0x008DCED0, &staticinit::Fn8DCED0);
STATICINIT1_REVERSED("StaticInit::8DCEF0", 0x008DCEF0, &staticinit::Fn8DCEF0);
STATICINIT1_REVERSED("StaticInit::8DCF70", 0x008DCF70, &staticinit::Fn8DCF70);
STATICINIT1_REVERSED("StaticInit::8DCF90", 0x008DCF90, &staticinit::Fn8DCF90);
STATICINIT1_REVERSED("StaticInit::8DCFB0", 0x008DCFB0, &staticinit::Fn8DCFB0);
STATICINIT1_REVERSED("StaticInit::8DCFD0", 0x008DCFD0, &staticinit::Fn8DCFD0);
STATICINIT1_REVERSED("StaticInit::8DCFF0", 0x008DCFF0, &staticinit::Fn8DCFF0);
STATICINIT1_REVERSED("StaticInit::8DD010", 0x008DD010, &staticinit::Fn8DD010);
STATICINIT1_REVERSED("StaticInit::8DD040", 0x008DD040, &staticinit::Fn8DD040);
STATICINIT1_REVERSED("StaticInit::8DD050", 0x008DD050, &staticinit::Fn8DD050);
STATICINIT1_REVERSED("StaticInit::8DD070", 0x008DD070, &staticinit::Fn8DD070);
STATICINIT1_REVERSED("StaticInit::8DD090", 0x008DD090, &staticinit::Fn8DD090);
STATICINIT1_REVERSED("StaticInit::8DD0B0", 0x008DD0B0, &staticinit::Fn8DD0B0);
STATICINIT1_REVERSED("StaticInit::8DD0D0", 0x008DD0D0, &staticinit::Fn8DD0D0);
STATICINIT1_REVERSED("StaticInit::8DD110", 0x008DD110, &staticinit::Fn8DD110);
STATICINIT1_REVERSED("StaticInit::8DD130", 0x008DD130, &staticinit::Fn8DD130);
STATICINIT1_REVERSED("StaticInit::8DD150", 0x008DD150, &staticinit::Fn8DD150);
STATICINIT1_REVERSED("StaticInit::8DD170", 0x008DD170, &staticinit::Fn8DD170);
STATICINIT1_REVERSED("StaticInit::8DD190", 0x008DD190, &staticinit::Fn8DD190);
STATICINIT1_REVERSED("StaticInit::8DD1B0", 0x008DD1B0, &staticinit::Fn8DD1B0);
STATICINIT1_REVERSED("StaticInit::8DD1D0", 0x008DD1D0, &staticinit::Fn8DD1D0);
STATICINIT1_REVERSED("StaticInit::8DD1F0", 0x008DD1F0, &staticinit::Fn8DD1F0);
STATICINIT1_REVERSED("StaticInit::8DD210", 0x008DD210, &staticinit::Fn8DD210);
STATICINIT1_REVERSED("StaticInit::8DD220", 0x008DD220, &staticinit::Fn8DD220);
STATICINIT1_REVERSED("StaticInit::8DD240", 0x008DD240, &staticinit::Fn8DD240);
STATICINIT1_REVERSED("StaticInit::8DD260", 0x008DD260, &staticinit::Fn8DD260);
STATICINIT1_REVERSED("StaticInit::8DD280", 0x008DD280, &staticinit::Fn8DD280);
STATICINIT1_REVERSED("StaticInit::8DD410", 0x008DD410, &staticinit::Fn8DD410);
STATICINIT1_REVERSED("StaticInit::8DD420", 0x008DD420, &staticinit::Fn8DD420);
STATICINIT1_REVERSED("StaticInit::8DD430", 0x008DD430, &staticinit::Fn8DD430);
STATICINIT1_REVERSED("StaticInit::8DD440", 0x008DD440, &staticinit::Fn8DD440);
STATICINIT1_REVERSED("StaticInit::8DD450", 0x008DD450, &staticinit::Fn8DD450);
STATICINIT1_REVERSED("StaticInit::8DD470", 0x008DD470, &staticinit::Fn8DD470);
STATICINIT1_REVERSED("StaticInit::8DD480", 0x008DD480, &staticinit::Fn8DD480);
STATICINIT1_REVERSED("StaticInit::8DD490", 0x008DD490, &staticinit::Fn8DD490);
STATICINIT1_REVERSED("StaticInit::8DD510", 0x008DD510, &staticinit::Fn8DD510);
STATICINIT1_REVERSED("StaticInit::8DD5A0", 0x008DD5A0, &staticinit::Fn8DD5A0);
STATICINIT1_REVERSED("StaticInit::8DD5C0", 0x008DD5C0, &staticinit::Fn8DD5C0);
STATICINIT1_REVERSED("StaticInit::8DD5E0", 0x008DD5E0, &staticinit::Fn8DD5E0);
STATICINIT1_REVERSED("StaticInit::8DD600", 0x008DD600, &staticinit::Fn8DD600);
STATICINIT1_REVERSED("StaticInit::8DD620", 0x008DD620, &staticinit::Fn8DD620);
STATICINIT1_REVERSED("StaticInit::8DD640", 0x008DD640, &staticinit::Fn8DD640);
STATICINIT1_REVERSED("StaticInit::8DD660", 0x008DD660, &staticinit::Fn8DD660);
STATICINIT1_REVERSED("StaticInit::8DD670", 0x008DD670, &staticinit::Fn8DD670);
STATICINIT1_REVERSED("StaticInit::8DD680", 0x008DD680, &staticinit::Fn8DD680);
STATICINIT1_REVERSED("StaticInit::8DD690", 0x008DD690, &staticinit::Fn8DD690);
STATICINIT1_REVERSED("StaticInit::8DD6A0", 0x008DD6A0, &staticinit::Fn8DD6A0);
STATICINIT1_REVERSED("StaticInit::8DD820", 0x008DD820, &staticinit::Fn8DD820);
STATICINIT1_REVERSED("StaticInit::8DD850", 0x008DD850, &staticinit::Fn8DD850);
STATICINIT1_REVERSED("StaticInit::8DD910", 0x008DD910, &staticinit::Fn8DD910);
STATICINIT1_REVERSED("StaticInit::8DD930", 0x008DD930, &staticinit::Fn8DD930);
STATICINIT1_REVERSED("StaticInit::8DD950", 0x008DD950, &staticinit::Fn8DD950);
STATICINIT1_REVERSED("StaticInit::8DD970", 0x008DD970, &staticinit::Fn8DD970);
STATICINIT1_REVERSED("StaticInit::8DD990", 0x008DD990, &staticinit::Fn8DD990);
STATICINIT1_REVERSED("StaticInit::8DDB00", 0x008DDB00, &staticinit::Fn8DDB00);
STATICINIT1_REVERSED("StaticInit::8DDB20", 0x008DDB20, &staticinit::Fn8DDB20);
STATICINIT1_REVERSED("StaticInit::8DDB40", 0x008DDB40, &staticinit::Fn8DDB40);
STATICINIT1_REVERSED("StaticInit::8DDB60", 0x008DDB60, &staticinit::Fn8DDB60);
STATICINIT1_REVERSED("StaticInit::8DDBB0", 0x008DDBB0, &staticinit::Fn8DDBB0);
STATICINIT1_REVERSED("StaticInit::8DDBD0", 0x008DDBD0, &staticinit::Fn8DDBD0);
STATICINIT1_REVERSED("StaticInit::8DDBF0", 0x008DDBF0, &staticinit::Fn8DDBF0);
STATICINIT1_REVERSED("StaticInit::8DDC10", 0x008DDC10, &staticinit::Fn8DDC10);
STATICINIT1_REVERSED("StaticInit::8DDC30", 0x008DDC30, &staticinit::Fn8DDC30);
STATICINIT1_REVERSED("StaticInit::8DDC50", 0x008DDC50, &staticinit::Fn8DDC50);
STATICINIT1_REVERSED("StaticInit::8DDC70", 0x008DDC70, &staticinit::Fn8DDC70);
STATICINIT1_REVERSED("StaticInit::8DDC90", 0x008DDC90, &staticinit::Fn8DDC90);
STATICINIT1_REVERSED("StaticInit::8DDCB0", 0x008DDCB0, &staticinit::Fn8DDCB0);
STATICINIT1_REVERSED("StaticInit::8DDCD0", 0x008DDCD0, &staticinit::Fn8DDCD0);
STATICINIT1_REVERSED("StaticInit::8DDD00", 0x008DDD00, &staticinit::Fn8DDD00);
STATICINIT1_REVERSED("StaticInit::8DDD30", 0x008DDD30, &staticinit::Fn8DDD30);
STATICINIT1_REVERSED("StaticInit::8DDD60", 0x008DDD60, &staticinit::Fn8DDD60);
STATICINIT1_REVERSED("StaticInit::8DDD90", 0x008DDD90, &staticinit::Fn8DDD90);
STATICINIT1_REVERSED("StaticInit::8DDDC0", 0x008DDDC0, &staticinit::Fn8DDDC0);
STATICINIT1_REVERSED("StaticInit::8DDDF0", 0x008DDDF0, &staticinit::Fn8DDDF0);
STATICINIT1_REVERSED("StaticInit::8DDE20", 0x008DDE20, &staticinit::Fn8DDE20);
STATICINIT1_REVERSED("StaticInit::8DDE50", 0x008DDE50, &staticinit::Fn8DDE50);
STATICINIT1_REVERSED("StaticInit::8DDE70", 0x008DDE70, &staticinit::Fn8DDE70);
STATICINIT1_REVERSED("StaticInit::8DDE90", 0x008DDE90, &staticinit::Fn8DDE90);
STATICINIT1_REVERSED("StaticInit::8DDEB0", 0x008DDEB0, &staticinit::Fn8DDEB0);
STATICINIT1_REVERSED("StaticInit::8DDED0", 0x008DDED0, &staticinit::Fn8DDED0);
STATICINIT1_REVERSED("StaticInit::8DDEE0", 0x008DDEE0, &staticinit::Fn8DDEE0);
STATICINIT1_REVERSED("StaticInit::8DDF10", 0x008DDF10, &staticinit::Fn8DDF10);
STATICINIT1_REVERSED("StaticInit::8DDF40", 0x008DDF40, &staticinit::Fn8DDF40);
STATICINIT1_REVERSED("StaticInit::8DDF70", 0x008DDF70, &staticinit::Fn8DDF70);
STATICINIT1_REVERSED("StaticInit::8DDFA0", 0x008DDFA0, &staticinit::Fn8DDFA0);
STATICINIT1_REVERSED("StaticInit::8DDFD0", 0x008DDFD0, &staticinit::Fn8DDFD0);
STATICINIT1_REVERSED("StaticInit::8DDFF0", 0x008DDFF0, &staticinit::Fn8DDFF0);
STATICINIT1_REVERSED("StaticInit::8DE010", 0x008DE010, &staticinit::Fn8DE010);
STATICINIT1_REVERSED("StaticInit::8DE030", 0x008DE030, &staticinit::Fn8DE030);
STATICINIT1_REVERSED("StaticInit::8DE050", 0x008DE050, &staticinit::Fn8DE050);
STATICINIT1_REVERSED("StaticInit::8DE070", 0x008DE070, &staticinit::Fn8DE070);
STATICINIT1_REVERSED("StaticInit::8DE0A0", 0x008DE0A0, &staticinit::Fn8DE0A0);
STATICINIT1_REVERSED("StaticInit::8DE0D0", 0x008DE0D0, &staticinit::Fn8DE0D0);
STATICINIT1_REVERSED("StaticInit::8DE100", 0x008DE100, &staticinit::Fn8DE100);
STATICINIT1_REVERSED("StaticInit::8DE130", 0x008DE130, &staticinit::Fn8DE130);
STATICINIT1_REVERSED("StaticInit::8DE160", 0x008DE160, &staticinit::Fn8DE160);
STATICINIT1_REVERSED("StaticInit::8DE190", 0x008DE190, &staticinit::Fn8DE190);
STATICINIT1_REVERSED("StaticInit::8DE1C0", 0x008DE1C0, &staticinit::Fn8DE1C0);
STATICINIT1_REVERSED("StaticInit::8DE1F0", 0x008DE1F0, &staticinit::Fn8DE1F0);
STATICINIT1_REVERSED("StaticInit::8DE220", 0x008DE220, &staticinit::Fn8DE220);
STATICINIT1_REVERSED("StaticInit::8DE250", 0x008DE250, &staticinit::Fn8DE250);
STATICINIT1_REVERSED("StaticInit::8DE280", 0x008DE280, &staticinit::Fn8DE280);
STATICINIT1_REVERSED("StaticInit::8DE2A0", 0x008DE2A0, &staticinit::Fn8DE2A0);
STATICINIT1_REVERSED("StaticInit::8DE2C0", 0x008DE2C0, &staticinit::Fn8DE2C0);
STATICINIT1_REVERSED("StaticInit::8DE2E0", 0x008DE2E0, &staticinit::Fn8DE2E0);
STATICINIT1_REVERSED("StaticInit::8DE300", 0x008DE300, &staticinit::Fn8DE300);
STATICINIT1_REVERSED("StaticInit::8DE330", 0x008DE330, &staticinit::Fn8DE330);
STATICINIT1_REVERSED("StaticInit::8DE350", 0x008DE350, &staticinit::Fn8DE350);
STATICINIT1_REVERSED("StaticInit::8DE380", 0x008DE380, &staticinit::Fn8DE380);
STATICINIT1_REVERSED("StaticInit::8DE3A0", 0x008DE3A0, &staticinit::Fn8DE3A0);
STATICINIT1_REVERSED("StaticInit::8DE3B0", 0x008DE3B0, &staticinit::Fn8DE3B0);
STATICINIT1_REVERSED("StaticInit::8DE3C0", 0x008DE3C0, &staticinit::Fn8DE3C0);
STATICINIT1_REVERSED("StaticInit::8DE3D0", 0x008DE3D0, &staticinit::Fn8DE3D0);
STATICINIT1_REVERSED("StaticInit::8DE3E0", 0x008DE3E0, &staticinit::Fn8DE3E0);
STATICINIT1_REVERSED("StaticInit::8DE3F0", 0x008DE3F0, &staticinit::Fn8DE3F0);
STATICINIT1_REVERSED("StaticInit::8DE400", 0x008DE400, &staticinit::Fn8DE400);
STATICINIT1_REVERSED("StaticInit::8DE410", 0x008DE410, &staticinit::Fn8DE410);
STATICINIT1_REVERSED("StaticInit::8DE420", 0x008DE420, &staticinit::Fn8DE420);
STATICINIT1_REVERSED("StaticInit::8DE460", 0x008DE460, &staticinit::Fn8DE460);
STATICINIT1_REVERSED("StaticInit::8DE480", 0x008DE480, &staticinit::Fn8DE480);
STATICINIT1_REVERSED("StaticInit::8DE4A0", 0x008DE4A0, &staticinit::Fn8DE4A0);
STATICINIT1_REVERSED("StaticInit::8DE4C0", 0x008DE4C0, &staticinit::Fn8DE4C0);
STATICINIT1_REVERSED("StaticInit::8DE4E0", 0x008DE4E0, &staticinit::Fn8DE4E0);
STATICINIT1_REVERSED("StaticInit::8DE500", 0x008DE500, &staticinit::Fn8DE500);
STATICINIT1_REVERSED("StaticInit::8DE510", 0x008DE510, &staticinit::Fn8DE510);
STATICINIT1_REVERSED("StaticInit::8DE520", 0x008DE520, &staticinit::Fn8DE520);
STATICINIT1_REVERSED("StaticInit::8DE530", 0x008DE530, &staticinit::Fn8DE530);
STATICINIT1_REVERSED("StaticInit::8DE570", 0x008DE570, &staticinit::Fn8DE570);
STATICINIT1_REVERSED("StaticInit::8DE590", 0x008DE590, &staticinit::Fn8DE590);
STATICINIT1_REVERSED("StaticInit::8DE5B0", 0x008DE5B0, &staticinit::Fn8DE5B0);
STATICINIT1_REVERSED("StaticInit::8DE5D0", 0x008DE5D0, &staticinit::Fn8DE5D0);
STATICINIT1_REVERSED("StaticInit::8DE5F0", 0x008DE5F0, &staticinit::Fn8DE5F0);
STATICINIT1_REVERSED("StaticInit::8DE610", 0x008DE610, &staticinit::Fn8DE610);
STATICINIT1_REVERSED("StaticInit::8DE630", 0x008DE630, &staticinit::Fn8DE630);
STATICINIT1_REVERSED("StaticInit::8DE650", 0x008DE650, &staticinit::Fn8DE650);
STATICINIT1_REVERSED("StaticInit::8DE670", 0x008DE670, &staticinit::Fn8DE670);
STATICINIT1_REVERSED("StaticInit::8DE690", 0x008DE690, &staticinit::Fn8DE690);
STATICINIT1_REVERSED("StaticInit::8DE6B0", 0x008DE6B0, &staticinit::Fn8DE6B0);
STATICINIT1_REVERSED("StaticInit::8DE6D0", 0x008DE6D0, &staticinit::Fn8DE6D0);
STATICINIT1_REVERSED("StaticInit::8DE6F0", 0x008DE6F0, &staticinit::Fn8DE6F0);
STATICINIT1_REVERSED("StaticInit::8DE710", 0x008DE710, &staticinit::Fn8DE710);
STATICINIT1_REVERSED("StaticInit::8DE730", 0x008DE730, &staticinit::Fn8DE730);
STATICINIT1_REVERSED("StaticInit::8DE750", 0x008DE750, &staticinit::Fn8DE750);
STATICINIT1_REVERSED("StaticInit::8DE790", 0x008DE790, &staticinit::Fn8DE790);
STATICINIT1_REVERSED("StaticInit::8DE7B0", 0x008DE7B0, &staticinit::Fn8DE7B0);
STATICINIT1_REVERSED("StaticInit::8DE7D0", 0x008DE7D0, &staticinit::Fn8DE7D0);
STATICINIT1_REVERSED("StaticInit::8DE7F0", 0x008DE7F0, &staticinit::Fn8DE7F0);
STATICINIT1_REVERSED("StaticInit::8DE840", 0x008DE840, &staticinit::Fn8DE840);
STATICINIT1_REVERSED("StaticInit::8DE890", 0x008DE890, &staticinit::Fn8DE890);
STATICINIT1_REVERSED("StaticInit::8DE8B0", 0x008DE8B0, &staticinit::Fn8DE8B0);
STATICINIT1_REVERSED("StaticInit::8DE8D0", 0x008DE8D0, &staticinit::Fn8DE8D0);
STATICINIT1_REVERSED("StaticInit::8DE8F0", 0x008DE8F0, &staticinit::Fn8DE8F0);
STATICINIT1_REVERSED("StaticInit::8DE910", 0x008DE910, &staticinit::Fn8DE910);
STATICINIT1_REVERSED("StaticInit::8DE970", 0x008DE970, &staticinit::Fn8DE970);
STATICINIT1_REVERSED("StaticInit::8DE990", 0x008DE990, &staticinit::Fn8DE990);
STATICINIT1_REVERSED("StaticInit::8DE9B0", 0x008DE9B0, &staticinit::Fn8DE9B0);
STATICINIT1_REVERSED("StaticInit::8DE9D0", 0x008DE9D0, &staticinit::Fn8DE9D0);
STATICINIT1_REVERSED("StaticInit::8DEA90", 0x008DEA90, &staticinit::Fn8DEA90);
STATICINIT1_REVERSED("StaticInit::8DEAB0", 0x008DEAB0, &staticinit::Fn8DEAB0);
STATICINIT1_REVERSED("StaticInit::8DEAD0", 0x008DEAD0, &staticinit::Fn8DEAD0);
STATICINIT1_REVERSED("StaticInit::8DEAF0", 0x008DEAF0, &staticinit::Fn8DEAF0);
STATICINIT1_REVERSED("StaticInit::8DEB10", 0x008DEB10, &staticinit::Fn8DEB10);
STATICINIT1_REVERSED("StaticInit::8DEB80", 0x008DEB80, &staticinit::Fn8DEB80);
STATICINIT1_REVERSED("StaticInit::8DEBE0", 0x008DEBE0, &staticinit::Fn8DEBE0);
STATICINIT1_REVERSED("StaticInit::8DEC00", 0x008DEC00, &staticinit::Fn8DEC00);
STATICINIT1_REVERSED("StaticInit::8DEC20", 0x008DEC20, &staticinit::Fn8DEC20);
STATICINIT1_REVERSED("StaticInit::8DEC40", 0x008DEC40, &staticinit::Fn8DEC40);
STATICINIT1_REVERSED("StaticInit::8DEC60", 0x008DEC60, &staticinit::Fn8DEC60);
STATICINIT1_REVERSED("StaticInit::8DEC80", 0x008DEC80, &staticinit::Fn8DEC80);
STATICINIT1_REVERSED("StaticInit::8DECA0", 0x008DECA0, &staticinit::Fn8DECA0);
STATICINIT1_REVERSED("StaticInit::8DECC0", 0x008DECC0, &staticinit::Fn8DECC0);
STATICINIT1_REVERSED("StaticInit::8DED10", 0x008DED10, &staticinit::Fn8DED10);
STATICINIT1_REVERSED("StaticInit::8DED30", 0x008DED30, &staticinit::Fn8DED30);
STATICINIT1_REVERSED("StaticInit::8DED50", 0x008DED50, &staticinit::Fn8DED50);
STATICINIT1_REVERSED("StaticInit::8DED70", 0x008DED70, &staticinit::Fn8DED70);
STATICINIT1_REVERSED("StaticInit::8DED90", 0x008DED90, &staticinit::Fn8DED90);
STATICINIT1_REVERSED("StaticInit::8DEDB0", 0x008DEDB0, &staticinit::Fn8DEDB0);
STATICINIT1_REVERSED("StaticInit::8DEDD0", 0x008DEDD0, &staticinit::Fn8DEDD0);
STATICINIT1_REVERSED("StaticInit::8DEDF0", 0x008DEDF0, &staticinit::Fn8DEDF0);
STATICINIT1_REVERSED("StaticInit::8DEE10", 0x008DEE10, &staticinit::Fn8DEE10);
STATICINIT1_REVERSED("StaticInit::8DEE30", 0x008DEE30, &staticinit::Fn8DEE30);
STATICINIT1_REVERSED("StaticInit::8DEE50", 0x008DEE50, &staticinit::Fn8DEE50);
STATICINIT1_REVERSED("StaticInit::8DEE70", 0x008DEE70, &staticinit::Fn8DEE70);
STATICINIT1_REVERSED("StaticInit::8DEEC0", 0x008DEEC0, &staticinit::Fn8DEEC0);
STATICINIT1_REVERSED("StaticInit::8DEEE0", 0x008DEEE0, &staticinit::Fn8DEEE0);
STATICINIT1_REVERSED("StaticInit::8DEF00", 0x008DEF00, &staticinit::Fn8DEF00);
STATICINIT1_REVERSED("StaticInit::8DEF20", 0x008DEF20, &staticinit::Fn8DEF20);
STATICINIT1_REVERSED("StaticInit::8DEF40", 0x008DEF40, &staticinit::Fn8DEF40);
STATICINIT1_REVERSED("StaticInit::8DEF50", 0x008DEF50, &staticinit::Fn8DEF50);
STATICINIT1_REVERSED("StaticInit::8DEF70", 0x008DEF70, &staticinit::Fn8DEF70);
STATICINIT1_REVERSED("StaticInit::8DEF90", 0x008DEF90, &staticinit::Fn8DEF90);
STATICINIT1_REVERSED("StaticInit::8DEFB0", 0x008DEFB0, &staticinit::Fn8DEFB0);
STATICINIT1_REVERSED("StaticInit::8DEFD0", 0x008DEFD0, &staticinit::Fn8DEFD0);
STATICINIT1_REVERSED("StaticInit::8DEFF0", 0x008DEFF0, &staticinit::Fn8DEFF0);
STATICINIT1_REVERSED("StaticInit::8DF010", 0x008DF010, &staticinit::Fn8DF010);
STATICINIT1_REVERSED("StaticInit::8DF030", 0x008DF030, &staticinit::Fn8DF030);
STATICINIT1_REVERSED("StaticInit::8DF050", 0x008DF050, &staticinit::Fn8DF050);
STATICINIT1_REVERSED("StaticInit::8DF070", 0x008DF070, &staticinit::Fn8DF070);
STATICINIT1_REVERSED("StaticInit::8DF090", 0x008DF090, &staticinit::Fn8DF090);
STATICINIT1_REVERSED("StaticInit::8DF0B0", 0x008DF0B0, &staticinit::Fn8DF0B0);
STATICINIT1_REVERSED("StaticInit::8DF0D0", 0x008DF0D0, &staticinit::Fn8DF0D0);
STATICINIT1_REVERSED("StaticInit::8DF0F0", 0x008DF0F0, &staticinit::Fn8DF0F0);
STATICINIT1_REVERSED("StaticInit::8DF1B0", 0x008DF1B0, &staticinit::Fn8DF1B0);
STATICINIT1_REVERSED("StaticInit::8DF1D0", 0x008DF1D0, &staticinit::Fn8DF1D0);
STATICINIT1_REVERSED("StaticInit::8DF1F0", 0x008DF1F0, &staticinit::Fn8DF1F0);
STATICINIT1_REVERSED("StaticInit::8DF210", 0x008DF210, &staticinit::Fn8DF210);
STATICINIT1_REVERSED("StaticInit::8DF2C0", 0x008DF2C0, &staticinit::Fn8DF2C0);
STATICINIT1_REVERSED("StaticInit::8DF2D0", 0x008DF2D0, &staticinit::Fn8DF2D0);
STATICINIT1_REVERSED("StaticInit::8DF2F0", 0x008DF2F0, &staticinit::Fn8DF2F0);
STATICINIT1_REVERSED("StaticInit::8DF310", 0x008DF310, &staticinit::Fn8DF310);
STATICINIT1_REVERSED("StaticInit::8DF330", 0x008DF330, &staticinit::Fn8DF330);
STATICINIT1_REVERSED("StaticInit::8DF350", 0x008DF350, &staticinit::Fn8DF350);
STATICINIT1_REVERSED("StaticInit::8DF360", 0x008DF360, &staticinit::Fn8DF360);
STATICINIT1_REVERSED("StaticInit::8DF370", 0x008DF370, &staticinit::Fn8DF370);
STATICINIT1_REVERSED("StaticInit::8DF390", 0x008DF390, &staticinit::Fn8DF390);
STATICINIT1_REVERSED("StaticInit::8DF3B0", 0x008DF3B0, &staticinit::Fn8DF3B0);
STATICINIT1_REVERSED("StaticInit::8DF3D0", 0x008DF3D0, &staticinit::Fn8DF3D0);
STATICINIT1_REVERSED("StaticInit::8DF3F0", 0x008DF3F0, &staticinit::Fn8DF3F0);
STATICINIT1_REVERSED("StaticInit::8DF410", 0x008DF410, &staticinit::Fn8DF410);
STATICINIT1_REVERSED("StaticInit::8DF430", 0x008DF430, &staticinit::Fn8DF430);
STATICINIT1_REVERSED("StaticInit::8DF450", 0x008DF450, &staticinit::Fn8DF450);
STATICINIT1_REVERSED("StaticInit::8DF4D0", 0x008DF4D0, &staticinit::Fn8DF4D0);
STATICINIT1_REVERSED("StaticInit::8DF4F0", 0x008DF4F0, &staticinit::Fn8DF4F0);
STATICINIT1_REVERSED("StaticInit::8DF510", 0x008DF510, &staticinit::Fn8DF510);
STATICINIT1_REVERSED("StaticInit::8DF530", 0x008DF530, &staticinit::Fn8DF530);
STATICINIT1_REVERSED("StaticInit::8DF550", 0x008DF550, &staticinit::Fn8DF550);
STATICINIT1_REVERSED("StaticInit::8DF570", 0x008DF570, &staticinit::Fn8DF570);
STATICINIT1_REVERSED("StaticInit::8DF590", 0x008DF590, &staticinit::Fn8DF590);
STATICINIT1_REVERSED("StaticInit::8DF5B0", 0x008DF5B0, &staticinit::Fn8DF5B0);
STATICINIT1_REVERSED("StaticInit::8DF5D0", 0x008DF5D0, &staticinit::Fn8DF5D0);
STATICINIT1_REVERSED("StaticInit::8DF5F0", 0x008DF5F0, &staticinit::Fn8DF5F0);
STATICINIT1_REVERSED("StaticInit::8DF610", 0x008DF610, &staticinit::Fn8DF610);
STATICINIT1_REVERSED("StaticInit::8DF630", 0x008DF630, &staticinit::Fn8DF630);
STATICINIT1_REVERSED("StaticInit::8DF650", 0x008DF650, &staticinit::Fn8DF650);
STATICINIT1_REVERSED("StaticInit::8DF670", 0x008DF670, &staticinit::Fn8DF670);
STATICINIT1_REVERSED("StaticInit::8DF690", 0x008DF690, &staticinit::Fn8DF690);
STATICINIT1_REVERSED("StaticInit::8DF6B0", 0x008DF6B0, &staticinit::Fn8DF6B0);
STATICINIT1_REVERSED("StaticInit::8DF6D0", 0x008DF6D0, &staticinit::Fn8DF6D0);
STATICINIT1_REVERSED("StaticInit::8DF6F0", 0x008DF6F0, &staticinit::Fn8DF6F0);
STATICINIT1_REVERSED("StaticInit::8DF710", 0x008DF710, &staticinit::Fn8DF710);
STATICINIT1_REVERSED("StaticInit::8DF730", 0x008DF730, &staticinit::Fn8DF730);
STATICINIT1_REVERSED("StaticInit::8DF750", 0x008DF750, &staticinit::Fn8DF750);
STATICINIT1_REVERSED("StaticInit::8DF770", 0x008DF770, &staticinit::Fn8DF770);
STATICINIT1_REVERSED("StaticInit::8DF790", 0x008DF790, &staticinit::Fn8DF790);
STATICINIT1_REVERSED("StaticInit::8DF7B0", 0x008DF7B0, &staticinit::Fn8DF7B0);
STATICINIT1_REVERSED("StaticInit::8DF7C0", 0x008DF7C0, &staticinit::Fn8DF7C0);
STATICINIT1_REVERSED("StaticInit::8DF7D0", 0x008DF7D0, &staticinit::Fn8DF7D0);
STATICINIT1_REVERSED("StaticInit::8DF7F0", 0x008DF7F0, &staticinit::Fn8DF7F0);
STATICINIT1_REVERSED("StaticInit::8DF810", 0x008DF810, &staticinit::Fn8DF810);
STATICINIT1_REVERSED("StaticInit::8DF830", 0x008DF830, &staticinit::Fn8DF830);
STATICINIT1_REVERSED("StaticInit::8DF850", 0x008DF850, &staticinit::Fn8DF850);
STATICINIT1_REVERSED("StaticInit::8DF870", 0x008DF870, &staticinit::Fn8DF870);
STATICINIT1_REVERSED("StaticInit::8DF890", 0x008DF890, &staticinit::Fn8DF890);
STATICINIT1_REVERSED("StaticInit::8DF8B0", 0x008DF8B0, &staticinit::Fn8DF8B0);
STATICINIT1_REVERSED("StaticInit::8DF8D0", 0x008DF8D0, &staticinit::Fn8DF8D0);
STATICINIT1_REVERSED("StaticInit::8DF8F0", 0x008DF8F0, &staticinit::Fn8DF8F0);
STATICINIT1_REVERSED("StaticInit::8DF910", 0x008DF910, &staticinit::Fn8DF910);
STATICINIT1_REVERSED("StaticInit::8DF930", 0x008DF930, &staticinit::Fn8DF930);
STATICINIT1_REVERSED("StaticInit::8DF950", 0x008DF950, &staticinit::Fn8DF950);
STATICINIT1_REVERSED("StaticInit::8DF970", 0x008DF970, &staticinit::Fn8DF970);
STATICINIT1_REVERSED("StaticInit::8DF990", 0x008DF990, &staticinit::Fn8DF990);
STATICINIT1_REVERSED("StaticInit::8DF9B0", 0x008DF9B0, &staticinit::Fn8DF9B0);
STATICINIT1_REVERSED("StaticInit::8DF9D0", 0x008DF9D0, &staticinit::Fn8DF9D0);
STATICINIT1_REVERSED("StaticInit::8DF9F0", 0x008DF9F0, &staticinit::Fn8DF9F0);
STATICINIT1_REVERSED("StaticInit::8DFA10", 0x008DFA10, &staticinit::Fn8DFA10);
STATICINIT1_REVERSED("StaticInit::8DFA30", 0x008DFA30, &staticinit::Fn8DFA30);
STATICINIT1_REVERSED("StaticInit::8DFA50", 0x008DFA50, &staticinit::Fn8DFA50);
STATICINIT1_REVERSED("StaticInit::8DFA70", 0x008DFA70, &staticinit::Fn8DFA70);
STATICINIT1_REVERSED("StaticInit::8DFA90", 0x008DFA90, &staticinit::Fn8DFA90);
STATICINIT1_REVERSED("StaticInit::8DFAB0", 0x008DFAB0, &staticinit::Fn8DFAB0);
STATICINIT1_REVERSED("StaticInit::8DFAD0", 0x008DFAD0, &staticinit::Fn8DFAD0);
STATICINIT1_REVERSED("StaticInit::8DFAF0", 0x008DFAF0, &staticinit::Fn8DFAF0);
STATICINIT1_REVERSED("StaticInit::8DFB10", 0x008DFB10, &staticinit::Fn8DFB10);
STATICINIT1_REVERSED("StaticInit::8DFB30", 0x008DFB30, &staticinit::Fn8DFB30);
STATICINIT1_REVERSED("StaticInit::8DFB50", 0x008DFB50, &staticinit::Fn8DFB50);
STATICINIT1_REVERSED("StaticInit::8DFB70", 0x008DFB70, &staticinit::Fn8DFB70);
STATICINIT1_REVERSED("StaticInit::8DFB90", 0x008DFB90, &staticinit::Fn8DFB90);
STATICINIT1_REVERSED("StaticInit::8DFBB0", 0x008DFBB0, &staticinit::Fn8DFBB0);
STATICINIT1_REVERSED("StaticInit::8DFBD0", 0x008DFBD0, &staticinit::Fn8DFBD0);
STATICINIT1_REVERSED("StaticInit::8DFBF0", 0x008DFBF0, &staticinit::Fn8DFBF0);
STATICINIT1_REVERSED("StaticInit::8DFC10", 0x008DFC10, &staticinit::Fn8DFC10);
STATICINIT1_REVERSED("StaticInit::8DFC30", 0x008DFC30, &staticinit::Fn8DFC30);
STATICINIT1_REVERSED("StaticInit::8DFC50", 0x008DFC50, &staticinit::Fn8DFC50);
STATICINIT1_REVERSED("StaticInit::8DFC70", 0x008DFC70, &staticinit::Fn8DFC70);
STATICINIT1_REVERSED("StaticInit::8DFC90", 0x008DFC90, &staticinit::Fn8DFC90);
STATICINIT1_REVERSED("StaticInit::8DFCB0", 0x008DFCB0, &staticinit::Fn8DFCB0);
STATICINIT1_REVERSED("StaticInit::8DFCD0", 0x008DFCD0, &staticinit::Fn8DFCD0);
STATICINIT1_REVERSED("StaticInit::8DFCF0", 0x008DFCF0, &staticinit::Fn8DFCF0);
STATICINIT1_REVERSED("StaticInit::8DFD10", 0x008DFD10, &staticinit::Fn8DFD10);
STATICINIT1_REVERSED("StaticInit::8DFD30", 0x008DFD30, &staticinit::Fn8DFD30);
STATICINIT1_REVERSED("StaticInit::8DFD50", 0x008DFD50, &staticinit::Fn8DFD50);
STATICINIT1_REVERSED("StaticInit::8DFD70", 0x008DFD70, &staticinit::Fn8DFD70);
STATICINIT1_REVERSED("StaticInit::8DFD90", 0x008DFD90, &staticinit::Fn8DFD90);
STATICINIT1_REVERSED("StaticInit::8DFDB0", 0x008DFDB0, &staticinit::Fn8DFDB0);
STATICINIT1_REVERSED("StaticInit::8DFDD0", 0x008DFDD0, &staticinit::Fn8DFDD0);
STATICINIT1_REVERSED("StaticInit::8DFDF0", 0x008DFDF0, &staticinit::Fn8DFDF0);
STATICINIT1_REVERSED("StaticInit::8DFE10", 0x008DFE10, &staticinit::Fn8DFE10);
STATICINIT1_REVERSED("StaticInit::8DFE30", 0x008DFE30, &staticinit::Fn8DFE30);
STATICINIT1_REVERSED("StaticInit::8DFE50", 0x008DFE50, &staticinit::Fn8DFE50);
STATICINIT1_REVERSED("StaticInit::8DFE70", 0x008DFE70, &staticinit::Fn8DFE70);
STATICINIT1_REVERSED("StaticInit::8DFE90", 0x008DFE90, &staticinit::Fn8DFE90);
STATICINIT1_REVERSED("StaticInit::8DFEB0", 0x008DFEB0, &staticinit::Fn8DFEB0);
STATICINIT1_REVERSED("StaticInit::8DFED0", 0x008DFED0, &staticinit::Fn8DFED0);
STATICINIT1_REVERSED("StaticInit::8DFEF0", 0x008DFEF0, &staticinit::Fn8DFEF0);
STATICINIT1_REVERSED("StaticInit::8DFF10", 0x008DFF10, &staticinit::Fn8DFF10);
STATICINIT1_REVERSED("StaticInit::8DFF30", 0x008DFF30, &staticinit::Fn8DFF30);
STATICINIT1_REVERSED("StaticInit::8DFF50", 0x008DFF50, &staticinit::Fn8DFF50);
STATICINIT1_REVERSED("StaticInit::8DFF70", 0x008DFF70, &staticinit::Fn8DFF70);
STATICINIT1_REVERSED("StaticInit::8DFF90", 0x008DFF90, &staticinit::Fn8DFF90);
STATICINIT1_REVERSED("StaticInit::8DFFB0", 0x008DFFB0, &staticinit::Fn8DFFB0);
STATICINIT1_REVERSED("StaticInit::8DFFD0", 0x008DFFD0, &staticinit::Fn8DFFD0);
STATICINIT1_REVERSED("StaticInit::8DFFF0", 0x008DFFF0, &staticinit::Fn8DFFF0);
STATICINIT1_REVERSED("StaticInit::8E0010", 0x008E0010, &staticinit::Fn8E0010);
STATICINIT1_REVERSED("StaticInit::8E0030", 0x008E0030, &staticinit::Fn8E0030);
STATICINIT1_REVERSED("StaticInit::8E0050", 0x008E0050, &staticinit::Fn8E0050);
STATICINIT1_REVERSED("StaticInit::8E0070", 0x008E0070, &staticinit::Fn8E0070);
STATICINIT1_REVERSED("StaticInit::8E0090", 0x008E0090, &staticinit::Fn8E0090);
STATICINIT1_REVERSED("StaticInit::8E00B0", 0x008E00B0, &staticinit::Fn8E00B0);
STATICINIT1_REVERSED("StaticInit::8E00D0", 0x008E00D0, &staticinit::Fn8E00D0);
STATICINIT1_REVERSED("StaticInit::8E00F0", 0x008E00F0, &staticinit::Fn8E00F0);
STATICINIT1_REVERSED("StaticInit::8E0110", 0x008E0110, &staticinit::Fn8E0110);
STATICINIT1_REVERSED("StaticInit::8E0130", 0x008E0130, &staticinit::Fn8E0130);
STATICINIT1_REVERSED("StaticInit::8E0150", 0x008E0150, &staticinit::Fn8E0150);
STATICINIT1_REVERSED("StaticInit::8E0170", 0x008E0170, &staticinit::Fn8E0170);
STATICINIT1_REVERSED("StaticInit::8E0190", 0x008E0190, &staticinit::Fn8E0190);
STATICINIT1_REVERSED("StaticInit::8E01B0", 0x008E01B0, &staticinit::Fn8E01B0);
STATICINIT1_REVERSED("StaticInit::8E01D0", 0x008E01D0, &staticinit::Fn8E01D0);
