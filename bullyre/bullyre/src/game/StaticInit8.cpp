// The static initializers and exit functions tools/init/gen_init.py leaves (tools/chunks_gb/
// chunk_init.txt), written by hand: arrays of globals constructed or destroyed in loops (the
// counter's `sub 1; jns` runs count + 1 times), blocks of floats and dwords zeroed, fixed
// strings and pointers released in reverse, sines, cosines and square roots of constants.
// Same conventions as StaticInit1-7.cpp: the game's addresses for every call, float constants
// computed in double (the x87 at its default 53 bits at start-up).

#include "game/StaticInit.h"
#include "reversed.h"

namespace staticinit {
namespace {

constexpr uintptr_t HashName = 0x00576ED0;
constexpr uintptr_t VectorConstructor = 0x00401240;   // `eh vector constructor iterator' (stdcall)
constexpr uintptr_t Free = 0x005EEAB0;
constexpr uintptr_t EntityRef_Set = 0x005D7F10;        // thiscall (ref, entity)
constexpr uintptr_t Entity_CleanUpOldReference = 0x004657E0;
constexpr uintptr_t Construct668E90 = 0x00668E90;
constexpr uintptr_t Destruct668EB0 = 0x00668EB0;
constexpr uintptr_t NiQuaternionSet = 0x00775B90;     // thiscall (w, x, y, z)

// ---- initializers

// 0x8D9D80 (initializer)
void __cdecl Fn8D9D80() {
    Dword(0x00BCBC0C) = Dword(0x009D3434);
    Dword(0x00BCBC10) = Dword(0x009D3430);
    Dword(0x00BCBC14) = Dword(0x009D342C);
}

// 0x8D9E30 (initializer)
void __cdecl Fn8D9E30() {
    Dword(0x00BCBC78) = 0;
    Dword(0x00BCBC7C) = 0;
}

// 0x8DA7A0 (initializer)
void __cdecl Fn8DA7A0() {
    for (uintptr_t p = 0x009E343C, i = 0; i < 64; ++i, p += 0x6C) {
        Dword(p) = 0x009009C4;   // vtable
        Byte(p + 4) = 0;
    }
    AtexitGame(0x008F1C70);
}

// 0x8DABE0 (initializer)
void __cdecl Fn8DABE0() {
    for (uintptr_t p = 0x00BD1010, i = 0; i < 10; ++i, p += 8) {
        Thiscall(Construct668E90, uint32_t(p));
    }
    AtexitGame(0x008F1D20);
}

// 0x8DACB0 (initializer)
void __cdecl Fn8DACB0() {
    Float(0x00BD3E70) = float(double(Float(0x009F4794)) + double(Float(0x009F4794)));
}

// 0x8DAF90 (initializer)
void __cdecl Fn8DAF90() {
    for (uintptr_t p = 0x00C0A110, i = 0; i < 300; ++i, p += 0x48) {
        Thiscall(Construct668E90, uint32_t(p - 0x10));
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
    AtexitGame(0x008F1E50);
}

// 0x8DB100 (initializer)
void __cdecl Fn8DB100() {
    Thiscall(Construct668E90, 0x00C0F6B0u);
    Float(0x00C0F6C0) = 0.0f;
    Dword(0x00C0F6B8) = 0;
    Byte(0x00C0F6BC) = 0;
    AtexitGame(0x008F1E90);
}

// 0x8DB330 (initializer)
void __cdecl Fn8DB330() {
    Thiscall(0x00454E00, 0x00A14810u);
    Float(0x00A1491C) = 1.0f;
    Byte(0x00A14926) = 0;
    Float(0x00A14920) = Float(0x009009AC);
    Byte(0x00A14925) = 0;
    Byte(0x00A14924) = 1;
}

// 0x8DB950 (initializer)
void __cdecl Fn8DB950() {
    FillDwords(0x00C1A7E8, 0xFFFFFFFF, 0x43);
    AtexitGame(0x008F2130);
}

// 0x8DBA80 (initializer)
void __cdecl Fn8DBA80() {
    Thiscall(0x0056E7A0, 0x00C1AEACu);
    for (uintptr_t p = 0x00C1AF70; p <= 0x00C1AF84; p += 4) {
        Float(p) = 0.0f;
    }
    Dword(0x00C1B174) = 0;
    AtexitGame(0x008F2170);
}

// 0x8DBAD0 (initializer)
void __cdecl Fn8DBAD0() {
    for (uintptr_t p = 0x00C1B178, i = 0; i < 0x510; ++i, p += 0x14) {
        Stdcall(VectorConstructor, uint32_t(p), 4u, 5u, 0x00667DA0u);
    }
    AtexitGame(0x008F21E0);
}

// 0x8DBB40 (initializer)
void __cdecl Fn8DBB40() {
    float c = Float(0x009009B8);
    for (uintptr_t p = 0x00C216D8, i = 0; i < 32; ++i, p += 0x28) {
        Dword(p - 0xC) = 0;
        Float(p - 0x18) = 0.0f;
        Float(p - 0x14) = 0.0f;
        Float(p - 0x10) = 0.0f;
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
        Float(p + 0xC) = c;
    }
    AtexitGame(0x008F21B0);
}

// 0x8DBB90 (initializer)
void __cdecl Fn8DBB90() {
    FillDwords(0x00C21BC0, 0, 0x32);
    Dword(0x00C21C88) = 0;
    Byte(0x00C21C8C) = 0;
    Dword(0x00C21C90) = 0;
    Dword(0x00C21C94) = 0;
    Dword(0x00C21C98) = 0;
    AtexitGame(0x008F2210);
}

// 0x8DBE90 (initializer)
void __cdecl Fn8DBE90() {
    Dword(0x00C2B144) = Dword(0x00BCC564);
    Dword(0x00C2B148) = Dword(0x00BCC568);
    Dword(0x00C2B14C) = Dword(0x00BCC56C);
}

// 0x8DBEC0 (initializer)
void __cdecl Fn8DBEC0() {
    Float(0x00C2B150) = 0.0f;
    Float(0x00C2B154) = 0.0f;
    Dword(0x00C2B15C) = 0;
    Float(0x00C2B158) = 0.0f;
    Dword(0x00C2B184) = 0;
    Float(0x00C2B160) = 0.0f;
    Float(0x00C2B164) = 0.0f;
    Float(0x00C2B168) = 0.0f;
    float c = Float(0x009009B8);
    Float(0x00C2B174) = c;
    Float(0x00C2B178) = 0.0f;
    Float(0x00C2B17C) = 0.0f;
    Float(0x00C2B180) = 0.0f;
    Float(0x00C2B188) = 0.0f;
    Float(0x00C2B18C) = 0.0f;
    Float(0x00C2B190) = 0.0f;
    Float(0x00C2B19C) = c;
    AtexitGame(0x008F22B0);
}

// 0x8DBF40 (initializer)
void __cdecl Fn8DBF40() {
    Float(0x00C2B1A0) = float(crt::Sin(Double(0x0090D1E0)));
}

// 0x8DBF60 (initializer)
void __cdecl Fn8DBF60() {
    Float(0x00C2B1A4) = float(crt::Cos(Double(0x0090CFA8)));
}

// 0x8DBF80 (initializer)
void __cdecl Fn8DBF80() {
    Float(0x00C2B1A8) = float(crt::Cos(Double(0x0090D1E8)));
}

// 0x8DBFA0 (initializer)
void __cdecl Fn8DBFA0() {
    Float(0x00C2B1D0) = float(crt::Sin(Double(0x0090D218)));
}

// 0x8DBFC0 (initializer)
void __cdecl Fn8DBFC0() {
    Float(0x00C2B1D4) = float(crt::Sin(Double(0x0090CFA8)));
}

// 0x8DC100 (initializer)
void __cdecl Fn8DC100() {
    for (uintptr_t p = 0x00C2B3C4, i = 0; i < 200; ++i, p += 0x10) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DC480 (initializer)
void __cdecl Fn8DC480() {
    for (uintptr_t p = 0x00A35244, i = 0; i < 87; ++i, p += 0xC) {
        Float(p) = 0.0f;
        Dword(p - 8) = 0x57;
        Byte(p - 4) = 0;
    }
    Dword(0x00A35650) = 0;
    AtexitGame(0x008F2330);
}

// 0x8DC680 (initializer)
void __cdecl Fn8DC680() {
    for (uintptr_t p = 0x00C2D1F4, i = 0; i < 50; ++i, p += 0x30) {
        Dword(p) = 0;
    }
    AtexitGame(0x008F2360);
}

// 0x8DC840 (initializer)
void __cdecl Fn8DC840() {
    for (uintptr_t p = 0x00C2DC5C, i = 0; i < 8; ++i, p += 0x4C) {
        for (uintptr_t q = p - 0x14; q <= p; q += 4) {
            Float(q) = 0.0f;
        }
        Stdcall(VectorConstructor, uint32_t(p + 4), 0xCu, 2u, 0x00508410u);
        Dword(p + 0x30) = 0;
    }
    AtexitGame(0x008F23A0);
}

// 0x8DC8A0 (initializer)
void __cdecl Fn8DC8A0() {
    for (uintptr_t p = 0x00C2DEAC, i = 0; i < 2; ++i, p += 8) {
        Dword(p) = 0;
        Thiscall(EntityRef_Set, uint32_t(p), 0u);
        Dword(p - 4) = 0xFFFFFFFF;
    }
    AtexitGame(0x008F23D0);
}

// 0x8DCBF0 (initializer)
void __cdecl Fn8DCBF0() {
    for (uintptr_t p = 0x00C39B7C, i = 0; i < 15; ++i, p += 0x3C) {
        Thiscall(Construct668E90, uint32_t(p - 0x18));
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
    AtexitGame(0x008F2400);
}

// 0x8DCD30 (initializer)
void __cdecl Fn8DCD30() {
    for (uintptr_t p = 0x00C3A058, i = 0; i < 32; ++i, p += 0x34) {
        Dword(p - 0x1C) = 0;
        Thiscall(Construct668E90, uint32_t(p));
    }
    AtexitGame(0x008F2460);
}

// 0x8DCD70 (initializer)
void __cdecl Fn8DCD70() {
    FillDwords(0x00C3A6B8, 0, 32);
    AtexitGame(0x008F2430);
}

// 0x8DCF00 (initializer)
void __cdecl Fn8DCF00() {
    Float(0x00C3B74C) = 0.0f;
    Float(0x00C3B750) = 0.0f;
    Byte(0x00C3B748) = 0;
    Float(0x00C3B754) = 0.0f;
    Dword(0x00C3B784) = 0;
    Float(0x00C3B758) = 0.0f;
    Byte(0x00C3B788) = 0;
    Float(0x00C3B75C) = 0.0f;
    Dword(0x00C3B7C4) = 0;
    Float(0x00C3B760) = 0.0f;
    for (uintptr_t p = 0x00C3B78C; p <= 0x00C3B7A0; p += 4) {
        Float(p) = 0.0f;
    }
    AtexitGame(0x008F24E0);
}

// 0x8DD0F0 (initializer)
void __cdecl Fn8DD0F0() {
    for (uintptr_t p = 0x00C3D0F8, i = 0; i < 32; ++i, p += 0xC) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DD2A0 (initializer): a packed table of 9-byte entries (a type byte, then 8 bytes), some
// values copied from other globals through the x87, the rest debug fill patterns
void __cdecl Fn8DD2A0() {
    Float(0x00A45C04) = Float(0x00BC9438);
    Dword(0x00A45C16) = 0xC9C9C9C9;
    Float(0x00A45C08) = Float(0x00BC9438);
    Dword(0x00A45C1A) = 0xC9C9C9C9;
    Float(0x00A45C0D) = Float(0x00BC9418);
    Dword(0x00A45C31) = 0x67676767;
    Float(0x00A45C11) = Float(0x00BC9418);
    Dword(0x00A45C35) = 0x67676767;
    Qword(0x00A45C1F) = Qword(0x00BC94B0);
    Qword(0x00A45C28) = Qword(0x00BC9490);
    Dword(0x00A45C3A) = 0x08675309;
    Dword(0x00A45C3E) = 0x08675309;
    Float(0x00A45C55) = 0.0f;
    Float(0x00A45C59) = 0.0f;
    Dword(0x00A45BE0) = 0x66666666;
    Dword(0x00A45BE4) = 0x66666666;
    Qword(0x00A45C5E) = 0;   // 0.0
    Dword(0x00A45C43) = 0xFEEEFEEE;
    Dword(0x00A45C47) = 0xFEEEFEEE;
    Byte(0x00A45BF1) = 1;
    Dword(0x00A45BE9) = 0;
    Dword(0x00A45BED) = 0;
    Byte(0x00A45BFA) = 0;
    Byte(0x00A45C03) = 0;
    for (uintptr_t p = 0x00A45C0C; p <= 0x00A45C54; p += 9) {
        Byte(p) = 1;
    }
    Dword(0x00A45C4C) = 0x7FBADBAD;
    Dword(0x00A45C50) = 0x7FBADBAD;
    Byte(0x00A45C5D) = 1;
    Byte(0x00A45C66) = 1;
    Byte(0x00A45C6F) = 0;
}

// 0x8DD3C0 (initializer)
void __cdecl Fn8DD3C0() {
    Byte(0x00C3D330) = Byte(0x00A45C54);
    Dword(0x00C3D328) = Dword(0x00A45C4C);
    Dword(0x00C3D32C) = Dword(0x00A45C50);
    Dword(0x00C3D331) = Dword(0x00A45C43);
    Dword(0x00C3D335) = Dword(0x00A45C47);
    Byte(0x00C3D339) = Byte(0x00A45C4B);
}

// 0x8DD4A0 (initializer)
void __cdecl Fn8DD4A0() {
    for (uintptr_t p = 0x00C660A8, i = 0; i < 56; ++i, p += 0x30) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DD4C0 (initializer)
void __cdecl Fn8DD4C0() {
    FillDwords(0x00C66B38, 0, 15);
}

// 0x8DD520 (initializer)
void __cdecl Fn8DD520() {
    Float(0x00C67570) = 0.0f;
    Dword(0x00C67588) = 0;
    Float(0x00C67574) = 0.0f;
    Dword(0x00C675AC) = 0;
    Float(0x00C67578) = 0.0f;
    Float(0x00C6757C) = 0.0f;
    Float(0x00C67580) = 0.0f;
    Float(0x00C67584) = 1.0f;
    for (uintptr_t p = 0x00C6758C; p <= 0x00C675A4; p += 4) {
        Float(p) = 0.0f;
    }
    Float(0x00C675A8) = 1.0f;
    Float(0x00C675B0) = 0.0f;
    Float(0x00C675B4) = 0.0f;
}

// 0x8DD6C0 (initializer)
void __cdecl Fn8DD6C0() {
    for (uintptr_t p = 0x00C7697C, i = 0; i < 21; ++i, p += 0x54) {
        Thiscall(0x005109E0, uint32_t(p), 5u);
        Dword(p) = 0x00917BAC;   // vtable
    }
    Dword(0x00C76978) = 0;
    AtexitGame(0x008F2710);
}

// 0x8DD710 (initializer)
void __cdecl Fn8DD710() {
    for (uintptr_t p = 0x00C77064, i = 0; i < 5; ++i, p += 0x4C) {
        Thiscall(0x005109B0, uint32_t(p));
    }
    Dword(0x00C77060) = 0;
    AtexitGame(0x008F2740);
}

// 0x8DD750 (initializer)
void __cdecl Fn8DD750() {
    for (uintptr_t p = 0x00C771E4, i = 0; i < 32; ++i, p += 0x1E0) {
        Thiscall(0x00535110, uint32_t(p));
    }
    Dword(0x00C771E0) = 0;
    AtexitGame(0x008F2800);
}

// 0x8DD790 (initializer)
void __cdecl Fn8DD790() {
    for (uintptr_t p = 0x00C7AE40, i = 0; i < 0x123; ++i, p += 0xD4) {
        uintptr_t object = p - 0x54;
        Thiscall(0x005109E0, uint32_t(object), 8u);
        Byte(p - 4) = 0;
        Dword(p) = 0;
        Dword(p + 4) = 0;
        Dword(p + 0x10) = 0;
        Dword(p + 0x14) = 0;
        Dword(p + 0x18) = 0;
        Dword(p + 0x1C) = 0;
        Dword(p + 0x30) = 0xFFFFFFFF;
        Dword(p + 0x40) = 0xFFFFFFFF;
        Dword(p + 0x74) = 0xFFFFFFFF;
        Dword(p + 0x78) = 0xFFFFFFFF;
        Dword(p + 0x7C) = 0xFFFFFFFF;
        Dword(object) = 0x00917C28;   // vtable
        for (uintptr_t q = p + 0x5C; q <= p + 0x70; q += 4) {
            Dword(q) = 0xFFFFFFFF;
        }
        Byte(p + 0x44) = 0;
    }
    Dword(0x00C7ADE8) = 0;
    AtexitGame(0x008F2770);
}

// 0x8DD860 (initializer)
void __cdecl Fn8DD860() {
    for (uintptr_t p = 0x00C8E914; p <= 0x00C8E928; p += 4) {
        Float(p) = 0.0f;
    }
}

// 0x8DD890 (initializer)
void __cdecl Fn8DD890() {
    for (uintptr_t p = 0x00C8E9B4, i = 0; i < 250; ++i, p += 0x3C) {
        for (uintptr_t q = p - 0x14; q <= p; q += 4) {
            Float(q) = 0.0f;
        }
    }
}

// 0x8DD8C0 (initializer)
void __cdecl Fn8DD8C0() {
    for (uintptr_t p = 0x00C92684, i = 0; i < 32; ++i, p += 0x2C) {
        for (uintptr_t q = p - 0x14; q <= p; q += 4) {
            Float(q) = 0.0f;
        }
    }
}

// 0x8DD8F0 (initializer)
void __cdecl Fn8DD8F0() {
    for (uintptr_t p = 0x00C92BF8, i = 0; i < 32; ++i, p += 0xC) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DD9B0 (initializer)
void __cdecl Fn8DD9B0() {
    for (uintptr_t p = 0x00C93788, i = 0; i < 20; ++i, p += 0x2C) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DD9D0 (initializer)
void __cdecl Fn8DD9D0() {
    for (uintptr_t p = 0x00C93AF8, i = 0; i < 12; ++i, p += 0x14) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DD9F0 (initializer)
void __cdecl Fn8DD9F0() {
    for (uintptr_t p = 0x00C93BF4, i = 0; i < 20; ++i, p += 0x40) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
    }
}

// 0x8DDA10 (initializer)
void __cdecl Fn8DDA10() {
    for (uintptr_t p = 0x00C940E0, i = 0; i < 0x17C; ++i, p += 0x68) {
        Stdcall(VectorConstructor, uint32_t(p), 0xCu, 7u, 0x00508410u);
    }
}

// 0x8DDA40 (initializer)
void __cdecl Fn8DDA40() {
    for (uintptr_t p = 0x00CA0664, i = 0; i < 32; ++i, p += 0x38) {
        for (uintptr_t q = p - 0x14; q <= p + 0x18; q += 4) {
            Float(q) = 0.0f;
        }
    }
}

// 0x8DDA80 (initializer)
void __cdecl Fn8DDA80() {
    for (uintptr_t p = 0x00CA0DBC, i = 0; i < 35; ++i, p += 0x88) {
        Float(p - 8) = 0.0f;
        Dword(p - 0x2C) = 0;
        for (uintptr_t q = p - 4; q <= p + 0xC; q += 4) {
            Float(q) = 0.0f;
        }
    }
    AtexitGame(0x008F2870);
}

// 0x8DDAD0 (initializer)
void __cdecl Fn8DDAD0() {
    for (uintptr_t p = 0x00CA2028, i = 0; i < 32; ++i, p += 0x58) {
        Dword(p) = 0;
    }
    AtexitGame(0x008F2860);
}

// 0x8DDB80 (initializer)
void __cdecl Fn8DDB80() {
    for (uintptr_t p = 0x00CA39B8, i = 0; i < 0x4334; ++i, p += 0x10) {
        Word(p) = 0xFFFF;
        Word(p + 2) = 0xFFFF;
    }
    AtexitGame(0x008F2880);
}

// 0x8DE430 (initializer)
void __cdecl Fn8DE430() {
    for (uintptr_t p = 0x00CF1718, i = 0; i < 10; ++i, p += 0xD4) {
        Thiscall(0x00560B20, uint32_t(p));
    }
}

// 0x8DE540 (initializer)
void __cdecl Fn8DE540() {
    for (uintptr_t p = 0x00AE5F74; p <= 0x00AE5FF4; p += 0x20) {
        Byte(p) &= 0xFE;
    }
}

// 0x8DE770 (initializer)
void __cdecl Fn8DE770() {
    for (uintptr_t p = 0x00CF3B38; p <= 0x00CF3B98; p += 0x30) {
        Byte(p) &= 0xFE;
    }
}

// 0x8DE810 (initializer)
void __cdecl Fn8DE810() {
    for (uintptr_t p = 0x00CF3C08, i = 0; i < 48; ++i, p += 8) {
        Dword(p) = 0;
    }
    AtexitGame(0x008F2CA0);
}

// 0x8DE930 (initializer)
void __cdecl Fn8DE930() {
    for (uint32_t i = 0; i < 0x30; ++i) {
        Word(0x00CF6910 + 2 * i) = uint16_t(i * 0x4E2);
    }
    FillDwords(0x00CF6970, 0, 0xC0);
}

// 0x8DE9F0 (initializer)
void __cdecl Fn8DE9F0() {
    Dword(0x00AE80E4) = 2;
    Float(0x00CF70D8) = 0.0f;
    Float(0x00CF70DC) = 1.0f;
    Float(0x00CF70E0) = 0.0f;
}

// 0x8DEA20 (initializer)
void __cdecl Fn8DEA20() {
    for (uintptr_t p = 0x00CF70E8; p <= 0x00CF7138; p += 0x10) {
        Float(p) = 0.0f;
        Float(p + 4) = 0.0f;
        Float(p + 8) = 0.0f;
    }
}

// 0x8DEB20 (initializer)
void __cdecl Fn8DEB20() {
    Thiscall(0x005B3DB0, 0x00CFADA8u);
    Byte(0x00CFAF1C) = 0;
    Dword(0x00CFAF20) = 0xFFFFFFFF;
    AtexitGame(0x008F2D50);
}

// 0x8DEB50 (initializer)
void __cdecl Fn8DEB50() {
    Thiscall(0x005B44F0, 0x00CFAF28u);
    Byte(0x00CFAF84) = 0;
    Dword(0x00CFAF88) = 0xFFFFFFFF;
    AtexitGame(0x008F2D60);
}

// 0x8DEBA0 (initializer)
void __cdecl Fn8DEBA0() {
    Thiscall(0x005B5AD0, 0x00CFB068u);
    Thiscall(0x005B5AD0, 0x00CFB18Cu);
    Byte(0x00CFB2B0) = 0;
    Dword(0x00CFB2B4) = 0xFFFFFFFF;
    Dword(0x00CFB2B8) = 0xFFFFFFFF;
    AtexitGame(0x008F2D80);
}

// 0x8DECE0 (initializer)
void __cdecl Fn8DECE0() {
    for (uintptr_t p = 0x00CFB2E8, i = 0; i < 6; ++i, p += 0xC) {
        Thiscall(0x006CB310, uint32_t(p));
    }
    Thiscall(0x006CB310, 0x00CFB334u);
}

// 0x8DEE90 (initializer)
void __cdecl Fn8DEE90() {
    for (uintptr_t p = 0x00CFB38C; p <= 0x00CFB3A0; p += 4) {
        Float(p) = 0.0f;
    }
}

// 0x8DF110 (initializer)
void __cdecl Fn8DF110() {
    for (uintptr_t p = 0x00CFC9E8, i = 0; i < 0x5E6; ++i, p += 0x10) {
        Dword(p - 8) = 0xFFFFFFFF;
        Dword(p - 4) = 0xFFFFFFFF;
        Dword(p) = 0xFFFFFFFF;
        Dword(p + 4) = 0;
    }
    AtexitGame(0x008F2DD0);
}

// 0x8DF150 (initializer)
void __cdecl Fn8DF150() {
    for (uintptr_t p = 0x00D02850, i = 0; i < 50; ++i, p += 0xC) {
        Dword(p) = 0;
        Dword(p + 8) = 0;
    }
    Dword(0x00D02AA8) = 0;
    Dword(0x00D093AC) = 0;
    Dword(0x00D093B0) = 0;
    Dword(0x00D093D4) = 0;
    Dword(0x00D093D8) = 0xFFFFFFFF;
    Dword(0x00D09508) = 0;
    AtexitGame(0x008F2DE0);
}

// 0x8DF230 (initializer)
void __cdecl Fn8DF230() {
    Thiscall(0x005E0F00, 0x00D09660u);
    Thiscall(0x005E7B60, 0x00D09A98u);
    Thiscall(0x005E7B60, 0x00D09ABCu);
    Float(0x00D09AE4) = 0.0f;
    Float(0x00D09AE8) = 0.0f;
    Dword(0x00D09AFC) = 0xFFFFFFFF;
    for (uintptr_t p = 0x00D09AEC; p <= 0x00D09AF8; p += 4) {
        Float(p) = 0.0f;
    }
    AtexitGame(0x008F2E40);
}

// 0x8DF290 (initializer)
void __cdecl Fn8DF290() {
    for (uintptr_t p = 0x00D09B08, i = 0; i < 1000; ++i, p += 0x24) {
        Thiscall(0x005E7B60, uint32_t(p));
    }
}

// 0x8DF460 (initializer)
void __cdecl Fn8DF460() {
    for (uintptr_t p = 0x01485248, i = 0; i < 0x493E0; ++i, p += 0x10) {
        Float(p - 8) = 0.0f;
        Float(p - 4) = 0.0f;
        Float(p) = 0.0f;
        Float(p + 4) = 0.0f;
    }
}

// 0x8DF490 (initializer)
void __cdecl Fn8DF490() {
    FillDwords(0x01B62F40, 0, 0x400);
    AtexitGame(0x008F2F50);
}

// 0x8DF4B0 (initializer)
void __cdecl Fn8DF4B0() {
    FillDwords(0x01B63F40, 0, 0xC8);
    AtexitGame(0x008F2F90);
}

// 0x8E0DF0 (initializer)
void __cdecl Fn8E0DF0() {
    Float(0x01B64D64) = float(crt::Cos(Double(0x0090CFA8)));
}

// 0x8E4FD0 (initializer)
void __cdecl Fn8E4FD0() {
    for (uintptr_t p = 0x01B655D4; p <= 0x01B65614; p += 0x10) {
        Dword(p + 8) = 0;
        Byte(p) = 0;
    }
    AtexitGame(0x008F5CE0);
}

// 0x8E8960 (initializer)
void __cdecl Fn8E8960() {
    for (uintptr_t p = 0x0208B800, i = 0; i < 8; ++i, p += 0x38) {
        Dword(p) = 0x0093EDF0;   // vtable
        Dword(p + 0x34) = 0xFFFFFFFF;
    }
    AtexitGame(0x008F7630);
}

// 0x8E9920 (initializer)
void __cdecl Fn8E9920() {
    Thiscall(NiQuaternionSet, 0x020C48A8u, 1.0f, 0.0f, 0.0f, 0.0f);
}

// 0x8E9B90 (initializer)
void __cdecl Fn8E9B90() {
    FillDwords(0x020C7C9C, 0, 0x64);
    Byte(0x020C7E90) = 0;
    AtexitGame(0x008F7A60);
}

// 0x8E9C80 (initializer)
void __cdecl Fn8E9C80() {
    double a = double(Float(0x00BC5F80));
    Float(0x020C8220) = float(Double(0x00912FF0) * a);
    Float(0x020C8224) = float(Double(0x00943CA8) * a);
    Float(0x020C8228) = float(a * Double(0x009316E8));
}

// 0x8EACF0 (initializer)
void __cdecl Fn8EACF0() {
    double a = double(Float(0x00BC6ABC));
    Float(0x020C83B0) = float(Double(0x00948A30) * a);
    Float(0x020C83B4) = float(Double(0x00912C68) * a);
    Float(0x020C83B8) = float(Double(0x00948A28) * a);
    Float(0x020C83BC) = float(Double(0x00948A20) * a);
    Float(0x020C83C0) = float(Double(0x00912738) * a);
    Float(0x020C83C4) = float(a * Double(0x00948A18));
}

// 0x8EAE60 (initializer)
void __cdecl Fn8EAE60() {
    Float(0x020C83D8) = float(Double(0x0090CFA8) - double(Float(0x00BC6C20)) * Double(0x008FF1F8));
}

// 0x8EAF10 (initializer)
void __cdecl Fn8EAF10() {
    for (uintptr_t p = 0x020C87B0, i = 0; i < 8; ++i, p += 8) {
        Thiscall(Construct668E90, uint32_t(p));
    }
    AtexitGame(0x008F8150);
}

// 0x8EBAD0 (initializer)
void __cdecl Fn8EBAD0() {
    for (uintptr_t p = 0x020CECF0, i = 0; i < 4; ++i, p += 0x914) {
        Thiscall(0x0050BF80, uint32_t(p));
    }
    AtexitGame(0x008F85E0);
}

// 0x8EBB80 (initializer)
void __cdecl Fn8EBB80() {
    FillDwords(0x020E21D8, 0, 10);
    AtexitGame(0x008F8600);
}

// 0x8EC230 (initializer)
void __cdecl Fn8EC230() {
    float s = float(crt::Sqrt(Double(0x00953178)));
    Float(0x020E2D3C) = float(Double(0x009531A0) / double(s));
}

// 0x8EC250 (initializer)
void __cdecl Fn8EC250() {
    float s = float(crt::Sqrt(Double(0x00953178)));
    double t = double(s) * Double(0x00953178);
    t = Double(0x00912CD0) / t;
    Float(0x020E2D40) = float(t * Double(0x009531A0));
}

// 0x8EC6B0 (initializer): seven floats from 0xBC8AD8 set (0, 0, untouched, 1, 1, f, f; built
// on the stack first), then 0x77EB30 called on them
void __cdecl Fn8EC6B0() {
    float f = Float(0x009055A0);
    Float(0x00BC8AE4) = 1.0f;
    Float(0x00BC8AF0) = f;
    Float(0x00BC8ADC) = 0.0f;
    Float(0x00BC8AD8) = 0.0f;
    Float(0x00BC8AEC) = f;
    Float(0x00BC8AE8) = 1.0f;
    Thiscall(0x0077EB30, 0x00BC8AD8u);
}

// 0x8ED040 (initializer)
void __cdecl Fn8ED040() {
    Float(0x020E431C) = float(1.0 / double(int32_t(Dword(0x00BC8B84))));
}

// 0x8ED060 (initializer)
void __cdecl Fn8ED060() {
    Dword(0x020E4320) = Dword(0x020E2D44);
    Dword(0x020E4324) = Dword(0x020E2D48);
    Dword(0x020E4328) = Dword(0x020E2D4C);
}

// 0x8ED090 (initializer)
void __cdecl Fn8ED090() {
    for (uintptr_t i = 0; i < 0x10; i += 4) {
        Dword(0x020E4330 + i) = Dword(0x00BC8A80 + i);
    }
}

// 0x8ED0C0 (initializer)
void __cdecl Fn8ED0C0() {
    for (uintptr_t i = 0; i < 0x10; i += 4) {
        Dword(0x020E4344 + i) = Dword(0x00BC75D4 + i);
    }
}

// 0x8ED8F0 (initializer)
void __cdecl Fn8ED8F0() {
    float f = Float(0x00968200);
    Thiscall(NiQuaternionSet, 0x020E56E8u, f, f, f, f);
}

// 0x8ED940 (initializer)
void __cdecl Fn8ED940() {
    float f = Float(0x009009BC);
    Thiscall(NiQuaternionSet, 0x020E572Cu, f, f, f, f);
}

// 0x8EF2C0 (initializer)
void __cdecl Fn8EF2C0() {
    Float(0x020E7050) = float(crt::Sqrt(Double(0x0092CD00)));
}

// ---- exit functions

// 0x8F1CF0 (exit)
void __cdecl Fn8F1CF0() {
    for (uintptr_t p = 0x00BD0FD8, i = 0; i < 10; ++i) {
        p -= 0x44;
        Thiscall(0x0059D110, uint32_t(p + 8), 0u);
        Thiscall(Destruct668EB0, uint32_t(p));
    }
}

// 0x8F1D60 (exit)
void __cdecl Fn8F1D60() {
    if (Dword(0x00BD5C78) != 0) {
        Cdecl(Free, Dword(0x00BD5C78));
        Dword(0x00BD5C78) = 0;
    }
}

// 0x8F1D80 (exit)
void __cdecl Fn8F1D80() {
    Thiscall(0x0049A650, 0x00BD779Cu);
    for (uintptr_t p = 0x00BD5FDC, i = 0; i < 20; ++i) {
        p -= 0x10;
        Thiscall(Destruct668EB0, uint32_t(p));
    }
}

// 0x8F2260 (exit): two inlined destructors of the same buffer (the second finds it cleared)
void __cdecl Fn8F2260() {
    uint32_t p = Dword(0x00C2AB70);
    if (p != 0) {
        Cdecl(Free, p);
        p = 0;
        Dword(0x00C2AB70) = 0;
    }
    Dword(0x00C2AB74) = 0;
    Dword(0x00C2AB78) = 0;
    if (p != 0) {
        Cdecl(Free, p);
        Dword(0x00C2AB70) = 0;
    }
}

// 0x8F22E0 (exit)
void __cdecl Fn8F22E0() {
    if (Dword(0x00C2B214) != 0) {
        Cdecl(Free, Dword(0x00C2B214));
        Dword(0x00C2B214) = 0;
    }
}

// 0x8F2300 (exit)
void __cdecl Fn8F2300() {
    for (uintptr_t p = 0x00C2B3A4, i = 0; i < 5; ++i) {
        p -= 0x20;
        Thiscall(EntityRef_Set, uint32_t(p), 0u);
    }
}

// two entity references let go (Entity_CleanUpOldReference(entity, &ref) when set)
void ReleaseRefs(uintptr_t first, uintptr_t second) {
    if (Dword(first) != 0) {
        Cdecl(Entity_CleanUpOldReference, Dword(first), uint32_t(first));
    }
    if (Dword(second) != 0) {
        Cdecl(Entity_CleanUpOldReference, Dword(second), uint32_t(second));
    }
}

// 0x8F2510 (exit)
void __cdecl Fn8F2510() {
    ReleaseRefs(0x00A45404, 0x00A45400);
}

// a buffer {data, size, capacity} with an optional allocator (vtable slot 2 frees): freed and
// cleared
void FreeBuffer(uintptr_t data, uintptr_t allocator) {
    uint32_t p = Dword(data);
    if (p != 0) {
        uint32_t a = Dword(allocator);
        if (a != 0) {
            reinterpret_cast<void(__thiscall*)(uint32_t, uint32_t)>(
                (*reinterpret_cast<re::Ptr32<uint32_t>*>(a))[2])(a, p);
        } else {
            Cdecl(Free, p);
        }
        Dword(data) = 0;
    }
    Dword(data + 8) = 0;
    Dword(data + 4) = 0;
}

// 0x8F2560 (exit)
void __cdecl Fn8F2560() {
    FreeBuffer(0x00C3D33C, 0x00C3D348);
}

// 0x8F25B0 (exit)
void __cdecl Fn8F25B0() {
    FreeBuffer(0x00C3D34C, 0x00C3D358);
}

// 0x8F2600 (exit)
void __cdecl Fn8F2600() {
    Dword(0x00A45C74) = 0x00914964;   // vtable
    Byte(0x00A45C88) = 0;
    Thiscall(0x0050E6B0, 0x00A45C74u);
}

// 0x8F2620 (exit)
void __cdecl Fn8F2620() {
    FreeBuffer(0x00C3D35C, 0x00C3D368);
}

// 0x8F26A0 (exit)
void __cdecl Fn8F26A0() {
    for (uintptr_t p = 0x00C66078, i = 0; i < 3; ++i) {
        p -= 4;
        ReleaseObject(p);
    }
}

// 0x8F26E0 (exit)
void __cdecl Fn8F26E0() {
    for (uintptr_t p = 0x00C76974, i = 0; i < 150; ++i) {
        p -= 0x58;
        Thiscall(0x00510D90, uint32_t(p));
    }
}

// 0x8F27B0 (exit)
void __cdecl Fn8F27B0() {
    Cdecl(Free, Dword(0x00C89F38));
    Dword(0x00C89F38) = 0;
    Dword(0x00C89F3C) = 0;
}

// 0x8F2840 (exit)
void __cdecl Fn8F2840() {
    if (Dword(0x00C936FC) != 0) {
        Thiscall(0x00523E30, 0x00C92D70u);
    }
}

// 0x8F2890 (exit)
void __cdecl Fn8F2890() {
    for (uintptr_t p = 0x00CECEC0, i = 0; i < 300; ++i) {
        p -= 0x4C;
        Thiscall(Destruct668EB0, uint32_t(p));
    }
}

// 0x8F2E80 (exit)
void __cdecl Fn8F2E80() {
    for (uintptr_t p = 0x00D13CB0, i = 0; i < 32; ++i) {
        p -= 0x2C;
        Thiscall(0x005D9C50, uint32_t(p + 0x10));
        Thiscall(EntityRef_Set, uint32_t(p), 0u);
    }
}

// 0x8F79F0 (exit)
void __cdecl Fn8F79F0() {
    ReleaseRefs(0x00BB650C, 0x00BB6508);
}

// a buffer freed (when set) and the pointer cleared
void FreeAndClear(uintptr_t variable) {
    if (Dword(variable) != 0) {
        Cdecl(Free, Dword(variable));
    }
    Dword(variable) = 0;
}

// 0x8F7A70 (exit)
void __cdecl Fn8F7A70() {
    FreeAndClear(0x00BC5D08);
}

// 0x8F7A90 (exit)
void __cdecl Fn8F7A90() {
    FreeAndClear(0x00BC5D14);
}

// 0x8F7AB0 (exit)
void __cdecl Fn8F7AB0() {
    FreeAndClear(0x00BC5D20);
}

// 0x8F7AD0 (exit)
void __cdecl Fn8F7AD0() {
    FreeAndClear(0x00BC5D2C);
}

// 0x8F7AF0 (exit)
void __cdecl Fn8F7AF0() {
    FreeAndClear(0x00BC5D38);
}

// an array of NiFixedStrings released from the end
void ReleaseFixedStrings(uintptr_t end, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        end -= 4;
        ReleaseFixedString(end);
    }
}

// 0x8F8640 (exit)
void __cdecl Fn8F8640() {
    ReleaseFixedStrings(0x020E23FC, 3);
}

// 0x8F8710 (exit)
void __cdecl Fn8F8710() {
    Cdecl(0x00752380, Dword(0x020E25F4));   // NiFree
}

// 0x8F8790 (exit)
void __cdecl Fn8F8790() {
    ReleaseFixedStrings(0x020E291C, 10);
}

// 0x8F87E0 (exit)
void __cdecl Fn8F87E0() {
    ReleaseFixedStrings(0x020E2ABC, 4);
}

// 0x8F8A00 (exit)
void __cdecl Fn8F8A00() {
    ReleaseFixedStrings(0x020E3204, 14);
}

// 0x8F8BF0 (exit)
void __cdecl Fn8F8BF0() {
    uint32_t p = Dword(0x00BC8B78);
    Dword(0x00BC8B74) = 0x009893C0;   // vtable
    Cdecl(0x00836F80, p);
}

// 0x8FB980 (exit): the TLS slot (or similar handle) let go through its import
void __cdecl Fn8FB980() {
    uint32_t handle = Dword(0x020E93AC);
    if (handle != 0xFFFFFFFF) {
        Stdcall(Dword(0x008FC1A4), handle);
    }
}

// ---- strings hashed (HashName) into globals

// 0x8DC2A0 (initializer)
void __cdecl Fn8DC2A0() {
    Dword(0x00C2C0D0) = Cdecl(0x00576ED0, 0x00911CF8u /* "N_Angie" */);  // HashName
    Dword(0x00C2C0D4) = Cdecl(0x00576ED0, 0x00911CECu /* "N_Beatrice" */);  // HashName
    Dword(0x00C2C0D8) = Cdecl(0x00576ED0, 0x00911CE0u /* "N_Christy" */);  // HashName
    Dword(0x00C2C0DC) = Cdecl(0x00576ED0, 0x00911CD4u /* "N_Eunice" */);  // HashName
    Dword(0x00C2C0E0) = Cdecl(0x00576ED0, 0x00911CCCu /* "N_Lola" */);  // HashName
    Dword(0x00C2C0E4) = Cdecl(0x00576ED0, 0x00911CC4u /* "N_Mandy" */);  // HashName
    Dword(0x00C2C0E8) = Cdecl(0x00576ED0, 0x00911CBCu /* "N_Pinky" */);  // HashName
    Dword(0x00C2C0EC) = Cdecl(0x00576ED0, 0x00911CB4u /* "N_Zoe" */);  // HashName
    Dword(0x00C2C0F0) = Cdecl(0x00576ED0, 0x00911CA8u /* "N_Cornelius" */);  // HashName
    Dword(0x00C2C0F4) = Cdecl(0x00576ED0, 0x00911CA0u /* "N_Kirby" */);  // HashName
    Dword(0x00C2C0F8) = Cdecl(0x00576ED0, 0x00911C94u /* "N_Duncan" */);  // HashName
    Dword(0x00C2C0FC) = Cdecl(0x00576ED0, 0x00911C8Cu /* "N_Gord" */);  // HashName
    Dword(0x00C2C100) = Cdecl(0x00576ED0, 0x00911C84u /* "N_Vance" */);  // HashName
    Dword(0x00C2C104) = Cdecl(0x00576ED0, 0x00911C7Cu /* "N_Trent" */);  // HashName
}

// 0x8E8C50 (initializer)
void __cdecl Fn8E8C50() {
    Dword(0x020C3048) = Cdecl(0x00576ED0, 0x0093F678u /* "FEM_ON" */);  // HashName
    Dword(0x020C304C) = Cdecl(0x00576ED0, 0x0093F680u /* "FEM_OFF" */);  // HashName
}

// 0x8E8C80 (initializer)
void __cdecl Fn8E8C80() {
    Dword(0x020C3050) = Cdecl(0x00576ED0, 0x0093FF20u /* "AUDM" */);  // HashName
    Dword(0x020C3054) = Cdecl(0x00576ED0, 0x0093FF18u /* "AUDS" */);  // HashName
    Dword(0x020C3058) = Cdecl(0x00576ED0, 0x0093FF10u /* "AUDSUR" */);  // HashName
}

// 0x8E8CC0 (initializer)
void __cdecl Fn8E8CC0() {
    Dword(0x020C305C) = Cdecl(0x00576ED0, 0x0093FF30u /* "DISP_FS" */);  // HashName
    Dword(0x020C3060) = Cdecl(0x00576ED0, 0x0093FF28u /* "DISP_WS" */);  // HashName
}

// 0x8E8CF0 (initializer)
void __cdecl Fn8E8CF0() {
    Dword(0x020C3064) = Cdecl(0x00576ED0, 0x0093FF68u /* "FEL_ENG" */);  // HashName
    Dword(0x020C3068) = Cdecl(0x00576ED0, 0x0093FF60u /* "FEL_FRE" */);  // HashName
    Dword(0x020C306C) = Cdecl(0x00576ED0, 0x0093FF58u /* "FEL_GER" */);  // HashName
    Dword(0x020C3070) = Cdecl(0x00576ED0, 0x0093FF50u /* "FEL_ITA" */);  // HashName
    Dword(0x020C3074) = Cdecl(0x00576ED0, 0x0093FF48u /* "FEL_SPA" */);  // HashName
    Dword(0x020C3078) = Cdecl(0x00576ED0, 0x0093FF40u /* "FEL_BRI" */);  // HashName
    Dword(0x020C307C) = Cdecl(0x00576ED0, 0x0093FF38u /* "FEL_RUS" */);  // HashName
    Dword(0x020C3080) = 0;
}

// 0x8E8E70 (initializer)
void __cdecl Fn8E8E70() {
    Dword(0x020C30B8) = Cdecl(0x00576ED0, 0x00940DC0u /* "N_Algernon" */);  // HashName
    Dword(0x020C30BC) = Cdecl(0x00576ED0, 0x00911CF8u /* "N_Angie" */);  // HashName
    Dword(0x020C30C0) = Cdecl(0x00576ED0, 0x00911CECu /* "N_Beatrice" */);  // HashName
    Dword(0x020C30C4) = Cdecl(0x00576ED0, 0x00940DB8u /* "N_Bif" */);  // HashName
    Dword(0x020C30C8) = Cdecl(0x00576ED0, 0x00940DB0u /* "N_Bo" */);  // HashName
    Dword(0x020C30CC) = Cdecl(0x00576ED0, 0x00940DA8u /* "N_Bryce" */);  // HashName
    Dword(0x020C30D0) = Cdecl(0x00576ED0, 0x00940DA0u /* "N_Bucky" */);  // HashName
    Dword(0x020C30D4) = Cdecl(0x00576ED0, 0x00940D98u /* "N_Casey" */);  // HashName
    Dword(0x020C30D8) = Cdecl(0x00576ED0, 0x00940D90u /* "N_Chad" */);  // HashName
    Dword(0x020C30DC) = Cdecl(0x00576ED0, 0x00911CE0u /* "N_Christy" */);  // HashName
    Dword(0x020C30E0) = Cdecl(0x00576ED0, 0x00940BD8u /* "N_Constantinos" */);  // HashName
    Dword(0x020C30E4) = Cdecl(0x00576ED0, 0x00911CA8u /* "N_Cornelius" */);  // HashName
    Dword(0x020C30E8) = Cdecl(0x00576ED0, 0x00940D88u /* "N_Damon" */);  // HashName
    Dword(0x020C30EC) = Cdecl(0x00576ED0, 0x00940D80u /* "N_Dan" */);  // HashName
    Dword(0x020C30F0) = Cdecl(0x00576ED0, 0x00940D78u /* "N_Darby" */);  // HashName
    Dword(0x020C30F4) = Cdecl(0x00576ED0, 0x00940D70u /* "N_Davis" */);  // HashName
    Dword(0x020C30F8) = Cdecl(0x00576ED0, 0x00940D64u /* "N_Donald" */);  // HashName
    Dword(0x020C30FC) = Cdecl(0x00576ED0, 0x00940D58u /* "N_Earnest" */);  // HashName
    Dword(0x020C3100) = Cdecl(0x00576ED0, 0x00940D50u /* "N_Ethan" */);  // HashName
    Dword(0x020C3104) = Cdecl(0x00576ED0, 0x00911CD4u /* "N_Eunice" */);  // HashName
    Dword(0x020C3108) = Cdecl(0x00576ED0, 0x00940D48u /* "N_Fatty" */);  // HashName
    Dword(0x020C310C) = Cdecl(0x00576ED0, 0x00940D40u /* "N_Gary" */);  // HashName
    Dword(0x020C3110) = Cdecl(0x00576ED0, 0x00940D34u /* "N_Gloria" */);  // HashName
    Dword(0x020C3114) = Cdecl(0x00576ED0, 0x00911C8Cu /* "N_Gord" */);  // HashName
    Dword(0x020C3118) = Cdecl(0x00576ED0, 0x00940D28u /* "N_Gordon" */);  // HashName
    Dword(0x020C311C) = Cdecl(0x00576ED0, 0x00940D20u /* "N_Hal" */);  // HashName
    Dword(0x020C3120) = Cdecl(0x00576ED0, 0x00940D18u /* "N_Ivan" */);  // HashName
    Dword(0x020C3124) = Cdecl(0x00576ED0, 0x00940D10u /* "N_Jimmy" */);  // HashName
    Dword(0x020C3128) = Cdecl(0x00576ED0, 0x00940D04u /* "N_Johnny" */);  // HashName
    Dword(0x020C312C) = Cdecl(0x00576ED0, 0x00940CFCu /* "N_Juri" */);  // HashName
    Dword(0x020C3130) = Cdecl(0x00576ED0, 0x00940CF0u /* "N_Justin" */);  // HashName
    Dword(0x020C3134) = Cdecl(0x00576ED0, 0x00940CE8u /* "N_Karen" */);  // HashName
    Dword(0x020C3138) = Cdecl(0x00576ED0, 0x00911CA0u /* "N_Kirby" */);  // HashName
    Dword(0x020C313C) = Cdecl(0x00576ED0, 0x00940CE0u /* "N_Lance" */);  // HashName
    Dword(0x020C3140) = Cdecl(0x00576ED0, 0x00940CD8u /* "N_Lefty" */);  // HashName
    Dword(0x020C3144) = Cdecl(0x00576ED0, 0x00911CCCu /* "N_Lola" */);  // HashName
    Dword(0x020C3148) = Cdecl(0x00576ED0, 0x00940CD0u /* "N_Lucky" */);  // HashName
    Dword(0x020C314C) = Cdecl(0x00576ED0, 0x00940CC8u /* "N_Luis" */);  // HashName
    Dword(0x020C3150) = Cdecl(0x00576ED0, 0x00911CC4u /* "N_Mandy" */);  // HashName
    Dword(0x020C3154) = Cdecl(0x00576ED0, 0x00940CBCu /* "N_Melody" */);  // HashName
    Dword(0x020C3158) = Cdecl(0x00576ED0, 0x00940CB0u /* "N_Melvin" */);  // HashName
    Dword(0x020C315C) = Cdecl(0x00576ED0, 0x00940CA4u /* "N_Norton" */);  // HashName
    Dword(0x020C3160) = Cdecl(0x00576ED0, 0x00940C98u /* "N_Parker" */);  // HashName
    Dword(0x020C3164) = Cdecl(0x00576ED0, 0x00940C8Cu /* "N_Peanut" */);  // HashName
    Dword(0x020C3168) = Cdecl(0x00576ED0, 0x00940C84u /* "N_Pedro" */);  // HashName
    Dword(0x020C316C) = Cdecl(0x00576ED0, 0x00940C7Cu /* "N_Peter" */);  // HashName
    Dword(0x020C3170) = Cdecl(0x00576ED0, 0x00911CBCu /* "N_Pinky" */);  // HashName
    Dword(0x020C3174) = Cdecl(0x00576ED0, 0x00940C74u /* "N_Ray" */);  // HashName
    Dword(0x020C3178) = Cdecl(0x00576ED0, 0x00940C6Cu /* "N_Ricky" */);  // HashName
    Dword(0x020C317C) = Cdecl(0x00576ED0, 0x00940C60u /* "N_Russell" */);  // HashName
    Dword(0x020C3180) = Cdecl(0x00576ED0, 0x00940C54u /* "N_Sheldon" */);  // HashName
    Dword(0x020C3184) = Cdecl(0x00576ED0, 0x00940C4Cu /* "N_Tad" */);  // HashName
    Dword(0x020C3188) = Cdecl(0x00576ED0, 0x00940C44u /* "N_Ted" */);  // HashName
    Dword(0x020C318C) = Cdecl(0x00576ED0, 0x00940C3Cu /* "N_Thad" */);  // HashName
    Dword(0x020C3190) = Cdecl(0x00576ED0, 0x00940C34u /* "N_Tom" */);  // HashName
    Dword(0x020C3194) = Cdecl(0x00576ED0, 0x00911C7Cu /* "N_Trent" */);  // HashName
    Dword(0x020C3198) = Cdecl(0x00576ED0, 0x00940C28u /* "N_Trevor" */);  // HashName
    Dword(0x020C319C) = Cdecl(0x00576ED0, 0x00940C20u /* "N_Troy" */);  // HashName
    Dword(0x020C31A0) = Cdecl(0x00576ED0, 0x00911C84u /* "N_Vance" */);  // HashName
    Dword(0x020C31A4) = Cdecl(0x00576ED0, 0x00940C18u /* "N_Wade" */);  // HashName
    Dword(0x020C31A8) = Cdecl(0x00576ED0, 0x00911CB4u /* "N_Zoe" */);  // HashName
}

} // namespace
} // namespace staticinit

#define STATICINIT8_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)

STATICINIT8_REVERSED("StaticInit::8D9D80", 0x008D9D80, &staticinit::Fn8D9D80);
STATICINIT8_REVERSED("StaticInit::8D9E30", 0x008D9E30, &staticinit::Fn8D9E30);
STATICINIT8_REVERSED("StaticInit::8DA7A0", 0x008DA7A0, &staticinit::Fn8DA7A0);
STATICINIT8_REVERSED("StaticInit::8DABE0", 0x008DABE0, &staticinit::Fn8DABE0);
STATICINIT8_REVERSED("StaticInit::8DACB0", 0x008DACB0, &staticinit::Fn8DACB0);
STATICINIT8_REVERSED("StaticInit::8DAF90", 0x008DAF90, &staticinit::Fn8DAF90);
STATICINIT8_REVERSED("StaticInit::8DB100", 0x008DB100, &staticinit::Fn8DB100);
STATICINIT8_REVERSED("StaticInit::8DB330", 0x008DB330, &staticinit::Fn8DB330);
STATICINIT8_REVERSED("StaticInit::8DB950", 0x008DB950, &staticinit::Fn8DB950);
STATICINIT8_REVERSED("StaticInit::8DBA80", 0x008DBA80, &staticinit::Fn8DBA80);
STATICINIT8_REVERSED("StaticInit::8DBAD0", 0x008DBAD0, &staticinit::Fn8DBAD0);
STATICINIT8_REVERSED("StaticInit::8DBB40", 0x008DBB40, &staticinit::Fn8DBB40);
STATICINIT8_REVERSED("StaticInit::8DBB90", 0x008DBB90, &staticinit::Fn8DBB90);
STATICINIT8_REVERSED("StaticInit::8DBE90", 0x008DBE90, &staticinit::Fn8DBE90);
STATICINIT8_REVERSED("StaticInit::8DBEC0", 0x008DBEC0, &staticinit::Fn8DBEC0);
STATICINIT8_REVERSED("StaticInit::8DBF40", 0x008DBF40, &staticinit::Fn8DBF40);
STATICINIT8_REVERSED("StaticInit::8DBF60", 0x008DBF60, &staticinit::Fn8DBF60);
STATICINIT8_REVERSED("StaticInit::8DBF80", 0x008DBF80, &staticinit::Fn8DBF80);
STATICINIT8_REVERSED("StaticInit::8DBFA0", 0x008DBFA0, &staticinit::Fn8DBFA0);
STATICINIT8_REVERSED("StaticInit::8DBFC0", 0x008DBFC0, &staticinit::Fn8DBFC0);
STATICINIT8_REVERSED("StaticInit::8DC100", 0x008DC100, &staticinit::Fn8DC100);
STATICINIT8_REVERSED("StaticInit::8DC480", 0x008DC480, &staticinit::Fn8DC480);
STATICINIT8_REVERSED("StaticInit::8DC680", 0x008DC680, &staticinit::Fn8DC680);
STATICINIT8_REVERSED("StaticInit::8DC840", 0x008DC840, &staticinit::Fn8DC840);
STATICINIT8_REVERSED("StaticInit::8DC8A0", 0x008DC8A0, &staticinit::Fn8DC8A0);
STATICINIT8_REVERSED("StaticInit::8DCBF0", 0x008DCBF0, &staticinit::Fn8DCBF0);
STATICINIT8_REVERSED("StaticInit::8DCD30", 0x008DCD30, &staticinit::Fn8DCD30);
STATICINIT8_REVERSED("StaticInit::8DCD70", 0x008DCD70, &staticinit::Fn8DCD70);
STATICINIT8_REVERSED("StaticInit::8DCF00", 0x008DCF00, &staticinit::Fn8DCF00);
STATICINIT8_REVERSED("StaticInit::8DD0F0", 0x008DD0F0, &staticinit::Fn8DD0F0);
STATICINIT8_REVERSED("StaticInit::8DD2A0", 0x008DD2A0, &staticinit::Fn8DD2A0);
STATICINIT8_REVERSED("StaticInit::8DD3C0", 0x008DD3C0, &staticinit::Fn8DD3C0);
STATICINIT8_REVERSED("StaticInit::8DD4A0", 0x008DD4A0, &staticinit::Fn8DD4A0);
STATICINIT8_REVERSED("StaticInit::8DD4C0", 0x008DD4C0, &staticinit::Fn8DD4C0);
STATICINIT8_REVERSED("StaticInit::8DD520", 0x008DD520, &staticinit::Fn8DD520);
STATICINIT8_REVERSED("StaticInit::8DD6C0", 0x008DD6C0, &staticinit::Fn8DD6C0);
STATICINIT8_REVERSED("StaticInit::8DD710", 0x008DD710, &staticinit::Fn8DD710);
STATICINIT8_REVERSED("StaticInit::8DD750", 0x008DD750, &staticinit::Fn8DD750);
STATICINIT8_REVERSED("StaticInit::8DD790", 0x008DD790, &staticinit::Fn8DD790);
STATICINIT8_REVERSED("StaticInit::8DD860", 0x008DD860, &staticinit::Fn8DD860);
STATICINIT8_REVERSED("StaticInit::8DD890", 0x008DD890, &staticinit::Fn8DD890);
STATICINIT8_REVERSED("StaticInit::8DD8C0", 0x008DD8C0, &staticinit::Fn8DD8C0);
STATICINIT8_REVERSED("StaticInit::8DD8F0", 0x008DD8F0, &staticinit::Fn8DD8F0);
STATICINIT8_REVERSED("StaticInit::8DD9B0", 0x008DD9B0, &staticinit::Fn8DD9B0);
STATICINIT8_REVERSED("StaticInit::8DD9D0", 0x008DD9D0, &staticinit::Fn8DD9D0);
STATICINIT8_REVERSED("StaticInit::8DD9F0", 0x008DD9F0, &staticinit::Fn8DD9F0);
STATICINIT8_REVERSED("StaticInit::8DDA10", 0x008DDA10, &staticinit::Fn8DDA10);
STATICINIT8_REVERSED("StaticInit::8DDA40", 0x008DDA40, &staticinit::Fn8DDA40);
STATICINIT8_REVERSED("StaticInit::8DDA80", 0x008DDA80, &staticinit::Fn8DDA80);
STATICINIT8_REVERSED("StaticInit::8DDAD0", 0x008DDAD0, &staticinit::Fn8DDAD0);
STATICINIT8_REVERSED("StaticInit::8DDB80", 0x008DDB80, &staticinit::Fn8DDB80);
STATICINIT8_REVERSED("StaticInit::8DE430", 0x008DE430, &staticinit::Fn8DE430);
STATICINIT8_REVERSED("StaticInit::8DE540", 0x008DE540, &staticinit::Fn8DE540);
STATICINIT8_REVERSED("StaticInit::8DE770", 0x008DE770, &staticinit::Fn8DE770);
STATICINIT8_REVERSED("StaticInit::8DE810", 0x008DE810, &staticinit::Fn8DE810);
STATICINIT8_REVERSED("StaticInit::8DE930", 0x008DE930, &staticinit::Fn8DE930);
STATICINIT8_REVERSED("StaticInit::8DE9F0", 0x008DE9F0, &staticinit::Fn8DE9F0);
STATICINIT8_REVERSED("StaticInit::8DEA20", 0x008DEA20, &staticinit::Fn8DEA20);
STATICINIT8_REVERSED("StaticInit::8DEB20", 0x008DEB20, &staticinit::Fn8DEB20);
STATICINIT8_REVERSED("StaticInit::8DEB50", 0x008DEB50, &staticinit::Fn8DEB50);
STATICINIT8_REVERSED("StaticInit::8DEBA0", 0x008DEBA0, &staticinit::Fn8DEBA0);
STATICINIT8_REVERSED("StaticInit::8DECE0", 0x008DECE0, &staticinit::Fn8DECE0);
STATICINIT8_REVERSED("StaticInit::8DEE90", 0x008DEE90, &staticinit::Fn8DEE90);
STATICINIT8_REVERSED("StaticInit::8DF110", 0x008DF110, &staticinit::Fn8DF110);
STATICINIT8_REVERSED("StaticInit::8DF150", 0x008DF150, &staticinit::Fn8DF150);
STATICINIT8_REVERSED("StaticInit::8DF230", 0x008DF230, &staticinit::Fn8DF230);
STATICINIT8_REVERSED("StaticInit::8DF290", 0x008DF290, &staticinit::Fn8DF290);
STATICINIT8_REVERSED("StaticInit::8DF460", 0x008DF460, &staticinit::Fn8DF460);
STATICINIT8_REVERSED("StaticInit::8DF490", 0x008DF490, &staticinit::Fn8DF490);
STATICINIT8_REVERSED("StaticInit::8DF4B0", 0x008DF4B0, &staticinit::Fn8DF4B0);
STATICINIT8_REVERSED("StaticInit::8E0DF0", 0x008E0DF0, &staticinit::Fn8E0DF0);
STATICINIT8_REVERSED("StaticInit::8E4FD0", 0x008E4FD0, &staticinit::Fn8E4FD0);
STATICINIT8_REVERSED("StaticInit::8E8960", 0x008E8960, &staticinit::Fn8E8960);
STATICINIT8_REVERSED("StaticInit::8E9920", 0x008E9920, &staticinit::Fn8E9920);
STATICINIT8_REVERSED("StaticInit::8E9B90", 0x008E9B90, &staticinit::Fn8E9B90);
STATICINIT8_REVERSED("StaticInit::8E9C80", 0x008E9C80, &staticinit::Fn8E9C80);
STATICINIT8_REVERSED("StaticInit::8EACF0", 0x008EACF0, &staticinit::Fn8EACF0);
STATICINIT8_REVERSED("StaticInit::8EAE60", 0x008EAE60, &staticinit::Fn8EAE60);
STATICINIT8_REVERSED("StaticInit::8EAF10", 0x008EAF10, &staticinit::Fn8EAF10);
STATICINIT8_REVERSED("StaticInit::8EBAD0", 0x008EBAD0, &staticinit::Fn8EBAD0);
STATICINIT8_REVERSED("StaticInit::8EBB80", 0x008EBB80, &staticinit::Fn8EBB80);
STATICINIT8_REVERSED("StaticInit::8EC230", 0x008EC230, &staticinit::Fn8EC230);
STATICINIT8_REVERSED("StaticInit::8EC250", 0x008EC250, &staticinit::Fn8EC250);
STATICINIT8_REVERSED("StaticInit::8EC6B0", 0x008EC6B0, &staticinit::Fn8EC6B0);
STATICINIT8_REVERSED("StaticInit::8ED040", 0x008ED040, &staticinit::Fn8ED040);
STATICINIT8_REVERSED("StaticInit::8ED060", 0x008ED060, &staticinit::Fn8ED060);
STATICINIT8_REVERSED("StaticInit::8ED090", 0x008ED090, &staticinit::Fn8ED090);
STATICINIT8_REVERSED("StaticInit::8ED0C0", 0x008ED0C0, &staticinit::Fn8ED0C0);
STATICINIT8_REVERSED("StaticInit::8ED8F0", 0x008ED8F0, &staticinit::Fn8ED8F0);
STATICINIT8_REVERSED("StaticInit::8ED940", 0x008ED940, &staticinit::Fn8ED940);
STATICINIT8_REVERSED("StaticInit::8EF2C0", 0x008EF2C0, &staticinit::Fn8EF2C0);
STATICINIT8_REVERSED("StaticExit::8F1CF0", 0x008F1CF0, &staticinit::Fn8F1CF0);
STATICINIT8_REVERSED("StaticExit::8F1D60", 0x008F1D60, &staticinit::Fn8F1D60);
STATICINIT8_REVERSED("StaticExit::8F1D80", 0x008F1D80, &staticinit::Fn8F1D80);
STATICINIT8_REVERSED("StaticExit::8F2260", 0x008F2260, &staticinit::Fn8F2260);
STATICINIT8_REVERSED("StaticExit::8F22E0", 0x008F22E0, &staticinit::Fn8F22E0);
STATICINIT8_REVERSED("StaticExit::8F2300", 0x008F2300, &staticinit::Fn8F2300);
STATICINIT8_REVERSED("StaticExit::8F2510", 0x008F2510, &staticinit::Fn8F2510);
STATICINIT8_REVERSED("StaticExit::8F2560", 0x008F2560, &staticinit::Fn8F2560);
STATICINIT8_REVERSED("StaticExit::8F25B0", 0x008F25B0, &staticinit::Fn8F25B0);
STATICINIT8_REVERSED("StaticExit::8F2600", 0x008F2600, &staticinit::Fn8F2600);
STATICINIT8_REVERSED("StaticExit::8F2620", 0x008F2620, &staticinit::Fn8F2620);
STATICINIT8_REVERSED("StaticExit::8F26A0", 0x008F26A0, &staticinit::Fn8F26A0);
STATICINIT8_REVERSED("StaticExit::8F26E0", 0x008F26E0, &staticinit::Fn8F26E0);
STATICINIT8_REVERSED("StaticExit::8F27B0", 0x008F27B0, &staticinit::Fn8F27B0);
STATICINIT8_REVERSED("StaticExit::8F2840", 0x008F2840, &staticinit::Fn8F2840);
STATICINIT8_REVERSED("StaticExit::8F2890", 0x008F2890, &staticinit::Fn8F2890);
STATICINIT8_REVERSED("StaticExit::8F2E80", 0x008F2E80, &staticinit::Fn8F2E80);
STATICINIT8_REVERSED("StaticExit::8F79F0", 0x008F79F0, &staticinit::Fn8F79F0);
STATICINIT8_REVERSED("StaticExit::8F7A70", 0x008F7A70, &staticinit::Fn8F7A70);
STATICINIT8_REVERSED("StaticExit::8F7A90", 0x008F7A90, &staticinit::Fn8F7A90);
STATICINIT8_REVERSED("StaticExit::8F7AB0", 0x008F7AB0, &staticinit::Fn8F7AB0);
STATICINIT8_REVERSED("StaticExit::8F7AD0", 0x008F7AD0, &staticinit::Fn8F7AD0);
STATICINIT8_REVERSED("StaticExit::8F7AF0", 0x008F7AF0, &staticinit::Fn8F7AF0);
STATICINIT8_REVERSED("StaticExit::8F8640", 0x008F8640, &staticinit::Fn8F8640);
STATICINIT8_REVERSED("StaticExit::8F8710", 0x008F8710, &staticinit::Fn8F8710);
STATICINIT8_REVERSED("StaticExit::8F8790", 0x008F8790, &staticinit::Fn8F8790);
STATICINIT8_REVERSED("StaticExit::8F87E0", 0x008F87E0, &staticinit::Fn8F87E0);
STATICINIT8_REVERSED("StaticExit::8F8A00", 0x008F8A00, &staticinit::Fn8F8A00);
STATICINIT8_REVERSED("StaticExit::8F8BF0", 0x008F8BF0, &staticinit::Fn8F8BF0);
STATICINIT8_REVERSED("StaticExit::8FB980", 0x008FB980, &staticinit::Fn8FB980);
STATICINIT8_REVERSED("StaticInit::8DC2A0", 0x008DC2A0, &staticinit::Fn8DC2A0);
STATICINIT8_REVERSED("StaticInit::8E8C50", 0x008E8C50, &staticinit::Fn8E8C50);
STATICINIT8_REVERSED("StaticInit::8E8C80", 0x008E8C80, &staticinit::Fn8E8C80);
STATICINIT8_REVERSED("StaticInit::8E8CC0", 0x008E8CC0, &staticinit::Fn8E8CC0);
STATICINIT8_REVERSED("StaticInit::8E8CF0", 0x008E8CF0, &staticinit::Fn8E8CF0);
STATICINIT8_REVERSED("StaticInit::8E8E70", 0x008E8E70, &staticinit::Fn8E8E70);
