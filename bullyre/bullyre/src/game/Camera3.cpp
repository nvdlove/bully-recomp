// CCamera's field of view as it stands: the override while one is set.

#include "game/Camera.h"
#include "reversed.h"

namespace {

// 0x43A570 (thiscall): +0x168 while +0x16C, otherwise +0x9C
float __fastcall Camera_GetFovOrOverride(CCamera* self, re::Edx) {
    if (self->fovOverridden == 0) {
        return self->fov;
    }
    return self->fovOverride;
}

} // namespace

REVERSED("CCamera::GetFovOrOverride", 0x0043A570, RE_BODY(&Camera_GetFovOrOverride), nullptr, nullptr,
         re::Mode::Original);
