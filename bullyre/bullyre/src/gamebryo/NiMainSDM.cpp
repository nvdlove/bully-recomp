// NiMainSDM: the main library's static data, made before and destroyed after the others (its
// Init and Shutdown are NiStaticDataManager's pre-init and post-shutdown callbacks, set by the
// static object's constructor): the global string table, the classes' statics (default
// properties, names, tables), NiStream's loaders by class name (unregistered at Shutdown), the
// render step names. With the pieces of it in this part of the library: NiStream's _SDMInit and
// post-process functions, NiShadowManager's _SDMInit / _SDMShutdown, NiMaterial's and
// NiMaterialLibrary's, NiShaderDeclaration's semantic names, NiShaderConstantMap's object mapping
// types, the default render list processor (NiViewRenderClick's), and NiTPointerAllocator's
// shutdown.

#include <cstddef>
#include <cstdint>

#include "game/Crt.h"
#include "gamebryo/NiCore.h"
#include "gamebryo/NiSystem.h"
#include "reversed.h"

namespace ni {
namespace {

using namespace ni::sys;

GAME_VAR(uint8_t, NiMainSDM_ms_bInitialized, 0x020E34B4);
GAME_VAR(int32_t, NiMainSDM_ms_iInitCount, 0x020E34B8);
GAME_VAR(NiRefObject*, NiViewRenderClick_ms_spDefaultProcessor, 0x020E2DF0);
GAME_VAR(void*, NiStream_ms_kLoaders, 0x020E247C);
GAME_VAR(NiTArray<void*>*, NiStream_ms_pkPostProcessFunctions, 0x020E2480);
GAME_ARRAY(const char*, NiShadowManager_ms_akMapNames, 0x020E23F0);   // [3]
GAME_VAR(void*, NiShadowManager_ms_pkRenderClickFactory, 0x020E23B8);
GAME_VAR(void*, NiMaterial_ms_pkMaterials, 0x020E2704);              // NiTFixedStringMap<NiMaterial*>
GAME_VAR(char, NiMaterialLibrary_ms_acDefaultWorkingDirectory, 0x020E2600);
GAME_VAR(uint32_t, NiMaterialLibrary_ms_uiLibraryCount, 0x020E25FC);
constexpr uintptr_t NiMaterialLibrary_ms_kLibraries = 0x020E25F4;   // NiTPrimitiveSet<NiMaterialLibrary*>
GAME_ARRAY(const char*, NiShaderDeclaration_ms_akSemanticNames, 0x020E31CC);   // [14]
GAME_ARRAY(uint32_t, NiShaderConstantMap_ms_akObjectMappingTypes, 0x020E3308);  // [30][3]
// NiTPointerAllocator's blocks (each new[]'d: the next block first) and free list
GAME_VAR(void*, NiTPointerAllocator_ms_pBlockList, 0x020E2454);
GAME_VAR(void*, NiTPointerAllocator_ms_pFreeList, 0x020E2458);
GAME_VAR(void*, NiRenderer_ms_pkRenderer, 0x020E2380);

constexpr uintptr_t NiRefObject_vtable = 0x008FEFD4;
constexpr uintptr_t NiRenderListProcessor_vtable = 0x0091786C;
constexpr uintptr_t RenderClickFactory_vtable = 0x008FF194;   // NiShadowRenderClickFactory
constexpr uintptr_t PointerMapBase_vtable = 0x00950760;
constexpr uintptr_t LoaderMap_vtable = 0x00951798;             // NiTStringPointerMap<CreateFunction>
constexpr uintptr_t FixedStringMapBase_vtable = 0x009A1978;
constexpr uintptr_t FixedStringMap_vtable = 0x009A162C;         // NiTFixedStringMap<NiMaterial*>
constexpr uintptr_t PrimitiveArray_vtable = 0x00961988;
constexpr uintptr_t NiMainSDM_Init_address = 0x007858F0;
constexpr uintptr_t NiMainSDM_Shutdown_address = 0x00785E20;

GAME_VAR(long, ms_uiObjects, 0x020E2168);

using CreateFn = void*(__cdecl*)();
GAME_FN(void, __cdecl, NiStream_RegisterLoader, 0x00759EC0, const char* name, CreateFn create);
GAME_FN(void, __cdecl, NiStream_UnregisterLoader, 0x00759EE0, const char* name);
GAME_FN(void, __cdecl, NiStaticDataManager_SetCallbacks, 0x00753660, void(__cdecl* preInit)(),
        void(__cdecl* postShutdown)());
GAME_FN(void, __thiscall, FixedString_AssignText, 0x00406730, re::Ptr32<const char>* self, const char* text);
GAME_FN(void, __thiscall, FixedString_Assign, 0x00401CD0, re::Ptr32<const char>* self, const re::Ptr32<const char>* other);
GAME_FN(const char*, __cdecl, StringTable_AddString, 0x00754760, const char* text);
GAME_FN(void, __thiscall, StringTable_RemoveString, 0x00754550, void* table, re::Ptr32<const char>* handle, uint32_t hash);
GAME_VAR(void*, GlobalStringTable, 0x020E216C);
GAME_VAR(const char*, NullFixedString, 0x00951028);
GAME_FN(void, __thiscall, PrimitiveArray_AddFirstEmpty, 0x008A4D20, NiTArray<void*>* self, const re::Ptr32<void>* value);
GAME_FN(void, __thiscall, LibrarySet_Realloc, 0x0083AD20, void* set, uint32_t allocated);
GAME_FN(void, __cdecl, NiDeleteArray_, 0x00752570, void* p, uint32_t size);

// an init or shutdown function of the library (cdecl, nothing in or out)
inline void Call(uintptr_t address) {
    reinterpret_cast<void(__cdecl*)()>(address)();
}
// the game's text (the class names stay the game's strings: the loader map doesn't copy keys)
inline const char* Text(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}
inline CreateFn Fn(uintptr_t address) {
    return reinterpret_cast<CreateFn>(address);
}

// a handle let go (NiFixedString's destructor, inlined)
inline void Release(re::Ptr32<const char>* handle) {
    const char* s = *handle;
    if (s != NullFixedString) {
        uint32_t hash = *reinterpret_cast<const uint16_t*>(s - 2);
        if (Decrement(const_cast<char*>(s) - 8) == 1) {
            StringTable_RemoveString(GlobalStringTable, handle, hash);
        }
    }
}

// a hash map's base (NiTMapBase: vtable, hash size, the buckets NiMalloc'd and zeroed, count)
inline void InitMap(void* map, uint32_t hashSize) {
    At<void*>(map, 0) = reinterpret_cast<void*>(PointerMapBase_vtable);
    At<uint32_t>(map, 4) = hashSize;
    At<uint32_t>(map, 0xC) = 0;
    void* table = NiMalloc(hashSize * 4);
    At<void*>(map, 8) = table;
    crt::Memset(table, 0, At<uint32_t>(map, 4) * 4);
}

} // namespace

// ---- NiMainSDM

// 0x7858F0: Init (once)
void __cdecl NiMainSDM_Init() {
    if (NiMainSDM_ms_bInitialized != 0) {
        return;
    }
    NiMainSDM_ms_bInitialized = 1;
    Call(0x00754870);   // NiGlobalStringTable::_SDMInit
    Call(0x0077F710);   // NiShaderConstantMap::FillPredefinedMappingTables
    Call(0x0076BF60);   // NiTextureEffect::_SDMInit
    Call(0x0076CFB0);   // NiAlphaProperty::_SDMInit
    Call(0x007DE2D0);   // NiDitherProperty::_SDMInit
    Call(0x007783C0);   // NiFogProperty::_SDMInit
    Call(0x00775950);   // NiMaterialProperty::_SDMInit
    Call(0x0049A650);   // (an empty function of the game)
    Call(0x007DC890);   // NiRendererSpecificProperty::_SDMInit
    Call(0x007786D0);   // NiShadeProperty::_SDMInit
    Call(0x007E29E0);   // NiStaticTables::Init
    Call(0x0077C630);   // NiShaderDeclaration::_SDMInit
    Call(0x007DA360);   // NiSpecularProperty::_SDMInit
    Call(0x00778BC0);   // NiStencilProperty::_SDMInit
    Call(0x00766C70);   // NiTexturingProperty::_SDMInit
    Call(0x00779670);   // NiVertexColorProperty::_SDMInit
    Call(0x007D7ED0);   // NiWireframeProperty::_SDMInit
    Call(0x0076D740);   // NiZBufferProperty::_SDMInit
    Call(0x007D3E90);   // NiImageConverter::_SDMInit
    Call(0x0075BF80);   // NiStream::_SDMInit
    NiStream_RegisterLoader(Text(0x00955100), Fn(0x007E18E0));   // "NiAdditionalGeometryData"
    NiStream_RegisterLoader(Text(0x009550EC), Fn(0x007E0960));   // "NiAlphaAccumulator"
    NiStream_RegisterLoader(Text(0x00952AC4), Fn(0x0076CEC0));   // "NiAlphaProperty"
    NiStream_RegisterLoader(Text(0x00954CF0), Fn(0x007841A0));   // "NiAmbientLight"
    NiStream_RegisterLoader(Text(0x009550D4), Fn(0x007E0830));   // "NiAutoNormalParticles"
    NiStream_RegisterLoader(Text(0x009550B8), Fn(0x007DFEF0));   // "NiAutoNormalParticlesData"
    NiStream_RegisterLoader(Text(0x009550A8), Fn(0x007DFB10));   // "NiBillboardNode"
    NiStream_RegisterLoader(Text(0x0095352C), Fn(0x00777E80));   // "NiBinaryExtraData"
    NiStream_RegisterLoader(Text(0x00952084), Fn(0x00765860));   // "NiBooleanExtraData"
    NiStream_RegisterLoader(Text(0x0095509C), Fn(0x007DEF20));   // "NiBSPNode"
    NiStream_RegisterLoader(Text(0x00951F04), Fn(0x00763D50));   // "NiCamera"
    NiStream_RegisterLoader(Text(0x00955084), Fn(0x007E0960));   // "NiClusterAccumulator"
    NiStream_RegisterLoader(Text(0x00955070), Fn(0x00760A60));   // "NiCollisionSwitch"
    NiStream_RegisterLoader(Text(0x0095254C), Fn(0x00769330));   // "NiColorExtraData"
    NiStream_RegisterLoader(Text(0x00955054), Fn(0x007DE970));   // "NiDefaultAVObjectPalette"
    NiStream_RegisterLoader(Text(0x00955040), Fn(0x007D55F0));   // "NiDirectionalLight"
    NiStream_RegisterLoader(Text(0x0095502C), Fn(0x007DE2A0));   // "NiDitherProperty"
    NiStream_RegisterLoader(Text(0x0095206C), Fn(0x007656F0));   // "NiExtraData"
    NiStream_RegisterLoader(Text(0x00952418), Fn(0x00768160));   // "NiFloatExtraData"
    NiStream_RegisterLoader(Text(0x009528CC), Fn(0x0076CB00));   // "NiFloatsExtraData"
    NiStream_RegisterLoader(Text(0x0095359C), Fn(0x00778220));   // "NiFogProperty"
    NiStream_RegisterLoader(Text(0x00952438), Fn(0x007682F0));   // "NiIntegerExtraData"
    NiStream_RegisterLoader(Text(0x00952BE8), Fn(0x0076DAE0));   // "NiIntegersExtraData"
    NiStream_RegisterLoader(Text(0x00955020), Fn(0x007DE120));   // "NiLODNode"
    NiStream_RegisterLoader(Text(0x00953968), Fn(0x007799A0));   // "NiLines"
    NiStream_RegisterLoader(Text(0x009539D4), Fn(0x00779C80));   // "NiLinesData"
    NiStream_RegisterLoader(Text(0x00953138), Fn(0x00775750));   // "NiMaterialProperty"
    NiStream_RegisterLoader(Text(0x00951CAC), Fn(0x00760A60));   // "NiNode"
    NiStream_RegisterLoader(Text(0x00955014), Fn(0x007D44C0));   // "NiPalette"
    NiStream_RegisterLoader(Text(0x00955008), Fn(0x007E0830));   // "NiParticles"
    NiStream_RegisterLoader(Text(0x00954FF8), Fn(0x007DFEF0));   // "NiParticlesData"
    NiStream_RegisterLoader(Text(0x008FF288), Fn(0x007DD580));   // "NiPersistentSrcTextureRendererData"
    NiStream_RegisterLoader(Text(0x0095265C), Fn(0x0076A880));   // "NiPixelData"
    NiStream_RegisterLoader(Text(0x0095309C), Fn(0x00775210));   // "NiPointLight"
    NiStream_RegisterLoader(Text(0x00954FE8), Fn(0x007DCE60));   // "NiRangeLODData"
    NiStream_RegisterLoader(Text(0x00954FCC), Fn(0x007DC830));   // "NiRendererSpecificProperty"
    NiStream_RegisterLoader(Text(0x00954FBC), Fn(0x007DC2A0));   // "NiScreenLODData"
    NiStream_RegisterLoader(Text(0x00954FA8), Fn(0x007AB320));   // "NiScreenElements"
    NiStream_RegisterLoader(Text(0x00954F90), Fn(0x00788850));   // "NiScreenElementsData"
    NiStream_RegisterLoader(Text(0x00954F80), Fn(0x007DBDC0));   // "NiScreenTexture"
    NiStream_RegisterLoader(Text(0x009535B8), Fn(0x007786A0));   // "NiShadeProperty"
    NiStream_RegisterLoader(Text(0x00954F74), Fn(0x007DB2E0));   // "NiSkinData"
    NiStream_RegisterLoader(Text(0x00953288), Fn(0x00777220));   // "NiSkinInstance"
    NiStream_RegisterLoader(Text(0x00952C5C), Fn(0x0076E380));   // "NiSkinPartition"
    NiStream_RegisterLoader(Text(0x00954F60), Fn(0x007DAE80));   // "NiSortAdjustNode"
    NiStream_RegisterLoader(Text(0x009525BC), Fn(0x00769AD0));   // "NiSourceTexture"
    NiStream_RegisterLoader(Text(0x00954F50), Fn(0x007DABD0));   // "NiSourceCubeMap"
    NiStream_RegisterLoader(Text(0x00954F3C), Fn(0x007DA2F0));   // "NiSpecularProperty"
    NiStream_RegisterLoader(Text(0x00952FE0), Fn(0x00774E90));   // "NiSpotLight"
    NiStream_RegisterLoader(Text(0x00953704), Fn(0x007788C0));   // "NiStencilProperty"
    NiStream_RegisterLoader(Text(0x009524AC), Fn(0x007684D0));   // "NiStringExtraData"
    NiStream_RegisterLoader(Text(0x00954F28), Fn(0x007D9EC0));   // "NiStringsExtraData"
    NiStream_RegisterLoader(Text(0x00954F18), Fn(0x007D9CB0));   // "NiSwitchNode"
    NiStream_RegisterLoader(Text(0x00954F00), Fn(0x007D8F20));   // "NiSwitchStringExtraData"
    NiStream_RegisterLoader(Text(0x00952850), Fn(0x0076BE00));   // "NiTextureEffect"
    NiStream_RegisterLoader(Text(0x009523F8), Fn(0x00766180));   // "NiTexturingProperty"
    NiStream_RegisterLoader(Text(0x00953BBC), Fn(0x0077B8B0));   // "NiTriShape"
    NiStream_RegisterLoader(Text(0x00953C54), Fn(0x0077BB10));   // "NiTriShapeData"
    NiStream_RegisterLoader(Text(0x00954EE8), Fn(0x007D8BD0));   // "NiTriShapeDynamicData"
    NiStream_RegisterLoader(Text(0x00954EDC), Fn(0x007D8A00));   // "NiTriStrips"
    NiStream_RegisterLoader(Text(0x00954ECC), Fn(0x007D8360));   // "NiTriStripsData"
    NiStream_RegisterLoader(Text(0x00954EB8), Fn(0x007D8010));   // "NiVectorExtraData"
    NiStream_RegisterLoader(Text(0x009538B8), Fn(0x00779530));   // "NiVertexColorProperty"
    NiStream_RegisterLoader(Text(0x00954EA0), Fn(0x007D3940));   // "NiVertWeightsExtraData"
    NiStream_RegisterLoader(Text(0x00954E8C), Fn(0x007D7EA0));   // "NiWireframeProperty"
    NiStream_RegisterLoader(Text(0x00952B70), Fn(0x0076D660));   // "NiZBufferProperty"
    NiStream_RegisterLoader(Text(0x00954E78), Fn(0x007D7D30));   // "NiScreenSpaceCamera"
    NiStream_RegisterLoader(Text(0x00954E64), Fn(0x007D6230));   // "NiScreenGeometry"
    NiStream_RegisterLoader(Text(0x00954E4C), Fn(0x007D70A0));   // "NiScreenGeometryData"
    NiStream_RegisterLoader(Text(0x00954E3C), Fn(0x007D6A70));   // "NiScreenPolygon"
    Call(0x007D7230);   // NiScreenGeometryData::_SDMInit
    Call(0x007548A0);   // NiTexture::ResetReplacementFormat
    Call(0x00770AE0);   // NiMath::_SDMInit
    Call(0x0049A650);   // (an empty function of the game)
    Call(0x007D61C0);   // NiSingleShaderMaterialLibrary::_SDMInit
    Call(0x00762C60);   // NiMaterial::_SDMInit
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E2DB8), Text(0x00954E1C));   // "Render Click"
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E2810), Text(0x00954E2C));   // "Render Frame"
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E393C), Text(0x00954E10));   // "Render Step"
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E35C4), Text(0x00954E04));   // "Render View"
    Call(0x00785870);   // NiViewRenderClick::_SDMInit
    Call(0x00756070);   // NiShadowManager::_SDMInit
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E25B0), Text(0x00951694));   // "ShadowCubeMap"
    Call(0x007D5F00);   // NiProcessorSpecificCode::Initialize, a tail jump
}

// 0x785E20: Shutdown (once)
void __cdecl NiMainSDM_Shutdown() {
    if (NiMainSDM_ms_bInitialized == 0) {
        return;
    }
    NiMainSDM_ms_bInitialized = 0;
    NiStream_UnregisterLoader(Text(0x00955100));   // "NiAdditionalGeometryData"
    NiStream_UnregisterLoader(Text(0x009550EC));   // "NiAlphaAccumulator"
    NiStream_UnregisterLoader(Text(0x00952AC4));   // "NiAlphaProperty"
    NiStream_UnregisterLoader(Text(0x00954CF0));   // "NiAmbientLight"
    NiStream_UnregisterLoader(Text(0x009550D4));   // "NiAutoNormalParticles"
    NiStream_UnregisterLoader(Text(0x009550B8));   // "NiAutoNormalParticlesData"
    NiStream_UnregisterLoader(Text(0x009550A8));   // "NiBillboardNode"
    NiStream_UnregisterLoader(Text(0x0095352C));   // "NiBinaryExtraData"
    NiStream_UnregisterLoader(Text(0x00952084));   // "NiBooleanExtraData"
    NiStream_UnregisterLoader(Text(0x0095509C));   // "NiBSPNode"
    NiStream_UnregisterLoader(Text(0x00951F04));   // "NiCamera"
    NiStream_UnregisterLoader(Text(0x00955084));   // "NiClusterAccumulator"
    NiStream_UnregisterLoader(Text(0x00955070));   // "NiCollisionSwitch"
    NiStream_UnregisterLoader(Text(0x0095254C));   // "NiColorExtraData"
    NiStream_UnregisterLoader(Text(0x00955054));   // "NiDefaultAVObjectPalette"
    NiStream_UnregisterLoader(Text(0x00955040));   // "NiDirectionalLight"
    NiStream_UnregisterLoader(Text(0x0095502C));   // "NiDitherProperty"
    NiStream_UnregisterLoader(Text(0x0095206C));   // "NiExtraData"
    NiStream_UnregisterLoader(Text(0x00952418));   // "NiFloatExtraData"
    NiStream_UnregisterLoader(Text(0x009528CC));   // "NiFloatsExtraData"
    NiStream_UnregisterLoader(Text(0x0095359C));   // "NiFogProperty"
    NiStream_UnregisterLoader(Text(0x00952438));   // "NiIntegerExtraData"
    NiStream_UnregisterLoader(Text(0x00952BE8));   // "NiIntegersExtraData"
    NiStream_UnregisterLoader(Text(0x00955020));   // "NiLODNode"
    NiStream_UnregisterLoader(Text(0x00953968));   // "NiLines"
    NiStream_UnregisterLoader(Text(0x009539D4));   // "NiLinesData"
    NiStream_UnregisterLoader(Text(0x00953138));   // "NiMaterialProperty"
    NiStream_UnregisterLoader(Text(0x00951CAC));   // "NiNode"
    NiStream_UnregisterLoader(Text(0x00955014));   // "NiPalette"
    NiStream_UnregisterLoader(Text(0x00955008));   // "NiParticles"
    NiStream_UnregisterLoader(Text(0x00954FF8));   // "NiParticlesData"
    NiStream_UnregisterLoader(Text(0x008FF288));   // "NiPersistentSrcTextureRendererData"
    NiStream_UnregisterLoader(Text(0x0095265C));   // "NiPixelData"
    NiStream_UnregisterLoader(Text(0x0095309C));   // "NiPointLight"
    NiStream_UnregisterLoader(Text(0x00954FE8));   // "NiRangeLODData"
    NiStream_UnregisterLoader(Text(0x00954FCC));   // "NiRendererSpecificProperty"
    NiStream_UnregisterLoader(Text(0x00954FBC));   // "NiScreenLODData"
    NiStream_UnregisterLoader(Text(0x00954F80));   // "NiScreenTexture"
    NiStream_UnregisterLoader(Text(0x009535B8));   // "NiShadeProperty"
    NiStream_UnregisterLoader(Text(0x00954F74));   // "NiSkinData"
    NiStream_UnregisterLoader(Text(0x00953288));   // "NiSkinInstance"
    NiStream_UnregisterLoader(Text(0x00952C5C));   // "NiSkinPartition"
    NiStream_UnregisterLoader(Text(0x00954F60));   // "NiSortAdjustNode"
    NiStream_UnregisterLoader(Text(0x00954FA8));   // "NiScreenElements"
    NiStream_UnregisterLoader(Text(0x00954F90));   // "NiScreenElementsData"
    NiStream_UnregisterLoader(Text(0x009525BC));   // "NiSourceTexture"
    NiStream_UnregisterLoader(Text(0x00954F3C));   // "NiSpecularProperty"
    NiStream_UnregisterLoader(Text(0x00952FE0));   // "NiSpotLight"
    NiStream_UnregisterLoader(Text(0x00953704));   // "NiStencilProperty"
    NiStream_UnregisterLoader(Text(0x009524AC));   // "NiStringExtraData"
    NiStream_UnregisterLoader(Text(0x00954F28));   // "NiStringsExtraData"
    NiStream_UnregisterLoader(Text(0x00954F18));   // "NiSwitchNode"
    NiStream_UnregisterLoader(Text(0x00954F00));   // "NiSwitchStringExtraData"
    NiStream_UnregisterLoader(Text(0x00952850));   // "NiTextureEffect"
    NiStream_UnregisterLoader(Text(0x009523F8));   // "NiTexturingProperty"
    NiStream_UnregisterLoader(Text(0x00953BBC));   // "NiTriShape"
    NiStream_UnregisterLoader(Text(0x00953C54));   // "NiTriShapeData"
    NiStream_UnregisterLoader(Text(0x00954EE8));   // "NiTriShapeDynamicData"
    NiStream_UnregisterLoader(Text(0x00954EDC));   // "NiTriStrips"
    NiStream_UnregisterLoader(Text(0x00954ECC));   // "NiTriStripsData"
    NiStream_UnregisterLoader(Text(0x00954EB8));   // "NiVectorExtraData"
    NiStream_UnregisterLoader(Text(0x009538B8));   // "NiVertexColorProperty"
    NiStream_UnregisterLoader(Text(0x00954EA0));   // "NiVertWeightsExtraData"
    NiStream_UnregisterLoader(Text(0x00954E8C));   // "NiWireframeProperty"
    NiStream_UnregisterLoader(Text(0x00952B70));   // "NiZBufferProperty"
    NiStream_UnregisterLoader(Text(0x00954E64));   // "NiScreenGeometry"
    NiStream_UnregisterLoader(Text(0x00954E4C));   // "NiScreenGeometryData"
    NiStream_UnregisterLoader(Text(0x00954E3C));   // "NiScreenPolygon"
    NiStream_UnregisterLoader(Text(0x00954E78));   // "NiScreenSpaceCamera"
    Call(0x00784140);   // NiShaderConstantMap's predefined mapping tables deleted
    Call(0x0076BFA0);   // NiTextureEffect::_SDMShutdown
    Call(0x0076D020);   // NiAlphaProperty::_SDMShutdown
    Call(0x007DE340);   // NiDitherProperty::_SDMShutdown
    Call(0x00778450);   // NiFogProperty::_SDMShutdown
    Call(0x007759B0);   // NiMaterialProperty::_SDMShutdown
    Call(0x007E29F0);   // NiStaticTables::Shutdown (the pointer array at 0xBC8B74 emptied)
    Call(0x007DC8F0);   // NiRendererSpecificProperty::_SDMShutdown
    Call(0x00778740);   // NiShadeProperty::_SDMShutdown
    Call(0x0077CA70);   // NiShaderDeclaration::_SDMShutdown
    Call(0x007DA3D0);   // NiSpecularProperty::_SDMShutdown
    Call(0x00778C40);   // NiStencilProperty::_SDMShutdown
    Call(0x00766D60);   // NiTexturingProperty::_SDMShutdown
    Call(0x007796E0);   // NiVertexColorProperty::_SDMShutdown
    Call(0x007D7F40);   // NiWireframeProperty::_SDMShutdown
    Call(0x0076D7C0);   // NiZBufferProperty::_SDMShutdown
    Call(0x007D3F10);   // NiImageConverter::_SDMShutdown
    Call(0x007599C0);   // NiStream::_SDMShutdown
    Call(0x00755780);   // NiRenderer::_SDMShutdown
    Call(0x007D78A0);   // NiScreenGeometryData::_SDMShutdown
    Call(0x00754A00);   // NiTexture::_SDMShutdown
    Call(0x00770AF0);   // NiMath::_SDMShutdown
    Call(0x007629E0);   // NiMaterial::_SDMShutdown
    Call(0x007D60E0);   // NiSingleShaderMaterialLibrary::_SDMShutdown
    Call(0x00762960);   // NiMaterialLibrary::_SDMShutdown
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E2DB8), nullptr);
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E2810), nullptr);
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E393C), nullptr);
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E35C4), nullptr);
    NiRefObject* processor = NiViewRenderClick_ms_spDefaultProcessor;   // the default processor let go
    if (processor != nullptr) {
        if (Decrement(&processor->refCount) == 0) {
            Virtual<void>(processor, 1);
        }
        NiViewRenderClick_ms_spDefaultProcessor = nullptr;
    }
    Call(0x007560D0);   // NiShadowManager::_SDMShutdown
    FixedString_AssignText(reinterpret_cast<re::Ptr32<const char>*>(0x020E25B0), nullptr);
    Call(0x00759980);   // NiTPointerAllocator::_SDMShutdown
    Call(0x007546C0);   // NiGlobalStringTable::_SDMShutdown, a tail jump
}

// 0x7861F0: the static object's constructor: Init and Shutdown set as the static data
// manager's callbacks by the first one
void* __fastcall NiMainSDM_Construct(void* self, re::Edx) {
    int32_t count = NiMainSDM_ms_iInitCount;
    NiMainSDM_ms_iInitCount = count + 1;
    if (count == 0) {
        NiStaticDataManager_SetCallbacks(reinterpret_cast<void(__cdecl*)()>(NiMainSDM_Init_address),
                                         reinterpret_cast<void(__cdecl*)()>(NiMainSDM_Shutdown_address));
    }
    return self;
}

// ---- the render list processor

// 0x777D10: NiRenderListProcessor::PreRenderProcessList (slot 3): the default renders the
// geometry at once (slot 34 RenderImmediate), nothing to the output
void __fastcall NiRenderListProcessor_PreRenderProcessList(void*, re::Edx, const void* input, void*, void*) {
    if (input == nullptr) {
        return;
    }
    void* renderer = NiRenderer_ms_pkRenderer;
    void* const* geometry = At<void* const*>(input, 0);
    uint32_t count = At<uint32_t>(input, 4);
    for (uint32_t i = 0; i < count; i++) {
        Virtual<void>(geometry[i], 34, renderer);
    }
}

// 0x785870: NiViewRenderClick::_SDMInit: the default processor (a plain NiRenderListProcessor)
void __cdecl NiViewRenderClick_SDMInit() {
    NiRefObject* processor = static_cast<NiRefObject*>(NiNew(8));
    if (processor != nullptr) {
        processor->vtable = reinterpret_cast<void*>(NiRefObject_vtable);
        processor->refCount = 0;
        Increment(&ms_uiObjects);
        processor->vtable = reinterpret_cast<void*>(NiRenderListProcessor_vtable);
    } else {
        processor = nullptr;
    }
    NiRefObject* old = NiViewRenderClick_ms_spDefaultProcessor;
    if (old != processor) {
        if (old != nullptr && Decrement(&old->refCount) == 0) {
            Virtual<void>(old, 1);
        }
        NiViewRenderClick_ms_spDefaultProcessor = processor;
        if (processor != nullptr) {
            Increment(&processor->refCount);
        }
    }
}

// ---- NiShadowManager's statics

// 0x756070: _SDMInit: the shadow map names and the render click factory
void __cdecl NiShadowManager_SDMInit() {
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[0], "DirShadow");
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[1], "PointShadow");
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[2], "SpotShadow");
    void* factory = NiNew(4);
    if (factory != nullptr) {
        *static_cast<re::Ptr32<void>*>(factory) = reinterpret_cast<void*>(RenderClickFactory_vtable);
        NiShadowManager_ms_pkRenderClickFactory = factory;
        return;
    }
    NiShadowManager_ms_pkRenderClickFactory = nullptr;
}

// 0x7560D0: _SDMShutdown
void __cdecl NiShadowManager_SDMShutdown() {
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[0], nullptr);
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[1], nullptr);
    FixedString_AssignText(&NiShadowManager_ms_akMapNames[2], nullptr);
    if (NiShadowManager_ms_pkRenderClickFactory != nullptr) {
        NiDelete(NiShadowManager_ms_pkRenderClickFactory, 4);
    }
    NiShadowManager_ms_pkRenderClickFactory = nullptr;
}

// ---- NiStream's statics

// 0x759980: NiTPointerAllocator::_SDMShutdown: the blocks freed, the free list forgotten
void __cdecl NiTPointerAllocator_SDMShutdown() {
    void* block = NiTPointerAllocator_ms_pBlockList;
    while (block != nullptr) {
        void* next = *static_cast<re::Ptr32<void>*>(block);
        NiDeleteArray_(block, 0xC);
        block = next;
    }
    NiTPointerAllocator_ms_pFreeList = nullptr;
    NiTPointerAllocator_ms_pBlockList = nullptr;
}

// 0x759F00: NiStream::UnregisterPostProcessFunction (the slot emptied; the last one dropped)
void __cdecl NiStream_UnregisterPostProcessFunction(void* function) {
    NiTArray<void*>* functions = NiStream_ms_pkPostProcessFunctions;
    uint32_t i = 0;
    for (;; i++) {
        if (i >= functions->size) {
            return;
        }
        if (functions->base[i] == function) {
            break;
        }
    }
    if (i >= functions->size) {
        return;
    }
    void* old = functions->base[i];
    functions->base[i] = nullptr;
    if (old != nullptr) {
        functions->esize--;
    }
    uint16_t last = uint16_t(functions->size - 1);
    if (i == last) {
        functions->size = last;
    }
}

// 0x75B7F0: NiStream::RegisterPostProcessFunction (into the first empty slot)
void __cdecl NiStream_RegisterPostProcessFunction(re::Ptr32<void> function) {
    PrimitiveArray_AddFirstEmpty(NiStream_ms_pkPostProcessFunctions, &function);
}

// 0x75BF80: NiStream::_SDMInit: the loaders' map (59 buckets, keys not copied) and the
// post-process functions' array (growing by 3)
void __cdecl NiStream_SDMInit() {
    void* loaders = NiNew(0x14);
    if (loaders == nullptr) {
        NiStream_ms_kLoaders = nullptr;
    } else {
        InitMap(loaders, 0x3B);
        At<uint8_t>(loaders, 0x10) = 0;
        At<void*>(loaders, 0) = reinterpret_cast<void*>(LoaderMap_vtable);
        NiStream_ms_kLoaders = loaders;
    }
    NiTArray<void*>* functions = static_cast<NiTArray<void*>*>(NiNew(0x10));
    if (functions != nullptr) {
        functions->maxSize = 0;
        functions->size = 0;
        functions->esize = 0;
        functions->base = nullptr;
        functions->growBy = 3;
        functions->vtable = reinterpret_cast<void*>(PrimitiveArray_vtable);
        NiStream_ms_pkPostProcessFunctions = functions;
        return;
    }
    NiStream_ms_pkPostProcessFunctions = nullptr;
}

// 0x7599C0: NiStream::_SDMShutdown: the loaders' map and the post-process functions' array
// deleted (each one's slot 0, the deleting destructor; the pointers left as they were)
void __cdecl NiStream_SDMShutdown() {
    if (void* loaders = NiStream_ms_kLoaders) {
        Virtual<void>(loaders, 0, 1u);
    }
    if (void* functions = NiStream_ms_pkPostProcessFunctions) {
        Virtual<void>(functions, 0, 1u);
    }
}

// ---- NiMaterial's statics

// 0x762960: NiMaterialLibrary::_SDMShutdown (the library set emptied)
void __cdecl NiMaterialLibrary_SDMShutdown() {
    NiMaterialLibrary_ms_uiLibraryCount = 0;
    LibrarySet_Realloc(reinterpret_cast<void*>(NiMaterialLibrary_ms_kLibraries), 0);
}

// 0x762C60: NiMaterial::_SDMInit: the materials' map (37 buckets), the working directory empty
void __cdecl NiMaterial_SDMInit() {
    void* map = NiNew(0x10);
    if (map != nullptr) {
        At<void*>(map, 0) = reinterpret_cast<void*>(FixedStringMapBase_vtable);
        At<uint32_t>(map, 4) = 0x25;
        At<uint32_t>(map, 0xC) = 0;
        void* table = NiMalloc(0x94);
        At<void*>(map, 8) = table;
        crt::Memset(table, 0, At<uint32_t>(map, 4) * 4);
        At<void*>(map, 0) = reinterpret_cast<void*>(FixedStringMap_vtable);
        NiMaterial_ms_pkMaterials = map;
        NiMaterialLibrary_ms_acDefaultWorkingDirectory = 0;
        return;
    }
    NiMaterial_ms_pkMaterials = nullptr;
    NiMaterialLibrary_ms_acDefaultWorkingDirectory = 0;
}

// 0x7629E0: NiMaterial::_SDMShutdown: the materials' map deleted (its slot 0)
void __cdecl NiMaterial_SDMShutdown() {
    if (void* map = NiMaterial_ms_pkMaterials) {
        Virtual<void>(map, 0, 1u);
    }
    NiMaterial_ms_pkMaterials = nullptr;
}

// ---- NiShaderDeclaration's statics

// 0x77C5B0: a declaration entry's destructor (its usage name, at +0xC, let go)
void __fastcall NiShaderDeclaration_EntryDestruct(void* self, re::Edx) {
    Release(&At<const char*>(self, 0xC));
}

// 0x77C630: NiShaderDeclaration::_SDMInit: the vertex usage names
void __cdecl NiShaderDeclaration_SDMInit() {
    static const char* const kNames[14] = {
        "POSITION", "BLENDWEIGHT", "BLENDINDICES", "NORMAL", "PSIZE", "TEXCOORD", "TANGENT",
        "BINORMAL", "TESSFACTOR", "POSITIONT", "COLOR", "FOG", "DEPTH", "SAMPLE",
    };
    for (int i = 0; i < 14; i++) {
        re::Ptr32<const char> name = StringTable_AddString(kNames[i]);
        FixedString_Assign(&NiShaderDeclaration_ms_akSemanticNames[i], &name);
        Release(&name);
    }
}

// 0x77CA70: NiShaderDeclaration::_SDMShutdown: each usage name set to the null text's handle
// (NiFixedString::operator=(nullptr), inlined), the old one let go (those never set skipped)
void __cdecl NiShaderDeclaration_SDMShutdown() {
    for (int i = 0; i < 14; i++) {
        re::Ptr32<const char> old = NiShaderDeclaration_ms_akSemanticNames[i];
        if (old == nullptr) {
            continue;
        }
        NiShaderDeclaration_ms_akSemanticNames[i] = StringTable_AddString(nullptr);
        Release(&old);
    }
}

// ---- NiShaderConstantMap's statics

// 0x77EE50: the object mappings' types: per mapping (from -1), its size in bytes, registers and
// attribute type
void __cdecl NiShaderConstantMap_InitObjectMappingTypes() {
    // the jump table's cases by mapping 0..28 (others: 6, the default)
    static const uint8_t kCase[29] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 3, 3, 1, 1, 1, 1, 0, 2, 2, 2, 6, 4, 5, 4};
    for (int32_t mapping = -1; mapping < 29; mapping++) {
        uint32_t registers = 1;
        uint32_t size = 4;
        uint32_t type = 0;
        int which = uint32_t(mapping) <= 0x1C ? kCase[mapping] : 6;
        switch (which) {
        case 0:
            size = registers;
            type = 4;
            break;
        case 1:
            type = 7;
            break;
        case 2:
            registers = size;
            size = 0x10;
            type = 9;
            break;
        case 3:
            registers = 3;
            size = 0xC;
            type = 0xD;
            break;
        case 4:
            type = 5;
            break;
        case 5:
            type = size;
            break;
        default:
            break;
        }
        uint32_t* entry = &NiShaderConstantMap_ms_akObjectMappingTypes[(mapping + 1) * 3];
        entry[0] = size;
        entry[1] = registers;
        entry[2] = type;
    }
}

#define NIMAINSDM_REVERSED(name, address, fn) \
    static void* RE_CONCAT(Original_, __LINE__); \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

NIMAINSDM_REVERSED("NiShadowManager::_SDMInit", 0x00756070, NiShadowManager_SDMInit);
NIMAINSDM_REVERSED("NiShadowManager::_SDMShutdown", 0x007560D0, NiShadowManager_SDMShutdown);
NIMAINSDM_REVERSED("NiTPointerAllocator::_SDMShutdown", 0x00759980, NiTPointerAllocator_SDMShutdown);
NIMAINSDM_REVERSED("NiStream::UnregisterPostProcessFunction", 0x00759F00, NiStream_UnregisterPostProcessFunction);
NIMAINSDM_REVERSED("NiStream::RegisterPostProcessFunction", 0x0075B7F0, NiStream_RegisterPostProcessFunction);
NIMAINSDM_REVERSED("NiStream::_SDMInit", 0x0075BF80, NiStream_SDMInit);
NIMAINSDM_REVERSED("NiMaterialLibrary::_SDMShutdown", 0x00762960, NiMaterialLibrary_SDMShutdown);
NIMAINSDM_REVERSED("NiMaterial::_SDMInit", 0x00762C60, NiMaterial_SDMInit);
NIMAINSDM_REVERSED("NiRenderListProcessor::PreRenderProcessList", 0x00777D10, NiRenderListProcessor_PreRenderProcessList);
NIMAINSDM_REVERSED("NiShaderDeclaration::Entry::~Entry", 0x0077C5B0, NiShaderDeclaration_EntryDestruct);
NIMAINSDM_REVERSED("NiShaderDeclaration::_SDMInit", 0x0077C630, NiShaderDeclaration_SDMInit);
NIMAINSDM_REVERSED("NiShaderConstantMap::InitObjectMappingTypes", 0x0077EE50, NiShaderConstantMap_InitObjectMappingTypes);
NIMAINSDM_REVERSED("NiViewRenderClick::_SDMInit", 0x00785870, NiViewRenderClick_SDMInit);
NIMAINSDM_REVERSED("NiMainSDM::Init", 0x007858F0, NiMainSDM_Init);
NIMAINSDM_REVERSED("NiMainSDM::Shutdown", 0x00785E20, NiMainSDM_Shutdown);
NIMAINSDM_REVERSED("NiMainSDM::NiMainSDM", 0x007861F0, NiMainSDM_Construct);
// the _SDMShutdowns Shutdown calls: never hooked in the 32-bit game, bodies at their addresses
// for the 64-bit build
RE_UNHOOKED_FN("NiStream::_SDMShutdown", 0x007599C0, &NiStream_SDMShutdown);
RE_UNHOOKED_FN("NiMaterial::_SDMShutdown", 0x007629E0, &NiMaterial_SDMShutdown);
RE_UNHOOKED_FN("NiShaderDeclaration::_SDMShutdown", 0x0077CA70, &NiShaderDeclaration_SDMShutdown);

} // namespace ni
