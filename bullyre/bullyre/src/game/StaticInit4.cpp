// The exe's static initializers (the C runtime runs the table at 0x8FC350 before WinMain:
// globals constructed, their destructors given to atexit) and those destructors, transcribed
// from their code by a script (each a few calls with constant arguments, a dword copied or a
// float constant computed). Calls go through the game's addresses, so the registry's modes
// apply; the float constants are computed in double, as at start-up (the x87 at its default
// 53 bits, before Direct3D sets 24). StaticInit.h has the helpers.

#include "game/StaticInit.h"

namespace staticinit {
namespace {

// 0x8EC050 (initializer)
void __cdecl Fn8EC050() {
    Dword(0x020E2AAC) = Dword(0x00951028);
    Dword(0x020E2AB0) = Dword(0x00951028);
    Dword(0x020E2AB4) = Dword(0x00951028);
    Dword(0x020E2AB8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F87E0));
}

// 0x8EC080 (initializer)
void __cdecl Fn8EC080() {
    Thiscall(0x00765410, 0x020E2ABCu, 0x009528CCu /* "NiFloatsExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EC0A0 (initializer)
void __cdecl Fn8EC0A0() {
    Thiscall(0x00765410, 0x020E2AF0u, 0x00952AC4u /* "NiAlphaProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC0C0 (initializer)
void __cdecl Fn8EC0C0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8830));
}

// 0x8EC0D0 (initializer)
void __cdecl Fn8EC0D0() {
    Thiscall(0x00765410, 0x020E2B28u, 0x00952B70u /* "NiZBufferProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC0F0 (initializer)
void __cdecl Fn8EC0F0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8860));
}

// 0x8EC100 (initializer)
void __cdecl Fn8EC100() {
    Thiscall(0x00765410, 0x020E2B60u, 0x00952BE8u /* "NiIntegersExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EC120 (initializer)
void __cdecl Fn8EC120() {
    Thiscall(0x00765410, 0x020E2B94u, 0x00952C5Cu /* "NiSkinPartition" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC140 (initializer)
void __cdecl Fn8EC140() {
    Thiscall(0x00765410, 0x020E2BC8u, 0x00952CFCu /* "NiTriBasedGeomData" */, 0x020E30A8u);  // NiRTTI::NiRTTI
}

// 0x8EC160 (initializer)
void __cdecl Fn8EC160() {
    Thiscall(0x00765410, 0x020E2C00u, 0x00952E00u /* "NiDynamicEffect" */, 0x020E2514u);  // NiRTTI::NiRTTI
}

// 0x8EC180 (initializer)
void __cdecl Fn8EC180() {
    Thiscall(0x00765410, 0x020E2C34u, 0x00951604u /* "NiShadowGenerator" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC1A0 (initializer)
void __cdecl Fn8EC1A0() {
    Thiscall(0x00765410, 0x020E2C68u, 0x00952F14u /* "NiLight" */, 0x020E2C00u);  // NiRTTI::NiRTTI
}

// 0x8EC1C0 (initializer)
void __cdecl Fn8EC1C0() {
    Thiscall(0x00765410, 0x020E2C9Cu, 0x00952FE0u /* "NiSpotLight" */, 0x020E2CD0u);  // NiRTTI::NiRTTI
}

// 0x8EC1E0 (initializer)
void __cdecl Fn8EC1E0() {
    Thiscall(0x00765410, 0x020E2CD0u, 0x0095309Cu /* "NiPointLight" */, 0x020E2C68u);  // NiRTTI::NiRTTI
}

// 0x8EC200 (initializer)
void __cdecl Fn8EC200() {
    Thiscall(0x00765410, 0x020E2D04u, 0x00953138u /* "NiMaterialProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC220 (initializer)
void __cdecl Fn8EC220() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8890));
}

// 0x8EC280 (initializer)
void __cdecl Fn8EC280() {
    Thiscall(0x00765410, 0x020E2D50u, 0x009531B8u /* "NiAccumulator" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC2A0 (initializer)
void __cdecl Fn8EC2A0() {
    Thiscall(0x00765410, 0x020E2D84u, 0x009531DCu /* "NiRenderClick" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC2C0 (initializer)
void __cdecl Fn8EC2C0() {
    Dword(0x020E2DB8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F88C0));
}

// 0x8EC2E0 (initializer)
void __cdecl Fn8EC2E0() {
    Thiscall(0x00765410, 0x020E2DBCu, 0x009531ECu /* "NiViewRenderClick" */, 0x020E2D84u);  // NiRTTI::NiRTTI
}

// 0x8EC300 (initializer)
void __cdecl Fn8EC300() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8900));
}

// 0x8EC310 (initializer)
void __cdecl Fn8EC310() {
    Thiscall(0x00765410, 0x020E2DF4u, 0x00953288u /* "NiSkinInstance" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC330 (initializer)
void __cdecl Fn8EC330() {
    Thiscall(0x00765410, 0x020E2E28u, 0x009534E8u /* "NiRenderListProcessor" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC350 (initializer)
void __cdecl Fn8EC350() {
    Thiscall(0x00765410, 0x020E2E5Cu, 0x00953500u /* "NiMaterialSwapProcessor" */, 0x020E2E28u);  // NiRTTI::NiRTTI
}

// 0x8EC370 (initializer)
void __cdecl Fn8EC370() {
    Thiscall(0x00765410, 0x020E2E90u, 0x00953518u /* "NiShadowRenderClick" */, 0x020E2DBCu);  // NiRTTI::NiRTTI
}

// 0x8EC390 (initializer)
void __cdecl Fn8EC390() {
    Thiscall(0x00765410, 0x020E2EC4u, 0x0095352Cu /* "NiBinaryExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8EC3B0 (initializer)
void __cdecl Fn8EC3B0() {
    Thiscall(0x00765410, 0x020E2EF8u, 0x0095359Cu /* "NiFogProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC3D0 (initializer)
void __cdecl Fn8EC3D0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8930));
}

// 0x8EC3E0 (initializer)
void __cdecl Fn8EC3E0() {
    Thiscall(0x00765410, 0x020E2F30u, 0x009535B8u /* "NiShadeProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC400 (initializer)
void __cdecl Fn8EC400() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8960));
}

// 0x8EC410 (initializer)
void __cdecl Fn8EC410() {
    Thiscall(0x00765410, 0x020E2F68u, 0x00953704u /* "NiStencilProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC430 (initializer)
void __cdecl Fn8EC430() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8990));
}

// 0x8EC440 (initializer)
void __cdecl Fn8EC440() {
    Thiscall(0x00765410, 0x020E2FA0u, 0x00953770u /* "NiDynamicTexture" */, 0x020E2184u);  // NiRTTI::NiRTTI
}

// 0x8EC460 (initializer)
void __cdecl Fn8EC460() {
    Thiscall(0x00765410, 0x020E2FD4u, 0x00953824u /* "NiTriBasedGeom" */, 0x020E25B4u);  // NiRTTI::NiRTTI
}

// 0x8EC480 (initializer)
void __cdecl Fn8EC480() {
    Thiscall(0x00765410, 0x020E3008u, 0x009538B8u /* "NiVertexColorProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8EC4A0 (initializer)
void __cdecl Fn8EC4A0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F89C0));
}

// 0x8EC4B0 (initializer)
void __cdecl Fn8EC4B0() {
    Thiscall(0x00765410, 0x020E3040u, 0x00953968u /* "NiLines" */, 0x020E25B4u);  // NiRTTI::NiRTTI
}

// 0x8EC4D0 (initializer)
void __cdecl Fn8EC4D0() {
    Thiscall(0x00765410, 0x020E3074u, 0x009539D4u /* "NiLinesData" */, 0x020E30A8u);  // NiRTTI::NiRTTI
}

// 0x8EC4F0 (initializer)
void __cdecl Fn8EC4F0() {
    Thiscall(0x00765410, 0x020E30A8u, 0x00953B10u /* "NiGeometryData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC510 (initializer)
void __cdecl Fn8EC510() {
    Stdcall(Dword(0x008FC084), 0x020E30DCu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F89F0));
}

// 0x8EC530 (initializer)
void __cdecl Fn8EC530() {
    Thiscall(0x00765410, 0x020E30FCu, 0x00953BBCu /* "NiTriShape" */, 0x020E2FD4u);  // NiRTTI::NiRTTI
}

// 0x8EC550 (initializer)
void __cdecl Fn8EC550() {
    Thiscall(0x00765410, 0x020E3130u, 0x00953C54u /* "NiTriShapeData" */, 0x020E2BC8u);  // NiRTTI::NiRTTI
}

// 0x8EC570 (initializer)
void __cdecl Fn8EC570() {
    Thiscall(0x00765410, 0x020E3164u, 0x00953CD0u /* "NiShader" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC590 (initializer)
void __cdecl Fn8EC590() {
    Thiscall(0x00765410, 0x020E3198u, 0x00953CDCu /* "NiShader::NiShaderInstanceDescriptor" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC5B0 (initializer)
void __cdecl Fn8EC5B0() {
    Dword(0x020E31CC) = Dword(0x00951028);
    Dword(0x020E31D0) = Dword(0x00951028);
    Dword(0x020E31D4) = Dword(0x00951028);
    Dword(0x020E31D8) = Dword(0x00951028);
    Dword(0x020E31DC) = Dword(0x00951028);
    Dword(0x020E31E0) = Dword(0x00951028);
    Dword(0x020E31E4) = Dword(0x00951028);
    Dword(0x020E31E8) = Dword(0x00951028);
    Dword(0x020E31EC) = Dword(0x00951028);
    Dword(0x020E31F0) = Dword(0x00951028);
    Dword(0x020E31F4) = Dword(0x00951028);
    Dword(0x020E31F8) = Dword(0x00951028);
    Dword(0x020E31FC) = Dword(0x00951028);
    Dword(0x020E3200) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8A00));
}

// 0x8EC610 (initializer)
void __cdecl Fn8EC610() {
    Thiscall(0x00765410, 0x020E3204u, 0x00953DD0u /* "NiShaderDeclaration" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC630 (initializer)
void __cdecl Fn8EC630() {
    Thiscall(0x00765410, 0x020E3244u, 0x00953E58u /* "NiShaderTimeController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8EC650 (initializer)
void __cdecl Fn8EC650() {
    Thiscall(0x00765410, 0x020E3278u, 0x00953E70u /* "NiTimeSyncController" */, 0x020E3244u);  // NiRTTI::NiRTTI
}

// 0x8EC670 (initializer)
void __cdecl Fn8EC670() {
    Thiscall(0x00765410, 0x020E32BCu, 0x00953FDCu /* "NiTimeController" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC690 (initializer)
void __cdecl Fn8EC690() {
    Stdcall(Dword(0x008FC084), 0x020E32F0u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8A50));
}

// 0x8EC720 (initializer)
void __cdecl Fn8EC720() {
    Thiscall(0x00765410, 0x020E3480u, 0x00954CF0u /* "NiAmbientLight" */, 0x020E2C68u);  // NiRTTI::NiRTTI
}

// 0x8EC740 (initializer)
void __cdecl Fn8EC740() {
    Thiscall(0x00765410, 0x020E34C0u, 0x00955248u /* "NiFragmentMaterial" */, 0x020E2708u);  // NiRTTI::NiRTTI
}

// 0x8EC760 (initializer)
void __cdecl Fn8EC760() {
    Thiscall(0x00765410, 0x020E34F4u, 0x00954F90u /* "NiScreenElementsData" */, 0x020E3130u);  // NiRTTI::NiRTTI
}

// 0x8EC780 (initializer)
void __cdecl Fn8EC780() {
    Thiscall(0x00765410, 0x020E3528u, 0x00955FACu /* "NiStandardMaterial" */, 0x020E34C0u);  // NiRTTI::NiRTTI
}

// 0x8EC7A0 (initializer)
void __cdecl Fn8EC7A0() {
    Thiscall(0x00765410, 0x020E355Cu, 0x00954FA8u /* "NiScreenElements" */, 0x020E30FCu);  // NiRTTI::NiRTTI
}

// 0x8EC7C0 (initializer)
void __cdecl Fn8EC7C0() {
    Thiscall(0x00765410, 0x020E3590u, 0x0095F77Cu /* "NiRenderView" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC7E0 (initializer)
void __cdecl Fn8EC7E0() {
    Dword(0x020E35C4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8A60));
}

// 0x8EC800 (initializer)
void __cdecl Fn8EC800() {
    Thiscall(0x00765410, 0x020E35C8u, 0x0095F78Cu /* "Ni3DRenderView" */, 0x020E3590u);  // NiRTTI::NiRTTI
}

// 0x8EC820 (initializer)
void __cdecl Fn8EC820() {
    Thiscall(0x00765410, 0x020E35FCu, 0x0095F7D0u /* "NiRenderClickValidator" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC840 (initializer)
void __cdecl Fn8EC840() {
    Thiscall(0x00765410, 0x020E3630u, 0x0095F7E8u /* "NiShadowClickValidator" */, 0x020E35FCu);  // NiRTTI::NiRTTI
}

// 0x8EC860 (initializer)
void __cdecl Fn8EC860() {
    Thiscall(0x00765410, 0x020E3664u, 0x0095F820u /* "NiShadowClickGenerator" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC880 (initializer)
void __cdecl Fn8EC880() {
    Thiscall(0x00765410, 0x020E3698u, 0x0095F88Cu /* "NiDepthStencilBuffer" */, 0x020E3804u);  // NiRTTI::NiRTTI
}

// 0x8EC8A0 (initializer)
void __cdecl Fn8EC8A0() {
    Thiscall(0x00765410, 0x020E36CCu, 0x009515A8u /* "NiPointShadowWriteMaterial" */, 0x020E34C0u);  // NiRTTI::NiRTTI
}

// 0x8EC8C0 (initializer)
void __cdecl Fn8EC8C0() {
    Thiscall(0x00765410, 0x020E3700u, 0x009515C4u /* "NiSpotShadowWriteMaterial" */, 0x020E34C0u);  // NiRTTI::NiRTTI
}

// 0x8EC8E0 (initializer)
void __cdecl Fn8EC8E0() {
    Thiscall(0x00765410, 0x020E3734u, 0x009515E0u /* "NiDirectionalShadowWriteMaterial" */, 0x020E34C0u);  // NiRTTI::NiRTTI
}

// 0x8EC900 (initializer)
void __cdecl Fn8EC900() {
    Thiscall(0x00765410, 0x020E3768u, 0x008FF798u /* "NiDefaultShadowClickGenerator" */, 0x020E3664u);  // NiRTTI::NiRTTI
}

// 0x8EC920 (initializer)
void __cdecl Fn8EC920() {
    Thiscall(0x00765410, 0x020E379Cu, 0x00951654u /* "NiVSMShadowTechnique" */, 0x020E37D0u);  // NiRTTI::NiRTTI
}

// 0x8EC940 (initializer)
void __cdecl Fn8EC940() {
    Thiscall(0x00765410, 0x020E37D0u, 0x009606FCu /* "NiShadowTechnique" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EC960 (initializer)
void __cdecl Fn8EC960() {
    Thiscall(0x00765410, 0x020E3804u, 0x00960764u /* "Ni2DBuffer" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC980 (initializer)
void __cdecl Fn8EC980() {
    Thiscall(0x00765410, 0x020E3838u, 0x00960770u /* "NiCollisionObject" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8EC9A0 (initializer)
void __cdecl Fn8EC9A0() {
    Thiscall(0x00765410, 0x020E386Cu, 0x009607FCu /* "NiRenderedCubeMap" */, 0x020E24E0u);  // NiRTTI::NiRTTI
}

// 0x8EC9C0 (initializer)
void __cdecl Fn8EC9C0() {
    Thiscall(0x00765410, 0x020E38A0u, 0x00960830u /* "NiSingleShaderMaterial" */, 0x020E2708u);  // NiRTTI::NiRTTI
}

// 0x8EC9E0 (initializer)
void __cdecl Fn8EC9E0() {
    Thiscall(0x00765410, 0x020E38D4u, 0x00954EA0u /* "NiVertWeightsExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8ECA00 (initializer)
void __cdecl Fn8ECA00() {
    Thiscall(0x00765410, 0x020E3908u, 0x00960898u /* "NiRenderStep" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8ECA20 (initializer)
void __cdecl Fn8ECA20() {
    Dword(0x020E393C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8AA0));
}

// 0x8ECA40 (initializer)
void __cdecl Fn8ECA40() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8AE0));
}

// 0x8ECA50 (initializer)
void __cdecl Fn8ECA50() {
    Thiscall(0x00765410, 0x020E3958u, 0x00955014u /* "NiPalette" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECA70 (initializer)
void __cdecl Fn8ECA70() {
    Stdcall(Dword(0x008FC084), 0x020E398Cu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8B10));
}

// 0x8ECA90 (initializer)
void __cdecl Fn8ECA90() {
    Thiscall(0x00765410, 0x020E39ACu, 0x00955040u /* "NiDirectionalLight" */, 0x020E2C68u);  // NiRTTI::NiRTTI
}

// 0x8ECAB0 (initializer)
void __cdecl Fn8ECAB0() {
    Thiscall(0x00765410, 0x020E39F4u, 0x00954E4Cu /* "NiScreenGeometryData" */, 0x020E3130u);  // NiRTTI::NiRTTI
}

// 0x8ECAD0 (initializer)
void __cdecl Fn8ECAD0() {
    Thiscall(0x00765410, 0x020E3A28u, 0x00954E64u /* "NiScreenGeometry" */, 0x020E30FCu);  // NiRTTI::NiRTTI
}

// 0x8ECAF0 (initializer)
void __cdecl Fn8ECAF0() {
    Thiscall(0x00765410, 0x020E3A5Cu, 0x00954E3Cu /* "NiScreenPolygon" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECB10 (initializer)
void __cdecl Fn8ECB10() {
    Thiscall(0x00765410, 0x020E3A90u, 0x00954E78u /* "NiScreenSpaceCamera" */, 0x020E273Cu);  // NiRTTI::NiRTTI
}

// 0x8ECB30 (initializer)
void __cdecl Fn8ECB30() {
    Thiscall(0x00765410, 0x020E3AC4u, 0x00954E8Cu /* "NiWireframeProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8ECB50 (initializer)
void __cdecl Fn8ECB50() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8B20));
}

// 0x8ECB60 (initializer)
void __cdecl Fn8ECB60() {
    Thiscall(0x00765410, 0x020E3AFCu, 0x00954EB8u /* "NiVectorExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8ECB80 (initializer)
void __cdecl Fn8ECB80() {
    Thiscall(0x00765410, 0x020E3B30u, 0x00954ECCu /* "NiTriStripsData" */, 0x020E2BC8u);  // NiRTTI::NiRTTI
}

// 0x8ECBA0 (initializer)
void __cdecl Fn8ECBA0() {
    Thiscall(0x00765410, 0x020E3B64u, 0x00954EDCu /* "NiTriStrips" */, 0x020E2FD4u);  // NiRTTI::NiRTTI
}

// 0x8ECBC0 (initializer)
void __cdecl Fn8ECBC0() {
    Thiscall(0x00765410, 0x020E3B98u, 0x00954EE8u /* "NiTriShapeDynamicData" */, 0x020E3130u);  // NiRTTI::NiRTTI
}

// 0x8ECBE0 (initializer)
void __cdecl Fn8ECBE0() {
    Thiscall(0x00765410, 0x020E3BCCu, 0x00954F00u /* "NiSwitchStringExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8ECC00 (initializer)
void __cdecl Fn8ECC00() {
    Thiscall(0x00765410, 0x020E3C00u, 0x00954F18u /* "NiSwitchNode" */, 0x020E2548u);  // NiRTTI::NiRTTI
}

// 0x8ECC20 (initializer)
void __cdecl Fn8ECC20() {
    Thiscall(0x00765410, 0x020E3C34u, 0x00954F28u /* "NiStringsExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8ECC40 (initializer)
void __cdecl Fn8ECC40() {
    Thiscall(0x00765410, 0x020E3C68u, 0x00954F3Cu /* "NiSpecularProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8ECC60 (initializer)
void __cdecl Fn8ECC60() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8B50));
}

// 0x8ECC70 (initializer)
void __cdecl Fn8ECC70() {
    Thiscall(0x00765410, 0x020E3CA0u, 0x00954F50u /* "NiSourceCubeMap" */, 0x020E2A10u);  // NiRTTI::NiRTTI
}

// 0x8ECC90 (initializer)
void __cdecl Fn8ECC90() {
    Thiscall(0x00765410, 0x020E3CD4u, 0x00954F60u /* "NiSortAdjustNode" */, 0x020E2548u);  // NiRTTI::NiRTTI
}

// 0x8ECCB0 (initializer)
void __cdecl Fn8ECCB0() {
    Thiscall(0x00765410, 0x020E3D08u, 0x00954F74u /* "NiSkinData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECCD0 (initializer)
void __cdecl Fn8ECCD0() {
    Thiscall(0x00765410, 0x020E3D3Cu, 0x00954F80u /* "NiScreenTexture" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECCF0 (initializer)
void __cdecl Fn8ECCF0() {
    Thiscall(0x00765410, 0x020E3D70u, 0x00954FBCu /* "NiScreenLODData" */, 0x020E4278u);  // NiRTTI::NiRTTI
}

// 0x8ECD10 (initializer)
void __cdecl Fn8ECD10() {
    Thiscall(0x00765410, 0x020E3DA4u, 0x00954FCCu /* "NiRendererSpecificProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8ECD30 (initializer)
void __cdecl Fn8ECD30() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8B80));
}

// 0x8ECD40 (initializer)
void __cdecl Fn8ECD40() {
    Thiscall(0x00765410, 0x020E3DDCu, 0x00954FE8u /* "NiRangeLODData" */, 0x020E4278u);  // NiRTTI::NiRTTI
}

// 0x8ECD60 (initializer)
void __cdecl Fn8ECD60() {
    Thiscall(0x00765410, 0x020E3E14u, 0x008FF288u /* "NiPersistentSrcTextureRendererData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECD80 (initializer)
void __cdecl Fn8ECD80() {
    Thiscall(0x00765410, 0x020E3E48u, 0x00955020u /* "NiLODNode" */, 0x020E3C00u);  // NiRTTI::NiRTTI
}

// 0x8ECDA0 (initializer)
void __cdecl Fn8ECDA0() {
    Thiscall(0x00765410, 0x020E3E7Cu, 0x0095502Cu /* "NiDitherProperty" */, 0x020E2820u);  // NiRTTI::NiRTTI
}

// 0x8ECDC0 (initializer)
void __cdecl Fn8ECDC0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8BB0));
}

// 0x8ECDD0 (initializer)
void __cdecl Fn8ECDD0() {
    Thiscall(0x00765410, 0x020E3EB4u, 0x00955054u /* "NiDefaultAVObjectPalette" */, 0x020E42ACu);  // NiRTTI::NiRTTI
}

// 0x8ECDF0 (initializer)
void __cdecl Fn8ECDF0() {
    Thiscall(0x00765410, 0x020E3EE8u, 0x0095509Cu /* "NiBSPNode" */, 0x020E2548u);  // NiRTTI::NiRTTI
}

// 0x8ECE10 (initializer)
void __cdecl Fn8ECE10() {
    Thiscall(0x00765410, 0x020E3F1Cu, 0x009550A8u /* "NiBillboardNode" */, 0x020E2548u);  // NiRTTI::NiRTTI
}

// 0x8ECE30 (initializer)
void __cdecl Fn8ECE30() {
    Thiscall(0x00765410, 0x020E3F50u, 0x00954FF8u /* "NiParticlesData" */, 0x020E30A8u);  // NiRTTI::NiRTTI
}

// 0x8ECE50 (initializer)
void __cdecl Fn8ECE50() {
    Thiscall(0x00765410, 0x020E3F84u, 0x00955008u /* "NiParticles" */, 0x020E25B4u);  // NiRTTI::NiRTTI
}

// 0x8ECE70 (initializer)
void __cdecl Fn8ECE70() {
    Thiscall(0x00765410, 0x020E3FB8u, 0x009550ECu /* "NiAlphaAccumulator" */, 0x020E42E0u);  // NiRTTI::NiRTTI
}

// 0x8ECE90 (initializer)
void __cdecl Fn8ECE90() {
    Thiscall(0x00765410, 0x020E3FECu, 0x00955100u /* "NiAdditionalGeometryData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ECEB0 (initializer)
void __cdecl Fn8ECEB0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8BF0));
}

// 0x8ECEC0 (initializer)
void __cdecl Fn8ECEC0() {
    Stdcall(Dword(0x008FC084), 0x020E40CCu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8BE0));
}

// 0x8ECEE0 (initializer)
void __cdecl Fn8ECEE0() {
    Thiscall(0x00765410, 0x020E40E4u, 0x00961E08u /* "NiFragmentShaderInstanceDescriptor" */, 0x020E3198u);  // NiRTTI::NiRTTI
}

// 0x8ECF00 (initializer)
void __cdecl Fn8ECF00() {
    Thiscall(0x00765410, 0x020E4118u, 0x00961E54u /* "NiMaterialNode" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8ECF20 (initializer)
void __cdecl Fn8ECF20() {
    Thiscall(0x00765410, 0x020E414Cu, 0x00962688u /* "NiMaterialResourceConsumerNode" */, 0x020E4118u);  // NiRTTI::NiRTTI
}

// 0x8ECF40 (initializer)
void __cdecl Fn8ECF40() {
    Thiscall(0x00765410, 0x020E4180u, 0x009626C8u /* "NiMaterialResourceProducerNode" */, 0x020E4118u);  // NiRTTI::NiRTTI
}

// 0x8ECF60 (initializer)
void __cdecl Fn8ECF60() {
    Thiscall(0x00765410, 0x020E41B4u, 0x009626E8u /* "NiMaterialFragmentNode" */, 0x020E4118u);  // NiRTTI::NiRTTI
}

// 0x8ECF80 (initializer)
void __cdecl Fn8ECF80() {
    Thiscall(0x00765410, 0x020E41F0u, 0x0096271Cu /* "NiScreenFillingRenderView" */, 0x020E3590u);  // NiRTTI::NiRTTI
}

// 0x8ECFA0 (initializer)
void __cdecl Fn8ECFA0() {
    Stdcall(Dword(0x008FC084), 0x020E4224u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8C10));
}

// 0x8ECFC0 (initializer)
void __cdecl Fn8ECFC0() {
    Thiscall(0x00765410, 0x020E4244u, 0x00962884u /* "NiVSMBlurMaterial" */, 0x020E34C0u);  // NiRTTI::NiRTTI
}

// 0x8ECFE0 (initializer)
void __cdecl Fn8ECFE0() {
    Thiscall(0x00765410, 0x020E4278u, 0x00965380u /* "NiLODData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED000 (initializer)
void __cdecl Fn8ED000() {
    Thiscall(0x00765410, 0x020E42ACu, 0x0096538Cu /* "NiAVObjectPalette" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED020 (initializer)
void __cdecl Fn8ED020() {
    Thiscall(0x00765410, 0x020E42E0u, 0x009653F8u /* "NiBackToFrontAccumulator" */, 0x020E2D50u);  // NiRTTI::NiRTTI
}

// 0x8ED0F0 (initializer)
void __cdecl Fn8ED0F0() {
    Thiscall(0x00765410, 0x020E4A54u, 0x00965A24u /* "NiGeomMorpherUpdateTask" */, 0x020E94B4u);  // NiRTTI::NiRTTI
}

// 0x8ED110 (initializer)
void __cdecl Fn8ED110() {
    Stdcall(Dword(0x008FC084), 0x020E4A88u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8C20));
}

// 0x8ED130 (initializer)
void __cdecl Fn8ED130() {
    Dword(0x020E4AD0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8C40));
}

// 0x8ED150 (initializer)
void __cdecl Fn8ED150() {
    Dword(0x020E4AD4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8C80));
}

// 0x8ED170 (initializer)
void __cdecl Fn8ED170() {
    Stdcall(Dword(0x008FC084), 0x020E4AB0u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8C30));
}

// 0x8ED190 (initializer)
void __cdecl Fn8ED190() {
    Thiscall(0x00765410, 0x020E4AD8u, 0x00965428u /* "NiVisController" */, 0x020E5618u);  // NiRTTI::NiRTTI
}

// 0x8ED1B0 (initializer)
void __cdecl Fn8ED1B0() {
    Thiscall(0x00765410, 0x020E4B0Cu, 0x00965438u /* "NiTransformInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED1D0 (initializer)
void __cdecl Fn8ED1D0() {
    Thiscall(0x00765410, 0x020E4B40u, 0x00965478u /* "NiTextureTransformController" */, 0x020E573Cu);  // NiRTTI::NiRTTI
}

// 0x8ED1F0 (initializer)
void __cdecl Fn8ED1F0() {
    Thiscall(0x00765410, 0x020E4B74u, 0x00965498u /* "NiTextKeyExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8ED210 (initializer)
void __cdecl Fn8ED210() {
    Thiscall(0x00765410, 0x020E4BA8u, 0x009654ACu /* "NiStringPalette" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED230 (initializer)
void __cdecl Fn8ED230() {
    Thiscall(0x00765410, 0x020E4BDCu, 0x009654BCu /* "NiRotData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED250 (initializer)
void __cdecl Fn8ED250() {
    Thiscall(0x00765410, 0x020E4C10u, 0x009654C8u /* "NiRollController" */, 0x020E5770u);  // NiRTTI::NiRTTI
}

// 0x8ED270 (initializer)
void __cdecl Fn8ED270() {
    Thiscall(0x00765410, 0x020E4C44u, 0x009654DCu /* "NiQuaternionInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED290 (initializer)
void __cdecl Fn8ED290() {
    Thiscall(0x00765410, 0x020E4C78u, 0x009654F8u /* "NiPosData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED2B0 (initializer)
void __cdecl Fn8ED2B0() {
    Thiscall(0x00765410, 0x020E4CACu, 0x00965504u /* "NiPoint3Interpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED2D0 (initializer)
void __cdecl Fn8ED2D0() {
    Thiscall(0x00765410, 0x020E4CE8u, 0x0096551Cu /* "NiPathInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED2F0 (initializer)
void __cdecl Fn8ED2F0() {
    Thiscall(0x00765410, 0x020E4D24u, 0x00965530u /* "NiPathController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED310 (initializer)
void __cdecl Fn8ED310() {
    Thiscall(0x00765410, 0x020E4D58u, 0x00965544u /* "NiMultiTargetTransformController" */, 0x020E564Cu);  // NiRTTI::NiRTTI
}

// 0x8ED330 (initializer)
void __cdecl Fn8ED330() {
    Thiscall(0x00765410, 0x020E4D8Cu, 0x00965568u /* "NiMorphData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED350 (initializer)
void __cdecl Fn8ED350() {
    Thiscall(0x00765410, 0x020E4DC0u, 0x00965574u /* "NiMaterialColorController" */, 0x020E57D8u);  // NiRTTI::NiRTTI
}

// 0x8ED370 (initializer)
void __cdecl Fn8ED370() {
    Thiscall(0x00765410, 0x020E4DF4u, 0x00965590u /* "NiLookAtInterpolator" */, 0x020E56F8u);  // NiRTTI::NiRTTI
}

// 0x8ED390 (initializer)
void __cdecl Fn8ED390() {
    Thiscall(0x00765410, 0x020E4E28u, 0x009655A8u /* "NiLookAtController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED3B0 (initializer)
void __cdecl Fn8ED3B0() {
    Thiscall(0x00765410, 0x020E4E5Cu, 0x009655BCu /* "NiLightDimmerController" */, 0x020E573Cu);  // NiRTTI::NiRTTI
}

// 0x8ED3D0 (initializer)
void __cdecl Fn8ED3D0() {
    Thiscall(0x00765410, 0x020E4E90u, 0x009655D4u /* "NiLightColorController" */, 0x020E57D8u);  // NiRTTI::NiRTTI
}

// 0x8ED3F0 (initializer)
void __cdecl Fn8ED3F0() {
    Thiscall(0x00765410, 0x020E4EC4u, 0x00965450u /* "NiTransformData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED410 (initializer)
void __cdecl Fn8ED410() {
    Thiscall(0x00765410, 0x020E4EF8u, 0x00965460u /* "NiTransformController" */, 0x020E5680u);  // NiRTTI::NiRTTI
}

// 0x8ED430 (initializer)
void __cdecl Fn8ED430() {
    Thiscall(0x00765410, 0x020E4F30u, 0x00965614u /* "NiGeomMorpherController" */, 0x020E564Cu);  // NiRTTI::NiRTTI
}

// 0x8ED450 (initializer)
void __cdecl Fn8ED450() {
    Thiscall(0x00765410, 0x020E4F64u, 0x0096562Cu /* "NiFloatsExtraDataPoint3Controller" */, 0x020E580Cu);  // NiRTTI::NiRTTI
}

// 0x8ED470 (initializer)
void __cdecl Fn8ED470() {
    Thiscall(0x00765410, 0x020E4F98u, 0x00965650u /* "NiFloatsExtraDataController" */, 0x020E580Cu);  // NiRTTI::NiRTTI
}

// 0x8ED490 (initializer)
void __cdecl Fn8ED490() {
    Thiscall(0x00765410, 0x020E4FCCu, 0x0096566Cu /* "NiFloatInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED4B0 (initializer)
void __cdecl Fn8ED4B0() {
    Thiscall(0x00765410, 0x020E5000u, 0x00965680u /* "NiFloatExtraDataController" */, 0x020E580Cu);  // NiRTTI::NiRTTI
}

// 0x8ED4D0 (initializer)
void __cdecl Fn8ED4D0() {
    Thiscall(0x00765410, 0x020E5034u, 0x0096569Cu /* "NiFloatData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED4F0 (initializer)
void __cdecl Fn8ED4F0() {
    Thiscall(0x00765410, 0x020E5068u, 0x009656A8u /* "NiFlipController" */, 0x020E573Cu);  // NiRTTI::NiRTTI
}

// 0x8ED510 (initializer)
void __cdecl Fn8ED510() {
    Thiscall(0x00765410, 0x020E509Cu, 0x009656BCu /* "NiControllerSequence" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED530 (initializer)
void __cdecl Fn8ED530() {
    Thiscall(0x00765410, 0x020E50D0u, 0x009656D4u /* "NiControllerManager" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED550 (initializer)
void __cdecl Fn8ED550() {
    Thiscall(0x00765410, 0x020E5104u, 0x009656E8u /* "NiColorInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED570 (initializer)
void __cdecl Fn8ED570() {
    Thiscall(0x00765410, 0x020E5138u, 0x009656FCu /* "NiColorExtraDataController" */, 0x020E580Cu);  // NiRTTI::NiRTTI
}

// 0x8ED590 (initializer)
void __cdecl Fn8ED590() {
    Thiscall(0x00765410, 0x020E516Cu, 0x00965718u /* "NiColorData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED5B0 (initializer)
void __cdecl Fn8ED5B0() {
    Thiscall(0x00765410, 0x020E51A0u, 0x00965724u /* "NiBSplineTransformInterpolator" */, 0x020E5840u);  // NiRTTI::NiRTTI
}

// 0x8ED5D0 (initializer)
void __cdecl Fn8ED5D0() {
    Thiscall(0x00765410, 0x020E51D4u, 0x00965744u /* "NiBSplinePoint3Interpolator" */, 0x020E5840u);  // NiRTTI::NiRTTI
}

// 0x8ED5F0 (initializer)
void __cdecl Fn8ED5F0() {
    Thiscall(0x00765410, 0x020E5208u, 0x00965760u /* "NiBSplineFloatInterpolator" */, 0x020E5840u);  // NiRTTI::NiRTTI
}

// 0x8ED610 (initializer)
void __cdecl Fn8ED610() {
    Thiscall(0x00765410, 0x020E523Cu, 0x0096577Cu /* "NiBSplineCompTransformInterpolator" */, 0x020E51A0u);  // NiRTTI::NiRTTI
}

// 0x8ED630 (initializer)
void __cdecl Fn8ED630() {
    Thiscall(0x00765410, 0x020E5270u, 0x009657A0u /* "NiBSplineCompPoint3Interpolator" */, 0x020E51D4u);  // NiRTTI::NiRTTI
}

// 0x8ED650 (initializer)
void __cdecl Fn8ED650() {
    Thiscall(0x00765410, 0x020E52A4u, 0x009657C0u /* "NiBSplineCompFloatInterpolator" */, 0x020E5208u);  // NiRTTI::NiRTTI
}

// 0x8ED670 (initializer)
void __cdecl Fn8ED670() {
    Thiscall(0x00765410, 0x020E52D8u, 0x009657E0u /* "NiBSplineCompColorInterpolator" */, 0x020E530Cu);  // NiRTTI::NiRTTI
}

// 0x8ED690 (initializer)
void __cdecl Fn8ED690() {
    Thiscall(0x00765410, 0x020E530Cu, 0x00965800u /* "NiBSplineColorInterpolator" */, 0x020E5840u);  // NiRTTI::NiRTTI
}

// 0x8ED6B0 (initializer)
void __cdecl Fn8ED6B0() {
    Thiscall(0x00765410, 0x020E5340u, 0x0096581Cu /* "NiBSplineData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED6D0 (initializer)
void __cdecl Fn8ED6D0() {
    Thiscall(0x00765410, 0x020E5374u, 0x0096582Cu /* "NiBSplineBasisData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED6F0 (initializer)
void __cdecl Fn8ED6F0() {
    Thiscall(0x00765410, 0x020E53A8u, 0x00965840u /* "NiBoolTimelineInterpolator" */, 0x020E53DCu);  // NiRTTI::NiRTTI
}

// 0x8ED710 (initializer)
void __cdecl Fn8ED710() {
    Thiscall(0x00765410, 0x020E53DCu, 0x0096585Cu /* "NiBoolInterpolator" */, 0x020E56B4u);  // NiRTTI::NiRTTI
}

// 0x8ED730 (initializer)
void __cdecl Fn8ED730() {
    Thiscall(0x00765410, 0x020E5410u, 0x00965870u /* "NiBoolData" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED750 (initializer)
void __cdecl Fn8ED750() {
    Thiscall(0x00765410, 0x020E5444u, 0x0096587Cu /* "NiBoneLODController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED770 (initializer)
void __cdecl Fn8ED770() {
    Thiscall(0x00765410, 0x020E5478u, 0x00965890u /* "NiBlendTransformInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED790 (initializer)
void __cdecl Fn8ED790() {
    Thiscall(0x00765410, 0x020E54ACu, 0x009658B0u /* "NiBlendQuaternionInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED7B0 (initializer)
void __cdecl Fn8ED7B0() {
    Thiscall(0x00765410, 0x020E54E0u, 0x009658D0u /* "NiBlendPoint3Interpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED7D0 (initializer)
void __cdecl Fn8ED7D0() {
    Thiscall(0x00765410, 0x020E5514u, 0x009658ECu /* "NiBlendFloatInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED7F0 (initializer)
void __cdecl Fn8ED7F0() {
    Thiscall(0x00765410, 0x020E5548u, 0x00965908u /* "NiBlendColorInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED810 (initializer)
void __cdecl Fn8ED810() {
    Thiscall(0x00765410, 0x020E557Cu, 0x00965924u /* "NiBlendBoolInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED830 (initializer)
void __cdecl Fn8ED830() {
    Thiscall(0x00765410, 0x020E55B0u, 0x0096593Cu /* "NiBlendAccumTransformInterpolator" */, 0x020E57A4u);  // NiRTTI::NiRTTI
}

// 0x8ED850 (initializer)
void __cdecl Fn8ED850() {
    Thiscall(0x00765410, 0x020E55E4u, 0x00965960u /* "NiAlphaController" */, 0x020E573Cu);  // NiRTTI::NiRTTI
}

// 0x8ED870 (initializer)
void __cdecl Fn8ED870() {
    Thiscall(0x00765410, 0x020E5618u, 0x00967F88u /* "NiBoolInterpController" */, 0x020E5680u);  // NiRTTI::NiRTTI
}

// 0x8ED890 (initializer)
void __cdecl Fn8ED890() {
    Thiscall(0x00765410, 0x020E564Cu, 0x0096805Cu /* "NiInterpController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED8B0 (initializer)
void __cdecl Fn8ED8B0() {
    Thiscall(0x00765410, 0x020E5680u, 0x00968114u /* "NiSingleInterpController" */, 0x020E564Cu);  // NiRTTI::NiRTTI
}

// 0x8ED8D0 (initializer)
void __cdecl Fn8ED8D0() {
    Thiscall(0x00765410, 0x020E56B4u, 0x009681E8u /* "NiKeyBasedInterpolator" */, 0x020E56F8u);  // NiRTTI::NiRTTI
}

// 0x8ED920 (initializer)
void __cdecl Fn8ED920() {
    Thiscall(0x00765410, 0x020E56F8u, 0x009682E4u /* "NiInterpolator" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8ED970 (initializer)
void __cdecl Fn8ED970() {
    Thiscall(0x00765410, 0x020E573Cu, 0x009683A0u /* "NiFloatInterpController" */, 0x020E5680u);  // NiRTTI::NiRTTI
}

// 0x8ED990 (initializer)
void __cdecl Fn8ED990() {
    Thiscall(0x00765410, 0x020E5770u, 0x009683B8u /* "NiFloatController" */, 0x020E32BCu);  // NiRTTI::NiRTTI
}

// 0x8ED9B0 (initializer)
void __cdecl Fn8ED9B0() {
    Thiscall(0x00765410, 0x020E57A4u, 0x00968500u /* "NiBlendInterpolator" */, 0x020E56F8u);  // NiRTTI::NiRTTI
}

// 0x8ED9D0 (initializer)
void __cdecl Fn8ED9D0() {
    Thiscall(0x00765410, 0x020E57D8u, 0x009685C0u /* "NiPoint3InterpController" */, 0x020E5680u);  // NiRTTI::NiRTTI
}

// 0x8ED9F0 (initializer)
void __cdecl Fn8ED9F0() {
    Thiscall(0x00765410, 0x020E580Cu, 0x00968694u /* "NiExtraDataController" */, 0x020E5680u);  // NiRTTI::NiRTTI
}

// 0x8EDA10 (initializer)
void __cdecl Fn8EDA10() {
    Thiscall(0x00765410, 0x020E5840u, 0x00968774u /* "NiBSplineInterpolator" */, 0x020E56F8u);  // NiRTTI::NiRTTI
}

// 0x8EDA30 (initializer)
void __cdecl Fn8EDA30() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8CC0));
}

// 0x8EDA40 (initializer)
void __cdecl Fn8EDA40() {
    Thiscall(0x00765410, 0x020E59CCu, 0x00988EA4u /* "NiVisualTrackerRenderClick" */, 0x020E2D84u);  // NiRTTI::NiRTTI
}

// 0x8EDA60 (initializer)
void __cdecl Fn8EDA60() {
    Thiscall(0x00765410, 0x020E5A04u, 0x00989264u /* "NiUILabel" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDA80 (initializer)
void __cdecl Fn8EDA80() {
    Thiscall(0x00765410, 0x020E5A40u, 0x00989414u /* "NiUIManagerRenderClick" */, 0x020E2D84u);  // NiRTTI::NiRTTI
}

// 0x8EDAA0 (initializer)
void __cdecl Fn8EDAA0() {
    Thiscall(0x00765410, 0x020E5A74u, 0x00989484u /* "NiUIBaseElement" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EDAC0 (initializer)
void __cdecl Fn8EDAC0() {
    Thiscall(0x00765410, 0x020E5AA8u, 0x00989508u /* "NiUIGroup" */, 0x020E5ADCu);  // NiRTTI::NiRTTI
}

// 0x8EDAE0 (initializer)
void __cdecl Fn8EDAE0() {
    Thiscall(0x00765410, 0x020E5ADCu, 0x0098956Cu /* "NiUIRenderGroup" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDB00 (initializer)
void __cdecl Fn8EDB00() {
    Thiscall(0x00765410, 0x020E5B10u, 0x00989810u /* "NiUIHotkeyImage" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDB20 (initializer)
void __cdecl Fn8EDB20() {
    Thiscall(0x00765410, 0x020E5B44u, 0x0098982Cu /* "NiUIButton" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDB40 (initializer)
void __cdecl Fn8EDB40() {
    Thiscall(0x00765410, 0x020E5B78u, 0x00989CC4u /* "NiUIImageButton" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDB60 (initializer)
void __cdecl Fn8EDB60() {
    Thiscall(0x00765410, 0x020E5BACu, 0x00989CD8u /* "NiUIImage" */, 0x020E5A74u);  // NiRTTI::NiRTTI
}

// 0x8EDB80 (initializer)
void __cdecl Fn8EDB80() {
    Dword(0x020E5BF4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8CF0));
}

// 0x8EDBA0 (initializer)
void __cdecl Fn8EDBA0() {
    Dword(0x020E5BF8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8D30));
}

// 0x8EDBC0 (initializer)
void __cdecl Fn8EDBC0() {
    Dword(0x020E5BFC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8D70));
}

// 0x8EDBE0 (initializer)
void __cdecl Fn8EDBE0() {
    Dword(0x020E5C00) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8DB0));
}

// 0x8EDC00 (initializer)
void __cdecl Fn8EDC00() {
    Dword(0x020E5C04) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8DF0));
}

// 0x8EDC20 (initializer)
void __cdecl Fn8EDC20() {
    Dword(0x020E5C08) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8E30));
}

// 0x8EDC40 (initializer)
void __cdecl Fn8EDC40() {
    Dword(0x020E5C0C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8E70));
}

// 0x8EDC60 (initializer)
void __cdecl Fn8EDC60() {
    Dword(0x020E5C10) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8EB0));
}

// 0x8EDC80 (initializer)
void __cdecl Fn8EDC80() {
    Dword(0x020E5C14) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8EF0));
}

// 0x8EDCA0 (initializer)
void __cdecl Fn8EDCA0() {
    Dword(0x020E5C18) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8F30));
}

// 0x8EDCC0 (initializer)
void __cdecl Fn8EDCC0() {
    Dword(0x020E5C1C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8F70));
}

// 0x8EDCE0 (initializer)
void __cdecl Fn8EDCE0() {
    Dword(0x020E5C20) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8FB0));
}

// 0x8EDD00 (initializer)
void __cdecl Fn8EDD00() {
    Dword(0x020E5C24) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F8FF0));
}

// 0x8EDD20 (initializer)
void __cdecl Fn8EDD20() {
    Dword(0x020E5C28) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9030));
}

// 0x8EDD40 (initializer)
void __cdecl Fn8EDD40() {
    Dword(0x020E5C2C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9070));
}

// 0x8EDD60 (initializer)
void __cdecl Fn8EDD60() {
    Dword(0x020E5C30) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F90B0));
}

// 0x8EDD80 (initializer)
void __cdecl Fn8EDD80() {
    Dword(0x020E5C34) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F90F0));
}

// 0x8EDDA0 (initializer)
void __cdecl Fn8EDDA0() {
    Dword(0x020E5C38) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9130));
}

// 0x8EDDC0 (initializer)
void __cdecl Fn8EDDC0() {
    Dword(0x020E5C3C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9170));
}

// 0x8EDDE0 (initializer)
void __cdecl Fn8EDDE0() {
    Dword(0x020E5C40) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F91B0));
}

// 0x8EDE00 (initializer)
void __cdecl Fn8EDE00() {
    Dword(0x020E5C44) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F91F0));
}

// 0x8EDE20 (initializer)
void __cdecl Fn8EDE20() {
    Dword(0x020E5C48) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9230));
}

// 0x8EDE40 (initializer)
void __cdecl Fn8EDE40() {
    Dword(0x020E5C4C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9270));
}

// 0x8EDE60 (initializer)
void __cdecl Fn8EDE60() {
    Dword(0x020E5C50) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F92B0));
}

// 0x8EDE80 (initializer)
void __cdecl Fn8EDE80() {
    Dword(0x020E5C54) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F92F0));
}

// 0x8EDEA0 (initializer)
void __cdecl Fn8EDEA0() {
    Dword(0x020E5C58) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9330));
}

// 0x8EDEC0 (initializer)
void __cdecl Fn8EDEC0() {
    Dword(0x020E5C5C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9370));
}

// 0x8EDEE0 (initializer)
void __cdecl Fn8EDEE0() {
    Dword(0x020E5C60) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F93B0));
}

// 0x8EDF00 (initializer)
void __cdecl Fn8EDF00() {
    Dword(0x020E5C78) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F93F0));
}

// 0x8EDF20 (initializer)
void __cdecl Fn8EDF20() {
    Dword(0x020E5C7C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9430));
}

// 0x8EDF40 (initializer)
void __cdecl Fn8EDF40() {
    Dword(0x020E5C80) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9470));
}

// 0x8EDF60 (initializer)
void __cdecl Fn8EDF60() {
    Dword(0x020E5C84) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F94B0));
}

// 0x8EDF80 (initializer)
void __cdecl Fn8EDF80() {
    Dword(0x020E5C88) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F94F0));
}

// 0x8EDFA0 (initializer)
void __cdecl Fn8EDFA0() {
    Dword(0x020E5C8C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9530));
}

// 0x8EDFC0 (initializer)
void __cdecl Fn8EDFC0() {
    Dword(0x020E5C90) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9570));
}

// 0x8EDFE0 (initializer)
void __cdecl Fn8EDFE0() {
    Dword(0x020E5C94) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F95B0));
}

// 0x8EE000 (initializer)
void __cdecl Fn8EE000() {
    Dword(0x020E5CAC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F95F0));
}

// 0x8EE020 (initializer)
void __cdecl Fn8EE020() {
    Dword(0x020E5CB0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9630));
}

// 0x8EE040 (initializer)
void __cdecl Fn8EE040() {
    Dword(0x020E5CB4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9670));
}

// 0x8EE060 (initializer)
void __cdecl Fn8EE060() {
    Dword(0x020E5CB8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F96B0));
}

// 0x8EE080 (initializer)
void __cdecl Fn8EE080() {
    Dword(0x020E5CBC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F96F0));
}

// 0x8EE0A0 (initializer)
void __cdecl Fn8EE0A0() {
    Dword(0x020E5CC0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9730));
}

// 0x8EE0C0 (initializer)
void __cdecl Fn8EE0C0() {
    Dword(0x020E5CC4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9770));
}

// 0x8EE0E0 (initializer)
void __cdecl Fn8EE0E0() {
    Dword(0x020E5CC8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F97B0));
}

// 0x8EE100 (initializer)
void __cdecl Fn8EE100() {
    Dword(0x020E5CCC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F97F0));
}

// 0x8EE120 (initializer)
void __cdecl Fn8EE120() {
    Dword(0x020E5CD0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9830));
}

// 0x8EE140 (initializer)
void __cdecl Fn8EE140() {
    Dword(0x020E5CD4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9870));
}

// 0x8EE160 (initializer)
void __cdecl Fn8EE160() {
    Dword(0x020E5CD8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F98B0));
}

// 0x8EE180 (initializer)
void __cdecl Fn8EE180() {
    Dword(0x020E5CDC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F98F0));
}

// 0x8EE1A0 (initializer)
void __cdecl Fn8EE1A0() {
    Dword(0x020E5CE0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9930));
}

// 0x8EE1C0 (initializer)
void __cdecl Fn8EE1C0() {
    Dword(0x020E5CE4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9970));
}

// 0x8EE1E0 (initializer)
void __cdecl Fn8EE1E0() {
    Dword(0x020E5CE8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F99B0));
}

// 0x8EE200 (initializer)
void __cdecl Fn8EE200() {
    Dword(0x020E5CEC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F99F0));
}

// 0x8EE220 (initializer)
void __cdecl Fn8EE220() {
    Dword(0x020E5CF0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9A30));
}

// 0x8EE240 (initializer)
void __cdecl Fn8EE240() {
    Dword(0x020E5CF4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9A70));
}

// 0x8EE260 (initializer)
void __cdecl Fn8EE260() {
    Dword(0x020E5CF8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9AB0));
}

// 0x8EE280 (initializer)
void __cdecl Fn8EE280() {
    Dword(0x020E5CFC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9AF0));
}

// 0x8EE2A0 (initializer)
void __cdecl Fn8EE2A0() {
    Dword(0x020E5D00) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9B30));
}

// 0x8EE2C0 (initializer)
void __cdecl Fn8EE2C0() {
    Dword(0x020E5D04) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9B70));
}

// 0x8EE2E0 (initializer)
void __cdecl Fn8EE2E0() {
    Dword(0x020E5D08) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9BB0));
}

// 0x8EE300 (initializer)
void __cdecl Fn8EE300() {
    Dword(0x020E5D0C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9BF0));
}

// 0x8EE320 (initializer)
void __cdecl Fn8EE320() {
    Dword(0x020E5D10) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9C30));
}

// 0x8EE340 (initializer)
void __cdecl Fn8EE340() {
    Dword(0x020E5D14) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9C70));
}

// 0x8EE360 (initializer)
void __cdecl Fn8EE360() {
    Dword(0x020E5D18) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9CB0));
}

// 0x8EE380 (initializer)
void __cdecl Fn8EE380() {
    Dword(0x020E5D1C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9CF0));
}

// 0x8EE3A0 (initializer)
void __cdecl Fn8EE3A0() {
    Dword(0x020E5D34) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9D30));
}

// 0x8EE3C0 (initializer)
void __cdecl Fn8EE3C0() {
    Dword(0x020E5D38) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9D70));
}

// 0x8EE3E0 (initializer)
void __cdecl Fn8EE3E0() {
    Dword(0x020E5D3C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9DB0));
}

// 0x8EE400 (initializer)
void __cdecl Fn8EE400() {
    Dword(0x020E5D40) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9DF0));
}

// 0x8EE420 (initializer)
void __cdecl Fn8EE420() {
    Dword(0x020E5D44) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9E30));
}

// 0x8EE440 (initializer)
void __cdecl Fn8EE440() {
    Dword(0x020E5D48) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9E70));
}

// 0x8EE460 (initializer)
void __cdecl Fn8EE460() {
    Dword(0x020E5D4C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9EB0));
}

// 0x8EE480 (initializer)
void __cdecl Fn8EE480() {
    Dword(0x020E5D50) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9EF0));
}

// 0x8EE4A0 (initializer)
void __cdecl Fn8EE4A0() {
    Dword(0x020E5D54) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9F30));
}

// 0x8EE4C0 (initializer)
void __cdecl Fn8EE4C0() {
    Dword(0x020E5D58) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9F70));
}

// 0x8EE4E0 (initializer)
void __cdecl Fn8EE4E0() {
    Dword(0x020E5D5C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9FB0));
}

// 0x8EE500 (initializer)
void __cdecl Fn8EE500() {
    Dword(0x020E5D60) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008F9FF0));
}

// 0x8EE520 (initializer)
void __cdecl Fn8EE520() {
    Dword(0x020E5D64) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA030));
}

// 0x8EE540 (initializer)
void __cdecl Fn8EE540() {
    Dword(0x020E5D7C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA070));
}

// 0x8EE560 (initializer)
void __cdecl Fn8EE560() {
    Dword(0x020E5D80) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA0B0));
}

// 0x8EE580 (initializer)
void __cdecl Fn8EE580() {
    Dword(0x020E5D84) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA0F0));
}

// 0x8EE5A0 (initializer)
void __cdecl Fn8EE5A0() {
    Dword(0x020E5D88) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA130));
}

// 0x8EE5C0 (initializer)
void __cdecl Fn8EE5C0() {
    Dword(0x020E5D8C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA170));
}

// 0x8EE5E0 (initializer)
void __cdecl Fn8EE5E0() {
    Dword(0x020E5D90) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA1B0));
}

// 0x8EE600 (initializer)
void __cdecl Fn8EE600() {
    Dword(0x020E5D94) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA1F0));
}

// 0x8EE620 (initializer)
void __cdecl Fn8EE620() {
    Dword(0x020E5D98) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA230));
}

// 0x8EE640 (initializer)
void __cdecl Fn8EE640() {
    Dword(0x020E5D9C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA270));
}

// 0x8EE660 (initializer)
void __cdecl Fn8EE660() {
    Dword(0x020E5DA0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA2B0));
}

// 0x8EE680 (initializer)
void __cdecl Fn8EE680() {
    Dword(0x020E5DA4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA2F0));
}

// 0x8EE6A0 (initializer)
void __cdecl Fn8EE6A0() {
    Dword(0x020E5DA8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA330));
}

// 0x8EE6C0 (initializer)
void __cdecl Fn8EE6C0() {
    Dword(0x020E5DAC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA370));
}

// 0x8EE6E0 (initializer)
void __cdecl Fn8EE6E0() {
    Dword(0x020E5DB0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA3B0));
}

// 0x8EE700 (initializer)
void __cdecl Fn8EE700() {
    Dword(0x020E5DB4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA3F0));
}

// 0x8EE720 (initializer)
void __cdecl Fn8EE720() {
    Dword(0x020E5DB8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA430));
}

// 0x8EE740 (initializer)
void __cdecl Fn8EE740() {
    Dword(0x020E5DBC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA470));
}

// 0x8EE760 (initializer)
void __cdecl Fn8EE760() {
    Dword(0x020E5DC0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA4B0));
}

// 0x8EE780 (initializer)
void __cdecl Fn8EE780() {
    Dword(0x020E5DC4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA4F0));
}

// 0x8EE7A0 (initializer)
void __cdecl Fn8EE7A0() {
    Dword(0x020E5DC8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA530));
}

// 0x8EE7C0 (initializer)
void __cdecl Fn8EE7C0() {
    Dword(0x020E5DCC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA570));
}

// 0x8EE7E0 (initializer)
void __cdecl Fn8EE7E0() {
    Dword(0x020E5DD0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA5B0));
}

// 0x8EE800 (initializer)
void __cdecl Fn8EE800() {
    Dword(0x020E5DD4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA5F0));
}

// 0x8EE820 (initializer)
void __cdecl Fn8EE820() {
    Dword(0x020E5DD8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA630));
}

// 0x8EE840 (initializer)
void __cdecl Fn8EE840() {
    Dword(0x020E5DDC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA670));
}

// 0x8EE860 (initializer)
void __cdecl Fn8EE860() {
    Dword(0x020E5DE0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA6B0));
}

// 0x8EE880 (initializer)
void __cdecl Fn8EE880() {
    Dword(0x020E5DE4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA6F0));
}

// 0x8EE8A0 (initializer)
void __cdecl Fn8EE8A0() {
    Dword(0x020E5DE8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA730));
}

// 0x8EE8C0 (initializer)
void __cdecl Fn8EE8C0() {
    Dword(0x020E5DEC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA770));
}

// 0x8EE8E0 (initializer)
void __cdecl Fn8EE8E0() {
    Dword(0x020E5DF0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA7B0));
}

// 0x8EE900 (initializer)
void __cdecl Fn8EE900() {
    Dword(0x020E5DF4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA7F0));
}

// 0x8EE920 (initializer)
void __cdecl Fn8EE920() {
    Dword(0x020E5DF8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA830));
}

// 0x8EE940 (initializer)
void __cdecl Fn8EE940() {
    Dword(0x020E5DFC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA870));
}

// 0x8EE960 (initializer)
void __cdecl Fn8EE960() {
    Dword(0x020E5E00) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA8B0));
}

// 0x8EE980 (initializer)
void __cdecl Fn8EE980() {
    Dword(0x020E5E04) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA8F0));
}

// 0x8EE9A0 (initializer)
void __cdecl Fn8EE9A0() {
    Dword(0x020E5E08) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA930));
}

// 0x8EE9C0 (initializer)
void __cdecl Fn8EE9C0() {
    Dword(0x020E5E0C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA970));
}

// 0x8EE9E0 (initializer)
void __cdecl Fn8EE9E0() {
    Dword(0x020E5E10) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA9B0));
}

// 0x8EEA00 (initializer)
void __cdecl Fn8EEA00() {
    Dword(0x020E5E14) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FA9F0));
}

// 0x8EEA20 (initializer)
void __cdecl Fn8EEA20() {
    Dword(0x020E5E18) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAA30));
}

// 0x8EEA40 (initializer)
void __cdecl Fn8EEA40() {
    Dword(0x020E5E1C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAA70));
}

// 0x8EEA60 (initializer)
void __cdecl Fn8EEA60() {
    Dword(0x020E5E34) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAAB0));
}

// 0x8EEA80 (initializer)
void __cdecl Fn8EEA80() {
    Dword(0x020E5E38) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAAF0));
}

// 0x8EEAA0 (initializer)
void __cdecl Fn8EEAA0() {
    Dword(0x020E5E3C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAB30));
}

// 0x8EEAC0 (initializer)
void __cdecl Fn8EEAC0() {
    Dword(0x020E5E40) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAB70));
}

// 0x8EEAE0 (initializer)
void __cdecl Fn8EEAE0() {
    Dword(0x020E5E44) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FABB0));
}

// 0x8EEB00 (initializer)
void __cdecl Fn8EEB00() {
    Dword(0x020E5E48) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FABF0));
}

// 0x8EEB20 (initializer)
void __cdecl Fn8EEB20() {
    Dword(0x020E5E4C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAC30));
}

// 0x8EEB40 (initializer)
void __cdecl Fn8EEB40() {
    Dword(0x020E5E50) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAC70));
}

// 0x8EEB60 (initializer)
void __cdecl Fn8EEB60() {
    Dword(0x020E5E54) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FACB0));
}

// 0x8EEB80 (initializer)
void __cdecl Fn8EEB80() {
    Dword(0x020E5E58) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FACF0));
}

// 0x8EEBA0 (initializer)
void __cdecl Fn8EEBA0() {
    Dword(0x020E5E5C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAD30));
}

// 0x8EEBC0 (initializer)
void __cdecl Fn8EEBC0() {
    Dword(0x020E5E60) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAD70));
}

// 0x8EEBE0 (initializer)
void __cdecl Fn8EEBE0() {
    Dword(0x020E5E64) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FADB0));
}

// 0x8EEC00 (initializer)
void __cdecl Fn8EEC00() {
    Dword(0x020E5E68) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FADF0));
}

// 0x8EEC20 (initializer)
void __cdecl Fn8EEC20() {
    Dword(0x020E5E6C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAE30));
}

// 0x8EEC40 (initializer)
void __cdecl Fn8EEC40() {
    Dword(0x020E5E70) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAE70));
}

// 0x8EEC60 (initializer)
void __cdecl Fn8EEC60() {
    Dword(0x020E5E74) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAEB0));
}

// 0x8EEC80 (initializer)
void __cdecl Fn8EEC80() {
    Dword(0x020E5E78) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAEF0));
}

// 0x8EECA0 (initializer)
void __cdecl Fn8EECA0() {
    Dword(0x020E5E7C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAF30));
}

// 0x8EECC0 (initializer)
void __cdecl Fn8EECC0() {
    Dword(0x020E5E80) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAF70));
}

// 0x8EECE0 (initializer)
void __cdecl Fn8EECE0() {
    Dword(0x020E5E98) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAFB0));
}

// 0x8EED00 (initializer)
void __cdecl Fn8EED00() {
    Dword(0x020E5E9C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FAFF0));
}

// 0x8EED20 (initializer)
void __cdecl Fn8EED20() {
    Dword(0x020E5EA0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB030));
}

// 0x8EED40 (initializer)
void __cdecl Fn8EED40() {
    Dword(0x020E5EA4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB070));
}

// 0x8EED60 (initializer)
void __cdecl Fn8EED60() {
    Dword(0x020E5EA8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB0B0));
}

// 0x8EED80 (initializer)
void __cdecl Fn8EED80() {
    Dword(0x020E5EAC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB0F0));
}

// 0x8EEDA0 (initializer)
void __cdecl Fn8EEDA0() {
    Dword(0x020E5EB0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB130));
}

// 0x8EEDC0 (initializer)
void __cdecl Fn8EEDC0() {
    Dword(0x020E5EB4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB170));
}

// 0x8EEDE0 (initializer)
void __cdecl Fn8EEDE0() {
    Dword(0x020E5EB8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB1B0));
}

// 0x8EEE00 (initializer)
void __cdecl Fn8EEE00() {
    Dword(0x020E5EBC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB1F0));
}

// 0x8EEE20 (initializer)
void __cdecl Fn8EEE20() {
    Dword(0x020E5EC0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB230));
}

// 0x8EEE40 (initializer)
void __cdecl Fn8EEE40() {
    Dword(0x020E5EC4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB270));
}

// 0x8EEE60 (initializer)
void __cdecl Fn8EEE60() {
    Dword(0x020E5EDC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB2B0));
}

// 0x8EEE80 (initializer)
void __cdecl Fn8EEE80() {
    Dword(0x020E5EE0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB2F0));
}

// 0x8EEEA0 (initializer)
void __cdecl Fn8EEEA0() {
    Dword(0x020E5EE4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB330));
}

// 0x8EEEC0 (initializer)
void __cdecl Fn8EEEC0() {
    Dword(0x020E5EE8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB370));
}

// 0x8EEEE0 (initializer)
void __cdecl Fn8EEEE0() {
    Dword(0x020E5EEC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB3B0));
}

// 0x8EEF00 (initializer)
void __cdecl Fn8EEF00() {
    Dword(0x020E5EF0) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB3F0));
}

// 0x8EEF20 (initializer)
void __cdecl Fn8EEF20() {
    Dword(0x020E5EF4) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB430));
}

// 0x8EEF40 (initializer)
void __cdecl Fn8EEF40() {
    Dword(0x020E5EF8) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB470));
}

// 0x8EEF60 (initializer)
void __cdecl Fn8EEF60() {
    Dword(0x020E5EFC) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB4B0));
}

// 0x8EEF80 (initializer)
void __cdecl Fn8EEF80() {
    Dword(0x020E5F00) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB4F0));
}

// 0x8EEFA0 (initializer)
void __cdecl Fn8EEFA0() {
    Dword(0x020E5F04) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB530));
}

// 0x8EEFC0 (initializer)
void __cdecl Fn8EEFC0() {
    Dword(0x020E5F08) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB570));
}

// 0x8EEFE0 (initializer)
void __cdecl Fn8EEFE0() {
    Dword(0x020E5F0C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB5B0));
}

// 0x8EF000 (initializer)
void __cdecl Fn8EF000() {
    Dword(0x020E5F10) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB5F0));
}

// 0x8EF020 (initializer)
void __cdecl Fn8EF020() {
    Dword(0x020E5F14) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB630));
}

// 0x8EF040 (initializer)
void __cdecl Fn8EF040() {
    Dword(0x020E5F18) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB670));
}

// 0x8EF060 (initializer)
void __cdecl Fn8EF060() {
    Dword(0x020E5F1C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB6B0));
}

// 0x8EF080 (initializer)
void __cdecl Fn8EF080() {
    Dword(0x020E5F20) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB6F0));
}

// 0x8EF0A0 (initializer)
void __cdecl Fn8EF0A0() {
    Dword(0x020E5F24) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB730));
}

// 0x8EF0C0 (initializer)
void __cdecl Fn8EF0C0() {
    Dword(0x020E5F28) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB770));
}

// 0x8EF0E0 (initializer)
void __cdecl Fn8EF0E0() {
    Dword(0x020E5F2C) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB7B0));
}

// 0x8EF100 (initializer)
void __cdecl Fn8EF100() {
    Dword(0x020E5F30) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB7F0));
}

// 0x8EF120 (initializer)
void __cdecl Fn8EF120() {
    Thiscall(0x00765410, 0x020E5F34u, 0x0098C2D4u /* "NiParamsKF" */, 0x020E5F68u);  // NiRTTI::NiRTTI
}

// 0x8EF140 (initializer)
void __cdecl Fn8EF140() {
    Thiscall(0x00765410, 0x020E5F68u, 0x0098C2E8u /* "NiExternalAssetParams" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EF160 (initializer)
void __cdecl Fn8EF160() {
    Thiscall(0x00765410, 0x020E5F9Cu, 0x0098C308u /* "NiParamsKFM" */, 0x020E5F68u);  // NiRTTI::NiRTTI
}

// 0x8EF180 (initializer)
void __cdecl Fn8EF180() {
    Thiscall(0x00765410, 0x020E5FD0u, 0x0098C31Cu /* "NiParamsNIF" */, 0x020E5F68u);  // NiRTTI::NiRTTI
}

// 0x8EF1A0 (initializer)
void __cdecl Fn8EF1A0() {
    Thiscall(0x00765410, 0x020E6D18u, 0x0099BDD4u /* "NiDX9Renderer" */, 0x020E2384u);  // NiRTTI::NiRTTI
}

// 0x8EF1C0 (initializer)
void __cdecl Fn8EF1C0() {
    Stdcall(Dword(0x008FC084), 0x020E6D4Cu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB830));
}

// 0x8EF1E0 (initializer)
void __cdecl Fn8EF1E0() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB840));
}

// 0x8EF1F0 (initializer)
uint32_t __cdecl Fn8EF1F0() {
    return Thiscall(0x0089D9F0, 0x020E6D6Cu);
}

// 0x8EF200 (initializer)
void __cdecl Fn8EF200() {
    Stdcall(Dword(0x008FC084), 0x020E6D84u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB860));
}

// 0x8EF220 (initializer)
void __cdecl Fn8EF220() {
    Thiscall(0x00765410, 0x020E6DA4u, 0x0099BEA0u /* "NiDX9ShadowWriteShader" */, 0x020E7054u);  // NiRTTI::NiRTTI
}

// 0x8EF240 (initializer)
void __cdecl Fn8EF240() {
    Thiscall(0x00765410, 0x020E6DD8u, 0x0099BF80u /* "NiDX9PersistentSrcTextureRendererData" */, 0x020E3E14u);  // NiRTTI::NiRTTI
}

// 0x8EF260 (initializer)
void __cdecl Fn8EF260() {
    Thiscall(0x00765410, 0x020E6E0Cu, 0x0099C06Cu /* "NiD3DShaderInterface" */, 0x020E3164u);  // NiRTTI::NiRTTI
}

// 0x8EF280 (initializer)
void __cdecl Fn8EF280() {
    Dword(0x020E6E54) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB870));
}

// 0x8EF2A0 (initializer)
void __cdecl Fn8EF2A0() {
    Thiscall(0x00765410, 0x020E6E58u, 0x0099C0D8u /* "NiD3DShader" */, 0x020E6E0Cu);  // NiRTTI::NiRTTI
}

// 0x8EF2E0 (initializer)
void __cdecl Fn8EF2E0() {
    Thiscall(0x00765410, 0x020E7054u, 0x0099C858u /* "NiDX9FragmentShader" */, 0x020E6E58u);  // NiRTTI::NiRTTI
}

// 0x8EF300 (initializer)
void __cdecl Fn8EF300() {
    Thiscall(0x00765410, 0x020E70A4u, 0x0099CA80u /* "NiDX92DBufferData" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8EF320 (initializer)
void __cdecl Fn8EF320() {
    Thiscall(0x00765410, 0x020E70D8u, 0x0099CA94u /* "NiDX9TextureBufferData" */, 0x020E70A4u);  // NiRTTI::NiRTTI
}

// 0x8EF340 (initializer)
void __cdecl Fn8EF340() {
    Thiscall(0x00765410, 0x020E710Cu, 0x0099CAACu /* "NiDX9Direct3DBufferData" */, 0x020E70A4u);  // NiRTTI::NiRTTI
}

// 0x8EF360 (initializer)
void __cdecl Fn8EF360() {
    Thiscall(0x00765410, 0x020E7140u, 0x0099CAC4u /* "NiDX9DepthStencilBufferData" */, 0x020E70A4u);  // NiRTTI::NiRTTI
}

// 0x8EF380 (initializer)
void __cdecl Fn8EF380() {
    Thiscall(0x00765410, 0x020E7174u, 0x0099CAE0u /* "NiDX9ImplicitDepthStencilBufferData" */, 0x020E7140u);  // NiRTTI::NiRTTI
}

// 0x8EF3A0 (initializer)
void __cdecl Fn8EF3A0() {
    Thiscall(0x00765410, 0x020E71A8u, 0x0099CB04u /* "NiDX9Direct3DDepthStencilBufferData" */, 0x020E7140u);  // NiRTTI::NiRTTI
}

// 0x8EF3C0 (initializer)
void __cdecl Fn8EF3C0() {
    Thiscall(0x00765410, 0x020E71DCu, 0x0099CB28u /* "NiDX9AdditionalDepthStencilBufferData" */, 0x020E7140u);  // NiRTTI::NiRTTI
}

// 0x8EF3E0 (initializer)
void __cdecl Fn8EF3E0() {
    Stdcall(Dword(0x008FC084), 0x020E7210u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB8B0));
}

// 0x8EF400 (initializer)
void __cdecl Fn8EF400() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB8C0));
}

// 0x8EF410 (initializer)
void __cdecl Fn8EF410() {
    Thiscall(0x00765410, 0x020E7230u, 0x0099CB50u /* "NiDX9OnscreenBufferData" */, 0x020E70A4u);  // NiRTTI::NiRTTI
}

// 0x8EF430 (initializer)
void __cdecl Fn8EF430() {
    Thiscall(0x00765410, 0x020E7264u, 0x0099CB68u /* "NiDX9ImplicitBufferData" */, 0x020E7230u);  // NiRTTI::NiRTTI
}

// 0x8EF450 (initializer)
void __cdecl Fn8EF450() {
    Thiscall(0x00765410, 0x020E7298u, 0x0099CB80u /* "NiDX9SwapChainBufferData" */, 0x020E7230u);  // NiRTTI::NiRTTI
}

// 0x8EF470 (initializer)
void __cdecl Fn8EF470() {
    Thiscall(0x00765410, 0x020E72CCu, 0x0099CB9Cu /* "NiDX9SwapChainDepthStencilBufferData" */, 0x020E7140u);  // NiRTTI::NiRTTI
}

// 0x8EF490 (initializer)
void __cdecl Fn8EF490() {
    Thiscall(0x00765410, 0x020E7304u, 0x0099D1B0u /* "NiDX9Direct3DTexture" */, 0x020E2184u);  // NiRTTI::NiRTTI
}

// 0x8EF4B0 (initializer)
void __cdecl Fn8EF4B0() {
    Thiscall(0x00765410, 0x020E7338u, 0x0099D8E4u /* "NiDX9ErrorShader" */, 0x020E6E58u);  // NiRTTI::NiRTTI
}

// 0x8EF4D0 (initializer)
void __cdecl Fn8EF4D0() {
    Thiscall(0x00765410, 0x020E736Cu, 0x0099D9D0u /* "NiD3DDefaultShader" */, 0x020E6E58u);  // NiRTTI::NiRTTI
}

// 0x8EF4F0 (initializer)
void __cdecl Fn8EF4F0() {
    Stdcall(Dword(0x008FC084), 0x020E7410u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB8E0));
}

// 0x8EF510 (initializer)
void __cdecl Fn8EF510() {
    Thiscall(0x00765410, 0x020E749Cu, 0x0099E0CCu /* "NiD3DHLSLPixelShader" */, 0x020E754Cu);  // NiRTTI::NiRTTI
}

// 0x8EF530 (initializer)
void __cdecl Fn8EF530() {
    Thiscall(0x00765410, 0x020E74D0u, 0x0099E17Cu /* "NiD3DHLSLVertexShader" */, 0x020E7580u);  // NiRTTI::NiRTTI
}

// 0x8EF550 (initializer)
void __cdecl Fn8EF550() {
    Thiscall(0x00765410, 0x020E7518u, 0x0099E390u /* "NiD3DShaderProgram" */, 0x020E965Cu);  // NiRTTI::NiRTTI
}

// 0x8EF570 (initializer)
void __cdecl Fn8EF570() {
    Thiscall(0x00765410, 0x020E754Cu, 0x0099E3A4u /* "NiD3DPixelShader" */, 0x020E7518u);  // NiRTTI::NiRTTI
}

// 0x8EF590 (initializer)
void __cdecl Fn8EF590() {
    Thiscall(0x00765410, 0x020E7580u, 0x0099E3B8u /* "NiD3DVertexShader" */, 0x020E7518u);  // NiRTTI::NiRTTI
}

// 0x8EF5B0 (initializer)
void __cdecl Fn8EF5B0() {
    Stdcall(Dword(0x008FC084), 0x020E75C0u);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB8F0));
}

// 0x8EF5D0 (initializer)
void __cdecl Fn8EF5D0() {
    Thiscall(0x0076B2F0, 0x020E75E0u, 0x00000001u, 0x00000080u, 0x00000000u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000002u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000003u, 0x00000005u, 0x00000020u, 0x00000000u);
}

// 0x8EF610 (initializer)
void __cdecl Fn8EF610() {
    Thiscall(0x0076B2F0, 0x020E7628u, 0x00000001u, 0x00000080u, 0x00000000u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000002u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000003u, 0x00000002u, 0x00000020u, 0x00000001u);
}

// 0x8EF650 (initializer)
void __cdecl Fn8EF650() {
    Thiscall(0x0076B2F0, 0x020E7670u, 0x00000001u, 0x00000080u, 0x00000000u, 0x00000001u, 0x00000003u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000002u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000003u, 0x00000006u, 0x00000020u, 0x00000000u);
}

// 0x8EF690 (initializer)
void __cdecl Fn8EF690() {
    Thiscall(0x0076B2F0, 0x020E76B8u, 0x00000001u, 0x00000080u, 0x00000000u, 0x00000001u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000003u, 0x00000006u, 0x00000020u, 0x00000001u);
}

// 0x8EF6D0 (initializer)
void __cdecl Fn8EF6D0() {
    Thiscall(0x0076B2F0, 0x020E7700u, 0x00000000u, 0x00000060u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000002u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF710 (initializer)
void __cdecl Fn8EF710() {
    Thiscall(0x0076B2F0, 0x020E7748u, 0x00000000u, 0x00000060u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000002u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF750 (initializer)
void __cdecl Fn8EF750() {
    Thiscall(0x0076B2F0, 0x020E7790u, 0x00000000u, 0x00000060u, 0x00000000u, 0x00000001u, 0x00000007u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000002u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF790 (initializer)
void __cdecl Fn8EF790() {
    Thiscall(0x0076B2F0, 0x020E77D8u, 0x00000000u, 0x00000060u, 0x00000000u, 0x00000001u, 0x00000008u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF7D0 (initializer)
void __cdecl Fn8EF7D0() {
    Thiscall(0x0076B2F0, 0x020E7820u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000009u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000002u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000003u, 0x00000005u, 0x00000010u, 0x00000000u);
}

// 0x8EF810 (initializer)
void __cdecl Fn8EF810() {
    Thiscall(0x0076B2F0, 0x020E7868u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000002u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000003u, 0x00000001u, 0x00000010u, 0x00000001u);
}

// 0x8EF850 (initializer)
void __cdecl Fn8EF850() {
    Thiscall(0x0076B2F0, 0x020E78B0u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000010u, 0x00000000u);
}

// 0x8EF890 (initializer)
void __cdecl Fn8EF890() {
    Thiscall(0x0076B2F0, 0x020E78F8u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Cu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000002u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000003u, 0x00000006u, 0x00000010u, 0x00000000u);
}

// 0x8EF8D0 (initializer)
void __cdecl Fn8EF8D0() {
    Thiscall(0x0076B2F0, 0x020E7940u, 0x00000008u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000003u, 0x00000000u, 0x00000010u, 0x00000001u);
}

// 0x8EF910 (initializer)
void __cdecl Fn8EF910() {
    Thiscall(0x0076B2F0, 0x020E7988u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Eu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000003u, 0x00000006u, 0x00000010u, 0x00000001u);
}

// 0x8EF950 (initializer)
void __cdecl Fn8EF950() {
    Thiscall(0x0076B2F0, 0x020E79D0u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x0000000Fu, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF990 (initializer)
void __cdecl Fn8EF990() {
    Thiscall(0x0076B2F0, 0x020E7A18u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000010u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EF9D0 (initializer)
void __cdecl Fn8EF9D0() {
    Thiscall(0x0076B2F0, 0x020E7A60u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFA10 (initializer)
void __cdecl Fn8EFA10() {
    Thiscall(0x0076B2F0, 0x020E7AA8u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000012u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFA50 (initializer)
void __cdecl Fn8EFA50() {
    Thiscall(0x0076B2F0, 0x020E7AF0u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000013u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000008u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000018u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFA90 (initializer)
void __cdecl Fn8EFA90() {
    Thiscall(0x0076B2F0, 0x020E7B38u, 0x0000000Fu, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000014u, 0x00000000u, 0x00000011u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000012u, 0x00000000u, 0x00000008u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000018u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFAD0 (initializer)
void __cdecl Fn8EFAD0() {
    Thiscall(0x0076B2F0, 0x020E7B80u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000015u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000020u, 0x00000001u, 0x0000000Eu, 0x00000005u, 0x00000008u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000018u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFB10 (initializer)
void __cdecl Fn8EFB10() {
    Thiscall(0x0076B2F0, 0x020E7BC8u, 0x00000000u, 0x00000040u, 0x00000000u, 0x00000001u, 0x00000016u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000018u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFB50 (initializer)
void __cdecl Fn8EFB50() {
    Thiscall(0x0076B2F0, 0x020E7C10u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000005u, 0x0000000Au, 0x00000000u, 0x00000001u, 0x00000005u, 0x0000000Au, 0x00000000u, 0x00000002u, 0x00000005u, 0x0000000Au, 0x00000000u, 0x00000003u, 0x00000005u, 0x00000002u, 0x00000000u);
}

// 0x8EFB90 (initializer)
void __cdecl Fn8EFB90() {
    Thiscall(0x0076B2F0, 0x020E7C58u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000018u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0000000Au, 0x00000000u, 0x00000001u, 0x00000000u, 0x0000000Au, 0x00000000u, 0x00000002u, 0x00000000u, 0x0000000Au, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000002u, 0x00000000u);
}

// 0x8EFBD0 (initializer)
void __cdecl Fn8EFBD0() {
    Thiscall(0x0076B2F0, 0x020E7CA0u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000019u, 0x00000000u, 0x00000000u, 0x00000006u, 0x0000000Au, 0x00000000u, 0x00000001u, 0x00000006u, 0x0000000Au, 0x00000000u, 0x00000002u, 0x00000006u, 0x0000000Au, 0x00000000u, 0x00000003u, 0x00000006u, 0x00000002u, 0x00000000u);
}

// 0x8EFC10 (initializer)
void __cdecl Fn8EFC10() {
    Thiscall(0x0076B2F0, 0x020E7CE8u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Au, 0x00000000u, 0x00000000u, 0x00000002u, 0x0000000Bu, 0x00000001u, 0x00000001u, 0x00000002u, 0x0000000Bu, 0x00000001u, 0x00000002u, 0x00000002u, 0x0000000Au, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFC50 (initializer)
void __cdecl Fn8EFC50() {
    Thiscall(0x0076B2F0, 0x020E7D30u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Bu, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000002u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000003u, 0x00000005u, 0x00000008u, 0x00000000u);
}

// 0x8EFC90 (initializer)
void __cdecl Fn8EFC90() {
    Thiscall(0x0076B2F0, 0x020E7D78u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Cu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000008u, 0x00000000u);
}

// 0x8EFCD0 (initializer)
void __cdecl Fn8EFCD0() {
    Thiscall(0x0076B2F0, 0x020E7DC0u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Du, 0x00000001u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000008u, 0x00000000u);
}

// 0x8EFD10 (initializer)
void __cdecl Fn8EFD10() {
    Thiscall(0x0076B2F0, 0x020E7E08u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Eu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000002u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000003u, 0x00000006u, 0x00000008u, 0x00000000u);
}

// 0x8EFD50 (initializer)
void __cdecl Fn8EFD50() {
    Thiscall(0x0076B2F0, 0x020E7E50u, 0x00000008u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000001Fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000002u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000003u, 0x00000000u, 0x00000008u, 0x00000001u);
}

// 0x8EFD90 (initializer)
void __cdecl Fn8EFD90() {
    Thiscall(0x0076B2F0, 0x020E7E98u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000002u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000003u, 0x00000006u, 0x00000008u, 0x00000001u);
}

// 0x8EFDD0 (initializer)
void __cdecl Fn8EFDD0() {
    Thiscall(0x0076B2F0, 0x020E7EE0u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000021u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFE10 (initializer)
void __cdecl Fn8EFE10() {
    Thiscall(0x0076B2F0, 0x020E7F28u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000022u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFE50 (initializer)
void __cdecl Fn8EFE50() {
    Thiscall(0x0076B2F0, 0x020E7F70u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000023u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFE90 (initializer)
void __cdecl Fn8EFE90() {
    Thiscall(0x0076B2F0, 0x020E7FB8u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFED0 (initializer)
void __cdecl Fn8EFED0() {
    Thiscall(0x0076B2F0, 0x020E8000u, 0x00000008u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000025u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFF10 (initializer)
void __cdecl Fn8EFF10() {
    Thiscall(0x0076B2F0, 0x020E8048u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000026u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFF50 (initializer)
void __cdecl Fn8EFF50() {
    Thiscall(0x0076B2F0, 0x020E8090u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000027u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFF90 (initializer)
void __cdecl Fn8EFF90() {
    Thiscall(0x0076B2F0, 0x020E80D8u, 0x0000000Fu, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000028u, 0x00000000u, 0x00000011u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8EFFD0 (initializer)
void __cdecl Fn8EFFD0() {
    Thiscall(0x0076B2F0, 0x020E8120u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000029u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0010 (initializer)
void __cdecl Fn8F0010() {
    Thiscall(0x0076B2F0, 0x020E8168u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Au, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0050 (initializer)
void __cdecl Fn8F0050() {
    Thiscall(0x0076B2F0, 0x020E81B0u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Bu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000020u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0090 (initializer)
void __cdecl Fn8F0090() {
    Thiscall(0x0076B2F0, 0x020E81F8u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Cu, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000018u, 0x00000001u, 0x00000001u, 0x00000005u, 0x00000008u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F00D0 (initializer)
void __cdecl Fn8F00D0() {
    Thiscall(0x0076B2F0, 0x020E8240u, 0x0000000Fu, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Du, 0x00000000u, 0x00000011u, 0x00000000u, 0x00000018u, 0x00000000u, 0x00000012u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0110 (initializer)
void __cdecl Fn8F0110() {
    Thiscall(0x0076B2F0, 0x020E8288u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000018u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0150 (initializer)
void __cdecl Fn8F0150() {
    Thiscall(0x0076B2F0, 0x020E82D0u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x0000002Fu, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000018u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0190 (initializer)
void __cdecl Fn8F0190() {
    Thiscall(0x0076B2F0, 0x020E8318u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000030u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F01D0 (initializer)
void __cdecl Fn8F01D0() {
    Thiscall(0x0076B2F0, 0x020E8360u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000031u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0210 (initializer)
void __cdecl Fn8F0210() {
    Thiscall(0x0076B2F0, 0x020E83A8u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000032u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0250 (initializer)
void __cdecl Fn8F0250() {
    Thiscall(0x0076B2F0, 0x020E83F0u, 0x00000008u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000033u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0290 (initializer)
void __cdecl Fn8F0290() {
    Thiscall(0x0076B2F0, 0x020E8438u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000034u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000001u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F02D0 (initializer)
void __cdecl Fn8F02D0() {
    Thiscall(0x0076B2F0, 0x020E8480u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000035u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0310 (initializer)
void __cdecl Fn8F0310() {
    Thiscall(0x0076B2F0, 0x020E84C8u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000036u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0350 (initializer)
void __cdecl Fn8F0350() {
    Thiscall(0x0076B2F0, 0x020E8510u, 0x0000000Fu, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000037u, 0x00000000u, 0x00000011u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0390 (initializer)
void __cdecl Fn8F0390() {
    Thiscall(0x0076B2F0, 0x020E8558u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000038u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F03D0 (initializer)
void __cdecl Fn8F03D0() {
    Thiscall(0x0076B2F0, 0x020E85A0u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000039u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0410 (initializer)
void __cdecl Fn8F0410() {
    Thiscall(0x0076B2F0, 0x020E85E8u, 0x00000008u, 0x00000010u, 0x00000000u, 0x00000001u, 0x0000003Au, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0450 (initializer)
void __cdecl Fn8F0450() {
    Thiscall(0x0076B2F0, 0x020E8630u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x0000003Bu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000010u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0490 (initializer)
void __cdecl Fn8F0490() {
    Thiscall(0x0076B2F0, 0x020E8678u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x0000003Cu, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F04D0 (initializer)
void __cdecl Fn8F04D0() {
    Thiscall(0x0076B2F0, 0x020E86C0u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x0000003Du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0510 (initializer)
void __cdecl Fn8F0510() {
    Thiscall(0x0076B2F0, 0x020E8708u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x0000003Eu, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0550 (initializer)
void __cdecl Fn8F0550() {
    Thiscall(0x0076B2F0, 0x020E8750u, 0x00000008u, 0x00000008u, 0x00000000u, 0x00000001u, 0x0000003Fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0590 (initializer)
void __cdecl Fn8F0590() {
    Thiscall(0x0076B2F0, 0x020E8798u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000040u, 0x00000000u, 0x00000000u, 0x00000006u, 0x00000008u, 0x00000001u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F05D0 (initializer)
void __cdecl Fn8F05D0() {
    Thiscall(0x0076B2F0, 0x020E87E0u, 0x00000001u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000041u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0610 (initializer)
void __cdecl Fn8F0610() {
    Thiscall(0x0076B2F0, 0x020E8828u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000001u, 0x00000042u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0650 (initializer)
void __cdecl Fn8F0650() {
    Thiscall(0x0076B2F0, 0x020E8870u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000043u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0690 (initializer)
void __cdecl Fn8F0690() {
    Thiscall(0x0076B2F0, 0x020E88B8u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000044u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F06D0 (initializer)
void __cdecl Fn8F06D0() {
    Thiscall(0x0076B2F0, 0x020E8900u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000045u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0710 (initializer)
void __cdecl Fn8F0710() {
    Thiscall(0x0076B2F0, 0x020E8948u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000046u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0750 (initializer)
void __cdecl Fn8F0750() {
    Thiscall(0x0076B2F0, 0x020E8990u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000047u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0790 (initializer)
void __cdecl Fn8F0790() {
    Thiscall(0x0076B2F0, 0x020E89D8u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000048u, 0x00000001u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F07D0 (initializer)
void __cdecl Fn8F07D0() {
    Thiscall(0x0076B2F0, 0x020E8A20u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000049u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0810 (initializer)
void __cdecl Fn8F0810() {
    Thiscall(0x0076B2F0, 0x020E8A68u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Au, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0850 (initializer)
void __cdecl Fn8F0850() {
    Thiscall(0x0076B2F0, 0x020E8AB0u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Bu, 0x00000001u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0890 (initializer)
void __cdecl Fn8F0890() {
    Thiscall(0x0076B2F0, 0x020E8AF8u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Cu, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F08D0 (initializer)
void __cdecl Fn8F08D0() {
    Thiscall(0x0076B2F0, 0x020E8B40u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Du, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0910 (initializer)
void __cdecl Fn8F0910() {
    Thiscall(0x0076B2F0, 0x020E8B88u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Eu, 0x00000001u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0950 (initializer)
void __cdecl Fn8F0950() {
    Thiscall(0x0076B2F0, 0x020E8BD0u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x0000004Fu, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0990 (initializer)
void __cdecl Fn8F0990() {
    Thiscall(0x0076B2F0, 0x020E8C18u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000050u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F09D0 (initializer)
void __cdecl Fn8F09D0() {
    Thiscall(0x0076B2F0, 0x020E8C60u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000051u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0A10 (initializer)
void __cdecl Fn8F0A10() {
    Thiscall(0x0076B2F0, 0x020E8CA8u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000052u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0A50 (initializer)
void __cdecl Fn8F0A50() {
    Thiscall(0x0076B2F0, 0x020E8CF0u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000053u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0A90 (initializer)
void __cdecl Fn8F0A90() {
    Thiscall(0x0076B2F0, 0x020E8D38u, 0x0000000Au, 0x00000000u, 0x00000000u, 0x00000001u, 0x00000054u, 0x00000000u, 0x00000004u, 0x00000004u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0AD0 (initializer)
void __cdecl Fn8F0AD0() {
    Thiscall(0x0076B2F0, 0x020E8D80u, 0x00000000u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000055u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000005u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000000u, 0x00000013u, 0x00000005u, 0x00000000u, 0x00000000u);
}

// 0x8F0B10 (initializer)
void __cdecl Fn8F0B10() {
    Thiscall(0x0076B2F0, 0x020E8DC8u, 0x00000001u, 0x00000010u, 0x00000000u, 0x00000001u, 0x00000056u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000005u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000005u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000001u, 0x00000000u);
}

// 0x8F0B50 (initializer)
void __cdecl Fn8F0B50() {
    Thiscall(0x0076B2F0, 0x020E8E10u, 0x00000001u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000057u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000003u, 0x00000000u, 0x00000008u, 0x00000000u);
}

// 0x8F0B90 (initializer)
void __cdecl Fn8F0B90() {
    Thiscall(0x0076B2F0, 0x020E8E58u, 0x00000000u, 0x00000020u, 0x00000000u, 0x00000001u, 0x00000058u, 0x00000000u, 0x00000002u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000001u, 0x00000000u, 0x00000008u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000008u, 0x00000000u, 0x0000000Eu, 0x00000005u, 0x00000008u, 0x00000000u);
}

// 0x8F0BD0 (initializer)
void __cdecl Fn8F0BD0() {
    Stdcall(Dword(0x008FC084), 0x020E8F0Cu);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB900));
}

// 0x8F0BF0 (initializer)
uint32_t __cdecl Fn8F0BF0() {
    return Thiscall(0x008BAC00, 0x020E8F2Cu);
}

// 0x8F0C00 (initializer)
void __cdecl Fn8F0C00() {
    Thiscall(0x00765410, 0x020E8F30u, 0x0099FFC8u /* "NiD3D10Renderer" */, 0x020E2384u);  // NiRTTI::NiRTTI
}

// 0x8F0C20 (initializer)
void __cdecl Fn8F0C20() {
    Thiscall(0x00765410, 0x020E9000u, 0x009A0928u /* "NiD3D10FragmentShader" */, 0x020E922Cu);  // NiRTTI::NiRTTI
}

// 0x8F0C40 (initializer)
void __cdecl Fn8F0C40() {
    Thiscall(0x00765410, 0x020E9034u, 0x009A09C8u /* "NiD3D10ShadowWriteShader" */, 0x020E9000u);  // NiRTTI::NiRTTI
}

// 0x8F0C60 (initializer)
void __cdecl Fn8F0C60() {
    Thiscall(0x00765410, 0x020E9068u, 0x009A0AB0u /* "NiD3D102DBufferData" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F0C80 (initializer)
void __cdecl Fn8F0C80() {
    Thiscall(0x00765410, 0x020E909Cu, 0x009A0AC4u /* "NiD3D10RenderTargetBufferData" */, 0x020E9068u);  // NiRTTI::NiRTTI
}

// 0x8F0CA0 (initializer)
void __cdecl Fn8F0CA0() {
    Thiscall(0x00765410, 0x020E90D0u, 0x009A0AE4u /* "NiD3D10SwapChainBufferData" */, 0x020E909Cu);  // NiRTTI::NiRTTI
}

// 0x8F0CC0 (initializer)
void __cdecl Fn8F0CC0() {
    Thiscall(0x00765410, 0x020E9104u, 0x009A0B00u /* "NiD3D10DepthStencilBufferData" */, 0x020E9068u);  // NiRTTI::NiRTTI
}

// 0x8F0CE0 (initializer)
void __cdecl Fn8F0CE0() {
    Thiscall(0x00765410, 0x020E9138u, 0x009A0B20u /* "NiD3D10Direct3DTexture" */, 0x020E2184u);  // NiRTTI::NiRTTI
}

// 0x8F0D00 (initializer)
void __cdecl Fn8F0D00() {
    Thiscall(0x00765410, 0x020E9170u, 0x009A1180u /* "NiD3D10ShaderInterface" */, 0x020E3164u);  // NiRTTI::NiRTTI
}

// 0x8F0D20 (initializer)
void __cdecl Fn8F0D20() {
    Thiscall(0x00765410, 0x020E91A8u, 0x009A14E0u /* "NiD3D10ErrorShader" */, 0x020E922Cu);  // NiRTTI::NiRTTI
}

// 0x8F0D40 (initializer)
void __cdecl Fn8F0D40() {
    Thiscall(0x00765410, 0x020E91E4u, 0x009A1510u /* "NiD3D10ShaderProgram" */, 0x020E965Cu);  // NiRTTI::NiRTTI
}

// 0x8F0D60 (initializer)
void __cdecl Fn8F0D60() {
    Thiscall(0x00765410, 0x020E922Cu, 0x009A1600u /* "NiD3D10Shader" */, 0x020E9170u);  // NiRTTI::NiRTTI
}

// 0x8F0D80 (initializer)
void __cdecl Fn8F0D80() {
    Dword(0x020E9260) = Dword(0x00951028);
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB910));
}

// 0x8F0DA0 (initializer)
void __cdecl Fn8F0DA0() {
    Thiscall(0x00765410, 0x020E9268u, 0x009A1658u /* "NiD3D10PixelShader" */, 0x020E91E4u);  // NiRTTI::NiRTTI
}

// 0x8F0DC0 (initializer)
void __cdecl Fn8F0DC0() {
    Thiscall(0x00765410, 0x020E929Cu, 0x009A1688u /* "NiD3D10GeometryShader" */, 0x020E91E4u);  // NiRTTI::NiRTTI
}

// 0x8F0DE0 (initializer)
void __cdecl Fn8F0DE0() {
    Thiscall(0x00765410, 0x020E92D0u, 0x009A16BCu /* "NiD3D10VertexShader" */, 0x020E91E4u);  // NiRTTI::NiRTTI
}

// 0x8F0E00 (initializer)
void __cdecl Fn8F0E00() {
    Thiscall(0x00765410, 0x020E930Cu, 0x009A1784u /* "NiSystemCursor" */, 0x020E9378u);  // NiRTTI::NiRTTI
}

// 0x8F0E20 (initializer)
void __cdecl Fn8F0E20() {
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB950));
}

// 0x8F0E30 (initializer)
void __cdecl Fn8F0E30() {
    Thiscall(0x00765410, 0x020E9344u, 0x009A1794u /* "NiCursorRenderClick" */, 0x020E2D84u);  // NiRTTI::NiRTTI
}

// 0x8F0E50 (initializer)
void __cdecl Fn8F0E50() {
    Thiscall(0x00765410, 0x020E9378u, 0x009A1800u /* "NiCursor" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F0E70 (initializer)
void __cdecl Fn8F0E70() {
    uint32_t r1 = Stdcall(Dword(0x008FC1AC));
    Dword(0x020E93AC) = r1;
    crt::Atexit(Fn<void(__cdecl*)()>(0x008FB980));
}

// 0x8F0E90 (initializer)
void __cdecl Fn8F0E90() {
    Thiscall(0x00765410, 0x020E94B4u, 0x009A182Cu /* "NiTask" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8F0EB0 (initializer)
void __cdecl Fn8F0EB0() {
    Thiscall(0x00765410, 0x020E94ECu, 0x009A1834u /* "NiParallelUpdateTaskManager" */, 0x020E9690u);  // NiRTTI::NiRTTI
}

// 0x8F0ED0 (initializer)
void __cdecl Fn8F0ED0() {
    Thiscall(0x00765410, 0x020E9520u, 0x009A1850u /* "NiParallelUpdateTaskManager::SignalTask" */, 0x020E94B4u);  // NiRTTI::NiRTTI
}

// 0x8F0EF0 (initializer)
void __cdecl Fn8F0EF0() {
    Thiscall(0x00765410, 0x020E9554u, 0x009A187Cu /* "NiAccumulatorProcessor" */, 0x020E2E28u);  // NiRTTI::NiRTTI
}

// 0x8F0F10 (initializer)
void __cdecl Fn8F0F10() {
    Thiscall(0x00765410, 0x020E9588u, 0x009A1894u /* "Ni2DRenderView" */, 0x020E3590u);  // NiRTTI::NiRTTI
}

// 0x8F0F30 (initializer)
void __cdecl Fn8F0F30() {
    Thiscall(0x00765410, 0x020E95BCu, 0x009A18A4u /* "NiScreenTextureRenderClick" */, 0x020E2D84u);  // NiRTTI::NiRTTI
}

// 0x8F0F50 (initializer)
void __cdecl Fn8F0F50() {
    Thiscall(0x00765410, 0x020E95F0u, 0x009A1968u /* "NiSCMExtraData" */, 0x020E2854u);  // NiRTTI::NiRTTI
}

// 0x8F0F70 (initializer)
void __cdecl Fn8F0F70() {
    Thiscall(0x00765410, 0x020E9624u, 0x009A1A4Cu /* "NiGPUProgramCache" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F0F90 (initializer)
void __cdecl Fn8F0F90() {
    Thiscall(0x00765410, 0x020E965Cu, 0x009A1A80u /* "NiGPUProgram" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F0FB0 (initializer)
void __cdecl Fn8F0FB0() {
    Thiscall(0x00765410, 0x020E9690u, 0x009A1A90u /* "NiTaskManager" */, 0x020E2248u);  // NiRTTI::NiRTTI
}

// 0x8F0FD0 (initializer)
void __cdecl Fn8F0FD0() {
    Thiscall(0x00765410, 0x020E96C4u, 0x009A1B58u /* "NiInputGamePad" */, 0x020E97DCu);  // NiRTTI::NiRTTI
}

// 0x8F0FF0 (initializer)
void __cdecl Fn8F0FF0() {
    Thiscall(0x00765410, 0x020E9704u, 0x009A1CD4u /* "NiDI8InputSystem" */, 0x020E9770u);  // NiRTTI::NiRTTI
}

// 0x8F1010 (initializer)
void __cdecl Fn8F1010() {
    Thiscall(0x00765410, 0x020E9738u, 0x009A1CE8u /* "NiDI8InputSystem::DI8CreateParams" */, 0x020E97A4u);  // NiRTTI::NiRTTI
}

// 0x8F1030 (initializer)
void __cdecl Fn8F1030() {
    Thiscall(0x00765410, 0x020E9770u, 0x009A1D8Cu /* "NiInputSystem" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F1050 (initializer)
void __cdecl Fn8F1050() {
    Thiscall(0x00765410, 0x020E97A4u, 0x009A1D9Cu /* "NiInputSystem::CreateParams" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F1070 (initializer)
void __cdecl Fn8F1070() {
    Thiscall(0x00765410, 0x020E97DCu, 0x009A1DE0u /* "NiInputDevice" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F1090 (initializer)
void __cdecl Fn8F1090() {
    Thiscall(0x00765410, 0x020E9810u, 0x009A1E20u /* "NiInputDI8Mouse" */, 0x020E98B0u);  // NiRTTI::NiRTTI
}

// 0x8F10B0 (initializer)
void __cdecl Fn8F10B0() {
    Thiscall(0x00765410, 0x020E9844u, 0x009A1E58u /* "NiInputDI8Keyboard" */, 0x020E98E4u);  // NiRTTI::NiRTTI
}

// 0x8F10D0 (initializer)
void __cdecl Fn8F10D0() {
    Thiscall(0x00765410, 0x020E987Cu, 0x009A1EB8u /* "NiInputDI8GamePad" */, 0x020E96C4u);  // NiRTTI::NiRTTI
}

// 0x8F10F0 (initializer)
void __cdecl Fn8F10F0() {
    Thiscall(0x00765410, 0x020E98B0u, 0x009A1EFCu /* "NiInputMouse" */, 0x020E97DCu);  // NiRTTI::NiRTTI
}

// 0x8F1110 (initializer)
void __cdecl Fn8F1110() {
    Thiscall(0x00765410, 0x020E98E4u, 0x009A1F34u /* "NiInputKeyboard" */, 0x020E97DCu);  // NiRTTI::NiRTTI
}

// 0x8F1130 (initializer)
void __cdecl Fn8F1130() {
    Thiscall(0x00765410, 0x020EA918u, 0x009A1FD0u /* "NiFont" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F1150 (initializer)
void __cdecl Fn8F1150() {
    Thiscall(0x00765410, 0x020EAB58u, 0x009A1FE4u /* "NiFontString" */, 0x00000000u);  // NiRTTI::NiRTTI
}

// 0x8F1170 (exit)
void __cdecl Fn8F1170() {
    ReleaseObject(0x00BCBBAC);
}

// 0x8F11A0 (exit)
void __cdecl Fn8F11A0() {
    ReleaseObject(0x00BCBBB0);
}

// 0x8F11D0 (exit)
void __cdecl Fn8F11D0() {
    ReleaseObject(0x00BCBBB4);
}

// 0x8F1200 (exit)
void __cdecl Fn8F1200() {
    ReleaseObject(0x00BCBBB8);
}

// 0x8F1230 (exit)
void __cdecl Fn8F1230() {
    ReleaseObject(0x00BCBBBC);
}

// 0x8F1260 (exit)
void __cdecl Fn8F1260() {
    ReleaseObject(0x00BCBBC0);
}

// 0x8F1290 (exit)
void __cdecl Fn8F1290() {
    ReleaseObject(0x00BCBBC4);
}

// 0x8F12C0 (exit)
void __cdecl Fn8F12C0() {
    ReleaseObject(0x00BCBBC8);
}

// 0x8F12F0 (exit)
void __cdecl Fn8F12F0() {
}

// 0x8F1300 (exit)
void __cdecl Fn8F1300() {
}

// 0x8F1330 (exit)
void __cdecl Fn8F1330() {
    ReleaseFixedString(0x00BCC490);
}

// 0x8F1370 (exit)
void __cdecl Fn8F1370() {
    ReleaseFixedString(0x00BCC498);
}

// 0x8F13B0 (exit)
void __cdecl Fn8F13B0() {
    ReleaseFixedString(0x00BCC49C);
}

// 0x8F13F0 (exit)
void __cdecl Fn8F13F0() {
    ReleaseFixedString(0x00BCC4A0);
}

// 0x8F1430 (exit)
void __cdecl Fn8F1430() {
    ReleaseFixedString(0x00BCC4A4);
}

// 0x8F1470 (exit)
void __cdecl Fn8F1470() {
    ReleaseFixedString(0x00BCC4AC);
}

// 0x8F14B0 (exit)
void __cdecl Fn8F14B0() {
    ReleaseFixedString(0x00BCC4B0);
}

// 0x8F14F0 (exit)
void __cdecl Fn8F14F0() {
    ReleaseFixedString(0x00BCC4B4);
}

// 0x8F1530 (exit)
void __cdecl Fn8F1530() {
    ReleaseFixedString(0x00BCC4B8);
}

// 0x8F1570 (exit)
void __cdecl Fn8F1570() {
    ReleaseFixedString(0x00BCC4C0);
}

// 0x8F15B0 (exit)
void __cdecl Fn8F15B0() {
    ReleaseFixedString(0x00BCC4C4);
}

// 0x8F15F0 (exit)
void __cdecl Fn8F15F0() {
    ReleaseFixedString(0x00BCC4C8);
}

// 0x8F1630 (exit)
void __cdecl Fn8F1630() {
    ReleaseFixedString(0x00BCC4CC);
}

// 0x8F1670 (exit)
void __cdecl Fn8F1670() {
    ReleaseFixedString(0x00BCC4D4);
}

// 0x8F16B0 (exit)
void __cdecl Fn8F16B0() {
    ReleaseFixedString(0x00BCC4D8);
}

// 0x8F16F0 (exit)
void __cdecl Fn8F16F0() {
    ReleaseFixedString(0x00BCC4DC);
}

// 0x8F1730 (exit)
void __cdecl Fn8F1730() {
    ReleaseFixedString(0x00BCC4E0);
}

// 0x8F1770 (exit)
void __cdecl Fn8F1770() {
    ReleaseFixedString(0x00BCC4E8);
}

// 0x8F17B0 (exit)
void __cdecl Fn8F17B0() {
    ReleaseFixedString(0x00BCC4EC);
}

// 0x8F17F0 (exit)
void __cdecl Fn8F17F0() {
    ReleaseFixedString(0x00BCC4F0);
}

// 0x8F1830 (exit)
void __cdecl Fn8F1830() {
    ReleaseFixedString(0x00BCC4F4);
}

// 0x8F1870 (exit)
void __cdecl Fn8F1870() {
    ReleaseFixedString(0x00BCC4FC);
}

// 0x8F18B0 (exit)
void __cdecl Fn8F18B0() {
    ReleaseFixedString(0x00BCC500);
}

// 0x8F18F0 (exit)
void __cdecl Fn8F18F0() {
    ReleaseFixedString(0x00BCC504);
}

// 0x8F1930 (exit)
void __cdecl Fn8F1930() {
    ReleaseFixedString(0x00BCC508);
}

// 0x8F1970 (exit)
void __cdecl Fn8F1970() {
    ReleaseFixedString(0x00BCC510);
}

// 0x8F19B0 (exit)
void __cdecl Fn8F19B0() {
    ReleaseFixedString(0x00BCC514);
}

// 0x8F19F0 (exit)
void __cdecl Fn8F19F0() {
    ReleaseFixedString(0x00BCC518);
}

// 0x8F1A30 (exit)
void __cdecl Fn8F1A30() {
    ReleaseFixedString(0x00BCC51C);
}

// 0x8F1A70 (exit)
void __cdecl Fn8F1A70() {
    ReleaseFixedString(0x00BCC524);
}

// 0x8F1AB0 (exit)
void __cdecl Fn8F1AB0() {
    ReleaseFixedString(0x00BCC528);
}

// 0x8F1AF0 (exit)
void __cdecl Fn8F1AF0() {
    ReleaseFixedString(0x00BCC52C);
}

// 0x8F1B30 (exit)
void __cdecl Fn8F1B30() {
    ReleaseFixedString(0x00BCC530);
}

// 0x8F1B70 (exit)
void __cdecl Fn8F1B70() {
    ReleaseFixedString(0x00BCC538);
}

// 0x8F1BB0 (exit)
void __cdecl Fn8F1BB0() {
    ReleaseFixedString(0x00BCC53C);
}

// 0x8F1BF0 (exit)
void __cdecl Fn8F1BF0() {
    ReleaseFixedString(0x00BCC540);
}

// 0x8F1C30 (exit)
void __cdecl Fn8F1C30() {
    ReleaseFixedString(0x00BCC544);
}

// 0x8F1CA0 (exit)
void __cdecl Fn8F1CA0() {
    Dword(0x00BCC580) = 0x00900A90u;
}

// 0x8F1CB0 (exit)
void __cdecl Fn8F1CB0() {
}

// 0x8F1CC0 (exit)
void __cdecl Fn8F1CC0() {
    Dword(0x009E5158) = 0x00900F5Cu /* "pWB" */;
}

// 0x8F1CD0 (exit)
uint32_t __cdecl Fn8F1CD0() {
    Dword(0x00BCEB50) = 0x00901088u;
    return Thiscall(0x005D9C50, 0x00BD09DCu);  // ScriptRef_Clear
}

// 0x8F1D50 (exit)
uint32_t __cdecl Fn8F1D50() {
    return Thiscall(0x005D9C50, 0x00BD3E68u);  // ScriptRef_Clear
}

// 0x8F1DB0 (exit)
void __cdecl Fn8F1DB0() {
    Dword(0x00A13650) = 0x0090387Cu;
}

// 0x8F1DC0 (exit)
void __cdecl Fn8F1DC0() {
}

// 0x8F1DD0 (exit)
void __cdecl Fn8F1DD0() {
}

// 0x8F1DF0 (exit)
uint32_t __cdecl Fn8F1DF0() {
    return Thiscall(0x0052C6D0, 0x00BF3E08u);  // RadarSlot_Destruct
}

// 0x8F1E00 (exit)
void __cdecl Fn8F1E00() {
}

// 0x8F1E10 (exit)
void __cdecl Fn8F1E10() {
}

// 0x8F1E20 (exit)
void __cdecl Fn8F1E20() {
    Dword(0x00A144B4) = 0x00900CDCu;
}

// 0x8F1E30 (exit)
uint32_t __cdecl Fn8F1E30() {
    return Thiscall(0x0043FE00, 0x00BF6228u);  // CPaths::DestructEntries
}

// 0x8F1E40 (exit)
uint32_t __cdecl Fn8F1E40() {
    return Thiscall(0x0072A900, 0x00C0F560u);  // PickupButes::Destruct
}

// 0x8F1E80 (exit)
void __cdecl Fn8F1E80() {
}

// 0x8F1EA0 (exit)
void __cdecl Fn8F1EA0() {
}

// 0x8F1EB0 (exit)
uint32_t __cdecl Fn8F1EB0() {
    return Thiscall(0x00453AD0, 0x00C13F70u);  // EffectProxyBank::Destruct
}

// 0x8F1EC0 (exit)
uint32_t __cdecl Fn8F1EC0() {
    return Thiscall(0x00453CF0, 0x00C13FD8u);  // CAreaEffects::Destruct
}

// 0x8F1ED0 (exit)
void __cdecl Fn8F1ED0() {
}

// 0x8F1EE0 (exit)
void __cdecl Fn8F1EE0() {
}

// 0x8F1EF0 (exit)
void __cdecl Fn8F1EF0() {
}

// 0x8F1F00 (exit)
void __cdecl Fn8F1F00() {
}

// 0x8F1F10 (exit)
void __cdecl Fn8F1F10() {
}

// 0x8F1F20 (exit)
void __cdecl Fn8F1F20() {
}

// 0x8F1F30 (exit)
void __cdecl Fn8F1F30() {
}

// 0x8F1F40 (exit)
void __cdecl Fn8F1F40() {
}

// 0x8F1F50 (exit)
void __cdecl Fn8F1F50() {
}

// 0x8F1F60 (exit)
uint32_t __cdecl Fn8F1F60() {
    return Thiscall(0x00457A20, 0x00C18748u);  // CShop::~CShop
}

// 0x8F1F70 (exit)
uint32_t __cdecl Fn8F1F70() {
    return Thiscall(0x0049A650, 0x00C1A760u);  // NameKey_Destruct
}

// 0x8F1F80 (exit)
uint32_t __cdecl Fn8F1F80() {
    return Thiscall(0x0049A650, 0x00C1A764u);  // NameKey_Destruct
}

// 0x8F1F90 (exit)
uint32_t __cdecl Fn8F1F90() {
    return Thiscall(0x0049A650, 0x00C1A768u);  // NameKey_Destruct
}

// 0x8F1FA0 (exit)
uint32_t __cdecl Fn8F1FA0() {
    return Thiscall(0x0049A650, 0x00C1A76Cu);  // NameKey_Destruct
}

// 0x8F1FB0 (exit)
uint32_t __cdecl Fn8F1FB0() {
    return Thiscall(0x0049A650, 0x00C1A770u);  // NameKey_Destruct
}

// 0x8F1FC0 (exit)
uint32_t __cdecl Fn8F1FC0() {
    return Thiscall(0x0049A650, 0x00C1A774u);  // NameKey_Destruct
}

// 0x8F1FD0 (exit)
uint32_t __cdecl Fn8F1FD0() {
    return Thiscall(0x0049A650, 0x00C1A778u);  // NameKey_Destruct
}

// 0x8F1FE0 (exit)
uint32_t __cdecl Fn8F1FE0() {
    return Thiscall(0x0049A650, 0x00C1A77Cu);  // NameKey_Destruct
}

// 0x8F1FF0 (exit)
uint32_t __cdecl Fn8F1FF0() {
    return Thiscall(0x0049A650, 0x00C1A780u);  // NameKey_Destruct
}

// 0x8F2000 (exit)
uint32_t __cdecl Fn8F2000() {
    return Thiscall(0x0049A650, 0x00C1A784u);  // NameKey_Destruct
}

// 0x8F2010 (exit)
uint32_t __cdecl Fn8F2010() {
    return Thiscall(0x0049A650, 0x00C1A788u);  // NameKey_Destruct
}

// 0x8F2020 (exit)
uint32_t __cdecl Fn8F2020() {
    return Thiscall(0x0049A650, 0x00C1A78Cu);  // NameKey_Destruct
}

// 0x8F2030 (exit)
uint32_t __cdecl Fn8F2030() {
    return Thiscall(0x0049A650, 0x00C1A790u);  // NameKey_Destruct
}

// 0x8F2040 (exit)
uint32_t __cdecl Fn8F2040() {
    return Thiscall(0x0049A650, 0x00C1A794u);  // NameKey_Destruct
}

// 0x8F2050 (exit)
uint32_t __cdecl Fn8F2050() {
    return Thiscall(0x0049A650, 0x00C1A798u);  // NameKey_Destruct
}

// 0x8F2060 (exit)
uint32_t __cdecl Fn8F2060() {
    return Thiscall(0x0049A650, 0x00C1A79Cu);  // NameKey_Destruct
}

// 0x8F2070 (exit)
uint32_t __cdecl Fn8F2070() {
    return Thiscall(0x0049A650, 0x00C1A7A0u);  // NameKey_Destruct
}

// 0x8F2080 (exit)
uint32_t __cdecl Fn8F2080() {
    return Thiscall(0x0049A650, 0x00C1A7A4u);  // NameKey_Destruct
}

// 0x8F2090 (exit)
uint32_t __cdecl Fn8F2090() {
    return Thiscall(0x0049A650, 0x00C1A7A8u);  // NameKey_Destruct
}

// 0x8F20A0 (exit)
uint32_t __cdecl Fn8F20A0() {
    return Thiscall(0x0049A650, 0x00C1A7ACu);  // NameKey_Destruct
}

// 0x8F20B0 (exit)
uint32_t __cdecl Fn8F20B0() {
    return Thiscall(0x0049A650, 0x00C1A7B0u);  // NameKey_Destruct
}

// 0x8F20C0 (exit)
uint32_t __cdecl Fn8F20C0() {
    return Thiscall(0x0049A650, 0x00C1A7B4u);  // NameKey_Destruct
}

// 0x8F20D0 (exit)
uint32_t __cdecl Fn8F20D0() {
    return Thiscall(0x0049A650, 0x00C1A7B8u);  // NameKey_Destruct
}

// 0x8F20E0 (exit)
uint32_t __cdecl Fn8F20E0() {
    return Thiscall(0x0049A650, 0x00C1A7BCu);  // NameKey_Destruct
}

// 0x8F20F0 (exit)
uint32_t __cdecl Fn8F20F0() {
    return Thiscall(0x0049A650, 0x00C1A7C0u);  // NameKey_Destruct
}

// 0x8F2100 (exit)
uint32_t __cdecl Fn8F2100() {
    return Thiscall(0x0049A650, 0x00C1A7C4u);  // NameKey_Destruct
}

// 0x8F2110 (exit)
uint32_t __cdecl Fn8F2110() {
    return Thiscall(0x0049A650, 0x00C1A7C8u);  // NameKey_Destruct
}

// 0x8F2120 (exit)
void __cdecl Fn8F2120() {
}

// 0x8F2140 (exit)
void __cdecl Fn8F2140() {
}

// 0x8F2190 (exit)
uint32_t __cdecl Fn8F2190() {
    return Thiscall(0x0044E070, 0x00C1AEA4u);  // CPtrList::Flush
}

// 0x8F21A0 (exit)
uint32_t __cdecl Fn8F21A0() {
    return Thiscall(0x0044E070, 0x00C216B8u);  // CPtrList::Flush
}

// 0x8F2230 (exit)
void __cdecl Fn8F2230() {
}

// 0x8F2240 (exit)
void __cdecl Fn8F2240() {
}

// 0x8F2250 (exit)
void __cdecl Fn8F2250() {
}

// 0x8F2340 (exit)
void __cdecl Fn8F2340() {
}

// 0x8F2350 (exit)
void __cdecl Fn8F2350() {
}

// 0x8F2390 (exit)
void __cdecl Fn8F2390() {
}

// 0x8F24A0 (exit)
void __cdecl Fn8F24A0() {
}

// 0x8F24B0 (exit)
uint32_t __cdecl Fn8F24B0() {
    return Thiscall(0x0049A650, 0x00C3A978u);  // NameKey_Destruct
}

// 0x8F24C0 (exit)
void __cdecl Fn8F24C0() {
}

// 0x8F24D0 (exit)
void __cdecl Fn8F24D0() {
}

// 0x8F24F0 (exit)
uint32_t __cdecl Fn8F24F0() {
    return Thiscall(0x004E9910, 0x00C3B808u);  // CDoorManager::Destruct
}

// 0x8F2500 (exit)
void __cdecl Fn8F2500() {
}

// 0x8F2540 (exit)
uint32_t __cdecl Fn8F2540() {
    return Thiscall(0x004F2EA0, 0x00C3CC68u);  // CCamera::~CCamera
}

// 0x8F2550 (exit)
void __cdecl Fn8F2550() {
    Dword(0x00A45A30) = 0x00914800u;
}

// 0x8F2670 (exit)
uint32_t __cdecl Fn8F2670() {
    return Thiscall(0x0050E710, 0x00A45C8Cu);  // LockedMemoryChunks::Destruct_50E710
}

// 0x8F2680 (exit)
void __cdecl Fn8F2680() {
}

// 0x8F2690 (exit)
void __cdecl Fn8F2690() {
}

// 0x8F27A0 (exit)
uint32_t __cdecl Fn8F27A0() {
    return Thiscall(0x00510D90, 0x00C89EECu);  // MIK_ClumpDestruct
}

// 0x8F27D0 (exit)
uint32_t __cdecl Fn8F27D0() {
    return Thiscall(0x004498F0, 0x00A76AD4u);  // DM_PoolFlushVirtual
}

// 0x8F27E0 (exit)
uint32_t __cdecl Fn8F27E0() {
    return Thiscall(0x004498F0, 0x00A76AF0u);  // DM_PoolFlushVirtual
}

// 0x8F27F0 (exit)
uint32_t __cdecl Fn8F27F0() {
    return Thiscall(0x004498F0, 0x00A76B0Cu);  // DM_PoolFlushVirtual
}

// 0x8F2830 (exit)
uint32_t __cdecl Fn8F2830() {
    return Thiscall(0x0051B470, 0x00A76B28u);  // EffectPool_Flush
}

// 0x8F28C0 (exit)
void __cdecl Fn8F28C0() {
}

// 0x8F28D0 (exit)
void __cdecl Fn8F28D0() {
}

// 0x8F28E0 (exit)
void __cdecl Fn8F28E0() {
}

// 0x8F28F0 (exit)
void __cdecl Fn8F28F0() {
}

// 0x8F2900 (exit)
void __cdecl Fn8F2900() {
}

// 0x8F2910 (exit)
void __cdecl Fn8F2910() {
}

// 0x8F2920 (exit)
void __cdecl Fn8F2920() {
}

// 0x8F2930 (exit)
void __cdecl Fn8F2930() {
}

// 0x8F2940 (exit)
uint32_t __cdecl Fn8F2940() {
    return Thiscall(0x005462A0, 0x00AB4688u);  // PhotoAlbum::~PhotoAlbum
}

// 0x8F2950 (exit)
void __cdecl Fn8F2950() {
}

// 0x8F2960 (exit)
void __cdecl Fn8F2960() {
}

// 0x8F2970 (exit)
void __cdecl Fn8F2970() {
}

// 0x8F2980 (exit)
void __cdecl Fn8F2980() {
}

// 0x8F2990 (exit)
void __cdecl Fn8F2990() {
}

// 0x8F29A0 (exit)
void __cdecl Fn8F29A0() {
}

// 0x8F29B0 (exit)
void __cdecl Fn8F29B0() {
}

// 0x8F29C0 (exit)
void __cdecl Fn8F29C0() {
}

// 0x8F29D0 (exit)
void __cdecl Fn8F29D0() {
}

// 0x8F29E0 (exit)
void __cdecl Fn8F29E0() {
}

} // namespace
} // namespace staticinit

#define STATICINIT4_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT4_REVERSED("StaticInit::8EC050", 0x008EC050, &staticinit::Fn8EC050);
STATICINIT4_REVERSED("StaticInit::8EC080", 0x008EC080, &staticinit::Fn8EC080);
STATICINIT4_REVERSED("StaticInit::8EC0A0", 0x008EC0A0, &staticinit::Fn8EC0A0);
STATICINIT4_REVERSED("StaticInit::8EC0C0", 0x008EC0C0, &staticinit::Fn8EC0C0);
STATICINIT4_REVERSED("StaticInit::8EC0D0", 0x008EC0D0, &staticinit::Fn8EC0D0);
STATICINIT4_REVERSED("StaticInit::8EC0F0", 0x008EC0F0, &staticinit::Fn8EC0F0);
STATICINIT4_REVERSED("StaticInit::8EC100", 0x008EC100, &staticinit::Fn8EC100);
STATICINIT4_REVERSED("StaticInit::8EC120", 0x008EC120, &staticinit::Fn8EC120);
STATICINIT4_REVERSED("StaticInit::8EC140", 0x008EC140, &staticinit::Fn8EC140);
STATICINIT4_REVERSED("StaticInit::8EC160", 0x008EC160, &staticinit::Fn8EC160);
STATICINIT4_REVERSED("StaticInit::8EC180", 0x008EC180, &staticinit::Fn8EC180);
STATICINIT4_REVERSED("StaticInit::8EC1A0", 0x008EC1A0, &staticinit::Fn8EC1A0);
STATICINIT4_REVERSED("StaticInit::8EC1C0", 0x008EC1C0, &staticinit::Fn8EC1C0);
STATICINIT4_REVERSED("StaticInit::8EC1E0", 0x008EC1E0, &staticinit::Fn8EC1E0);
STATICINIT4_REVERSED("StaticInit::8EC200", 0x008EC200, &staticinit::Fn8EC200);
STATICINIT4_REVERSED("StaticInit::8EC220", 0x008EC220, &staticinit::Fn8EC220);
STATICINIT4_REVERSED("StaticInit::8EC280", 0x008EC280, &staticinit::Fn8EC280);
STATICINIT4_REVERSED("StaticInit::8EC2A0", 0x008EC2A0, &staticinit::Fn8EC2A0);
STATICINIT4_REVERSED("StaticInit::8EC2C0", 0x008EC2C0, &staticinit::Fn8EC2C0);
STATICINIT4_REVERSED("StaticInit::8EC2E0", 0x008EC2E0, &staticinit::Fn8EC2E0);
STATICINIT4_REVERSED("StaticInit::8EC300", 0x008EC300, &staticinit::Fn8EC300);
STATICINIT4_REVERSED("StaticInit::8EC310", 0x008EC310, &staticinit::Fn8EC310);
STATICINIT4_REVERSED("StaticInit::8EC330", 0x008EC330, &staticinit::Fn8EC330);
STATICINIT4_REVERSED("StaticInit::8EC350", 0x008EC350, &staticinit::Fn8EC350);
STATICINIT4_REVERSED("StaticInit::8EC370", 0x008EC370, &staticinit::Fn8EC370);
STATICINIT4_REVERSED("StaticInit::8EC390", 0x008EC390, &staticinit::Fn8EC390);
STATICINIT4_REVERSED("StaticInit::8EC3B0", 0x008EC3B0, &staticinit::Fn8EC3B0);
STATICINIT4_REVERSED("StaticInit::8EC3D0", 0x008EC3D0, &staticinit::Fn8EC3D0);
STATICINIT4_REVERSED("StaticInit::8EC3E0", 0x008EC3E0, &staticinit::Fn8EC3E0);
STATICINIT4_REVERSED("StaticInit::8EC400", 0x008EC400, &staticinit::Fn8EC400);
STATICINIT4_REVERSED("StaticInit::8EC410", 0x008EC410, &staticinit::Fn8EC410);
STATICINIT4_REVERSED("StaticInit::8EC430", 0x008EC430, &staticinit::Fn8EC430);
STATICINIT4_REVERSED("StaticInit::8EC440", 0x008EC440, &staticinit::Fn8EC440);
STATICINIT4_REVERSED("StaticInit::8EC460", 0x008EC460, &staticinit::Fn8EC460);
STATICINIT4_REVERSED("StaticInit::8EC480", 0x008EC480, &staticinit::Fn8EC480);
STATICINIT4_REVERSED("StaticInit::8EC4A0", 0x008EC4A0, &staticinit::Fn8EC4A0);
STATICINIT4_REVERSED("StaticInit::8EC4B0", 0x008EC4B0, &staticinit::Fn8EC4B0);
STATICINIT4_REVERSED("StaticInit::8EC4D0", 0x008EC4D0, &staticinit::Fn8EC4D0);
STATICINIT4_REVERSED("StaticInit::8EC4F0", 0x008EC4F0, &staticinit::Fn8EC4F0);
STATICINIT4_REVERSED("StaticInit::8EC510", 0x008EC510, &staticinit::Fn8EC510);
STATICINIT4_REVERSED("StaticInit::8EC530", 0x008EC530, &staticinit::Fn8EC530);
STATICINIT4_REVERSED("StaticInit::8EC550", 0x008EC550, &staticinit::Fn8EC550);
STATICINIT4_REVERSED("StaticInit::8EC570", 0x008EC570, &staticinit::Fn8EC570);
STATICINIT4_REVERSED("StaticInit::8EC590", 0x008EC590, &staticinit::Fn8EC590);
STATICINIT4_REVERSED("StaticInit::8EC5B0", 0x008EC5B0, &staticinit::Fn8EC5B0);
STATICINIT4_REVERSED("StaticInit::8EC610", 0x008EC610, &staticinit::Fn8EC610);
STATICINIT4_REVERSED("StaticInit::8EC630", 0x008EC630, &staticinit::Fn8EC630);
STATICINIT4_REVERSED("StaticInit::8EC650", 0x008EC650, &staticinit::Fn8EC650);
STATICINIT4_REVERSED("StaticInit::8EC670", 0x008EC670, &staticinit::Fn8EC670);
STATICINIT4_REVERSED("StaticInit::8EC690", 0x008EC690, &staticinit::Fn8EC690);
STATICINIT4_REVERSED("StaticInit::8EC720", 0x008EC720, &staticinit::Fn8EC720);
STATICINIT4_REVERSED("StaticInit::8EC740", 0x008EC740, &staticinit::Fn8EC740);
STATICINIT4_REVERSED("StaticInit::8EC760", 0x008EC760, &staticinit::Fn8EC760);
STATICINIT4_REVERSED("StaticInit::8EC780", 0x008EC780, &staticinit::Fn8EC780);
STATICINIT4_REVERSED("StaticInit::8EC7A0", 0x008EC7A0, &staticinit::Fn8EC7A0);
STATICINIT4_REVERSED("StaticInit::8EC7C0", 0x008EC7C0, &staticinit::Fn8EC7C0);
STATICINIT4_REVERSED("StaticInit::8EC7E0", 0x008EC7E0, &staticinit::Fn8EC7E0);
STATICINIT4_REVERSED("StaticInit::8EC800", 0x008EC800, &staticinit::Fn8EC800);
STATICINIT4_REVERSED("StaticInit::8EC820", 0x008EC820, &staticinit::Fn8EC820);
STATICINIT4_REVERSED("StaticInit::8EC840", 0x008EC840, &staticinit::Fn8EC840);
STATICINIT4_REVERSED("StaticInit::8EC860", 0x008EC860, &staticinit::Fn8EC860);
STATICINIT4_REVERSED("StaticInit::8EC880", 0x008EC880, &staticinit::Fn8EC880);
STATICINIT4_REVERSED("StaticInit::8EC8A0", 0x008EC8A0, &staticinit::Fn8EC8A0);
STATICINIT4_REVERSED("StaticInit::8EC8C0", 0x008EC8C0, &staticinit::Fn8EC8C0);
STATICINIT4_REVERSED("StaticInit::8EC8E0", 0x008EC8E0, &staticinit::Fn8EC8E0);
STATICINIT4_REVERSED("StaticInit::8EC900", 0x008EC900, &staticinit::Fn8EC900);
STATICINIT4_REVERSED("StaticInit::8EC920", 0x008EC920, &staticinit::Fn8EC920);
STATICINIT4_REVERSED("StaticInit::8EC940", 0x008EC940, &staticinit::Fn8EC940);
STATICINIT4_REVERSED("StaticInit::8EC960", 0x008EC960, &staticinit::Fn8EC960);
STATICINIT4_REVERSED("StaticInit::8EC980", 0x008EC980, &staticinit::Fn8EC980);
STATICINIT4_REVERSED("StaticInit::8EC9A0", 0x008EC9A0, &staticinit::Fn8EC9A0);
STATICINIT4_REVERSED("StaticInit::8EC9C0", 0x008EC9C0, &staticinit::Fn8EC9C0);
STATICINIT4_REVERSED("StaticInit::8EC9E0", 0x008EC9E0, &staticinit::Fn8EC9E0);
STATICINIT4_REVERSED("StaticInit::8ECA00", 0x008ECA00, &staticinit::Fn8ECA00);
STATICINIT4_REVERSED("StaticInit::8ECA20", 0x008ECA20, &staticinit::Fn8ECA20);
STATICINIT4_REVERSED("StaticInit::8ECA40", 0x008ECA40, &staticinit::Fn8ECA40);
STATICINIT4_REVERSED("StaticInit::8ECA50", 0x008ECA50, &staticinit::Fn8ECA50);
STATICINIT4_REVERSED("StaticInit::8ECA70", 0x008ECA70, &staticinit::Fn8ECA70);
STATICINIT4_REVERSED("StaticInit::8ECA90", 0x008ECA90, &staticinit::Fn8ECA90);
STATICINIT4_REVERSED("StaticInit::8ECAB0", 0x008ECAB0, &staticinit::Fn8ECAB0);
STATICINIT4_REVERSED("StaticInit::8ECAD0", 0x008ECAD0, &staticinit::Fn8ECAD0);
STATICINIT4_REVERSED("StaticInit::8ECAF0", 0x008ECAF0, &staticinit::Fn8ECAF0);
STATICINIT4_REVERSED("StaticInit::8ECB10", 0x008ECB10, &staticinit::Fn8ECB10);
STATICINIT4_REVERSED("StaticInit::8ECB30", 0x008ECB30, &staticinit::Fn8ECB30);
STATICINIT4_REVERSED("StaticInit::8ECB50", 0x008ECB50, &staticinit::Fn8ECB50);
STATICINIT4_REVERSED("StaticInit::8ECB60", 0x008ECB60, &staticinit::Fn8ECB60);
STATICINIT4_REVERSED("StaticInit::8ECB80", 0x008ECB80, &staticinit::Fn8ECB80);
STATICINIT4_REVERSED("StaticInit::8ECBA0", 0x008ECBA0, &staticinit::Fn8ECBA0);
STATICINIT4_REVERSED("StaticInit::8ECBC0", 0x008ECBC0, &staticinit::Fn8ECBC0);
STATICINIT4_REVERSED("StaticInit::8ECBE0", 0x008ECBE0, &staticinit::Fn8ECBE0);
STATICINIT4_REVERSED("StaticInit::8ECC00", 0x008ECC00, &staticinit::Fn8ECC00);
STATICINIT4_REVERSED("StaticInit::8ECC20", 0x008ECC20, &staticinit::Fn8ECC20);
STATICINIT4_REVERSED("StaticInit::8ECC40", 0x008ECC40, &staticinit::Fn8ECC40);
STATICINIT4_REVERSED("StaticInit::8ECC60", 0x008ECC60, &staticinit::Fn8ECC60);
STATICINIT4_REVERSED("StaticInit::8ECC70", 0x008ECC70, &staticinit::Fn8ECC70);
STATICINIT4_REVERSED("StaticInit::8ECC90", 0x008ECC90, &staticinit::Fn8ECC90);
STATICINIT4_REVERSED("StaticInit::8ECCB0", 0x008ECCB0, &staticinit::Fn8ECCB0);
STATICINIT4_REVERSED("StaticInit::8ECCD0", 0x008ECCD0, &staticinit::Fn8ECCD0);
STATICINIT4_REVERSED("StaticInit::8ECCF0", 0x008ECCF0, &staticinit::Fn8ECCF0);
STATICINIT4_REVERSED("StaticInit::8ECD10", 0x008ECD10, &staticinit::Fn8ECD10);
STATICINIT4_REVERSED("StaticInit::8ECD30", 0x008ECD30, &staticinit::Fn8ECD30);
STATICINIT4_REVERSED("StaticInit::8ECD40", 0x008ECD40, &staticinit::Fn8ECD40);
STATICINIT4_REVERSED("StaticInit::8ECD60", 0x008ECD60, &staticinit::Fn8ECD60);
STATICINIT4_REVERSED("StaticInit::8ECD80", 0x008ECD80, &staticinit::Fn8ECD80);
STATICINIT4_REVERSED("StaticInit::8ECDA0", 0x008ECDA0, &staticinit::Fn8ECDA0);
STATICINIT4_REVERSED("StaticInit::8ECDC0", 0x008ECDC0, &staticinit::Fn8ECDC0);
STATICINIT4_REVERSED("StaticInit::8ECDD0", 0x008ECDD0, &staticinit::Fn8ECDD0);
STATICINIT4_REVERSED("StaticInit::8ECDF0", 0x008ECDF0, &staticinit::Fn8ECDF0);
STATICINIT4_REVERSED("StaticInit::8ECE10", 0x008ECE10, &staticinit::Fn8ECE10);
STATICINIT4_REVERSED("StaticInit::8ECE30", 0x008ECE30, &staticinit::Fn8ECE30);
STATICINIT4_REVERSED("StaticInit::8ECE50", 0x008ECE50, &staticinit::Fn8ECE50);
STATICINIT4_REVERSED("StaticInit::8ECE70", 0x008ECE70, &staticinit::Fn8ECE70);
STATICINIT4_REVERSED("StaticInit::8ECE90", 0x008ECE90, &staticinit::Fn8ECE90);
STATICINIT4_REVERSED("StaticInit::8ECEB0", 0x008ECEB0, &staticinit::Fn8ECEB0);
STATICINIT4_REVERSED("StaticInit::8ECEC0", 0x008ECEC0, &staticinit::Fn8ECEC0);
STATICINIT4_REVERSED("StaticInit::8ECEE0", 0x008ECEE0, &staticinit::Fn8ECEE0);
STATICINIT4_REVERSED("StaticInit::8ECF00", 0x008ECF00, &staticinit::Fn8ECF00);
STATICINIT4_REVERSED("StaticInit::8ECF20", 0x008ECF20, &staticinit::Fn8ECF20);
STATICINIT4_REVERSED("StaticInit::8ECF40", 0x008ECF40, &staticinit::Fn8ECF40);
STATICINIT4_REVERSED("StaticInit::8ECF60", 0x008ECF60, &staticinit::Fn8ECF60);
STATICINIT4_REVERSED("StaticInit::8ECF80", 0x008ECF80, &staticinit::Fn8ECF80);
STATICINIT4_REVERSED("StaticInit::8ECFA0", 0x008ECFA0, &staticinit::Fn8ECFA0);
STATICINIT4_REVERSED("StaticInit::8ECFC0", 0x008ECFC0, &staticinit::Fn8ECFC0);
STATICINIT4_REVERSED("StaticInit::8ECFE0", 0x008ECFE0, &staticinit::Fn8ECFE0);
STATICINIT4_REVERSED("StaticInit::8ED000", 0x008ED000, &staticinit::Fn8ED000);
STATICINIT4_REVERSED("StaticInit::8ED020", 0x008ED020, &staticinit::Fn8ED020);
STATICINIT4_REVERSED("StaticInit::8ED0F0", 0x008ED0F0, &staticinit::Fn8ED0F0);
STATICINIT4_REVERSED("StaticInit::8ED110", 0x008ED110, &staticinit::Fn8ED110);
STATICINIT4_REVERSED("StaticInit::8ED130", 0x008ED130, &staticinit::Fn8ED130);
STATICINIT4_REVERSED("StaticInit::8ED150", 0x008ED150, &staticinit::Fn8ED150);
STATICINIT4_REVERSED("StaticInit::8ED170", 0x008ED170, &staticinit::Fn8ED170);
STATICINIT4_REVERSED("StaticInit::8ED190", 0x008ED190, &staticinit::Fn8ED190);
STATICINIT4_REVERSED("StaticInit::8ED1B0", 0x008ED1B0, &staticinit::Fn8ED1B0);
STATICINIT4_REVERSED("StaticInit::8ED1D0", 0x008ED1D0, &staticinit::Fn8ED1D0);
STATICINIT4_REVERSED("StaticInit::8ED1F0", 0x008ED1F0, &staticinit::Fn8ED1F0);
STATICINIT4_REVERSED("StaticInit::8ED210", 0x008ED210, &staticinit::Fn8ED210);
STATICINIT4_REVERSED("StaticInit::8ED230", 0x008ED230, &staticinit::Fn8ED230);
STATICINIT4_REVERSED("StaticInit::8ED250", 0x008ED250, &staticinit::Fn8ED250);
STATICINIT4_REVERSED("StaticInit::8ED270", 0x008ED270, &staticinit::Fn8ED270);
STATICINIT4_REVERSED("StaticInit::8ED290", 0x008ED290, &staticinit::Fn8ED290);
STATICINIT4_REVERSED("StaticInit::8ED2B0", 0x008ED2B0, &staticinit::Fn8ED2B0);
STATICINIT4_REVERSED("StaticInit::8ED2D0", 0x008ED2D0, &staticinit::Fn8ED2D0);
STATICINIT4_REVERSED("StaticInit::8ED2F0", 0x008ED2F0, &staticinit::Fn8ED2F0);
STATICINIT4_REVERSED("StaticInit::8ED310", 0x008ED310, &staticinit::Fn8ED310);
STATICINIT4_REVERSED("StaticInit::8ED330", 0x008ED330, &staticinit::Fn8ED330);
STATICINIT4_REVERSED("StaticInit::8ED350", 0x008ED350, &staticinit::Fn8ED350);
STATICINIT4_REVERSED("StaticInit::8ED370", 0x008ED370, &staticinit::Fn8ED370);
STATICINIT4_REVERSED("StaticInit::8ED390", 0x008ED390, &staticinit::Fn8ED390);
STATICINIT4_REVERSED("StaticInit::8ED3B0", 0x008ED3B0, &staticinit::Fn8ED3B0);
STATICINIT4_REVERSED("StaticInit::8ED3D0", 0x008ED3D0, &staticinit::Fn8ED3D0);
STATICINIT4_REVERSED("StaticInit::8ED3F0", 0x008ED3F0, &staticinit::Fn8ED3F0);
STATICINIT4_REVERSED("StaticInit::8ED410", 0x008ED410, &staticinit::Fn8ED410);
STATICINIT4_REVERSED("StaticInit::8ED430", 0x008ED430, &staticinit::Fn8ED430);
STATICINIT4_REVERSED("StaticInit::8ED450", 0x008ED450, &staticinit::Fn8ED450);
STATICINIT4_REVERSED("StaticInit::8ED470", 0x008ED470, &staticinit::Fn8ED470);
STATICINIT4_REVERSED("StaticInit::8ED490", 0x008ED490, &staticinit::Fn8ED490);
STATICINIT4_REVERSED("StaticInit::8ED4B0", 0x008ED4B0, &staticinit::Fn8ED4B0);
STATICINIT4_REVERSED("StaticInit::8ED4D0", 0x008ED4D0, &staticinit::Fn8ED4D0);
STATICINIT4_REVERSED("StaticInit::8ED4F0", 0x008ED4F0, &staticinit::Fn8ED4F0);
STATICINIT4_REVERSED("StaticInit::8ED510", 0x008ED510, &staticinit::Fn8ED510);
STATICINIT4_REVERSED("StaticInit::8ED530", 0x008ED530, &staticinit::Fn8ED530);
STATICINIT4_REVERSED("StaticInit::8ED550", 0x008ED550, &staticinit::Fn8ED550);
STATICINIT4_REVERSED("StaticInit::8ED570", 0x008ED570, &staticinit::Fn8ED570);
STATICINIT4_REVERSED("StaticInit::8ED590", 0x008ED590, &staticinit::Fn8ED590);
STATICINIT4_REVERSED("StaticInit::8ED5B0", 0x008ED5B0, &staticinit::Fn8ED5B0);
STATICINIT4_REVERSED("StaticInit::8ED5D0", 0x008ED5D0, &staticinit::Fn8ED5D0);
STATICINIT4_REVERSED("StaticInit::8ED5F0", 0x008ED5F0, &staticinit::Fn8ED5F0);
STATICINIT4_REVERSED("StaticInit::8ED610", 0x008ED610, &staticinit::Fn8ED610);
STATICINIT4_REVERSED("StaticInit::8ED630", 0x008ED630, &staticinit::Fn8ED630);
STATICINIT4_REVERSED("StaticInit::8ED650", 0x008ED650, &staticinit::Fn8ED650);
STATICINIT4_REVERSED("StaticInit::8ED670", 0x008ED670, &staticinit::Fn8ED670);
STATICINIT4_REVERSED("StaticInit::8ED690", 0x008ED690, &staticinit::Fn8ED690);
STATICINIT4_REVERSED("StaticInit::8ED6B0", 0x008ED6B0, &staticinit::Fn8ED6B0);
STATICINIT4_REVERSED("StaticInit::8ED6D0", 0x008ED6D0, &staticinit::Fn8ED6D0);
STATICINIT4_REVERSED("StaticInit::8ED6F0", 0x008ED6F0, &staticinit::Fn8ED6F0);
STATICINIT4_REVERSED("StaticInit::8ED710", 0x008ED710, &staticinit::Fn8ED710);
STATICINIT4_REVERSED("StaticInit::8ED730", 0x008ED730, &staticinit::Fn8ED730);
STATICINIT4_REVERSED("StaticInit::8ED750", 0x008ED750, &staticinit::Fn8ED750);
STATICINIT4_REVERSED("StaticInit::8ED770", 0x008ED770, &staticinit::Fn8ED770);
STATICINIT4_REVERSED("StaticInit::8ED790", 0x008ED790, &staticinit::Fn8ED790);
STATICINIT4_REVERSED("StaticInit::8ED7B0", 0x008ED7B0, &staticinit::Fn8ED7B0);
STATICINIT4_REVERSED("StaticInit::8ED7D0", 0x008ED7D0, &staticinit::Fn8ED7D0);
STATICINIT4_REVERSED("StaticInit::8ED7F0", 0x008ED7F0, &staticinit::Fn8ED7F0);
STATICINIT4_REVERSED("StaticInit::8ED810", 0x008ED810, &staticinit::Fn8ED810);
STATICINIT4_REVERSED("StaticInit::8ED830", 0x008ED830, &staticinit::Fn8ED830);
STATICINIT4_REVERSED("StaticInit::8ED850", 0x008ED850, &staticinit::Fn8ED850);
STATICINIT4_REVERSED("StaticInit::8ED870", 0x008ED870, &staticinit::Fn8ED870);
STATICINIT4_REVERSED("StaticInit::8ED890", 0x008ED890, &staticinit::Fn8ED890);
STATICINIT4_REVERSED("StaticInit::8ED8B0", 0x008ED8B0, &staticinit::Fn8ED8B0);
STATICINIT4_REVERSED("StaticInit::8ED8D0", 0x008ED8D0, &staticinit::Fn8ED8D0);
STATICINIT4_REVERSED("StaticInit::8ED920", 0x008ED920, &staticinit::Fn8ED920);
STATICINIT4_REVERSED("StaticInit::8ED970", 0x008ED970, &staticinit::Fn8ED970);
STATICINIT4_REVERSED("StaticInit::8ED990", 0x008ED990, &staticinit::Fn8ED990);
STATICINIT4_REVERSED("StaticInit::8ED9B0", 0x008ED9B0, &staticinit::Fn8ED9B0);
STATICINIT4_REVERSED("StaticInit::8ED9D0", 0x008ED9D0, &staticinit::Fn8ED9D0);
STATICINIT4_REVERSED("StaticInit::8ED9F0", 0x008ED9F0, &staticinit::Fn8ED9F0);
STATICINIT4_REVERSED("StaticInit::8EDA10", 0x008EDA10, &staticinit::Fn8EDA10);
STATICINIT4_REVERSED("StaticInit::8EDA30", 0x008EDA30, &staticinit::Fn8EDA30);
STATICINIT4_REVERSED("StaticInit::8EDA40", 0x008EDA40, &staticinit::Fn8EDA40);
STATICINIT4_REVERSED("StaticInit::8EDA60", 0x008EDA60, &staticinit::Fn8EDA60);
STATICINIT4_REVERSED("StaticInit::8EDA80", 0x008EDA80, &staticinit::Fn8EDA80);
STATICINIT4_REVERSED("StaticInit::8EDAA0", 0x008EDAA0, &staticinit::Fn8EDAA0);
STATICINIT4_REVERSED("StaticInit::8EDAC0", 0x008EDAC0, &staticinit::Fn8EDAC0);
STATICINIT4_REVERSED("StaticInit::8EDAE0", 0x008EDAE0, &staticinit::Fn8EDAE0);
STATICINIT4_REVERSED("StaticInit::8EDB00", 0x008EDB00, &staticinit::Fn8EDB00);
STATICINIT4_REVERSED("StaticInit::8EDB20", 0x008EDB20, &staticinit::Fn8EDB20);
STATICINIT4_REVERSED("StaticInit::8EDB40", 0x008EDB40, &staticinit::Fn8EDB40);
STATICINIT4_REVERSED("StaticInit::8EDB60", 0x008EDB60, &staticinit::Fn8EDB60);
STATICINIT4_REVERSED("StaticInit::8EDB80", 0x008EDB80, &staticinit::Fn8EDB80);
STATICINIT4_REVERSED("StaticInit::8EDBA0", 0x008EDBA0, &staticinit::Fn8EDBA0);
STATICINIT4_REVERSED("StaticInit::8EDBC0", 0x008EDBC0, &staticinit::Fn8EDBC0);
STATICINIT4_REVERSED("StaticInit::8EDBE0", 0x008EDBE0, &staticinit::Fn8EDBE0);
STATICINIT4_REVERSED("StaticInit::8EDC00", 0x008EDC00, &staticinit::Fn8EDC00);
STATICINIT4_REVERSED("StaticInit::8EDC20", 0x008EDC20, &staticinit::Fn8EDC20);
STATICINIT4_REVERSED("StaticInit::8EDC40", 0x008EDC40, &staticinit::Fn8EDC40);
STATICINIT4_REVERSED("StaticInit::8EDC60", 0x008EDC60, &staticinit::Fn8EDC60);
STATICINIT4_REVERSED("StaticInit::8EDC80", 0x008EDC80, &staticinit::Fn8EDC80);
STATICINIT4_REVERSED("StaticInit::8EDCA0", 0x008EDCA0, &staticinit::Fn8EDCA0);
STATICINIT4_REVERSED("StaticInit::8EDCC0", 0x008EDCC0, &staticinit::Fn8EDCC0);
STATICINIT4_REVERSED("StaticInit::8EDCE0", 0x008EDCE0, &staticinit::Fn8EDCE0);
STATICINIT4_REVERSED("StaticInit::8EDD00", 0x008EDD00, &staticinit::Fn8EDD00);
STATICINIT4_REVERSED("StaticInit::8EDD20", 0x008EDD20, &staticinit::Fn8EDD20);
STATICINIT4_REVERSED("StaticInit::8EDD40", 0x008EDD40, &staticinit::Fn8EDD40);
STATICINIT4_REVERSED("StaticInit::8EDD60", 0x008EDD60, &staticinit::Fn8EDD60);
STATICINIT4_REVERSED("StaticInit::8EDD80", 0x008EDD80, &staticinit::Fn8EDD80);
STATICINIT4_REVERSED("StaticInit::8EDDA0", 0x008EDDA0, &staticinit::Fn8EDDA0);
STATICINIT4_REVERSED("StaticInit::8EDDC0", 0x008EDDC0, &staticinit::Fn8EDDC0);
STATICINIT4_REVERSED("StaticInit::8EDDE0", 0x008EDDE0, &staticinit::Fn8EDDE0);
STATICINIT4_REVERSED("StaticInit::8EDE00", 0x008EDE00, &staticinit::Fn8EDE00);
STATICINIT4_REVERSED("StaticInit::8EDE20", 0x008EDE20, &staticinit::Fn8EDE20);
STATICINIT4_REVERSED("StaticInit::8EDE40", 0x008EDE40, &staticinit::Fn8EDE40);
STATICINIT4_REVERSED("StaticInit::8EDE60", 0x008EDE60, &staticinit::Fn8EDE60);
STATICINIT4_REVERSED("StaticInit::8EDE80", 0x008EDE80, &staticinit::Fn8EDE80);
STATICINIT4_REVERSED("StaticInit::8EDEA0", 0x008EDEA0, &staticinit::Fn8EDEA0);
STATICINIT4_REVERSED("StaticInit::8EDEC0", 0x008EDEC0, &staticinit::Fn8EDEC0);
STATICINIT4_REVERSED("StaticInit::8EDEE0", 0x008EDEE0, &staticinit::Fn8EDEE0);
STATICINIT4_REVERSED("StaticInit::8EDF00", 0x008EDF00, &staticinit::Fn8EDF00);
STATICINIT4_REVERSED("StaticInit::8EDF20", 0x008EDF20, &staticinit::Fn8EDF20);
STATICINIT4_REVERSED("StaticInit::8EDF40", 0x008EDF40, &staticinit::Fn8EDF40);
STATICINIT4_REVERSED("StaticInit::8EDF60", 0x008EDF60, &staticinit::Fn8EDF60);
STATICINIT4_REVERSED("StaticInit::8EDF80", 0x008EDF80, &staticinit::Fn8EDF80);
STATICINIT4_REVERSED("StaticInit::8EDFA0", 0x008EDFA0, &staticinit::Fn8EDFA0);
STATICINIT4_REVERSED("StaticInit::8EDFC0", 0x008EDFC0, &staticinit::Fn8EDFC0);
STATICINIT4_REVERSED("StaticInit::8EDFE0", 0x008EDFE0, &staticinit::Fn8EDFE0);
STATICINIT4_REVERSED("StaticInit::8EE000", 0x008EE000, &staticinit::Fn8EE000);
STATICINIT4_REVERSED("StaticInit::8EE020", 0x008EE020, &staticinit::Fn8EE020);
STATICINIT4_REVERSED("StaticInit::8EE040", 0x008EE040, &staticinit::Fn8EE040);
STATICINIT4_REVERSED("StaticInit::8EE060", 0x008EE060, &staticinit::Fn8EE060);
STATICINIT4_REVERSED("StaticInit::8EE080", 0x008EE080, &staticinit::Fn8EE080);
STATICINIT4_REVERSED("StaticInit::8EE0A0", 0x008EE0A0, &staticinit::Fn8EE0A0);
STATICINIT4_REVERSED("StaticInit::8EE0C0", 0x008EE0C0, &staticinit::Fn8EE0C0);
STATICINIT4_REVERSED("StaticInit::8EE0E0", 0x008EE0E0, &staticinit::Fn8EE0E0);
STATICINIT4_REVERSED("StaticInit::8EE100", 0x008EE100, &staticinit::Fn8EE100);
STATICINIT4_REVERSED("StaticInit::8EE120", 0x008EE120, &staticinit::Fn8EE120);
STATICINIT4_REVERSED("StaticInit::8EE140", 0x008EE140, &staticinit::Fn8EE140);
STATICINIT4_REVERSED("StaticInit::8EE160", 0x008EE160, &staticinit::Fn8EE160);
STATICINIT4_REVERSED("StaticInit::8EE180", 0x008EE180, &staticinit::Fn8EE180);
STATICINIT4_REVERSED("StaticInit::8EE1A0", 0x008EE1A0, &staticinit::Fn8EE1A0);
STATICINIT4_REVERSED("StaticInit::8EE1C0", 0x008EE1C0, &staticinit::Fn8EE1C0);
STATICINIT4_REVERSED("StaticInit::8EE1E0", 0x008EE1E0, &staticinit::Fn8EE1E0);
STATICINIT4_REVERSED("StaticInit::8EE200", 0x008EE200, &staticinit::Fn8EE200);
STATICINIT4_REVERSED("StaticInit::8EE220", 0x008EE220, &staticinit::Fn8EE220);
STATICINIT4_REVERSED("StaticInit::8EE240", 0x008EE240, &staticinit::Fn8EE240);
STATICINIT4_REVERSED("StaticInit::8EE260", 0x008EE260, &staticinit::Fn8EE260);
STATICINIT4_REVERSED("StaticInit::8EE280", 0x008EE280, &staticinit::Fn8EE280);
STATICINIT4_REVERSED("StaticInit::8EE2A0", 0x008EE2A0, &staticinit::Fn8EE2A0);
STATICINIT4_REVERSED("StaticInit::8EE2C0", 0x008EE2C0, &staticinit::Fn8EE2C0);
STATICINIT4_REVERSED("StaticInit::8EE2E0", 0x008EE2E0, &staticinit::Fn8EE2E0);
STATICINIT4_REVERSED("StaticInit::8EE300", 0x008EE300, &staticinit::Fn8EE300);
STATICINIT4_REVERSED("StaticInit::8EE320", 0x008EE320, &staticinit::Fn8EE320);
STATICINIT4_REVERSED("StaticInit::8EE340", 0x008EE340, &staticinit::Fn8EE340);
STATICINIT4_REVERSED("StaticInit::8EE360", 0x008EE360, &staticinit::Fn8EE360);
STATICINIT4_REVERSED("StaticInit::8EE380", 0x008EE380, &staticinit::Fn8EE380);
STATICINIT4_REVERSED("StaticInit::8EE3A0", 0x008EE3A0, &staticinit::Fn8EE3A0);
STATICINIT4_REVERSED("StaticInit::8EE3C0", 0x008EE3C0, &staticinit::Fn8EE3C0);
STATICINIT4_REVERSED("StaticInit::8EE3E0", 0x008EE3E0, &staticinit::Fn8EE3E0);
STATICINIT4_REVERSED("StaticInit::8EE400", 0x008EE400, &staticinit::Fn8EE400);
STATICINIT4_REVERSED("StaticInit::8EE420", 0x008EE420, &staticinit::Fn8EE420);
STATICINIT4_REVERSED("StaticInit::8EE440", 0x008EE440, &staticinit::Fn8EE440);
STATICINIT4_REVERSED("StaticInit::8EE460", 0x008EE460, &staticinit::Fn8EE460);
STATICINIT4_REVERSED("StaticInit::8EE480", 0x008EE480, &staticinit::Fn8EE480);
STATICINIT4_REVERSED("StaticInit::8EE4A0", 0x008EE4A0, &staticinit::Fn8EE4A0);
STATICINIT4_REVERSED("StaticInit::8EE4C0", 0x008EE4C0, &staticinit::Fn8EE4C0);
STATICINIT4_REVERSED("StaticInit::8EE4E0", 0x008EE4E0, &staticinit::Fn8EE4E0);
STATICINIT4_REVERSED("StaticInit::8EE500", 0x008EE500, &staticinit::Fn8EE500);
STATICINIT4_REVERSED("StaticInit::8EE520", 0x008EE520, &staticinit::Fn8EE520);
STATICINIT4_REVERSED("StaticInit::8EE540", 0x008EE540, &staticinit::Fn8EE540);
STATICINIT4_REVERSED("StaticInit::8EE560", 0x008EE560, &staticinit::Fn8EE560);
STATICINIT4_REVERSED("StaticInit::8EE580", 0x008EE580, &staticinit::Fn8EE580);
STATICINIT4_REVERSED("StaticInit::8EE5A0", 0x008EE5A0, &staticinit::Fn8EE5A0);
STATICINIT4_REVERSED("StaticInit::8EE5C0", 0x008EE5C0, &staticinit::Fn8EE5C0);
STATICINIT4_REVERSED("StaticInit::8EE5E0", 0x008EE5E0, &staticinit::Fn8EE5E0);
STATICINIT4_REVERSED("StaticInit::8EE600", 0x008EE600, &staticinit::Fn8EE600);
STATICINIT4_REVERSED("StaticInit::8EE620", 0x008EE620, &staticinit::Fn8EE620);
STATICINIT4_REVERSED("StaticInit::8EE640", 0x008EE640, &staticinit::Fn8EE640);
STATICINIT4_REVERSED("StaticInit::8EE660", 0x008EE660, &staticinit::Fn8EE660);
STATICINIT4_REVERSED("StaticInit::8EE680", 0x008EE680, &staticinit::Fn8EE680);
STATICINIT4_REVERSED("StaticInit::8EE6A0", 0x008EE6A0, &staticinit::Fn8EE6A0);
STATICINIT4_REVERSED("StaticInit::8EE6C0", 0x008EE6C0, &staticinit::Fn8EE6C0);
STATICINIT4_REVERSED("StaticInit::8EE6E0", 0x008EE6E0, &staticinit::Fn8EE6E0);
STATICINIT4_REVERSED("StaticInit::8EE700", 0x008EE700, &staticinit::Fn8EE700);
STATICINIT4_REVERSED("StaticInit::8EE720", 0x008EE720, &staticinit::Fn8EE720);
STATICINIT4_REVERSED("StaticInit::8EE740", 0x008EE740, &staticinit::Fn8EE740);
STATICINIT4_REVERSED("StaticInit::8EE760", 0x008EE760, &staticinit::Fn8EE760);
STATICINIT4_REVERSED("StaticInit::8EE780", 0x008EE780, &staticinit::Fn8EE780);
STATICINIT4_REVERSED("StaticInit::8EE7A0", 0x008EE7A0, &staticinit::Fn8EE7A0);
STATICINIT4_REVERSED("StaticInit::8EE7C0", 0x008EE7C0, &staticinit::Fn8EE7C0);
STATICINIT4_REVERSED("StaticInit::8EE7E0", 0x008EE7E0, &staticinit::Fn8EE7E0);
STATICINIT4_REVERSED("StaticInit::8EE800", 0x008EE800, &staticinit::Fn8EE800);
STATICINIT4_REVERSED("StaticInit::8EE820", 0x008EE820, &staticinit::Fn8EE820);
STATICINIT4_REVERSED("StaticInit::8EE840", 0x008EE840, &staticinit::Fn8EE840);
STATICINIT4_REVERSED("StaticInit::8EE860", 0x008EE860, &staticinit::Fn8EE860);
STATICINIT4_REVERSED("StaticInit::8EE880", 0x008EE880, &staticinit::Fn8EE880);
STATICINIT4_REVERSED("StaticInit::8EE8A0", 0x008EE8A0, &staticinit::Fn8EE8A0);
STATICINIT4_REVERSED("StaticInit::8EE8C0", 0x008EE8C0, &staticinit::Fn8EE8C0);
STATICINIT4_REVERSED("StaticInit::8EE8E0", 0x008EE8E0, &staticinit::Fn8EE8E0);
STATICINIT4_REVERSED("StaticInit::8EE900", 0x008EE900, &staticinit::Fn8EE900);
STATICINIT4_REVERSED("StaticInit::8EE920", 0x008EE920, &staticinit::Fn8EE920);
STATICINIT4_REVERSED("StaticInit::8EE940", 0x008EE940, &staticinit::Fn8EE940);
STATICINIT4_REVERSED("StaticInit::8EE960", 0x008EE960, &staticinit::Fn8EE960);
STATICINIT4_REVERSED("StaticInit::8EE980", 0x008EE980, &staticinit::Fn8EE980);
STATICINIT4_REVERSED("StaticInit::8EE9A0", 0x008EE9A0, &staticinit::Fn8EE9A0);
STATICINIT4_REVERSED("StaticInit::8EE9C0", 0x008EE9C0, &staticinit::Fn8EE9C0);
STATICINIT4_REVERSED("StaticInit::8EE9E0", 0x008EE9E0, &staticinit::Fn8EE9E0);
STATICINIT4_REVERSED("StaticInit::8EEA00", 0x008EEA00, &staticinit::Fn8EEA00);
STATICINIT4_REVERSED("StaticInit::8EEA20", 0x008EEA20, &staticinit::Fn8EEA20);
STATICINIT4_REVERSED("StaticInit::8EEA40", 0x008EEA40, &staticinit::Fn8EEA40);
STATICINIT4_REVERSED("StaticInit::8EEA60", 0x008EEA60, &staticinit::Fn8EEA60);
STATICINIT4_REVERSED("StaticInit::8EEA80", 0x008EEA80, &staticinit::Fn8EEA80);
STATICINIT4_REVERSED("StaticInit::8EEAA0", 0x008EEAA0, &staticinit::Fn8EEAA0);
STATICINIT4_REVERSED("StaticInit::8EEAC0", 0x008EEAC0, &staticinit::Fn8EEAC0);
STATICINIT4_REVERSED("StaticInit::8EEAE0", 0x008EEAE0, &staticinit::Fn8EEAE0);
STATICINIT4_REVERSED("StaticInit::8EEB00", 0x008EEB00, &staticinit::Fn8EEB00);
STATICINIT4_REVERSED("StaticInit::8EEB20", 0x008EEB20, &staticinit::Fn8EEB20);
STATICINIT4_REVERSED("StaticInit::8EEB40", 0x008EEB40, &staticinit::Fn8EEB40);
STATICINIT4_REVERSED("StaticInit::8EEB60", 0x008EEB60, &staticinit::Fn8EEB60);
STATICINIT4_REVERSED("StaticInit::8EEB80", 0x008EEB80, &staticinit::Fn8EEB80);
STATICINIT4_REVERSED("StaticInit::8EEBA0", 0x008EEBA0, &staticinit::Fn8EEBA0);
STATICINIT4_REVERSED("StaticInit::8EEBC0", 0x008EEBC0, &staticinit::Fn8EEBC0);
STATICINIT4_REVERSED("StaticInit::8EEBE0", 0x008EEBE0, &staticinit::Fn8EEBE0);
STATICINIT4_REVERSED("StaticInit::8EEC00", 0x008EEC00, &staticinit::Fn8EEC00);
STATICINIT4_REVERSED("StaticInit::8EEC20", 0x008EEC20, &staticinit::Fn8EEC20);
STATICINIT4_REVERSED("StaticInit::8EEC40", 0x008EEC40, &staticinit::Fn8EEC40);
STATICINIT4_REVERSED("StaticInit::8EEC60", 0x008EEC60, &staticinit::Fn8EEC60);
STATICINIT4_REVERSED("StaticInit::8EEC80", 0x008EEC80, &staticinit::Fn8EEC80);
STATICINIT4_REVERSED("StaticInit::8EECA0", 0x008EECA0, &staticinit::Fn8EECA0);
STATICINIT4_REVERSED("StaticInit::8EECC0", 0x008EECC0, &staticinit::Fn8EECC0);
STATICINIT4_REVERSED("StaticInit::8EECE0", 0x008EECE0, &staticinit::Fn8EECE0);
STATICINIT4_REVERSED("StaticInit::8EED00", 0x008EED00, &staticinit::Fn8EED00);
STATICINIT4_REVERSED("StaticInit::8EED20", 0x008EED20, &staticinit::Fn8EED20);
STATICINIT4_REVERSED("StaticInit::8EED40", 0x008EED40, &staticinit::Fn8EED40);
STATICINIT4_REVERSED("StaticInit::8EED60", 0x008EED60, &staticinit::Fn8EED60);
STATICINIT4_REVERSED("StaticInit::8EED80", 0x008EED80, &staticinit::Fn8EED80);
STATICINIT4_REVERSED("StaticInit::8EEDA0", 0x008EEDA0, &staticinit::Fn8EEDA0);
STATICINIT4_REVERSED("StaticInit::8EEDC0", 0x008EEDC0, &staticinit::Fn8EEDC0);
STATICINIT4_REVERSED("StaticInit::8EEDE0", 0x008EEDE0, &staticinit::Fn8EEDE0);
STATICINIT4_REVERSED("StaticInit::8EEE00", 0x008EEE00, &staticinit::Fn8EEE00);
STATICINIT4_REVERSED("StaticInit::8EEE20", 0x008EEE20, &staticinit::Fn8EEE20);
STATICINIT4_REVERSED("StaticInit::8EEE40", 0x008EEE40, &staticinit::Fn8EEE40);
STATICINIT4_REVERSED("StaticInit::8EEE60", 0x008EEE60, &staticinit::Fn8EEE60);
STATICINIT4_REVERSED("StaticInit::8EEE80", 0x008EEE80, &staticinit::Fn8EEE80);
STATICINIT4_REVERSED("StaticInit::8EEEA0", 0x008EEEA0, &staticinit::Fn8EEEA0);
STATICINIT4_REVERSED("StaticInit::8EEEC0", 0x008EEEC0, &staticinit::Fn8EEEC0);
STATICINIT4_REVERSED("StaticInit::8EEEE0", 0x008EEEE0, &staticinit::Fn8EEEE0);
STATICINIT4_REVERSED("StaticInit::8EEF00", 0x008EEF00, &staticinit::Fn8EEF00);
STATICINIT4_REVERSED("StaticInit::8EEF20", 0x008EEF20, &staticinit::Fn8EEF20);
STATICINIT4_REVERSED("StaticInit::8EEF40", 0x008EEF40, &staticinit::Fn8EEF40);
STATICINIT4_REVERSED("StaticInit::8EEF60", 0x008EEF60, &staticinit::Fn8EEF60);
STATICINIT4_REVERSED("StaticInit::8EEF80", 0x008EEF80, &staticinit::Fn8EEF80);
STATICINIT4_REVERSED("StaticInit::8EEFA0", 0x008EEFA0, &staticinit::Fn8EEFA0);
STATICINIT4_REVERSED("StaticInit::8EEFC0", 0x008EEFC0, &staticinit::Fn8EEFC0);
STATICINIT4_REVERSED("StaticInit::8EEFE0", 0x008EEFE0, &staticinit::Fn8EEFE0);
STATICINIT4_REVERSED("StaticInit::8EF000", 0x008EF000, &staticinit::Fn8EF000);
STATICINIT4_REVERSED("StaticInit::8EF020", 0x008EF020, &staticinit::Fn8EF020);
STATICINIT4_REVERSED("StaticInit::8EF040", 0x008EF040, &staticinit::Fn8EF040);
STATICINIT4_REVERSED("StaticInit::8EF060", 0x008EF060, &staticinit::Fn8EF060);
STATICINIT4_REVERSED("StaticInit::8EF080", 0x008EF080, &staticinit::Fn8EF080);
STATICINIT4_REVERSED("StaticInit::8EF0A0", 0x008EF0A0, &staticinit::Fn8EF0A0);
STATICINIT4_REVERSED("StaticInit::8EF0C0", 0x008EF0C0, &staticinit::Fn8EF0C0);
STATICINIT4_REVERSED("StaticInit::8EF0E0", 0x008EF0E0, &staticinit::Fn8EF0E0);
STATICINIT4_REVERSED("StaticInit::8EF100", 0x008EF100, &staticinit::Fn8EF100);
STATICINIT4_REVERSED("StaticInit::8EF120", 0x008EF120, &staticinit::Fn8EF120);
STATICINIT4_REVERSED("StaticInit::8EF140", 0x008EF140, &staticinit::Fn8EF140);
STATICINIT4_REVERSED("StaticInit::8EF160", 0x008EF160, &staticinit::Fn8EF160);
STATICINIT4_REVERSED("StaticInit::8EF180", 0x008EF180, &staticinit::Fn8EF180);
STATICINIT4_REVERSED("StaticInit::8EF1A0", 0x008EF1A0, &staticinit::Fn8EF1A0);
STATICINIT4_REVERSED("StaticInit::8EF1C0", 0x008EF1C0, &staticinit::Fn8EF1C0);
STATICINIT4_REVERSED("StaticInit::8EF1E0", 0x008EF1E0, &staticinit::Fn8EF1E0);
STATICINIT4_REVERSED("StaticInit::8EF1F0", 0x008EF1F0, &staticinit::Fn8EF1F0);
STATICINIT4_REVERSED("StaticInit::8EF200", 0x008EF200, &staticinit::Fn8EF200);
STATICINIT4_REVERSED("StaticInit::8EF220", 0x008EF220, &staticinit::Fn8EF220);
STATICINIT4_REVERSED("StaticInit::8EF240", 0x008EF240, &staticinit::Fn8EF240);
STATICINIT4_REVERSED("StaticInit::8EF260", 0x008EF260, &staticinit::Fn8EF260);
STATICINIT4_REVERSED("StaticInit::8EF280", 0x008EF280, &staticinit::Fn8EF280);
STATICINIT4_REVERSED("StaticInit::8EF2A0", 0x008EF2A0, &staticinit::Fn8EF2A0);
STATICINIT4_REVERSED("StaticInit::8EF2E0", 0x008EF2E0, &staticinit::Fn8EF2E0);
STATICINIT4_REVERSED("StaticInit::8EF300", 0x008EF300, &staticinit::Fn8EF300);
STATICINIT4_REVERSED("StaticInit::8EF320", 0x008EF320, &staticinit::Fn8EF320);
STATICINIT4_REVERSED("StaticInit::8EF340", 0x008EF340, &staticinit::Fn8EF340);
STATICINIT4_REVERSED("StaticInit::8EF360", 0x008EF360, &staticinit::Fn8EF360);
STATICINIT4_REVERSED("StaticInit::8EF380", 0x008EF380, &staticinit::Fn8EF380);
STATICINIT4_REVERSED("StaticInit::8EF3A0", 0x008EF3A0, &staticinit::Fn8EF3A0);
STATICINIT4_REVERSED("StaticInit::8EF3C0", 0x008EF3C0, &staticinit::Fn8EF3C0);
STATICINIT4_REVERSED("StaticInit::8EF3E0", 0x008EF3E0, &staticinit::Fn8EF3E0);
STATICINIT4_REVERSED("StaticInit::8EF400", 0x008EF400, &staticinit::Fn8EF400);
STATICINIT4_REVERSED("StaticInit::8EF410", 0x008EF410, &staticinit::Fn8EF410);
STATICINIT4_REVERSED("StaticInit::8EF430", 0x008EF430, &staticinit::Fn8EF430);
STATICINIT4_REVERSED("StaticInit::8EF450", 0x008EF450, &staticinit::Fn8EF450);
STATICINIT4_REVERSED("StaticInit::8EF470", 0x008EF470, &staticinit::Fn8EF470);
STATICINIT4_REVERSED("StaticInit::8EF490", 0x008EF490, &staticinit::Fn8EF490);
STATICINIT4_REVERSED("StaticInit::8EF4B0", 0x008EF4B0, &staticinit::Fn8EF4B0);
STATICINIT4_REVERSED("StaticInit::8EF4D0", 0x008EF4D0, &staticinit::Fn8EF4D0);
STATICINIT4_REVERSED("StaticInit::8EF4F0", 0x008EF4F0, &staticinit::Fn8EF4F0);
STATICINIT4_REVERSED("StaticInit::8EF510", 0x008EF510, &staticinit::Fn8EF510);
STATICINIT4_REVERSED("StaticInit::8EF530", 0x008EF530, &staticinit::Fn8EF530);
STATICINIT4_REVERSED("StaticInit::8EF550", 0x008EF550, &staticinit::Fn8EF550);
STATICINIT4_REVERSED("StaticInit::8EF570", 0x008EF570, &staticinit::Fn8EF570);
STATICINIT4_REVERSED("StaticInit::8EF590", 0x008EF590, &staticinit::Fn8EF590);
STATICINIT4_REVERSED("StaticInit::8EF5B0", 0x008EF5B0, &staticinit::Fn8EF5B0);
STATICINIT4_REVERSED("StaticInit::8EF5D0", 0x008EF5D0, &staticinit::Fn8EF5D0);
STATICINIT4_REVERSED("StaticInit::8EF610", 0x008EF610, &staticinit::Fn8EF610);
STATICINIT4_REVERSED("StaticInit::8EF650", 0x008EF650, &staticinit::Fn8EF650);
STATICINIT4_REVERSED("StaticInit::8EF690", 0x008EF690, &staticinit::Fn8EF690);
STATICINIT4_REVERSED("StaticInit::8EF6D0", 0x008EF6D0, &staticinit::Fn8EF6D0);
STATICINIT4_REVERSED("StaticInit::8EF710", 0x008EF710, &staticinit::Fn8EF710);
STATICINIT4_REVERSED("StaticInit::8EF750", 0x008EF750, &staticinit::Fn8EF750);
STATICINIT4_REVERSED("StaticInit::8EF790", 0x008EF790, &staticinit::Fn8EF790);
STATICINIT4_REVERSED("StaticInit::8EF7D0", 0x008EF7D0, &staticinit::Fn8EF7D0);
STATICINIT4_REVERSED("StaticInit::8EF810", 0x008EF810, &staticinit::Fn8EF810);
STATICINIT4_REVERSED("StaticInit::8EF850", 0x008EF850, &staticinit::Fn8EF850);
STATICINIT4_REVERSED("StaticInit::8EF890", 0x008EF890, &staticinit::Fn8EF890);
STATICINIT4_REVERSED("StaticInit::8EF8D0", 0x008EF8D0, &staticinit::Fn8EF8D0);
STATICINIT4_REVERSED("StaticInit::8EF910", 0x008EF910, &staticinit::Fn8EF910);
STATICINIT4_REVERSED("StaticInit::8EF950", 0x008EF950, &staticinit::Fn8EF950);
STATICINIT4_REVERSED("StaticInit::8EF990", 0x008EF990, &staticinit::Fn8EF990);
STATICINIT4_REVERSED("StaticInit::8EF9D0", 0x008EF9D0, &staticinit::Fn8EF9D0);
STATICINIT4_REVERSED("StaticInit::8EFA10", 0x008EFA10, &staticinit::Fn8EFA10);
STATICINIT4_REVERSED("StaticInit::8EFA50", 0x008EFA50, &staticinit::Fn8EFA50);
STATICINIT4_REVERSED("StaticInit::8EFA90", 0x008EFA90, &staticinit::Fn8EFA90);
STATICINIT4_REVERSED("StaticInit::8EFAD0", 0x008EFAD0, &staticinit::Fn8EFAD0);
STATICINIT4_REVERSED("StaticInit::8EFB10", 0x008EFB10, &staticinit::Fn8EFB10);
STATICINIT4_REVERSED("StaticInit::8EFB50", 0x008EFB50, &staticinit::Fn8EFB50);
STATICINIT4_REVERSED("StaticInit::8EFB90", 0x008EFB90, &staticinit::Fn8EFB90);
STATICINIT4_REVERSED("StaticInit::8EFBD0", 0x008EFBD0, &staticinit::Fn8EFBD0);
STATICINIT4_REVERSED("StaticInit::8EFC10", 0x008EFC10, &staticinit::Fn8EFC10);
STATICINIT4_REVERSED("StaticInit::8EFC50", 0x008EFC50, &staticinit::Fn8EFC50);
STATICINIT4_REVERSED("StaticInit::8EFC90", 0x008EFC90, &staticinit::Fn8EFC90);
STATICINIT4_REVERSED("StaticInit::8EFCD0", 0x008EFCD0, &staticinit::Fn8EFCD0);
STATICINIT4_REVERSED("StaticInit::8EFD10", 0x008EFD10, &staticinit::Fn8EFD10);
STATICINIT4_REVERSED("StaticInit::8EFD50", 0x008EFD50, &staticinit::Fn8EFD50);
STATICINIT4_REVERSED("StaticInit::8EFD90", 0x008EFD90, &staticinit::Fn8EFD90);
STATICINIT4_REVERSED("StaticInit::8EFDD0", 0x008EFDD0, &staticinit::Fn8EFDD0);
STATICINIT4_REVERSED("StaticInit::8EFE10", 0x008EFE10, &staticinit::Fn8EFE10);
STATICINIT4_REVERSED("StaticInit::8EFE50", 0x008EFE50, &staticinit::Fn8EFE50);
STATICINIT4_REVERSED("StaticInit::8EFE90", 0x008EFE90, &staticinit::Fn8EFE90);
STATICINIT4_REVERSED("StaticInit::8EFED0", 0x008EFED0, &staticinit::Fn8EFED0);
STATICINIT4_REVERSED("StaticInit::8EFF10", 0x008EFF10, &staticinit::Fn8EFF10);
STATICINIT4_REVERSED("StaticInit::8EFF50", 0x008EFF50, &staticinit::Fn8EFF50);
STATICINIT4_REVERSED("StaticInit::8EFF90", 0x008EFF90, &staticinit::Fn8EFF90);
STATICINIT4_REVERSED("StaticInit::8EFFD0", 0x008EFFD0, &staticinit::Fn8EFFD0);
STATICINIT4_REVERSED("StaticInit::8F0010", 0x008F0010, &staticinit::Fn8F0010);
STATICINIT4_REVERSED("StaticInit::8F0050", 0x008F0050, &staticinit::Fn8F0050);
STATICINIT4_REVERSED("StaticInit::8F0090", 0x008F0090, &staticinit::Fn8F0090);
STATICINIT4_REVERSED("StaticInit::8F00D0", 0x008F00D0, &staticinit::Fn8F00D0);
STATICINIT4_REVERSED("StaticInit::8F0110", 0x008F0110, &staticinit::Fn8F0110);
STATICINIT4_REVERSED("StaticInit::8F0150", 0x008F0150, &staticinit::Fn8F0150);
STATICINIT4_REVERSED("StaticInit::8F0190", 0x008F0190, &staticinit::Fn8F0190);
STATICINIT4_REVERSED("StaticInit::8F01D0", 0x008F01D0, &staticinit::Fn8F01D0);
STATICINIT4_REVERSED("StaticInit::8F0210", 0x008F0210, &staticinit::Fn8F0210);
STATICINIT4_REVERSED("StaticInit::8F0250", 0x008F0250, &staticinit::Fn8F0250);
STATICINIT4_REVERSED("StaticInit::8F0290", 0x008F0290, &staticinit::Fn8F0290);
STATICINIT4_REVERSED("StaticInit::8F02D0", 0x008F02D0, &staticinit::Fn8F02D0);
STATICINIT4_REVERSED("StaticInit::8F0310", 0x008F0310, &staticinit::Fn8F0310);
STATICINIT4_REVERSED("StaticInit::8F0350", 0x008F0350, &staticinit::Fn8F0350);
STATICINIT4_REVERSED("StaticInit::8F0390", 0x008F0390, &staticinit::Fn8F0390);
STATICINIT4_REVERSED("StaticInit::8F03D0", 0x008F03D0, &staticinit::Fn8F03D0);
STATICINIT4_REVERSED("StaticInit::8F0410", 0x008F0410, &staticinit::Fn8F0410);
STATICINIT4_REVERSED("StaticInit::8F0450", 0x008F0450, &staticinit::Fn8F0450);
STATICINIT4_REVERSED("StaticInit::8F0490", 0x008F0490, &staticinit::Fn8F0490);
STATICINIT4_REVERSED("StaticInit::8F04D0", 0x008F04D0, &staticinit::Fn8F04D0);
STATICINIT4_REVERSED("StaticInit::8F0510", 0x008F0510, &staticinit::Fn8F0510);
STATICINIT4_REVERSED("StaticInit::8F0550", 0x008F0550, &staticinit::Fn8F0550);
STATICINIT4_REVERSED("StaticInit::8F0590", 0x008F0590, &staticinit::Fn8F0590);
STATICINIT4_REVERSED("StaticInit::8F05D0", 0x008F05D0, &staticinit::Fn8F05D0);
STATICINIT4_REVERSED("StaticInit::8F0610", 0x008F0610, &staticinit::Fn8F0610);
STATICINIT4_REVERSED("StaticInit::8F0650", 0x008F0650, &staticinit::Fn8F0650);
STATICINIT4_REVERSED("StaticInit::8F0690", 0x008F0690, &staticinit::Fn8F0690);
STATICINIT4_REVERSED("StaticInit::8F06D0", 0x008F06D0, &staticinit::Fn8F06D0);
STATICINIT4_REVERSED("StaticInit::8F0710", 0x008F0710, &staticinit::Fn8F0710);
STATICINIT4_REVERSED("StaticInit::8F0750", 0x008F0750, &staticinit::Fn8F0750);
STATICINIT4_REVERSED("StaticInit::8F0790", 0x008F0790, &staticinit::Fn8F0790);
STATICINIT4_REVERSED("StaticInit::8F07D0", 0x008F07D0, &staticinit::Fn8F07D0);
STATICINIT4_REVERSED("StaticInit::8F0810", 0x008F0810, &staticinit::Fn8F0810);
STATICINIT4_REVERSED("StaticInit::8F0850", 0x008F0850, &staticinit::Fn8F0850);
STATICINIT4_REVERSED("StaticInit::8F0890", 0x008F0890, &staticinit::Fn8F0890);
STATICINIT4_REVERSED("StaticInit::8F08D0", 0x008F08D0, &staticinit::Fn8F08D0);
STATICINIT4_REVERSED("StaticInit::8F0910", 0x008F0910, &staticinit::Fn8F0910);
STATICINIT4_REVERSED("StaticInit::8F0950", 0x008F0950, &staticinit::Fn8F0950);
STATICINIT4_REVERSED("StaticInit::8F0990", 0x008F0990, &staticinit::Fn8F0990);
STATICINIT4_REVERSED("StaticInit::8F09D0", 0x008F09D0, &staticinit::Fn8F09D0);
STATICINIT4_REVERSED("StaticInit::8F0A10", 0x008F0A10, &staticinit::Fn8F0A10);
STATICINIT4_REVERSED("StaticInit::8F0A50", 0x008F0A50, &staticinit::Fn8F0A50);
STATICINIT4_REVERSED("StaticInit::8F0A90", 0x008F0A90, &staticinit::Fn8F0A90);
STATICINIT4_REVERSED("StaticInit::8F0AD0", 0x008F0AD0, &staticinit::Fn8F0AD0);
STATICINIT4_REVERSED("StaticInit::8F0B10", 0x008F0B10, &staticinit::Fn8F0B10);
STATICINIT4_REVERSED("StaticInit::8F0B50", 0x008F0B50, &staticinit::Fn8F0B50);
STATICINIT4_REVERSED("StaticInit::8F0B90", 0x008F0B90, &staticinit::Fn8F0B90);
STATICINIT4_REVERSED("StaticInit::8F0BD0", 0x008F0BD0, &staticinit::Fn8F0BD0);
STATICINIT4_REVERSED("StaticInit::8F0BF0", 0x008F0BF0, &staticinit::Fn8F0BF0);
STATICINIT4_REVERSED("StaticInit::8F0C00", 0x008F0C00, &staticinit::Fn8F0C00);
STATICINIT4_REVERSED("StaticInit::8F0C20", 0x008F0C20, &staticinit::Fn8F0C20);
STATICINIT4_REVERSED("StaticInit::8F0C40", 0x008F0C40, &staticinit::Fn8F0C40);
STATICINIT4_REVERSED("StaticInit::8F0C60", 0x008F0C60, &staticinit::Fn8F0C60);
STATICINIT4_REVERSED("StaticInit::8F0C80", 0x008F0C80, &staticinit::Fn8F0C80);
STATICINIT4_REVERSED("StaticInit::8F0CA0", 0x008F0CA0, &staticinit::Fn8F0CA0);
STATICINIT4_REVERSED("StaticInit::8F0CC0", 0x008F0CC0, &staticinit::Fn8F0CC0);
STATICINIT4_REVERSED("StaticInit::8F0CE0", 0x008F0CE0, &staticinit::Fn8F0CE0);
STATICINIT4_REVERSED("StaticInit::8F0D00", 0x008F0D00, &staticinit::Fn8F0D00);
STATICINIT4_REVERSED("StaticInit::8F0D20", 0x008F0D20, &staticinit::Fn8F0D20);
STATICINIT4_REVERSED("StaticInit::8F0D40", 0x008F0D40, &staticinit::Fn8F0D40);
STATICINIT4_REVERSED("StaticInit::8F0D60", 0x008F0D60, &staticinit::Fn8F0D60);
STATICINIT4_REVERSED("StaticInit::8F0D80", 0x008F0D80, &staticinit::Fn8F0D80);
STATICINIT4_REVERSED("StaticInit::8F0DA0", 0x008F0DA0, &staticinit::Fn8F0DA0);
STATICINIT4_REVERSED("StaticInit::8F0DC0", 0x008F0DC0, &staticinit::Fn8F0DC0);
STATICINIT4_REVERSED("StaticInit::8F0DE0", 0x008F0DE0, &staticinit::Fn8F0DE0);
STATICINIT4_REVERSED("StaticInit::8F0E00", 0x008F0E00, &staticinit::Fn8F0E00);
STATICINIT4_REVERSED("StaticInit::8F0E20", 0x008F0E20, &staticinit::Fn8F0E20);
STATICINIT4_REVERSED("StaticInit::8F0E30", 0x008F0E30, &staticinit::Fn8F0E30);
STATICINIT4_REVERSED("StaticInit::8F0E50", 0x008F0E50, &staticinit::Fn8F0E50);
STATICINIT4_REVERSED("StaticInit::8F0E70", 0x008F0E70, &staticinit::Fn8F0E70);
STATICINIT4_REVERSED("StaticInit::8F0E90", 0x008F0E90, &staticinit::Fn8F0E90);
STATICINIT4_REVERSED("StaticInit::8F0EB0", 0x008F0EB0, &staticinit::Fn8F0EB0);
STATICINIT4_REVERSED("StaticInit::8F0ED0", 0x008F0ED0, &staticinit::Fn8F0ED0);
STATICINIT4_REVERSED("StaticInit::8F0EF0", 0x008F0EF0, &staticinit::Fn8F0EF0);
STATICINIT4_REVERSED("StaticInit::8F0F10", 0x008F0F10, &staticinit::Fn8F0F10);
STATICINIT4_REVERSED("StaticInit::8F0F30", 0x008F0F30, &staticinit::Fn8F0F30);
STATICINIT4_REVERSED("StaticInit::8F0F50", 0x008F0F50, &staticinit::Fn8F0F50);
STATICINIT4_REVERSED("StaticInit::8F0F70", 0x008F0F70, &staticinit::Fn8F0F70);
STATICINIT4_REVERSED("StaticInit::8F0F90", 0x008F0F90, &staticinit::Fn8F0F90);
STATICINIT4_REVERSED("StaticInit::8F0FB0", 0x008F0FB0, &staticinit::Fn8F0FB0);
STATICINIT4_REVERSED("StaticInit::8F0FD0", 0x008F0FD0, &staticinit::Fn8F0FD0);
STATICINIT4_REVERSED("StaticInit::8F0FF0", 0x008F0FF0, &staticinit::Fn8F0FF0);
STATICINIT4_REVERSED("StaticInit::8F1010", 0x008F1010, &staticinit::Fn8F1010);
STATICINIT4_REVERSED("StaticInit::8F1030", 0x008F1030, &staticinit::Fn8F1030);
STATICINIT4_REVERSED("StaticInit::8F1050", 0x008F1050, &staticinit::Fn8F1050);
STATICINIT4_REVERSED("StaticInit::8F1070", 0x008F1070, &staticinit::Fn8F1070);
STATICINIT4_REVERSED("StaticInit::8F1090", 0x008F1090, &staticinit::Fn8F1090);
STATICINIT4_REVERSED("StaticInit::8F10B0", 0x008F10B0, &staticinit::Fn8F10B0);
STATICINIT4_REVERSED("StaticInit::8F10D0", 0x008F10D0, &staticinit::Fn8F10D0);
STATICINIT4_REVERSED("StaticInit::8F10F0", 0x008F10F0, &staticinit::Fn8F10F0);
STATICINIT4_REVERSED("StaticInit::8F1110", 0x008F1110, &staticinit::Fn8F1110);
STATICINIT4_REVERSED("StaticInit::8F1130", 0x008F1130, &staticinit::Fn8F1130);
STATICINIT4_REVERSED("StaticInit::8F1150", 0x008F1150, &staticinit::Fn8F1150);
STATICINIT4_REVERSED("StaticExit::8F1170", 0x008F1170, &staticinit::Fn8F1170);
STATICINIT4_REVERSED("StaticExit::8F11A0", 0x008F11A0, &staticinit::Fn8F11A0);
STATICINIT4_REVERSED("StaticExit::8F11D0", 0x008F11D0, &staticinit::Fn8F11D0);
STATICINIT4_REVERSED("StaticExit::8F1200", 0x008F1200, &staticinit::Fn8F1200);
STATICINIT4_REVERSED("StaticExit::8F1230", 0x008F1230, &staticinit::Fn8F1230);
STATICINIT4_REVERSED("StaticExit::8F1260", 0x008F1260, &staticinit::Fn8F1260);
STATICINIT4_REVERSED("StaticExit::8F1290", 0x008F1290, &staticinit::Fn8F1290);
STATICINIT4_REVERSED("StaticExit::8F12C0", 0x008F12C0, &staticinit::Fn8F12C0);
STATICINIT4_REVERSED("StaticExit::8F12F0", 0x008F12F0, &staticinit::Fn8F12F0);
STATICINIT4_REVERSED("StaticExit::8F1300", 0x008F1300, &staticinit::Fn8F1300);
STATICINIT4_REVERSED("StaticExit::8F1330", 0x008F1330, &staticinit::Fn8F1330);
STATICINIT4_REVERSED("StaticExit::8F1370", 0x008F1370, &staticinit::Fn8F1370);
STATICINIT4_REVERSED("StaticExit::8F13B0", 0x008F13B0, &staticinit::Fn8F13B0);
STATICINIT4_REVERSED("StaticExit::8F13F0", 0x008F13F0, &staticinit::Fn8F13F0);
STATICINIT4_REVERSED("StaticExit::8F1430", 0x008F1430, &staticinit::Fn8F1430);
STATICINIT4_REVERSED("StaticExit::8F1470", 0x008F1470, &staticinit::Fn8F1470);
STATICINIT4_REVERSED("StaticExit::8F14B0", 0x008F14B0, &staticinit::Fn8F14B0);
STATICINIT4_REVERSED("StaticExit::8F14F0", 0x008F14F0, &staticinit::Fn8F14F0);
STATICINIT4_REVERSED("StaticExit::8F1530", 0x008F1530, &staticinit::Fn8F1530);
STATICINIT4_REVERSED("StaticExit::8F1570", 0x008F1570, &staticinit::Fn8F1570);
STATICINIT4_REVERSED("StaticExit::8F15B0", 0x008F15B0, &staticinit::Fn8F15B0);
STATICINIT4_REVERSED("StaticExit::8F15F0", 0x008F15F0, &staticinit::Fn8F15F0);
STATICINIT4_REVERSED("StaticExit::8F1630", 0x008F1630, &staticinit::Fn8F1630);
STATICINIT4_REVERSED("StaticExit::8F1670", 0x008F1670, &staticinit::Fn8F1670);
STATICINIT4_REVERSED("StaticExit::8F16B0", 0x008F16B0, &staticinit::Fn8F16B0);
STATICINIT4_REVERSED("StaticExit::8F16F0", 0x008F16F0, &staticinit::Fn8F16F0);
STATICINIT4_REVERSED("StaticExit::8F1730", 0x008F1730, &staticinit::Fn8F1730);
STATICINIT4_REVERSED("StaticExit::8F1770", 0x008F1770, &staticinit::Fn8F1770);
STATICINIT4_REVERSED("StaticExit::8F17B0", 0x008F17B0, &staticinit::Fn8F17B0);
STATICINIT4_REVERSED("StaticExit::8F17F0", 0x008F17F0, &staticinit::Fn8F17F0);
STATICINIT4_REVERSED("StaticExit::8F1830", 0x008F1830, &staticinit::Fn8F1830);
STATICINIT4_REVERSED("StaticExit::8F1870", 0x008F1870, &staticinit::Fn8F1870);
STATICINIT4_REVERSED("StaticExit::8F18B0", 0x008F18B0, &staticinit::Fn8F18B0);
STATICINIT4_REVERSED("StaticExit::8F18F0", 0x008F18F0, &staticinit::Fn8F18F0);
STATICINIT4_REVERSED("StaticExit::8F1930", 0x008F1930, &staticinit::Fn8F1930);
STATICINIT4_REVERSED("StaticExit::8F1970", 0x008F1970, &staticinit::Fn8F1970);
STATICINIT4_REVERSED("StaticExit::8F19B0", 0x008F19B0, &staticinit::Fn8F19B0);
STATICINIT4_REVERSED("StaticExit::8F19F0", 0x008F19F0, &staticinit::Fn8F19F0);
STATICINIT4_REVERSED("StaticExit::8F1A30", 0x008F1A30, &staticinit::Fn8F1A30);
STATICINIT4_REVERSED("StaticExit::8F1A70", 0x008F1A70, &staticinit::Fn8F1A70);
STATICINIT4_REVERSED("StaticExit::8F1AB0", 0x008F1AB0, &staticinit::Fn8F1AB0);
STATICINIT4_REVERSED("StaticExit::8F1AF0", 0x008F1AF0, &staticinit::Fn8F1AF0);
STATICINIT4_REVERSED("StaticExit::8F1B30", 0x008F1B30, &staticinit::Fn8F1B30);
STATICINIT4_REVERSED("StaticExit::8F1B70", 0x008F1B70, &staticinit::Fn8F1B70);
STATICINIT4_REVERSED("StaticExit::8F1BB0", 0x008F1BB0, &staticinit::Fn8F1BB0);
STATICINIT4_REVERSED("StaticExit::8F1BF0", 0x008F1BF0, &staticinit::Fn8F1BF0);
STATICINIT4_REVERSED("StaticExit::8F1C30", 0x008F1C30, &staticinit::Fn8F1C30);
STATICINIT4_REVERSED("StaticExit::8F1CA0", 0x008F1CA0, &staticinit::Fn8F1CA0);
STATICINIT4_REVERSED("StaticExit::8F1CB0", 0x008F1CB0, &staticinit::Fn8F1CB0);
STATICINIT4_REVERSED("StaticExit::8F1CC0", 0x008F1CC0, &staticinit::Fn8F1CC0);
STATICINIT4_REVERSED("StaticExit::8F1CD0", 0x008F1CD0, &staticinit::Fn8F1CD0);
STATICINIT4_REVERSED("StaticExit::8F1D50", 0x008F1D50, &staticinit::Fn8F1D50);
STATICINIT4_REVERSED("StaticExit::8F1DB0", 0x008F1DB0, &staticinit::Fn8F1DB0);
STATICINIT4_REVERSED("StaticExit::8F1DC0", 0x008F1DC0, &staticinit::Fn8F1DC0);
STATICINIT4_REVERSED("StaticExit::8F1DD0", 0x008F1DD0, &staticinit::Fn8F1DD0);
STATICINIT4_REVERSED("StaticExit::8F1DF0", 0x008F1DF0, &staticinit::Fn8F1DF0);
STATICINIT4_REVERSED("StaticExit::8F1E00", 0x008F1E00, &staticinit::Fn8F1E00);
STATICINIT4_REVERSED("StaticExit::8F1E10", 0x008F1E10, &staticinit::Fn8F1E10);
STATICINIT4_REVERSED("StaticExit::8F1E20", 0x008F1E20, &staticinit::Fn8F1E20);
STATICINIT4_REVERSED("StaticExit::8F1E30", 0x008F1E30, &staticinit::Fn8F1E30);
STATICINIT4_REVERSED("StaticExit::8F1E40", 0x008F1E40, &staticinit::Fn8F1E40);
STATICINIT4_REVERSED("StaticExit::8F1E80", 0x008F1E80, &staticinit::Fn8F1E80);
STATICINIT4_REVERSED("StaticExit::8F1EA0", 0x008F1EA0, &staticinit::Fn8F1EA0);
STATICINIT4_REVERSED("StaticExit::8F1EB0", 0x008F1EB0, &staticinit::Fn8F1EB0);
STATICINIT4_REVERSED("StaticExit::8F1EC0", 0x008F1EC0, &staticinit::Fn8F1EC0);
STATICINIT4_REVERSED("StaticExit::8F1ED0", 0x008F1ED0, &staticinit::Fn8F1ED0);
STATICINIT4_REVERSED("StaticExit::8F1EE0", 0x008F1EE0, &staticinit::Fn8F1EE0);
STATICINIT4_REVERSED("StaticExit::8F1EF0", 0x008F1EF0, &staticinit::Fn8F1EF0);
STATICINIT4_REVERSED("StaticExit::8F1F00", 0x008F1F00, &staticinit::Fn8F1F00);
STATICINIT4_REVERSED("StaticExit::8F1F10", 0x008F1F10, &staticinit::Fn8F1F10);
STATICINIT4_REVERSED("StaticExit::8F1F20", 0x008F1F20, &staticinit::Fn8F1F20);
STATICINIT4_REVERSED("StaticExit::8F1F30", 0x008F1F30, &staticinit::Fn8F1F30);
STATICINIT4_REVERSED("StaticExit::8F1F40", 0x008F1F40, &staticinit::Fn8F1F40);
STATICINIT4_REVERSED("StaticExit::8F1F50", 0x008F1F50, &staticinit::Fn8F1F50);
STATICINIT4_REVERSED("StaticExit::8F1F60", 0x008F1F60, &staticinit::Fn8F1F60);
STATICINIT4_REVERSED("StaticExit::8F1F70", 0x008F1F70, &staticinit::Fn8F1F70);
STATICINIT4_REVERSED("StaticExit::8F1F80", 0x008F1F80, &staticinit::Fn8F1F80);
STATICINIT4_REVERSED("StaticExit::8F1F90", 0x008F1F90, &staticinit::Fn8F1F90);
STATICINIT4_REVERSED("StaticExit::8F1FA0", 0x008F1FA0, &staticinit::Fn8F1FA0);
STATICINIT4_REVERSED("StaticExit::8F1FB0", 0x008F1FB0, &staticinit::Fn8F1FB0);
STATICINIT4_REVERSED("StaticExit::8F1FC0", 0x008F1FC0, &staticinit::Fn8F1FC0);
STATICINIT4_REVERSED("StaticExit::8F1FD0", 0x008F1FD0, &staticinit::Fn8F1FD0);
STATICINIT4_REVERSED("StaticExit::8F1FE0", 0x008F1FE0, &staticinit::Fn8F1FE0);
STATICINIT4_REVERSED("StaticExit::8F1FF0", 0x008F1FF0, &staticinit::Fn8F1FF0);
STATICINIT4_REVERSED("StaticExit::8F2000", 0x008F2000, &staticinit::Fn8F2000);
STATICINIT4_REVERSED("StaticExit::8F2010", 0x008F2010, &staticinit::Fn8F2010);
STATICINIT4_REVERSED("StaticExit::8F2020", 0x008F2020, &staticinit::Fn8F2020);
STATICINIT4_REVERSED("StaticExit::8F2030", 0x008F2030, &staticinit::Fn8F2030);
STATICINIT4_REVERSED("StaticExit::8F2040", 0x008F2040, &staticinit::Fn8F2040);
STATICINIT4_REVERSED("StaticExit::8F2050", 0x008F2050, &staticinit::Fn8F2050);
STATICINIT4_REVERSED("StaticExit::8F2060", 0x008F2060, &staticinit::Fn8F2060);
STATICINIT4_REVERSED("StaticExit::8F2070", 0x008F2070, &staticinit::Fn8F2070);
STATICINIT4_REVERSED("StaticExit::8F2080", 0x008F2080, &staticinit::Fn8F2080);
STATICINIT4_REVERSED("StaticExit::8F2090", 0x008F2090, &staticinit::Fn8F2090);
STATICINIT4_REVERSED("StaticExit::8F20A0", 0x008F20A0, &staticinit::Fn8F20A0);
STATICINIT4_REVERSED("StaticExit::8F20B0", 0x008F20B0, &staticinit::Fn8F20B0);
STATICINIT4_REVERSED("StaticExit::8F20C0", 0x008F20C0, &staticinit::Fn8F20C0);
STATICINIT4_REVERSED("StaticExit::8F20D0", 0x008F20D0, &staticinit::Fn8F20D0);
STATICINIT4_REVERSED("StaticExit::8F20E0", 0x008F20E0, &staticinit::Fn8F20E0);
STATICINIT4_REVERSED("StaticExit::8F20F0", 0x008F20F0, &staticinit::Fn8F20F0);
STATICINIT4_REVERSED("StaticExit::8F2100", 0x008F2100, &staticinit::Fn8F2100);
STATICINIT4_REVERSED("StaticExit::8F2110", 0x008F2110, &staticinit::Fn8F2110);
STATICINIT4_REVERSED("StaticExit::8F2120", 0x008F2120, &staticinit::Fn8F2120);
STATICINIT4_REVERSED("StaticExit::8F2140", 0x008F2140, &staticinit::Fn8F2140);
STATICINIT4_REVERSED("StaticExit::8F2190", 0x008F2190, &staticinit::Fn8F2190);
STATICINIT4_REVERSED("StaticExit::8F21A0", 0x008F21A0, &staticinit::Fn8F21A0);
STATICINIT4_REVERSED("StaticExit::8F2230", 0x008F2230, &staticinit::Fn8F2230);
STATICINIT4_REVERSED("StaticExit::8F2240", 0x008F2240, &staticinit::Fn8F2240);
STATICINIT4_REVERSED("StaticExit::8F2250", 0x008F2250, &staticinit::Fn8F2250);
STATICINIT4_REVERSED("StaticExit::8F2340", 0x008F2340, &staticinit::Fn8F2340);
STATICINIT4_REVERSED("StaticExit::8F2350", 0x008F2350, &staticinit::Fn8F2350);
STATICINIT4_REVERSED("StaticExit::8F2390", 0x008F2390, &staticinit::Fn8F2390);
STATICINIT4_REVERSED("StaticExit::8F24A0", 0x008F24A0, &staticinit::Fn8F24A0);
STATICINIT4_REVERSED("StaticExit::8F24B0", 0x008F24B0, &staticinit::Fn8F24B0);
STATICINIT4_REVERSED("StaticExit::8F24C0", 0x008F24C0, &staticinit::Fn8F24C0);
STATICINIT4_REVERSED("StaticExit::8F24D0", 0x008F24D0, &staticinit::Fn8F24D0);
STATICINIT4_REVERSED("StaticExit::8F24F0", 0x008F24F0, &staticinit::Fn8F24F0);
STATICINIT4_REVERSED("StaticExit::8F2500", 0x008F2500, &staticinit::Fn8F2500);
STATICINIT4_REVERSED("StaticExit::8F2540", 0x008F2540, &staticinit::Fn8F2540);
STATICINIT4_REVERSED("StaticExit::8F2550", 0x008F2550, &staticinit::Fn8F2550);
STATICINIT4_REVERSED("StaticExit::8F2670", 0x008F2670, &staticinit::Fn8F2670);
STATICINIT4_REVERSED("StaticExit::8F2680", 0x008F2680, &staticinit::Fn8F2680);
STATICINIT4_REVERSED("StaticExit::8F2690", 0x008F2690, &staticinit::Fn8F2690);
STATICINIT4_REVERSED("StaticExit::8F27A0", 0x008F27A0, &staticinit::Fn8F27A0);
STATICINIT4_REVERSED("StaticExit::8F27D0", 0x008F27D0, &staticinit::Fn8F27D0);
STATICINIT4_REVERSED("StaticExit::8F27E0", 0x008F27E0, &staticinit::Fn8F27E0);
STATICINIT4_REVERSED("StaticExit::8F27F0", 0x008F27F0, &staticinit::Fn8F27F0);
STATICINIT4_REVERSED("StaticExit::8F2830", 0x008F2830, &staticinit::Fn8F2830);
STATICINIT4_REVERSED("StaticExit::8F28C0", 0x008F28C0, &staticinit::Fn8F28C0);
STATICINIT4_REVERSED("StaticExit::8F28D0", 0x008F28D0, &staticinit::Fn8F28D0);
STATICINIT4_REVERSED("StaticExit::8F28E0", 0x008F28E0, &staticinit::Fn8F28E0);
STATICINIT4_REVERSED("StaticExit::8F28F0", 0x008F28F0, &staticinit::Fn8F28F0);
STATICINIT4_REVERSED("StaticExit::8F2900", 0x008F2900, &staticinit::Fn8F2900);
STATICINIT4_REVERSED("StaticExit::8F2910", 0x008F2910, &staticinit::Fn8F2910);
STATICINIT4_REVERSED("StaticExit::8F2920", 0x008F2920, &staticinit::Fn8F2920);
STATICINIT4_REVERSED("StaticExit::8F2930", 0x008F2930, &staticinit::Fn8F2930);
STATICINIT4_REVERSED("StaticExit::8F2940", 0x008F2940, &staticinit::Fn8F2940);
STATICINIT4_REVERSED("StaticExit::8F2950", 0x008F2950, &staticinit::Fn8F2950);
STATICINIT4_REVERSED("StaticExit::8F2960", 0x008F2960, &staticinit::Fn8F2960);
STATICINIT4_REVERSED("StaticExit::8F2970", 0x008F2970, &staticinit::Fn8F2970);
STATICINIT4_REVERSED("StaticExit::8F2980", 0x008F2980, &staticinit::Fn8F2980);
STATICINIT4_REVERSED("StaticExit::8F2990", 0x008F2990, &staticinit::Fn8F2990);
STATICINIT4_REVERSED("StaticExit::8F29A0", 0x008F29A0, &staticinit::Fn8F29A0);
STATICINIT4_REVERSED("StaticExit::8F29B0", 0x008F29B0, &staticinit::Fn8F29B0);
STATICINIT4_REVERSED("StaticExit::8F29C0", 0x008F29C0, &staticinit::Fn8F29C0);
STATICINIT4_REVERSED("StaticExit::8F29D0", 0x008F29D0, &staticinit::Fn8F29D0);
STATICINIT4_REVERSED("StaticExit::8F29E0", 0x008F29E0, &staticinit::Fn8F29E0);
