#pragma once

// CGame: the game's top level (GTA San Andreas' CGame, stage for stage): the
// initialisation in its stages (before RenderWare - here Gamebryo -, the renderer's,
// once after it, the game's), the frame's Process, the restarts and shutdowns; the
// areas' names and the loading screen.
// The game's profiler markers (0x49A650, an empty function in the release build,
// called with the stage names seen in the comments) are left out.

#include <cstdint>

#include "reversed.h"

namespace CGame {

GAME_VAR(int32_t, currArea, 0x00BD1008);            // 0 outside, interiors otherwise
GAME_VAR(int32_t, currLevel, 0x00BD0FC4);           // (RemoveIslandsNotUsed's, always 0)
GAME_ARRAY(char, aDatFile, 0x00BD0FA4);             // the level file Initialise loaded
GAME_VAR(int32_t, ms_genericTxd, 0x00BD0FA0);       // "generic"'s texture dictionary
GAME_VAR(void*, ms_camera, 0x00C1AE7C);             // the main camera (made by InitialiseRenderWare)
GAME_VAR(void*, ms_cameraData, 0x00C1AE78);         // (freed by ShutdownRenderWare)
GAME_VAR(uint8_t, ms_bDrasticTidyUp, 0x00BD1060);   // DrasticTidyUpMemory running (no reentry)
GAME_VAR(uint8_t, ms_bLateInitDone, 0x00BD0FD2);    // InitialiseScripts ran
GAME_VAR(uint8_t, ms_bTidiedTextures, 0x00BD0FD1);  // TidyUpMemory(true) happened
GAME_VAR(uint8_t, ms_bMovePlayer, 0x00BD0FD3);      // the player is to be moved (a restart)
GAME_VAR(uint8_t, ms_bUnknownFDC, 0x00BD0FDC);      // (cleared by ReInitGameObjectVariables)
GAME_VAR(uint8_t, ms_bReady, 0x00BD0FCB);           // (set after the scripts load; cleared on a restart)
GAME_ARRAY(uint8_t, ms_areaIsInterior, 0x00901DD0); // [64]
GAME_ARRAY(const char*, ms_areaNames, 0x00903058);  // [64] "MainMap", ...
GAME_ARRAY(const char*, ms_hxdNames, 0x009F4718);   // [18] the animation HXDs loaded at start

// 0x42E4C0: the texture dictionaries, the ped stats and the archive reader
void __cdecl FinalShutdown();
// 0x42E4D0: everything the game made, in turn (true)
uint8_t __cdecl Shutdown();
// 0x42E6B0
void __cdecl ClearTidiedTextures();
// 0x42E6C0: the areas (interiors) the game treats as shops/specials
uint8_t __cdecl IsSpecialArea(int area);
// 0x42E700 / 0x42E710 / 0x42E720: the "move the player" flag
uint8_t __cdecl ShouldMovePlayer();
void __cdecl SetMovePlayer();
void __cdecl ClearMovePlayer();
// 0x42E790: Anim\hxds.dat and Config\Dat\PropHXDs.dat: the props' animation files,
// slots from 18 on
void __cdecl LoadPropHXDs();
// 0x42E970: the camera, the loading screen, fonts, the animation files (false
// when the camera can't be made)
uint8_t __cdecl InitialiseRenderWare();
// 0x42ED70
void __cdecl ShutdownRenderWare();
// 0x42EDD0: effects, the logo movies, the data files the game needs once
uint8_t __cdecl InitialiseOnceAfterRW();
// 0x42EFE0: the game, from a level file (true)
uint8_t __cdecl Initialise(const char* datFile);
// 0x42FA20: the scripts, the scene, the action trees: once, when the front end is done
void __cdecl InitialiseScripts();
// 0x42FB90: the game's objects back to their start (a new game)
void __cdecl ReInitGameObjectVariables();
// 0x42FE70
void __cdecl InitialiseWhenRestarting();
// 0x42FEB0: a frame of the game
void __cdecl Process();
// 0x4302A0: the animation manager's unused data, and the models' (under the game's
// lock); the second argument is unused
void __cdecl TidyUpMemory(int moveTextures, int unused);
// 0x430320
uint8_t __cdecl IsAreaInterior(int area);
// 0x430330
void __cdecl ShutDownForRestart();
// 0x4304E0: TidyUpMemory, and with little memory left the big buildings far away too
void __cdecl DrasticTidyUpMemory(int flag);
// 0x4305B0: "INVALID" past 63
const char* __cdecl GetAreaName(uint32_t area);
// 0x4305D0: -1 when it isn't one
int __cdecl GetAreaFromName(const char* name);

} // namespace CGame

// 0x4305F0: two vectors differ (cdecl, CVector's operator!=)
uint32_t __cdecl VectorsDiffer(const float* a, const float* b);

// The loading screen (0x43C4F0 on): the texture it shows and the progress bar.
namespace CLoadingScreen {
// 0x43AD40
uint32_t __cdecl IsActive();
} // namespace CLoadingScreen
