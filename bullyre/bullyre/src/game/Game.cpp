#include "game/Game.h"

#include <windows.h>

#include <cstring>

#include "game/FileLoader.h"

#include "game/Crt.h"
#include "reversed.h"
// The game's systems CGame starts, runs and stops, by address: their own passes name
// them. Where the loading screen or the profiler says what one is, it's in the name.
namespace {

struct Vec3 {
    float x, y, z;
};

// the objects the thiscalls below run on
inline void* const TheAnimManager = reinterpret_cast<void*>(0x020C4910);
inline void* const TheCamera = reinterpret_cast<void*>(0x00C3CC68);
inline void* const TheLurStore = reinterpret_cast<void*>(0x00D02850);   // the scripts (catdog)
inline void* const TheMissionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const TheWorldPlayer = reinterpret_cast<void*>(0x00C1AEA8);   // CWorld::Player (the ped first)
inline void* const TheRestart = reinterpret_cast<void*>(0x00BD1078);   // the restart points (Restart.cpp)
inline void* const TheScreamer = reinterpret_cast<void*>(0x00CF3DA0);   // the audio
GAME_VAR(void*, FindPlayerPed_ptr, 0x00C1AEA8);
GAME_VAR(void*, ms_pRadar, 0x00C674CC);
GAME_VAR(void*, ms_pHUDInventory, 0x00C67554);
GAME_VAR(void*, ms_pHUDItems_C674F0, 0x00C674F0);
GAME_VAR(void*, ms_pEffects_B6F1D4, 0x00B6F1D4);
GAME_VAR(void*, ms_pEffects_B6F1D8, 0x00B6F1D8);
GAME_VAR(void*, ms_pEffects_B6F1DC, 0x00B6F1DC);
GAME_VAR(void*, ms_pEffects_B6F1E8, 0x00B6F1E8);
GAME_VAR(void*, ms_pEffects_B6F1F0, 0x00B6F1F0);
GAME_VAR(void*, ms_pPatrolPaths, 0x00D13CB4);          // (made by Initialise, 0x10 bytes)
GAME_VAR(void*, ms_pPedStats, 0x00C2C084);
GAME_VAR(uint8_t, ms_bExtraEffects, 0x00BF3808);       // Config/Extra/Effects2.bft
GAME_VAR(uint8_t, ms_bExtraObjectData, 0x00BF380B);
GAME_VAR(uint8_t, ms_bExtraAccessories, 0x00BF380E);
GAME_VAR(uint8_t, ms_bUseIpc, 0x00BF3811);              // (and shared.bin)
GAME_VAR(uint8_t, ms_bSkipLevel2, 0x00BF3813);
GAME_VAR(uint8_t, ms_bSkipLevel1, 0x00BF3814);
GAME_VAR(uint8_t, ms_bWidescreen, 0x00C1870E);
GAME_VAR(int32_t, ms_language, 0x00C18728);
GAME_VAR(uint8_t, ms_bLogoMoviesPlaying, 0x00BCBB50);
GAME_VAR(uint8_t, ms_bRestartingScripts, 0x020C32EE);
GAME_VAR(int16_t, ms_debugTextLines, 0x00C21CA0);
GAME_VAR(void*, ms_pCompletion, 0x020CE75C);
GAME_VAR(void*, ms_pScriptCallback, 0x020C8704);       // an object with a byte "active" at +4 (vf1: run)
GAME_VAR(uint8_t, ms_bPaused_C1A998, 0x00C1A998);
GAME_VAR(uint8_t, ms_bPaused_C1A999, 0x00C1A999);
GAME_VAR(uint8_t, ms_bPaused_C1A99A, 0x00C1A99A);
GAME_VAR(uint8_t, ms_bInFrontEnd_20C5BE6, 0x020C5BE6);
GAME_VAR(uint8_t, ms_bFrontEnd_20C5BE5, 0x020C5BE5);
GAME_VAR(uint8_t, ms_bCutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t, ms_bUserStart, 0x020C32E9);
GAME_VAR(int32_t, ms_gameState, 0x00BA6890);
GAME_VAR(uint8_t, ms_bShowCompletion, 0x020C43FD);
GAME_VAR(int32_t, ms_missionActive_20C87AC, 0x020C87AC);
GAME_VAR(int32_t, ms_cameraMode_C3CCF8, 0x00C3CCF8);
GAME_VAR(void*, ms_pVehicles_C2C108, 0x00C2C108);
GAME_VAR(void*, ms_pObj_BCD080, 0x00BCD080);
GAME_VAR(void*, ms_pObj_C0F5E4, 0x00C0F5E4);
GAME_VAR(void*, ms_pObj_C0F78C, 0x00C0F78C);
GAME_VAR(void*, ms_pObj_C2AF9C, 0x00C2AF9C);
GAME_VAR(void*, ms_pObj_C2B034, 0x00C2B034);
GAME_VAR(void*, ms_pObj_C2C12C, 0x00C2C12C);
GAME_VAR(void*, ms_pObj_C2C1A0, 0x00C2C1A0);
GAME_VAR(int32_t*, ms_pFreeMemory, 0x00D141A8);         // (the main heap's free bytes first)
GAME_VAR(uint8_t, ms_bNoBigBuildingTidy, 0x00BD0FCA);
GAME_VAR(uint32_t, ms_liquids_BC5DF8, 0x00BC5DF8);
GAME_VAR(uint8_t, ms_modelInfoLock, 0x00D1420C);        // the models may be touched (0 while tidying)
inline CRITICAL_SECTION* const ms_modelInfoSection = reinterpret_cast<CRITICAL_SECTION*>(0x00D14214);
GAME_ARRAY(void*, ms_modelInfoPtrs, 0x00C67738);        // CModelInfo::ms_modelInfoPtrs (12200)
inline char* const ms_textBuffer = reinterpret_cast<char*>(0x00C221A8);
inline uint8_t* const ipbBuffer = reinterpret_cast<uint8_t*>(0x00BD7800);   // (0x1C000 bytes)

// ---- FinalShutdown / Shutdown ----
GAME_FN(void, __cdecl, TxdStore_Shutdown, 0x005F17F0);
GAME_FN(void, __cdecl, PedStats_Shutdown, 0x0049A1B0);
GAME_FN(void, __cdecl, CdStreamShutdown, 0x0073A5C0);
GAME_FN(void, __cdecl, Fn_55E0D0, 0x0055E0D0);
GAME_FN(void, __thiscall, Fn_46ED00, 0x0046ED00, void*);
GAME_FN(void, __thiscall, Fn_45B030, 0x0045B030, void*);
GAME_FN(void, __cdecl, Fn_5287F0, 0x005287F0);
GAME_FN(void, __cdecl, Fn_4C9920, 0x004C9920);
GAME_FN(void, __cdecl, Fn_444BB0, 0x00444BB0);
GAME_FN(void, __cdecl, Fn_4D93F0, 0x004D93F0);
GAME_FN(void, __cdecl, Fn_499CD0, 0x00499CD0);
GAME_FN(void, __cdecl, Fn_46F420, 0x0046F420);
GAME_FN(void, __cdecl, Fn_49F1E0, 0x0049F1E0);
GAME_FN(void, __cdecl, Fn_448960, 0x00448960);
GAME_FN(void, __cdecl, Fn_472C60, 0x00472C60);
GAME_FN(void, __cdecl, Fn_41EBD0, 0x0041EBD0);
GAME_FN(void, __cdecl, Fn_4A27D0, 0x004A27D0);
GAME_FN(void, __cdecl, Fn_4A6640, 0x004A6640);
GAME_FN(void, __cdecl, Fn_4298C0, 0x004298C0);
GAME_FN(void, __cdecl, Fn_45C130, 0x0045C130);
GAME_FN(void, __cdecl, Fn_451510, 0x00451510);
GAME_FN(void, __cdecl, Fn_45F2F0, 0x0045F2F0);
GAME_FN(void, __cdecl, Fn_45DE40, 0x0045DE40, void* ped);
GAME_FN(void, __cdecl, Fn_533880, 0x00533880);
GAME_FN(void, __thiscall, Fn_4A76F0, 0x004A76F0, void*);
GAME_FN(void, __cdecl, Fn_5649B0, 0x005649B0);
GAME_FN(void, __cdecl, Fn_51B4D0, 0x0051B4D0);
GAME_FN(void, __cdecl, Fn_6C3DB0, 0x006C3DB0);
GAME_FN(void, __thiscall, Radar_54B430, 0x0054B430, void*);
GAME_FN(void, __cdecl, Fn_52D820, 0x0052D820);
GAME_FN(void, __cdecl, Fn_5F2690, 0x005F2690);
GAME_FN(void, __cdecl, Fn_510E70, 0x00510E70);
GAME_FN(void, __cdecl, Fn_527750, 0x00527750);
GAME_FN(void, __cdecl, Fn_4DB530, 0x004DB530);
GAME_FN(void, __cdecl, Fn_520DA0, 0x00520DA0);
GAME_FN(void, __thiscall, Fn_6AC990, 0x006AC990, void*);
GAME_FN(void, __cdecl, Fn_6A9890, 0x006A9890);
GAME_FN(void, __cdecl, Fn_458450, 0x00458450);
GAME_FN(void, __thiscall, Fn_4261C0, 0x004261C0, void*);
GAME_FN(void, __thiscall, Fn_5FD140, 0x005FD140, void*);
GAME_FN(void, __cdecl, Fn_571240, 0x00571240);
GAME_FN(void, __cdecl, Fn_50F110, 0x0050F110);
GAME_FN(void, __thiscall, Fn_669F70, 0x00669F70, void*);
GAME_FN(void, __cdecl, Fn_570F60, 0x00570F60);
GAME_FN(void, __thiscall, Fn_6D45A0, 0x006D45A0, void*);
GAME_FN(void, __thiscall, Fn_68FBF0, 0x0068FBF0, void*);
GAME_FN(void, __thiscall, LipStore_Shutdown, 0x0068E010, void*);
GAME_FN(void, __cdecl, Fn_5F4DE0, 0x005F4DE0);
GAME_FN(void, __thiscall, LurStore_Shutdown, 0x005DC070, void*);
GAME_FN(void, __thiscall, Fn_487F50, 0x00487F50, void*);
GAME_FN(void, __cdecl, Fn_570790, 0x00570790);
GAME_FN(void, __thiscall, Fn_463FA0, 0x00463FA0, void*);
GAME_FN(void*, __cdecl, Fn_40F7A0, 0x0040F7A0);
GAME_FN(void, __thiscall, Fn_40F7D0, 0x0040F7D0, void*);
GAME_FN(void, __thiscall, Fn_5E6480, 0x005E6480, void*);
GAME_FN(void, __thiscall, Fn_523E30, 0x00523E30, void*);
GAME_FN(void, __cdecl, TxdStore_RemoveTxdSlot, 0x005F2240, int slot);
GAME_FN(void, __thiscall, AnimManager_Shutdown, 0x006C32D0, void*);
GAME_FN(void, __cdecl, Fn_44D090, 0x0044D090);
GAME_FN(void, __cdecl, Fn_5AF140, 0x005AF140);
GAME_FN(void, __thiscall, Fn_5A9690, 0x005A9690, void*);
GAME_FN(void, __cdecl, Fn_5B3FB0, 0x005B3FB0);
GAME_FN(void, __cdecl, Fn_5B46E0, 0x005B46E0);
GAME_FN(void, __cdecl, Fn_5B4EF0, 0x005B4EF0);
GAME_FN(void, __cdecl, Fn_5B5DA0, 0x005B5DA0);

// ---- files, memory, texture dictionaries, animations ----
GAME_FN(int, __cdecl, File_GetFileLength, 0x0042D210, const char* path);
GAME_FN(FileHandle, __cdecl, File_OpenFile, 0x0042D260, const char* path, const char* mode, int);
GAME_FN(uint32_t, __cdecl, File_Read, 0x0042D2A0, FileHandle file, void* buffer, uint32_t size);
GAME_FN(int, __cdecl, File_CloseFile, 0x0042D450, FileHandle file);
GAME_FN(uint32_t, __cdecl, File_ReadItems, 0x0042D0E0, void* buffer, uint32_t size, uint32_t count, FileHandle file);
GAME_FN(FileStream*, __cdecl, File_OpenStream, 0x0042D4C0, int kind, int mode, void* arg);
GAME_FN(int, __cdecl, File_CloseStream, 0x0042D570, FileStream* stream, FileStreamBuffer* out);
GAME_FN(uint8_t, __cdecl, File_ReadLineFromMemory, 0x0042D600, MemoryFile* file, char* buffer, int size);
GAME_FN(uint8_t, __thiscall, MemoryFile_Load, 0x00426610, MemoryFile* file, const char* path);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __cdecl, PushMemoryTag, 0x005EF0A0, int tag);
GAME_FN(void, __cdecl, PopMemoryTag, 0x005EF100);
GAME_FN(int, __cdecl, Txd_AddTxdSlot, 0x005F26E0, const char* name);
GAME_FN(void, __cdecl, Txd_LoadTxdFile, 0x005F2A80, int slot, const char* path);
GAME_FN(void, __cdecl, Txd_Fn_5F28C0, 0x005F28C0, int slot);
GAME_FN(void, __cdecl, Txd_AddRef, 0x005F1130, int slot);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_Initialise, 0x005F2600);
GAME_FN(void, __cdecl, Txd_Fn_5F3580, 0x005F3580);
GAME_FN(void, __cdecl, Txd_Fn_5F2BD0, 0x005F2BD0);
GAME_FN(void, __cdecl, Txd_RemoveUnused, 0x005F18C0);
GAME_FN(void, __thiscall, Anim_LoadHxd, 0x006C1C10, void* manager, int, const char* name, re::Ptr32<void>* group, FileStream* stream);
GAME_FN(void, __thiscall, Anim_LoadHxd2, 0x006C1E30, void* manager, const char* name, re::Ptr32<void>* group, int);
GAME_FN(void, __cdecl, Streaming_SetHxd, 0x0052DA60, void* group, int slot);
GAME_FN(void, __thiscall, Anim_Fn_6BF820, 0x006BF820, void* manager);
GAME_FN(void, __thiscall, Anim_CacheHierarchy, 0x0042E3C0, void* manager, const char* name);
GAME_FN(void, __thiscall, Anim_LoadMgr, 0x006C2E50, void* manager, const char* path);
GAME_FN(void*, __thiscall, AnimGroup_Find, 0x0042E350, void* group, const char* name, re::Ptr32<void>* out);
GAME_FN(void, __thiscall, Anim_LoadAgr, 0x006BF500, void* manager, void* group, void* data, uintptr_t);
GAME_FN(void, __thiscall, Anim_Fn_6BD060, 0x006BD060, void* manager, void* group, void* data, int);
GAME_FN(void, __thiscall, Anim_LoadMxd, 0x006C2FE0, void* manager, const char* path);
GAME_FN(int, __thiscall, Anim_Fn_6BD0D0, 0x006BD0D0, void* manager, const char* path, char* out, int);
GAME_FN(void, __thiscall, Anim_Fn_6C2730, 0x006C2730, void* manager, char* name, int, int, int* out);
GAME_FN(void, __thiscall, Anim_Fn_6C14B0, 0x006C14B0, void* manager, int);
GAME_FN(void, __thiscall, Anim_Fn_6C0A30, 0x006C0A30, void* manager);

// ---- the renderer's side ----
GAME_FN(void*, __cdecl, Camera_Create, 0x005EF9D0, int width, int height, int);
GAME_FN(void, __cdecl, Camera_SetFarClip, 0x006C8780, void* camera, float);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float);
GAME_FN(void, __cdecl, Camera_SetView, 0x005EF8A0, void* camera, uintptr_t, float, float);
GAME_FN(void, __cdecl, Camera_Destroy, 0x005EF940, void* camera);
GAME_FN(void, __cdecl, Fn_6895D0, 0x006895D0);
GAME_FN(void, __cdecl, LoadingScreen, 0x0043C4F0, const char* title, const char* message, const char* screen);
GAME_FN(void, __cdecl, KeepLoadingScreenAlive, 0x0043C060, int, int);
GAME_FN(void, __cdecl, Fn_402860, 0x00402860);
GAME_FN(void, __cdecl, ScreenFX_Init, 0x0055EF60);
GAME_FN(void, __thiscall, Fn_5EE3F0, 0x005EE3F0, void*, void*);
GAME_FN(void, __thiscall, Fn_5EE4A0, 0x005EE4A0, void*);
GAME_FN(void, __cdecl, Fonts_514A70, 0x00514A70);
GAME_FN(void, __cdecl, Fonts_689A80, 0x00689A80);
GAME_FN(void, __cdecl, Fonts_6A5EE0, 0x006A5EE0);
GAME_FN(void, __cdecl, Fn_43AD50, 0x0043AD50);
GAME_FN(void, __cdecl, Fn_514040, 0x00514040);
GAME_FN(void, __thiscall, Fn_5131D0, 0x005131D0, void*);
GAME_FN(void, __thiscall, Fn_561040, 0x00561040, void*);
GAME_FN(void, __cdecl, LoadAllShaders, 0x00401280);
GAME_FN(void*, __cdecl, MoviePlayer_Get, 0x00411370);

// ---- InitialiseOnceAfterRW ----
GAME_FN(void, __thiscall, EffectSys_Init, 0x00669F00, void*);
GAME_FN(void, __thiscall, Effects_LoadTxd, 0x00578280, void*, const char*);
GAME_FN(void, __thiscall, Effects_LoadDefCnt, 0x00666B80, void*, const char*);
GAME_FN(void, __thiscall, Effects_LoadStore, 0x0066B750, void*, const char*);
GAME_FN(void, __thiscall, Effects_Load, 0x00667AA0, void*, const char*);
GAME_FN(void, __cdecl, Fn_72E610, 0x0072E610);
GAME_FN(void, __thiscall, WorldSurfTable_Load, 0x00464420, void*);
GAME_FN(void, __thiscall, Hardness_Load, 0x0045B270, void*, const char*);
GAME_FN(void, __thiscall, ColStepEffects_Load, 0x0072E390, void*, const char*);
GAME_FN(void, __cdecl, SpecialFX_Init, 0x00529380);
GAME_FN(void, __thiscall, TextManager_Init, 0x0068FC00, void*);
GAME_FN(void, __cdecl, Fn_44D320, 0x0044D320);
GAME_FN(void, __cdecl, Fn_45B8A0, 0x0045B8A0);
GAME_FN(void, __cdecl, Fn_571440, 0x00571440);
GAME_FN(void, __thiscall, HandlingManager_Init, 0x004CA700, void*);
GAME_FN(void, __cdecl, SurfaceTable_Load, 0x0045B3A0, const char* path);
GAME_FN(void, __cdecl, Fn_6AEF70, 0x006AEF70);
GAME_FN(void, __cdecl, Minigame_Init, 0x00705A90);

// ---- Initialise ----
GAME_FN(void, __thiscall, MissionMgr_LoadBank, 0x006AC730, void*, const char* name, const char* path);
GAME_FN(void, __cdecl, Fn_46DBF0, 0x0046DBF0);
GAME_FN(void, __thiscall, Fn_6D3EA0, 0x006D3EA0, void*);
GAME_FN(void, __cdecl, Fn_4A1100, 0x004A1100);
GAME_FN(void, __cdecl, Fn_4A83C0, 0x004A83C0);
GAME_FN(void*, __thiscall, PatrolPaths_Construct, 0x005ED540, void*);
GAME_FN(void, __cdecl, Fn_570710, 0x00570710);
GAME_FN(void, __thiscall, GameLogic_Init, 0x00430640, void*);
GAME_FN(void, __cdecl, Fn_450850, 0x00450850);
GAME_FN(void, __thiscall, Camera_SetCamera, 0x004F1800, void*, void* camera);
GAME_FN(void, __thiscall, Camera_Init, 0x004F3720, void*);
GAME_FN(void, __thiscall, Fn_523F40, 0x00523F40, void*);
GAME_FN(void, __thiscall, Fn_4160F0, 0x004160F0, void*);
GAME_FN(void, __thiscall, Fn_43FA00, 0x0043FA00, void*);
GAME_FN(void, __cdecl, Fn_5360A0, 0x005360A0);
GAME_FN(void, __cdecl, Fn_538C50, 0x00538C50);
GAME_FN(void, __cdecl, Fn_51CE00, 0x0051CE00);
GAME_FN(void, __cdecl, Fn_5649A0, 0x005649A0);
GAME_FN(void, __cdecl, World_Initialise, 0x0045D580);
GAME_FN(void, __cdecl, Fn_520F10, 0x00520F10);
GAME_FN(void, __cdecl, Fn_6C3720, 0x006C3720);
GAME_FN(void, __cdecl, InitModelIndices, 0x0043EA60);
GAME_FN(void, __cdecl, Fn_51C6A0, 0x0051C6A0);
GAME_FN(void, __cdecl, Pickups_Init, 0x00444A50);
GAME_FN(void, __cdecl, Fn_4C71E0, 0x004C71E0);
GAME_FN(void, __thiscall, Collectibles_Init, 0x00428D40, void*);
GAME_FN(void, __thiscall, Fn_425790, 0x00425790, void*);
GAME_FN(int, __cdecl, CdStreamAddImage, 0x0073A890, const char* path);
GAME_FN(void, __cdecl, Fn_49A340, 0x0049A340);
GAME_FN(void, __cdecl, FileLoader_LoadLevel, 0x0042CE80, const char* path);
GAME_FN(uint8_t, __cdecl, FileLoader_LoadModelInfoFile, 0x00429E30, const char* path, int);
GAME_FN(void, __thiscall, PedStats_Init, 0x0049A2D0, void*);
GAME_FN(void, __thiscall, Screamer_InitialiseBankManager, 0x00595BA0, void*);
GAME_FN(void, __thiscall, Screamer_Initialise, 0x00595D30, void*);
GAME_FN(void, __cdecl, ObjectData_Initialise, 0x004D0DE0, const char* path);
GAME_FN(void, __cdecl, Streaming_Init, 0x005334C0);
GAME_FN(void, __cdecl, ModelInfo_GetModelInfo, 0x0051C190, const char* name, int* id);
GAME_FN(void, __cdecl, Streaming_SwapModel, 0x0052E870, int id, const char* name);
GAME_FN(void, __cdecl, GenerateAllBoundingBoxes, 0x0045B960, int);
GAME_FN(void, __thiscall, Fn_72C410, 0x0072C410, void*);
GAME_FN(void, __thiscall, Accessories_Load, 0x004A75D0, void*, const char* path);
GAME_FN(void, __thiscall, WeaponAccessories_Load, 0x004DADF0, void*, const char* path);
GAME_FN(void, __thiscall, WeaponAccessories_MatchLookupToIndex, 0x004DAD90, void*);
GAME_FN(void, __thiscall, Liquids_Load, 0x006D4870, void*, const char* path);
GAME_FN(void, __cdecl, VehicleColors_Load, 0x00535700);
GAME_FN(void, __thiscall, Grids_Load, 0x004434E0, void*, const char* path);
GAME_FN(void, __thiscall, WorldPlayer_Clear, 0x00445AE0, void*);
GAME_FN(void, __thiscall, Camera_SetDefaultFOV, 0x004F1660, void*);
GAME_FN(void, __cdecl, Streaming_Fn_533460, 0x00533460);
GAME_FN(void, __cdecl, Streaming_RequestBigBuildings, 0x00532270, int level);
GAME_FN(void, __cdecl, Streaming_LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, Ped_Initialise, 0x00472FA0);
GAME_FN(void, __thiscall, CoverManager_Init, 0x00488340, void*);
GAME_FN(void, __thiscall, ReflectionManager_LoadData, 0x00523FB0, void*);
GAME_FN(void, __cdecl, FindBigBuildings, 0x00452B00);
GAME_FN(void, __thiscall, Fn_46EE70, 0x0046EE70, void*);
GAME_FN(void, __thiscall, Radar_Initialize, 0x0054B3F0, void*);
GAME_FN(void, __thiscall, HUDInventory_Initialize, 0x00540E50, void*);
GAME_FN(void, __cdecl, Weapon_InitialiseWeapons, 0x004D93E0);
GAME_FN(void, __cdecl, Weapon_RegisterCB, 0x004DA480);
GAME_FN(void, __thiscall, Accessories_Initialize, 0x004A79C0, void*);
GAME_FN(void, __cdecl, PAnimModelInfo_InitPropButes, 0x00520CD0);
GAME_FN(void, __thiscall, Fn_55CD20, 0x0055CD20, void*);
GAME_FN(void, __cdecl, HUDItems_Init, 0x00541A60);
GAME_FN(void, __cdecl, GameCompletionMgr_Initialise, 0x006A9850);
GAME_FN(void, __cdecl, Fn_46F7B0, 0x0046F7B0);
GAME_FN(void, __cdecl, Fn_448450, 0x00448450);
GAME_FN(void, __cdecl, Fn_49F190, 0x0049F190);
GAME_FN(void, __cdecl, Fn_472C10, 0x00472C10);
GAME_FN(void, __cdecl, Fn_4A22D0, 0x004A22D0);
GAME_FN(void, __cdecl, DockManager_Initialise, 0x0046E940);
GAME_FN(void, __cdecl, Fn_4A65F0, 0x004A65F0);
GAME_FN(void, __cdecl, Fn_4298A0, 0x004298A0);
GAME_FN(void, __cdecl, Fn_45C0F0, 0x0045C0F0);
GAME_FN(void, __cdecl, Fn_44EAD0, 0x0044EAD0);
GAME_FN(void, __cdecl, AmbientVehicles_Initialise, 0x0041EB10);
GAME_FN(void, __cdecl, Coronas_Init, 0x00510E00);
GAME_FN(void, __cdecl, Fn_525030, 0x00525030);
GAME_FN(void, __cdecl, Fn_4DB2A0, 0x004DB2A0);
GAME_FN(void, __cdecl, Fn_5276F0, 0x005276F0);
GAME_FN(void, __cdecl, Stats_Initialise, 0x004593F0);
GAME_FN(void, __thiscall, PedMgr_Init, 0x0048AE00, void*);
GAME_FN(void, __cdecl, SurfaceDataMgr_Init, 0x00570F40);
GAME_FN(void, __thiscall, Fn_6CBCA0, 0x006CBCA0, void*);
GAME_FN(void, __thiscall, Fn_4E9290, 0x004E9290, void*);
GAME_FN(void, __cdecl, Fn_426020, 0x00426020, int);
GAME_FN(void, __cdecl, Fn_510820, 0x00510820);
GAME_FN(void, __thiscall, FlashLights_Init, 0x00513190, void*);
GAME_FN(void, __cdecl, ModelInfo_SetupPropActionTrees, 0x0051BA20);

// ---- InitialiseScripts ----
GAME_FN(void, __cdecl, Fn_570470, 0x00570470);
GAME_FN(void, __thiscall, LurStore_InitScripts, 0x005DC2F0, void*);
GAME_FN(void, __cdecl, ModelInfo_LoadDefaultPropActFiles, 0x0051C3D0);
GAME_FN(void, __thiscall, LurStore_Update, 0x005DBB40, void*, int);
GAME_FN(void, __cdecl, LoadScene, 0x0055E210);
GAME_FN(void, __thiscall, TrafficLights_Setup, 0x0045B180, void*);
GAME_FN(void, __cdecl, ActionContext_Init, 0x005F37D0);
GAME_FN(void, __thiscall, Ped_Fn_474090, 0x00474090, void* ped);
GAME_FN(void, __cdecl, WalkableMeshQueryService_Initialize, 0x005E8F60);
GAME_FN(void, __cdecl, ModelInfo_DropIndex, 0x0051A940);
GAME_FN(void, __cdecl, Fn_5F0A60, 0x005F0A60);

// ---- ReInitGameObjectVariables ----
GAME_FN(void, __thiscall, Fn_7DB6B0, 0x007DB6B0, void*);
GAME_FN(void, __cdecl, Fn_55DFE0, 0x0055DFE0);
GAME_FN(void, __cdecl, Fn_5BB390, 0x005BB390);
GAME_FN(void, __thiscall, Fn_416020, 0x00416020, void*);
GAME_FN(void, __thiscall, Fn_425230, 0x00425230, void*);
GAME_FN(void, __thiscall, Fn_668150, 0x00668150, void*);
GAME_FN(void, __thiscall, Fn_453D40, 0x00453D40, void*);
GAME_FN(void, __thiscall, Radar_Fn_549180, 0x00549180, void*);
GAME_FN(void, __cdecl, Fn_6A4E20, 0x006A4E20);
GAME_FN(void, __cdecl, Fn_41EB70, 0x0041EB70);
GAME_FN(void, __cdecl, Fn_51B800, 0x0051B800);
GAME_FN(void, __thiscall, Fn_6D3BB0, 0x006D3BB0, void*);
GAME_FN(void, __cdecl, Fn_49DF80, 0x0049DF80);
GAME_FN(void, __cdecl, Fn_4A2800, 0x004A2800);
GAME_FN(void, __cdecl, Fn_46EC80, 0x0046EC80);
GAME_FN(void, __cdecl, Fn_472BF0, 0x00472BF0);
GAME_FN(void, __cdecl, Fn_4A6AC0, 0x004A6AC0);
GAME_FN(void, __cdecl, Fn_44E6A0, 0x0044E6A0);
GAME_FN(void, __thiscall, Fn_48AE70, 0x0048AE70, void*);
GAME_FN(void, __thiscall, Fn_43FA40, 0x0043FA40, void*);
GAME_FN(void, __cdecl, Fn_4C9530, 0x004C9530);
GAME_FN(void, __cdecl, Fn_520E50, 0x00520E50);
GAME_FN(void, __cdecl, Fn_52F600, 0x0052F600);
GAME_FN(void, __thiscall, Fn_43F780, 0x0043F780, void*);
GAME_FN(void, __cdecl, Fn_6C38A0, 0x006C38A0);
GAME_FN(void, __thiscall, Ped_Fn_47D2F0, 0x0047D2F0, void* ped);
GAME_FN(void, __thiscall, Fn_68F340, 0x0068F340, void*);
GAME_FN(void, __thiscall, Fn_4545C0, 0x004545C0, void*, int);
GAME_FN(void, __thiscall, Radar_Fn_54AD80, 0x0054AD80, void*);
GAME_FN(void, __thiscall, Fn_50DE30, 0x0050DE30, void*);

// ---- Process ----
GAME_FN(void, __cdecl, Fn_738610, 0x00738610);
GAME_FN(void, __cdecl, Fn_6C44B0, 0x006C44B0);
GAME_FN(void, __cdecl, Fn_6A5F20, 0x006A5F20);
GAME_FN(void, __cdecl, Fn_7379A0, 0x007379A0);
GAME_FN(void, __thiscall, Restart_Process, 0x00432690, void*);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int, int);
template <typename... A>
inline int __cdecl Sprintf(char* out, const char* format, A... rest) { return crt::Sprintf(out, format, rest...); }
GAME_FN(void, __cdecl, DumpMemInfo, 0x00530100, int);
GAME_FN(uint32_t, __cdecl, Timer_GetCurrentTime, 0x0045BCA0);
GAME_FN(void, __cdecl, Streaming_Update, 0x005346B0);
GAME_FN(void, __thiscall, Fn_5E54B0, 0x005E54B0, void*);
GAME_FN(void, __thiscall, Fn_5E6920, 0x005E6920, void*);
GAME_FN(void, __thiscall, Fn_417820, 0x00417820, void*);
GAME_FN(void, __thiscall, Fn_488350, 0x00488350, void*);
GAME_FN(void, __thiscall, Fn_6AEB40, 0x006AEB40, void*);
GAME_FN(void, __thiscall, Fn_418A00, 0x00418A00, void*);
GAME_FN(void, __thiscall, Fn_72E090, 0x0072E090, void*);
GAME_FN(void, __cdecl, Fn_4E8E10, 0x004E8E10);
GAME_FN(void, __thiscall, Fn_6CA7B0, 0x006CA7B0, void*);
GAME_FN(uint8_t, __cdecl, Fn_514570, 0x00514570);
GAME_FN(void, __thiscall, MissionMgr_Process, 0x006ADDD0, void*);
GAME_FN(void, __thiscall, Fn_424DD0, 0x00424DD0, void*);
GAME_FN(void*, __cdecl, Fn_45C950, 0x0045C950);
GAME_FN(void, __thiscall, Fn_45C3C0, 0x0045C3C0, void*);
GAME_FN(void, __cdecl, Fn_68A250, 0x0068A250);
GAME_FN(void, __cdecl, Fn_4260D0, 0x004260D0);
GAME_FN(void, __cdecl, Fn_536320, 0x00536320);
GAME_FN(void, __cdecl, Fn_527770, 0x00527770);
GAME_FN(void, __cdecl, Fn_521390, 0x00521390);
GAME_FN(void, __thiscall, Effects_BD0CF8_Process, 0x0042E2A0, void*);
GAME_FN(void, __thiscall, Fn_537E00, 0x00537E00, void*);
GAME_FN(void, __thiscall, Fn_454500, 0x00454500, void*);
GAME_FN(void, __thiscall, Fn_667EA0, 0x00667EA0, void*);
GAME_FN(void, __thiscall, Fn_4A7A40, 0x004A7A40, void*);
GAME_FN(void, __thiscall, Fn_43A320, 0x0043A320, void*);
GAME_FN(void, __thiscall, Fn_48B890, 0x0048B890, void*);
GAME_FN(void, __thiscall, Fn_44FFB0, 0x0044FFB0, void*);
GAME_FN(void, __thiscall, Fn_46F800, 0x0046F800, void*);
GAME_FN(void, __thiscall, Fn_4729A0, 0x004729A0, void*);
GAME_FN(void, __thiscall, Vehicles_Process, 0x004A0B60, void*, int);
GAME_FN(int, __thiscall, Camera_Fn_4F12F0, 0x004F12F0, void*);
GAME_FN(void, __thiscall, Fn_422970, 0x00422970, void*);
GAME_FN(void, __thiscall, Fn_449390, 0x00449390, void*);
GAME_FN(void, __thiscall, Fn_4A29B0, 0x004A29B0, void*);
GAME_FN(void, __thiscall, Fn_4A6AD0, 0x004A6AD0, void*);
GAME_FN(void, __cdecl, Fn_4A69A0, 0x004A69A0);
GAME_FN(void, __cdecl, Fn_4C7C40, 0x004C7C40);
GAME_FN(void, __cdecl, Fn_68E440, 0x0068E440);
GAME_FN(void, __cdecl, Fn_683A50, 0x00683A50);
GAME_FN(void, __cdecl, Fn_45FAB0, 0x0045FAB0);
GAME_FN(void, __cdecl, Fn_4440F0, 0x004440F0, const Vec3* pos, float, int);
GAME_FN(void, __cdecl, Fn_445AC0, 0x00445AC0);
GAME_FN(void, __cdecl, Fn_4D9400, 0x004D9400);
GAME_FN(void, __thiscall, Fn_4579A0, 0x004579A0, void*);
GAME_FN(void, __cdecl, Fn_4C9B40, 0x004C9B40);
GAME_FN(void, __cdecl, Fn_6B0780, 0x006B0780);
GAME_FN(void, __cdecl, Fn_538FC0, 0x00538FC0);
GAME_FN(void, __thiscall, Camera_Process, 0x004F3900, void*);
GAME_FN(void, __thiscall, Ped_Fn_49AEF0, 0x0049AEF0, void* ped);
GAME_FN(void, __cdecl, Fn_512540, 0x00512540);
GAME_FN(void, __cdecl, Fn_510EA0, 0x00510EA0);
GAME_FN(void, __cdecl, Fn_526700, 0x00526700);
GAME_FN(void, __cdecl, Fn_513030, 0x00513030);
GAME_FN(void, __cdecl, Fn_45A6B0, 0x0045A6B0);
GAME_FN(void, __thiscall, MissionMgr_ShowCompletion, 0x006AC270, void*);

// ---- TidyUpMemory, ShutDownForRestart, DrasticTidyUpMemory ----
GAME_FN(void, __cdecl, Fn_6C7380, 0x006C7380, void* data, void* callback, uintptr_t);
inline void* const Fn_6CA7D0 = reinterpret_cast<void*>(0x006CA7D0);
GAME_FN(void, __thiscall, Fn_50DEB0, 0x0050DEB0, void*);
GAME_FN(void, __cdecl, Fn_683C00, 0x00683C00);
GAME_FN(void, __thiscall, Fn_68F230, 0x0068F230, void*);
GAME_FN(void*, __thiscall, MissionMgr_Fn_6AA690, 0x006AA690, void*);
GAME_FN(void*, __thiscall, MissionMgr_Fn_6AA680, 0x006AA680, void*);
GAME_FN(void, __thiscall, Mission_Fn_6ACA70, 0x006ACA70, void*);
GAME_FN(uint8_t, __thiscall, Mission_Fn_6AA300, 0x006AA300, void*, int, int, int, int, int, int, int);
GAME_FN(void, __thiscall, String_Construct, 0x005FA910, void* string, const char* s);
GAME_FN(int, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void*, void* name);
GAME_FN(uint8_t*, __thiscall, MissionMgr_GetMission, 0x006AA660, void*, int);
GAME_FN(void, __cdecl, Fn_704C60, 0x00704C60, int);
GAME_FN(void, __thiscall, Screamer_Fn_5962E0, 0x005962E0, void*);
GAME_FN(void, __thiscall, Fn_4244B0, 0x004244B0, void*);
GAME_FN(void, __thiscall, Fn_45C870, 0x0045C870, void*);
GAME_FN(void, __thiscall, MissionMgr_Fn_6AB7B0, 0x006AB7B0, void*);
GAME_FN(void, __thiscall, Fn_6AE260, 0x006AE260, void*);
GAME_FN(void, __cdecl, Fn_5140C0, 0x005140C0);
GAME_FN(void, __cdecl, Fn_6CB620, 0x006CB620);
GAME_FN(void, __thiscall, Fn_457560, 0x00457560, void*);
GAME_FN(void, __thiscall, Fn_455B60, 0x00455B60, void*);
GAME_FN(void, __thiscall, Fn_48B430, 0x0048B430, void*);
GAME_FN(void, __cdecl, Fn_45D600, 0x0045D600);
GAME_FN(void, __cdecl, Streaming_Fn_533980, 0x00533980);
GAME_FN(void, __cdecl, Fn_458140, 0x00458140);
GAME_FN(void, __cdecl, Fn_56F260, 0x0056F260);
GAME_FN(const float*, __cdecl, FindPlayerCoors, 0x00446900, Vec3* out);
GAME_FN(void, __cdecl, Fn_56FE00, 0x0056FE00, const float* xy);
GAME_VAR(uint8_t, ms_bFlag_C18754, 0x00C18754);

// CGame's own (through the game's addresses, so modes apply)
GAME_FN(void, __cdecl, Game_LoadPropHXDsCount, 0x0042E470);
GAME_FN(void, __cdecl, Game_LoadPropHXDs, 0x0042E790);
GAME_FN(void, __cdecl, Game_ReInitGameObjectVariables, 0x0042FB90);
GAME_FN(void, __cdecl, Game_TidyUpMemory, 0x004302A0, int, int);

} // namespace

namespace CGame {

// (the game inlines this read of a FileStream: from its file, or from its memory)
static void ReadFromStream(FileStream* stream, void* out) {
    if (stream->file) {
        File_ReadItems(out, 4, 1, stream->file);
    } else if (stream->data) {
        std::memcpy(out, stream->data + stream->position, 4);
        stream->position += 4;
    }
}

void __cdecl FinalShutdown() {
    TxdStore_Shutdown();
    PedStats_Shutdown();
    CdStreamShutdown();
}

uint8_t __cdecl Shutdown() {
    Fn_55E0D0();
    Fn_46ED00(reinterpret_cast<void*>(0x00C2AB70));
    Fn_45B030(reinterpret_cast<void*>(0x00C1A7DC));
    Fn_5287F0();
    Fn_4C9920();
    Fn_444BB0();
    Fn_4D93F0();
    Fn_499CD0();
    Fn_46F420();
    Fn_49F1E0();
    Fn_448960();
    Fn_472C60();
    Fn_41EBD0();
    Fn_4A27D0();
    Fn_4A6640();
    Fn_4298C0();
    Fn_45C130();
    Fn_451510();
    Fn_45F2F0();
    if (FindPlayerPed_ptr) {
        Fn_45DE40(FindPlayerPed_ptr);
    }
    Fn_533880();
    Fn_4A76F0(reinterpret_cast<void*>(0x00C2C730));
    Fn_5649B0();
    Fn_51B4D0();
    Fn_6C3DB0();
    Radar_54B430(ms_pRadar);   // (without the null check the other calls have)
    Fn_52D820();
    Fn_5F2690();
    Fn_510E70();
    Fn_527750();
    Fn_4DB530();
    Fn_520DA0();
    Fn_6AC990(TheMissionMgr);
    Fn_6A9890();
    Fn_458450();
    Fn_4261C0(reinterpret_cast<void*>(0x00BCEB50));
    Fn_5FD140(reinterpret_cast<void*>(0x00BCCE80));
    Fn_571240();
    Fn_50F110();
    Fn_669F70(reinterpret_cast<void*>(0x00B6F1D0));
    Fn_570F60();
    Fn_6D45A0(reinterpret_cast<void*>(0x020C7C48));
    Fn_68FBF0(reinterpret_cast<void*>(0x020C2FA4));
    LipStore_Shutdown(reinterpret_cast<void*>(0x0208B9F0));
    Fn_5F4DE0();
    LurStore_Shutdown(TheLurStore);
    Fn_487F50(reinterpret_cast<void*>(0x00C2B210));
    if (void* paths = ms_pPatrolPaths) {
        // (virtual deleting destructor, vf2)
        (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*, int)>>*>(paths))[2](paths, 1);
        ms_pPatrolPaths = nullptr;
    }
    Fn_570790();
    Fn_463FA0(reinterpret_cast<void*>(0x00C21BC0));
    Fn_40F7D0(Fn_40F7A0());
    Fn_5E6480(reinterpret_cast<void*>(0x00D12AF8));
    Fn_523E30(reinterpret_cast<void*>(0x00C92D70));
    TxdStore_RemoveTxdSlot(ms_genericTxd);
    AnimManager_Shutdown(TheAnimManager);
    Fn_44D090();
    Fn_5AF140();
    Fn_5A9690(reinterpret_cast<void*>(0x00CF7268));
    Fn_5B3FB0();
    Fn_5B46E0();
    Fn_5B4EF0();
    Fn_5B5DA0();
    return 1;
}

void __cdecl ClearTidiedTextures() {
    ms_bTidiedTextures = 0;
}

uint8_t __cdecl IsSpecialArea(int area) {
    switch (area) {
    case 0x1C: case 0x1D: case 0x21: case 0x22: case 0x1A: case 0x2E: case 0x2C: case 0x27: case 0x0F:
        return 1;
    default:
        return 0;
    }
}

uint8_t __cdecl ShouldMovePlayer() {
    return ms_bMovePlayer;
}

void __cdecl SetMovePlayer() {
    ms_bMovePlayer = 1;
}

void __cdecl ClearMovePlayer() {
    ms_bMovePlayer = 0;
}

void __cdecl LoadPropHXDs() {
    // Anim\hxds.dat into the placement buffer, read as a stream from there
    int size = File_GetFileLength("Anim\\hxds.dat");
    FileHandle file = File_OpenFile("Anim\\hxds.dat", "r", 1);   // (a third argument OpenFile ignores)
    File_Read(file, ipbBuffer, uint32_t(size));
    File_CloseFile(file);
    FileStreamBuffer buffer{ipbBuffer, 0x1C000};
    FileStream* stream = File_OpenStream(3, 1, &buffer);
    uint32_t count = 0;   // (read, unused)
    if (stream) {
        ReadFromStream(stream, &count);
    }
    MemoryFile list{nullptr, 0, 0};
    MemoryFile_Load(&list, "Config\\Dat\\PropHXDs.dat");
    char line[0x80];
    File_ReadLineFromMemory(&list, line, 0x80);   // (the count LoadPropHXDsCount reads)
    int slot = 18;
    while (File_ReadLineFromMemory(&list, line, 0x80)) {
        uint32_t skipped[2] = {0, 0};   // (two words per line the game reads past)
        if (stream) {
            ReadFromStream(stream, &skipped[0]);
            ReadFromStream(stream, &skipped[1]);
        }
        if (line[0] != '#' && line[0] != '\t' && line[0] != '\n') {
            re::Ptr32<void> group = nullptr;
            Anim_LoadHxd(TheAnimManager, 0, line, &group, stream);
            Streaming_SetHxd(group, slot);
            slot++;
        }
    }
    File_CloseStream(stream, nullptr);
    if (list.data) {
        Free(list.data);
    }
}

uint8_t __cdecl InitialiseRenderWare() {
    // "CGame_InitRW", "PreLegalScreen"
    Txd_Initialise();
    Txd_Fn_5F3580();
    ms_camera = Camera_Create(640, 480, 1);
    if (!ms_camera) {
        return 0;
    }
    Camera_SetFarClip(ms_camera, 2000.0f);
    Camera_SetNearClip(ms_camera, 0.9f);
    Camera_SetView(ms_camera, 0, 0.7f, 1.3333334f);
    PushMemoryTag(0x17);
    Fn_6895D0();
    PopMemoryTag();
    // "OnLegalScreen": the loading screen's picture for the language
    static const char* const screens[8] = {"loadscam", "loadscfr", "loadscde", "loadscit",
                                           "loadsces", "loadscbr", "loadscru", "loadscjp"};
    static const char* const wideScreens[8] = {"wloadscn", "wloadsfr", "wloadsde", "wloadsit",
                                               "wloadses", "wloadsbr", "wloadsru", "wloadsjp"};
    // (no bounds check: the language is 0..7)
    LoadingScreen("Loading the Game", reinterpret_cast<const char*>(0x008FEFBB) /* the exe's "" */,
                  ms_bWidescreen ? wideScreens[ms_language] : screens[ms_language]);
    Fn_402860();
    ScreenFX_Init();   // "ScrFX"
    Fn_5EE3F0(reinterpret_cast<void*>(0x00D140B8), ms_cameraData);
    PushMemoryTag(0x17);   // "Fonts"
    Fonts_514A70();
    Fonts_689A80();
    Fonts_6A5EE0();
    PopMemoryTag();
    PushMemoryTag(3);
    Anim_Fn_6BF820(TheAnimManager);
    PushMemoryTag(5);
    Game_LoadPropHXDsCount();
    PopMemoryTag();
    for (int i = 0; i < 18; i++) {   // "BunchOfHXDs"
        re::Ptr32<void> group = nullptr;
        Anim_LoadHxd(TheAnimManager, 0, ms_hxdNames[i], &group, nullptr);
        Streaming_SetHxd(group, i);
    }
    PushMemoryTag(5);   // "LoadPropHXDs"
    Game_LoadPropHXDs();
    PopMemoryTag();
    Anim_CacheHierarchy(TheAnimManager, "ANIM\\MOT_CTRL.HXD");   // "CacheHierarchy"
    Anim_LoadMgr(TheAnimManager, "models\\peds\\MXDs.MGR");        // "LoadMGRFiles"
    Anim_LoadMgr(TheAnimManager, "models\\objects\\MXDs.MGR");
    Anim_LoadMgr(TheAnimManager, "models\\vehicles\\MXDs.MGR");
    PushMemoryTag(5);
    Anim_LoadMgr(TheAnimManager, "models\\wprops\\MXDs.MGR");
    PopMemoryTag();
    Anim_LoadMgr(TheAnimManager, "models\\accssory\\MXDs.MGR");
    PopMemoryTag();   // (the 3)
    PushMemoryTag(4);
    static const char* const agrNames[3] = {"C_Player", "NPC_Cher", "Grap"};
    re::Ptr32<void> group = nullptr;
    Anim_LoadHxd2(TheAnimManager, ms_hxdNames[0], &group, 0);
    Streaming_SetHxd(group, 0);
    for (int i = 0; i < 3; i++) {
        re::Ptr32<void> data;
        if (AnimGroup_Find(group, agrNames[i], &data)) {
            Anim_LoadAgr(TheAnimManager, group, data, 0);
            Anim_Fn_6BD060(TheAnimManager, group, data, 1);
        }
    }
    Anim_LoadMxd(TheAnimManager, "MODELS\\MOT_CTRL.MXD");
    char name[0x80];
    std::memset(name, 0, sizeof(name));
    if (Anim_Fn_6BD0D0(TheAnimManager, "MODELS\\MOT_CTRL.MXD", name, 0) == 0) {
        int out = 0;
        Anim_Fn_6C2730(TheAnimManager, name, 0, 1, &out);
    }
    PopMemoryTag();
    return 1;
}

void __cdecl ShutdownRenderWare() {
    Fn_43AD50();
    Fn_514040();
    Fn_5131D0(reinterpret_cast<void*>(0x00C66B78));
    Fn_5EE4A0(reinterpret_cast<void*>(0x00D140B8));
    Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
    Free(ms_cameraData);
    Camera_Destroy(ms_camera);
    ms_cameraData = nullptr;
    ms_camera = nullptr;
    Txd_Fn_5F2BD0();
}

uint8_t __cdecl InitialiseOnceAfterRW() {
    // "InitOnceAfterRW", "EffectSys"
    EffectSys_Init(reinterpret_cast<void*>(0x00B6F1D0));
    int icon = Txd_AddTxdSlot("LoadingIcon");
    Txd_LoadTxdFile(icon, "TXD/liNA.NFT");
    Txd_AddRef(icon);
    Effects_LoadTxd(ms_pEffects_B6F1F0, "TXD/Effects.NFT");
    Effects_LoadDefCnt(ms_pEffects_B6F1DC, "Config/FxDefCnt.ct");   // "DefCntFile"
    Effects_LoadStore(ms_pEffects_B6F1D8, "Config/FxStore.st");     // "StoreFile"
    Effects_Load(ms_pEffects_B6F1E8, ms_bExtraEffects ? "Config/Extra/Effects2.bft" : "Config/Effects.bft");
    Fn_72E610();
    // "LogoMovies": the movie player's vf1 (the first movie, 2)
    void* player = MoviePlayer_Get();
    const char* movie = *reinterpret_cast<const re::Ptr32<const char>*>(0x009F470C);
    (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*, const char*, int)>>*>(player))[1](player, movie, 2);
    ms_bLogoMoviesPlaying = 0;
    LoadingScreen("Loading the Game", "stuff after the legal screen", nullptr);
    LoadAllShaders();
    // "TheRest"
    WorldSurfTable_Load(reinterpret_cast<void*>(0x00C21BC0));                                  // "WorldSurfTableFile"
    Hardness_Load(reinterpret_cast<void*>(0x00C1A7E8), "Config\\Dat\\Hardness.dat");            // "HardnessFile"
    ColStepEffects_Load(reinterpret_cast<void*>(0x020C8BA0), "Config/Dat/ColStpEf.dat");        // "CollisionStepEffectFile"
    SpecialFX_Init();                                                                           // "SpecialFX"
    TextManager_Init(reinterpret_cast<void*>(0x020C2FA4));                                      // "TextManager"
    Fn_44D320();
    Fn_45B8A0();
    Fn_571440();
    HandlingManager_Init(reinterpret_cast<void*>(0x00C2DED0));                                  // "HandlingManager"
    SurfaceTable_Load("Config\\Dat\\SURFACE.DAT");                                             // "SurfaceTable"
    Fn_6AEF70();
    Minigame_Init();                                                                            // "Minigame"
    return 1;
}

uint8_t __cdecl Initialise(const char* datFile) {
    // "CGameInitialise", "Page1"
    LoadingScreen("Loading the Game", "First loading screen", nullptr);
    MissionMgr_LoadBank(TheMissionMgr, "MissionBank", "Act/Missions.act");
    std::strcpy(aDatFile, datFile);
    Fn_46DBF0();
    PushMemoryTag(0x21);
    Fn_6D3EA0(reinterpret_cast<void*>(0x020C7C48));
    Fn_4A1100();
    Fn_4A83C0();
    PopMemoryTag();
    currLevel = 0;
    currArea = 0;
    LoadingScreen("CGame::Initialise", nullptr, nullptr);
    if (!ms_pPatrolPaths) {
        void* paths = Malloc(0x10);
        ms_pPatrolPaths = paths ? PatrolPaths_Construct(paths) : nullptr;
    }
    Fn_570710();
    PushMemoryTag(0x17);   // "CTxdStore"
    LoadingScreen("Loading the Game", "Loading generic textures", nullptr);
    ms_genericTxd = Txd_AddTxdSlot("generic");
    Txd_Fn_5F28C0(ms_genericTxd);
    Txd_AddRef(ms_genericTxd);
    LoadingScreen("Loading the Game", "Loading particles", nullptr);
    int particle = Txd_AddTxdSlot("particle");
    Txd_LoadTxdFile(particle, "TXD/PARTICLE.NFT");
    Txd_AddRef(particle);
    Txd_SetCurrentTxd(ms_genericTxd);
    LoadingScreen("Loading the Game", "Setup game variables", nullptr);
    PopMemoryTag();
    LoadingScreen(nullptr, "g_GameLogic.Init()", nullptr);
    GameLogic_Init(TheRestart);
    Fn_450850();
    LoadingScreen(nullptr, "theCameraManager()", nullptr);
    // "Page2"
    Camera_SetCamera(TheCamera, ms_camera);
    Camera_Init(TheCamera);
    Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
    Fn_523F40(reinterpret_cast<void*>(0x00C92D70));
    Fn_4160F0(reinterpret_cast<void*>(0x00BCC580));
    LoadingScreen(nullptr, "CDebug::DebugInitTextBuffer()", nullptr);
    ms_debugTextLines = 0;
    Fn_43FA00(reinterpret_cast<void*>(0x00BF6228));
    Fn_5360A0();
    Fn_538C50();
    Fn_51CE00();
    Fn_5649A0();
    LoadingScreen(nullptr, "CWorld::Initialise()", nullptr);
    PushMemoryTag(2);
    World_Initialise();
    PopMemoryTag();
    PushMemoryTag(0x17);
    Fn_520F10();
    PopMemoryTag();
    PushMemoryTag(4);
    Fn_6C3720();
    PopMemoryTag();
    LoadingScreen(nullptr, "InitModelIndices()", nullptr);
    PushMemoryTag(7);
    InitModelIndices();
    Fn_51C6A0();
    LoadingScreen(nullptr, "CPickups::Init()", nullptr);
    Pickups_Init();   // "Pickups"
    Fn_4C71E0();
    Collectibles_Init(reinterpret_cast<void*>(0x00BCEB50));   // "Collectibles"
    Fn_5FD140(reinterpret_cast<void*>(0x00BCCE80));
    Fn_425790(reinterpret_cast<void*>(0x009E5158));
    LoadingScreen(nullptr, "CdStreamAddImage('Stream/world.img')", nullptr);
    CdStreamAddImage("Stream/world.img");
    Fn_49A340();
    LoadingScreen(nullptr, "CFileLoader::LoadLevel('Config/Dat/default.dat')", nullptr);
    // "Page3", "Page3_A"
    if (!ms_bSkipLevel1) {
        FileLoader_LoadLevel("Config/Dat/default.dat");   // "LoadLevel1"
    }
    LoadingScreen(nullptr, "CFileLoader::LoadModelInfoFile('Config\\Extra\\shared.bin', 0)", nullptr);
    if (ms_bUseIpc) {
        FileLoader_LoadModelInfoFile("Config\\Extra\\shared.bin", 0);
    }
    LoadingScreen(nullptr, "CFileLoader::LoadLevel(pDatFile)", nullptr);
    if (!ms_bSkipLevel2) {
        FileLoader_LoadLevel(datFile);   // "LoadLevel2"
    }
    // "Page3_B", "PEDStats"
    PedStats_Init(ms_pPedStats);
    LoadingScreen(nullptr, "Screamer.InitialiseBankManager()", nullptr);
    PushMemoryTag(0x2E);   // "Audio", "BankMan"
    Screamer_InitialiseBankManager(TheScreamer);
    LoadingScreen(nullptr, "Screamer.Initialise()", nullptr);
    Screamer_Initialise(TheScreamer);   // "Screamer"
    LoadingScreen(nullptr, "Screamer.BankManager.StartDVDLoadingMusic()", nullptr);
    // ("StartDVDLoadMusic": empty in this build)
    PopMemoryTag();
    LoadingScreen(nullptr, "CObjectData::Initialise", nullptr);
    ObjectData_Initialise(ms_bExtraObjectData ? "Config\\Extra\\OBJECT2.DAT" : "Config\\Dat\\OBJECT.DAT");
    LoadingScreen(nullptr, "CStreaming::Init()", nullptr);
    // "Page4", "Page4_A"
    PushMemoryTag(2);
    Streaming_Init();   // "StreamingInit"
    // "SecondSwapModels"
    int player = -1;
    ModelInfo_GetModelInfo("player", &player);
    Streaming_SwapModel(player, "player");
    Streaming_SwapModel(CFileLoader::ms_firstClothModel, "B_Buzz");
    PopMemoryTag();
    LoadingScreen(nullptr, "GenerateAllBoundingBoxes", nullptr);
    GenerateAllBoundingBoxes(0);
    LoadingScreen(nullptr, "Misc", nullptr);
    Fn_72C410(reinterpret_cast<void*>(0x020C8A00));
    LoadingScreen(nullptr, "CScreenParticles::Particle2DInitialise()", nullptr);
    LoadingScreen(nullptr, "g_AccessoryManager.LoadAccessoryFile", nullptr);
    // "Page4_B", "Accessories"
    Accessories_Load(reinterpret_cast<void*>(0x00C2C730),
                     ms_bExtraAccessories ? "Config/Extra/accssor2.dat" : "Config/Dat/accssory.dat");
    LoadingScreen(nullptr, "g_WeaponAccessoryManager.LoadWeaponAccessoryFile", nullptr);
    WeaponAccessories_Load(reinterpret_cast<void*>(0x00C3AEF8), "Config/Dat/acswpn.dat");   // "Weapons"
    LoadingScreen(nullptr, "g_WeaponAccessoryManager.MatchLookupToIndex()", nullptr);
    WeaponAccessories_MatchLookupToIndex(reinterpret_cast<void*>(0x00C3AEF8));
    LoadingScreen(nullptr, "g_LiquidManager.LoadLiquidFile", nullptr);
    Liquids_Load(reinterpret_cast<void*>(0x00BC5DA8), "Config/Dat/liquids.dat");   // "Liquids"
    LoadingScreen("Loading the Game", "Add Particles", nullptr);
    VehicleColors_Load();   // "VehicleColors"
    PopMemoryTag();   // (the 7 from InitModelIndices)
    LoadingScreen("Loading the Game", "Setup paths", nullptr);
    // "Page5"
    Grids_Load(reinterpret_cast<void*>(0x00BF6228), "GRIDS.dat");
    LoadingScreen(nullptr, "CWorld::Player.Clear()", nullptr);
    WorldPlayer_Clear(TheWorldPlayer);
    LoadingScreen(nullptr, "theCameraManager().SetDefaultFOV( )", nullptr);
    Camera_SetDefaultFOV(TheCamera);
    LoadingScreen("Loading the Game", "Setup streaming", nullptr);
    Streaming_Fn_533460();
    LoadingScreen(nullptr, "CStreaming::LoadInitialPeds()", nullptr);
    LoadingScreen(nullptr, "CStreaming::RequestBigBuildings(LEVEL_GENERIC)", nullptr);
    Streaming_RequestBigBuildings(0);
    LoadingScreen(nullptr, "CStreaming::LoadAllRequestedModels()", nullptr);
    Streaming_LoadAllRequestedModels(0);
    LoadingScreen(nullptr, "CStreaming::RemoveIslandsNotUsed(currLevel)", nullptr);
    // (RemoveIslandsNotUsed(currLevel): empty in this build)
    LoadingScreen("Loading the Game", "Load animations", nullptr);
    LoadingScreen(nullptr, "CStreaming::LoadAllRequestedModels()", nullptr);
    Streaming_LoadAllRequestedModels(0);
    LoadingScreen(nullptr, "CPed::Initialise()", nullptr);
    Ped_Initialise();
    LoadingScreen(nullptr, "gCoverManager.Init()", nullptr);
    CoverManager_Init(reinterpret_cast<void*>(0x00C2B210));
    LoadingScreen(nullptr, "g_ReflectionManager.LoadData()", nullptr);
    ReflectionManager_LoadData(reinterpret_cast<void*>(0x00C92D70));
    LoadingScreen("Loading the Game", "Find big buildings", nullptr);
    FindBigBuildings();
    // "Page6"
    LoadingScreen("Loading the Game", "Setup game variables", nullptr);
    Fn_46EE70(reinterpret_cast<void*>(0x00C2AB70));
    LoadingScreen(nullptr, "radar->Initialize( )", nullptr);
    if (ms_pRadar) {
        Radar_Initialize(ms_pRadar);
    }
    LoadingScreen(nullptr, "pHUDInventory->Initialize()", nullptr);
    if (ms_pHUDInventory) {
        HUDInventory_Initialize(ms_pHUDInventory);
    }
    LoadingScreen(nullptr, "CWeapon::InitialiseWeapons()", nullptr);
    Weapon_InitialiseWeapons();
    LoadingScreen(nullptr, "CWeapon::RegisterCB()", nullptr);
    Weapon_RegisterCB();
    LoadingScreen(nullptr, "g_AccessoryManager.Initialize()", nullptr);
    Accessories_Initialize(reinterpret_cast<void*>(0x00C2C730));
    LoadingScreen(nullptr, "CPAnimModelInfo::InitPropButes()", nullptr);
    PAnimModelInfo_InitPropButes();
    Fn_55CD20(ms_pHUDItems_C674F0);
    LoadingScreen(nullptr, "cHUDItems::Init()", nullptr);
    HUDItems_Init();
    LoadingScreen(nullptr, "CGameCompletionMgr::Initialise()", nullptr);
    GameCompletionMgr_Initialise();
    LoadingScreen("Loading the Game", "Setup game variables", nullptr);
    Fn_46F7B0();
    Fn_448450();
    Fn_49F190();
    Fn_472C10();
    Fn_4A22D0();
    LoadingScreen(nullptr, "CDockManager::Initialise()", nullptr);
    DockManager_Initialise();
    Fn_4A65F0();
    Fn_4298A0();
    Fn_45C0F0();
    // "Page7", "Page7_A"
    Fn_44EAD0();
    LoadingScreen(nullptr, "CAmbientVehicles::Initialise()", nullptr);
    PushMemoryTag(0x1C);
    AmbientVehicles_Initialise();
    PopMemoryTag();
    LoadingScreen(nullptr, "CCoronas::Init()", nullptr);
    Coronas_Init();
    Fn_525030();
    Fn_4DB2A0();
    Fn_5276F0();
    // "Page7_B", "Stats_Init"
    LoadingScreen(nullptr, "CStats::Initialise()", nullptr);
    Stats_Initialise();
    PedMgr_Init(reinterpret_cast<void*>(0x00C2B300));   // "PedMgr"
    LoadingScreen("Loading the Game", "Load scripts", nullptr);
    // "SurfDataMgr"
    LoadingScreen(nullptr, "SurfaceDataMgr::surfaceDataMgr()", nullptr);
    SurfaceDataMgr_Init();
    // "Page7_C"
    Fn_6CBCA0(reinterpret_cast<void*>(0x020C5F70));
    Fn_4E9290(reinterpret_cast<void*>(0x00C3B808));
    LoadingScreen("Loading the Game", "Setup game variables", nullptr);
    Fn_426020(20000);
    Fn_510820();
    LoadingScreen(nullptr, "g_FlashLights.Init()", nullptr);
    FlashLights_Init(reinterpret_cast<void*>(0x00C66B78));
    Fn_4C9920();
    // "Page7_D"
    LoadingScreen("Loading the Game", "Credits, prop action trees", nullptr);
    ModelInfo_SetupPropActionTrees();   // "SetupActTrees"
    LoadingScreen("Loading the Game", "Start script", nullptr);
    return 1;
}

void __cdecl InitialiseScripts() {
    if (ms_bLateInitDone) {
        return;
    }
    ms_bLateInitDone = 1;
    Fn_570470();
    // "ScriptMgrInit"
    LoadingScreen("Loading the Game", "Init Catdog Scripts", nullptr);
    PushMemoryTag(0x23);
    LurStore_InitScripts(TheLurStore);
    PopMemoryTag();
    LoadingScreen(nullptr, "CModelInfo::LoadDefaultPropActFiles()", nullptr);
    ModelInfo_LoadDefaultPropActFiles();   // "LoadActFiles"
    PushMemoryTag(0x23);
    if (!ms_bRestartingScripts) {
        LurStore_Update(TheLurStore, 0);   // "ScriptMan_Update"
    }
    PopMemoryTag();
    // "Page8"
    LoadingScreen("Loading the Game", "Load scene", nullptr);
    LoadScene();
    LoadingScreen("Loading the Game", "Setup traffic lights & stop signs", nullptr);
    TrafficLights_Setup(reinterpret_cast<void*>(0x00C1A7DC));
    LoadingScreen(nullptr, "ActionContext::Init()", nullptr);
    ActionContext_Init();
    if (FindPlayerPed_ptr) {
        Ped_Fn_474090(FindPlayerPed_ptr);
    }
    LoadingScreen(nullptr, "CModelInfo::SetupPropActionTrees()", nullptr);
    ModelInfo_SetupPropActionTrees();
    LoadingScreen(nullptr, "CWalkableMeshQueryService::Initialize()", nullptr);
    WalkableMeshQueryService_Initialize();
    LoadingScreen(nullptr, "CModelInfo::DropIndex()", nullptr);
    ModelInfo_DropIndex();
    Fn_5F0A60();
    ms_bReady = re::PtrCast<uint8_t*>(ms_pCompletion)[0x270] == 0;
}

void __cdecl ReInitGameObjectVariables() {
    Fn_7DB6B0(reinterpret_cast<void*>(0x00BA6C4C));
    Fn_55DFE0();
    Fn_5BB390();
    Minigame_Init();
    Fn_6CBCA0(reinterpret_cast<void*>(0x020C5F70));
    Fn_4E9290(reinterpret_cast<void*>(0x00C3B808));
    Fn_416020(reinterpret_cast<void*>(0x00BCC580));
    Fn_425230(reinterpret_cast<void*>(0x009E5158));
    Fn_561040(reinterpret_cast<void*>(0x00CF1F60));
    Fn_668150(ms_pEffects_B6F1D4);
    Fn_453D40(reinterpret_cast<void*>(0x00C13FD8));
    Accessories_Initialize(reinterpret_cast<void*>(0x00C2C730));
    GameLogic_Init(TheRestart);
    if (!ms_bRestartingScripts) {
        Camera_SetCamera(TheCamera, ms_camera);
        Camera_Init(TheCamera);
    }
    ms_debugTextLines = 0;
    Fn_5360A0();
    HUDItems_Init();
    Radar_Fn_549180(ms_pRadar);   // (unchecked here)
    Fn_6A4E20();
    Fn_41EB70();
    KeepLoadingScreenAlive(1, 0);
    Fn_6AEF70();
    KeepLoadingScreenAlive(1, 0);
    Camera_SetDefaultFOV(TheCamera);
    Streaming_RequestBigBuildings(0);
    Streaming_LoadAllRequestedModels(0);
    Fn_51B800();
    Ped_Initialise();
    Weapon_InitialiseWeapons();
    KeepLoadingScreenAlive(1, 0);
    Fn_6D3BB0(reinterpret_cast<void*>(0x020C7C48));
    KeepLoadingScreenAlive(1, 0);
    Fn_448450();
    KeepLoadingScreenAlive(1, 0);
    Fn_49DF80();
    Fn_4A2800();
    Fn_46EC80();
    Fn_472BF0();
    Fn_4A6AC0();
    Fn_44E6A0();
    Fn_4298A0();
    Fn_45C0F0();
    Fn_48AE70(reinterpret_cast<void*>(0x00C2B300));
    KeepLoadingScreenAlive(1, 0);
    ms_liquids_BC5DF8 = 0;
    Liquids_Load(reinterpret_cast<void*>(0x00BC5DA8), "Config/Dat/liquids.dat");
    KeepLoadingScreenAlive(1, 0);
    WorldPlayer_Clear(TheWorldPlayer);
    GenerateAllBoundingBoxes(0);
    Fn_426020(20000);
    Fn_4C71E0();
    Pickups_Init();
    Fn_43FA40(reinterpret_cast<void*>(0x00BF6228));
    Fn_4C9530();
    KeepLoadingScreenAlive(1, 0);
    SpecialFX_Init();
    KeepLoadingScreenAlive(1, 0);
    Collectibles_Init(reinterpret_cast<void*>(0x00BCEB50));
    KeepLoadingScreenAlive(1, 0);
    Fn_5FD140(reinterpret_cast<void*>(0x00BCCE80));
    Fn_425790(reinterpret_cast<void*>(0x009E5158));
    Fn_520E50();
    Fn_52F600();
    Fn_72C410(reinterpret_cast<void*>(0x020C8A00));
    Fn_43F780(reinterpret_cast<void*>(0x00BF4508));
    GameCompletionMgr_Initialise();
    Stats_Initialise();
    PushMemoryTag(0x23);
    KeepLoadingScreenAlive(1, 0);
    LurStore_InitScripts(TheLurStore);
    KeepLoadingScreenAlive(1, 0);
    LurStore_Update(TheLurStore, 1);
    PopMemoryTag();
    KeepLoadingScreenAlive(1, 0);
    Fn_6C38A0();
    ActionContext_Init();
    if (FindPlayerPed_ptr) {
        Ped_Fn_47D2F0(FindPlayerPed_ptr);
    }
    Fn_68F340(reinterpret_cast<void*>(0x0208D388));
    Fn_4545C0(reinterpret_cast<void*>(0x00A147E0), 0);
    if (ms_pRadar) {
        Radar_Fn_54AD80(ms_pRadar);
    }
    KeepLoadingScreenAlive(1, 0);
    Fn_50DE30(reinterpret_cast<void*>(0x00A45A30));
    ms_bUnknownFDC = 0;
}

void __cdecl InitialiseWhenRestarting() {
    Fn_45B8A0();
    Camera_Init(TheCamera);
    Game_ReInitGameObjectVariables();
    if (ms_bRestartingScripts) {
        ms_bRestartingScripts = 0;
        KeepLoadingScreenAlive(1, 0);
    }
    GenerateAllBoundingBoxes(0);
}

void __cdecl Process() {
    Fn_738610();
    Fn_6C44B0();
    if (!ms_bPaused_C1A999 && (!ms_bInFrontEnd_20C5BE6 || ms_bFrontEnd_20C5BE5)) {
        Fn_6A5F20();
    }
    if (ms_bUserStart && (ms_gameState == 0 || ms_gameState == 6 || ms_gameState == 8) && ms_pCompletion) {
        Fn_7379A0();
    }
    Restart_Process(TheRestart);
    // (a debug pad combination: a memory report)
    if (Pad_GetValue(0xC, 0) != 0.0f && Pad_GetValue(0xD, 0) != 0.0f && Pad_GetValue(3, 0) != 0.0f) {
        Sprintf(ms_textBuffer, "meminfo%02dh%02dm%02ds.xml", 0, 0, 0);
        DumpMemInfo(1);
    }
    Txd_RemoveUnused();
    uint32_t before = Timer_GetCurrentTime();
    Streaming_Update();
    uint32_t streamingTime = Timer_GetCurrentTime() - before;
    Fn_5E54B0(reinterpret_cast<void*>(0x00D12AF8));
    Fn_5E6920(reinterpret_cast<void*>(0x00D12AF8));
    if (!ms_bPaused_C1A99A && !ms_bPaused_C1A999 && !ms_bPaused_C1A998) {
        Fn_417820(reinterpret_cast<void*>(0x00BCC580));
        Fn_488350(reinterpret_cast<void*>(0x00C2B210));
        Fn_6AEB40(reinterpret_cast<void*>(0x00BA6C4C));
        Fn_418A00(reinterpret_cast<void*>(0x00BCCE80));
        Fn_72E090(reinterpret_cast<void*>(0x020C8A00));
        if (!ms_bInFrontEnd_20C5BE6) {
            Fn_4E8E10();
            Fn_6CA7B0(reinterpret_cast<void*>(0x020C5F70));
            if (!Fn_514570()) {
                MissionMgr_Process(TheMissionMgr);
            }
            Fn_424DD0(reinterpret_cast<void*>(0x00BCD098));
            Fn_45C3C0(Fn_45C950());
        }
        Fn_68A250();
        Fn_4260D0();
        if (void* callback = ms_pScriptCallback) {
            if (reinterpret_cast<uint8_t*>(callback)[4]) {
                (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*)>>*>(callback))[1](callback);
            }
        }
        Fn_536320();
        PushMemoryTag(0x23);
        LurStore_Update(TheLurStore, 0);
        PopMemoryTag();
        Fn_527770();
        Fn_521390();
        Effects_BD0CF8_Process(reinterpret_cast<void*>(0x00BD0CF8));
        Fn_537E00(reinterpret_cast<void*>(0x00CE7580));
        Fn_454500(reinterpret_cast<void*>(0x00C13FD8));
        Fn_667EA0(ms_pEffects_B6F1D4);
        Fn_4A7A40(reinterpret_cast<void*>(0x00C2C730));
        Fn_43A320(reinterpret_cast<void*>(0x00BD5C90));
        Fn_48B890(reinterpret_cast<void*>(0x00C2B300));
        Fn_44FFB0(ms_pObj_C0F78C);
        Fn_46F800(ms_pObj_C2AF9C);
        Fn_4729A0(ms_pObj_C2B034);
        if (streamingTime <= 4) {
            Timer_GetCurrentTime();
            Vehicles_Process(ms_pVehicles_C2C108, 1);
            Timer_GetCurrentTime();
        } else {
            // (a byte flag: the game pushes the whole dword, its upper bytes whatever the
            // stack slot held)
            Vehicles_Process(ms_pVehicles_C2C108, Camera_Fn_4F12F0(TheCamera) != 0);
        }
        Fn_422970(ms_pObj_BCD080);
        Fn_449390(ms_pObj_C0F5E4);
        Fn_4A29B0(ms_pObj_C2C12C);
        Fn_4A6AD0(ms_pObj_C2C1A0);
        Fn_4A69A0();
        if (!ms_bCutsceneRunning) {
            Fn_4C7C40();
        }
        Fn_68E440();
        Fn_683A50();
        Anim_Fn_6C0A30(TheAnimManager);
        PushMemoryTag(0x19);
        Fn_45FAB0();
        PopMemoryTag();
        uint8_t* ped = static_cast<uint8_t*>(FindPlayerPed_ptr);
        uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
        const Vec3* pos = reinterpret_cast<const Vec3*>(matrix ? matrix + 0x30 : ped + 4);   // (CPlaceable::GetPosition)
        Fn_4440F0(pos, 0.75f, 0);
        Fn_445AC0();
        Fn_4D9400();
        Fn_4579A0(reinterpret_cast<void*>(0x00C18748));
        Fn_4C9B40();
        Fn_6B0780();
        Fn_538FC0();
        Camera_SetCamera(TheCamera, ms_camera);
        Camera_Process(TheCamera);
        if (ms_cameraMode_C3CCF8 == 2) {
            Ped_Fn_49AEF0(FindPlayerPed_ptr);
        }
        Fn_512540();
        Fn_510EA0();
        Fn_526700();
        PushMemoryTag(0x1C);
        Fn_513030();
        PopMemoryTag();
        Fn_45A6B0();
    } else if (ms_missionActive_20C87AC) {
        MissionMgr_Process(TheMissionMgr);
        if (void* callback = ms_pScriptCallback) {
            if (reinterpret_cast<uint8_t*>(callback)[4]) {
                (*reinterpret_cast<re::Ptr32<re::Ptr32<void __thiscall(void*)>>*>(callback))[1](callback);
            }
        }
    }
    if (ms_bShowCompletion) {
        MissionMgr_ShowCompletion(TheMissionMgr);
    }
}

void __cdecl TidyUpMemory(int moveTextures, int) {
    EnterCriticalSection(re::GameSection(ms_modelInfoSection));
    ms_modelInfoLock = 0;
    if (re::Flag(moveTextures)) {
        ms_bTidiedTextures = 1;
    }
    Anim_Fn_6C14B0(TheAnimManager, 1);
    for (int i = 0; i < 12200; i++) {
        void* mi = ms_modelInfoPtrs[i];
        if (!mi) {
            continue;
        }
        // (vf5: the model's object; kind 2 has its unused data dropped)
        uint8_t* object = (*reinterpret_cast<re::Ptr32<re::Ptr32<uint8_t* __thiscall(void*)>>*>(mi))[5](mi);
        if (object && *object == 2) {
            Fn_6C7380(object, Fn_6CA7D0, 0);
        }
    }
    ms_modelInfoLock = 1;
    LeaveCriticalSection(re::GameSection(ms_modelInfoSection));
}

uint8_t __cdecl IsAreaInterior(int area) {
    return ms_areaIsInterior[area];
}

void __cdecl ShutDownForRestart() {
    LoadingScreen("CGame::ShutDownForRestart()", "Start of method", nullptr);
    ms_bReady = 0;
    Fn_50DEB0(reinterpret_cast<void*>(0x00A45A30));
    Fn_683C00();
    Fn_68F230(reinterpret_cast<void*>(0x0208D388));
    Mission_Fn_6ACA70(MissionMgr_Fn_6AA690(TheMissionMgr));
    Mission_Fn_6AA300(MissionMgr_Fn_6AA690(TheMissionMgr), 0, 0, 0, 0, 0, 0, 0);
    Mission_Fn_6ACA70(MissionMgr_Fn_6AA680(TheMissionMgr));
    Mission_Fn_6AA300(MissionMgr_Fn_6AA680(TheMissionMgr), 0, 0, 0, 0, 0, 0, 0);
    // the first mission's flag +0x33 cleared (the name as the game's string class)
    alignas(4) uint8_t name[4];
    String_Construct(name, "1_01");
    int mission = MissionMgr_FindMission(TheMissionMgr, name);
    // (the string's destructor: the empty 0x49A650)
    MissionMgr_GetMission(TheMissionMgr, mission)[0x33] = 0;
    Fn_704C60(1);
    Camera_Init(TheCamera);
    Screamer_Fn_5962E0(TheScreamer);
    Fn_4244B0(reinterpret_cast<void*>(0x00BCD098));
    Fn_45C870(Fn_45C950());
    MissionMgr_Fn_6AB7B0(TheMissionMgr);
    Fn_6AE260(reinterpret_cast<void*>(0x020C4420));
    Pickups_Init();
    Fn_5140C0();
    Fn_6CB620();
    if (ms_bFlag_C18754) {
        Fn_457560(reinterpret_cast<void*>(0x00C18748));
    }
    Fn_455B60(reinterpret_cast<void*>(0x00C18748));
    Fn_48B430(reinterpret_cast<void*>(0x00C2B300));
    KeepLoadingScreenAlive(1, 0);
    Fn_45D600();
    WorldPlayer_Clear(TheWorldPlayer);
    KeepLoadingScreenAlive(1, 0);
    Streaming_Fn_533980();
    KeepLoadingScreenAlive(1, 0);
    Fn_499CD0();
    Fn_5287F0();
    Fn_458140();
    KeepLoadingScreenAlive(1, 0);
    Game_TidyUpMemory(1, 0);
}

void __cdecl DrasticTidyUpMemory(int flag) {
    if (ms_bDrasticTidyUp) {
        return;
    }
    ms_bDrasticTidyUp = 1;
    LoadingScreen("CGame::DrasticTidyUpMemory", "Start", nullptr);
    Game_TidyUpMemory(1, flag);
    if (*ms_pFreeMemory < 500000 && !ms_bNoBigBuildingTidy) {
        Fn_56F260();
        Game_TidyUpMemory(1, flag);
        Vec3 coors;
        const float* found = FindPlayerCoors(&coors);
        float xy[2] = {found[0], found[1]};
        Fn_56FE00(xy);
        LoadingScreen("CGame::DrasticTidyUpMemory", "BigBuildings", nullptr);
    }
    LoadingScreen("CGame::DrasticTidyUpMemory", "LoadAllRequestedModels", nullptr);
    Streaming_LoadAllRequestedModels(0);
    ms_bDrasticTidyUp = 0;
    LoadingScreen("CGame::DrasticTidyUpMemory", "End", nullptr);
}

const char* __cdecl GetAreaName(uint32_t area) {
    if (area > 63) {
        return reinterpret_cast<const char*>(0x00903168);  // (the exe's "INVALID")
    }
    return ms_areaNames[area];
}

} // namespace CGame

namespace {
GAME_FN(int, __cdecl, FindNameInList, 0x0073AC90, const re::Ptr32<const char>* names, int count, const char* name, int fallback, int);
} // namespace

int __cdecl CGame::GetAreaFromName(const char* name) {
    return FindNameInList(ms_areaNames, 64, name, -1, 0);
}

uint32_t __cdecl VectorsDiffer(const float* a, const float* b) {
    return !(a[0] == b[0] && a[1] == b[1] && a[2] == b[2]);
}

// ---- registration ----------------------------------------------------------------------

#define GAME_REVERSED(name, fn, address)                                                            \
    static void* Original_##fn;                                                                     \
    REVERSED(name, address, RE_BODY(&fn), nullptr, &Original_##fn, re::Mode::Original)

namespace CGame {
GAME_REVERSED("CGame::FinalShutdown", FinalShutdown, 0x0042E4C0);
GAME_REVERSED("CGame::Shutdown", Shutdown, 0x0042E4D0);
GAME_REVERSED("CGame::ClearTidiedTextures", ClearTidiedTextures, 0x0042E6B0);
GAME_REVERSED("CGame::IsSpecialArea", IsSpecialArea, 0x0042E6C0);
GAME_REVERSED("CGame::ShouldMovePlayer", ShouldMovePlayer, 0x0042E700);
GAME_REVERSED("CGame::SetMovePlayer", SetMovePlayer, 0x0042E710);
GAME_REVERSED("CGame::ClearMovePlayer", ClearMovePlayer, 0x0042E720);
GAME_REVERSED("CGame::LoadPropHXDs", LoadPropHXDs, 0x0042E790);
GAME_REVERSED("CGame::InitialiseRenderWare", InitialiseRenderWare, 0x0042E970);
GAME_REVERSED("CGame::ShutdownRenderWare", ShutdownRenderWare, 0x0042ED70);
GAME_REVERSED("CGame::InitialiseOnceAfterRW", InitialiseOnceAfterRW, 0x0042EDD0);
GAME_REVERSED("CGame::Initialise", Initialise, 0x0042EFE0);
GAME_REVERSED("CGame::InitialiseScripts", InitialiseScripts, 0x0042FA20);
GAME_REVERSED("CGame::ReInitGameObjectVariables", ReInitGameObjectVariables, 0x0042FB90);
GAME_REVERSED("CGame::InitialiseWhenRestarting", InitialiseWhenRestarting, 0x0042FE70);
GAME_REVERSED("CGame::Process", Process, 0x0042FEB0);
GAME_REVERSED("CGame::TidyUpMemory", TidyUpMemory, 0x004302A0);
GAME_REVERSED("CGame::IsAreaInterior", IsAreaInterior, 0x00430320);
GAME_REVERSED("CGame::ShutDownForRestart", ShutDownForRestart, 0x00430330);
GAME_REVERSED("CGame::DrasticTidyUpMemory", DrasticTidyUpMemory, 0x004304E0);
GAME_REVERSED("CGame::GetAreaName", GetAreaName, 0x004305B0);
GAME_REVERSED("CGame::GetAreaFromName", GetAreaFromName, 0x004305D0);
} // namespace CGame
GAME_REVERSED("VectorsDiffer", VectorsDiffer, 0x004305F0);
