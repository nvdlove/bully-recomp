// Art class (MGClassArt): the arcade minigame where the player draws lines across a deck
// (a grid of cells) to fill it while enemies roam it (0x6DD740..0x6E3740, with its two
// script natives; the deck and the objects' constructors before 0x6DD740 are another
// pass's).
//
// Positions are in play-area units: 0..1 across the 4:3 area of the 640x480 virtual
// screen (x * screenScaleX * 480 + its left edge, y * screenScaleY * 360.142857 + its
// top edge, 10 pixels up). The game objects all share ArtObject's head; their update
// virtual (+4) returns what happened this tick:
//   0 nothing, 1 gone (delete it), 2 hit the player, 3 a floater hit a line,
//   4 caught in a filled area, 5 an exploder went off.
// Object kinds (+4): 0 player, 1 floater, 2 bouncer, 3 line crawler, 4 fast line crawler,
// 7 exploder debris, 9 speed boost, 10 freeze, 11 reversal, 12 clear (9..12 the power-ups).
// The x87 runs at 24-bit precision here (the main thread): float throughout.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "game/FEScreensCommon.h"
#include "game/ScriptApi.h"
#include "game/TrackMath.h"
#include "game/VCall.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

// ---- the game's state used here ---------------------------------------------------------

GAME_VAR(float, artScreenScaleX, 0x00A957A8);
GAME_VAR(float, artScreenScaleY, 0x00A957AC);
GAME_VAR(uint32_t, artTimeInMs, 0x00C1A9B4);
GAME_VAR(int32_t, artLanguage, 0x00C18724);
GAME_VAR(uint8_t*, artCurrentMinigame, 0x020C8704);
GAME_VAR(uint8_t, artMinigameRunning, 0x020C823C);   // set while the art class runs
GAME_VAR(int32_t, artUserStartResponse, 0x020C87AC);
inline void* const artSounds = reinterpret_cast<void*>(0x00CF3DA0);

// (a uint32's value as the game converts it: fild, plus 2^32 when the sign bit is set)
inline float U2F(uint32_t v) {
    return float(v);
}
// the CRT's x87 sqrt (0x85C500), cos (0x85AD80) and sin (0x85AEF0)
inline float Sqrt(float v) {
    return float(std::sqrt(double(v)));
}
inline float Cos(float v) {
    return trk::CrtCos(v);
}
inline float Sin(float v) {
    return trk::CrtSin(v);
}
// the 4:3 area's width and height on the screen (360.142857 is a double constant)
inline float AreaWidth() {
    return artScreenScaleX * 480.0f;
}
inline float AreaHeight() {
    return float(double(artScreenScaleY) * 360.142857);
}

// a texture's size (its virtuals +0x48, +0x4C)
inline uint32_t TexWidth(void* texture) {
    return vc::Call<uint32_t>(texture, 0x48);
}
inline uint32_t TexHeight(void* texture) {
    return vc::Call<uint32_t>(texture, 0x4C);
}

// ---- the deck (the grid being filled; another pass's) ---------------------------------------

struct ArtDeck {
    uint8_t unknown0[0xC];
    uint32_t size;            // +0xC cells across (and down)
    re::Ptr32<uint16_t> cells;          // +0x10 (bit 0: filled; bits 1.. the lines out of each vertex)
    uint32_t cursorCol;       // +0x14 the drawing cursor
    uint32_t cursorRow;       // +0x18
    uint8_t cursorHalfX;      // +0x1C
    uint8_t cursorHalfY;      // +0x1D
    uint8_t unknown1E[6];
    uint8_t drawing;          // +0x24 a line being drawn (off the filled edges)
    uint8_t unknown25[0x17];
    uint32_t unfilled;        // +0x3C cells left
    uint8_t unknown40[0x23];
    uint8_t alpha63;          // +0x63
    uint8_t unknown64[3];
    uint8_t alpha67;          // +0x67
    uint8_t unknown68[3];
    uint8_t alpha6B;          // +0x6B
};
static_assert(offsetof(ArtDeck, drawing) == 0x24);
static_assert(offsetof(ArtDeck, unfilled) == 0x3C);
static_assert(sizeof(ArtDeck) == 0x6C);

// the share of the deck filled, percent (integer: the game's unsigned division)
inline uint32_t FilledPercent(const ArtDeck* deck) {
    uint32_t total = deck->size * deck->size;
    uint32_t filled = total - deck->unfilled;
    return filled != 0 ? filled * 100 / total : 0;
}

// ---- the objects ------------------------------------------------------------------------

struct ArtObject {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x0 (vf0 placed from a pre-instance, vf1 Update, vf2 Draw)
    int32_t kind;             // +0x4
    re::Ptr32<void> texture;            // +0x8
    re::Ptr32<void> glow;               // +0xC drawn under the sprite (nullptr: none)
    uint8_t glowColour[4];    // +0x10
    float x, y;               // +0x14
    float width, height;      // +0x1C the texture's size in play-area units
    float radius;             // +0x24
    uint32_t age;             // +0x28 ticks
    int32_t state;            // +0x2C 0 moving, 1 still, 2 still and blinking, 3 fading out
    uint8_t moved;            // +0x30 (the player: it moved this tick)
    uint8_t unknown31[3];
    float spin;               // +0x34 (the glow's pulse)
    uint32_t stateTime;       // +0x38

    void DrawSprite(void* texture, const uint8_t* colour, float scale);
    uint8_t SweepHits(const float* move, ArtObject* other);
    void Draw();
};
static_assert(offsetof(ArtObject, spin) == 0x34);
static_assert(sizeof(ArtObject) == 0x3C);

// the player's cursor (0x84 bytes)
struct ArtPlayer : ArtObject {
    re::Ptr32<void> swapTexture;        // +0x3C swapped with the texture while reversed
    re::Ptr32<void> powerUpGlow;        // +0x40
    re::Ptr32<ArtDeck> deck;            // +0x44
    float speed;              // +0x48
    float pathSpeedup;        // +0x4C off the drawn lines
    float boostFactor;        // +0x50 power-up 0
    float freezeFactor;       // +0x54 power-up 2
    uint32_t glideSteps;      // +0x58
    float headingX, headingY; // +0x5C
    uint32_t glideStep;       // +0x64
    float glideFromX;         // +0x68
    float glideFromY;         // +0x6C
    int32_t stepResult;       // +0x70 the deck's last step (2 drawing on, 3 a line closed, 4 hit its own line)
    uint8_t poweredUp;        // +0x74
    uint8_t unknown75[3];
    float powerUpTime;        // +0x78 seconds left
    int32_t powerUp;          // +0x7C 0 speed boost, 1 reversal, 2 freeze
    int32_t lastPad;          // +0x80 the d-pad direction held first (-1: none)

    void Reset();
    void StartPowerUp(int kind, const uint8_t* colour, float time);
    float* ReadPad(float* out);
    ArtPlayer* Construct(void* texture, void* glow, void* swapTexture, ArtDeck* deck, float speed,
                         float pathSpeedup, float boostFactor, float freezeFactor);
    int Update(uint32_t ticks);
    void DrawPlayer();
};
static_assert(offsetof(ArtPlayer, lastPad) == 0x80);
static_assert(sizeof(ArtPlayer) == 0x84);

// floaters, bouncers, freeze and clear: drift and bounce off the lines (0x58 bytes)
struct ArtFloater : ArtObject {
    re::Ptr32<ArtDeck> deck;            // +0x3C
    float dirX, dirY;         // +0x40
    float speed;              // +0x48
    re::Ptr32<ArtPlayer> player;        // +0x4C
    float playerLastX;        // +0x50
    float playerLastY;        // +0x54

    int Move(uint32_t ticks);
    int Update(uint32_t ticks);
    void SetDirection(int turns);
    ArtFloater* Construct(int kind, void* texture, ArtDeck* deck, float speed, ArtPlayer* player);
    void Place(const uint8_t* entry);
};
static_assert(sizeof(ArtFloater) == 0x58);

// the trail a line crawler leaves (0x44 bytes from +0x70; another pass's)
struct ArtLinePath {
    re::Ptr32<ArtDeck> deck;            // +0x0
    uint8_t unknown4[0x18];
    uint32_t count;           // +0x1C
    uint8_t unknown20[0x24];
};
static_assert(sizeof(ArtLinePath) == 0x44);

// line crawlers, speed boost and reversal: run along the filled area's edges (0xB4 bytes)
struct ArtLineObject : ArtObject {
    re::Ptr32<ArtDeck> deck;            // +0x3C
    float speed;              // +0x40
    re::Ptr32<ArtPlayer> player;        // +0x44
    float playerLastX;        // +0x48
    float playerLastY;        // +0x4C
    uint32_t fromCol;         // +0x50 the segment being run
    uint32_t fromRow;         // +0x54
    int32_t dir;              // +0x58 0 up, 1 right, 2 down, 3 left (5: none yet)
    uint32_t col, row;        // +0x5C its end
    float segmentLength;      // +0x64
    float segmentLeft;        // +0x68
    int32_t unknown6C;        // +0x6C (5)
    ArtLinePath path;         // +0x70

    ArtLineObject* Construct(int kind, void* texture, ArtDeck* deck, float speed, ArtPlayer* player);
    int Move(uint32_t ticks);
    int Update(uint32_t ticks);
};
static_assert(offsetof(ArtLineObject, path) == 0x70);
static_assert(sizeof(ArtLineObject) == 0xB4);

// the exploder: circles a point, flashing faster, then goes off (0x88 bytes)
struct ArtExploder : ArtObject {
    re::Ptr32<ArtDeck> deck;            // +0x3C
    uint32_t timeOut;         // +0x40 seconds
    float flashRateStart;     // +0x44 flashes a second
    float flashRateEnd;       // +0x48
    float circleStart;        // +0x4C the circle's diameter
    float circleEnd;          // +0x50
    float turnRateStart;      // +0x54 radians a second
    float turnRateEnd;        // +0x58
    uint8_t alpha;            // +0x5C
    uint8_t unknown5D[3];
    float centreX, centreY;   // +0x60
    uint8_t exploded;         // +0x68
    uint8_t unknown69[3];
    float time;               // +0x6C seconds
    float lastFlash;          // +0x70
    float flashLeft;          // +0x74
    float angle;              // +0x78
    re::Ptr32<ArtPlayer> player;        // +0x7C
    float playerLastX;        // +0x80
    float playerLastY;        // +0x84

    int Update(uint32_t ticks);
    void DrawExploder();
};
static_assert(offsetof(ArtExploder, player) == 0x7C);
static_assert(sizeof(ArtExploder) == 0x88);

// the exploder's debris: flies straight out (0x50 bytes)
struct ArtDebris : ArtObject {
    float dirX, dirY;         // +0x3C
    float speed;              // +0x44
    re::Ptr32<ArtDeck> deck;            // +0x48
    re::Ptr32<ArtPlayer> player;        // +0x4C

    int Update(uint32_t ticks);
    ArtDebris* Construct(void* texture, ArtDeck* deck, ArtPlayer* player, const float* pos, const float* dir,
                         float speed);
};
static_assert(sizeof(ArtDebris) == 0x50);

// a power-up's indicator at the bottom of the screen while it lasts (0x54 bytes)
struct ArtPowerUpIcon {
    re::Ptr32<void> texture;            // +0x0
    uint16_t text[0x1A];      // +0x4
    re::Ptr32<void> glow;               // +0x38
    uint8_t glowColour[4];    // +0x3C
    uint8_t unknown40[4];
    uint32_t alpha;           // +0x44 (counts down to fade)
    int32_t state;            // +0x48 1 fading in, 2 fading out
    uint8_t unknown4C[4];
    float pulse;              // +0x50

    void Draw(float x, float y, uint32_t alpha);
};
static_assert(offsetof(ArtPowerUpIcon, alpha) == 0x44);
static_assert(sizeof(ArtPowerUpIcon) == 0x54);

// ---- particles ----------------------------------------------------------------------------

// an explosion's flash (0x20 bytes)
struct ArtFlash {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x0 (vf0 Update, vf1 Draw)
    float x, y;               // +0x4
    re::Ptr32<void> texture;            // +0xC
    uint32_t colour;          // +0x10 RGBA (alpha the top byte)
    float size;               // +0x14
    uint32_t age;             // +0x18
    uint32_t life;            // +0x1C

    void Draw(uint32_t alpha);
};
static_assert(sizeof(ArtFlash) == 0x20);

struct ArtParticle {
    float x, y;
    float vx, vy;
};
static_assert(sizeof(ArtParticle) == 0x10);

// a burst of particles flying out of a point and fading (MGClassArt::ParticleRing, on
// ParticleArray: 0x24 bytes)
struct ArtRing {
    re::Ptr32<re::Ptr32<void>> vtable;            // +0x0 (vf0 the deleting destructor, vf1 Update, vf2 Draw)
    uint32_t count;           // +0x4
    re::Ptr32<void> texture;            // +0x8
    uint32_t colour;          // +0xC
    float size;               // +0x10
    uint32_t age;             // +0x14
    re::Ptr32<ArtParticle> particles;   // +0x18
    uint32_t life;            // +0x1C ticks
    float grow;               // +0x20 the size's growth over its life

    ArtRing* Construct(uint32_t count, void* texture, const uint8_t* colour, float size, uint32_t life, float grow);
    ArtRing* Destroy(int flags);
    void Emit(const float* pos, float spread, float speed, float angle);
    void Draw(uint32_t alpha);
};
static_assert(sizeof(ArtRing) == 0x24);

// a vector of pointers as the game keeps them
template <typename T>
struct ArtArray {
    re::Ptr32<re::Ptr32<T>> data;
    uint32_t capacity;
    uint32_t count;
};
static_assert(sizeof(ArtArray<void>) == 0xC);

// the minigame's particles (+0x618)
struct ArtParticles {
    ArtArray<ArtFlash> flashes;   // +0x0
    ArtArray<ArtRing> rings;      // +0xC

    void Update(uint32_t ticks);
    ArtRing* AddRing(uint32_t count, void* texture, const uint8_t* colour, float size, uint32_t life, float grow);
    void AddExplosion(const float* pos, float spread, float size, void* ringTexture, void* flashTexture);
    void AddBigExplosion(const float* pos, float spread, float size, void* ringTexture, void* flashTexture);
    void AddSparkle(const float* pos, void* texture);
};
static_assert(sizeof(ArtParticles) == 0x18);

// ---- the minigame -------------------------------------------------------------------------

// the textures (+0x1C: one per name registered in Start)
struct ArtTextures {
    re::Ptr32<void> bouncer;            // +0x0
    re::Ptr32<void> clear;              // +0x4
    re::Ptr32<void> exploder;           // +0x8
    re::Ptr32<void> debris;             // +0xC
    re::Ptr32<void> flash;              // +0x10
    re::Ptr32<void> fastLineCrawler;    // +0x14
    re::Ptr32<void> floater;            // +0x18
    re::Ptr32<void> freeze;             // +0x1C
    re::Ptr32<void> glow;               // +0x20
    re::Ptr32<void> homer;              // +0x24
    re::Ptr32<void> lineCrawler;        // +0x28
    re::Ptr32<void> lives;              // +0x2C
    re::Ptr32<void> markParticle;       // +0x30
    re::Ptr32<void> player;             // +0x34
    re::Ptr32<void> playerReversal;     // +0x38
    re::Ptr32<void> reversal;           // +0x3C
    re::Ptr32<void> smartLineCrawler;   // +0x40
    re::Ptr32<void> speedBoost;         // +0x44
    re::Ptr32<void> frame;              // +0x48
};
static_assert(sizeof(ArtTextures) == 0x4C);

// the sounds (+0x4C: one id per name registered in Start)
enum ArtSound : uint32_t {
    ART_SOUND_BRUSH, ART_SOUND_LINE_DRAW, ART_SOUND_LINE_SUCCESS, ART_SOUND_TIME_WARNING,
    ART_SOUND_PLAYER_HITS_CRAWLER, ART_SOUND_ENEMY_SPAWN, ART_SOUND_ENEMY_DESTROYED,
    ART_SOUND_HATRICK_DESTROYED, ART_SOUND_POWER_UP_SPAWN, ART_SOUND_GLUE_FREEZE, ART_SOUND_SCISSORS,
    ART_SOUND_SPEED_BOOST, ART_SOUND_ERASER, ART_SOUND_MUSIC, ART_SOUND_FAIL, ART_SOUND_SUCCESS,
};

// an object to create once the game starts (the scripts' MGCA_PreInstanceEnemy /
// PowerUp: 0x14 bytes)
struct ArtPreInstance {
    uint8_t enemy;            // +0x0
    uint8_t unknown1[3];
    int32_t kind;             // +0x4
    float x, y;               // +0x8 (ints for kinds 0 and 1: the line crawlers' vertex)
    int32_t extra;            // +0x10 the floaters' turns
};
static_assert(sizeof(ArtPreInstance) == 0x14);

// the scripts' settings (MGCA_Set*: +0xC4, copied to +0x2EC when the game sets up)
struct ArtConfig {
    re::Ptr32<const uint16_t> text[18];          // +0x0 the locale strings
    int32_t textureBank;               // +0x48
    float completionThreshold;         // +0x4C percent
    float playerSpeed;                 // +0x50
    float pathSpeedup;                 // +0x54
    float enemySpeed[7];               // +0x58
    float enemyInitialPeriod[7];       // +0x74 seconds
    float enemyPeriod[7];              // +0x90
    uint32_t enemySpawnThreshold[7];   // +0xAC percent filled
    uint32_t enemyInstanceMax[7];      // +0xC8
    uint32_t enemyTotalMax;            // +0xE4
    float exploderTimeOut;             // +0xE8 seconds
    float debrisSpeed;                 // +0xEC
    uint32_t debrisCount;              // +0xF0
    float powerUpSpeed[4];             // +0xF4
    float powerUpInitialPeriod[4];     // +0x104
    float powerUpPeriod[4];            // +0x114
    uint32_t powerUpInstanceMax[4];    // +0x124
    uint32_t powerUpTotalMax;          // +0x134
    uint32_t powerUpSpawnThreshold[4]; // +0x138
    float powerUpDuration[4];          // +0x148
    float speedBoostFactor;            // +0x158
    float freezeFactor;                // +0x15C
    float clearRadius;                 // +0x160
    float levelTimeout;                // +0x164
    uint32_t lives;                    // +0x168
    uint32_t deckResolution;           // +0x16C
    uint8_t powerUpColour[4][4];       // +0x170
    uint32_t preInstanceCount;         // +0x180
    ArtPreInstance preInstances[8];    // +0x184
};
static_assert(offsetof(ArtConfig, enemyTotalMax) == 0xE4);
static_assert(offsetof(ArtConfig, lives) == 0x168);
static_assert(offsetof(ArtConfig, preInstances) == 0x184);
static_assert(sizeof(ArtConfig) == 0x224);

// an enemy or power-up kind's spawner (0x10 bytes)
struct ArtSpawner {
    uint8_t active;           // +0x0 (once the deck is filled enough)
    uint8_t unknown1[3];
    uint32_t spawned;         // +0x4
    uint8_t waiting;          // +0x8
    uint8_t unknown9[3];
    float timer;              // +0xC seconds to the next
};
static_assert(sizeof(ArtSpawner) == 0x10);

struct MGClassArt {
    re::Ptr32<re::Ptr32<void>> vtable;                     // +0x0
    uint8_t active;                    // +0x4
    uint8_t hud;                       // +0x5
    uint8_t success;                   // +0x6
    uint8_t unknown7[0x15];
    re::Ptr32<ArtTextures> textures;             // +0x1C
    uint8_t unknown20[0x18];
    int32_t unknown38;                 // +0x38 (1)
    int32_t unknown3C;                 // +0x3C (6)
    uint8_t unknown40[0xC];
    re::Ptr32<uint32_t> sounds;                  // +0x4C
    int32_t popupKind;                 // +0x50 1 the exit question, 2 another
    uint8_t unknown54[0x4C];
    uint8_t popupShowing;              // +0xA0
    uint8_t unknownA1[0x23];
    ArtConfig scriptConfig;            // +0xC4
    uint8_t setUp;                     // +0x2E8
    uint8_t unknown2E9[3];
    ArtConfig config;                  // +0x2EC
    re::Ptr32<void> frameTexture;                // +0x510
    re::Ptr32<void> backTexture;                 // +0x514
    int32_t txdSlot;                   // +0x518
    re::Ptr32<ArtDeck> deck;                     // +0x51C
    re::Ptr32<ArtPlayer> player;                 // +0x520
    ArtSpawner enemySpawners[7];       // +0x524
    ArtSpawner powerUpSpawners[4];     // +0x594
    uint32_t enemyCount;               // +0x5D4
    uint32_t powerUpCount;             // +0x5D8
    uint32_t lastTime;                 // +0x5DC
    uint8_t frozen;                    // +0x5E0 the freeze power-up holds the enemies
    uint8_t unknown5E1[3];
    int32_t phase;                     // +0x5E4 0 fading in, 1 playing, 2 waiting to start, 3 lost a life,
                                       //   4 won, 5 lost, 6 won (fading)
    uint32_t phaseTime;                // +0x5E8
    uint8_t fade;                      // +0x5EC
    uint8_t unknown5ED[3];
    re::Ptr32<ArtObject> killer;                 // +0x5F0
    uint8_t showMessage;               // +0x5F4
    uint8_t firstLife;                 // +0x5F5
    uint8_t unknown5F6[2];
    re::Ptr32<uint8_t> music;                    // +0x5F8 playing sounds (+0x5C: stop)
    re::Ptr32<uint8_t> powerUpSound;             // +0x5FC
    re::Ptr32<uint8_t> brushSound;               // +0x600
    re::Ptr32<uint8_t> lineSound;                // +0x604
    re::Ptr32<uint8_t> sound608;                 // +0x608
    int32_t unknown60C;                // +0x60C
    int32_t failureCode;               // +0x610
    uint8_t preInstanced;              // +0x614
    uint8_t unknown615[3];
    ArtParticles particles;            // +0x618
    uint8_t colour630[4];              // +0x630
    uint8_t colour634[4];              // +0x634
    int32_t unknown638;                // +0x638
    int32_t unknown63C;                // +0x63C
    ArtArray<ArtPowerUpIcon> icons;    // +0x640
    ArtArray<ArtObject> objects;       // +0x64C
};
static_assert(offsetof(MGClassArt, scriptConfig) == 0xC4);
static_assert(offsetof(MGClassArt, config) == 0x2EC);
static_assert(offsetof(MGClassArt, deck) == 0x51C);
static_assert(offsetof(MGClassArt, enemySpawners) == 0x524);
static_assert(offsetof(MGClassArt, enemyCount) == 0x5D4);
static_assert(offsetof(MGClassArt, phase) == 0x5E4);
static_assert(offsetof(MGClassArt, failureCode) == 0x610);
static_assert(offsetof(MGClassArt, particles) == 0x618);
static_assert(offsetof(MGClassArt, objects) == 0x64C);
static_assert(sizeof(MGClassArt) == 0x658);

// ---- game functions -----------------------------------------------------------------------

// the objects (the constructors and the base update are another pass's)
GAME_FN(void, __thiscall, ArtObject_Construct, 0x006DC010, ArtObject* self, int kind, void* texture);
GAME_FN(int, __thiscall, ArtObject_Update, 0x006DADE0, ArtObject* self, uint32_t ticks);
GAME_FN(void, __thiscall, ArtObject_DrawSprite, 0x006DD740, ArtObject* self, void* texture, const uint8_t* colour,
        float scale);
GAME_FN(uint8_t, __thiscall, ArtObject_SweepHits, 0x006DD8A0, ArtObject* self, const float* move, ArtObject* other);
GAME_FN(void, __thiscall, ArtObject_Draw, 0x006E0060, ArtObject* self);
GAME_FN(uint8_t, __thiscall, ArtObject_IsPowerUp, 0x006DADC0, ArtObject* self);
GAME_FN(void, __thiscall, ArtPlayer_Reset, 0x006DDA80, ArtPlayer* self);
GAME_FN(void, __thiscall, ArtPlayer_EndPowerUp, 0x006DC110, ArtPlayer* self);
GAME_FN(void, __thiscall, ArtPlayer_StartPowerUp, 0x006DDB00, ArtPlayer* self, int kind, const uint8_t* colour,
        float time);
GAME_FN(float*, __thiscall, ArtPlayer_ReadPad, 0x006DDB90, ArtPlayer* self, float* out);
GAME_FN(int, __thiscall, ArtFloater_Move, 0x006DDD50, ArtFloater* self, uint32_t ticks);
GAME_FN(void, __thiscall, ArtFloater_SetDirection, 0x006E08A0, ArtFloater* self, int turns);
GAME_FN(int, __thiscall, ArtLineObject_Move, 0x006DDFF0, ArtLineObject* self, uint32_t ticks);
GAME_FN(void, __thiscall, ArtLineObject_PickStart, 0x006DC150, ArtLineObject* self);
GAME_FN(void, __thiscall, ArtLinePath_AddSegment, 0x006DB010, ArtLinePath* self, uint32_t col, uint32_t row,
        uint32_t length, int dir);
GAME_FN(void, __thiscall, ArtLinePath_Advance, 0x006DB230, ArtLinePath* self, float distance);
GAME_FN(uint8_t, __thiscall, ArtLinePath_Cut, 0x006DB360, ArtLinePath* self);
GAME_FN(ArtObject*, __thiscall, ArtExploder_Construct, 0x006DC280, void* self, void* texture, ArtDeck* deck,
        float timeOut, ArtPlayer* player);
GAME_FN(ArtPowerUpIcon*, __thiscall, ArtPowerUpIcon_Construct, 0x006DC3A0, void* self, void* texture,
        const uint16_t* text, void* glow, const uint8_t* colour, float time);
GAME_FN(void, __thiscall, ArtPowerUpIcon_Draw, 0x006DE650, ArtPowerUpIcon* self, float x, float y, uint32_t alpha);
GAME_FN(ArtObject*, __thiscall, ArtLineObject_Construct, 0x006DDF70, void* self, int kind, void* texture,
        ArtDeck* deck, float speed, ArtPlayer* player);
GAME_FN(ArtObject*, __thiscall, ArtFloater_Construct, 0x006E14A0, void* self, int kind, void* texture, ArtDeck* deck,
        float speed, ArtPlayer* player);
GAME_FN(ArtObject*, __thiscall, ArtPlayer_Construct, 0x006E0170, void* self, void* texture, void* glow,
        void* swapTexture, ArtDeck* deck, float speed, float pathSpeedup, float boostFactor, float freezeFactor);
GAME_FN(ArtObject*, __thiscall, ArtDebris_Construct, 0x006E0A10, void* self, void* texture, ArtDeck* deck,
        ArtPlayer* player, const float* pos, const float* dir, float speed);
GAME_FN(void, __thiscall, ArtFlash_DrawGame, 0x006DEB80, ArtFlash* self, uint32_t alpha);
GAME_FN(ArtRing*, __thiscall, ArtRing_ConstructGame, 0x006DED70, void* self, uint32_t count, void* texture,
        const uint8_t* colour, float size, uint32_t life, float grow);
GAME_FN(void, __thiscall, ArtRing_Emit, 0x006DEE30, ArtRing* self, const float* pos, float spread, float speed,
        float angle);
GAME_FN(void, __thiscall, ArtArray_Remove, 0x006DC540, void* array, uint32_t index);
GAME_FN(void, __thiscall, ArtArray_Reserve, 0x006DB480, void* array, uint32_t capacity, int);
GAME_FN(ArtRing*, __thiscall, ArtParticles_AddRing, 0x006DF2F0, ArtParticles* self, uint32_t count, void* texture,
        const uint8_t* colour, float size, uint32_t life, float grow);
GAME_FN(void, __thiscall, ArtParticles_AddExplosion, 0x006E0AA0, ArtParticles* self, const float* pos, float spread,
        float size, void* ringTexture, void* flashTexture);
GAME_FN(void, __thiscall, ArtParticles_AddBigExplosion, 0x006E0CA0, ArtParticles* self, const float* pos,
        float spread, float size, void* ringTexture, void* flashTexture);
GAME_FN(void, __thiscall, ArtParticles_AddSparkle, 0x006E0EA0, ArtParticles* self, const float* pos, void* texture);
GAME_FN(void, __thiscall, ArtParticles_Update, 0x006DF260, ArtParticles* self, uint32_t ticks);
GAME_FN(void, __thiscall, ArtParticles_Clear, 0x006DC4C0, ArtParticles* self);

// the deck
GAME_FN(uint8_t, __thiscall, ArtDeck_Collide, 0x006DCB00, ArtDeck* self, const float* pos, float* move, float* hit,
        uint8_t* hitVertical, uint8_t* hitLine);
GAME_FN(uint8_t, __thiscall, ArtDeck_IsFilledAt, 0x006DA7E0, ArtDeck* self, const float* pos);
GAME_FN(void, __thiscall, ArtDeck_GetRuns, 0x006DA4A0, ArtDeck* self, uint32_t col, uint32_t row, int flag,
        uint32_t* left, uint32_t* up, uint32_t* right, uint32_t* down);
GAME_FN(float*, __thiscall, ArtDeck_GetCursor, 0x006DBDF0, ArtDeck* self, float* out);
GAME_FN(int, __thiscall, ArtDeck_Step, 0x006DCEE0, ArtDeck* self, int dir);
GAME_FN(void, __thiscall, ArtDeck_ToGrid, 0x006DA8D0, ArtDeck* self, const float* pos, int* col, int* row);
GAME_FN(float*, __thiscall, ArtDeck_RandomPoint, 0x006DB950, ArtDeck* self, float* out);
GAME_FN(void, __thiscall, ArtDeck_SetAlpha, 0x006DBE80, ArtDeck* self, int alpha);
GAME_FN(ArtDeck*, __thiscall, ArtDeck_Construct, 0x006DCA20, void* self, uint32_t size, void* backTexture,
        void* frameTexture);

// drawing and text
GAME_FN(uint8_t*, __thiscall, RGBA_Set, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, DrawRectUV, 0x0052C8F0, float x1, float y1, float x2, float y2, void* texture,
        const uint8_t* colour, const float* uv1, const float* uv2);
GAME_FN(void, __cdecl, Font_SetScale, 0x00688080, float scale);
GAME_FN(void, __cdecl, Font_PrintInSlot, 0x0068D610, int slot, const uint16_t* text, int);
GAME_VAR(uint8_t, artFontDropOn, 0x0208B21C);
GAME_VAR(uint16_t, artFontOutline, 0x0208B788);
GAME_VAR(uint8_t, artFontFlags, 0x0208B78A);
GAME_VAR(float, artFontWrap, 0x0208B778);

// sound and the pad
GAME_FN(uint8_t*, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, uint32_t id, const float* pos, int, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustDown, 0x00738900, int control, int pad);
GAME_FN(void, __cdecl, Pad_Rumble, 0x00738680, int pad, int duration, int strength, int);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(void, __thiscall, Vector2D_Normalise, 0x00443730, float* v);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const float* v);

// a sound of the art class's (their usual arguments)
inline uint8_t* PlaySound(const MGClassArt* mg, ArtSound sound) {
    return Sounds_Play(artSounds, mg->sounds[sound], nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}
inline void StopSound(re::Ptr32<uint8_t>& sound) {
    if (sound != nullptr) {
        sound[0x5C] = 1;
        sound = nullptr;
    }
}

// a vector's push_back (inlined everywhere: grows by doubling through 0x6DB480)
template <typename T>
inline void PushBack(ArtArray<T>& array, T* item) {
    if (array.count == array.capacity) {
        ArtArray_Reserve(&array, array.capacity == 0 ? 1 : array.capacity * 2, 0);
    }
    array.data[array.count] = item;
    array.count += 1;
}

// ---- ArtObject ------------------------------------------------------------------------------

// 0x6DD740: a texture centred on the object, `scale` times its size (half-texel inset
// texture coordinates)
void ArtObject::DrawSprite(void* tex, const uint8_t* colour, float scale) {
    float uv1[2];
    float uv2[2];
    float texW = U2F(TexWidth(tex));
    float texH = U2F(TexHeight(tex));
    uv1[0] = 0.5f / texW;
    uv1[1] = 0.5f / texH;
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    float w = width * scale;
    float h = scale * height;
    float halfW = w * 0.5f;
    float areaW = AreaWidth();
    float left = (640.0f - areaW) * 0.5f;
    float x1 = (x - halfW) * areaW + left;
    float halfH = h * 0.5f;
    float areaH = AreaHeight();
    float top = (480.0f - areaH) * 0.5f;
    float y1 = ((y - halfH) * areaH + top) - 10.0f;
    float x2 = (x + halfW) * areaW + left;
    float y2 = ((y + halfH) * areaH + top) - 10.0f;
    DrawRectUV(x1, y1, x2, y2, tex, colour, uv1, uv2);
}

// 0x6DD8A0: whether moving by `move` brings this object's circle onto the other's (a
// swept circle against a circle)
uint8_t ArtObject::SweepHits(const float* move, ArtObject* other) {
    float moveXSq = move[0] * move[0];
    float moveLength = Sqrt(std::fabs(moveXSq + move[1] * move[1]));
    if (moveLength == 0.0f) {
        return 0;
    }
    float radii = other->radius + radius;
    float toX = other->x - x;
    float toY = other->y - y;
    float distance = Sqrt(std::fabs(toX * toX + toY * toY));
    if (radii > distance) {
        return 1;
    }
    float dot = move[0] * toX + toY * move[1];
    float along = dot / Sqrt(std::fabs(move[1] * move[1] + moveXSq));
    float t = along / Sqrt(std::fabs(move[1] * move[1] + moveXSq));
    float closestX = move[0] * t;
    float closestY = t * move[1];
    if (-radii > along) {
        return 0;
    }
    if (Sqrt(std::fabs(move[1] * move[1] + moveXSq)) + radii < along) {
        return 0;
    }
    float off[2] = {toX - closestX, toY - closestY};
    if (radii < Vector_Magnitude2D(off)) {
        return 0;
    }
    return 1;
}

// ---- ArtPlayer ------------------------------------------------------------------------------

// 0x6DDA80: back on the deck's cursor, still, the power-up over
void ArtPlayer::Reset() {
    float cursor[2];
    float* at = ArtDeck_GetCursor(deck, cursor);
    x = at[0];
    y = at[1];
    headingX = x;
    headingY = y;
    glideStep = glideSteps;
    lastPad = -1;
    age = 0;
    glow = nullptr;   // (the glow's colour copied onto itself)
    uint8_t was = poweredUp;
    poweredUp = 0;
    if (was != 0 && powerUp == 1) {
        void* t = texture;
        texture = swapTexture;
        swapTexture = t;
    }
}

// 0x6DDB00: a power-up starts (any running one ends first); the reversal swaps the
// texture
void ArtPlayer::StartPowerUp(int kind_, const uint8_t* colour, float time) {
    glow = nullptr;
    if (poweredUp != 0 && powerUp == 1) {
        void* t = texture;
        texture = swapTexture;
        swapTexture = t;
    }
    powerUpTime = time;
    poweredUp = 1;
    powerUp = kind_;
    if (kind_ == 1) {
        void* t = texture;
        texture = swapTexture;
        swapTexture = t;
    }
    glow = powerUpGlow;
    glowColour[0] = colour[0];
    glowColour[1] = colour[1];
    glowColour[2] = colour[2];
    glowColour[3] = colour[3];
}

// 0x6DDB90: the pad's direction: the d-pad (the direction held first is 1.0, others
// 0.99, so it wins), else the left stick; any input counts the player as past its
// spawn blink
float* ArtPlayer::ReadPad(float* out) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    if (Pad_GetValue(1, 0) != 0.0f) {   // right
        int last = lastPad;
        out[0] = last == 1 ? 1.0f : 0.99f;
        if (last == -1) {
            lastPad = 1;
        }
    }
    if (Pad_GetValue(0, 0) != 0.0f) {   // left
        int last = lastPad;
        out[0] = last == 0 ? -1.0f : -0.99f;
        if (last == -1) {
            lastPad = 0;
        }
    }
    if (Pad_GetValue(2, 0) != 0.0f) {   // up
        int last = lastPad;
        out[1] = last == 2 ? -1.0f : -0.99f;
        if (last == -1) {
            lastPad = 2;
        }
    }
    if (Pad_GetValue(3, 0) != 0.0f) {   // down
        int last = lastPad;
        out[1] = last == 3 ? 1.0f : 0.99f;
        if (last == -1) {
            lastPad = 3;
        }
    }
    if (out[0] == 0.0f && out[1] == 0.0f) {
        lastPad = -1;
        out[0] = -Pad_GetValue(0x10, 0);
        out[1] = -Pad_GetValue(0x11, 0);
    }
    if (Sqrt(std::fabs(out[0] * out[0] + out[1] * out[1])) > 0.01f && age < 60) {
        age = 60;
    }
    return out;
}

// ---- ArtFloater -----------------------------------------------------------------------------

// 0x6DDD50: drifts `ticks` worth, bouncing off the deck's lines (a step stops just
// short of a line, 95% of the way, unless that is under half a pixel); 2 when it hits
// the player while a line is being drawn, 3 when an enemy hits the line being drawn
int ArtFloater::Move(uint32_t ticks) {
    int result = 0;
    float inv = 1.0f / U2F(deck->size);
    float left = inv * speed * U2F(ticks);
    while (left > 0.0f) {
        float move[2];
        float hit[2];
        uint8_t hitVertical;
        uint8_t hitLine;
        move[0] = dirX * left;
        move[1] = left * dirY;
        if (ArtDeck_Collide(deck, &x, move, hit, &hitVertical, &hitLine) == 0) {
            if (deck->drawing != 0 && age >= 60 && ArtObject_SweepHits(this, move, player) != 0) {
                result = 2;
            }
            x = x + move[0];
            y = move[1] + y;
            return result;
        }
        float dx = hit[0] - x;
        float dy = hit[1] - y;
        float scaleX = artScreenScaleX;
        float minStep = (0.5f * scaleX) / (scaleX * 480.0f);
        float distance = Sqrt(std::fabs(dx * dx + dy * dy));
        if (minStep > distance) {
            left = left - minStep;
        } else {
            dx = dx * 0.95f;
            dy = 0.95f * dy;
            x = x + dx;
            y = y + dy;
            left = left - Sqrt(std::fabs(dx * dx + dy * dy));
        }
        if (hitVertical != 0) {
            dirY = -dirY;
        } else {
            dirX = -dirX;
        }
        if (ArtObject_IsPowerUp(this) == 0 && age >= 60 && hitLine != 0) {
            result = 3;
        }
    }
    return result;
}

// ---- ArtLineObject --------------------------------------------------------------------------

// 0x6DDF70
ArtLineObject* ArtLineObject::Construct(int kind_, void* texture_, ArtDeck* deck_, float speed_, ArtPlayer* player_) {
    ArtObject_Construct(this, kind_, texture_);
    deck = deck_;
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x0094431C);
    player = player_;
    path.deck = deck_;
    path.count = 0;
    unknown6C = 5;
    radius = (1.0f / U2F(deck->size)) * 0.49f;
    speed = speed_;
    ArtLineObject_PickStart(this);
    return this;
}

// 0x6DDFF0: runs `ticks` worth along the lines: at each vertex its virtual +0xC picks
// the next direction from how far the lines run each way; 2 when it hits the player
// while no line is being drawn
int ArtLineObject::Move(uint32_t ticks) {
    int result = 0;
    float inv = 1.0f / U2F(deck->size);
    float left = inv * speed * U2F(ticks);
    if (!(left > 0.0f)) {
        return 0;
    }
    uint32_t length = 0;   // (the game's is uninitialised when the virtual gives no direction)
    do {
        if (segmentLeft >= left) {
            float move[2] = {0.0f, 0.0f};   // (uninitialised in the game for no direction)
            switch (dir) {
            case 0:
                move[0] = 0.0f;
                move[1] = -left;
                break;
            case 1:
                move[0] = left;
                move[1] = 0.0f;
                break;
            case 2:
                move[0] = 0.0f;
                move[1] = left;
                break;
            case 3:
                move[0] = -left;
                move[1] = 0.0f;
                break;
            }
            if (age >= 60 && deck->drawing == 0 && ArtObject_SweepHits(this, move, player) != 0) {
                result = 2;
            }
            x = x + move[0];
            y = move[1] + y;
            segmentLeft = segmentLeft - left;
            ArtLinePath_Advance(&path, left);
            return result;
        }
        float cellInv = 1.0f / U2F(deck->size);
        float targetX = U2F(col) * cellInv;
        float targetY = cellInv * U2F(row);
        if (age >= 60 && deck->drawing == 0) {
            float move[2] = {targetX - x, targetY - y};
            if (ArtObject_SweepHits(this, move, player) != 0) {
                result = 2;
            }
        }
        x = targetX;
        y = targetY;
        left = left - segmentLeft;
        uint32_t runLeft, runUp, runRight, runDown;
        ArtDeck_GetRuns(deck, col, row, 1, &runLeft, &runUp, &runRight, &runDown);
        int next = vc::Call<int>(this, 0xC, runLeft, runUp, runRight, runDown);
        dir = next;
        fromCol = col;
        fromRow = row;
        switch (next) {
        case 0:
            row -= runUp;
            length = runUp;
            break;
        case 1:
            col += runRight;
            length = runRight;
            break;
        case 2:
            row += runDown;
            length = runDown;
            break;
        case 3:
            col -= runLeft;
            length = runLeft;
            break;
        }
        float lengthInv = 1.0f / U2F(deck->size);
        float lengthF = lengthInv * U2F(length);
        segmentLeft = lengthF;
        segmentLength = lengthF;
        ArtLinePath_AddSegment(&path, fromCol, fromRow, length, next);
    } while (left > 0.0f);
    return result;
}

// ---- ArtExploder ----------------------------------------------------------------------------

// 0x6DE2F0: circles its centre, the circle widening and the flashes quickening until
// its time is up (5); still (state 1), it hurts the player running into it while
// drawing. (The game reads the deck back from ecx after 0x6DA7E0, which keeps it.)
int ArtExploder::Update(uint32_t ticks) {
    int base = ArtObject_Update(this, ticks);
    int state_ = state;
    if (state_ == 3) {
        return base;
    }
    if (exploded != 0) {
        return 1;
    }
    if (ArtDeck_IsFilledAt(deck, &x) != 0) {
        alpha = 0xFF;
        return 4;
    }
    if (state_ != 1 && state_ != 0) {
        return 0;
    }
    int result = 0;
    if (state_ == 1) {
        if (deck->drawing != 0 && age >= 60) {
            float move[2] = {player->x - playerLastX, player->y - playerLastY};
            if (ArtObject_SweepHits(player, move, this) != 0) {
                result = 2;
            }
        }
    } else {
        float dt = U2F(ticks) / 60.0f;
        time = dt + time;
        float t = time;
        float timeOutF = U2F(timeOut);
        if (timeOutF <= t) {
            exploded = 1;
            result = 5;
        } else {
            float fraction = t / timeOutF;
            if (flashLeft > 0.0f) {
                flashLeft = flashLeft - dt;
                if (flashLeft <= 0.0f) {
                    alpha = 0xFF;
                    flashLeft = 0.0f;
                    lastFlash = time;
                }
            } else {
                float interval = 1.0f / ((flashRateEnd - flashRateStart) * fraction + flashRateStart);
                if (interval <= time - lastFlash) {
                    alpha = 0;
                    flashLeft = interval / 3.0f;
                }
            }
            float oldX = x;
            float oldY = y;
            float circle = ((circleEnd - circleStart) * fraction + circleStart) * 0.5f;
            float turnRate = (turnRateEnd - turnRateStart) * fraction + turnRateStart;
            angle = dt * turnRate + angle;
            float a = angle;
            x = Cos(a) * circle + centreX;
            y = Sin(a) * circle + centreY;
            if (age >= 60 && deck->drawing != 0) {
                float move[2] = {x - oldX, y - oldY};
                if (ArtObject_SweepHits(this, move, player) != 0) {
                    result = 2;
                }
            }
        }
    }
    playerLastX = player->x;
    playerLastY = player->y;
    return result;
}

// ---- ArtDebris ------------------------------------------------------------------------------

// 0x6DE560: flies on; gone (1) once off the deck
int ArtDebris::Update(uint32_t ticks) {
    int base = ArtObject_Update(this, ticks);
    if (state != 0) {
        return base;
    }
    int result = 0;
    float step = 1.0f / U2F(deck->size) * U2F(ticks) * speed;
    float move[2] = {step * dirX, step * dirY};
    if (ArtObject_SweepHits(this, move, player) != 0) {
        result = 2;
    }
    x = x + move[0];
    y = y + move[1];
    if (x <= 0.0f || y <= 0.0f || x >= 1.0f || y >= 1.0f) {
        return 1;
    }
    return result;
}

// ---- ArtPowerUpIcon -------------------------------------------------------------------------

// 0x6DE650: the icon (swaying; three times the size as it fades in) with its pulsing glow
// and its name, at (x, y) from the bottom middle of the screen
void ArtPowerUpIcon::Draw(float x, float y, uint32_t fade) {
    float a = U2F(alpha) / 255.0f;
    float scale = 1.0f;
    if (state == 1) {
        scale = (3.0f - a * 3.0f) + 1.0f;
    }
    float c = Cos(pulse);
    scale = scale + c * 0.1f * scale;
    uint8_t rgba[4];
    uint8_t shown = uint8_t(int(float(uint8_t(fade)) * a));
    RGBA_Set(rgba, 0x80, 0x80, 0x80, shown);
    float scaleX = artScreenScaleX;
    float w = U2F(TexWidth(texture)) * scale * scaleX;
    float scaleY = artScreenScaleY;
    float h = U2F(TexHeight(texture)) * scale * scaleY;
    float s = Sin(pulse);
    float c2 = Cos(pulse);
    x = x + 320.0f;
    y = y + 468.0f;
    float sway = scale * 5.0f;
    float centreX = sway * s + x;
    float centreY = sway * c2 + y;
    float x1 = centreX - w * 0.5f;
    float y1 = centreY - 0.5f * h;
    float x2 = w + x1;
    float y2 = h + y1;
    float uv1[2];
    float uv2[2];
    uv1[0] = 0.5f / U2F(TexWidth(texture));
    uv1[1] = 0.5f / U2F(TexHeight(texture));
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    DrawRectUV(x1, y1, x2, y2, texture, rgba, uv1, uv2);

    float pulse2 = pulse + 1.5707964f;
    float c3 = Cos(pulse2);
    scale = scale + c3 * 0.3f * scale;
    w = U2F(TexWidth(glow)) * scale;
    h = U2F(TexHeight(glow)) * scale;
    rgba[0] = glowColour[0];   // (the alpha stays the icon's)
    rgba[1] = glowColour[1];
    rgba[2] = glowColour[2];
    x1 = centreX - w * 0.5f;
    y1 = centreY - 0.5f * h;
    x2 = w + x1;
    y2 = h + y1;
    uv1[0] = 0.5f / U2F(TexWidth(glow));
    uv1[1] = 0.5f / U2F(TexHeight(glow));
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    DrawRectUV(x1, y1, x2, y2, glow, rgba, uv1, uv2);

    fescr::Font_Reset();
    if (artFontDropOn != 0) {
        artFontOutline = 1;
        fescr::g_fontDrop = 0;
    }
    uint8_t colour[4];
    uint8_t* set = RGBA_Set(colour, 0xFF, 0xFF, 0xFF, shown);
    std::memcpy(fescr::g_fontColour, set, 4);
    fescr::g_fontDrop = 0;
    set = RGBA_Set(colour, 0, 0, 0, int(fade));
    std::memcpy(fescr::g_fontDropColour, set, 4);
    Font_SetScale(artLanguage == 7 || artLanguage == 6 ? 0.6f : 1.0f);
    artFontFlags |= 0x10;
    fescr::Font_PrintString(x + 32.0f, y - 12.0f, text, 1);
    artFontFlags &= ~0x10;
}

// ---- particles ----------------------------------------------------------------------------

// the play area's mapping to the screen
struct AreaMap {
    float w, h, left, top;
};
inline AreaMap Area() {
    AreaMap m;
    m.h = AreaHeight();
    m.top = (480.0f - m.h) * 0.5f;
    m.w = AreaWidth();
    m.left = (640.0f - m.w) * 0.5f;
    return m;
}
inline float ScreenX(const AreaMap& m, float x) {
    return x * m.w + m.left;
}
inline float ScreenY(const AreaMap& m, float y) {
    return (y * m.h + m.top) - 10.0f;
}

// 0x6DEB80: fades over its life
void ArtFlash::Draw(uint32_t fade) {
    uint8_t rgba[4];
    std::memcpy(rgba, &colour, 4);
    float left = U2F(life - age) / U2F(life);
    float k = left * (float(uint8_t(fade)) / 255.0f);
    rgba[3] = uint8_t(int(k * U2F(colour >> 24)));
    float half = size * 0.5f;
    float x1 = x - half;
    float y1 = y - half;
    float x2 = size + x1;
    float y2 = size + y1;
    float uv1[2];
    float uv2[2];
    uv1[0] = 0.5f / U2F(TexWidth(texture));
    uv1[1] = 0.5f / U2F(TexHeight(texture));
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    AreaMap m = Area();
    DrawRectUV(ScreenX(m, x1), ScreenY(m, y1), ScreenX(m, x2), ScreenY(m, y2), texture, rgba, uv1, uv2);
}

// 0x6DED70
ArtRing* ArtRing::Construct(uint32_t count_, void* texture_, const uint8_t* colour_, float size_, uint32_t life_,
                            float grow_) {
    texture = texture_;
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x009442B4);   // ParticleArray
    count = count_;
    std::memcpy(&colour, colour_, 4);
    size = size_;
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x009442FC);   // ParticleRing
    uint64_t bytes = uint64_t(count_) * sizeof(ArtParticle);
    particles = static_cast<ArtParticle*>(Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
    grow = grow_;
    life = life_;
    age = 0;
    return this;
}

// 0x6DEDF0: the deleting destructor
ArtRing* ArtRing::Destroy(int flags) {
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x009442FC);
    Free(particles);
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x009442B4);
    if ((flags & 1) != 0) {
        Free(this);
    }
    return this;
}

// 0x6DEE30: the particles scattered `spread` around `pos`, flying out at angles spaced
// round the circle from `angle`, slower the further out they start
void ArtRing::Emit(const float* pos, float spread, float speed, float angle) {
    for (uint32_t i = 0; i < count; ++i) {
        float c = Cos(angle);
        float s = Sin(angle);
        float offX = float(Rand() % 256 - 0x80) * 0.00390625f * spread;
        float offY = float(Rand() % 256 - 0x80) * 0.00390625f * spread;
        ArtParticle& p = particles[i];
        p.x = pos[0] + offX;
        p.y = offY + pos[1];
        float k = (speed - Sqrt(std::fabs(offY * offY + offX * offX))) / U2F(life);
        p.vx = k * c;
        p.vy = k * s;
        angle = 6.2831855f / U2F(count) + angle;
    }
}

// 0x6DEFC0: growing over its life, fading over its last fifth
void ArtRing::Draw(uint32_t fade) {
    float t = U2F(age) / U2F(life);
    uint8_t rgba[4];
    std::memcpy(rgba, &colour, 4);
    float drawn = size + grow * size * t;
    uint8_t a;
    if (!(t >= 0.8f)) {
        a = rgba[3];
    } else {
        a = uint8_t(int((1.0f - t) * U2F(colour >> 24) / 0.19999999f));
    }
    rgba[3] = uint8_t(int(float(uint8_t(fade)) / 255.0f * float(a)));
    float uv1[2];
    float uv2[2];
    uv1[0] = 0.5f / U2F(TexWidth(texture));
    uv1[1] = 0.5f / U2F(TexHeight(texture));
    uv2[0] = 1.0f - uv1[0];
    uv2[1] = 1.0f - uv1[1];
    if (count == 0) {
        return;
    }
    float half = drawn * 0.5f;
    for (uint32_t i = 0; i < count; ++i) {
        const ArtParticle& p = particles[i];
        float x1 = p.x - half;
        float y1 = p.y - half;
        float x2 = x1 + drawn;
        float y2 = y1 + drawn;
        AreaMap m = Area();
        DrawRectUV(ScreenX(m, x1), ScreenY(m, y1), ScreenX(m, x2), ScreenY(m, y2), texture, rgba, uv1, uv2);
    }
}

// 0x6DF260: steps the flashes (their update +0 gives 0 once over: freed) and the rings
// (+4; deleted through their destructor)
void ArtParticles::Update(uint32_t ticks) {
    for (uint32_t i = 0; i < flashes.count; ++i) {
        ArtFlash* flash = flashes.data[i];
        if (vc::Call<uint8_t>(flash, 0, ticks) == 0) {
            Free(flash);
            ArtArray_Remove(&flashes, i);
            --i;
        }
    }
    for (uint32_t i = 0; i < rings.count; ++i) {
        ArtRing* ring = rings.data[i];
        if (vc::Call<uint8_t>(ring, 4, ticks) == 0) {
            vc::Call<void*>(ring, 0, 1);
            ArtArray_Remove(&rings, i);
            --i;
        }
    }
}

// 0x6DF2F0
ArtRing* ArtParticles::AddRing(uint32_t count, void* texture, const uint8_t* colour, float size, uint32_t life,
                               float grow) {
    void* memory = Malloc(sizeof(ArtRing));
    ArtRing* ring = memory != nullptr ? ArtRing_ConstructGame(memory, count, texture, colour, size, life, grow) : nullptr;
    PushBack(rings, ring);
    return ring;
}

// ---- MGClassArt: drawing ------------------------------------------------------------------

GAME_FN(void, __thiscall, ArtDeck_Draw, 0x006DD110, ArtDeck* self);
GAME_FN(void, __thiscall, ArtPopup_Draw, 0x00706000, void* popup);
GAME_FN(void, __thiscall, MGClassArt_DrawIcon, 0x006DC930, MGClassArt* self, float x, float y, float w, float h,
        void* texture);
GAME_FN(void, __cdecl, DrawRect4, 0x0052C530, const float* rect, const uint8_t* topLeft, const uint8_t* topRight,
        const uint8_t* bottomLeft, const uint8_t* bottomRight);
GAME_FN(void, __cdecl, SetRenderState, 0x00519790, int state, int value);
GAME_VAR(float, artTimeStep, 0x00C1A9A4);
GAME_VAR(float, artTitleSpin, 0x020C8240);
GAME_VAR(float, artTwoPi, 0x00BC60C0);
// the title screen's layout (other values for languages 6 and 7, set when it first draws)
GAME_VAR(float, artTitleY, 0x00BC61F0);
GAME_VAR(float, artTitleWrap, 0x00BC61F4);
GAME_VAR(float, artHeadingY, 0x00BC61F8);
GAME_VAR(float, artIconSize, 0x00BC61FC);
GAME_VAR(float, artHeadingScale, 0x00BC6200);
GAME_VAR(float, artTextScale, 0x00BC6204);
GAME_VAR(float, artLabelScale, 0x00BC6208);
inline const char* const artPercentKey = reinterpret_cast<const char*>(0x00944374);   // "PCT"

// the font's drop colour (black at the fade)
inline void SetFontDropColour(uint8_t alpha) {
    uint8_t colour[4];
    uint8_t* set = RGBA_Set(colour, 0, 0, 0, alpha);
    std::memcpy(fescr::g_fontDropColour, set, 4);
}
inline void SetFontColour(int r, int g, int b, uint8_t alpha) {
    uint8_t colour[4];
    uint8_t* set = RGBA_Set(colour, r, g, b, alpha);
    std::memcpy(fescr::g_fontColour, set, 4);
}

// 0x6DF370 (virtual +8): the deck, the message, the percentage, the lives, the objects,
// the power-up icons and the particles; while waiting to start (phase 2), the title
// screen over it all (its icons and their names)
void __fastcall MGClassArtRender(MGClassArt* mg, re::Edx) {
    if (mg->setUp == 0) {
        return;
    }
    if (mg->popupShowing != 0) {
        ArtPopup_Draw(&mg->popupKind);
        return;
    }
    ArtDeck_Draw(mg->deck);
    fescr::Font_Reset();
    artFontWrap = 400.0f;
    SetFontDropColour(mg->fade);
    fescr::Font_SetJustify(2);
    if (mg->showMessage != 0) {
        Font_PrintInSlot(3, mg->firstLife != 0 ? mg->config.text[0] : mg->config.text[1], 0);
    } else if (mg->phase == 1) {
        Font_PrintInSlot(3, mg->config.text[2], 0);
    }
    fescr::Font_AddNumber(U2F(FilledPercent(mg->deck)));
    uint16_t percent[0x20];
    fescr::Font_Format(percent, fescr::Text_Get(fescr::kTheText, artPercentKey), 0x20);
    fescr::Font_Reset();
    artFontWrap = 400.0f;
    SetFontDropColour(mg->fade);
    fescr::Font_SetJustify(1);
    fescr::Font_SetSizePreset(3);
    fescr::g_fontDrop = 0;
    SetFontColour(0xD7, 0xD7, 0xD7, mg->fade);
    int percentY = artLanguage == 7 || artLanguage == 6 ? 421 : 412;
    fescr::Font_PrintString(560.0f, float(percentY), percent, 1);
    SetRenderState(9, 2);
    SetRenderState(2, 3);

    if (mg->phase != 2) {   // the lives
        float x = 430.0f;
        float y = artLanguage == 7 || artLanguage == 6 ? 421.0f : 408.0f;
        uint8_t rgba[4];
        RGBA_Set(rgba, 0x80, 0x80, 0x80, mg->fade);
        void* lives = mg->textures != nullptr ? mg->textures->lives : nullptr;
        float uv1[2] = {0.0f, 0.0f};
        float uv2[2] = {1.0f, 1.0f};
        float u = 0.5f / U2F(TexWidth(lives));
        float v = 0.5f / U2F(TexHeight(lives));
        uv1[0] = u + uv1[0];
        uv1[1] = v + uv1[1];
        uv2[0] = uv2[0] - u;
        uv2[1] = uv2[1] - v;
        if (mg->config.lives != 0) {
            float y2 = y + 27.0f;
            for (uint32_t i = 0; i < mg->config.lives; ++i) {
                float x2 = x + 27.0f;
                DrawRectUV(x, y, x2, y2, lives, rgba, uv1, uv2);
                x = x + 8.0f;
            }
        }
    }
    for (uint32_t i = 0; i < mg->objects.count; ++i) {
        vc::Call<void>(mg->objects.data[i], 8);
    }
    if (mg->player->state != 1) {
        vc::Call<void>(mg->player, 8);
    }
    for (uint32_t i = 0; i < mg->icons.count; ++i) {
        ArtPowerUpIcon_Draw(mg->icons.data[i], -100.0f, -28.0f, mg->fade);
    }
    uint32_t fade = mg->fade;
    for (uint32_t i = 0; i < mg->particles.rings.count; ++i) {
        vc::Call<void>(mg->particles.rings.data[i], 8, fade);
    }
    for (uint32_t i = 0; i < mg->particles.flashes.count; ++i) {
        vc::Call<void>(mg->particles.flashes.data[i], 4, fade);
    }
    if (mg->phase != 2) {
        return;
    }

    // the title screen
    if (artLanguage == 7 || artLanguage == 6) {
        artLabelScale = 0.45f;
        artTextScale = 0.6f;
        artHeadingScale = 0.75f;
        artIconSize = 32.0f;
        artHeadingY = 55.0f;
        artTitleWrap = 450.0f;
        artTitleY = 85.0f;
    }
    uint8_t top[4];
    uint8_t bottom[4];
    RGBA_Set(top, 0x28, 0x28, 0x50, 0xC0);
    RGBA_Set(bottom, 0x14, 0x10, 0x28, 0xC0);
    artTitleSpin = artTimeStep * 0.3926991f + artTitleSpin;
    float spin = artTitleSpin;
    if (artTwoPi < spin) {
        artTitleSpin = spin - artTwoPi;
        spin = artTitleSpin;
    }
    float pulse = Sin(spin) * 0.1f + 1.0f;
    float scaleX = artScreenScaleX;
    float iconW = scaleX * artIconSize * pulse;
    float scaleY = artScreenScaleY;
    float iconH = artIconSize * scaleY * pulse;
    float rect[4];   // left, bottom, right, top
    float areaW = scaleX * 480.0f;
    float left = (640.0f - areaW) * 0.5f;
    rect[0] = areaW * 0.0f + left;
    rect[2] = left + areaW;
    float areaH = float(double(scaleY) * 360.142857);
    float topEdge = (480.0f - areaH) * 0.5f;
    rect[1] = (topEdge + areaH) - 10.0f;
    rect[3] = (topEdge + areaH * 0.0f) - 10.0f;
    DrawRect4(rect, top, top, bottom, bottom);

    fescr::Font_Reset();
    if (artFontDropOn != 0) {
        artFontOutline = 1;
        fescr::g_fontDrop = 0;
    }
    SetFontColour(0xFF, 0xFF, 0xFF, mg->fade);
    SetFontDropColour(mg->fade);
    fescr::Font_SetJustify(2);
    Font_SetScale(artHeadingScale);
    fescr::Font_PrintString(320.0f, artHeadingY + 10.0f, mg->config.text[6], 1);
    fescr::Font_SetJustify(2);
    Font_SetScale(artLabelScale);
    artFontWrap = scaleX * artTitleWrap;
    fescr::Font_PrintString(320.0f, artTitleY + 10.0f, mg->config.text[7], 1);
    fescr::Font_SetJustify(2);
    Font_SetScale(artTextScale);
    fescr::Font_PrintString(320.0f, 160.0f, mg->config.text[13], 1);

    // the enemies: floater, bouncer, line crawler, fast line crawler, exploder
    ArtTextures* t = mg->textures;
    float halfH = iconH * 0.5f;
    float rowY = (200.0f - halfH) + 10.0f;
    float sx140 = scaleX * 140.0f;
    float column0 = 320.0f - sx140;
    float sx25 = scaleX * 25.0f;
    float x = column0 - sx25;
    float halfW = iconW * 0.5f;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->floater : nullptr);
    float sx50 = scaleX * 50.0f;
    x = sx50 + x;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->bouncer : nullptr);
    x = 320.0f - sx25;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->lineCrawler : nullptr);
    x = x + sx50;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->fastLineCrawler : nullptr);
    float column2 = sx140 + 320.0f;
    x = column2;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->exploder : nullptr);
    Font_SetScale(artLabelScale);
    fescr::Font_PrintString(column0, 230.0f, mg->config.text[14], 1);
    fescr::Font_PrintString(320.0f, 230.0f, mg->config.text[15], 1);
    fescr::Font_PrintString(column2, 230.0f, mg->config.text[16], 1);
    fescr::Font_SetJustify(2);
    Font_SetScale(artTextScale);
    fescr::Font_PrintString(320.0f, 275.0f, mg->config.text[8], 1);
    Font_SetScale(artLabelScale);

    // the power-ups: speed boost, clear, freeze
    rowY = (315.0f - halfH) + 10.0f;
    float sx120 = scaleX * 120.0f;
    x = 320.0f - sx120;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->speedBoost : nullptr);
    fescr::Font_PrintString(x, 345.0f, mg->config.text[9], 1);
    x = sx120 * 0.0f + 320.0f;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->clear : nullptr);
    fescr::Font_PrintString(x, 345.0f, mg->config.text[11], 1);
    x = sx120 + 320.0f;
    MGClassArt_DrawIcon(mg, x - halfW, rowY, iconW, iconH, t != nullptr ? t->freeze : nullptr);
    fescr::Font_PrintString(x, 345.0f, mg->config.text[12], 1);
    fescr::Font_Render();
    Font_PrintInSlot(3, mg->config.text[17], 0);
    fescr::Font_Render();
}

// ---- MGClassArt: a life lost ------------------------------------------------------------------

// (another pass's: every object still (state 1) or moving (0); the power-ups too when
// `all`; sets `frozen`)
GAME_FN(void, __thiscall, MGClassArt_SetStill, 0x006DB6E0, MGClassArt* self, int still, int all);

// 0x6DFFC0: the player back on the cursor; the line crawlers and the line power-ups
// restart, the debris goes
void __fastcall MGClassArtResetLife(MGClassArt* mg, re::Edx) {
    ArtPlayer_Reset(mg->player);
    mg->player->state = 0;
    MGClassArt_SetStill(mg, 0, 1);
    for (uint32_t i = 0; i < mg->objects.count; ++i) {
        ArtObject* object = mg->objects.data[i];
        switch (object->kind) {
        case 3:
        case 4:
        case 5:
        case 9:
        case 11:
            ArtLineObject_PickStart(static_cast<ArtLineObject*>(object));
            break;
        case 7:
            Free(object);
            ArtArray_Remove(&mg->objects, i);
            --i;
            break;
        }
    }
}

// ---- ArtObject: drawing -----------------------------------------------------------------------

// 0x6E0060 (virtual +8): blinking while it spawns (its first 60 ticks) and while still
// and blinking, fading out while dying; its glow pulsing under it
void ArtObject::Draw() {
    uint8_t rgba[4];
    RGBA_Set(rgba, 0x80, 0x80, 0x80, 0xFF);
    switch (state) {
    case 0:
        if (age < 60) {
            rgba[3] = ((age / 10) & 1) != 0 ? 0xFF : 0;
        }
        break;
    case 2:
        rgba[3] = ((stateTime / 10) & 1) != 0 ? 0xFF : 0;
        break;
    case 3:
        rgba[3] = uint8_t(stateTime);
        break;
    }
    if (glow != nullptr) {
        glowColour[3] = rgba[3];
        float c = Cos(spin + 1.5707964f);
        ArtObject_DrawSprite(this, glow, glowColour, c * 0.15f + 1.0f);
    }
    float c = Cos(spin);
    ArtObject_DrawSprite(this, texture, rgba, c * 0.1f + 1.0f);
}

// ---- ArtPlayer: construction, update, drawing ---------------------------------------------

// 0x6E0170
ArtPlayer* ArtPlayer::Construct(void* texture_, void* glow_, void* swapTexture_, ArtDeck* deck_, float speed_,
                                float pathSpeedup_, float boostFactor_, float freezeFactor_) {
    ArtObject_Construct(this, 0, texture_);
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x0094437C);
    deck = deck_;
    float inv = 1.0f / U2F(deck_->size);
    swapTexture = swapTexture_;
    powerUpGlow = glow_;
    glideSteps = 3;
    poweredUp = 0;
    radius = inv * 0.49f;
    speed = speed_;
    pathSpeedup = pathSpeedup_;
    boostFactor = boostFactor_;
    freezeFactor = freezeFactor_;
    ArtPlayer_Reset(this);
    state = 1;
    return this;
}

// 0x6E0200 (virtual +4): the pad moves the cursor along the deck: each axis pushed (the
// stronger first) aims `speed` worth that way from the heading, and the deck steps its
// cursor cell by cell to there (each step's result kept: 2 a line closed, 3 a line drawn
// on, 4 back onto its own line); the sprite then glides to the cursor over a few ticks.
// Always 0 while it moves (the base's result otherwise)
int ArtPlayer::Update(uint32_t ticks) {
    float scaleX = artScreenScaleX;
    width = U2F(TexWidth(texture)) * scaleX / (scaleX * 480.0f);
    float scaleY = artScreenScaleY;
    height = U2F(TexHeight(texture)) * scaleY / float(double(scaleY) * 360.142857);
    float inv = 1.0f / U2F(deck->size);
    moved = 0;
    radius = inv * 0.49f;
    int base = ArtObject_Update(this, ticks);
    if (state != 0) {
        return base;
    }
    if (poweredUp != 0) {
        float ticksF = U2F(ticks);
        float dt = ticksF / 60.0f;
        if (dt <= powerUpTime) {
            spin = ticksF * 0.06981317f + spin;
            powerUpTime = powerUpTime - dt;
        } else {
            ArtPlayer_EndPowerUp(this);
        }
    }
    stepResult = 1;
    float stepInv = 1.0f / U2F(deck->size);
    float distance = stepInv * speed * U2F(ticks);
    if (deck->drawing == 0) {
        distance = pathSpeedup * distance;
    }
    if (poweredUp != 0) {
        if (powerUp == 0) {
            distance = boostFactor * distance;
        } else if (powerUp == 2) {
            distance = freezeFactor * distance;
        }
    }
    float pad[2];
    ArtPlayer_ReadPad(this, pad);
    if (poweredUp != 0 && powerUp == 1) {
        pad[0] = -pad[0];
        pad[1] = -pad[1];
    }
    float absX = std::fabs(pad[0]);
    uint8_t horizontal = absX >= 0.25f ? 1 : 0;
    float absY = std::fabs(pad[1]);
    uint8_t vertical = absY >= 0.25f ? 1 : 0;
    float target[2] = {headingX, headingY};
    for (;;) {
        if (horizontal != 0 && (vertical == 0 || absY < absX)) {
            target[0] = pad[0] < 0.0f ? -1.0f : 1.0f;
            horizontal = 0;
            target[1] = 0.0f;
        } else if (vertical != 0) {
            target[0] = 0.0f;
            target[1] = pad[1] < 0.0f ? -1.0f : 1.0f;
            vertical = 0;
        } else {
            break;
        }
        target[0] = headingX + target[0] * distance;
        target[1] = headingY + distance * target[1];
        if (0.0f > target[0]) {
            target[0] = 0.0f;
        }
        if (0.0f > target[1]) {
            target[1] = 0.0f;
        }
        if (target[0] > 1.0f) {
            target[0] = 1.0f;
        }
        if (target[1] > 1.0f) {
            target[1] = 1.0f;
        }
        int col;
        int row;
        ArtDeck_ToGrid(deck, target, &col, &row);
        int dCol = col - int(deck->cursorCol + (deck->cursorHalfX != 0 ? 1 : 0));
        int dRow = row - int(deck->cursorRow + (deck->cursorHalfY != 0 ? 1 : 0));
        if (dCol == 0 && dRow == 0) {
            break;
        }
        for (;;) {
            int dir;
            if (dCol != 0) {
                dir = dCol < 0 ? 3 : 1;
                dCol += dCol < 0 ? 1 : -1;
            } else if (dRow != 0) {
                dir = dRow < 0 ? 0 : 2;
                dRow += dRow < 0 ? 1 : -1;
            } else {
                break;
            }
            int result = ArtDeck_Step(deck, dir);
            stepResult = result;
            switch (result) {
            case 0:   // stopped
                if (horizontal == 0 && vertical == 0) {
                    float cursor[2];
                    float* at = ArtDeck_GetCursor(deck, cursor);
                    target[0] = at[0];
                    target[1] = at[1];
                    horizontal = 0;
                    vertical = 0;
                }
                dCol = 0;
                dRow = 0;
                break;
            case 1:
            case 2:
            case 3:
                horizontal = 0;
                vertical = 0;
                break;
            case 4:   // its own line: back to the heading
                target[0] = headingX;
                target[1] = headingY;
                horizontal = 0;
                vertical = 0;
                dCol = 0;
                dRow = 0;
                break;
            }
        }
    }
    if (target[0] != headingX || target[1] != headingY) {
        headingX = target[0];
        headingY = target[1];
        glideStep = 0;
        glideFromX = x;
        glideFromY = y;
    }
    uint32_t steps = glideSteps;
    if (glideStep < steps) {
        uint32_t step = glideStep + 1;
        glideStep = step;
        float cursor[2];
        float* at = ArtDeck_GetCursor(deck, cursor);
        float dx = at[0] - glideFromX;
        float dy = at[1] - glideFromY;
        float f = U2F(step) / U2F(steps);
        dx = dx * f;
        dy = f * dy;
        x = glideFromX + dx;
        y = glideFromY + dy;
        moved = 1;
    }
    return 0;
}

// 0x6E0770 (virtual +8): drawn 12 pixels right and up of its point
void ArtPlayer::DrawPlayer() {
    float oldY = y;
    float oldX = x;
    float scaleX = artScreenScaleX;
    float offX = scaleX * 12.0f / (scaleX * 480.0f);
    float scaleY = artScreenScaleY;
    float offY = -(12.0f * scaleY / float(double(scaleY) * 360.142857));
    x = offX + x;
    y = y + offY;
    ArtObject_Draw(this);
    x = oldX;
    y = oldY;
}

// ---- ArtFloater / ArtLineObject: update -----------------------------------------------------

// 0x6E07F0 (virtual +4): 4 once a filled area covers it; still, it hurts the player
// running into it while drawing. (The game reads the deck back from ecx after 0x6DA7E0.)
int ArtFloater::Update(uint32_t ticks) {
    int base = ArtObject_Update(this, ticks);
    int state_ = state;
    if (state_ == 3) {
        return base;
    }
    if (ArtDeck_IsFilledAt(deck, &x) != 0) {
        return 4;
    }
    int result = 0;
    if (state_ == 1) {
        if (age >= 60 && deck->drawing != 0) {
            float move[2] = {player->x - playerLastX, player->y - playerLastY};
            if (ArtObject_SweepHits(player, move, this) != 0) {
                result = state_ + 1;
            }
        }
    } else if (state_ == 0) {
        result = ArtFloater_Move(this, ticks);
    }
    playerLastX = player->x;
    playerLastY = player->y;
    return result;
}

// 0x6E08A0: heading (1, -0.95) normalised, turned a quarter `turns` times
void ArtFloater::SetDirection(int turns) {
    dirX = 1.0f;
    dirY = -0.95f;
    Vector2D_Normalise(&dirX);
    for (; turns != 0; --turns) {
        float t = dirX;
        dirX = -dirY;
        dirY = t;
    }
}

// 0x6E08F0 (virtual +4): 4 once its trail is cut by a filled area; still, it hurts the
// player running into it off the lines
int ArtLineObject::Update(uint32_t ticks) {
    int base = ArtObject_Update(this, ticks);
    int state_ = state;
    if (state_ == 3) {
        return base;
    }
    if (ArtLinePath_Cut(&path) != 0) {
        return 4;
    }
    int result = 0;
    if (state_ == 1) {
        if (age >= 60 && deck->drawing == 0) {
            float move[2] = {player->x - playerLastX, player->y - playerLastY};
            if (ArtObject_SweepHits(player, move, this) != 0) {
                result = state_ + 1;
            }
        }
    } else if (state_ == 0) {
        result = ArtLineObject_Move(this, ticks);
    }
    playerLastX = player->x;
    playerLastY = player->y;
    return result;
}

// 0x6E09A0 (virtual +8): moving, at its flash's alpha without a glow
void ArtExploder::DrawExploder() {
    if (state != 0) {
        ArtObject_Draw(this);
        return;
    }
    uint8_t rgba[4];
    RGBA_Set(rgba, 0x80, 0x80, 0x80, alpha);
    float c = Cos(spin);
    ArtObject_DrawSprite(this, texture, rgba, c * 0.1f + 1.0f);
}

// 0x6E0A10: kind 7, past its spawn blink (age 160), a quarter of its size for a radius
ArtDebris* ArtDebris::Construct(void* texture_, ArtDeck* deck_, ArtPlayer* player_, const float* pos,
                                const float* dir, float speed_) {
    ArtObject_Construct(this, 7, texture_);
    deck = deck_;
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x0094438C);
    player = player_;
    x = pos[0];
    y = pos[1];
    dirX = dir[0];
    dirY = dir[1];
    Vector2D_Normalise(&dirX);
    speed = speed_;
    float size = height > width ? width : height;
    age = 0xA0;
    radius = size * 0.5f * 0.5f;
    return this;
}

// ---- ArtFloater: construction --------------------------------------------------------------

// 0x6E14A0: somewhere random on the deck, heading a random quarter
ArtFloater* ArtFloater::Construct(int kind_, void* texture_, ArtDeck* deck_, float speed_, ArtPlayer* player_) {
    ArtObject_Construct(this, kind_, texture_);
    vtable = reinterpret_cast<re::Ptr32<void>*>(0x009443CC);
    deck = deck_;
    player = player_;
    float point[2];
    float* at = ArtDeck_RandomPoint(deck_, point);
    x = at[0];
    y = at[1];
    speed = speed_;
    ArtFloater_SetDirection(this, Rand() % 4);
    return this;
}

// 0x6E1510 (virtual +0): placed from a pre-instance (its point, its turns)
void ArtFloater::Place(const uint8_t* entry) {
    const ArtPreInstance* pre = reinterpret_cast<const ArtPreInstance*>(entry);
    x = pre->x;
    y = pre->y;
    ArtFloater_SetDirection(this, pre->extra);
}

// ---- ArtParticles: effects ----------------------------------------------------------------

// an explosion's three rings (`count` particles each, the given growth and life) and its
// flash
// (the big one's third ring flies at `size`, the small one's at size * 0.8)
void AddExplosionRings(ArtParticles* self, const float* pos, float spread, float size, void* ringTexture,
                       void* flashTexture, uint32_t life, float grow, bool bigThirdSpeed, float size1, float size2,
                       float size3) {
    uint8_t rgba[4];
    ArtRing* ring = ArtParticles_AddRing(self, 20, ringTexture, RGBA_Set(rgba, 0x96, 0x5A, 0, 0xFF), size1, life,
                                         grow);
    ArtRing_Emit(ring, pos, spread, size * 0.75f, 0.0f);
    ring = ArtParticles_AddRing(self, 20, ringTexture, RGBA_Set(rgba, 0xFF, 0xAA, 0, 0xFF), size2, life, grow);
    float speed = size * 0.8f;
    ArtRing_Emit(ring, pos, spread, speed, 0.15707964f);
    ring = ArtParticles_AddRing(self, 20, ringTexture, RGBA_Set(rgba, 0xFF, 0xFF, 0xDC, 0xFF), size3, life, grow);
    ArtRing_Emit(ring, pos, spread, bigThirdSpeed ? size : speed, 0.47123891f);
    ArtFlash* flash = static_cast<ArtFlash*>(Malloc(sizeof(ArtFlash)));
    if (flash != nullptr) {
        uint8_t* colour = RGBA_Set(rgba, 0xFF, 0xFF, 0xDC, 0xFF);
        flash->vtable = reinterpret_cast<re::Ptr32<void>*>(0x009442F0);
        flash->x = pos[0];
        flash->y = pos[1];
        flash->texture = flashTexture;
        std::memcpy(&flash->colour, colour, 4);
        flash->age = 0;
        flash->size = size;
        flash->life = 30;
    }
    PushBack(self->flashes, flash);
}

// 0x6E0AA0: a small explosion (rings of a second, half again their size)
void ArtParticles::AddExplosion(const float* pos, float spread, float size, void* ringTexture, void* flashTexture) {
    AddExplosionRings(this, pos, spread, size, ringTexture, flashTexture, 60, 0.5f, false, 0.04f, 0.033333335f, 0.02f);
}

// 0x6E0CA0: the player's explosion (rings of three seconds, a quarter again their size)
void ArtParticles::AddBigExplosion(const float* pos, float spread, float size, void* ringTexture,
                                   void* flashTexture) {
    AddExplosionRings(this, pos, spread, size, ringTexture, flashTexture, 180, 0.25f, true, 0.02f, 0.016666668f,
                      0.014285714f);
}

// 0x6E0EA0: the sparkle of a line closed
void ArtParticles::AddSparkle(const float* pos, void* texture) {
    uint8_t rgba[4];
    ArtRing* ring = ArtParticles_AddRing(this, 6, texture, RGBA_Set(rgba, 0x78, 0xB4, 0xF0, 0xFF), 0.01f, 60, 0.5f);
    ArtRing_Emit(ring, pos, 0.01f, 0.02f, 0.0f);
    ring = ArtParticles_AddRing(this, 4, texture, RGBA_Set(rgba, 0xFF, 0xFF, 0xFF, 0xFF), 0.008333334f, 60, 0.5f);
    ArtRing_Emit(ring, pos, 0.01f, 0.025f, 0.0f);
}

// ---- MGClassArt: construction and clean-up ----------------------------------------------------

GAME_FN(void, __thiscall, Minigame_Construct, 0x007059D0, void* self);
GAME_FN(void, __thiscall, Minigame_Destruct, 0x00705A50, void* self);
GAME_FN(void, __thiscall, MGClassArt_SetDefaults, 0x006DA040, MGClassArt* self);
GAME_FN(void, __thiscall, MGClassArt_Cleanup, 0x006E1010, MGClassArt* self);
GAME_FN(void, __cdecl, NiRelease, 0x005F0F90, void* object);
GAME_FN(void, __cdecl, Streaming_RemoveModel, 0x0052F640, int id);

// 0x6E0F80
MGClassArt* __fastcall MGClassArtConstruct(MGClassArt* mg, re::Edx) {
    Minigame_Construct(mg);
    mg->vtable = reinterpret_cast<re::Ptr32<void>*>(0x009443AC);
    std::memset(&mg->particles, 0, sizeof(mg->particles));
    std::memset(&mg->icons, 0, sizeof(mg->icons));
    std::memset(&mg->objects, 0, sizeof(mg->objects));
    mg->unknown38 = 1;
    mg->unknown3C = 6;
    mg->hud = 0;
    mg->success = 0;
    mg->active = 0;
    MGClassArt_SetDefaults(mg);
    mg->setUp = 0;
    mg->txdSlot = -1;
    return mg;
}

// 0x6E1010: everything the set-up made released (the textures, the txd, the deck, the
// player, the objects, the icons, the particles)
void __fastcall MGClassArtCleanup(MGClassArt* mg, re::Edx) {
    if (mg->setUp == 0) {
        return;
    }
    NiRelease(mg->frameTexture);
    mg->frameTexture = nullptr;
    NiRelease(mg->backTexture);
    mg->backTexture = nullptr;
    Streaming_RemoveModel(mg->txdSlot + 0x2FA8);
    mg->txdSlot = -1;
    if (mg->deck != nullptr) {
        vc::Call<void*>(mg->deck, 0, 1);
    }
    mg->deck = nullptr;
    if (mg->player != nullptr) {
        Free(mg->player);
    }
    mg->player = nullptr;
    for (uint32_t i = 0; i < mg->objects.count; ++i) {
        if (mg->objects.data[i] != nullptr) {
            Free(mg->objects.data[i]);
        }
    }
    if (mg->objects.data != nullptr) {
        Free(mg->objects.data);
        mg->objects.data = nullptr;
    }
    mg->objects.capacity = 0;
    mg->objects.count = 0;
    for (uint32_t i = 0; i < mg->icons.count; ++i) {
        if (mg->icons.data[i] != nullptr) {
            Free(mg->icons.data[i]);
        }
    }
    if (mg->icons.data != nullptr) {
        Free(mg->icons.data);
        mg->icons.data = nullptr;
    }
    mg->icons.capacity = 0;
    mg->icons.count = 0;
    ArtParticles_Clear(&mg->particles);
    mg->setUp = 0;
}

// ---- MGClassArt: hits ---------------------------------------------------------------------------

// 0x6E1150: the player is hit (by `killer`): the power-up icons fade, the sounds stop, it
// explodes; everything still, the killer blinking; a life is lost (phase 3)
void __fastcall MGClassArtKillPlayer(MGClassArt* mg, re::Edx, ArtObject* killer) {
    for (uint32_t i = 0; i < mg->icons.count; ++i) {
        mg->icons.data[i]->state = 2;
    }
    StopSound(mg->powerUpSound);
    StopSound(mg->brushSound);
    StopSound(mg->lineSound);
    PlaySound(mg, ART_SOUND_PLAYER_HITS_CRAWLER);
    void* flash = mg->textures != nullptr ? mg->textures->flash : nullptr;
    void* glow = mg->textures != nullptr ? mg->textures->glow : nullptr;
    float scaleX = artScreenScaleX;
    float areaW = 480.0f * scaleX;
    float size = scaleX * 100.0f / areaW;
    float spread = scaleX * 20.0f / areaW;
    ArtParticles_AddBigExplosion(&mg->particles, &mg->player->x, spread, size, glow, flash);
    mg->killer = killer;
    MGClassArt_SetStill(mg, 1, 1);
    mg->player->state = 1;
    if (killer != mg->player) {
        killer->stateTime = 0;
        killer->state = 2;
    }
    Pad_Rumble(0, 100, 200, 0);
    mg->phaseTime = 0;
    mg->phase = 3;
}

// 0x6E12B0: an object destroyed (fading out) with a small explosion
void __fastcall MGClassArtDestroyObject(MGClassArt* mg, re::Edx, ArtObject* object) {
    object->stateTime = 0xFF;
    object->state = 3;
    void* flash = mg->textures != nullptr ? mg->textures->flash : nullptr;
    void* glow = mg->textures != nullptr ? mg->textures->glow : nullptr;
    float scaleX = artScreenScaleX;
    float areaW = 480.0f * scaleX;
    float size = scaleX * 50.0f / areaW;
    float spread = scaleX * 20.0f / areaW;
    ArtParticles_AddExplosion(&mg->particles, &object->x, spread, size, glow, flash);
    PlaySound(mg, ART_SOUND_ENEMY_DESTROYED);
    Pad_Rumble(0, 100, 200, 0);
}

// 0x6E1380: the clear power-up: the enemies within its radius of `pos` destroyed
void __fastcall MGClassArtClearRadius(MGClassArt* mg, re::Edx, const float* pos) {
    for (uint32_t i = 0; i < mg->objects.count; ++i) {
        ArtObject* object = mg->objects.data[i];
        int kind = object->kind;
        if (kind < 1 || kind > 8 || kind == 7 || object->state == 3) {
            continue;
        }
        float dx = object->x - pos[0];
        float dy = object->y - pos[1];
        if (mg->config.clearRadius >= Sqrt(std::fabs(dx * dx + dy * dy))) {
            object->stateTime = 0xFF;
            object->state = 3;
        }
    }
    void* flash = mg->textures != nullptr ? mg->textures->flash : nullptr;
    void* glow = mg->textures != nullptr ? mg->textures->glow : nullptr;
    float radius = mg->config.clearRadius;
    ArtParticles_AddExplosion(&mg->particles, pos, radius, radius + radius, glow, flash);
    Pad_Rumble(0, 100, 0xFF, 0);
}

// ---- MGClassArt: the minigame's own virtuals ------------------------------------------------

GAME_FN(void, __thiscall, Minigame_EndSounds, 0x00704DE0, void* self);
GAME_FN(void, __thiscall, Minigame_EndTextures, 0x00704FD0, void* self);
GAME_FN(void, __thiscall, MGClassArtDestructGame, 0x006E1530, MGClassArt* self);
GAME_FN(void, __thiscall, MGClassArt_Setup, 0x006E15F0, MGClassArt* self, const ArtConfig* config);
GAME_FN(void, __thiscall, MGClassArt_StartReversal, 0x006DC8E0, MGClassArt* self);
GAME_FN(void, __thiscall, MGClassArt_ClearRadius, 0x006E1380, MGClassArt* self, const float* pos);
GAME_FN(int, __cdecl, Txd_FindTxdSlot, 0x005F22D0, const char* name, int seasonal);
GAME_FN(void, __cdecl, Streaming_RequestModel, 0x00532020, int id, int flags);
GAME_FN(void, __cdecl, Streaming_LoadAllRequestedModels, 0x00533EE0, int);
GAME_FN(void, __cdecl, Txd_PushCurrentTxd, 0x005F0A90);
GAME_FN(void, __cdecl, Txd_SetCurrentTxd, 0x005F1100, int slot);
GAME_FN(void, __cdecl, Txd_PopCurrentTxd, 0x005F0AA0);
GAME_FN(void*, __cdecl, Txd_FindTexture, 0x005F1770, const char* name, const char* mask);

// a minigame's texture and sound tables (Minigame::LoadTextures / LoadSounds)
struct MinigameTextureEntry {
    int32_t id;
    re::Ptr32<const char> name;
    re::Ptr32<const char> mask;
};
static_assert(sizeof(MinigameTextureEntry) == 0xC);
struct MinigameSoundEntry {
    int32_t id;
    re::Ptr32<const char> name;
};
static_assert(sizeof(MinigameSoundEntry) == 0x8);
GAME_FN(void, __thiscall, Minigame_LoadTextures, 0x00704D30, void* self, int count,
        const MinigameTextureEntry* entries, const char* txd);
GAME_FN(void, __thiscall, Minigame_LoadSounds, 0x00704E90, void* self, int count, const MinigameSoundEntry* entries,
        const char* bank);

inline const char* GameString(uintptr_t address) {
    return reinterpret_cast<const char*>(address);
}

// 0x6E1530: the destructor (the arrays' storage freed, then Minigame's)
void __fastcall MGClassArtDestruct(MGClassArt* mg, re::Edx) {
    mg->vtable = reinterpret_cast<re::Ptr32<void>*>(0x009443AC);
    MGClassArt_Cleanup(mg);
    if (mg->objects.data != nullptr) {
        Free(mg->objects.data);
        mg->objects.data = nullptr;
    }
    if (mg->icons.data != nullptr) {
        Free(mg->icons.data);
        mg->icons.data = nullptr;
    }
    ArtParticles_Clear(&mg->particles);
    if (mg->particles.rings.data != nullptr) {
        Free(mg->particles.rings.data);
        mg->particles.rings.data = nullptr;
    }
    if (mg->particles.flashes.data != nullptr) {
        Free(mg->particles.flashes.data);
        mg->particles.flashes.data = nullptr;
    }
    Minigame_Destruct(mg);
}

// 0x6E15C0 (virtual +0x14): the game ends
void __fastcall MGClassArtEnd(MGClassArt* mg, re::Edx) {
    mg->active = 0;
    artMinigameRunning = 0;
    MGClassArt_Cleanup(mg);
    Minigame_EndSounds(mg);
    Minigame_EndTextures(mg);
}

// 0x6E15F0: the game set up from the scripts' settings: the txd "MG_Art_<bank + 1>" and
// its backgrounds ("BG_<n>" and "BG_<n>_mono"), the deck, the player, the spawners (each
// waiting its initial period when it has one), the state from the start
void __fastcall MGClassArtSetup(MGClassArt* mg, re::Edx, const ArtConfig* scriptConfig) {
    MGClassArt_Cleanup(mg);
    std::memcpy(&mg->config, scriptConfig, sizeof(ArtConfig));
    char bank = char(mg->config.textureBank + '1');
    char txdName[] = "MG_Art_X";
    char back[] = "BG_X";
    char mono[] = "BG_X_mono";
    txdName[7] = bank;
    back[3] = bank;
    mono[3] = bank;
    int slot = Txd_FindTxdSlot(txdName, 0);
    mg->txdSlot = slot;
    Streaming_RequestModel(slot + 0x2FA8, 9);
    Streaming_LoadAllRequestedModels(1);
    Txd_PushCurrentTxd();
    Txd_SetCurrentTxd(mg->txdSlot);
    mg->frameTexture = Txd_FindTexture(back, nullptr);
    mg->backTexture = Txd_FindTexture(mono, nullptr);
    Txd_PopCurrentTxd();
    void* memory = Malloc(sizeof(ArtDeck));
    mg->deck = memory != nullptr
                   ? ArtDeck_Construct(memory, mg->config.deckResolution, mg->backTexture, mg->frameTexture)
                   : nullptr;
    memory = Malloc(sizeof(ArtPlayer));
    ArtPlayer* player = nullptr;
    if (memory != nullptr) {
        ArtTextures* t = mg->textures;
        player = static_cast<ArtPlayer*>(ArtPlayer_Construct(
            memory, t != nullptr ? t->player : nullptr, t != nullptr ? t->glow : nullptr,
            t != nullptr ? t->playerReversal : nullptr, mg->deck, mg->config.playerSpeed, mg->config.pathSpeedup,
            mg->config.speedBoostFactor, mg->config.freezeFactor));
    }
    mg->player = player;
    for (int i = 0; i < 7; ++i) {
        ArtSpawner& s = mg->enemySpawners[i];
        s.active = 0;
        s.spawned = 0;
        s.waiting = mg->config.enemyInitialPeriod[i] != 0.0f ? 1 : 0;
        s.timer = mg->config.enemyInitialPeriod[i];
    }
    for (int i = 0; i < 4; ++i) {
        ArtSpawner& s = mg->powerUpSpawners[i];
        s.active = 0;
        s.spawned = 0;
        s.waiting = mg->config.powerUpInitialPeriod[i] != 0.0f ? 1 : 0;
        s.timer = mg->config.powerUpInitialPeriod[i];
    }
    mg->enemyCount = 0;
    mg->powerUpCount = 0;
    mg->frozen = 0;
    mg->phase = 0;
    mg->phaseTime = 0;
    mg->fade = 0;
    mg->killer = nullptr;
    mg->showMessage = 0;
    mg->powerUpSound = nullptr;
    mg->brushSound = nullptr;
    mg->lineSound = nullptr;
    mg->music = nullptr;
    mg->sound608 = nullptr;
    mg->unknown60C = 0;
    mg->unknown638 = 0;
    mg->unknown63C = 0;
    mg->lastTime = artTimeInMs;
    mg->firstLife = 1;
    mg->colour630[0] = 100;
    mg->colour630[1] = 0;
    mg->colour630[2] = 0;
    mg->colour630[3] = 0xFF;
    mg->colour634[0] = 0;
    mg->colour634[1] = 100;
    mg->colour634[2] = 0;
    mg->colour634[3] = 0xFF;
    mg->failureCode = 0;
    mg->preInstanced = 0;
    mg->setUp = 1;
}

// 0x6E18C0: the player picked up a power-up: speed boost (9), freeze (10: the enemies
// still) or reversal (11) start on the player with their sound and an icon (the older
// icons fading); clear (12) destroys the enemies around it
void __fastcall MGClassArtCollect(MGClassArt* mg, re::Edx, ArtObject* powerUp) {
    if (mg->powerUpSound != nullptr && powerUp->kind != 12) {
        mg->powerUpSound[0x5C] = 1;
        mg->powerUpSound = nullptr;
    }
    void* iconTexture = nullptr;
    const uint16_t* name;
    uint8_t colour[4];
    float time;
    ArtSound sound;
    ArtTextures* t = mg->textures;
    switch (powerUp->kind) {
    case 9:
        if (t != nullptr) {
            iconTexture = t->speedBoost;
        }
        std::memcpy(colour, mg->config.powerUpColour[0], 4);
        time = mg->config.powerUpDuration[0];
        name = mg->config.text[3];
        ArtPlayer_StartPowerUp(mg->player, 0, colour, time);
        sound = ART_SOUND_SPEED_BOOST;
        break;
    case 10:
        if (t != nullptr) {
            iconTexture = t->freeze;
        }
        std::memcpy(colour, mg->config.powerUpColour[2], 4);
        time = mg->config.powerUpDuration[2];
        name = mg->config.text[4];
        MGClassArt_SetStill(mg, 1, 0);
        ArtPlayer_StartPowerUp(mg->player, 2, colour, time);
        sound = ART_SOUND_GLUE_FREEZE;
        break;
    case 11:
        if (t != nullptr) {
            iconTexture = t->reversal;
        }
        std::memcpy(colour, mg->config.powerUpColour[1], 4);
        time = mg->config.powerUpDuration[1];
        name = mg->config.text[5];
        MGClassArt_StartReversal(mg);
        ArtPlayer_StartPowerUp(mg->player, 1, colour, time);
        sound = ART_SOUND_SCISSORS;
        break;
    case 12:
        MGClassArt_ClearRadius(mg, &powerUp->x);
        PlaySound(mg, ART_SOUND_ERASER);
        return;
    default:
        return;
    }
    mg->powerUpSound = PlaySound(mg, sound);
    if (iconTexture == nullptr) {
        return;
    }
    for (uint32_t i = 0; i < mg->icons.count; ++i) {
        mg->icons.data[i]->state = 2;
    }
    void* memory = Malloc(sizeof(ArtPowerUpIcon));
    ArtPowerUpIcon* icon = nullptr;
    if (memory != nullptr) {
        void* glow = mg->textures != nullptr ? mg->textures->glow : nullptr;
        icon = ArtPowerUpIcon_Construct(memory, iconTexture, name, glow, colour, time);
    }
    PushBack(mg->icons, icon);
}

// 0x6E1B70: an enemy of the kind: 0 line crawler, 1 fast line crawler, 3 floater,
// 4 bouncer, 6 exploder (2 and 5, the homer and the smart line crawler, make none: the
// game then writes through null when frozen); still while the enemies are frozen
ArtObject* __fastcall MGClassArtCreateEnemy(MGClassArt* mg, re::Edx, int kind) {
    ArtTextures* t = mg->textures;
    ArtObject* object = nullptr;
    void* memory;
    switch (kind) {
    case 0:
        memory = Malloc(sizeof(ArtLineObject));
        if (memory != nullptr) {
            object = ArtLineObject_Construct(memory, 3, t != nullptr ? t->lineCrawler : nullptr, mg->deck,
                                             mg->config.enemySpeed[0], mg->player);
        }
        break;
    case 1:
        memory = Malloc(sizeof(ArtLineObject));
        if (memory != nullptr) {
            object = ArtLineObject_Construct(memory, 4, t != nullptr ? t->fastLineCrawler : nullptr, mg->deck,
                                             mg->config.enemySpeed[1], mg->player);
        }
        break;
    case 3:
        memory = Malloc(sizeof(ArtFloater));
        if (memory != nullptr) {
            object = ArtFloater_Construct(memory, 1, t != nullptr ? t->floater : nullptr, mg->deck,
                                          mg->config.enemySpeed[3], mg->player);
        }
        break;
    case 4:
        memory = Malloc(sizeof(ArtFloater));
        if (memory != nullptr) {
            object = ArtFloater_Construct(memory, 2, t != nullptr ? t->bouncer : nullptr, mg->deck,
                                          mg->config.enemySpeed[4], mg->player);
        }
        break;
    case 6:
        memory = Malloc(sizeof(ArtExploder));
        if (memory != nullptr) {
            object = ArtExploder_Construct(memory, t != nullptr ? t->exploder : nullptr, mg->deck,
                                           mg->config.exploderTimeOut, mg->player);
        }
        break;
    }
    if (mg->frozen != 0) {
        object->state = 1;
    }
    return object;
}

// 0x6E1D20: a power-up of the kind: 0 speed boost, 1 reversal (line objects), 2 freeze,
// 3 clear (floaters), glowing in its colour
ArtObject* __fastcall MGClassArtCreatePowerUp(MGClassArt* mg, re::Edx, int kind) {
    ArtTextures* t = mg->textures;
    void* glow = t != nullptr ? t->glow : nullptr;
    ArtObject* object = nullptr;
    void* memory;
    switch (kind) {
    case 0:
        memory = Malloc(sizeof(ArtLineObject));
        if (memory != nullptr) {
            object = ArtLineObject_Construct(memory, 9, t != nullptr ? t->speedBoost : nullptr, mg->deck,
                                             mg->config.powerUpSpeed[0], mg->player);
        }
        break;
    case 1:
        memory = Malloc(sizeof(ArtLineObject));
        if (memory != nullptr) {
            object = ArtLineObject_Construct(memory, 11, t != nullptr ? t->reversal : nullptr, mg->deck,
                                             mg->config.powerUpSpeed[1], mg->player);
        }
        break;
    case 2:
        memory = Malloc(sizeof(ArtFloater));
        if (memory != nullptr) {
            object = ArtFloater_Construct(memory, 10, t != nullptr ? t->freeze : nullptr, mg->deck,
                                          mg->config.powerUpSpeed[2], mg->player);
        }
        break;
    case 3:
        memory = Malloc(sizeof(ArtFloater));
        if (memory != nullptr) {
            object = ArtFloater_Construct(memory, 12, t != nullptr ? t->clear : nullptr, mg->deck,
                                          mg->config.powerUpSpeed[3], mg->player);
        }
        break;
    default:
        return nullptr;
    }
    object->glow = glow;
    std::memcpy(object->glowColour, mg->config.powerUpColour[kind], 4);
    return object;
}

// 0x6E1F30 (virtual +0): the deleting destructor
MGClassArt* __fastcall MGClassArtDelete(MGClassArt* mg, re::Edx, int flags) {
    MGClassArtDestructGame(mg);
    if ((flags & 1) != 0) {
        Free(mg);
    }
    return mg;
}

// 0x6E1F50 (virtual +0x10): the game starts: its textures (txd "MG_Art") and sounds
// (bank "ArtClass") loaded, then set up from the scripts' settings
void __fastcall MGClassArtStart(MGClassArt* mg, re::Edx) {
    const MinigameTextureEntry textures[19] = {
        {0, GameString(0x00944680), GameString(0x00944674)},    // Bouncer
        {1, GameString(0x0094466C), GameString(0x00944664)},    // Clear
        {2, GameString(0x00944658), GameString(0x0094464C)},    // Exploder
        {3, GameString(0x0094463C), GameString(0x00944628)},    // Exploder_Debris
        {4, GameString(0x00944618), GameString(0x00944604)},    // Explosion_Flash
        {5, GameString(0x009445F4), GameString(0x009445E0)},    // FastLineCrawler
        {6, GameString(0x009445D8), GameString(0x009445CC)},    // Floater
        {7, GameString(0x009445C4), GameString(0x009445B8)},    // Freeze
        {8, GameString(0x009445B0), GameString(0x009445A8)},    // Glow
        {9, GameString(0x009445A0), GameString(0x00944598)},    // Homer
        {10, GameString(0x0094458C), GameString(0x0094457C)},   // LineCrawler
        {11, GameString(0x00944574), GameString(0x0094456C)},   // lives
        {12, GameString(0x0094455C), GameString(0x0094454C)},   // Mark_Particle
        {13, GameString(0x0092BBE8), GameString(0x00944540)},   // Player
        {14, GameString(0x00944530), GameString(0x0094451C)},   // Player_Reversal
        {15, GameString(0x00944510), GameString(0x00944504)},   // Reversal
        {16, GameString(0x009444F0), GameString(0x009444DC)},   // SmartLineCrawler
        {17, GameString(0x009444D0), GameString(0x009444C0)},   // SpeedBoost
        {18, GameString(0x0092BC04), GameString(0x009444B8)},   // Frame
    };
    Minigame_LoadTextures(mg, 19, textures, GameString(0x00944688));   // "MG_Art"
    const MinigameSoundEntry sounds[16] = {
        {0, GameString(0x009444B0)},    // Brush
        {1, GameString(0x009444A4)},    // LineDraw
        {2, GameString(0x00944498)},    // LineSuccess
        {3, GameString(0x0094448C)},    // TimeWarning
        {4, GameString(0x0094447C)},    // PlyrHitsCrawlr
        {5, GameString(0x00944470)},    // EnemySpawn
        {6, GameString(0x00944460)},    // EnemyDestroyd
        {7, GameString(0x00944444)},    // HatrickDestroyd
        {8, GameString(0x00944438)},    // PwrUpSpawn
        {9, GameString(0x0094442C)},    // GlueFreeze
        {10, GameString(0x00944420)},   // Scissors
        {11, GameString(0x009444D0)},   // SpeedBoost
        {12, GameString(0x00944418)},   // Erasor
        {13, GameString(0x0094440C)},   // ArtMusic
        {14, GameString(0x00944404)},   // ArtFail
        {15, GameString(0x009443F8)},   // ArtSuccess
    };
    Minigame_LoadSounds(mg, 16, sounds, GameString(0x00944454));   // "ArtClass"
    MGClassArt_Setup(mg, &mg->scriptConfig);
    mg->active = 1;
    mg->success = 0;
    artMinigameRunning = 1;
}

// ---- MGClassArt: the game's update ------------------------------------------------------------

GAME_FN(ArtObject*, __thiscall, MGClassArt_CreateEnemy, 0x006E1B70, MGClassArt* self, int kind);
GAME_FN(ArtObject*, __thiscall, MGClassArt_CreatePowerUp, 0x006E1D20, MGClassArt* self, int kind);
GAME_FN(void, __thiscall, MGClassArt_UpdateSpawns, 0x006E2330, MGClassArt* self, uint32_t ms, int frozen);
GAME_FN(void, __thiscall, MGClassArt_UpdateGame, 0x006E2750, MGClassArt* self, uint32_t ms, uint32_t ticks);
GAME_FN(void, __thiscall, MGClassArt_KillPlayer, 0x006E1150, MGClassArt* self, ArtObject* killer);
GAME_FN(void, __thiscall, MGClassArt_DestroyObject, 0x006E12B0, MGClassArt* self, ArtObject* object);
GAME_FN(void, __thiscall, MGClassArt_Collect, 0x006E18C0, MGClassArt* self, ArtObject* powerUp);
GAME_FN(void, __thiscall, MGClassArt_ResetLife, 0x006DFFC0, MGClassArt* self);
// (another pass's: the music and +0x608 stopped; the other sounds stopped, the player
// fading out when `fadePlayer`; an object's spawner counts given back; the icons'
// update; the exit)
GAME_FN(void, __thiscall, MGClassArt_StopMusic, 0x006DA010, MGClassArt* self);
GAME_FN(void, __thiscall, MGClassArt_StopSounds, 0x006DB650, MGClassArt* self, int fadePlayer);
GAME_FN(void, __thiscall, MGClassArt_Uncount, 0x006DB770, MGClassArt* self, ArtObject* object);
GAME_FN(void, __thiscall, MGClassArt_UpdateIcons, 0x006DC880, MGClassArt* self, uint32_t ticks);
GAME_FN(void, __thiscall, MGClassArt_Exit, 0x006DC7D0, MGClassArt* self, int, int);
GAME_FN(void, __thiscall, ArtDeck_ClearLine, 0x006DA870, ArtDeck* self);
GAME_FN(void, __thiscall, ArtDeck_ResetCursor, 0x006DA480, ArtDeck* self);
GAME_FN(void*, __thiscall, Minigame_GetTexture, 0x006D7CB0, void* self, int index);
GAME_FN(int, __thiscall, Popup_Result, 0x00705080, void* popup);
GAME_FN(void, __thiscall, Popup_Show, 0x00705220, void* popup, int, const char* title, const char* buttons, int,
        int);
GAME_FN(void, __cdecl, Fn_45B960, 0x0045B960, int);

// a spawner's turn: counts down while it waits; at zero it spawns while there are fewer
// than its kind's and the total's maximums, then waits its period again (none: stops).
// (Below the fill threshold an inactive spawner still runs; reaching it only sets the
// spawner active, skipping that tick: the game's check reads that way)
template <typename Create>
void UpdateSpawner(MGClassArt* mg, ArtSpawner& s, int kind, uint32_t ms, uint32_t threshold, uint32_t instanceMax,
                   uint32_t& count, uint32_t totalMax, const float& period, ArtSound sound, Create create) {
    if (s.active == 0 && FilledPercent(mg->deck) >= threshold) {
        s.active = 1;
        return;
    }
    if (s.waiting == 0) {
        return;
    }
    if (!(s.timer <= 0.0f)) {
        s.timer = s.timer - U2F(ms) / 1000.0f;
        return;
    }
    if (s.spawned >= instanceMax || count >= totalMax) {
        return;
    }
    ArtObject* object = create(mg, kind);
    PushBack(mg->objects, object);
    PlaySound(mg, sound);
    s.spawned += 1;
    count += 1;
    if (period == 0.0f) {
        s.waiting = 0;
    } else {
        s.timer = period;
    }
}

// 0x6E2330: the pre-instances created once, then the spawners (the enemies' only while
// they aren't frozen)
void __fastcall MGClassArtUpdateSpawns(MGClassArt* mg, re::Edx, uint32_t ms, int frozen) {
    if (mg->preInstanced == 0) {
        for (uint32_t i = 0; i < mg->config.preInstanceCount; ++i) {
            ArtPreInstance* pre = &mg->scriptConfig.preInstances[i];
            ArtObject* object;
            if (pre->enemy != 0) {
                object = MGClassArt_CreateEnemy(mg, pre->kind);
                mg->enemySpawners[pre->kind].spawned += 1;
                mg->enemyCount += 1;
            } else {
                object = MGClassArt_CreatePowerUp(mg, pre->kind);
                mg->powerUpSpawners[pre->kind].spawned += 1;
                mg->powerUpCount += 1;
            }
            vc::Call<void>(object, 0, pre);
            // (the vector's growth inlined here)
            ArtArray<ArtObject>& objects = mg->objects;
            if (objects.count == objects.capacity) {
                uint32_t capacity = objects.capacity == 0 ? 1 : objects.capacity * 2;
                objects.capacity = capacity;
                re::Ptr32<ArtObject>* data = nullptr;
                if (capacity != 0) {
                    uint64_t bytes = uint64_t(capacity) * 4;
                    data = static_cast<re::Ptr32<ArtObject>*>(Malloc(bytes > 0xFFFFFFFFu ? 0xFFFFFFFFu : uint32_t(bytes)));
                }
                if (objects.data != nullptr) {
                    for (uint32_t j = 0; j < objects.count; ++j) {
                        data[j] = objects.data[j];
                    }
                    Free(objects.data);
                }
                objects.data = data;
            }
            objects.data[objects.count] = object;
            objects.count += 1;
        }
        mg->preInstanced = 1;
    }
    if (re::Flag(frozen) == false) {
        for (int i = 0; i < 7; ++i) {
            UpdateSpawner(mg, mg->enemySpawners[i], i, ms, mg->config.enemySpawnThreshold[i],
                          mg->config.enemyInstanceMax[i], mg->enemyCount, mg->config.enemyTotalMax,
                          mg->config.enemyPeriod[i], ART_SOUND_ENEMY_SPAWN, MGClassArt_CreateEnemy);
        }
    }
    for (int i = 0; i < 4; ++i) {
        UpdateSpawner(mg, mg->powerUpSpawners[i], i, ms, mg->config.powerUpSpawnThreshold[i],
                      mg->config.powerUpInstanceMax[i], mg->powerUpCount, mg->config.powerUpTotalMax,
                      mg->config.powerUpPeriod[i], ART_SOUND_POWER_UP_SPAWN, MGClassArt_CreatePowerUp);
    }
}

// 0x6E2750: one update of `ticks` (`ms` elapsed): the phase (fading in, waiting for the
// start button, a life lost, won or lost and fading out: the game ends), the player and
// its brush and line sounds, the deck's step results, the spawners, then each object's
// update result handled (gone, hit the player or a power-up collected, hit a line,
// caught, exploded into debris); the game is won once the deck is filled to the
// completion threshold
void __fastcall MGClassArtUpdateGame(MGClassArt* mg, re::Edx, uint32_t ms, uint32_t ticks) {
    switch (mg->phase) {
    case 0:   // fading in
        mg->phaseTime += ticks * 4;
        if (mg->phaseTime < 0xFF) {
            ArtDeck_SetAlpha(mg->deck, uint8_t(mg->phaseTime));
            mg->deck->alpha6B = uint8_t(mg->phaseTime);
            mg->fade = uint8_t(mg->phaseTime);
            return;
        }
        ArtDeck_SetAlpha(mg->deck, 0xFF);
        mg->deck->alpha6B = 0xFF;
        mg->fade = 0xFF;
        mg->phase = 2;
        mg->phaseTime = 0;
        mg->music = PlaySound(mg, ART_SOUND_MUSIC);
        return;
    case 2:   // the title screen: the start button
        if (Pad_JustDown(7, 0) != 0) {
            mg->showMessage = 1;
            mg->phase = 3;
        }
        break;
    case 3:   // a life lost (or the start): two seconds, then the start button
        mg->phaseTime += ticks;
        if (mg->phaseTime >= 0x78) {
            mg->showMessage = 1;
            if (mg->killer != nullptr) {
                if (mg->killer->state == 2) {
                    mg->killer->state = 1;
                }
                mg->killer = nullptr;
            }
            if (mg->config.lives == 0) {
                MGClassArt_StopMusic(mg);
                mg->showMessage = 0;
                MGClassArt_StopSounds(mg, 0);
                mg->failureCode = 1;
                mg->phaseTime = 0;
                mg->phase = 5;
                PlaySound(mg, ART_SOUND_FAIL);
                return;
            }
            if (Pad_JustDown(7, 0) != 0) {
                mg->config.lives -= 1;
                mg->showMessage = 0;
                mg->firstLife = 0;
                ArtDeck_ClearLine(mg->deck);
                ArtDeck_ResetCursor(mg->deck);
                MGClassArt_ResetLife(mg);
                mg->phaseTime = 0;
                mg->phase = 1;
            } else if (Pad_JustDown(6, 0) != 0) {
                mg->showMessage = 0;
                mg->phase = 2;
            }
        }
        break;
    case 4:   // won: the deck's drawing fades (forty seconds or the button), then 6
        mg->phaseTime += ticks * 4;
        if (mg->phaseTime < 0xFF) {
            ArtDeck_SetAlpha(mg->deck, uint8_t(0xFF - uint8_t(mg->phaseTime)));
            mg->deck->alpha67 = uint8_t(mg->phaseTime);
            mg->fade = uint8_t(0xFF - uint8_t(mg->phaseTime));
        } else {
            ArtDeck_SetAlpha(mg->deck, 0);
            mg->deck->alpha67 = 0xFF;
            mg->fade = 0;
            if (mg->phaseTime >= 0x960 || Pad_JustDown(7, 0) != 0) {
                mg->phaseTime = 0;
                mg->phase = 6;
            }
        }
        break;
    case 5:   // lost: everything fades, the game ends
        mg->phaseTime += ticks * 4;
        if (mg->phaseTime < 0xFF) {
            uint8_t a = uint8_t(0xFF - uint8_t(mg->phaseTime));
            ArtDeck_SetAlpha(mg->deck, a);
            mg->deck->alpha6B = a;
            mg->deck->alpha63 = a;
            mg->fade = a;
            break;
        }
        ArtDeck_SetAlpha(mg->deck, 0);
        mg->deck->alpha6B = 0;
        mg->deck->alpha63 = 0;
        mg->fade = 0;
        mg->active = 0;
        artMinigameRunning = 0;
        mg->success = 0;
        return;
    case 6:   // won: the picture fades, the game ends
        mg->phaseTime += ticks * 4;
        if (mg->phaseTime < 0xFF) {
            uint8_t a = uint8_t(0xFF - uint8_t(mg->phaseTime));
            mg->deck->alpha6B = a;
            mg->deck->alpha63 = a;
            mg->deck->alpha67 = a;
            break;
        }
        mg->deck->alpha6B = 0;
        mg->deck->alpha63 = 0;
        mg->deck->alpha67 = 0;
        mg->active = 0;
        artMinigameRunning = 0;
        mg->success = 1;
        return;
    }

    vc::Call<int>(mg->player, 4, ticks);
    if (mg->player->moved != 0) {
        if (mg->brushSound == nullptr) {
            mg->brushSound = PlaySound(mg, ART_SOUND_BRUSH);
        }
        if (mg->deck->drawing != 0) {
            if (mg->lineSound == nullptr) {
                mg->lineSound = PlaySound(mg, ART_SOUND_LINE_DRAW);
            }
        } else {
            StopSound(mg->lineSound);
        }
    } else {
        StopSound(mg->brushSound);
        StopSound(mg->lineSound);
    }
    if (mg->phase == 1) {
        switch (mg->player->stepResult) {
        case 2: {
            void* mark = mg->textures != nullptr ? mg->textures->markParticle : nullptr;
            float cursor[2];
            ArtParticles_AddSparkle(&mg->particles, ArtDeck_GetCursor(mg->deck, cursor), mark);
            break;
        }
        case 3:
            PlaySound(mg, ART_SOUND_LINE_SUCCESS);
            break;
        case 4:
            MGClassArt_KillPlayer(mg, mg->player);
            return;
        }
        ArtPlayer* player = mg->player;
        if (mg->frozen != 0 && (player->poweredUp == 0 || player->powerUp != 2)) {
            MGClassArt_SetStill(mg, 0, 0);
        }
        MGClassArt_UpdateSpawns(mg, ms, mg->frozen);
        if (mg->powerUpSound != nullptr && mg->player->poweredUp == 0) {
            mg->powerUpSound[0x5C] = 1;
            mg->powerUpSound = nullptr;
        }
    }

    for (uint32_t i = 0; i < mg->objects.count; ++i) {
        ArtObject* object = mg->objects.data[i];
        ArtPlayer* player;
        switch (vc::Call<int>(object, 4, ticks)) {
        case 1:   // gone
            MGClassArt_Uncount(mg, object);
            Free(object);
            ArtArray_Remove(&mg->objects, i);
            --i;
            break;
        case 2:   // hit the player: a power-up collected; else the player dies (an enemy is
                  // destroyed instead while reversed, the debris excepted)
            if (object->kind >= 9 && object->kind <= 12) {
                object->stateTime = 0xFF;
                object->state = 3;
                MGClassArt_Collect(mg, object);
                break;
            }
            player = mg->player;
            if (player->age < 60) {
                break;
            }
            if (object->kind == 7 || player->poweredUp == 0 || player->powerUp != 1) {
                MGClassArt_KillPlayer(mg, object);
            } else {
                MGClassArt_DestroyObject(mg, object);
            }
            break;
        case 3:   // hit the line being drawn
            if (mg->player->age >= 60) {
                MGClassArt_KillPlayer(mg, object);
            }
            break;
        case 4:   // caught in a filled area: a power-up collected, an enemy destroyed
            if (object->kind >= 9 && object->kind <= 12) {
                object->stateTime = 0xFF;
                object->state = 3;
                MGClassArt_Collect(mg, object);
            } else {
                MGClassArt_DestroyObject(mg, object);
            }
            break;
        case 5: {   // an exploder went off: its debris flies out evenly round
            PlaySound(mg, ART_SOUND_HATRICK_DESTROYED);
            const float* pos = &object->x;
            void* flash = Minigame_GetTexture(mg, 4);
            void* glow = Minigame_GetTexture(mg, 8);
            ArtParticles_AddExplosion(&mg->particles, pos, 0.0625f, 0.25f, glow, flash);
            for (uint32_t j = 0; j < mg->config.debrisCount; ++j) {
                float a = U2F(j) / U2F(mg->config.debrisCount) * 6.2831855f;
                float dir[2];
                dir[0] = Cos(a);
                dir[1] = Sin(a);
                void* memory = Malloc(sizeof(ArtDebris));
                ArtObject* debris = nullptr;
                if (memory != nullptr) {
                    debris = ArtDebris_Construct(memory, Minigame_GetTexture(mg, 3), mg->deck, mg->player, pos, dir,
                                                 mg->config.debrisSpeed);
                }
                PushBack(mg->objects, debris);
            }
            Pad_Rumble(0, 100, 0xFF, 0);
            break;
        }
        }
    }

    if (mg->phase == 1 && mg->config.completionThreshold <= U2F(FilledPercent(mg->deck))) {
        StopSound(mg->music);
        StopSound(mg->sound608);
        MGClassArt_StopSounds(mg, 1);
        mg->failureCode = 0;
        mg->phase = 4;
        PlaySound(mg, ART_SOUND_SUCCESS);
    }
}

// 0x6E3020 (virtual +4): the exit question while it shows; else (no start response
// pending) the start button asks it, or a frame of the game: ticks are 16 ms (at least
// one a frame)
void __fastcall MGClassArtUpdate(MGClassArt* mg, re::Edx) {
    if (mg->setUp == 0) {
        return;
    }
    if (mg->active == 0) {
        MGClassArt_Exit(mg, 0, 3);
        return;
    }
    if (mg->popupShowing != 0) {
        if (mg->popupKind == 1) {
            if (Popup_Result(&mg->popupKind) == 2) {
                mg->active = 0;
                artMinigameRunning = 0;
            }
            mg->lastTime = artTimeInMs;
            return;
        }
        if (mg->popupShowing != 0 && mg->popupKind == 2) {
            Popup_Result(&mg->popupKind);
            mg->lastTime = artTimeInMs;
            return;
        }
    }
    if (artUserStartResponse != 0) {
        return;
    }
    if (Pad_JustDown(8, 0) != 0 && mg->phase != 2 && mg->phase != 4) {
        Popup_Show(&mg->popupKind, 1, GameString(0x00944690), GameString(0x00941D14), 7, 8);   // "EXIT_QUESTION"
        mg->lastTime = artTimeInMs;
        return;
    }
    Fn_45B960(0);
    uint32_t now = artTimeInMs;
    uint32_t elapsed = now - mg->lastTime;
    uint32_t ticks = (elapsed >> 4) + 1;
    MGClassArt_UpdateGame(mg, elapsed, ticks);
    MGClassArt_UpdateIcons(mg, ticks);
    ArtParticles_Update(&mg->particles, ticks);
    mg->lastTime = now;
}

// ---- script natives ------------------------------------------------------------------------

GAME_FN(void, __thiscall, MGClassArt_AddPreInstance, 0x006D9FB0, void* self, const ArtPreInstance* pre);

// 0x6E33D0: MGCA_PreInstanceEnemy(kind, ...): line crawlers (0, 1) take a vertex (two
// ints) and a third int, the floaters and bouncers (3, 4) a point and an int, the
// exploder (6) a point. (The fields a kind doesn't set are the stack's leftovers.)
int __cdecl Native_MGCA_PreInstanceEnemy(lua_State* L) {
    ArtPreInstance pre;
    int kind = Script::GetInt(L, 0);
    pre.enemy = 1;
    pre.kind = kind;
    switch (kind) {
    case 0:
    case 1: {
        int col = Script::GetInt(L, 1);
        int row = Script::GetInt(L, 2);
        std::memcpy(&pre.x, &col, 4);
        std::memcpy(&pre.y, &row, 4);
        pre.extra = Script::GetInt(L, 3);
        break;
    }
    case 3:
    case 4:
        pre.x = Script::GetFloat(L, 1);
        pre.y = Script::GetFloat(L, 2);
        pre.extra = Script::GetInt(L, 3);
        break;
    case 6:
        pre.x = Script::GetFloat(L, 1);
        pre.y = Script::GetFloat(L, 2);
        break;
    }
    MGClassArt_AddPreInstance(artCurrentMinigame, &pre);
    return 0;
}

// 0x6E3740: MGCA_PreInstancePowerUp(kind, ...): the line ones (0, 1) take a vertex, the
// floaters (2, 3) a point; both then an int
int __cdecl Native_MGCA_PreInstancePowerUp(lua_State* L) {
    ArtPreInstance pre;
    int kind = Script::GetInt(L, 0);
    pre.enemy = 0;
    pre.kind = kind;
    switch (kind) {
    case 0:
    case 1: {
        int col = Script::GetInt(L, 1);
        int row = Script::GetInt(L, 2);
        std::memcpy(&pre.x, &col, 4);
        std::memcpy(&pre.y, &row, 4);
        pre.extra = Script::GetInt(L, 3);
        break;
    }
    case 2:
    case 3:
        pre.x = Script::GetFloat(L, 1);
        pre.y = Script::GetFloat(L, 2);
        pre.extra = Script::GetInt(L, 3);
        break;
    }
    MGClassArt_AddPreInstance(artCurrentMinigame, &pre);
    return 0;
}

} // namespace

// Untested: the game's code runs until each is checked.
#define ARTARC42_REVERSED(name, address, fn) \
    REVERSED(name, address, RE_BODY(fn), nullptr, nullptr, re::Mode::Original)
#define ARTARC42_MEMBER(name, address, member) \
    REVERSED(name, address, re::MemberAddress(member), nullptr, nullptr, re::Mode::Original)

ARTARC42_MEMBER("MGClassArt::GameObject::DrawSprite_6DD740", 0x006DD740, &ArtObject::DrawSprite);
ARTARC42_MEMBER("MGClassArt::GameObject::SweepHits_6DD8A0", 0x006DD8A0, &ArtObject::SweepHits);
ARTARC42_MEMBER("MGClassArt::Player::Reset_6DDA80", 0x006DDA80, &ArtPlayer::Reset);
ARTARC42_MEMBER("MGClassArt::Player::StartPowerUp_6DDB00", 0x006DDB00, &ArtPlayer::StartPowerUp);
ARTARC42_MEMBER("MGClassArt::Player::ReadPad_6DDB90", 0x006DDB90, &ArtPlayer::ReadPad);
ARTARC42_MEMBER("MGClassArt::FloaterObject::Move_6DDD50", 0x006DDD50, &ArtFloater::Move);
ARTARC42_MEMBER("MGClassArt::LineObject::LineObject_6DDF70", 0x006DDF70, &ArtLineObject::Construct);
ARTARC42_MEMBER("MGClassArt::LineObject::Move_6DDFF0", 0x006DDFF0, &ArtLineObject::Move);
ARTARC42_MEMBER("MGClassArt::Exploder::Update_6DE2F0", 0x006DE2F0, &ArtExploder::Update);
ARTARC42_MEMBER("MGClassArt::ExploderDebris::Update_6DE560", 0x006DE560, &ArtDebris::Update);
ARTARC42_MEMBER("MGClassArt::PowerUpIcon::Draw_6DE650", 0x006DE650, &ArtPowerUpIcon::Draw);
ARTARC42_MEMBER("MGClassArt::FlashParticle::Draw_6DEB80", 0x006DEB80, &ArtFlash::Draw);
ARTARC42_MEMBER("MGClassArt::ParticleRing::ParticleRing_6DED70", 0x006DED70, &ArtRing::Construct);
ARTARC42_MEMBER("MGClassArt::ParticleRing::Delete_6DEDF0", 0x006DEDF0, &ArtRing::Destroy);
ARTARC42_MEMBER("MGClassArt::ParticleRing::Emit_6DEE30", 0x006DEE30, &ArtRing::Emit);
ARTARC42_MEMBER("MGClassArt::ParticleRing::Draw_6DEFC0", 0x006DEFC0, &ArtRing::Draw);
ARTARC42_MEMBER("MGClassArt::Particles::Update_6DF260", 0x006DF260, &ArtParticles::Update);
ARTARC42_MEMBER("MGClassArt::Particles::AddRing_6DF2F0", 0x006DF2F0, &ArtParticles::AddRing);
ARTARC42_REVERSED("MGClassArt::Render_6DF370", 0x006DF370, &MGClassArtRender);
ARTARC42_REVERSED("MGClassArt::ResetLife_6DFFC0", 0x006DFFC0, &MGClassArtResetLife);
ARTARC42_MEMBER("MGClassArt::GameObject::Draw_6E0060", 0x006E0060, &ArtObject::Draw);
ARTARC42_MEMBER("MGClassArt::Player::Player_6E0170", 0x006E0170, &ArtPlayer::Construct);
ARTARC42_MEMBER("MGClassArt::Player::Update_6E0200", 0x006E0200, &ArtPlayer::Update);
ARTARC42_MEMBER("MGClassArt::Player::Draw_6E0770", 0x006E0770, &ArtPlayer::DrawPlayer);
ARTARC42_MEMBER("MGClassArt::FloaterObject::Update_6E07F0", 0x006E07F0, &ArtFloater::Update);
ARTARC42_MEMBER("MGClassArt::FloaterObject::SetDirection_6E08A0", 0x006E08A0, &ArtFloater::SetDirection);
ARTARC42_MEMBER("MGClassArt::LineObject::Update_6E08F0", 0x006E08F0, &ArtLineObject::Update);
ARTARC42_MEMBER("MGClassArt::Exploder::Draw_6E09A0", 0x006E09A0, &ArtExploder::DrawExploder);
ARTARC42_MEMBER("MGClassArt::ExploderDebris::ExploderDebris_6E0A10", 0x006E0A10, &ArtDebris::Construct);
ARTARC42_MEMBER("MGClassArt::Particles::AddExplosion_6E0AA0", 0x006E0AA0, &ArtParticles::AddExplosion);
ARTARC42_MEMBER("MGClassArt::Particles::AddBigExplosion_6E0CA0", 0x006E0CA0, &ArtParticles::AddBigExplosion);
ARTARC42_MEMBER("MGClassArt::Particles::AddSparkle_6E0EA0", 0x006E0EA0, &ArtParticles::AddSparkle);
ARTARC42_REVERSED("MGClassArt::MGClassArt_6E0F80", 0x006E0F80, &MGClassArtConstruct);
ARTARC42_REVERSED("MGClassArt::Cleanup_6E1010", 0x006E1010, &MGClassArtCleanup);
ARTARC42_REVERSED("MGClassArt::KillPlayer_6E1150", 0x006E1150, &MGClassArtKillPlayer);
ARTARC42_REVERSED("MGClassArt::DestroyObject_6E12B0", 0x006E12B0, &MGClassArtDestroyObject);
ARTARC42_REVERSED("MGClassArt::ClearRadius_6E1380", 0x006E1380, &MGClassArtClearRadius);
ARTARC42_MEMBER("MGClassArt::FloaterObject::FloaterObject_6E14A0", 0x006E14A0, &ArtFloater::Construct);
ARTARC42_MEMBER("MGClassArt::FloaterObject::Place_6E1510", 0x006E1510, &ArtFloater::Place);
ARTARC42_REVERSED("MGClassArt::~MGClassArt_6E1530", 0x006E1530, &MGClassArtDestruct);
ARTARC42_REVERSED("MGClassArt::End_6E15C0", 0x006E15C0, &MGClassArtEnd);
ARTARC42_REVERSED("MGClassArt::Setup_6E15F0", 0x006E15F0, &MGClassArtSetup);
ARTARC42_REVERSED("MGClassArt::Collect_6E18C0", 0x006E18C0, &MGClassArtCollect);
ARTARC42_REVERSED("MGClassArt::CreateEnemy_6E1B70", 0x006E1B70, &MGClassArtCreateEnemy);
ARTARC42_REVERSED("MGClassArt::CreatePowerUp_6E1D20", 0x006E1D20, &MGClassArtCreatePowerUp);
ARTARC42_REVERSED("MGClassArt::Delete_6E1F30", 0x006E1F30, &MGClassArtDelete);
ARTARC42_REVERSED("MGClassArt::Start_6E1F50", 0x006E1F50, &MGClassArtStart);
ARTARC42_REVERSED("MGClassArt::UpdateSpawns_6E2330", 0x006E2330, &MGClassArtUpdateSpawns);
ARTARC42_REVERSED("MGClassArt::UpdateGame_6E2750", 0x006E2750, &MGClassArtUpdateGame);
ARTARC42_REVERSED("MGClassArt::Update_6E3020", 0x006E3020, &MGClassArtUpdate);
ARTARC42_REVERSED("Native_MGCA_PreInstanceEnemy", 0x006E33D0, &Native_MGCA_PreInstanceEnemy);
ARTARC42_REVERSED("Native_MGCA_PreInstancePowerUp", 0x006E3740, &Native_MGCA_PreInstancePowerUp);
