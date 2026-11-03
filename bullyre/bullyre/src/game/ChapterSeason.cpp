// The chapter and its season (the object at CTxdStore::ms_currentChapter): a new
// chapter's season swaps the ped models for the winter ones (season 2) or back.

#include <cstddef>
#include <cstdint>

#include "reversed.h"

namespace {

struct CChapterSeason {
    uint8_t chapter;       // +0
    uint8_t pad1[3];
    int32_t winterPeds;    // +4 1: the winter ped models
    uint8_t loading;       // +8 (a save being loaded: nothing swapped)
};
static_assert(offsetof(CChapterSeason, loading) == 8);

GAME_ARRAY(int32_t, cs_chapterSeason, 0x00A147EC);   // [chapter]
GAME_VAR(int32_t, cs_firstPedModel, 0x00A136C8);
GAME_VAR(int32_t, cs_lastPedModel, 0x00A136CC);
GAME_ARRAY(uint8_t*, cs_modelInfoPtrs, 0x00C67738);
inline void* const cs_pedMgr = reinterpret_cast<void*>(0x00C2B300);

GAME_FN(void, __thiscall, CS_PedMgr_48B430, 0x0048B430, void* mgr);
GAME_FN(void, __cdecl, CS_RemoveModel, 0x0052F640, int id);
GAME_FN(void, __cdecl, CS_Streaming_5331B0, 0x005331B0);
GAME_FN(void, __cdecl, CS_Streaming_530A70, 0x00530A70, int);
GAME_FN(void, __cdecl, CS_PedModels_SetWinter, 0x0051AE60, int winter);
GAME_FN(void, __cdecl, CS_Fn_4EA630, 0x004EA630);
GAME_FN(void, __cdecl, CS_Fn_5348D0, 0x005348D0);
// ours, called where modes apply
GAME_FN(void, __thiscall, CS_SetWinterPeds, 0x00454540, CChapterSeason* self, int winter);

} // namespace

namespace ChapterSeason {

// 0x454540: every ped model removed (and its vf3) when the ped set changes
void __fastcall SetWinterPeds(CChapterSeason* self, re::Edx, int winter) {
    if (self->winterPeds == winter) {
        return;
    }
    self->winterPeds = winter;
    CS_PedMgr_48B430(cs_pedMgr);
    for (int id = cs_firstPedModel + 1; id <= cs_lastPedModel; id++) {
        uint8_t* mi = cs_modelInfoPtrs[id];
        if (mi) {
            CS_RemoveModel(id);
            re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(mi);
            re::PtrCast<void(__thiscall*)(uint8_t*)>(vtable[3])(mi);
        }
    }
    CS_Streaming_5331B0();
    CS_Streaming_530A70(0);
    CS_PedModels_SetWinter(winter);
}

// 0x4545C0: a new chapter (0x4EA630 on a change, unless loading); on a new season the
// peds swapped (unless loading) and 0x5348D0
void __fastcall SetChapter(CChapterSeason* self, re::Edx, int chapterArg) {
    uint8_t chapter = uint8_t(chapterArg);
    int32_t oldSeason = cs_chapterSeason[self->chapter];
    if (!self->loading && self->chapter != chapter) {
        CS_Fn_4EA630();
    }
    self->chapter = chapter;
    int32_t season = cs_chapterSeason[chapter];
    if (oldSeason == season) {
        return;
    }
    if (!self->loading) {
        if (season == 2) {
            CS_SetWinterPeds(self, 1);
            CS_Fn_5348D0();
            return;
        }
        CS_SetWinterPeds(self, 0);
    }
    CS_Fn_5348D0();
}

} // namespace ChapterSeason

// ---- registration ---------------------------------------------------------------------

#define C04C_CS_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

C04C_CS_REVERSED("CChapterSeason::SetWinterPeds", 0x00454540, &ChapterSeason::SetWinterPeds);
C04C_CS_REVERSED("CChapterSeason::SetChapter", 0x004545C0, &ChapterSeason::SetChapter);
