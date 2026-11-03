// The action tree's prop and weapon-attachment tracks' virtuals (0x65FB40..0x661470,
// 0x663120..0x664350): picking props up and attaching them (and weapons) to the ped's
// hand or other weapons, scattering attachments, sockets and desired props, spawning,
// the first-person, camera limit and zoom tracks, weapon info, toss targets, ammo
// attachments. A track's fields are its own past +0x18 (ActionTracksCommon.h); the owner's
// +8 is the ped: +0x1D0 its weapon, +0x1D4 its weapon container, +0x544 the prop it's
// after, +0x1574/+0x1578 the model (and extra) it picked up.

#include <cmath>
#include <cstdint>

#include "game/ActionTracksCommon.h"
#include "game/TrackMath.h"

#include "game/Crt.h"
#include "reversed.h"
#include "runflags.h"
using namespace actc;

namespace {

GAME_VAR(int32_t, weaponModelFirst, 0x00A136B0);
GAME_VAR(int32_t, weaponModelLast, 0x00A136B4);
GAME_VAR(int32_t, ammoModel_BF432C, 0x00BF432C);
inline void* const audio_CF3DA0 = reinterpret_cast<void*>(0x00CF3DA0);
GAME_VAR(uint32_t, noModelHash_1B65558, 0x01B65558);
GAME_VAR(void*, modelNotify_C674F0, 0x00C674F0);
GAME_ARRAY(float, zeroVector3, 0x00BCC564);
inline void* const animManager_20C4910 = reinterpret_cast<void*>(0x020C4910);
inline void* const theCamera = reinterpret_cast<void*>(0x00C3CC68);
GAME_VAR(void*, cameraController_C3CED0, 0x00C3CED0);
GAME_VAR(int32_t, gameMode, 0x00C3CCF8);
GAME_VAR(uint8_t, vehicleLimitOn_C3CE8C, 0x00C3CE8C);
GAME_ARRAY(uint32_t, vehicleLimitForward_C3CE70, 0x00C3CE70);
GAME_VAR(float, vehicleLimit_C3CE7C, 0x00C3CE7C);
GAME_VAR(float, vehicleLimit_C3CE80, 0x00C3CE80);
GAME_VAR(float, vehicleLimit_C3CE84, 0x00C3CE84);
GAME_VAR(float, vehicleLimit_C3CE88, 0x00C3CE88);
GAME_VAR(int32_t, firstPersonMode_C3CD70, 0x00C3CD70);
GAME_VAR(float, firstPerson_C3CD74, 0x00C3CD74);
GAME_VAR(float, firstPerson_C3CD78, 0x00C3CD78);
GAME_VAR(uint8_t*, cameraButes_20C88B8, 0x020C88B8);
GAME_VAR(uint8_t, noFirstPerson_20C32E9, 0x020C32E9);
GAME_VAR(uint8_t, noFirstPerson_20C32EA, 0x020C32EA);
GAME_VAR(void*, rwCamera_C1AE7C, 0x00C1AE7C);
GAME_VAR(void*, niCamera, 0x00C3CCF4);
GAME_VAR(uint8_t*, particleRenderer_B6F1EC, 0x00B6F1EC);
GAME_VAR(uint8_t*, effectManager_B6F1D4, 0x00B6F1D4);
inline void* const renderer_D140B8 = reinterpret_cast<void*>(0x00D140B8);
GAME_VAR(uint8_t*, zoomState_C674F0, 0x00C674F0);
GAME_VAR(uint8_t*, zoomState_C674E0, 0x00C674E0);
GAME_VAR(int32_t, zoomLevel_1B65C0C, 0x01B65C0C);
GAME_VAR(float, zoom_1B65C10, 0x01B65C10);
GAME_VAR(uint8_t, zoomFlag_C3CEFE, 0x00C3CEFE);
GAME_VAR(float, zoomLimitA_C3CDD0, 0x00C3CDD0);
GAME_VAR(int32_t, zoomUseA_C3CDD4, 0x00C3CDD4);
GAME_VAR(float, zoomLimitC_C3CD04, 0x00C3CD04);
GAME_VAR(int32_t, remoteWeapon_BF44E0, 0x00BF44E0);
GAME_VAR(uint8_t*, cameraLimitEntity_C3CE90, 0x00C3CE90);
GAME_VAR(uint8_t*, firstPersonPed_C3CE14, 0x00C3CE14);
GAME_VAR(uint8_t*, ignoredEntity_C1AE70, 0x00C1AE70);
GAME_VAR(float, firstPersonTurn_C3CD08, 0x00C3CD08);
GAME_VAR(float, zoomWidth_C3CDD8, 0x00C3CDD8);
inline void* const gameState_A45500 = reinterpret_cast<void*>(0x00A45500);
GAME_VAR(float, weaponAim_C3CD94, 0x00C3CD94);
GAME_VAR(float, weaponAim_C3CD98, 0x00C3CD98);
GAME_VAR(float, weaponAim_C3CD9C, 0x00C3CD9C);
GAME_VAR(float, weaponAim_C3CDA0, 0x00C3CDA0);
GAME_VAR(float, weaponAim_C3CDA4, 0x00C3CDA4);
GAME_VAR(float, negativeZero_93C970, 0x0093C970);
GAME_VAR(uint32_t, cameraOffset_C2C0AC, 0x00C2C0AC);
GAME_VAR(float, cameraOffset_C2C0A8, 0x00C2C0A8);
GAME_ARRAY(uint32_t, cameraOffset_C2C0C0, 0x00C2C0C0);
GAME_VAR(uint32_t, defaultOffset_911B90, 0x00911B90);
//@@TOP@@

GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, uint32_t size);
GAME_FN(int, __cdecl, GetModelIndexByHash, 0x0051AE20, uint32_t hash, int first, int last);
GAME_FN(uint8_t*, __cdecl, GetWeaponModelInfo, 0x0051B450, int model);
GAME_FN(int32_t, __cdecl, Entity_FindBone, 0x00655620, uint8_t* entity, uint32_t hash);
GAME_FN(void, __cdecl, Object_Remove_4D4AB0, 0x004D4AB0, uint8_t* object);
GAME_FN(void, __thiscall, Inventory_AddAmmo, 0x0045D030, uint8_t* inventory, int32_t weapon, int32_t ammo, int notify,
        int extra);
GAME_FN(uint8_t*, __cdecl, Object_GetPropAnim, 0x00443870, uint8_t* object);
GAME_FN(uint8_t*, __thiscall, TaskList_Current, 0x00470340, uint8_t* list);
GAME_FN(void, __thiscall, TaskList_Pop, 0x004702C0, uint8_t* list);
GAME_FN(void, __thiscall, PropAnim_PickUp, 0x00444C10, uint8_t* anim, uint8_t* ped, int);
GAME_FN(void, __thiscall, Weapon_Reset_4D9410, 0x004D9410, uint8_t* weapon);
GAME_FN(void, __thiscall, Weapon_SetModel, 0x004DA840, uint8_t* weapon, int model, uint32_t extra);
GAME_FN(void, __thiscall, Weapon_Attach, 0x004D9B60, uint8_t* weapon, void* holderAnim, int32_t holderBone,
        int32_t ownBone);
GAME_FN(void, __thiscall, Weapon_Fn4D9460, 0x004D9460, uint8_t* weapon);
GAME_FN(int32_t, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(int32_t, __thiscall, Sounds_Play, 0x0059B8A0, void* sounds, int32_t sound, const float* pos, uint32_t, uintptr_t, int,
        int, int, int, uintptr_t, uintptr_t, int, int, float, int);
GAME_FN(uint8_t*, __cdecl, ActionTree_FindNode, 0x005F5B10, uintptr_t name, uintptr_t);
GAME_FN(void, __thiscall, Owner_SetNode, 0x005F3830, uint8_t* owner, uint8_t* node);
GAME_FN(int32_t, __thiscall, Container_FindModel, 0x004DAAA0, uint8_t* container, int model);
GAME_FN(int32_t, __thiscall, Container_FindBone, 0x004DAA50, uint8_t* container, const uint32_t* bone);
GAME_FN(uint8_t*, __thiscall, Container_Get, 0x004DAB70, uint8_t* container, int32_t slot);
GAME_FN(void, __thiscall, Container_Add, 0x004DAAE0, uint8_t* container, uint8_t* weapon);
GAME_FN(uint8_t*, __thiscall, Weapon_Construct, 0x004DA760, void* memory);
GAME_FN(uint8_t*, __thiscall, Weapon_GetAttachment, 0x004DA600, uint8_t* weapon, int index);
GAME_FN(void, __thiscall, Weapon_RemoveAttachment, 0x004DA090, uint8_t* weapon, int index);
GAME_FN(void, __thiscall, AnimManager_GetBonePosition, 0x006C1190, void* manager, void* anim, int32_t bone,
        float* out, uintptr_t);
GAME_FN(float*, __thiscall, Entity_GetMatrix_46DD20, 0x0046DD20, uint8_t* entity);
// (0x0045D500 stores 16 bytes through `out`: called through re::CallVec16)
static inline float* Matrix_TransformPoint_45D500(float* out, const float* matrix, const float* v) { return re::CallVec16<0x0045D500, float*>(out, matrix, v); }
GAME_FN(V3*, __thiscall, Placeable_GetForward, 0x004154A0, uint8_t* placeable, V3* out);
GAME_FN(V3*, __cdecl, VectorAdd, 0x004129A0, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __cdecl, VectorCross, 0x00414730, V3* out, const V3* a, const V3* b);
GAME_FN(V3*, __thiscall, ScatterOffsetFn, 0x00660470, uint8_t* self, V3* out, float x, float y, float z, float* base);
GAME_FN(void, __thiscall, Weapon_Detach_4D9510, 0x004D9510, uint8_t* weapon);
GAME_FN(void, __thiscall, Container_Clear_4DA9E0, 0x004DA9E0, uint8_t* container);
GAME_FN(void, __thiscall, Container_Remove, 0x004DAA20, uint8_t* container, int32_t slot);
GAME_FN(uint8_t*, __thiscall, TargetingSystem_GetTarget, 0x004D5AD0, void* targeting);
GAME_FN(uint8_t*, __thiscall, TargetingSystem_GetLookTarget, 0x004D7780, void* targeting);
GAME_FN(int32_t, __cdecl, GetModelIndexFromHash_51AF00, 0x0051AF00, uint32_t hash);
GAME_FN(void, __thiscall, ModelNotify_55C840, 0x0055C840, void* self, int32_t model);
GAME_FN(uint8_t*, __thiscall, Skeleton_FindBone, 0x00465580, void* skeleton, uint32_t hash, int32_t* bone);
GAME_FN(void, __thiscall, Name_Copy, 0x005FA880, void* self, const void* from);
GAME_FN(void, __thiscall, Path_Build, 0x005FA8C0, void* self, int count, const void* names);
GAME_FN(uint32_t, __cdecl, Butes_Find, 0x0072AEA0, const void* path, const void* root);
GAME_FN(int32_t, __cdecl, Pickups_Generate_443FE0, 0x00443FE0, float x, float y, float z, int32_t model, int, uintptr_t,
        uintptr_t, int);
GAME_FN(int32_t, __cdecl, Pickups_Index_4438B0, 0x004438B0, int32_t id);
GAME_FN(void, __thiscall, Camera_SetShotRequest_4F1890, 0x004F1890, void* camera, const void* request);
GAME_FN(void, __thiscall, Camera_SetShot, 0x004F2C20, void* camera, int shot, int time, int flag, int, int);
GAME_FN(void, __thiscall, Weapon_Fn4D9650, 0x004D9650, uint8_t* weapon, int on);
GAME_FN(void, __cdecl, FirstPerson_Fn4DB5F0, 0x004DB5F0);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(void, __stdcall, VectorDestruct, 0x004017C0, void* array, uint32_t size, int count, void* dtor);
GAME_FN(void, __cdecl, RwCamera_EndUpdate, 0x006C86C0, void* camera);
GAME_FN(int, __cdecl, RwCamera_BeginUpdate, 0x006C86A0, void* camera);
GAME_FN(float, __cdecl, Camera_GetNearClip, 0x006C82E0, void* camera);
GAME_FN(float, __cdecl, Camera_GetFarClip, 0x006C82D0, void* camera);
GAME_FN(void, __cdecl, Camera_SetNearClip, 0x006C8820, void* camera, float plane);
GAME_FN(void, __cdecl, Camera_SetFarClip, 0x006C8780, void* camera, float plane);
GAME_FN(void, __cdecl, Camera_Fn_6C8300, 0x006C8300, void* camera, int);
GAME_FN(uint8_t*, __thiscall, RGBA_Construct, 0x00510DE0, uint8_t* rgba, int r, int g, int b, int a);
GAME_FN(void, __cdecl, Camera_Clear_6CA7D0, 0x006CA7D0, void* camera, const uint8_t* colour, int);
GAME_FN(void, __thiscall, Weapon_Render_4D9C10, 0x004D9C10, uint8_t* weapon);
GAME_FN(void, __thiscall, Renderer_ResetState, 0x005EE560, void* renderer);
GAME_FN(void, __thiscall, ParticleRenderer_Fn578150, 0x00578150, void* self, void* camera);
GAME_FN(void, __thiscall, EffectManager_RenderParticles, 0x00668050, uint8_t* self);
GAME_FN(int, __cdecl, GetWeaponIndex, 0x0051AEA0, const char* name);
GAME_FN(int32_t, __thiscall, Inventory_FindItem, 0x0045CF30, uint8_t* inventory, int32_t weapon);
GAME_FN(uint8_t*, __thiscall, Inventory_GetItemInSlot, 0x0045CC90, uint8_t* inventory, int32_t slot);
GAME_FN(void, __thiscall, Targeting_ClearPartnerTarget, 0x004D7800, void* targeting);
GAME_FN(void, __thiscall, Targeting_UpdatePartnerTarget, 0x004D7F60, void* targeting);
GAME_FN(void, __thiscall, Camera_SetFov, 0x004F33A0, void* camera, float fov, int time, int mode, float speed, int);
GAME_FN(void, __thiscall, CameraZoomControlTrack_SetLevel, 0x0065E180, uint8_t* self, int32_t level);
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
GAME_FN(void, __thiscall, Weapon_AddAttachment, 0x004DA010, uint8_t* weapon, uint8_t* attachment, int index,
        int32_t holderBone, int32_t ownBone);
GAME_FN(uint8_t*, __thiscall, Weapon_TakeAttachment, 0x004DA090, uint8_t* weapon, int index);
GAME_FN(int32_t, __thiscall, Inventory_GetAmmo, 0x0045CF40, uint8_t* inventory, int32_t weapon);
GAME_FN(int32_t, __thiscall, Inventory_SetAmmo, 0x0045D240, uint8_t* inventory, int32_t weapon, int32_t ammo,
        int notify);
GAME_FN(int32_t, __cdecl, Projectiles_Find_4D4140, 0x004D4140, float x, float y, float z, float, float, int32_t model,
        void* owner, int);
GAME_FN(void, __cdecl, Projectiles_Remove_4D3F10, 0x004D3F10, int32_t index, int);
GAME_FN(void, __thiscall, Targeting_SetTarget, 0x004D79E0, void* targeting, uint8_t* target);
GAME_FN(uint8_t*, __thiscall, Reference_Get0C, 0x005F4F90, uint8_t* ref, int32_t key);
GAME_FN(void, __thiscall, Controller_Fn5F4660, 0x005F4660, void* controller, uint8_t* node);
GAME_FN(int32_t, __cdecl, Projectiles_GetModel_4D3650, 0x004D3650, int32_t index);
GAME_FN(uint8_t*, __cdecl, Projectiles_Get_4D34B0, 0x004D34B0, int32_t index);
GAME_FN(void, __thiscall, Fn478760, 0x00478760, uint8_t* self, void* p);
GAME_FN(void, __thiscall, WeaponInfo_Load_7282A0, 0x007282A0, uint8_t* info);
GAME_FN(void, __thiscall, ScatterTrack_Fn65DA20, 0x0065DA20, uint8_t* self, uint8_t* weapon);
GAME_FN(float*, __thiscall, ScatterTrack_BonePositionFn, 0x006603F0, uint8_t* self, float* out, void* anim,
        int32_t bone);
GAME_FN(V3*, __thiscall, ScatterTrack_PointFn, 0x00660670, uint8_t* self, V3* out, float x, float y, float z);
GAME_FN(V3*, __cdecl, LaunchVelocityNoPed, 0x0065FFA0, V3* out, float fx, float fy, float fz, float tx, float ty,
        float tz, float power, uint8_t* info);
GAME_FN(int32_t, __thiscall, Weapon_Fn4D95E0, 0x004D95E0, uint8_t* weapon);
GAME_FN(void, __thiscall, EffectProxy_Kill, 0x00668BA0, void* proxy);
GAME_FN(uint8_t*, __cdecl, AddProjectile_x64, 0x004D4D90, uint8_t* creator, int32_t weapon, re::Float3, re::Float3, uint8_t* info, float spread, float power, uintptr_t r20, uintptr_t sinceLast, int32_t hold, re::Float3, re::Float3, uintptr_t, int flag);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline uint8_t* AddProjectile(uint8_t* a0, int32_t a1, float a2, float a3, float a4, float a5, float a6, float a7, uint8_t* a8, float a9, float a10, int32_t a11, uintptr_t a12, int32_t a13, uint32_t a14, uint32_t a15, uint32_t a16, uint32_t a17, uint32_t a18, uint32_t a19, int a20, int a21) {
    return AddProjectile_x64(a0, a1, re::Float3{a2, a3, a4}, re::Float3{a5, a6, a7}, a8, a9, a10, a11, a12, a13, re::Float3{re::FloatOf(uint32_t(a14)), re::FloatOf(uint32_t(a15)), re::FloatOf(uint32_t(a16))}, re::Float3{re::FloatOf(uint32_t(a17)), re::FloatOf(uint32_t(a18)), re::FloatOf(uint32_t(a19))}, a20, a21);
}
GAME_FN(void, __cdecl, Projectiles_Remove_Fn, 0x004D3F10, int32_t index, int);
GAME_FN(V3*, __thiscall, Camera_GetForward_41AA70, 0x0041AA70, void* camera, V3* out);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const float* start, const float* end, ColPoint* colPoint,
        re::Ptr32<uint8_t>* entity, int flags, int, int);
GAME_FN(V3*, __thiscall, Targeting_GetTargetPos, 0x004D6000, void* targeting, V3* out, void* entity, int flag);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, ColPoint* colPoint);
GAME_FN(int32_t, __thiscall, State_Get8_856850, 0x00856850, void* self);
GAME_FN(void, __thiscall, Ped_Fn4734C0, 0x004734C0, uint8_t* ped, float value);
GAME_FN(void, __thiscall, Ped_Fn475730, 0x00475730, uint8_t* ped, float value);
GAME_FN(uint8_t, __cdecl, FirstPerson_CanTarget_6639C0, 0x006639C0);
GAME_FN(void, __cdecl, FirstPerson_Fn4DB290, 0x004DB290, uint32_t value);
GAME_FN(void, __cdecl, FirstPerson_Fn4DB660, 0x004DB660, float value, int a, int b, int c, int d);
GAME_FN(void*, __cdecl, GetPlayerPadInput, 0x00738070);
GAME_FN(uint8_t, __cdecl, Pad_Fn738500, 0x00738500, void* pad);
GAME_FN(float, __cdecl, Pad_GetValue_738840, 0x00738840, int control, int pad);
GAME_FN(uint8_t, __cdecl, Pad_JustPressed, 0x00738900, int control, int pad);
GAME_FN(void, __thiscall, SoundHandle_Set_59D110, 0x0059D110, void* slot, uintptr_t handle);
GAME_FN(float, __cdecl, SFX_Fn59E4F0, 0x0059E4F0, float volume, int ms);
GAME_FN(void, __cdecl, SoundHandle_Play_597400, 0x00597400, void* slot, int32_t sound, int, int, int, int, int);
GAME_FN(void, __thiscall, CameraZoomControlTrack_StepFn, 0x00661320, uint8_t* self, float amount);
GAME_FN(void, __thiscall, AmmoAttachmentTrack_ShowFn, 0x00661470, uint8_t* self, int count);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
//@@DECL@@

// the weapon's pick-up sound (named by its info's +0x78), when it has one
inline void PlayAttachSound(uint8_t* info) {
    const char* sound = At<const char*>(info, 0x78);
    if (!sound) {
        return;
    }
    int32_t id = Sounds_GetId(audio_CF3DA0, sound);
    Sounds_Play(audio_CF3DA0, id, nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
}

// 0x65FB40 (thiscall): PropPickupTrack::Start: the prop the ped is after taken: a weapon
// object removed (ammo given for the ammo model), a prop animation's object picked up; the
// model (and extra) kept on the ped for the attach tracks
uint8_t __fastcall PropPickupTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint8_t* object = At<uint8_t*>(ped, 0x544);
    int32_t model = -1;
    uint32_t extra = 0;
    if (object) {
        if (At<uint32_t>(object, 0xC4)) {
            model = At<int16_t>(object, 0x10E);
            Object_Remove_4D4AB0(object);
            if (model == ammoModel_BF432C) {
                Inventory_AddAmmo(At<uint8_t*>(ped, 0x1C4), model, 1, At<int32_t>(ped, 0x1310) == 0xD ? 1 : 0, 0);
            }
        } else if ((object[0x108] & 7) == 4) {
            uint8_t* anim = Object_GetPropAnim(object);
            if (anim) {
                uint8_t* tasks = ped + 0x5F0;
                uint8_t* current = TaskList_Current(tasks);
                bool take = true;
                if (current && At<int32_t>(current, 8) == 0x17) {
                    if (At<uint32_t>(anim, 0x34) != At<uint32_t>(current, 0x10)) {
                        take = false;
                    } else {
                        TaskList_Pop(tasks);
                    }
                }
                if (take) {
                    extra = At<uint32_t>(anim, 0x38);
                    model = At<int32_t>(anim, 0x30);
                    PropAnim_PickUp(anim, At<uint8_t*>(At<uint8_t*>(self, 0x18), 8), 0);
                }
            }
        }
    }
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x18), 8), 0x1574) = model;
    At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x18), 8), 0x1578) = extra;
    return 1;
}

// 0x65FC30 (thiscall): PropAttachTrack::Start: the picked-up model made the ped's weapon,
// attached by the model's own bones (+0x20) or the track's (+0x18 the ped's, +0x1C the
// weapon's); a sound (+0x21); the model's action node started (+0x22)
uint8_t __fastcall PropAttachTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint32_t extra = At<uint32_t>(ped, 0x1578);
    int32_t model = At<int32_t>(ped, 0x1574);
    if (model == -1 || model == 0) {
        return 1;
    }
    Weapon_Reset_4D9410(At<uint8_t*>(ped, 0x1D0));
    Weapon_SetModel(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0), model, extra);
    uint8_t* info = GetWeaponModelInfo(model);
    uint8_t* holder = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
    int32_t holderBone;
    uint32_t ownHash;
    if (self[0x20]) {
        holderBone = Entity_FindBone(holder, At<uint32_t>(info, 0x90));
        ownHash = At<uint32_t>(info, 0x94);
    } else {
        holderBone = Entity_FindBone(holder, At<uint32_t>(self, 0x18));
        ownHash = At<uint32_t>(self, 0x1C);
    }
    int32_t ownBone = Entity_FindBone(At<uint8_t*>(holder, 0x1D0), ownHash);
    uint8_t* ped2 = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
    Weapon_Attach(At<uint8_t*>(ped2, 0x1D0), At<void*>(ped2, 0x114), holderBone, ownBone);
    Weapon_Fn4D9460(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0));
    if (self[0x21] && At<uint32_t>(info, 0x78)) {
        PlayAttachSound(info);
    }
    uint8_t* actions = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x2E0) + 0x300;
    if (self[0x22] && At<uint32_t>(info, 0x1C)) {
        Owner_SetNode(actions, ActionTree_FindNode(At<uint32_t>(info, 0x1C), 0));
        actions[0x3C] = 1;
    }
    return 1;
}

// 0x65FD80 (thiscall): PropAttachExTrack::Start: a model (+0x18) attached as one of the
// ped's weapons (found in its weapon container by model (+0x25) or by bone (+0x1C), else
// a new weapon object added), by the model's bones (+0x24) or the track's (+0x1C, +0x20);
// a sound (+0x26)
uint8_t __fastcall PropAttachExTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x28) = owner;
    int model = GetModelIndexByHash(At<uint32_t>(self, 0x18), weaponModelFirst, weaponModelLast);
    if (model == -1 || model == 0) {
        return 1;
    }
    uint8_t* container = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D4);
    int32_t slot = self[0x25] == 1 ? Container_FindModel(container, model)
                                   : Container_FindBone(container, &At<uint32_t>(self, 0x1C));
    uint8_t* weapon = nullptr;
    if (slot != -1) {
        weapon = Container_Get(container, slot);
    }
    if (!weapon) {
        void* memory = Malloc(0x298);
        weapon = memory ? Weapon_Construct(memory) : nullptr;
        Container_Add(container, weapon);
    }
    Weapon_Reset_4D9410(weapon);
    Weapon_SetModel(weapon, model, 0);
    int32_t ownBone = -1;
    uint8_t* info = GetWeaponModelInfo(model);
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x28), 8);
    int32_t holderBone;
    uint32_t ownHash;
    if (self[0x24]) {
        holderBone = Entity_FindBone(ped, At<uint32_t>(info, 0x90));
        ownHash = At<uint32_t>(info, 0x94);
    } else {
        holderBone = Entity_FindBone(ped, At<uint32_t>(self, 0x1C));
        ownHash = At<uint32_t>(self, 0x20);
    }
    if (ownHash) {
        ownBone = Entity_FindBone(weapon, ownHash);
    }
    Weapon_Attach(weapon, At<void*>(ped, 0x114), holderBone, ownBone);
    Weapon_Fn4D9460(weapon);
    if (self[0x26] && At<uint32_t>(info, 0x78)) {
        PlayAttachSound(info);
    }
    return 1;
}

// 0x65FF00 (thiscall): PropRemoveAttachmentTrack::Start: the weapon's attachment on a bone
// (+0x18; none found: bone 0) taken off
uint8_t __fastcall PropRemoveAttachmentTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0);
    uint8_t* anim = At<uint8_t*>(weapon, 0x114);
    if (!anim || !At<uint32_t>(anim, 0x94)) {
        return 1;
    }
    int32_t bone = Entity_FindBone(weapon, At<uint32_t>(self, 0x18));
    if (bone == -1) {
        bone = 0;
    }
    for (int i = 0; i < 100; i++) {
        uint8_t* attachment =
            Weapon_GetAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1D0), i);
        if (attachment && At<int32_t>(attachment, 0x138) == bone) {
            Weapon_RemoveAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1D0), i);
            Weapon_Reset_4D9410(attachment);
            return 1;
        }
    }
    return 1;
}

// ---- PropScatterAttachmentTrack's helpers: +0x20 sideways, +0x24 forwards scatter, +0x3C
// the owner --------------------------------------------------------------------------------

// 0x6603F0 (thiscall): a bone's position (no bone: a point above the ped)
float* __fastcall ScatterTrack_BonePosition(uint8_t* self, re::Edx, float* out, void* anim, int32_t bone) {
    float position[3];
    if (anim && bone != -1) {
        AnimManager_GetBonePosition(animManager_20C4910, anim, bone, position, 0);
    } else {
        float up[3] = {0.0f, 0.0f, 1.0f};
        uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
        Matrix_TransformPoint_45D500(position, Entity_GetMatrix_46DD20(ped), up);
    }
    out[0] = position[0];
    out[1] = position[1];
    out[2] = position[2];
    return out;
}

// 0x660470 (thiscall): a base point pushed by the scatter: along the flat way from `from`
// to it (the ped's facing when they're the same) by +0x24, and sideways by +0x20 times
// that distance; the result also returned
V3* __fastcall ScatterTrack_Offset(uint8_t* self, re::Edx, V3* out, float x, float y, float z, float* base) {
    V3 from = {x, y, z};
    V3 push = {0.0f, 0.0f, 0.0f};
    if (!(0.0f == At<float>(self, 0x20)) || !(0.0f == At<float>(self, 0x24))) {
        V3 d;
        VectorSub(&d, base, &from.x);
        if (d.x == zeroVector3[0] && d.y == zeroVector3[1] && d.z == zeroVector3[2]) {
            V3 forward;
            d = *Placeable_GetForward(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), &forward);
        }
        d.z = 0.0f;
        float flat = d.y * d.y + d.x * d.x;
        flat = flat + 0.0f * 0.0f;
        flat = std::fabs(flat);
        float distance = crt::Sqrtf(flat);   // sqrt
        V3 up = {0.0f, 0.0f, 1.0f};
        V3 side;
        VectorCross(&side, &d, &up);
        Vector_Normalise(&side);
        float along = At<float>(self, 0x24);
        V3 forwards = {along * d.x, d.y * along, along * d.z};
        float across = At<float>(self, 0x20);
        V3 scaled = {side.x * distance, side.y * distance, distance * side.z};
        V3 sideways = {scaled.x * across, scaled.y * across, across * scaled.z};
        V3 sum;
        const V3* total = VectorAdd(&sum, &sideways, &forwards);
        push.x = total->x + 0.0f;
        push.y = total->y + 0.0f;
        push.z = 0.0f + total->z;
    }
    base[0] = base[0] + push.x;
    base[1] = base[1] + push.y;
    base[2] = push.z + base[2];
    out->x = base[0];
    out->y = base[1];
    out->z = base[2];
    return out;
}

// 0x660670 (thiscall): a point a step ahead of `p` (the ped's facing), scattered from it
V3* __fastcall ScatterTrack_Point(uint8_t* self, re::Edx, V3* out, float x, float y, float z) {
    V3 p = {x, y, z};
    V3 forward;
    const V3* f = Placeable_GetForward(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), &forward);
    VectorAdd(out, &p, f);
    V3 result;
    ScatterOffsetFn(self, &result, p.x, p.y, p.z, &out->x);
    return out;
}

// 0x6606D0 (thiscall): PropSetSocketTrack::Start: the ped's weapon attached again, by the
// model's bones (+0x20) or the track's (+0x18 the ped's, +0x1C the weapon's)
uint8_t __fastcall PropSetSocketTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
    int32_t model = At<int16_t>(weapon, 0x10E);
    if (model == -1 || model == 0) {
        return 1;
    }
    int32_t holderBone;
    int32_t ownBone;
    if (self[0x20]) {
        uint8_t* info = GetWeaponModelInfo(model);
        uint8_t* holder = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
        holderBone = Entity_FindBone(holder, At<uint32_t>(info, 0x90));
        ownBone = Entity_FindBone(At<uint8_t*>(holder, 0x1D0), At<uint32_t>(info, 0x94));
    } else {
        holderBone = Entity_FindBone(ped, At<uint32_t>(self, 0x18));
        ownBone = Entity_FindBone(weapon, At<uint32_t>(self, 0x1C));
    }
    Weapon_Detach_4D9510(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0));
    uint8_t* p = At<uint8_t*>(At<uint8_t*>(self, 0x24), 8);
    Weapon_Attach(At<uint8_t*>(p, 0x1D0), At<void*>(p, 0x114), holderBone, ownBone);
    Weapon_Fn4D9460(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0));
    return 1;
}

// 0x6607A0 (thiscall): PropDetachTrack::Start: the ped's weapon let go of (its model kept
// for the attach tracks; the action node it started stopped (+0x1C))
uint8_t __fastcall PropDetachTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    At<uint32_t>(At<uint8_t*>(owner, 8), 0x1578) = 0;
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x20), 8);
    if (!At<uint32_t>(ped, 0x1368)) {
        At<int32_t>(ped, 0x1574) = -1;
        return 1;
    }
    At<int32_t>(ped, 0x1574) = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
    Weapon_Reset_4D9410(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1D0));
    uint8_t* owners = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x2E0);
    uint8_t* actions = owners + 0x300;
    if (self[0x1C] && actions[0x3C]) {
        Owner_SetNode(actions, At<uint8_t*>(owners, 0x4B0));
        actions[0x3C] = 0;
    }
    return 1;
}

// 0x660840 (thiscall): PropDetachExTrack::Start: the weapon on a bone (+0x18; none: all)
// taken out of the ped's weapon container and deleted
uint8_t __fastcall PropDetachExTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (!At<uint32_t>(ped, 0x1368)) {
        return 1;
    }
    if (!At<uint32_t>(self, 0x18)) {
        Container_Clear_4DA9E0(At<uint8_t*>(ped, 0x1D4));
        return 1;
    }
    int32_t slot = Container_FindBone(At<uint8_t*>(ped, 0x1D4), &At<uint32_t>(self, 0x18));
    if (slot == -1) {
        return 1;
    }
    uint8_t* weapon = Container_Get(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1D4), slot);
    Weapon_Reset_4D9410(weapon);
    Container_Remove(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1D4), slot);
    vc::Call<void>(weapon, 0, 1);
    return 1;
}

// 0x6608D0 (thiscall): PropSetDesiredTrack::Start: the prop the ped wants: its target's
// weapon (+0x1C), else its +0x155C's (+0x1E), else the model +0x18 (unless none); told to
// 0xC674F0 when +0x1D
uint8_t __fastcall PropSetDesiredTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    int32_t model = -1;
    At<uint8_t*>(self, 0x20) = owner;
    bool byHash = true;
    if (self[0x1C] && owner && At<uint8_t*>(owner, 8)) {
        uint8_t* target = TargetingSystem_GetTarget(At<void*>(At<uint8_t*>(owner, 8), 0x1C8));
        if (!target) {
            target = TargetingSystem_GetLookTarget(At<void*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1C8));
        }
        if (target && (target[0x108] & 7) == 3) {
            model = At<int16_t>(At<uint8_t*>(target, 0x1D0), 0x10E);
            byHash = model == -1;
        }
    } else if (self[0x1E] && owner && At<uint8_t*>(owner, 8) && At<uint8_t*>(At<uint8_t*>(owner, 8), 0x155C)) {
        model = At<int16_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x155C), 0x1D0), 0x10E);
        byHash = model == -1;
    }
    if (byHash && At<uint32_t>(self, 0x18) != noModelHash_1B65558) {
        model = GetModelIndexFromHash_51AF00(At<uint32_t>(self, 0x18));
    }
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x2E0), 0x18) = model;
    if (self[0x1D]) {
        ModelNotify_55C840(modelNotify_C674F0, model);
    }
    return 1;
}

// 0x6609A0 (thiscall): PropSetInTransitionTrack::Start: the model +0x18 as the one picked up
uint8_t __fastcall PropSetInTransitionTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    At<int32_t>(ped, 0x1574) = GetModelIndexFromHash_51AF00(At<uint32_t>(self, 0x18));
    return 1;
}

// 0x6609D0 (thiscall): PropGetDesiredTrack::Start: the wanted prop taken as the one picked up
uint8_t __fastcall PropGetDesiredTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    At<int32_t>(ped, 0x1574) = At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18);
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x18), 8), 0x2E0), 0x18) = 0;
    return 1;
}

// 0x660A10 (thiscall): PropSpawnTrack::Start: the picked-up model spawned as a pickup at
// a bone (+0x18) of the ped (else half a metre above it), its Butes found by name (+0x1C)
uint8_t __fastcall PropSpawnTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x20) = owner;
    const float* at = Pos(At<uint8_t*>(owner, 8));
    float position[3] = {at[0], at[1], at[2]};
    position[2] = position[2] + 0.5f;
    int32_t bone = 0;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    uint8_t* anim = At<uint8_t*>(ped, 0x114);
    if (anim && At<void*>(anim, 0x94) && Skeleton_FindBone(At<void*>(anim, 0x94), At<uint32_t>(self, 0x18), &bone) &&
        bone != -1 && bone >= 0) {
        AnimManager_GetBonePosition(animManager_20C4910, At<void*>(ped, 0x114), bone, position, 0);
    }
    uint32_t butes = 0;
    if (At<uint32_t>(self, 0x1C)) {
        uint32_t names[2];
        Name_Copy(&names[0], reinterpret_cast<const void*>(0x01B652EC));
        Name_Copy(&names[1], self + 0x1C);
        uint8_t path[8];
        Path_Build(path, 2, names);
        butes = Butes_Find(path, reinterpret_cast<const void*>(0x01B652E8));
        NameKey_Destruct(&names[1]);
        NameKey_Destruct(&names[0]);
    }
    int32_t model = At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1574);
    if (model == -1 || model == 0) {
        return 1;
    }
    int32_t id = Pickups_Generate_443FE0(position[0], position[1], position[2], model, 1, butes, 0, 0);
    if (id == -1) {
        return 1;
    }
    int32_t index = Pickups_Index_4438B0(id);
    At<uint32_t>(reinterpret_cast<uint8_t*>(0x00C0A100) + index * 0x48, 0x38) =
        At<uint32_t>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x20), 8), 0x1D0), 0x268);
    return 1;
}

// 0x660B90 (thiscall): SetCameraTrack::Start: a camera shot (+0x18, for +0x1C seconds,
// +0x20): through the camera's controller when it's this owner's, else for the player
// only
uint8_t __fastcall SetCameraTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x24) = owner;
    if (re::SeamlessDoorHoldsCamera("SetCameraTrack")) {
        return 1;   // (the remaster's seamless doors: runflags.h)
    }
    if (cameraController_C3CED0 == At<void*>(owner, 0)) {
        uint8_t request[0x14];
        At<uint32_t>(request, 0x10) = 0;
        At<uint32_t>(request, 0xC) = 0;   // (its bytes past the flag are the stack's in the game)
        At<uint32_t>(request, 0) = At<uint32_t>(self, 0x18);
        float ms = At<float>(self, 0x1C) * 1000.0f;
        At<int32_t>(request, 4) = int32_t(ms);   // (_ftol)
        request[8] = self[0x20];
        Camera_SetShotRequest_4F1890(theCamera, request);
        return 1;
    }
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (ped && At<int32_t>(ped, 0x1310) != 0xD) {
        return 0;
    }
    float ms = At<float>(self, 0x1C) * 1000.0f;
    Camera_SetShot(theCamera, At<int32_t>(self, 0x18), int32_t(ms), self[0x20], 0, 0);
    return 1;
}

// 0x660C40 (thiscall): CameraSetLimitTrack::Update: in a vehicle, the camera's limits
// (+0x18, +0x20, +0x1C, +0x24 degrees, as radians) around the vehicle's facing
uint8_t __fastcall CameraSetLimitTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x38), 8);
    if (!ped) {
        return 0;
    }
    uint8_t* vehicle = At<uint8_t*>(ped, 0x16C8);
    if (At<uint32_t>(ped, 0x148C) && vehicle) {
        V3 forward;
        Placeable_GetForward(vehicle, &forward);
        float a = At<float>(self, 0x18) * 3.1415927f;
        a = a / 180.0f;
        float b = At<float>(self, 0x20) * 3.1415927f;
        b = b / 180.0f;
        float c = At<float>(self, 0x1C) * 3.1415927f;
        c = c / 180.0f;
        float d = 3.1415927f * At<float>(self, 0x24);
        d = d / 180.0f;
        vehicleLimitOn_C3CE8C = 1;
        vehicleLimitForward_C3CE70[0] = reinterpret_cast<uint32_t&>(forward.x);
        vehicleLimitForward_C3CE70[1] = reinterpret_cast<uint32_t&>(forward.y);
        vehicleLimitForward_C3CE70[2] = reinterpret_cast<uint32_t&>(forward.z);
        vehicleLimit_C3CE84 = a;
        vehicleLimit_C3CE88 = b;
        vehicleLimit_C3CE7C = c;
        vehicleLimit_C3CE80 = d;
    }
    return 1;
}

// a Butes entry by the path (0x1B6531C, 0x1B6535C, name) under 0x1B65360 (0: none)
inline uint32_t FirstPersonButes(const char* name) {
    uint32_t names[3];
    Name_Copy(&names[0], reinterpret_cast<const void*>(0x01B6531C));
    Name_Copy(&names[1], reinterpret_cast<const void*>(0x01B6535C));
    NameKey_Construct(&names[2], name);
    uint8_t path[8];
    Path_Build(path, 3, names);
    uint32_t butes = Butes_Find(path, reinterpret_cast<const void*>(0x01B65360));
    VectorDestruct(names, 4, 3, reinterpret_cast<void*>(0x0049A650));
    return butes;
}

// UpdateFirstPersonTrack: +0x28 its bone's hash, +0x2C/+0x2D the mode bits, +0x48/+0x4C
// Butes names, +0x50 the weapon kept attached, +0x54 the owner, +0x58 the ped, +0x5C the
// bone, +0x60 the model, +0x64/+0x68 the Butes

// 0x660D10 (thiscall): UpdateFirstPersonTrack::Start (the player in first person only)
uint8_t __fastcall UpdateFirstPersonTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x54) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (At<int32_t>(ped, 0x1310) != 0xD || gameMode != 2) {
        return 1;
    }
    At<uint8_t*>(self, 0x58) = ped;
    uint8_t* anim = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), 0x114);
    int32_t bone = 0;
    if (!anim || !At<void*>(anim, 0x94) || !Skeleton_FindBone(At<void*>(anim, 0x94), At<uint32_t>(self, 0x28), &bone) ||
        bone == -1) {
        bone = -1;
    }
    At<int32_t>(self, 0x5C) = bone;
    if (!self[0x50]) {
        Weapon_Detach_4D9510(At<uint8_t*>(At<uint8_t*>(self, 0x58), 0x1D0));
        Weapon_Fn4D9650(At<uint8_t*>(At<uint8_t*>(self, 0x58), 0x1D0), 1);
    }
    int32_t model = At<int32_t>(At<uint8_t*>(self, 0x58), 0x1574);
    At<int32_t>(self, 0x60) = model != -1 ? model : At<int16_t>(At<uint8_t*>(At<uint8_t*>(self, 0x58), 0x1D0), 0x10E);
    firstPersonMode_C3CD70 = self[0x2C] * 2 | self[0x2D];
    At<uint32_t>(self, 0x64) = 0;
    if (At<const char*>(self, 0x48)) {
        At<uint32_t>(self, 0x64) = FirstPersonButes(At<const char*>(self, 0x48));
    }
    At<uint32_t>(self, 0x68) = 0;
    if (At<const char*>(self, 0x4C)) {
        At<uint32_t>(self, 0x68) = FirstPersonButes(At<const char*>(self, 0x4C));
    }
    if (!At<uint32_t>(self, 0x68)) {
        At<uint32_t>(self, 0x68) = At<uint32_t>(self, 0x64);
    }
    return 1;
}

// 0x660EE0 (thiscall): UpdateFirstPersonTrack::End: the weapon shown again; first person
// back to its defaults
uint8_t __fastcall UpdateFirstPersonTrack_End(uint8_t* self, re::Edx) {
    if (!self[0x50]) {
        Weapon_Fn4D9460(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x54), 8), 0x1D0));
        Weapon_Fn4D9650(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x54), 8), 0x1D0), 0);
    }
    FirstPerson_Fn4DB5F0();
    uint8_t* butes = cameraButes_20C88B8;
    firstPersonMode_C3CD70 = 1;
    float a = At<float>(butes, 0x58);
    firstPerson_C3CD78 = At<float>(butes, 0x5C);
    firstPerson_C3CD74 = a;
    return 1;
}

// 0x660F40 (thiscall): RenderFirstPersonTrack::Resolve: the ped and its weapon drawn over
// the scene with the track's clip planes (+0x18, +0x1C, +0x20), the depth cleared first,
// the particles after; the camera's planes put back
void __fastcall RenderFirstPersonTrack_Render(uint8_t* self, re::Edx) {
    if (noFirstPerson_20C32E9 || noFirstPerson_20C32EA) {
        return;
    }
    RwCamera_EndUpdate(rwCamera_C1AE7C);
    float nearPlane = Camera_GetNearClip(niCamera);
    float farPlane = Camera_GetFarClip(niCamera);
    Camera_SetNearClip(niCamera, At<float>(self, 0x18));
    Camera_SetFarClip(niCamera, At<float>(self, 0x1C));
    Camera_Fn_6C8300(niCamera, int(At<float>(self, 0x20)));   // (_ftol)
    uint8_t colour[4];
    RGBA_Construct(colour, 0xFF, 0xFF, 0xFF, 0xFF);
    Camera_Clear_6CA7D0(rwCamera_C1AE7C, colour, 2);
    RwCamera_BeginUpdate(rwCamera_C1AE7C);
    vc::Call<void>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x5C);
    Weapon_Render_4D9C10(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x24), 8), 0x1D0));
    Renderer_ResetState(renderer_D140B8);
    ParticleRenderer_Fn578150(particleRenderer_B6F1EC, niCamera);
    EffectManager_RenderParticles(effectManager_B6F1D4);
    RwCamera_EndUpdate(rwCamera_C1AE7C);
    Camera_SetNearClip(niCamera, nearPlane);
    Camera_SetFarClip(niCamera, farPlane);
    Camera_Fn_6C8300(niCamera, 10);
    RwCamera_BeginUpdate(rwCamera_C1AE7C);
}
// 0x660F4A: the middle of 0x660F40 (its body after the first test), not a function
RE_UNHOOKED("RenderFirstPersonTrack::Render (its body)", 0x00660F4A);

// 0x6610B0 (thiscall): SetWeaponInfoTrack::Start: the ped's aiming (its targeting +0x44..
// +0x6C: half angles in radians, ranges) and weapon settings from the track; a model
// requested by name (+0x44); the inventory item's +0x18 set (+0x48)
uint8_t __fastcall SetWeaponInfoTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x4C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (!ped) {
        return 1;
    }
    float f1c = At<float>(self, 0x1C);
    int32_t model = At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E);
    uint8_t* aim = At<uint8_t*>(ped, 0x1C8);
    float b = At<float>(self, 0x20) * 3.1415927f;
    b = b / 180.0f;
    float f24 = At<float>(self, 0x24);
    float f28 = At<float>(self, 0x28);
    float f2c = At<float>(self, 0x2C);
    float f = At<float>(self, 0x30) * 3.1415927f;
    f = f / 180.0f;
    float f34 = At<float>(self, 0x34);
    float f38 = At<float>(self, 0x38);
    float f3c = At<float>(self, 0x3C);
    float a = 3.1415927f * At<float>(self, 0x18);
    a = a / 180.0f;
    aim[0x6C] = self[0x49];
    At<float>(aim, 0x44) = a * 0.5f;
    At<float>(aim, 0x48) = f1c;
    At<float>(aim, 0x4C) = b * 0.5f;
    At<float>(aim, 0x50) = f24;
    At<float>(aim, 0x54) = f28;
    At<float>(aim, 0x58) = f2c;
    At<float>(aim, 0x5C) = 0.5f * f;
    At<float>(aim, 0x60) = f34;
    At<float>(aim, 0x64) = f38;
    At<float>(aim, 0x68) = f3c;
    // (+0x12C below: the named model's index when there's a name, else the weapon's own)
    if (At<const char*>(self, 0x44)) {
        model = GetWeaponIndex(At<const char*>(self, 0x44));
    }
    float f40 = At<float>(self, 0x40);
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x4C), 8), 0x1D0);
    At<float>(weapon, 0x120) = At<float>(self, 0x2C);
    At<int32_t>(weapon, 0x12C) = model;
    At<float>(weapon, 0x124) = f40;
    uint8_t* p = At<uint8_t*>(At<uint8_t*>(self, 0x4C), 8);
    int32_t id = At<int16_t>(At<uint8_t*>(p, 0x1D0), 0x10E);
    if (id == -1) {
        return 1;
    }
    int32_t slot = Inventory_FindItem(At<uint8_t*>(p, 0x1C4), id);
    if (slot == -1) {
        return 1;
    }
    uint8_t* item = Inventory_GetItemInSlot(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x4C), 8), 0x1C4), slot);
    if (item) {
        At<uint32_t>(item, 0x18) = self[0x48];
    }
    return 1;
}

// 0x661220 (thiscall): SelectBestDirTossTargetTrack::Start: the partner target cleared
// (+0x18) or updated
uint8_t __fastcall SelectBestDirTossTargetTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (!ped) {
        return 1;
    }
    if (self[0x18]) {
        Targeting_ClearPartnerTarget(At<void*>(ped, 0x1C8));
        return 1;
    }
    Targeting_UpdatePartnerTarget(At<void*>(ped, 0x1C8));
    return 1;
}

// 0x661250 (thiscall): CameraZoomControlTrack::Start: zoom reset; the field of view set
// (+0x18 seconds to reach it, or the zoomed one (+0x1C) when +0x24)
uint8_t __fastcall CameraZoomControlTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x44) = owner;
    At<float>(zoomState_C674F0, 0x128) = 1.0f;
    At<uint32_t>(zoomState_C674F0, 0x138) = At<uint32_t>(self, 0x2C);
    zoomLevel_1B65C0C = 0;
    zoom_1B65C10 = 0.0f;
    if (!self[0x24]) {
        zoomLevel_1B65C0C = 0;
        float ms = At<float>(self, 0x18) * 1000.0f;
        Camera_SetFov(theCamera, 0.0f, int32_t(ms), 2, At<float>(self, 0x30), At<int32_t>(self, 0x34));
    } else {
        float fov = trk::CrtPow(At<float>(self, 0x1C), 0.0f);
        fov = fov * float(At<int32_t>(cameraButes_20C88B8, 0x6C));
        Camera_SetFov(theCamera, fov, 0, 1, At<float>(self, 0x30), At<int32_t>(self, 0x34));
    }
    At<float>(zoomState_C674E0, 0x3C) = zoom_1B65C10;
    At<uint32_t>(self, 0x48) = 0;
    zoomFlag_C3CEFE = self[0x40];
    return 1;
}

// 0x661320 (thiscall): the zoom stepped in or out (by the sign of `amount`): its current
// level found by multiplying the Butes' base by +0x1C until it passes the camera's field
// of view (at most +0x20 levels), then one level on, kept within 0..+0x20 - 1
void __fastcall CameraZoomControlTrack_Step(uint8_t* self, re::Edx, float amount) {
    float z = float(At<int32_t>(cameraButes_20C88B8, 0x6C));
    int32_t i = 0;
    float levels = At<float>(self, 0x20);
    if (levels > 0.0f) {
        float a = zoomLimitA_C3CDD0;
        int32_t useA = zoomUseA_C3CDD4;
        float c = zoomLimitC_C3CD04;
        bool growing = 1.0f < At<float>(self, 0x1C);
        for (;;) {
            float t = useA ? a : c;
            bool unordered = z != z || t != t;
            bool go = growing ? (z < t || unordered) : (z > t || unordered);
            if (!go) {
                break;
            }
            z = z * At<float>(self, 0x1C);
            i++;
            if (!(float(i) < At<float>(self, 0x20))) {
                break;
            }
        }
    }
    float fi = float(i);
    float x = (At<float>(self, 0x20) > fi || At<float>(self, 0x20) != At<float>(self, 0x20)) ? fi
                                                                                             : At<float>(self, 0x20) - 1.0f;
    int32_t current = int32_t(x);   // (_ftol)
    int32_t direction = 0.0f < amount ? 1 : -1;
    int32_t next = direction + current;
    if (next < 0) {
        next = 0;
    } else if (!(At<float>(self, 0x20) > float(next) || At<float>(self, 0x20) != At<float>(self, 0x20))) {
        next = int32_t(At<float>(self, 0x20) - 1.0f);
    }
    if (next != current) {
        CameraZoomControlTrack_SetLevel(self, next);
    }
}

// 0x661470 (thiscall): AmmoAttachmentTrack's attachments shown for `count` rounds: slots
// +0x24.. of the weapon's +0x18 get an attachment of the weapon's ammo model (at bones
// "<+0x1C><n>" of the weapon, its own bone +0x20); the others taken off and deleted
void __fastcall AmmoAttachmentTrack_Show(uint8_t* self, re::Edx, int count) {
    uint8_t* weapon = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0);
    int32_t slots = At<int32_t>(self, 0x18);
    int32_t ammo = At<int32_t>(weapon, 0x12C);
    uint8_t shown = uint8_t(count);
    if (int32_t(shown) > slots) {
        shown = uint8_t(slots);
    }
    if (ammo == -1) {
        shown = 0;
    }
    int32_t limit = shown;
    for (int32_t i = 0; i < At<int32_t>(self, 0x18); i++) {
        int32_t k = i - self[0x24];
        if (k < limit && k >= 0) {
            if (Weapon_GetAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0), i)) {
                continue;
            }
            void* memory = Malloc(0x298);
            uint8_t* round = memory ? Weapon_Construct(memory) : nullptr;
            Weapon_SetModel(round, ammo, 0);
            char name[0x58];
            Sprintf(name, reinterpret_cast<const char*>(0x0093BDF4), At<const char*>(self, 0x1C), i + 1);   // "%s%d"
            uint32_t hash = HashName(name);
            uint8_t* holder = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0);
            uint8_t* anim = At<uint8_t*>(holder, 0x114);
            int32_t bone = 0;
            int32_t holderBone = 0;
            if (anim && At<void*>(anim, 0x94) && Skeleton_FindBone(At<void*>(anim, 0x94), hash, &bone) && bone != -1) {
                holderBone = bone;
            }
            uint8_t* own = At<uint8_t*>(round, 0x114);
            int32_t ownBone = 0;
            bone = 0;
            if (own && At<void*>(own, 0x94) && Skeleton_FindBone(At<void*>(own, 0x94), At<uint32_t>(self, 0x20), &bone) &&
                bone != -1) {
                ownBone = bone;
            }
            Weapon_AddAttachment(holder, round, i, holderBone, ownBone);
        } else if (Weapon_GetAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0), i)) {
            uint8_t* taken = Weapon_TakeAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1D0), i);
            Weapon_Reset_4D9410(taken);
            if (taken) {
                vc::Call<void>(taken, 0, 1);
            }
        }
    }
}

// 0x661640 (thiscall): AmmoChangeTrack::Start: +0x18 rounds added to the weapon's ammo
uint8_t __fastcall AmmoChangeTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    int32_t weapon = At<int32_t>(At<uint8_t*>(ped, 0x1D0), 0x12C);
    int32_t ammo = Inventory_GetAmmo(At<uint8_t*>(ped, 0x1C4), weapon);
    Inventory_SetAmmo(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1C4), weapon,
                      At<int32_t>(self, 0x18) + ammo, 1);
    return 1;
}

// 0x661690 (thiscall, the track unused): WeaponRemoveCollisionProjectileTrack::Start: the
// projectile the ped collided with (the owner's +0x10 record, kinds 2 and 3) removed
uint8_t __fastcall WeaponRemoveCollisionProjectileTrack_Start(uint8_t*, re::Edx, uint8_t* owner) {
    uint8_t* record = At<uint8_t*>(owner, 0x10);
    int32_t kind = At<int32_t>(record, 0x60);
    if (kind == 2 || kind == 3) {
        const float* at = Pos(At<uint8_t*>(owner, 8));
        int32_t index = Projectiles_Find_4D4140(at[0], at[1], at[2], 1.0f, 1.0f, At<int16_t>(record, 0x5E),
                                                At<void*>(record, 4), 0);
        if (index != -1) {
            Projectiles_Remove_4D3F10(index, 0);
        }
    }
    return 1;
}

// 0x661700 (thiscall): CPedTargetLockTrack::Start: the ped's target locked (+0x18) or let go
uint8_t __fastcall CPedTargetLockTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x1C) = owner;
    void* targeting = At<void*>(At<uint8_t*>(owner, 8), 0x1C8);
    if (self[0x18]) {
        uint8_t* target = TargetingSystem_GetTarget(targeting);
        if (target) {
            Targeting_SetTarget(At<void*>(At<uint8_t*>(At<uint8_t*>(self, 0x1C), 8), 0x1C8), target);
        }
        return 0;
    }
    Targeting_SetTarget(targeting, nullptr);
    return 0;
}

// 0x661750 (thiscall): ExecuteOnTargetTrack::Update: its node (+0x18) played on the ped's
// target when that's a ped
uint8_t __fastcall ExecuteOnTargetTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ref = self + 0x18;
    if (!Reference_Get0C(ref, -1)) {
        return 0;
    }
    uint8_t* target = TargetingSystem_GetTarget(At<void*>(At<uint8_t*>(At<uint8_t*>(self, 0x28), 8), 0x1C8));
    if (!target || (target[0x108] & 7) != 3) {
        return 1;
    }
    uint8_t* node = Reference_Get0C(ref, -1);
    Controller_Fn5F4660(At<uint8_t*>(target, 0x2E0) + 0x258, node);
    return 0;
}

// 0x663120 (thiscall): ProjectilePickupTrack::Start: the projectile the ped is after
// picked up (when its weapon's info allows: its thing given to the ped's +0x508, the
// projectile removed); its model kept as the picked-up one
uint8_t __fastcall ProjectilePickupTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x18) = owner;
    int32_t model = -1;
    At<int32_t>(At<uint8_t*>(owner, 8), 0x1574) = model;
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x18), 8);
    uint8_t* projectile = At<uint8_t*>(ped, 0x544);
    if (!projectile || !At<uint32_t>(projectile, 0xC4) || !At<uint32_t>(ped, 0x149C)) {
        return 0;
    }
    int32_t index = At<int32_t>(projectile, 0x238);
    if (index != -1) {
        int32_t m = Projectiles_GetModel_4D3650(index);
        uint8_t* info = At<uint8_t*>(modelInfoPtrs[m], 0x18);
        if (info && info[0x24]) {
            uint8_t* p = Projectiles_Get_4D34B0(index);
            Fn478760(At<uint8_t*>(At<uint8_t*>(self, 0x18), 8) + 0x508, p + 4);
            Projectiles_Remove_Fn(index, 0);
            model = m;
        }
    }
    At<int32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x18), 8), 0x1574) = model;
    return 1;
}

// 0x6631D0 (thiscall): PropAttachToPropTrack::Start: a model (+0x18) attached to the ped's
// weapon in its first free attachment slot (at the weapon's bone +0x1C, its own +0x20),
// with Butes (+0x24 by name, else its weapon info's)
uint8_t __fastcall PropAttachToPropTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x2C) = owner;
    uint8_t* anim = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), 0x114);
    if (!anim || !At<uint32_t>(anim, 0x94)) {
        return 1;
    }
    int32_t model = GetModelIndexFromHash_51AF00(At<uint32_t>(self, 0x18));
    if (model == -1 || model == 0) {
        return 1;
    }
    At<uint32_t>(self, 0x28) = 0;
    if (At<uint32_t>(self, 0x24)) {
        uint32_t names[3];
        Name_Copy(&names[0], reinterpret_cast<const void*>(0x01B6531C));
        Name_Copy(&names[1], reinterpret_cast<const void*>(0x01B65394));
        Name_Copy(&names[2], self + 0x24);
        uint8_t path[8];
        Path_Build(path, 3, names);
        At<uint32_t>(self, 0x28) = Butes_Find(path, reinterpret_cast<const void*>(0x01B65398));
        VectorDestruct(names, 4, 3, reinterpret_cast<void*>(0x0049A650));
    } else {
        uint8_t* info = At<uint8_t*>(modelInfoPtrs[model], 0x18);
        if (info) {
            WeaponInfo_Load_7282A0(info);
            if (At<uint32_t>(info, 0x80)) {
                WeaponInfo_Load_7282A0(info);
                At<uint32_t>(self, 0x28) = At<uint32_t>(info, 0x80);
            }
        }
    }
    int slot = 0;
    for (; slot < 100; slot++) {
        if (!Weapon_GetAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x2C), 8), 0x1D0), slot)) {
            break;
        }
    }
    if (slot == 100) {
        return 1;
    }
    void* memory = Malloc(0x298);
    uint8_t* prop = memory ? Weapon_Construct(memory) : nullptr;
    Weapon_SetModel(prop, model, 0);
    if (At<uint32_t>(self, 0x28)) {
        At<uint32_t>(At<uint8_t*>(prop, 0x264), 0x80) = At<uint32_t>(self, 0x28);
    }
    uint8_t* holder = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x2C), 8), 0x1D0);
    int32_t holderBone = Entity_FindBone(holder, At<uint32_t>(self, 0x1C));
    if (holderBone == -1) {
        holderBone = 0;
    }
    int32_t ownBone = Entity_FindBone(prop, At<uint32_t>(self, 0x20));
    if (ownBone == -1) {
        ownBone = 0;
    }
    Weapon_AddAttachment(holder, prop, slot, holderBone, ownBone);
    return 1;
}

// a scatter power clamped to 0..1 (NaN kept)
inline float ClampPower(float p) {
    if (p < 0.0f) {
        return 0.0f;
    }
    if (1.0f < p) {
        return 1.0f;
    }
    return p;
}

// 0x663390 (thiscall): PropScatterAttachmentTrack::Start: the ped's weapon (+0x18), or its
// attachment on a bone (+0x1C), thrown: from its bone's position to a scattered point
// ahead, as a projectile (power: its +0x128 by +0x28); a remote weapon (0xBF44E0) only
// stops its effect. (With no attachment on the bone it goes on with the last slot tried.)
uint8_t __fastcall PropScatterAttachmentTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x3C) = owner;
    uint8_t* anim = At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), 0x114);
    if (!anim || !At<uint32_t>(anim, 0x94)) {
        return 1;
    }
    float from[3] = {0.0f, 0.0f, 0.0f};
    V3 to = {0.0f, 0.0f, 0.0f};
    uint8_t* thrown;
    if (self[0x18]) {
        thrown = At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0);
        At<float>(self, 0x34) = At<float>(thrown, 0x120);
        float p = At<float>(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), 0x128) * At<float>(self, 0x28);
        At<float>(self, 0x30) = p;
        At<float>(self, 0x30) = ClampPower(p);
        float bone[3];
        const float* b = ScatterTrack_BonePositionFn(self, bone, At<void*>(At<uint8_t*>(owner, 8), 0x114),
                                                     At<int32_t>(thrown, 0x138));
        from[0] = b[0];
        from[1] = b[1];
        from[2] = b[2];
        V3 point;
        to = *ScatterTrack_PointFn(self, &point, from[0], from[1], from[2]);
        uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
        if (At<uint32_t>(ped, 0x1368)) {
            At<int32_t>(ped, 0x1574) = At<int16_t>(thrown, 0x10E);
            Weapon_Reset_4D9410(thrown);
        } else {
            At<int32_t>(ped, 0x1574) = -1;
        }
    } else {
        int32_t bone = Entity_FindBone(At<uint8_t*>(At<uint8_t*>(owner, 8), 0x1D0), At<uint32_t>(self, 0x1C));
        if (bone == -1) {
            bone = 0;
        }
        int slot = 0;
        thrown = nullptr;
        for (; slot < 100; slot++) {
            thrown = Weapon_GetAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), slot);
            if (thrown && At<int32_t>(thrown, 0x138) == bone) {
                break;
            }
        }
        if (slot < 100) {
            At<float>(self, 0x34) = 1.0f;
            float p = At<float>(thrown, 0x128) * At<float>(self, 0x28);
            At<float>(self, 0x30) = p;
            At<float>(self, 0x30) = ClampPower(p);
            float position[3];
            const float* b = ScatterTrack_BonePositionFn(
                self, position, At<void*>(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), 0x114),
                At<int32_t>(thrown, 0x138));
            from[0] = b[0];
            from[1] = b[1];
            from[2] = b[2];
            V3 point;
            to = *ScatterTrack_PointFn(self, &point, from[0], from[1], from[2]);
            Weapon_TakeAttachment(At<uint8_t*>(At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8), 0x1D0), slot);
            Weapon_Reset_4D9410(thrown);
        }
    }
    ScatterTrack_Fn65DA20(self, thrown);
    V3 velocity;
    LaunchVelocityNoPed(&velocity, from[0], from[1], from[2], to.x, to.y, to.z, At<float>(self, 0x30),
                        At<uint8_t*>(self, 0x38));
    int32_t r20 = Weapon_Fn4D95E0(thrown);
    int32_t sinceLast = At<int32_t>(thrown, 0x140);
    if (sinceLast) {
        sinceLast = int32_t(timeInMs) - sinceLast;
    }
    if (At<int16_t>(thrown, 0x10E) == remoteWeapon_BF44E0) {
        if (r20) {
            EffectProxy_Kill(reinterpret_cast<void*>(intptr_t(r20)));
        }
        return 1;
    }
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x3C), 8);
    float holdMs = At<float>(ped, 0x1CC0) * 1000.0f;
    int32_t hold = int32_t(int64_t(holdMs));
    const uint32_t* zero = reinterpret_cast<const uint32_t*>(zeroVector3);
    AddProjectile(ped, At<int32_t>(thrown, 0x12C), from[0], from[1], from[2], velocity.x, velocity.y, velocity.z,
                  At<uint8_t*>(self, 0x38), 0.0f, At<float>(self, 0x30), r20, sinceLast, hold, zero[0], zero[1],
                  zero[2], zero[0], zero[1], zero[2], 0, 0);
    return 1;
}

// the camera limit's entity reference let go of
inline void ClearCameraLimit() {
    vehicleLimitOn_C3CE8C = 0;
    if (cameraLimitEntity_C3CE90) {
        Entity_CleanUpOldReference(cameraLimitEntity_C3CE90, &cameraLimitEntity_C3CE90);
        cameraLimitEntity_C3CE90 = nullptr;
    }
}

// 0x663780 (thiscall): CameraSetLimitTrack::Start: the camera kept within limits (+0x18,
// +0x20, +0x1C, +0x24 degrees) around a direction: the vehicle's facing (+0x3C set), away
// from the ped's +0x548 (+0x34), else +0x28; the camera's own when that's near zero; off
// when either lower limit is negative
uint8_t __fastcall CameraSetLimitTrack_Start(uint8_t* self, re::Edx, uint8_t* owner) {
    At<uint8_t*>(self, 0x38) = owner;
    uint8_t* ped = At<uint8_t*>(owner, 8);
    if (!ped) {
        return 0;
    }
    uint32_t d[3];
    reinterpret_cast<float&>(d[0]) = At<float>(self, 0x28);
    reinterpret_cast<float&>(d[1]) = At<float>(self, 0x2C);
    reinterpret_cast<float&>(d[2]) = At<float>(self, 0x30);
    if (At<uint32_t>(ped, 0x148C) && At<uint8_t*>(ped, 0x16C8)) {
        V3 forward;
        const V3* f = Placeable_GetForward(At<uint8_t*>(ped, 0x16C8), &forward);
        d[0] = reinterpret_cast<const uint32_t&>(f->x);
        d[1] = reinterpret_cast<const uint32_t&>(f->y);
        d[2] = reinterpret_cast<const uint32_t&>(f->z);
        self[0x3C] = 1;
    } else if (self[0x34] && At<uint8_t*>(ped, 0x548)) {
        V3 forward;
        const V3* f = Placeable_GetForward(At<uint8_t*>(ped, 0x548), &forward);
        float x = -f->x;
        float y = -f->y;
        float z = -f->z;
        d[0] = reinterpret_cast<uint32_t&>(x);
        d[1] = reinterpret_cast<uint32_t&>(y);
        d[2] = reinterpret_cast<uint32_t&>(z);
    }
    float dx = reinterpret_cast<float&>(d[0]);
    float dy = reinterpret_cast<float&>(d[1]);
    float dz = reinterpret_cast<float&>(d[2]);
    float length = dy * dy + dx * dx;
    length = length + dz * dz;
    length = std::fabs(length);
    length = crt::Sqrtf(length);   // sqrt
    if (length < 0.001f) {
        V3 forward;
        const V3* f = Camera_GetForward_41AA70(theCamera, &forward);
        d[0] = reinterpret_cast<const uint32_t&>(f->x);
        d[1] = reinterpret_cast<const uint32_t&>(f->y);
        d[2] = reinterpret_cast<const uint32_t&>(f->z);
    }
    float a = At<float>(self, 0x18) * 3.1415927f;
    a = a / 180.0f;
    float b = At<float>(self, 0x20) * 3.1415927f;
    b = b / 180.0f;
    float c = At<float>(self, 0x1C) * 3.1415927f;
    c = c / 180.0f;
    float e = 3.1415927f * At<float>(self, 0x24);
    e = e / 180.0f;
    if (!(c >= 0.0f) || !(e >= 0.0f)) {
        ClearCameraLimit();
        return 1;
    }
    vehicleLimitForward_C3CE70[0] = d[0];
    vehicleLimit_C3CE84 = a;
    vehicleLimitForward_C3CE70[1] = d[1];
    vehicleLimit_C3CE88 = b;
    vehicleLimitForward_C3CE70[2] = d[2];
    vehicleLimit_C3CE7C = c;
    vehicleLimitOn_C3CE8C = 1;
    vehicleLimit_C3CE80 = e;
    return 1;
}

// 0x663980 (thiscall): CameraSetLimitTrack::End: the limit off (when it was a vehicle's)
uint8_t __fastcall CameraSetLimitTrack_End(uint8_t* self, re::Edx) {
    if (self[0x3C]) {
        ClearCameraLimit();
    }
    return 1;
}

// 0x6639C0 (cdecl-like, ecx unused): whether the first-person ped (0xC3CE14) can hit what
// it aims at: its look target in clear view (or the target itself in the way); else what
// is along the camera's view to the weapon's range (or a nearer hit from the camera): a
// ped, or an object whose vf 0x7C holds and vf 0x80 is under 30000 (not ones with +0xC0 or
// a flagged ped when +0xF8 is set). (The ped is left as the world's ignored entity.)
uint8_t __cdecl FirstPerson_CanTarget() {
    uint8_t* ped = firstPersonPed_C3CE14;
    re::Ptr32<uint8_t> entity = nullptr;
    ColPoint colPoint;
    colPoint.pos = {0.0f, 0.0f, 0.0f};
    colPoint.entity = nullptr;
    colPoint.normal = {0.0f, 0.0f, 0.0f};
    colPoint.depth = 3.4028235e38f;
    V3 start = {0.0f, 0.0f, 0.0f};
    V3 end = {0.0f, 0.0f, 0.0f};
    ignoredEntity_C1AE70 = ped;
    uint8_t* weapon = At<uint8_t*>(ped, 0x1D0);
    if (!weapon || At<int16_t>(weapon, 0x10E) == -1) {
        return 0;
    }
    const float* camera = reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(niCamera)) + 0x24);
    float camX = camera[0];
    float camY = camera[1];
    float camZ = camera[2];
    uint8_t* target = TargetingSystem_GetLookTarget(At<void*>(ped, 0x1C8));
    if (target) {
        start = {camX, camY, camZ};
        V3 aim;
        end = *Targeting_GetTargetPos(At<void*>(ped, 0x1C8), &aim, target, 0);
        uint8_t result;
        if (ProcessLineOfSight(&start.x, &end.x, &colPoint, &entity, 0x18B, 1, 0)) {
            result = entity == target ? 1 : 0;
        } else {
            result = 1;
        }
        ReleaseColPoint(colPoint);
        return result;
    }
    float lo = At<float>(weapon, 0x120);
    float power = ClampPower(At<float>(weapon, 0x128));
    float hi = At<float>(weapon, 0x124);
    float range = hi + (lo - hi) * power;
    if (!(0.01f < range || 0.01f == range || range != range)) {
        range = 0.01f;
    }
    const float* camera2 = reinterpret_cast<const float*>(GetLocalTransform(GetCameraNode(niCamera)) + 0x24);
    start = {camera2[0], camera2[1], camera2[2]};
    V3 forward;
    const V3* f = Camera_GetForward_41AA70(theCamera, &forward);
    V3 step = {f->x * range, f->y * range, range * f->z};
    V3 sum;
    end = *VectorAdd(&sum, &start, &step);
    if (ProcessLineOfSight(&start.x, &end.x, &colPoint, &entity, 0x10F, 1, 1)) {
        end = colPoint.pos;
    }
    re::Ptr32<uint8_t> entity2 = nullptr;
    start = {camX, camY, camZ};
    uint8_t* hit = ProcessLineOfSight(&start.x, &end.x, &colPoint, &entity2, 0x10F, 1, 1) ? entity2 : entity;
    if (hit) {
        uint8_t type = hit[0x108] & 7;
        bool held = At<uint32_t>(hit, 0xC0) != 0;
        bool flagged = type == 3 && hit[0x1C98];
        if ((flagged || held) && At<uint32_t>(hit, 0xF8)) {
            ReleaseColPoint(colPoint);
            return 0;
        }
        if (type == 3 || (vc::Call<uint8_t>(hit, 0x7C) && vc::Call<float>(hit, 0x80) < 30000.0f)) {
            ColPoint_Destruct(&colPoint);
            return 1;
        }
    }
    ReleaseColPoint(colPoint);
    return 0;
}

// 0x663D90 (thiscall): UpdateFirstPersonTrack::Update: the player's first-person aim:
// turning, the Butes of whether it can hit (+0x64 or +0x68) passed on, the zoom's field
// of view, the weapon's aim offsets, the camera offset (+0x18.. unless +0x50)
uint8_t __fastcall UpdateFirstPersonTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* owner = At<uint8_t*>(self, 0x54);
    if (At<int32_t>(At<uint8_t*>(owner, 8), 0x1310) != 0xD || gameMode != 2 || !At<uint8_t*>(self, 0x58)) {
        return 0;
    }
    if (State_Get8_856850(gameState_A45500) == 0) {
        Ped_Fn4734C0(At<uint8_t*>(self, 0x58), -firstPersonTurn_C3CD08);
        Ped_Fn475730(At<uint8_t*>(self, 0x58), At<float>(At<uint8_t*>(self, 0x58), 0x1700));
    }
    uint8_t* ped = At<uint8_t*>(self, 0x58);
    int32_t model = At<int32_t>(self, 0x60);
    if (model != At<int16_t>(At<uint8_t*>(ped, 0x1D0), 0x10E) && model != At<int32_t>(ped, 0x1574) &&
        model != At<int32_t>(At<uint8_t*>(ped, 0x2E0), 0x18)) {
        return 0;
    }
    FirstPerson_Fn4DB5F0();
    uint8_t canHit = 0;
    if (State_Get8_856850(gameState_A45500) == 0) {
        canHit = FirstPerson_CanTarget_6639C0();
        uint8_t* butes = At<uint8_t*>(self, canHit ? 0x64 : 0x68);
        if (butes) {
            NameKey_Destruct(butes);
            FirstPerson_Fn4DB290(At<uint32_t>(butes, 0x40));
            uint8_t* b = At<uint8_t*>(self, canHit ? 0x64 : 0x68);
            int base = canHit ? 0x30 : 0x20;
            FirstPerson_Fn4DB660(At<float>(b, 0x1C), b[base], b[base + 4], b[base + 8], b[base + 0xC]);
        }
    }
    float fov = zoomUseA_C3CDD4 ? zoomLimitA_C3CDD0 : zoomLimitC_C3CD04;
    float zoom = fov / float(At<int32_t>(cameraButes_20C88B8, 0x6C));
    if (canHit) {
        zoom = At<float>(self, 0x44) * zoom;
    }
    if (zoomState_C674E0[0x38]) {
        zoom = 0.0f;
    }
    if (Pad_Fn738500(GetPlayerPadInput())) {
        weaponAim_C3CD94 = At<float>(self, 0x30) * zoom;
        weaponAim_C3CD98 = At<float>(self, 0x34);
        weaponAim_C3CD9C = At<float>(self, 0x38);
        weaponAim_C3CDA0 = At<float>(self, 0x3C);
    } else {
        weaponAim_C3CD94 = zoom * 1.5f;
        weaponAim_C3CD98 = 1.0f;
        weaponAim_C3CD9C = negativeZero_93C970;
        weaponAim_C3CDA0 = negativeZero_93C970;
    }
    weaponAim_C3CDA4 = At<float>(self, 0x40);
    if (!At<uint32_t>(At<uint8_t*>(At<uint8_t*>(self, 0x58), 0x1D0), 0x18)) {
        return 1;
    }
    if (self[0x50]) {
        cameraOffset_C2C0AC = defaultOffset_911B90;
        return 1;
    }
    cameraOffset_C2C0C0[0] = At<uint32_t>(self, 0x18);
    cameraOffset_C2C0C0[1] = At<uint32_t>(self, 0x1C);
    cameraOffset_C2C0C0[2] = At<uint32_t>(self, 0x20);
    cameraOffset_C2C0C0[3] = 0;   // (an uninitialised stack word in the game)
    cameraOffset_C2C0AC = At<uint32_t>(self, 0x5C);
    cameraOffset_C2C0A8 = At<float>(self, 0x24);
    return 1;
}

// the zoom's clicks at its ends ("Zoom_Short", once while a zoom button is pressed)
inline void ZoomEndSound(uint8_t* self) {
    if (At<uint32_t>(self, 0x4C)) {
        return;
    }
    if (!Pad_JustPressed(2, 0) && !Pad_JustPressed(3, 0)) {
        return;
    }
    int32_t id = Sounds_GetId(audio_CF3DA0, reinterpret_cast<const char*>(0x0093C9B0));
    int32_t handle = Sounds_Play(audio_CF3DA0, id, nullptr, 1, 0, 0, 0x78, 0, 0, 0, 0, 6, 1, 1.0f, 0);
    SoundHandle_Set_59D110(self + 0x4C, handle);
}

// 0x663FF0 (thiscall): CameraZoomControlTrack::Update: the zoom driven by the triggers:
// smoothly (+0x24: by +0x28 a frame, 0..1, with its sounds and field of view) or in steps
uint8_t __fastcall CameraZoomControlTrack_Update(uint8_t* self, re::Edx, float) {
    zoomFlag_C3CEFE = self[0x40];
    if (zoomState_C674E0[0x38]) {
        return 1;
    }
    if (self[0x24]) {
        float width = zoomWidth_C3CDD8;
        At<float>(zoomState_C674E0, 0x3C) = zoom_1B65C10;
        double in = Pad_GetValue_738840(2, 0);
        float out = Pad_GetValue_738840(3, 0);
        float delta = float(in) - out;
        delta = delta / 5.0f;
        float was = zoom_1B65C10;
        float now = At<float>(self, 0x28) * delta;
        now = now + was;
        zoom_1B65C10 = now;
        if (now > 1.0f) {
            ZoomEndSound(self);
            zoom_1B65C10 = 1.0f;
        } else if (0.0f > now) {
            ZoomEndSound(self);
            zoom_1B65C10 = 0.0f;
        }
        float moved = was - zoom_1B65C10;
        moved = std::fabs(moved);
        if (moved > 0.001f) {
            int32_t volume = int32_t(SFX_Fn59E4F0(1024.0f, 4));   // (_ftol)
            int32_t id = Sounds_GetId(audio_CF3DA0, reinterpret_cast<const char*>(0x0093C9A8));   // "Zoom"
            SoundHandle_Play_597400(self + 0x4C, id, 0, 2, volume, 0, 0);
        }
        At<float>(zoomState_C674E0, 0x3C) = zoom_1B65C10;
        float speed = std::fabs(delta);
        if (speed > 0.001f) {
            float fov = trk::CrtPow(At<float>(self, 0x1C), zoom_1B65C10);
            fov = fov * float(At<int32_t>(cameraButes_20C88B8, 0x6C));
            Camera_SetFov(theCamera, fov, 0, 1, At<float>(self, 0x30), At<int32_t>(self, 0x34));
        }
        float scaled = delta * 5.0f;
        scaled = std::fabs(scaled);
        if (width < scaled) {
            width = scaled;
        }
        zoomWidth_C3CDD8 = width;
        return 1;
    }
    if (zoomUseA_C3CDD4 == 2 || zoomUseA_C3CDD4 == 3) {
        return 1;
    }
    float in = Pad_GetValue_738840(2, 0) + 0.0f;
    double held = in;
    float out = Pad_GetValue_738840(3, 0);
    float delta = float(held) - out;
    if (!(delta == 0.0f)) {
        CameraZoomControlTrack_StepFn(self, delta);
    }
    return 1;
}

// 0x664310 (thiscall): AmmoAttachmentTrack::Update: its rounds shown for the ammo left
uint8_t __fastcall AmmoAttachmentTrack_Update(uint8_t* self, re::Edx, float) {
    uint8_t* ped = At<uint8_t*>(At<uint8_t*>(self, 0x28), 8);
    int32_t ammo = Inventory_GetAmmo(At<uint8_t*>(ped, 0x1C4), At<int32_t>(At<uint8_t*>(ped, 0x1D0), 0x12C));
    AmmoAttachmentTrack_ShowFn(self, ammo);
    return 1;
}

//@@NEXT@@
} // namespace

#define PROPTRACKV_REVERSED(name, address, ...) \
    REVERSED(name, address, RE_BODY(__VA_ARGS__), nullptr, nullptr, re::Mode::Original)

PROPTRACKV_REVERSED("PropPickupTrack::Start_65FB40", 0x0065FB40, &PropPickupTrack_Start);
PROPTRACKV_REVERSED("PropAttachTrack::Start_65FC30", 0x0065FC30, &PropAttachTrack_Start);
PROPTRACKV_REVERSED("PropAttachExTrack::Start_65FD80", 0x0065FD80, &PropAttachExTrack_Start);
PROPTRACKV_REVERSED("PropRemoveAttachmentTrack::Start_65FF00", 0x0065FF00, &PropRemoveAttachmentTrack_Start);
PROPTRACKV_REVERSED("PropScatterAttachmentTrack::BonePosition_6603F0", 0x006603F0, &ScatterTrack_BonePosition);
PROPTRACKV_REVERSED("PropScatterAttachmentTrack::Offset_660470", 0x00660470, &ScatterTrack_Offset);
PROPTRACKV_REVERSED("PropScatterAttachmentTrack::Point_660670", 0x00660670, &ScatterTrack_Point);
PROPTRACKV_REVERSED("PropSetSocketTrack::Start_6606D0", 0x006606D0, &PropSetSocketTrack_Start);
PROPTRACKV_REVERSED("PropDetachTrack::Start_6607A0", 0x006607A0, &PropDetachTrack_Start);
PROPTRACKV_REVERSED("PropDetachExTrack::Start_660840", 0x00660840, &PropDetachExTrack_Start);
PROPTRACKV_REVERSED("PropSetDesiredTrack::Start_6608D0", 0x006608D0, &PropSetDesiredTrack_Start);
PROPTRACKV_REVERSED("PropSetInTransitionTrack::Start_6609A0", 0x006609A0, &PropSetInTransitionTrack_Start);
PROPTRACKV_REVERSED("PropGetDesiredTrack::Start_6609D0", 0x006609D0, &PropGetDesiredTrack_Start);
PROPTRACKV_REVERSED("PropSpawnTrack::Start_660A10", 0x00660A10, &PropSpawnTrack_Start);
PROPTRACKV_REVERSED("SetCameraTrack::Start_660B90", 0x00660B90, &SetCameraTrack_Start);
PROPTRACKV_REVERSED("CameraSetLimitTrack::Update_660C40", 0x00660C40, &CameraSetLimitTrack_Update);
PROPTRACKV_REVERSED("UpdateFirstPersonTrack::Start_660D10", 0x00660D10, &UpdateFirstPersonTrack_Start);
PROPTRACKV_REVERSED("UpdateFirstPersonTrack::End_660EE0", 0x00660EE0, &UpdateFirstPersonTrack_End);
PROPTRACKV_REVERSED("RenderFirstPersonTrack::Render_660F40", 0x00660F40, &RenderFirstPersonTrack_Render);
PROPTRACKV_REVERSED("SetWeaponInfoTrack::Start_6610B0", 0x006610B0, &SetWeaponInfoTrack_Start);
PROPTRACKV_REVERSED("SelectBestDirTossTargetTrack::Start_661220", 0x00661220, &SelectBestDirTossTargetTrack_Start);
PROPTRACKV_REVERSED("CameraZoomControlTrack::Start_661250", 0x00661250, &CameraZoomControlTrack_Start);
PROPTRACKV_REVERSED("CameraZoomControlTrack::Step_661320", 0x00661320, &CameraZoomControlTrack_Step);
PROPTRACKV_REVERSED("AmmoAttachmentTrack::Show_661470", 0x00661470, &AmmoAttachmentTrack_Show);
PROPTRACKV_REVERSED("AmmoChangeTrack::Start_661640", 0x00661640, &AmmoChangeTrack_Start);
PROPTRACKV_REVERSED("WeaponRemoveCollisionProjectileTrack::Start_661690", 0x00661690, &WeaponRemoveCollisionProjectileTrack_Start);
PROPTRACKV_REVERSED("CPedTargetLockTrack::Start_661700", 0x00661700, &CPedTargetLockTrack_Start);
PROPTRACKV_REVERSED("ExecuteOnTargetTrack::Update_661750", 0x00661750, &ExecuteOnTargetTrack_Update);
PROPTRACKV_REVERSED("ProjectilePickupTrack::Start_663120", 0x00663120, &ProjectilePickupTrack_Start);
PROPTRACKV_REVERSED("PropAttachToPropTrack::Start_6631D0", 0x006631D0, &PropAttachToPropTrack_Start);
PROPTRACKV_REVERSED("PropScatterAttachmentTrack::Start_663390", 0x00663390, &PropScatterAttachmentTrack_Start);
PROPTRACKV_REVERSED("CameraSetLimitTrack::Start_663780", 0x00663780, &CameraSetLimitTrack_Start);
PROPTRACKV_REVERSED("CameraSetLimitTrack::End_663980", 0x00663980, &CameraSetLimitTrack_End);
PROPTRACKV_REVERSED("UpdateFirstPersonTrack::CanTarget_6639C0", 0x006639C0, &FirstPerson_CanTarget);
PROPTRACKV_REVERSED("UpdateFirstPersonTrack::Update_663D90", 0x00663D90, &UpdateFirstPersonTrack_Update);
PROPTRACKV_REVERSED("CameraZoomControlTrack::Update_663FF0", 0x00663FF0, &CameraZoomControlTrack_Update);
PROPTRACKV_REVERSED("AmmoAttachmentTrack::Update_664310", 0x00664310, &AmmoAttachmentTrack_Update);
//@@REG@@
