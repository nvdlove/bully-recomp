#pragma once

#include <cstdint>

// Switches the host program sets before the game's code runs. The 32-bit build leaves them
// off (the hooked game behaves as it always has); bullyre_x64 turns them on.
namespace re {

// The game kept in a window at the desktop's display mode: no fullscreen window, no display
// mode change, no minimising when it loses focus. bullyre_x64's test runs mustn't change the
// user's monitor.
bool ForceWindowed();
void SetForceWindowed(bool on);

// With ForceWindowed (bullyre_x64 played): the game in an ordinary Windows window
// (src/game/PlayWindow.cpp): sized at its edges and maximized like any window (the game's
// resolution follows, by bullygfx's resizer), the keyboard and mouse shared with Windows (the
// Windows key works; the cursor held in the window only while playing), no input while another
// window is in front. PlayWindowCreated: the main window made; PlayWindowFrame: once a frame;
// GameInputAllowed: the game's input devices are to be read (false: as if nothing were pressed).
bool PlayWindow();
void SetPlayWindow(bool on);
void PlayWindowCreated(void* window);
void PlayWindowFrame();
bool GameInputAllowed();

// The game kept active when its window isn't in front (a test run started in the
// background never gets the focus, and the game waits for it at the loading screen).
bool KeepActive();
void SetKeepActive(bool on);

// (bullyre_x64's test runs) the game's window shown without being activated, and its requests
// for the foreground ignored: a test run never takes the focus from what the user is doing (nor
// gets the user's typing: the keyboard is read exclusive-foreground)
bool Background();
void SetBackground(bool on);

// The side of the light manager's shadow maps (its spot lights' NiPCFShadowTechnique ones):
// the game's 1024 unless set (bullyre_x64: 2048, --shadow-map N). Each light asks for its own
// size (the game's all 1024), scaled by this over 1024 and capped at the shadow manager's 2048.
uint16_t ShadowMapSize();
void SetShadowMapSize(uint16_t size);

// Screens wider than 16:9, up to 32:9 (bullyre_x64): the settings check takes them (the game's
// stops at 1.8, and falls back to 800x600), the 3D view gets wider (the same view up and down
// as 16:9, more at the sides) and the HUD keeps its shape (squeezed as the game squeezes it for
// 16:9, out to the screen's edges)
bool UltraWide();
void SetUltraWide(bool on);
// The screen's aspect over 16:9 when UltraWide and the screen is wider, else 1 (from the
// display settings' mode: DisplaySettings.cpp)
float WiderThan169();

// Seamless doors (bullyre_x64; --no-seamless-doors turns them off): the interior behind the door
// the player is near loaded beside the current area, and its door taking the player in without
// the fade to black (SeamlessDoors.cpp). SeamlessDoorsFrame runs each frame from the area
// transition manager's Update; SeamlessDoorSkipsFade(area) is asked when a door's transition
// would fade.
bool SeamlessDoors();
void SetSeamlessDoors(bool on);
// With SeamlessDoors: both areas at a door peopled (the second population pass; on by default)
bool TwoAreas();
void SetTwoAreas(bool on);
void SeamlessDoorsFrame();
bool SeamlessDoorSkipsFade(int area);
void SeamlessDoorArrived();
// (MdBullyMainViewRenderClick::RenderScene, the main pass: after the world, before the close-up
// layer) the interior by the player drawn through its open door
void SeamlessDoorsDraw(void* culler, void* scene, void* sorter, void* accumulator);
// (--portal-test) the door looked through not drawn, as if open: the doorway can be checked
// where the door won't open (a new game's shops)
bool PortalTest();
void SetPortalTest(bool on);
// (--portal-full) the doorway made the whole screen: what the camera through it sees
bool PortalFull();
void SetPortalFull(bool on);
// (--no-portal) doors without the other side drawn through them (step 1 only)
bool Portals();
void SetPortals(bool on);
// (the seamless doors' pass) CRenderer::ScanWorld from a camera through a door: only what that
// camera sees (not the player's sector, nor the big buildings: those are the town's, seen from
// far away they'd be let go of and made again for the main view the next frame)
bool PortalScan();
void SetPortalScan(bool on);
// (CameraShotTrack / SetCameraTrack::Start) the door's own camera shot left out: the player is at
// a door whose other side is drawn through it, so the camera stays behind them (`what`: the
// track, for --doors)
bool SeamlessDoorHoldsCamera(const char* what);
// (AreaTransitionManager::UpdateTransition, the door's countdown) the player is through the
// doorway whose other side is drawn: the countdown ends there, for the switch to follow at once
// (past the doorway's plane the player is behind the other side's picture)
bool SeamlessDoorCrossed();
// (AreaTransitionManager::UpdateTransition, the switch) through a door that took the player in
// without a fade: the place just inside that the doorway maps the player's own to (the player
// carries on from there, not from the route's place further in), the heading turned likewise, and
// the direction the camera looked in, turned (false: as before, the route's place)
bool SeamlessDoorPlacement(float* position, float* heading, float* look);
// (AreaTransitionManager::UpdateTransition, the switch: the new area set, the world not yet
// cleared for it) through a door taken without a fade: the area left kept loaded around its door
// and drawn around the doorway while the camera, behind the player, is still on its side
void SeamlessDoorSwitch();
// (FollowCameraBase's collision pass) the camera not kept out of walls: just through a seamless
// door, the camera is still on the side left, outside the new area's walls
bool SeamlessCameraFree();
// (FollowCameraController3::Place) the camera's place: at the switch, where the camera was,
// carried through the doorway (once; false when there is none)
bool SeamlessCameraPlace(float* position);
// Two areas live at a seamless door (the remaster's): the area behind the door the player is by
// (or the one just left, while its door is near) peopled too, around where the doorway maps the
// player to there (CPedPopulation::Update's second pass, PopulationPeds.cpp).
//   SeamlessPopulationGhost: that area and place (false: no second area now).
//   SeamlessGhostCamera: CCamera made the main camera moved through the doorway (on), and back
// (the second pass's "in view" is through the door); false when it can't be.
//   SeamlessKeepsPeds: the switch of a seamless door (PedArrivals::AreaChanged and StopPeds leave
// the peds: both sides' go on).
bool SeamlessPopulationGhost(int* area, float* at);
bool SeamlessGhostCamera(bool on);
bool SeamlessKeepsPeds();
// (CCamera::IsSphereVisible in the second population pass) a sphere in the area there seen through
// the doorway from the main camera (all of it when the camera is past the doorway already)
bool SeamlessSeenThroughDoorway(const float* centre, float radius);
// The population's pass (0: the only one; 1: the current area's, with a second; 2: the second's),
// its centre (FindPlayerCentreOfWorld's answer in pass 2; null otherwise), and whether a ped is
// the other pass's (nearer the other centre: not culled, counted or removed by this one)
int PopulationPass();
const float* PopulationCentre();
bool PopulationSkipsPed(const uint8_t* ped);
// Where the light manager's update measures its lights from instead of the player (null: the
// player): the seamless doors' pass lights the area through a door as the player would be in it
const float* LightsFrom();
void SetLightsFrom(const float* position);

// (bullyre_x64 --doors) the doors of each area the player arrives in listed, and each door
// transition's states timed (stderr): SeamlessDoors.cpp
bool DoorReport();
void SetDoorReport(bool on);
void ReportDoors();
// (--use-door S:AREA) the nearest door to AREA used at S seconds on the keys' clock, as the door
// code does once a door opens (AreaTransitionManager::SetTarget): the transition from there on
// is a door's, whether or not the door would open
void AddScriptedDoorUse(uint32_t atMs, int area);
void UseDoor(int area);

// A picture of the next frame asked for (the watchdog's): the renderer's Present takes it,
// TakeFrameGrab() true once per request, and writes the back buffer to FrameGrabPath()
void RequestFrameGrab(const char* path);

// Keys a test run presses (bullyre_x64 --keys): a DirectInput key code held from `atMs` (from the
// first one added) for `forMs`. With any, the game's keyboard reads (PlayerInput's, the DI8
// keyboard's) start from all keys up when the device can't be read (a background window) and
// get the scripted ones down (0x80). Codes 0x100-0x103 are the mouse's buttons (the game's own
// numbering: 0x100 the left), pressed in PlayerInput's mouse reads the same way.
void AddScriptedKey(uint16_t code, uint32_t atMs, uint32_t forMs);

// Teleports a test run asks for (bullyre_x64 --teleport): at `atMs` (the keys' clock) the player
// moved to (x, y, z) in `area`, as the script function PlayerSetPosXYZArea does (a request to
// the area transition manager). RunScheduled() carries them out on the main thread (MainLoop::Idle).
void AddScriptedTeleport(uint32_t atMs, float x, float y, float z, int area, float heading);
// A test run's quit (bullyre_x64 --quit): at `atMs` the front end's Quit chosen (FEQuitGame's
// OnEnter, 0x6998B0: the main loop ends and the game shuts down), from RunScheduled().
void AddScriptedQuit(uint32_t atMs);
// Missions a test run starts (bullyre_x64 --missions): at `atMs` the missions' table written to
// bullyre_x64_missions.txt and the first of `missions` force-started as the script function
// ForceStartMissionIndex does (a running one ended first), then every `everyMs` the next;
// CurrentScriptedMission() names the one started last
void AddScriptedMissions(uint32_t atMs, uint32_t everyMs, const int* missions, int count);
// (--mission-pass P) each one passed `afterMs` after its start (the script function MissionSucceed)
void SetScriptedMissionPass(uint32_t afterMs);
const char* CurrentScriptedMission();
// (bullyre_x64 --tag T) the files a run writes in the game's folder named bullyre_x64_T_<what>
// instead of bullyre_x64_<what>: test runs side by side
void SetOutputTag(const char* tag);
const char* OutputName(const char* what);
void RunScheduled();

// The renderer's geometry draws (DrawIndexedPrimitive), counted for the bring-up's watchdog:
// CountDraw(hr) returns hr; DrawCounts gives the calls and the failed ones so far
long CountDraw(long hr, int site = 0);

// (x64 bring-up) a hardware watch on writes to 4 bytes at `address` in the calling thread
// (DR0; nullptr: off): a debugger stops at the writing instruction (single step)
void WatchWrites(const void* address);
uint32_t DrawSiteCount(int site);
void DrawCounts(uint32_t* calls, uint32_t* failed);
// (x64 bring-up) a keyboard or mouse read noted in bullyre_x64_input.txt when its result or the
// keys / buttons down change (the first 400 changes): what the game sees of real input
void NoteInput(const char* where, long hr, const uint8_t* state, uint32_t size);
bool ScriptedKeysActive();
// (bullyre_x64 --start) ENTER pressed now and then until the game has started (the movies, the
// title menu's STORY): a test run's way in however long the loading takes
void SetEnterUntilStarted(bool on);
void ApplyScriptedKeys(uint8_t* state);
void ApplyScriptedMouse(uint8_t* state);   // (a DIMOUSESTATE: the buttons at +0xC)
bool TakeFrameGrab();
const char* FrameGrabPath();

} // namespace re
