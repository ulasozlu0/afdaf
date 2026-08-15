
// MY BASE HERE
#include "Engine.hpp"
#include "../Interface/Renderer.hpp"
#include "../Core/Logger.hpp"
#include <math.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "../Security/RetSpoof.h"
#include "../Security/Spoof.h"
#include "mutex"
#pragma comment(lib, "winmm.lib")
#include "../Features/InstaLock.hpp"
#include "../Features/PakLoader.hpp"
//#include "skin.hpp"
uintptr_t camera_engine;
bool should_hook_gay;

int screen_width = GetSystemMetrics(SM_CXSCREEN);
int screen_height = GetSystemMetrics(SM_CYSCREEN);

bool InGame = 0;
uworld* UWorldSave;
float spin_value;

static auto OldAimAngles = fvector();

// visibility is now computed locally inside the actor loop - no more stale globals

namespace keys
{
    fkey left_mouse;
    fkey right_mouse;
    fkey insert;

    fkey w;
    fkey a;
    fkey s;
    fkey d;
    fkey space;

    fkey capslock;
}

// Forward declaration - defined later in this file (~line 2502)
namespace hooks {
    bool IsValidUObject(uobject* obj);
    void InvalidateViewModelCache();
}
using hooks::IsValidUObject;





// MSVC C2712: __try must not share a function with C++ object unwinding (e.g. fvector locals). Body stays in impl; outer is SEH-only.
static bool safe_get_firing_data_impl(ashootercharacter* character, fvector* loc, fvector* dir, aplayercontroller* controllers) {
    fvector muzzleLoc(0, 0, 0);
    fvector muzzleRot(0, 0, 0);

    character->get_firing_location_and_direction(&muzzleLoc, &muzzleRot, true);

    if (muzzleLoc.size() > 1.0) {
        *loc = muzzleLoc;
        *dir = muzzleRot;
        return (dir->size() > 0.1);
    }

    aplayercameramanager* cam = (controllers && memory::IsValidPointer((uintptr_t)controllers)) ? controllers->get_camera_manager() : nullptr;
    if (cam && memory::IsValidPointer((uintptr_t)cam)) {
        *loc = cam->get_camera_location();
        fvector camRot = cam->get_camera_rotation();
        double p = camRot.x * M_PI / 180.0;
        double y = camRot.y * M_PI / 180.0;
        double cosP = cos(p);
        dir->x = (float)(cosP * cos(y));
        dir->y = (float)(cosP * sin(y));
        dir->z = (float)sin(p);
        return true;
    }
    return false;
}

static bool safe_get_firing_data(ashootercharacter* character, fvector* loc, fvector* dir, aplayercontroller* controllers) {
    if (!character || !memory::IsValidPointer((uintptr_t)character)) return false;
    if (!loc || !dir) return false;
    __try {
        return safe_get_firing_data_impl(character, loc, dir, controllers);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// GetFiringLocationAndDirection returns location + pitch/yaw rotation (NOT a direction vector).
static bool safe_get_firing_loc_rot_impl(ashootercharacter* character, fvector* loc, fvector* rot, aplayercontroller* controllers) {
    fvector muzzleLoc(0, 0, 0);
    fvector muzzleRot(0, 0, 0);
    character->get_firing_location_and_direction(&muzzleLoc, &muzzleRot, true);

    if (muzzleLoc.size() > 1.0) {
        *loc = muzzleLoc;
        *rot = muzzleRot;
        return true;
    }

    aplayercameramanager* cam = (controllers && memory::IsValidPointer((uintptr_t)controllers))
        ? controllers->get_camera_manager() : nullptr;
    if (cam && memory::IsValidPointer((uintptr_t)cam)) {
        *loc = cam->get_camera_location();
        *rot = cam->get_camera_rotation();
        return true;
    }
    return false;
}

static bool safe_get_firing_loc_rot(ashootercharacter* character, fvector* loc, fvector* rot, aplayercontroller* controllers) {
    if (!character || !memory::IsValidPointer((uintptr_t)character)) return false;
    if (!loc || !rot) return false;
    __try {
        return safe_get_firing_loc_rot_impl(character, loc, rot, controllers);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void* m_memset(void* dest, char c, unsigned int len)
{
    unsigned int i;
    unsigned int fill;
    unsigned int chunks = len / sizeof(fill);
    char* char_dest = (char*)dest;
    unsigned int* uint_dest = (unsigned int*)dest;
    fill = (c << 24) + (c << 16) + (c << 8) + c;

    for (i = len; i > chunks * sizeof(fill); i--) {
        char_dest[i - 1] = c;
    }

    for (i = chunks; i > 0; i--) {
        uint_dest[i - 1] = fill;
    }

    return dest;
}

void* m_memcpy(void* dest, void* src, unsigned int len)
{
    unsigned int i;
    char* char_src = (char*)src;
    char* char_dest = (char*)dest;
    for (i = 0; i < len; i++) {
        char_dest[i] = char_src[i];
    }
    return dest;
}

static bool previous_viewmodel_state = false;


float brillpg = 1.0f;
flinearcolor hlpclrg{ 1.0f, 0.5f, 0.0f, 0.9f };

static flinearcolor tlrclr = { 1.0f, 1.0f, 1.0f, 0.7f };
static flinearcolor basee_color = { 0.835f, 0.576f, 0.584f, 1.0f };



namespace hooks
{
    static int TargetX = 0;
    static int TargetY = 0;
    float ESPThickness = 1;
    bool enemiesarround = true;
    int enemyID = 0;
    int enemyIDvis = 0;
    int CloseRangeDistanceID = 0;
    float CloseRangeDistance = 50.f;

    static bool auto_shot_active = false;
    static ashootercharacter* safe_target_actor = nullptr;
    static std::mutex target_mutex; // Mutex pour synchroniser l'accÃ¨s


    aplayercontroller* controllers;

    acknowledgedpawn* pawn;

    aplayercameramanager* camera_cache;

    ashootercharacter* character;

    static int seh_fp_num_materials(uintptr_t obj);
    static uskeletalmeshcomponent* seh_bot_get_mesh(ashootercharacter* actor);
    static uskeletalmeshcomponent* seh_bot_get_cosmetic_mesh(ashootercharacter* actor);
    static uskeletalmeshcomponent* seh_fp_getmesh1p(ashootercharacter* s);
    static uskeletalmeshcomponent* seh_fp_get_overlay1p(ashootercharacter* s);
    static uskeletalmeshcomponent* seh_fp_mesh3p(ashootercharacter* s);
    static bool seh_qs_is_alive(ashootercharacter* ply);

    static void seh_set_material(uintptr_t mesh_ptr, int idx, uobject* mat) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr)) return;
        __try { reinterpret_cast<UPrimitiveComponent*>(mesh_ptr)->set_material(idx, mat); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_SetMaterial(uintptr_t mesh_ptr, int idx, uobject* mat) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr)) return;
        __try { reinterpret_cast<uskeletalmeshcomponent*>(mesh_ptr)->SetMaterial(idx, mat); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    // Creating a UMaterialInstanceDynamic allocates GPU resources. Calling it per-frame
    // exhausts VRAM ("Out of video memory trying to allocate a rendering resource"), so
    // every MID is created once and reused; only its parameters are updated afterwards.
    struct MidKey {
        uintptr_t mesh;
        uintptr_t src;
        int idx;
        bool operator==(const MidKey& o) const { return mesh == o.mesh && src == o.src && idx == o.idx; }
    };
    struct MidKeyHash {
        size_t operator()(const MidKey& k) const {
            return (size_t)(k.mesh * 0x9E3779B97F4A7C15ull) ^ (size_t)(k.src << 1) ^ (size_t)(k.idx * 2654435761u);
        }
    };
    static std::unordered_map<MidKey, uobject*, MidKeyHash> g_mid_cache;
    static ULONGLONG g_mid_created = 0;
    static ULONGLONG g_mid_reused = 0;
    static ULONGLONG g_mid_create_blocked_until = 0;
    static ULONGLONG g_weapon_swap_grace_until = 0;
    static ULONGLONG g_mid_budget_tick = 0;
    static int g_mid_created_this_tick = 0;
    static constexpr int MAX_MID_CREATES_PER_FRAME = 8;
    static constexpr size_t MID_CACHE_SOFT_LIMIT = 384;
    static constexpr size_t MID_CACHE_HARD_LIMIT = 512;

    static inline bool is_weapon_swap_grace_active() {
        return GetTickCount64() < g_weapon_swap_grace_until;
    }

    static bool is_mesh_component_safe(uintptr_t mesh_ptr) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr)) return false;
        if (!IsValidUObject((uobject*)mesh_ptr)) return false;
        __try {
            int n = reinterpret_cast<UPrimitiveComponent*>(mesh_ptr)->get_num_materials();
            return n > 0 && n <= 64;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void ClearMidCache(const char* reason) {
        if (!g_mid_cache.empty()) {
            LOG_INFO("MID cache cleared (%s): entries=%zu created=%llu reused=%llu",
                reason, g_mid_cache.size(), g_mid_created, g_mid_reused);
        }
        g_mid_cache.clear();
    }

    static void PruneMidCache(const char* reason);

    static void notify_weapon_swap_grace(const char* reason) {
        const ULONGLONG now = GetTickCount64();
        const ULONGLONG swap_until = now + 450;
        if (swap_until > g_weapon_swap_grace_until)
            g_weapon_swap_grace_until = swap_until;
        if (swap_until > g_mid_create_blocked_until)
            g_mid_create_blocked_until = swap_until;
        // Never ClearMidCache here — orphaned MIDs leak VRAM.
        PruneMidCache(reason);
    }

    static uobject* seh_raw_create_dynamic_mat(uintptr_t mesh_ptr, int idx, uobject* source_mat) {
        __try { return reinterpret_cast<UPrimitiveComponent*>(mesh_ptr)->create_and_set_material_instance_dynamic_from_material(idx, source_mat); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static uobject* seh_get_material_slot(uintptr_t mesh_ptr, int idx) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr)) return nullptr;
        __try { return reinterpret_cast<UPrimitiveComponent*>(mesh_ptr)->get_material(idx); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static bool seh_mid_is_alive(uobject* mid) {
        if (!mid || !memory::IsValidPointerFast((uintptr_t)mid)) return false;
        __try { return IsValidUObject(mid); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static void PruneMidCache(const char* reason) {
        const size_t before = g_mid_cache.size();
        for (auto it = g_mid_cache.begin(); it != g_mid_cache.end(); ) {
            const uintptr_t mesh_ptr = it->first.mesh;
            uobject* cached = it->second;

            if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr) ||
                !seh_mid_is_alive(cached) || !is_mesh_component_safe(mesh_ptr)) {
                it = g_mid_cache.erase(it);
                continue;
            }

            uobject* current = seh_get_material_slot(mesh_ptr, it->first.idx);
            if (current != cached)
                seh_set_material(mesh_ptr, it->first.idx, cached);

            ++it;
        }
        if (before != g_mid_cache.size()) {
            LOG_INFO("MID cache pruned (%s): %zu -> %zu", reason, before, g_mid_cache.size());
        }
    }

    static void TrimMidCacheOverLimit(const char* reason) {
        if (g_mid_cache.size() <= MID_CACHE_HARD_LIMIT)
            return;

        PruneMidCache(reason);
        if (g_mid_cache.size() <= MID_CACHE_HARD_LIMIT)
            return;

        const size_t before = g_mid_cache.size();
        for (auto it = g_mid_cache.begin(); it != g_mid_cache.end(); ) {
            if (!is_mesh_component_safe(it->first.mesh) || !seh_mid_is_alive(it->second))
                it = g_mid_cache.erase(it);
            else
                ++it;
        }
        if (before != g_mid_cache.size()) {
            LOG_INFO("MID cache trimmed (%s): %zu -> %zu", reason, before, g_mid_cache.size());
        }
    }

    static uobject* seh_create_dynamic_mat(uintptr_t mesh_ptr, int idx, uobject* source_mat, bool local_hand = false) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr) || !source_mat || !memory::IsValidPointerFast((uintptr_t)source_mat))
            return nullptr;
        if (!is_mesh_component_safe(mesh_ptr)) return nullptr;

        MidKey key{ mesh_ptr, (uintptr_t)source_mat, idx };

        auto it = g_mid_cache.find(key);
        if (it != g_mid_cache.end()) {
            uobject* cached = it->second;
            if (seh_mid_is_alive(cached)) {
                uobject* current = seh_get_material_slot(mesh_ptr, idx);
                if (current != cached)
                    seh_set_material(mesh_ptr, idx, cached);
                g_mid_reused++;
                return cached;
            }
            g_mid_cache.erase(it);
        }

        // Local 1P hand chams may allocate during weapon swap grace (reuse still preferred).
        if (!local_hand && (is_weapon_swap_grace_active() || GetTickCount64() < g_mid_create_blocked_until))
            return nullptr;

        if (g_mid_cache.size() > MID_CACHE_SOFT_LIMIT)
            PruneMidCache("size pressure");
        TrimMidCacheOverLimit("size limit");

        const ULONGLONG budget_now = GetTickCount64();
        if (budget_now != g_mid_budget_tick) {
            g_mid_budget_tick = budget_now;
            g_mid_created_this_tick = 0;
        }
        if (g_mid_created_this_tick >= MAX_MID_CREATES_PER_FRAME) {
            return nullptr;
        }

        uobject* mid = seh_raw_create_dynamic_mat(mesh_ptr, idx, source_mat);
        if (!mid || !seh_mid_is_alive(mid)) return nullptr;

        g_mid_cache[key] = mid;
        g_mid_created++;
        g_mid_created_this_tick++;

        if (g_mid_created == 1 || g_mid_created % 500 == 0) {
            LOG_DBG("MID created #%llu (cache=%zu reused=%llu) mesh=0x%llX idx=%d",
                g_mid_created, g_mid_cache.size(), g_mid_reused, (unsigned long long)mesh_ptr, idx);
        }

        return mid;
    }

    static void seh_set_vector_param(UMaterialInstanceDynamic* mat, fname name, flinearcolor val) {
        if (!mat || !memory::IsValidPointerFast((uintptr_t)mat)) return;
        __try { mat->set_vector_parameter_value1(name, val); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_set_scalar_param(UMaterialInstanceDynamic* mat, fname name, double val) {
        if (!mat || !memory::IsValidPointerFast((uintptr_t)mat)) return;
        __try { mat->set_scalar_parameter_value(name, val); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_set_texture_param(UMaterialInstanceDynamic* mat, fname name, uobject* tex) {
        if (!mat || !memory::IsValidPointerFast((uintptr_t)mat)) return;
        __try { mat->set_texture_parameter_value(name, tex); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_set_ares_outline_mode(uintptr_t mesh_ptr, int mode, bool enable) {
        if (!mesh_ptr || !memory::IsValidPointerFast(mesh_ptr)) return;
        __try {
            reinterpret_cast<uskeletalmeshcomponent* (__fastcall*)(uskeletalmeshcomponent*, int, bool)>(
                memory::module_base + offsets::set_ares_outline_mode)(
                reinterpret_cast<uskeletalmeshcomponent*>(mesh_ptr), mode, enable);
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_set_relative_scale_3d(uintptr_t component_ptr, fvector scale) {
        if (!component_ptr || !memory::IsValidPointerFast(component_ptr)) return;
        if (!IsValidUObject((uobject*)component_ptr)) return;
        __try { reinterpret_cast<uskeletalmeshcomponent*>(component_ptr)->SetRelativeScale3D(scale); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void seh_eject_shell_casing(uintptr_t weapon_ptr) {
        if (!weapon_ptr || !memory::IsValidPointerFast(weapon_ptr)) return;
        __try { reinterpret_cast<currentequippable*>(weapon_ptr)->EjectShellCasing(); } __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    ashootercharacter* target_actor;


    inline static float  FOVChangorSprite = 5.0f;

    inline static float  Glow1 = 10.0f;
    inline static flinearcolor fresnel(2.093f, 0.019f, 20.0f, Glow1);

    inline float fresnelBaseR = 2.093f;
    inline float fresnelBaseG = 0.019f;
    inline float fresnelBaseB = 20.0f;

    //inline flinearcolor handcolor(fresnelBaseR, fresnelBaseG, fresnelBaseB, Glow1);

    /*  void Recoil_Control() {
          Sleep(3);
          mouse.mouse_event(TargetX, 5, 0);
      }*/

    flinearcolor get_color(bool condition) {
        return condition ? flinearcolor{ 0.1f, 1.0f, 0.1f, 1 } : flinearcolor{ 1.0f, 0.0f, 0.0f, 1 };
    }

    static flinearcolor Invisible{ 255.0f, 0.0f, 0.0f, 1.0f };  // Couleur rouge pour les Ã©lÃ©ments invisibles

    inline flinearcolor ChamsColor = Invisible;

    float rainbowTime = 0;  // Le temps qui augmentera pour gÃ©nÃ©rer un arc-en-ciel dynamique

    //flinearcolor Invisible = flinearcolor(0.0f, 0.0f, 0.0f, 0.0f);  // Invisible est une couleur de base (transparent ou noir)

    // Fonction pour gÃ©nÃ©rer une couleur arc-en-ciel en fonction du temps
    flinearcolor GetRainbowColor(float time)
    {
        const float PI = 3.14159265359f;
        float r = 0.5f + 0.5f * sin(time);
        float g = 0.5f + 0.5f * sin(time + 2.0f * PI / 3.0f);
        float b = 0.5f + 0.5f * sin(time + 4.0f * PI / 3.0f);
        return flinearcolor(r, g, b, 1.0f); // Alpha Ã  1.0 pour la pleine opacitÃ©
    }

    fvector2d posss = { ((float)GetSystemMetrics(SM_CXSCREEN) / 2) - 500, ((float)GetSystemMetrics(SM_CYSCREEN) / 2) - 475 };

    auto calculate_box_dimensions = [](fvector2d head_long_out, fvector2d base_out) -> std::pair<float, float> {
        float box_height = abs(head_long_out.y - base_out.y);
        float box_width = box_height * 0.55f;
        return { box_width, box_height };
        };

    static fvector seh_get_bone(uskeletalmeshcomponent* mesh, int ue_bone) {
        __try {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return fvector();
            return mesh->get_bone_location(ue_bone);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return fvector();
        }
    }

    auto get_target_bone_matrix = [](uskeletalmeshcomponent* mesh, int bone) -> fvector {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return fvector();
        switch (bone) {
        case 0: return seh_get_bone(mesh, 8);
        case 1: return seh_get_bone(mesh, 6);
        case 2: return seh_get_bone(mesh, 4);
        default: return fvector();
        }
        };

    void draw_head(ucanvas* canvas, uobject* Font, const wchar_t* text, flinearcolor color, fvector2d pos) {
        canvas->k2_drawtext(menu::font, fstring(text), pos, { 1.50f, 1.50f }, color, 0.f, { 0, 0, 0, 0.30f }, { 0, 0 }, true, true, true, { 0, 0, 0, 0.90f });
    }


    float DegreeToRadian(float degrees) {
        return degrees * (3.1415926535897932f / 180);
    }



    void draw_head2(ucanvas* canvas, uobject* Font, const wchar_t* text, flinearcolor color, fvector2d pos) {
        flinearcolor white_color = flinearcolor(1.0f, 1.0f, 1.0f, 1.0f); // Blanc (totalement opaque)

        // DÃ©finir la couleur de l'ombre : une version grise avec une opacitÃ© rÃ©duite (50 %)
        flinearcolor shadow_color = flinearcolor(0.5f, 0.5f, 0.5f, 0.5f); // Gris semi-transparent

        // DÃ©finir le dÃ©calage de l'ombre pour le texte (lÃ©gÃ¨rement dÃ©calÃ© en x et y)
        fvector2d shadow_offset = fvector2d{ 2.0f, 2.0f };

        // Draw the shadow first (slightly offset from the original position)
        canvas->k2_drawtext(menu::font, fstring(text), pos + shadow_offset, { 1.0f, 1.0f }, shadow_color, 0.f, shadow_color, shadow_offset, true, true, true, { 0, 0, 0, 0.90f });

        // Draw the main text with the purple color (overlaid on top of the shadow)
        canvas->k2_drawtext(menu::font, fstring(text), pos, { 1.0f, 1.0f }, white_color, 0.f, { 0, 0, 0, 0.30f }, { 0, 0 }, true, true, true, { 0, 0, 0, 0.90f });
    }





    boolean in_rect(double centerX, double centerY, double radius, double x, double y) {
        return x >= centerX - radius && x <= centerX + radius &&
            y >= centerY - radius && y <= centerY + radius;
    }





    inline bool is_target_in_fov(double screen_center_x, double screen_center_y, fvector2d target_pos) {

        return in_rect(screen_center_x, screen_center_y, globals::aimbot::a1m_f0v, target_pos.x, target_pos.y);
    }

    __forceinline fvector GetBoneMatrix(void* Mesh, int BoneIndex) {

        if (!Mesh) [[unlikely]]
            return fvector(0.f, 0.f, 0.f);


        if (BoneIndex < 0) [[unlikely]]
            return fvector(0.f, 0.f, 0.f);


        using BoneMatrixFn = FMatrix * (__fastcall*)(void*, FMatrix*, int);
        static const BoneMatrixFn fn = reinterpret_cast<BoneMatrixFn>(memory::module_base + offsets::bone_matrix);


        FMatrix BoneMatrix;


        fn(Mesh, &BoneMatrix, BoneIndex);


        return fvector(BoneMatrix.WPlane.X, BoneMatrix.WPlane.Y, BoneMatrix.WPlane.Z);
    }

    //fvector GetBoneMatrix(void* Mesh, int Idx) {

    //    FMatrix Matrix;
    //    reinterpret_cast<FMatrix* (__cdecl*)(void*, FMatrix*, int, uintptr_t, void*)>(spoofcall_stub)(Mesh, &Matrix, Idx, 0x46C4660, (void*)(memory::module_base + offsets::bone_matrix));

    //    
    //    return fvector(Matrix.WPlane.X, Matrix.WPlane.Y, Matrix.WPlane.Z);
    //}



    void DrawFilledRect(ucanvas* can, fvector2d topleft, fvector2d downright, flinearcolor clr, float alpha)
    {
        if (!can) return;
        float left = fminf((float)topleft.x, (float)downright.x);
        float right = fmaxf((float)topleft.x, (float)downright.x);
        float top = fminf((float)topleft.y, (float)downright.y);
        float bottom = fmaxf((float)topleft.y, (float)downright.y);
        flinearcolor fill = { clr.r * 0.15f, clr.g * 0.15f, clr.b * 0.15f, alpha };
        float spacing = 2.0f;
        for (float y = top; y <= bottom; y += spacing)
            can->K2_DrawTransparentLine({ left, y }, { right, y }, spacing, fill);
    }

    void DrawFilledBox(ucanvas* can, fvector2d topleft, fvector2d downright, flinearcolor color, float alpha = 0.10f)
    {
        if (!can) return;
        DrawFilledRect(can, topleft, downright, color, alpha);
        can->k2_drawline(topleft, { downright.x, topleft.y }, 1.0f, color);
        can->k2_drawline(topleft, { topleft.x, downright.y }, 1.0f, color);
        can->k2_drawline(downright, { topleft.x, downright.y }, 1.0f, color);
        can->k2_drawline(downright, { downright.x, topleft.y }, 1.0f, color);
    }

    void DrawBoxHealth(ucanvas* can, aplayercontroller* controller, uskeletalmeshcomponent* mesh,
        ashootercharacter* actor, flinearcolor color)
    {
        if (!can || !controller || !mesh || !actor) return;
        fvector head = mesh->get_bone_location(8);
        fvector base = mesh->get_bone_location(0);
        head.z += 6.f;
        fvector2d head2d, base2d;
        if (!controller->project_world_location_to_screen(head, head2d, false)) return;
        if (!controller->project_world_location_to_screen(base, base2d, false)) return;

        float height = fabsf((float)(base2d.y - head2d.y));
        float width = height * 0.55f;
        float cx = (float)head2d.x;
        float cy = (float)(head2d.y + base2d.y) * 0.5f;

        fvector2d tl = { cx - width * 0.5f, cy - height * 0.65f };
        fvector2d br = { cx + width * 0.5f, cy + height * 0.65f };

        DrawFilledRect(can, tl, br, color, globals::visuals::filled_box_alpha);

        float lineW = width / 4.f, lineH = height * 1.3f / 4.f;
        can->k2_drawline(tl, { tl.x + lineW, tl.y }, 1.0f, color);
        can->k2_drawline(tl, { tl.x, tl.y + lineH }, 1.0f, color);
        can->k2_drawline({ br.x - lineW, tl.y }, { br.x, tl.y }, 1.0f, color);
        can->k2_drawline({ br.x, tl.y }, { br.x, tl.y + lineH }, 1.0f, color);
        can->k2_drawline({ tl.x, br.y - lineH }, { tl.x, br.y }, 1.0f, color);
        can->k2_drawline({ tl.x, br.y }, { tl.x + lineW, br.y }, 1.0f, color);
        can->k2_drawline({ br.x - lineW, br.y }, { br.x, br.y }, 1.0f, color);
        can->k2_drawline({ br.x, br.y - lineH }, { br.x, br.y }, 1.0f, color);

        float hp = actor->health();
        float hp_pct = hp / 100.0f;
        if (hp_pct > 1.0f) hp_pct = 1.0f;
        if (hp_pct < 0.0f) hp_pct = 0.0f;

        float bar_top = tl.y;
        float bar_bot = br.y;
        float bar_x = tl.x - 5.0f;
        float bar_height = bar_bot - bar_top;
        float hp_start = bar_bot - (bar_height * hp_pct);

        flinearcolor hp_color = { 1.0f - hp_pct, hp_pct, 0.0f, 1.0f };
        can->k2_drawline({ bar_x, bar_bot }, { bar_x, hp_start }, 2.5f, hp_color);
    }

    void DrawBox(ucanvas* can, fvector2d topleft, fvector2d downright, flinearcolor color)
    {

        if (!can) {
            return;
        }

        if (!topleft.is_valid() || !downright.is_valid()) {
            return;
        }


        if (topleft.x > downright.x || topleft.y > downright.y) {
            if (topleft.x > downright.x) {
                double temp = topleft.x;
                topleft.x = downright.x;
                downright.x = temp;
            }
            if (topleft.y > downright.y) {
                double temp = topleft.y;
                topleft.y = downright.y;
                downright.y = temp;
            }
        }

        double h = downright.y - topleft.y;
        double w = downright.x - topleft.x;

        if (h <= 0.0 || w <= 0.0) {
            return;
        }


        double thicc = (ESPThickness > 0.0) ? ESPThickness : 1.0;

        fvector2d topright = fvector2d(downright.x, topleft.y);
        fvector2d bottomleft = fvector2d(topleft.x, downright.y);

        can->k2_drawline(topleft, topright, thicc, color);
        can->k2_drawline(topright, downright, thicc, color);
        can->k2_drawline(downright, bottomleft, thicc, color);
        can->k2_drawline(bottomleft, topleft, thicc, color);
    }

    double cross_product(fvector2d O, fvector2d A, fvector2d B) {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    }

    void DrawGraham3D(ucanvas* canvas, aplayercontroller* controller, uskeletalmeshcomponent* mesh, flinearcolor color)
    {
        if (!canvas || !controller || !mesh) return;

        fvector vBaseBone = mesh->get_bone_location(0);
        fvector vHeadBone = mesh->get_bone_location(8);
        if (!vBaseBone.is_valid() || !vHeadBone.is_valid()) return;

        fvector pt[4];
        pt[0] = fvector(vBaseBone.x + 53.0f, vBaseBone.y - 55.0f, vBaseBone.z);
        pt[1] = fvector(vBaseBone.x - 53.0f, vBaseBone.y - 55.0f, vBaseBone.z);
        pt[2] = fvector(vBaseBone.x - 53.0f, vBaseBone.y + 55.0f, vBaseBone.z);
        pt[3] = fvector(vBaseBone.x + 53.0f, vBaseBone.y + 55.0f, vBaseBone.z);

        fvector2d points[8];
        int p_count = 0;

        for (int i = 0; i < 4; i++) {
            fvector2d bottom_screen, top_screen;
            if (controller->project_world_location_to_screen(fvector(pt[i].x, pt[i].y, vBaseBone.z), bottom_screen, 0) && bottom_screen.is_valid()) {
                points[p_count++] = bottom_screen;
            }
            if (controller->project_world_location_to_screen(fvector(pt[i].x, pt[i].y, vHeadBone.z + 26.0f), top_screen, 0) && top_screen.is_valid()) {
                points[p_count++] = top_screen;
            }
        }

        if (p_count < 3) return;

        for (int i = 0; i < p_count - 1; ++i) {
            for (int j = i + 1; j < p_count; ++j) {
                if (points[j].x < points[i].x || (points[j].x == points[i].x && points[j].y < points[i].y)) {
                    fvector2d temp = points[i];
                    points[i] = points[j];
                    points[j] = temp;
                }
            }
        }

        fvector2d hull[16];
        int h_count = 0;

        for (int i = 0; i < p_count; ++i) {
            while (h_count >= 2 && cross_product(hull[h_count - 2], hull[h_count - 1], points[i]) <= 0)
                h_count--;
            hull[h_count++] = points[i];
        }

        for (int i = p_count - 2, t = h_count + 1; i >= 0; i--) {
            while (h_count >= t && cross_product(hull[h_count - 2], hull[h_count - 1], points[i]) <= 0)
                h_count--;
            hull[h_count++] = points[i];
        }

        h_count--;

        if (h_count < 3) return;

        double thicc = (ESPThickness > 0.0) ? ESPThickness : 1.5f;

        for (int i = 0; i < h_count; ++i) {
            canvas->k2_drawline(hull[i], hull[(i + 1) % h_count], thicc, color);
        }
    }

    void Draw3DBox(ucanvas* canvas, aplayercontroller* controllers, fvector origin, fvector extent, const flinearcolor& color)
    {



        fvector vertex[8] = {
            origin + fvector(-extent.x, -extent.y, -extent.z),
            origin + fvector(extent.x, -extent.y, -extent.z),
            origin + fvector(extent.x, extent.y, -extent.z),
            origin + fvector(-extent.x, extent.y, -extent.z),
            origin + fvector(-extent.x, -extent.y, extent.z),
            origin + fvector(extent.x, -extent.y, extent.z),
            origin + fvector(extent.x, extent.y, extent.z),
            origin + fvector(-extent.x, extent.y, extent.z)
        };

        fvector2d screenVertex[8];
        bool canProject = true;


        for (int i = 0; i < 8; i++)
        {

            if (!controllers->project_world_location_to_screen(vertex[i], screenVertex[i], false))
            {
                canProject = false;
                break;
            }
        }

        if (!canProject)
            return;


        canvas->k2_drawline(screenVertex[0], screenVertex[1], ESPThickness, color);
        canvas->k2_drawline(screenVertex[1], screenVertex[2], ESPThickness, color);
        canvas->k2_drawline(screenVertex[2], screenVertex[3], ESPThickness, color);
        canvas->k2_drawline(screenVertex[3], screenVertex[0], ESPThickness, color);


        canvas->k2_drawline(screenVertex[4], screenVertex[5], ESPThickness, color);
        canvas->k2_drawline(screenVertex[5], screenVertex[6], ESPThickness, color);
        canvas->k2_drawline(screenVertex[6], screenVertex[7], ESPThickness, color);
        canvas->k2_drawline(screenVertex[7], screenVertex[4], ESPThickness, color);


        canvas->k2_drawline(screenVertex[0], screenVertex[4], ESPThickness, color);
        canvas->k2_drawline(screenVertex[1], screenVertex[5], ESPThickness, color);
        canvas->k2_drawline(screenVertex[2], screenVertex[6], ESPThickness, color);
        canvas->k2_drawline(screenVertex[3], screenVertex[7], ESPThickness, color);
    }
    void DrawCornerBox(ucanvas* canvas, int x, int y, int W, int H, flinearcolor color, int thickness)
    {

        float lineW = W / 3.0f;
        float lineH = H / 3.0f;

        // Convert flinearcolor to a color format compatible with k2_drawline
        flinearcolor clr = menu::RGBtoFLC(color.r, color.g, color.b);  // Assuming RGBtoFLC converts correctly

        // Top-left corner
        canvas->k2_drawline(fvector2d(x, y), fvector2d(x, y + lineH), thickness, color); // Left vertical line
        canvas->k2_drawline(fvector2d(x, y), fvector2d(x + lineW, y), thickness, color); // Top horizontal line

        // Top-right corner
        canvas->k2_drawline(fvector2d(x + W - lineW, y), fvector2d(x + W, y), thickness, color); // Top horizontal line
        canvas->k2_drawline(fvector2d(x + W, y), fvector2d(x + W, y + lineH), thickness, color); // Right vertical line

        // Bottom-left corner
        canvas->k2_drawline(fvector2d(x, y + H - lineH), fvector2d(x, y + H), thickness, color); // Bottom-left vertical line
        canvas->k2_drawline(fvector2d(x, y + H), fvector2d(x + lineW, y + H), thickness, color); // Bottom horizontal line

        // Bottom-right corner
        canvas->k2_drawline(fvector2d(x + W - lineW, y + H), fvector2d(x + W, y + H), thickness, color); // Bottom-right horizontal line
        canvas->k2_drawline(fvector2d(x + W, y + H - lineH), fvector2d(x + W, y + H), thickness, color); // Right vertical line
    }

    bool bOutline = 1;

    void draw_text(ucanvas* canvas, uobject* Font, const wchar_t* text, flinearcolor color, fvector2d pos) {
        canvas->k2_drawtext(menu::font, fstring(text), pos, { 1.00f, 1.00f }, color, 0.f, { 0, 0, 0, 0.30f }, { 0, 0 }, true, true, true, { 0, 0, 0, 0.45f });
    }

    static void draw_text_rgb_string(ucanvas* canvas, uobject* Font, fstring text, float x, float y, flinearcolor color, bool CenterX = false) {
        canvas->k2_drawtext(menu::font, text, { x, y }, { 1.1f, 1.1f }, color, 0.f, { 0, 0, 0, 1.0f }, { 0, 0 }, CenterX, false, true, { 0, 0, 0, 1.0f });
    }

    int current_selection = 3;
    const int max_selection = 26;
    static bool open_canvas = true;

    currentequippable* myweapon;
    currentequippable* lastweapon;

    static flinearcolor maincolor{ 1.0f,1.0f,1.0f,1.0f };
    float bowv4l = 1;

    void draw_triangle(ucanvas* canvas, int current_selection, float x, float y, float size, flinearcolor color) {
        static float time = 0.0f;

        float animationSpeed = 0.05f;
        float maxMovementRange = 4.0f;

        float animatedX = x + (sin(time) * maxMovementRange);

        time += animationSpeed;

        if (time > 6.2832f) {
            time -= 6.2832f;
        }

        fvector2d point1, point2, point3;

        switch (current_selection) {
        case 3:
            point1 = { animatedX, y };
            point2 = { animatedX + size, y + size / 2 };
            point3 = { animatedX, y + size };
            break;
        case 4:
            point1 = { animatedX, y };
            point2 = { animatedX + size / 2, y + size };
            point3 = { animatedX + size, y };
            break;
        case 5:
            point1 = { animatedX + size, y };
            point2 = { animatedX, y + size / 2 };
            point3 = { animatedX + size, y + size };
            break;
        case 6:
            point1 = { animatedX, y + size };
            point2 = { animatedX + size / 2, y };
            point3 = { animatedX + size, y + size };
            break;
        default:
            point1 = { animatedX, y };
            point2 = { animatedX + size, y + size / 2 };
            point3 = { animatedX, y + size };
            break;
        }

        canvas->k2_drawline(point1, point2, 2.0f, color);
        canvas->k2_drawline(point2, point3, 2.0f, color);
        canvas->k2_drawline(point3, point1, 2.0f, color);
    }

    void DrawAdaptiveBoundingBox(ucanvas* canvas, aplayercontroller* my_controller, uskeletalmeshcomponent* mesh, flinearcolor color)
    {
        if (!canvas || !my_controller || !mesh) return;

        fvector vHeadBone = mesh->get_bone_location(8);
        fvector vBaseBone = mesh->get_bone_location(0);

        if (!vHeadBone.is_valid() || !vBaseBone.is_valid()) return;

        fvector2d bottom1, bottom2, bottom3, bottom4;
        fvector2d top1, top2, top3, top4;

        bool valid_projection =
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x + 53, vBaseBone.y - 55, vBaseBone.z), bottom1, 0) && bottom1.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x - 53, vBaseBone.y - 55, vBaseBone.z), bottom2, 0) && bottom2.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x - 53, vBaseBone.y + 55, vBaseBone.z), bottom3, 0) && bottom3.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x + 53, vBaseBone.y + 55, vBaseBone.z), bottom4, 0) && bottom4.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x + 53, vHeadBone.y - 55, vHeadBone.z + 26), top1, 0) && top1.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x - 53, vHeadBone.y - 55, vHeadBone.z + 26), top2, 0) && top2.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x - 53, vHeadBone.y + 55, vHeadBone.z + 26), top3, 0) && top3.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x + 53, vHeadBone.y + 55, vHeadBone.z + 26), top4, 0) && top4.is_valid();

        if (!valid_projection) return;

        float left_most = fmin(fmin(bottom1.x, bottom2.x), fmin(bottom3.x, bottom4.x)) - 1.0f;
        float right_most = fmax(fmax(top1.x, top2.x), fmax(top3.x, top4.x)) + 1.0f;
        float top_most = fmin(fmin(top1.y, top2.y), fmin(top3.y, top4.y)) - 5.0f;
        float bottom_most = fmax(fmax(bottom1.y, bottom2.y), fmax(bottom3.y, bottom4.y)) + 5.0f;

        fvector2d top_left = { left_most, top_most };
        fvector2d bottom_right = { right_most, bottom_most };

        if (globals::visuals::box2d) {
            canvas->k2_drawline(top_left, { bottom_right.x, top_left.y }, 1.5f, color);
            canvas->k2_drawline(top_left, { top_left.x, bottom_right.y }, 1.5f, color);
            canvas->k2_drawline(bottom_right, { bottom_right.x, top_left.y }, 1.5f, color);
            canvas->k2_drawline(bottom_right, { top_left.x, bottom_right.y }, 1.5f, color);
        }

    }

    void DrawAdaptiveCornerBox(ucanvas* canvas, aplayercontroller* my_controller, uskeletalmeshcomponent* mesh, flinearcolor color, double thickness = 1.5f)
    {
        if (!canvas || !my_controller || !mesh) return;

        fvector vHeadBone = mesh->get_bone_location(8);
        fvector vBaseBone = mesh->get_bone_location(0);
        if (!vHeadBone.is_valid() || !vBaseBone.is_valid()) return;

        fvector2d bottom1, bottom2, bottom3, bottom4;
        fvector2d top1, top2, top3, top4;

        bool valid_projection =
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x + 53, vBaseBone.y - 55, vBaseBone.z), bottom1, 0) && bottom1.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x - 53, vBaseBone.y - 55, vBaseBone.z), bottom2, 0) && bottom2.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x - 53, vBaseBone.y + 55, vBaseBone.z), bottom3, 0) && bottom3.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vBaseBone.x + 53, vBaseBone.y + 55, vBaseBone.z), bottom4, 0) && bottom4.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x + 53, vHeadBone.y - 55, vHeadBone.z + 26), top1, 0) && top1.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x - 53, vHeadBone.y - 55, vHeadBone.z + 26), top2, 0) && top2.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x - 53, vHeadBone.y + 55, vHeadBone.z + 26), top3, 0) && top3.is_valid() &&
            my_controller->project_world_location_to_screen(fvector(vHeadBone.x + 53, vHeadBone.y + 55, vHeadBone.z + 26), top4, 0) && top4.is_valid();

        if (!valid_projection) return;

        // Calculer les limites de la box
        float left_most = fmin(fmin(bottom1.x, bottom2.x), fmin(bottom3.x, bottom4.x)) - 1.0f;
        float right_most = fmax(fmax(top1.x, top2.x), fmax(top3.x, top4.x)) + 1.0f;
        float top_most = fmin(fmin(top1.y, top2.y), fmin(top3.y, top4.y)) - 5.0f;
        float bottom_most = fmax(fmax(bottom1.y, bottom2.y), fmax(bottom3.y, bottom4.y)) + 5.0f;

        // Calculer dimensions et positions
        float width = right_most - left_most;
        float height = bottom_most - top_most;

        // Longueur des coins (25% de chaque dimension)
        float corner_length_x = width * 0.25f;
        float corner_length_y = height * 0.25f;

        // Dessiner les 4 coins directement avec k2_drawline

        // Coin TOP-LEFT
        canvas->k2_drawline({ left_most, top_most }, { left_most + corner_length_x, top_most }, thickness, color); // Horizontal
        canvas->k2_drawline({ left_most, top_most }, { left_most, top_most + corner_length_y }, thickness, color); // Vertical

        // Coin TOP-RIGHT  
        canvas->k2_drawline({ right_most, top_most }, { right_most - corner_length_x, top_most }, thickness, color); // Horizontal
        canvas->k2_drawline({ right_most, top_most }, { right_most, top_most + corner_length_y }, thickness, color); // Vertical

        // Coin BOTTOM-LEFT
        canvas->k2_drawline({ left_most, bottom_most }, { left_most + corner_length_x, bottom_most }, thickness, color); // Horizontal
        canvas->k2_drawline({ left_most, bottom_most }, { left_most, bottom_most - corner_length_y }, thickness, color); // Vertical

        // Coin BOTTOM-RIGHT
        canvas->k2_drawline({ right_most, bottom_most }, { right_most - corner_length_x, bottom_most }, thickness, color); // Horizontal
        canvas->k2_drawline({ right_most, bottom_most }, { right_most, bottom_most - corner_length_y }, thickness, color); // Vertical
    }

    // Nouvelle fonction pour calculer la distance 3D rÃ©elle sans projection Ã©cran
    inline bool is_target_in_fov_360(fvector player_pos, fvector target_pos, double fov_radius) {
        // Calculer la distance 3D directe entre le joueur et la cible
        fvector delta = target_pos - player_pos;
        double distance_3d = sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

        // Le FOV devient une sphÃ¨re autour du joueur plutÃ´t qu'un cÃ´ne de vision
        return distance_3d <= fov_radius;
    }

    // Version alternative utilisant distance horizontale seulement (plus rÃ©aliste)
    inline bool is_target_in_fov_360_horizontal(fvector player_pos, fvector target_pos, double fov_radius) {
        // Calculer seulement la distance horizontale (ignorer Z pour la hauteur)
        fvector delta = target_pos - player_pos;
        double distance_2d = sqrt(delta.x * delta.x + delta.y * delta.y);

        return distance_2d <= fov_radius;
    }

    // Fonction hybride : FOV normal OU 360Â° selon le mode
    inline bool is_target_in_fov_adaptive(double screen_center_x, double screen_center_y,
        fvector2d target_screen_pos,
        fvector player_world_pos,
        fvector target_world_pos,
        double fov_value,
        bool use_360_fov) {
        if (use_360_fov) {
            // Mode 360Â° : utiliser la distance monde directe
            return is_target_in_fov_360_horizontal(player_world_pos, target_world_pos, fov_value);
        }
        else {
            // Mode normal : utiliser la projection Ã©cran classique
            return in_rect(screen_center_x, screen_center_y, fov_value, target_screen_pos.x, target_screen_pos.y);
        }
    }



    enum class ValorantBones : int {
        CHEST = 6,
        RIBS = 5,
        HEAD = 8,
        NECK = 9,
        LEFT_SHOULDER = 36,
        RIGHT_SHOULDER = 33,
        LEFT_ARM = 37,
        RIGHT_ARM = 30,
        LEFT_HAND = 38,
        RIGHT_HAND = 32,
        PELVIS = 60,
        LEFT_HIPS = 77,
        RIGHT_HIPS = 63,
        LEFT_KNEE = 79,
        RIGHT_KNEE = 65,
        LEFT_ANKLE = 80,
        RIGHT_ANKLE = 66
    };

    // Structure pour stocker toutes les positions des bones
    struct ValorantSkeletonData {
        fvector2d head;
        fvector2d neck;
        fvector2d chest;
        fvector2d ribs;
        fvector2d left_shoulder;
        fvector2d right_shoulder;
        fvector2d left_arm;
        fvector2d right_arm;
        fvector2d left_hand;
        fvector2d right_hand;
        fvector2d pelvis;
        fvector2d left_hips;
        fvector2d right_hips;
        fvector2d left_knee;
        fvector2d right_knee;
        fvector2d left_ankle;
        fvector2d right_ankle;

        bool valid = false;
    };


    ValorantSkeletonData extract_skeleton_bones(aplayercontroller* controller, uskeletalmeshcomponent* mesh) {
        ValorantSkeletonData skeleton_data;

        if (!controller || !mesh) {
            skeleton_data.valid = false;
            return skeleton_data;
        }

        auto get_bone_screen_pos = [&](ValorantBones bone_id, fvector2d& out_pos) -> bool {
            fvector world_pos = mesh->get_bone_location(static_cast<int>(bone_id));
            if (!world_pos.is_valid()) return false;

            return controller->project_world_location_to_screen(world_pos, out_pos, false) && out_pos.is_valid();
            };

        // Extraire toutes les positions
        bool all_bones_valid =
            get_bone_screen_pos(ValorantBones::HEAD, skeleton_data.head) &&
            get_bone_screen_pos(ValorantBones::NECK, skeleton_data.neck) &&
            get_bone_screen_pos(ValorantBones::CHEST, skeleton_data.chest) &&
            get_bone_screen_pos(ValorantBones::RIBS, skeleton_data.ribs) &&
            get_bone_screen_pos(ValorantBones::LEFT_SHOULDER, skeleton_data.left_shoulder) &&
            get_bone_screen_pos(ValorantBones::RIGHT_SHOULDER, skeleton_data.right_shoulder) &&
            get_bone_screen_pos(ValorantBones::LEFT_ARM, skeleton_data.left_arm) &&
            get_bone_screen_pos(ValorantBones::RIGHT_ARM, skeleton_data.right_arm) &&
            get_bone_screen_pos(ValorantBones::LEFT_HAND, skeleton_data.left_hand) &&
            get_bone_screen_pos(ValorantBones::RIGHT_HAND, skeleton_data.right_hand) &&
            get_bone_screen_pos(ValorantBones::PELVIS, skeleton_data.pelvis) &&
            get_bone_screen_pos(ValorantBones::LEFT_HIPS, skeleton_data.left_hips) &&
            get_bone_screen_pos(ValorantBones::RIGHT_HIPS, skeleton_data.right_hips) &&
            get_bone_screen_pos(ValorantBones::LEFT_KNEE, skeleton_data.left_knee) &&
            get_bone_screen_pos(ValorantBones::RIGHT_KNEE, skeleton_data.right_knee) &&
            get_bone_screen_pos(ValorantBones::LEFT_ANKLE, skeleton_data.left_ankle) &&
            get_bone_screen_pos(ValorantBones::RIGHT_ANKLE, skeleton_data.right_ankle);

        skeleton_data.valid = all_bones_valid;
        return skeleton_data;
    }

    // Fonction pour dessiner le skeleton complet
    void draw_valorant_skeleton(ucanvas* canvas, const ValorantSkeletonData& skeleton, flinearcolor color, double thickness = 1.5f) {
        if (!canvas || !skeleton.valid) return;

        // Dessiner la colonne vertÃ©brale (tÃªte -> cou -> chest -> ribs -> pelvis)
        canvas->k2_drawline(skeleton.head, skeleton.neck, thickness, color);
        canvas->k2_drawline(skeleton.neck, skeleton.chest, thickness, color);
        canvas->k2_drawline(skeleton.chest, skeleton.ribs, thickness, color);
        canvas->k2_drawline(skeleton.ribs, skeleton.pelvis, thickness, color);

        // Dessiner les Ã©paules et bras complets (neck -> shoulders -> arms -> hands)
        canvas->k2_drawline(skeleton.neck, skeleton.left_shoulder, thickness, color);
        canvas->k2_drawline(skeleton.neck, skeleton.right_shoulder, thickness, color);
        canvas->k2_drawline(skeleton.left_shoulder, skeleton.left_arm, thickness, color);
        canvas->k2_drawline(skeleton.right_shoulder, skeleton.right_arm, thickness, color);
        canvas->k2_drawline(skeleton.left_arm, skeleton.left_hand, thickness, color);
        canvas->k2_drawline(skeleton.right_arm, skeleton.right_hand, thickness, color);

        // Dessiner les hanches (left hips <- pelvis -> right hips)
        canvas->k2_drawline(skeleton.pelvis, skeleton.left_hips, thickness, color);
        canvas->k2_drawline(skeleton.pelvis, skeleton.right_hips, thickness, color);

        // Dessiner les jambes gauches (left hips -> left knee -> left ankle)
        canvas->k2_drawline(skeleton.left_hips, skeleton.left_knee, thickness, color);
        canvas->k2_drawline(skeleton.left_knee, skeleton.left_ankle, thickness, color);

        // Dessiner les jambes droites (right hips -> right knee -> right ankle)
        canvas->k2_drawline(skeleton.right_hips, skeleton.right_knee, thickness, color);
        canvas->k2_drawline(skeleton.right_knee, skeleton.right_ankle, thickness, color);
    }

    // Fonction principale Ã  appeler dans ton ESP
    void draw_valorant_skeleton_esp(aplayercontroller* controller, uskeletalmeshcomponent* mesh, ucanvas* canvas, flinearcolor color) {
        if (!controller || !mesh || !canvas) return;

        // Extraire les positions des bones
        ValorantSkeletonData skeleton_data = extract_skeleton_bones(controller, mesh);

        // Dessiner le skeleton si valide
        if (skeleton_data.valid) {
            draw_valorant_skeleton(canvas, skeleton_data, color);
        }
    }


    // Mosca glow: wall color uses camera/POV-relative LOS (matches training bots behind glass/walls better than naked line_of_sight-only).
    static bool MoscaWorldVisibleForOutline(aplayercontroller* ctrl, ashootercharacter* viewer, ashootercharacter* target)
    {
        if (!ctrl || !target) return true;
        if (viewer) {
            uskeletalmeshcomponent* vm = viewer->get_mesh();
            if (vm && memory::IsValidPointer((uintptr_t)vm)) {
                fvector vp = vm->get_bone_location(8);
                if (vp.is_valid())
                    return ctrl->line_of_sight_from_position(target, vp);
            }
        }
        return ctrl->line_of_sight(target);
    }

    static flinearcolor MoscaOutlinePickColor(aplayercontroller* ctrl, ashootercharacter* viewer, ashootercharacter* target)
    {
        if (globals::visuals::outline_custom_color)
            return flinearcolor(globals::visuals::outline_color_rgb[0], globals::visuals::outline_color_rgb[1], globals::visuals::outline_color_rgb[2], 1.0f);
        if (!MoscaWorldVisibleForOutline(ctrl, viewer, target))
            return flinearcolor(1.0f, 1.0f, 0.0f, 1.0f);
        if (globals::visuals::vischeck)
            return defines::VisibleBox_ESPColor;
        return defines::visuals_color;
    }

    void DrawOutlineWeaponGlowESP(ucanvas* canvas, aplayercontroller* controller,
        uskeletalmeshcomponent* mesh, flinearcolor color, float line_thickness)
    {
        if (!canvas || !controller || !mesh) return;

        int nb = mesh->get_num_bones();
        if (nb < 2) return;

        std::vector<fvector2d> sc((size_t)nb);
        std::vector<uint8_t> ok((size_t)nb, 0);

        for (int i = 0; i < nb; ++i) {
            fvector w = mesh->get_bone_location(i);
            if (!w.is_valid()) continue;
            if (!controller->project_world_location_to_screen(w, sc[(size_t)i], false) || !sc[(size_t)i].is_valid())
                continue;
            ok[(size_t)i] = 1;
        }

        struct WDrawCmd { fvector2d a, b; };
        std::vector<WDrawCmd> cmds;
        cmds.reserve((size_t)nb);

        for (int i = 0; i < nb - 1; ++i) {
            if (!ok[(size_t)i] || !ok[(size_t)i + 1]) continue;
            float dx = (float)(sc[(size_t)i + 1].x - sc[(size_t)i].x);
            float dy = (float)(sc[(size_t)i + 1].y - sc[(size_t)i].y);
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 0.5f || d > 300.f) continue;
            cmds.push_back({ sc[(size_t)i], sc[(size_t)i + 1] });
        }
        if (cmds.empty()) return;

        float core_thick = 0.7f + line_thickness * 0.25f;
        if (core_thick > 1.8f) core_thick = 1.8f;

        for (int g = 3; g >= 1; g--) {
            float t = (float)g / 4.f;
            float glow_thick = core_thick + (float)g * 1.2f;
            float a = color.a * 0.07f * (1.f - t * 0.5f);
            flinearcolor gc = flinearcolor(color.r, color.g, color.b, a);
            for (size_t i = 0; i < cmds.size(); i++)
                canvas->k2_drawline(cmds[i].a, cmds[i].b, glow_thick, gc);
        }

        flinearcolor core_col = flinearcolor(color.r, color.g, color.b, color.a * 0.9f);
        for (size_t i = 0; i < cmds.size(); i++)
            canvas->k2_drawline(cmds[i].a, cmds[i].b, core_thick, core_col);
    }

    void DrawPlayerGlowESP(ucanvas* canvas, aplayercontroller* controller,
        uskeletalmeshcomponent* mesh, float line_thickness, ashootercharacter* actor,
        flinearcolor outline_color = { 1.f, 1.f, 1.f, 1.f });

    // Real silhouette outline: project every bone to screen, puff each with a
    // small screen-space ring, take 2D convex hull of the resulting points,
    // stroke the hull polygon.
    void DrawSilhouetteHullOutline(ucanvas* canvas, aplayercontroller* controller,
        uskeletalmeshcomponent* mesh, float line_thickness, ashootercharacter* actor,
        flinearcolor color)
    {
        if (!canvas || !controller || !mesh || !actor) return;
        int nbones = mesh->get_num_bones();
        if (nbones <= 0 || nbones > 300) return;

        fvector head_w = mesh->get_bone_location(8);
        fvector pel_w  = mesh->get_bone_location(3);
        fvector2d sHead, sPel;
        if (!head_w.is_valid() || !pel_w.is_valid()) return;
        if (!controller->project_world_location_to_screen(head_w, sHead, false) || !sHead.is_valid()) return;
        if (!controller->project_world_location_to_screen(pel_w,  sPel,  false) || !sPel.is_valid())  return;
        float screen_h = fabsf((float)sPel.y - (float)sHead.y);
        if (screen_h < 4.f) return;
        float scale = screen_h / 120.f;
        if (scale < 0.55f) scale = 0.55f;
        if (scale > 5.0f)  scale = 5.0f;

        float puff = globals::visuals::outline_hull_padding * scale;
        if (puff < 2.f) puff = 2.f;

        std::vector<fvector2d> pts;
        pts.reserve((size_t)nbones * 9);

        const int kRing = 8;
        static float ring_cos[kRing], ring_sin[kRing];
        static bool ring_ready = false;
        if (!ring_ready) {
            for (int k = 0; k < kRing; k++) {
                float a = (float)k * (2.0f * 3.14159265f / (float)kRing);
                ring_cos[k] = cosf(a); ring_sin[k] = sinf(a);
            }
            ring_ready = true;
        }

        for (int b = 0; b < nbones; b++) {
            fvector w = mesh->get_bone_location(b);
            if (!w.is_valid()) continue;
            fvector2d s;
            if (!controller->project_world_location_to_screen(w, s, false)) continue;
            if (!s.is_valid()) continue;
            if (s.x < -10000 || s.x > 20000 || s.y < -10000 || s.y > 20000) continue;
            pts.push_back({ s.x, s.y });
            for (int k = 0; k < kRing; k++) {
                pts.push_back({ s.x + ring_cos[k] * puff, s.y + ring_sin[k] * puff });
            }
        }
        if (pts.size() < 3) return;

        std::sort(pts.begin(), pts.end(), [](const fvector2d& a, const fvector2d& b) {
            if (a.x != b.x) return a.x < b.x;
            return a.y < b.y;
        });
        auto cross_z = [](const fvector2d& O, const fvector2d& A, const fvector2d& B) -> double {
            return ((double)A.x - (double)O.x) * ((double)B.y - (double)O.y)
                 - ((double)A.y - (double)O.y) * ((double)B.x - (double)O.x);
        };
        int n = (int)pts.size();
        std::vector<fvector2d> hull(2 * n);
        int k = 0;
        for (int i = 0; i < n; i++) {
            while (k >= 2 && cross_z(hull[k - 2], hull[k - 1], pts[i]) <= 0.0) k--;
            hull[k++] = pts[i];
        }
        int t = k + 1;
        for (int i = n - 2; i >= 0; i--) {
            while (k >= t && cross_z(hull[k - 2], hull[k - 1], pts[i]) <= 0.0) k--;
            hull[k++] = pts[i];
        }
        if (k < 4) return;
        hull.resize(k - 1);

        float core_thick = 1.2f + line_thickness * 0.35f;
        if (core_thick > 2.4f) core_thick = 2.4f;

        int glow_passes = (int)globals::visuals::outline_esp_glow_passes;
        if (glow_passes < 0) glow_passes = 0;
        if (glow_passes > 5) glow_passes = 5;

        for (int g = glow_passes; g >= 1; g--) {
            float t01 = (float)g / (float)(glow_passes + 1);
            float glow_thick = core_thick + (float)g * 1.4f * scale;
            float alpha = color.a * 0.10f * (1.f - t01 * 0.5f);
            flinearcolor gc = flinearcolor(color.r, color.g, color.b, alpha);
            for (size_t i = 0; i < hull.size(); i++) {
                fvector2d a = hull[i];
                fvector2d b = hull[(i + 1) % hull.size()];
                canvas->k2_drawline(a, b, glow_thick, gc);
            }
        }

        flinearcolor core_col = flinearcolor(color.r, color.g, color.b, color.a * 0.95f);
        for (size_t i = 0; i < hull.size(); i++) {
            fvector2d a = hull[i];
            fvector2d b = hull[(i + 1) % hull.size()];
            canvas->k2_drawline(a, b, core_thick, core_col);
        }
    }

    void DrawOutlineESP(ucanvas* canvas, aplayercontroller* controller,
        uskeletalmeshcomponent* mesh, flinearcolor color,
        float line_thickness, ashootercharacter* actor)
    {
        if (!canvas || !controller || !mesh || !actor) return;
        if (globals::visuals::outline_style == 1) {
            DrawSilhouetteHullOutline(canvas, controller, mesh, line_thickness, actor, color);
        } else {
            DrawPlayerGlowESP(canvas, controller, mesh, line_thickness, actor, color);
        }
    }

    void DrawPlayerGlowESP(ucanvas* canvas, aplayercontroller* controller,
        uskeletalmeshcomponent* mesh, float line_thickness, ashootercharacter* actor,
        flinearcolor outline_color)
    {
        if (!canvas || !controller || !mesh || !actor) return;

        int bone_count = memory::read<int32_t>((uintptr_t)mesh + offsets::bone_cout);

        struct BoneIDs {
            int head, neck, chest;
            int l_shoulder, l_elbow, l_hand;
            int r_shoulder, r_elbow, r_hand;
            int l_thigh, l_knee, l_foot;
            int r_thigh, r_knee, r_foot;
        };

        BoneIDs bones;
        switch (bone_count) {
        case 101:
            bones = { 8, 21, 19, 24, 25, 26, 50, 51, 52, 76, 77, 78, 83, 84, 85 };
            break;
        case 106:
            bones = { 8, 7, 6, 25, 26, 27, 51, 52, 53, 79, 80, 82, 86, 87, 89 };
            break;
        case 104:
            bones = { 8, 7, 6, 25, 26, 27, 49, 50, 51, 77, 78, 80, 84, 85, 87 };
            break;
        case 103:
        default:
            bones = { 8, 7, 6, 25, 26, 27, 50, 51, 52, 77, 78, 80, 84, 85, 87 };
            break;
        }

        fvector wHead = mesh->get_bone_location(bones.head);
        if (!wHead.is_valid()) return;
        fvector wNeck = mesh->get_bone_location(bones.neck);
        fvector wChest = mesh->get_bone_location(bones.chest);
        if (!wNeck.is_valid() || !wChest.is_valid()) return;

        float head_to_chest = sqrtf(
            (float)((wHead.x - wChest.x) * (wHead.x - wChest.x) +
                    (wHead.y - wChest.y) * (wHead.y - wChest.y) +
                    (wHead.z - wChest.z) * (wHead.z - wChest.z)));
        if (head_to_chest < 5.0f) return;

        auto get_valid_bone = [&](int bone_id, fvector& out_world) -> bool {
            out_world = mesh->get_bone_location(bone_id);
            if (!out_world.is_valid()) return false;
            float dx = (float)(out_world.x - wHead.x);
            float dy = (float)(out_world.y - wHead.y);
            float dz = (float)(out_world.z - wHead.z);
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist > head_to_chest * 15.0f) return false;
            return true;
        };

        fvector wLS, wLE, wLH, wRS, wRE, wRH;
        fvector wLT, wLK, wLF, wRT, wRK, wRF;
        bool ok_ls = get_valid_bone(bones.l_shoulder, wLS);
        bool ok_le = get_valid_bone(bones.l_elbow, wLE);
        bool ok_lh = get_valid_bone(bones.l_hand, wLH);
        bool ok_rs = get_valid_bone(bones.r_shoulder, wRS);
        bool ok_re = get_valid_bone(bones.r_elbow, wRE);
        bool ok_rh = get_valid_bone(bones.r_hand, wRH);
        bool ok_lt = get_valid_bone(bones.l_thigh, wLT);
        bool ok_lk = get_valid_bone(bones.l_knee, wLK);
        bool ok_lf = get_valid_bone(bones.l_foot, wLF);
        bool ok_rt = get_valid_bone(bones.r_thigh, wRT);
        bool ok_rk = get_valid_bone(bones.r_knee, wRK);
        bool ok_rf = get_valid_bone(bones.r_foot, wRF);

        int valid_count = (ok_ls?1:0) + (ok_le?1:0) + (ok_rs?1:0) + (ok_re?1:0) +
                          (ok_lt?1:0) + (ok_lk?1:0) + (ok_rt?1:0) + (ok_rk?1:0);
        if (valid_count < 4) return;

        auto to_screen = [&](const fvector& world, fvector2d& out) -> bool {
            return controller->project_world_location_to_screen(world, out, false) && out.is_valid();
        };

        fvector2d sHead, sNeck, sChest;
        if (!to_screen(wHead, sHead)) return;
        bool s_neck = to_screen(wNeck, sNeck);
        bool s_chest = to_screen(wChest, sChest);
        if (!s_neck || !s_chest) return;

        fvector2d sLS, sLE, sLH, sRS, sRE, sRH;
        fvector2d sLT, sLK, sLF, sRT, sRK, sRF;
        ok_ls = ok_ls && to_screen(wLS, sLS);
        ok_le = ok_le && to_screen(wLE, sLE);
        ok_lh = ok_lh && to_screen(wLH, sLH);
        ok_rs = ok_rs && to_screen(wRS, sRS);
        ok_re = ok_re && to_screen(wRE, sRE);
        ok_rh = ok_rh && to_screen(wRH, sRH);
        ok_lt = ok_lt && to_screen(wLT, sLT);
        ok_lk = ok_lk && to_screen(wLK, sLK);
        ok_lf = ok_lf && to_screen(wLF, sLF);
        ok_rt = ok_rt && to_screen(wRT, sRT);
        ok_rk = ok_rk && to_screen(wRK, sRK);
        ok_rf = ok_rf && to_screen(wRF, sRF);

        fvector2d sPelvis;
        if (ok_lt && ok_rt) {
            sPelvis.x = (sLT.x + sRT.x) * 0.5;
            sPelvis.y = (sLT.y + sRT.y) * 0.5;
        } else if (ok_lt) {
            sPelvis = sLT;
        } else if (ok_rt) {
            sPelvis = sRT;
        } else {
            sPelvis = sChest;
        }
        bool ok_pelvis = ok_lt || ok_rt;

        float neck_chest_dist = sqrtf(
            (float)((sNeck.x - sChest.x) * (sNeck.x - sChest.x) +
                    (sNeck.y - sChest.y) * (sNeck.y - sChest.y)));
        if (neck_chest_dist < 2.0f) return;

        float pad = globals::visuals::mosca_padding;
        float br = neck_chest_dist * pad * 0.20f;
        if (br < 1.5f) br = 1.5f;

        float head_r = neck_chest_dist * 0.7f;
        if (head_r < 4.0f) head_r = 4.0f;
        float head_cx = (float)sHead.x;
        float head_cy = (float)sHead.y - head_r * 0.6f;

        auto cap_len = [](const fvector2d& a, const fvector2d& b) -> float {
            float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y);
            return sqrtf(dx * dx + dy * dy);
        };

        auto cap_radius = [&](const fvector2d& a, const fvector2d& b, float mult) -> float {
            float len = cap_len(a, b);
            float r = len * mult;
            if (r < br * 0.35f) r = br * 0.35f;
            if (r > br * 2.2f) r = br * 2.2f;
            return r;
        };

        struct Capsule { fvector2d a, b; float radius; bool valid; };
        Capsule capsules[] = {
            { sHead,  sNeck,  head_r * 0.45f,                          true },
            { sNeck,  sChest, cap_radius(sNeck, sChest, 0.50f),       true },
            { sChest, sPelvis,cap_radius(sChest, sPelvis, 0.38f),     ok_pelvis },

            { sChest, sLS,    cap_radius(sChest, sLS, 0.42f),         ok_ls },
            { sLS,    sLE,    cap_radius(sLS, sLE, 0.30f),            ok_ls && ok_le },
            { sLE,    sLH,    cap_radius(sLE, sLH, 0.22f),           ok_le && ok_lh },

            { sChest, sRS,    cap_radius(sChest, sRS, 0.42f),         ok_rs },
            { sRS,    sRE,    cap_radius(sRS, sRE, 0.30f),            ok_rs && ok_re },
            { sRE,    sRH,    cap_radius(sRE, sRH, 0.22f),           ok_re && ok_rh },

            { sPelvis,sLT,    cap_radius(sPelvis, sLT, 0.28f),        ok_pelvis && ok_lt },
            { sLT,    sLK,    cap_radius(sLT, sLK, 0.22f),           ok_lt && ok_lk },
            { sLK,    sLF,    cap_radius(sLK, sLF, 0.18f),           ok_lk && ok_lf },

            { sPelvis,sRT,    cap_radius(sPelvis, sRT, 0.28f),        ok_pelvis && ok_rt },
            { sRT,    sRK,    cap_radius(sRT, sRK, 0.22f),           ok_rt && ok_rk },
            { sRK,    sRF,    cap_radius(sRK, sRF, 0.18f),           ok_rk && ok_rf },
        };

        float screen_height = 0;
        for (int i = 0; i < (int)(sizeof(capsules)/sizeof(capsules[0])); i++) {
            if (!capsules[i].valid) continue;
            float ya = fabsf((float)(capsules[i].a.y - head_cy));
            float yb = fabsf((float)(capsules[i].b.y - head_cy));
            if (ya > screen_height) screen_height = ya;
            if (yb > screen_height) screen_height = yb;
        }
        if (screen_height < 5.f) return;
        float scale = screen_height / 120.f;
        if (scale < 0.3f) scale = 0.3f;
        if (scale > 5.0f) scale = 5.0f;
        int cap_cnt = sizeof(capsules) / sizeof(capsules[0]);
        const float leg_zone_y = (float)sPelvis.y + br * 0.2f;

        struct Interval { float l, r; };

        auto scanline = [&](float y) -> std::vector<Interval> {
            std::vector<Interval> raw;
            for (int ci = 0; ci < cap_cnt; ci++) {
                if (!capsules[ci].valid) continue;
                fvector2d ca = capsules[ci].a;
                fvector2d cb = capsules[ci].b;
                float r = capsules[ci].radius;
                if (r < 0.5f) continue;
                if (y < min(ca.y, cb.y) - r || y > max(ca.y, cb.y) + r) continue;

                float mn = 99999.f, mx = -99999.f;
                float dyA = y - (float)ca.y;
                if (fabsf(dyA) <= r) { float hw = sqrtf(r * r - dyA * dyA); mn = min(mn, (float)ca.x - hw); mx = max(mx, (float)ca.x + hw); }
                float dyB = y - (float)cb.y;
                if (fabsf(dyB) <= r) { float hw = sqrtf(r * r - dyB * dyB); mn = min(mn, (float)cb.x - hw); mx = max(mx, (float)cb.x + hw); }
                float dx = (float)cb.x - (float)ca.x, dy = (float)cb.y - (float)ca.y;
                float len = sqrtf(dx * dx + dy * dy);
                if (len > 0.001f && fabsf(dy) > 0.001f) {
                    float nx = -dy / len, ny = dx / len;
                    float t1 = (y - ((float)ca.y + r * ny)) / dy;
                    if (t1 >= 0.f && t1 <= 1.f) { float ix = ((float)ca.x + r * nx) + t1 * dx; mn = min(mn, ix); mx = max(mx, ix); }
                    float t2 = (y - ((float)ca.y - r * ny)) / dy;
                    if (t2 >= 0.f && t2 <= 1.f) { float ix = ((float)ca.x - r * nx) + t2 * dx; mn = min(mn, ix); mx = max(mx, ix); }
                } else if (len > 0.001f) {
                    if (y >= (float)ca.y - r && y <= (float)ca.y + r) {
                        float hw = sqrtf(r * r - (y - (float)ca.y) * (y - (float)ca.y));
                        mn = min(mn, min((float)ca.x, (float)cb.x) - hw);
                        mx = max(mx, max((float)ca.x, (float)cb.x) + hw);
                    }
                }
                if (mn <= mx) raw.push_back({ mn, mx });
            }
            float hd = fabsf(y - head_cy);
            if (hd < head_r) { float hw = sqrtf(head_r * head_r - hd * hd); raw.push_back({ head_cx - hw, head_cx + hw }); }

            if (raw.empty()) return {};
            std::sort(raw.begin(), raw.end(), [](const Interval& a, const Interval& b) { return a.l < b.l; });
            std::vector<Interval> merged;
            float merge_gap = (y > leg_zone_y) ? (br * 0.03f) : (br * 0.12f);
            Interval cur = raw[0];
            for (size_t i = 1; i < raw.size(); i++) {
                float gap = raw[i].l - cur.r;
                if (gap <= merge_gap) { if (raw[i].r > cur.r) cur.r = raw[i].r; }
                else { merged.push_back(cur); cur = raw[i]; }
            }
            merged.push_back(cur);
            return merged;
        };

        int min_y_v = (int)(head_cy - head_r);
        int max_y_v = (int)(head_cy + head_r);
        for (int i = 0; i < cap_cnt; i++) {
            if (!capsules[i].valid) continue;
            float r = capsules[i].radius;
            int a1 = (int)(capsules[i].a.y - r), a2 = (int)(capsules[i].a.y + r);
            int b1 = (int)(capsules[i].b.y - r), b2 = (int)(capsules[i].b.y + r);
            if (a1 < min_y_v) min_y_v = a1; if (b1 < min_y_v) min_y_v = b1;
            if (a2 > max_y_v) max_y_v = a2; if (b2 > max_y_v) max_y_v = b2;
        }
        if (min_y_v < 0) min_y_v = 0;
        if (max_y_v > 4000) max_y_v = 4000;
        if (max_y_v <= min_y_v) return;
        int y_range = max_y_v - min_y_v;
        if (y_range > 3000 || y_range < 3) return;

        int step = (int)(y_range / 200.f);
        if (step < 1) step = 1;
        if (step > 3) step = 3;

        float jump_thr = globals::visuals::mosca_jump_threshold * scale;
        if (jump_thr < 8.f) jump_thr = 8.f;

        struct DrawCmd { fvector2d a, b; };
        std::vector<DrawCmd> cmds;
        cmds.reserve(y_range / step * 4);

        {
            std::vector<Interval> prev;
            float py = -1.f;
            for (int j = 0; j <= y_range; j += step) {
                float y = (float)(min_y_v + j);
                std::vector<Interval> cur = scanline(y);
                if (j > 0 && py >= 0.f && !cur.empty() && !prev.empty()) {
                    int np = (int)prev.size(), nc = (int)cur.size();
                    for (int c = 0; c < nc; c++) {
                        float bd = jump_thr; int bi = -1;
                        for (int p = 0; p < np; p++) {
                            if (prev[p].r < cur[c].l || prev[p].l > cur[c].r) continue;
                            float d = fabsf(cur[c].l - prev[p].l);
                            if (d < bd) { bd = d; bi = p; }
                        }
                        if (bi != -1) cmds.push_back({ { prev[bi].l, py }, { cur[c].l, y } });
                    }
                    for (int c = 0; c < nc; c++) {
                        float bd = jump_thr; int bi = -1;
                        for (int p = 0; p < np; p++) {
                            if (prev[p].r < cur[c].l || prev[p].l > cur[c].r) continue;
                            float d = fabsf(cur[c].r - prev[p].r);
                            if (d < bd) { bd = d; bi = p; }
                        }
                        if (bi != -1) cmds.push_back({ { prev[bi].r, py }, { cur[c].r, y } });
                    }
                    for (int c = 0; c < nc; c++) {
                        bool ov = false;
                        for (int p = 0; p < np; p++) { if (max(cur[c].l, prev[p].l) <= min(cur[c].r, prev[p].r)) { ov = true; break; } }
                        if (!ov) cmds.push_back({ { cur[c].l, y }, { cur[c].r, y } });
                    }
                    for (int p = 0; p < np; p++) {
                        bool ov = false;
                        for (int c = 0; c < nc; c++) { if (max(prev[p].l, cur[c].l) <= min(prev[p].r, cur[c].r)) { ov = true; break; } }
                        if (!ov) cmds.push_back({ { prev[p].l, py }, { prev[p].r, py } });
                    }
                    if (y < leg_zone_y) {
                        for (int c = 0; c < nc - 1; c++) {
                            float gap_l = cur[c].r;
                            float gap_r = cur[c + 1].l;
                            for (int p = 0; p < np; p++) {
                                if (prev[p].l <= gap_l + 2.f && prev[p].r >= gap_r - 2.f) {
                                    cmds.push_back({ { gap_l, y }, { gap_r, y } }); break;
                                }
                            }
                        }
                    }
                }
                prev = cur;
                py = y;
            }
            if (!prev.empty()) {
                for (size_t i = 0; i < prev.size(); i++)
                    cmds.push_back({ { prev[i].l, py }, { prev[i].r, py } });
            }
        }

        if (cmds.empty()) return;

        float core_thick = 0.8f + line_thickness * 0.3f;
        if (core_thick > 2.0f) core_thick = 2.0f;

        for (int g = 4; g >= 1; g--) {
            float t = (float)g / 5.f;
            float glow_thick = core_thick + (float)g * 1.1f * scale;
            float alpha = outline_color.a * 0.08f * (1.f - t * 0.5f);
            flinearcolor gc = flinearcolor(outline_color.r, outline_color.g, outline_color.b, alpha);
            for (size_t i = 0; i < cmds.size(); i++)
                canvas->k2_drawline(cmds[i].a, cmds[i].b, glow_thick, gc);
        }

        flinearcolor core_col = flinearcolor(outline_color.r, outline_color.g, outline_color.b, outline_color.a * 0.95f);
        for (size_t i = 0; i < cmds.size(); i++)
            canvas->k2_drawline(cmds[i].a, cmds[i].b, core_thick, core_col);
    }




    namespace G
    {
        currentequippable* MyWeapon = nullptr;
        currentequippable* LastWeapon = nullptr;
    }




    template<class k, class e>
    class tmap
    {
    public:
        k Key;
        e Element;
        char __pad0x[0x8];
    };
    inline fstring BuddyName;
    inline uobject* buddy;



    static flinearcolor Name_Color{ 1.f,1.f,1.f,1.f };
    float RainbowTime = 0.0f;
    const float RainbowSpeed = 1.0f;
    const float PI = 3.14159265359f;

    flinearcolor convert_to_flinearcolor(int r, int g, int b, int a) {
        return flinearcolor(
            (float)r / 255.0f,
            (float)g / 255.0f,
            (float)b / 255.0f,
            (float)a / 255.0f
        );
    }

    int index = 453;


    std::wstring to_wide_string(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }

    static bool bFlickSilent = true;
    static bool bLockedCameraRotation = false;

    fvector2d head_scren;

    double screen_center_x;
    double screen_center_y;

    fvector	LocalCameraLocation;
    float LocalCameraFOV;
    fvector	LocalCameraRotation;

    static bool first_location = true;
    static bool aim_check = false;
    static bool second_locked_camera = false;
    static bool finished_hook = false;
    static fvector first_camera_location;
    static fvector first_camera_rotation;

    static fvector saved_client_view;

    void(*SetCameraCachePOVOriginal)(uintptr_t, FMinimalViewInfo*) = nullptr;




    static bool aimbot_key_pressed_last_frame = false;





    namespace helper {
        fstring convert_weapon_name(fstring weapon_name)
        {
            std::wstring weapon_name_str = weapon_name.wide();

            if (weapon_name_str.find(L"Ability_Melee_Base_C") != std::wstring::npos)
                return L"Melee";
            else if (weapon_name_str.find(L"BasePistol_C") != std::wstring::npos)
                return L"Classic";
            else if (weapon_name_str.find(L"SawedOffShotgun_C") != std::wstring::npos)
                return L"Shorty";
            else if (weapon_name_str.find(L"AutomaticPistol_C") != std::wstring::npos)
                return L"Frenzy";
            else if (weapon_name_str.find(L"LugerPistol_C") != std::wstring::npos)
                return L"Ghost";
            else if (weapon_name_str.find(L"RevolverPistol_C") != std::wstring::npos)
                return L"Sheriff";
            else if (weapon_name_str.find(L"Vector_C") != std::wstring::npos)
                return L"Stinger";
            else if (weapon_name_str.find(L"SubMachineGun_MP5") != std::wstring::npos)
                return L"Spectre";
            else if (weapon_name_str.find(L"PumpShotgun_C") != std::wstring::npos)
                return L"Bucky";
            else if (weapon_name_str.find(L"AutomaticShotgun_C") != std::wstring::npos)
                return L"Judge";
            else if (weapon_name_str.find(L"AssaultRifle_Burst_C") != std::wstring::npos)
                return L"Bulldog";
            else if (weapon_name_str.find(L"DMR_C") != std::wstring::npos)
                return L"Guardian";
            else if (weapon_name_str.find(L"AssaultRifle_ACR_C") != std::wstring::npos)
                return L"Phantom";
            else if (weapon_name_str.find(L"AssaultRifle_AK_C") != std::wstring::npos)
                return L"Vandal";
            else if (weapon_name_str.find(L"LeverSniperRifle_C") != std::wstring::npos)
                return L"Marshal";
            else if (weapon_name_str.find(L"BoltSniper_C") != std::wstring::npos)
                return L"Operator";
            else if (weapon_name_str.find(L"LightMachineGun_C") != std::wstring::npos)
                return L"Ares";
            else if (weapon_name_str.find(L"HeavyMachineGun_C") != std::wstring::npos)
                return L"Odin";
            else if (weapon_name_str.find(L"Gun_Deadeye_Q_Pistol_C") != std::wstring::npos)
                return L"Headhunter";
            else if (weapon_name_str.find(L"Ability_Wushu_X_Dagger_Production_C") != std::wstring::npos)
                return L"Blade storm";
            else if (weapon_name_str.find(L"Gun_Sprinter_X_HeavyLightningGun_Production_C") != std::wstring::npos)
                return L"Overdrive";
            else if (weapon_name_str.find(L"DS_Gun_C") != std::wstring::npos)
                return L"Outlaw";
            else if (weapon_name_str.find(L"Gun_Deadeye_X_Giantslayer_Prototype_C") != std::wstring::npos)
                return L"Tour de force";
            return L"Invalid";
        }
    }





    static fvector seh_calc_spread_inner(ashootercharacter* actor, uint64_t firing_state_component, currentequippable* weapon, fvector direction);

    static fvector calc_spread(ashootercharacter* actor, uint64_t firing_state_component, currentequippable* weapon, fvector direction)
    {
        __try {
            return seh_calc_spread_inner(actor, firing_state_component, weapon, direction);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("calc_spread: EXCEPTION caught inside spread calculation");
            return fvector(0, 0, 0);
        }
    }

    static fvector seh_calc_spread_inner(ashootercharacter* actor, uint64_t firing_state_component, currentequippable* weapon, fvector direction)
    {
        if (!actor || !memory::IsValidPointer((uintptr_t)actor) || !firing_state_component || !memory::IsValidPointer(firing_state_component) || !weapon || !memory::IsValidPointer((uintptr_t)weapon))
            return fvector(0, 0, 0);


        uint64_t stability_component = memory::read<uint64_t>(firing_state_component + offsets::stability_component);
        if (!stability_component || !memory::IsValidPointer(stability_component))
            return fvector(0, 0, 0);


        alignas(16) static uint8_t error_values[4096];
        alignas(16) static uint8_t seed_data_snapshot[4096];
        alignas(16) static uint8_t spread_angles[4096];
        alignas(16) static uint8_t out_spread_angles[4096];


        static auto func1_fn = (float* (__fastcall*)(uint64_t, float*))(memory::module_base + offsets::get_spread_values);
        static auto func2_fn = (void(__fastcall*)(uint64_t, fvector*, float, float, int, int, uint64_t))(memory::module_base + offsets::get_spread_angles);
        static auto func3_fn = (fvector * (__fastcall*)(fvector*, fvector*))(memory::module_base + offsets::to_vector_and_normalize);
        static auto func4_fn = (fvector * (__fastcall*)(fvector*, fvector*))(memory::module_base + offsets::to_angle_and_normalize);
        static auto func5_fn = (uintptr_t(__fastcall*)(__int64, float*))(memory::module_base + offsets::get_spread_values);


        *(uint64_t*)(&out_spread_angles[0]) = (uint64_t)&spread_angles[0];
        *(int*)(&out_spread_angles[0] + 8) = 1;
        *(int*)(&out_spread_angles[0] + 12) = 1;


        uint64_t seed_data = memory::read<uint64_t>(firing_state_component + offsets::seed_data);
        if (!seed_data || !memory::IsValidPointer(seed_data))
            return fvector(0, 0, 0);


        memcpy((void*)seed_data_snapshot, (void*)seed_data, sizeof(seed_data_snapshot));


        reinterpret_cast<float* (__cdecl*)(uint64_t, float*, uintptr_t, void*)>(
            spoofcall_stub)(stability_component, (float*)&error_values[0], 0x46C4660, func1_fn);

        fvector temp1, temp2 = fvector(0, 0, 0);
        fvector previous_firing_direction, firing_direction = fvector(0, 0, 0);


        actor->get_firing_location_and_direction(&temp1, &previous_firing_direction, false);


        reinterpret_cast<fvector* (__cdecl*)(fvector*, fvector*, uintptr_t, void*)>(
            spoofcall_stub)(&previous_firing_direction, &temp2, 0x46C4660, func3_fn);
        reinterpret_cast<fvector* (__cdecl*)(fvector*, fvector*, uintptr_t, void*)>(
            spoofcall_stub)(&temp2, &temp1, 0x46C4660, func4_fn);

        previous_firing_direction = temp1;


        temp1.x += *(float*)(&error_values[0] + 12);
        temp1.y += *(float*)(&error_values[0] + 16);


        reinterpret_cast<fvector* (__cdecl*)(fvector*, fvector*, uintptr_t, void*)>(
            spoofcall_stub)(&temp1, &firing_direction, 0x46C4660, func3_fn);


        float test[20];
        uintptr_t v38 = reinterpret_cast<uintptr_t(__cdecl*)(__int64, float*, uintptr_t, void*)>(
            spoofcall_stub)(stability_component, test, 0x46C4660, func5_fn);

        if (!v38 || !memory::IsValidPointer(v38))
            return fvector(0, 0, 0);

        float v46 = memory::read<float>(v38 + 0x14);
        float v48 = *(float*)(&error_values[0] + 8) + *(float*)(&error_values[0] + 4);
        int error_retries = memory::read<int>(firing_state_component + offsets::error_retries);


        reinterpret_cast<void(__cdecl*)(
            uint64_t, fvector*, float, float, int, int, uint64_t, uintptr_t, void*)>(
                spoofcall_stub)(
                    ((uint64_t)&seed_data_snapshot[0]) + 0xD8, &firing_direction,
                    v48, v46, error_retries, 1,
                    (uint64_t)&out_spread_angles[0], 0x46C4660, func2_fn);


        fvector spread_vector = *(fvector*)(&spread_angles[0]);

        reinterpret_cast<fvector* (__cdecl*)(fvector*, fvector*, uintptr_t, void*)>(
            spoofcall_stub)(&spread_vector, &firing_direction, 0x46C4660, func4_fn);

        return firing_direction - previous_firing_direction;
    }

    void angle_rotation(const fvector& angles, fvector* forward)
    {
        float	sp, sy, cp, cy;

        sy = sin(DegreeToRadian(angles.y));
        cy = cos(DegreeToRadian(angles.y));

        sp = sin(DegreeToRadian(angles.x));
        cp = cos(DegreeToRadian(angles.x));

        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    }

    fvector RotationToVector(const fvector& rotation) {
        const double DEG_TO_RAD = 0.017453292519943295f;

        double sy = sinf(rotation.y * DEG_TO_RAD);
        double cy = cosf(rotation.y * DEG_TO_RAD);
        double sp = sinf(rotation.x * DEG_TO_RAD);
        double cp = cosf(rotation.x * DEG_TO_RAD);

        return fvector(cp * cy, cp * sy, -sp);
    }

    inline float clamp_value(float value, float min_val, float max_val) {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }









    void apply_outline_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) return;

        globals::chams::apply_visible_preset();
        globals::chams::apply_invisible_preset();

        auto is_visible = controllers->line_of_sight(actor);

        float glowIntensity;
        flinearcolor centerEdgeColor, innerEdgeColor, outerEdgeColor;

        if (is_visible) {
            centerEdgeColor = flinearcolor(globals::chams::CenterEdgeR_Visible, globals::chams::CenterEdgeG_Visible, globals::chams::CenterEdgeB_Visible, globals::chams::intensityvisibleoutline);
            innerEdgeColor = flinearcolor(globals::chams::InnerEdgeR_Visible, globals::chams::InnerEdgeG_Visible, globals::chams::InnerEdgeB_Visible, globals::chams::intensityvisibleoutline);
            outerEdgeColor = flinearcolor(globals::chams::OuterEdgeR_Visible, globals::chams::OuterEdgeG_Visible, globals::chams::OuterEdgeB_Visible, globals::chams::intensityvisibleoutline);
            glowIntensity = globals::chams::GlowVisible;
        }
        else {
            centerEdgeColor = flinearcolor(globals::chams::CenterEdgeR_Invisible, globals::chams::CenterEdgeG_Invisible, globals::chams::CenterEdgeB_Invisible, globals::chams::intensityinvisbleoutline);
            innerEdgeColor = flinearcolor(globals::chams::InnerEdgeR_Invisible, globals::chams::InnerEdgeG_Invisible, globals::chams::InnerEdgeB_Invisible, globals::chams::intensityinvisbleoutline);
            outerEdgeColor = flinearcolor(globals::chams::OuterEdgeR_Invisible, globals::chams::OuterEdgeG_Invisible, globals::chams::OuterEdgeB_Invisible, globals::chams::intensityinvisbleoutline);
            glowIntensity = globals::chams::GlowInvisible;
        }

        static fname silohuette_color_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name, glow_intensity_param;
        if (!silohuette_color_name.comparison_index) {
            silohuette_color_name = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
            glow_intensity_param = string::string_to_name(L"GlowIntensity");
        }

        uobject* visible_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );

        uobject* invisible_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/VFX/Materials/HunterReveal_MI.HunterReveal_MI"
        );

        if (!visible_material || !memory::IsValidPointer((uintptr_t)visible_material) ||
            !invisible_material || !memory::IsValidPointer((uintptr_t)invisible_material)) return;

        if (globals::chams::outlinetype == 0 || !is_visible)
        {
            auto main_mesh = actor->get_mesh();
            if (main_mesh && memory::IsValidPointer((uintptr_t)main_mesh)) {
                auto num_materials = main_mesh->get_num_materials();
                if (num_materials > 0 && num_materials <= 64) {
                for (int i = 0; i < num_materials; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)main_mesh, i, is_visible ? visible_material : invisible_material);

                    if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                    auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                    dynCast->set_vector_parameter_value1(silohuette_color_name, outerEdgeColor);
                    dynCast->set_vector_parameter_value1(center_edge_color_name, centerEdgeColor);
                    dynCast->set_vector_parameter_value1(inner_edge_color_name, innerEdgeColor);
                    dynCast->set_vector_parameter_value1(outer_edge_color_name, outerEdgeColor);
                    dynCast->set_scalar_parameter_value(glow_intensity_param, glowIntensity);
                }
                }
            }


            uskeletalmeshcomponent* mesh_cosmetic_3p = actor->GetCosmeticMesh3P();
            if (mesh_cosmetic_3p && memory::IsValidPointer((uintptr_t)mesh_cosmetic_3p)) {
                auto num_materials = mesh_cosmetic_3p->get_num_materials();
                if (num_materials > 0 && num_materials <= 64) {
                for (int i = 0; i < num_materials; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh_cosmetic_3p, i, is_visible ? visible_material : invisible_material);

                    if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                    auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                    dynCast->set_vector_parameter_value1(silohuette_color_name, outerEdgeColor);
                    dynCast->set_vector_parameter_value1(center_edge_color_name, centerEdgeColor);
                    dynCast->set_vector_parameter_value1(inner_edge_color_name, innerEdgeColor);
                    dynCast->set_vector_parameter_value1(outer_edge_color_name, outerEdgeColor);
                    dynCast->set_scalar_parameter_value(glow_intensity_param, glowIntensity);
                }
                }
            }
        }
        else if (globals::chams::outlinetype == 1 && is_visible)
        {
            auto main_mesh = actor->get_mesh();
            if (main_mesh) {
                actor->reset_character_materials_internal(main_mesh);
            }


            uskeletalmeshcomponent* mesh_cosmetic_3p = actor->GetCosmeticMesh3P();
            if (mesh_cosmetic_3p) {
                actor->reset_character_materials_internal(mesh_cosmetic_3p);
            }
        }
    }

    void apply_hand_outline_chams(acknowledgedpawn* pawn)
    {
        if (!pawn) return;

        globals::chams::apply_visible_preset();

        flinearcolor centerEdgeColor(
            globals::chams::CenterEdgeR_Visible,
            globals::chams::CenterEdgeG_Visible,
            globals::chams::CenterEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );
        flinearcolor innerEdgeColor(
            globals::chams::InnerEdgeR_Visible,
            globals::chams::InnerEdgeG_Visible,
            globals::chams::InnerEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );
        flinearcolor outerEdgeColor(
            globals::chams::OuterEdgeR_Visible,
            globals::chams::OuterEdgeG_Visible,
            globals::chams::OuterEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );

        static fname silohuette_color_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name, glow_intensity_param;
        if (!silohuette_color_name.comparison_index) {
            silohuette_color_name = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
            glow_intensity_param = string::string_to_name(L"GlowIntensity");
        }

        uobject* hand_outline_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );

        if (!hand_outline_material || !memory::IsValidPointer((uintptr_t)hand_outline_material)) return;

        auto apply_to_mesh = [&](uskeletalmeshcomponent* mesh)
            {
                if (!mesh) return;

                auto num_materials = mesh->get_num_materials();
                for (int i = 0; i < num_materials; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, hand_outline_material);
                    if (!material_instance_dynamic) continue;

                    auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dynCast) continue;

                    dynCast->set_vector_parameter_value1(silohuette_color_name, outerEdgeColor);
                    dynCast->set_vector_parameter_value1(center_edge_color_name, centerEdgeColor);
                    dynCast->set_vector_parameter_value1(inner_edge_color_name, innerEdgeColor);
                    dynCast->set_vector_parameter_value1(outer_edge_color_name, outerEdgeColor);
                    dynCast->set_scalar_parameter_value(glow_intensity_param, globals::chams::GlowVisible);
                }
            };

        apply_to_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p));
        apply_to_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay));
    }

    void apply_self_outline_chams(ashootercharacter* character)
    {
        if (!character) return;

        globals::chams::apply_visible_preset();

        flinearcolor centerEdgeColor(
            globals::chams::CenterEdgeR_Visible,
            globals::chams::CenterEdgeG_Visible,
            globals::chams::CenterEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );
        flinearcolor innerEdgeColor(
            globals::chams::InnerEdgeR_Visible,
            globals::chams::InnerEdgeG_Visible,
            globals::chams::InnerEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );
        flinearcolor outerEdgeColor(
            globals::chams::OuterEdgeR_Visible,
            globals::chams::OuterEdgeG_Visible,
            globals::chams::OuterEdgeB_Visible,
            globals::chams::intensityvisibleoutline
        );

        static fname silohuette_color_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name, glow_intensity_param;
        if (!silohuette_color_name.comparison_index) {
            silohuette_color_name = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
            glow_intensity_param = string::string_to_name(L"GlowIntensity");
        }

        uobject* self_outline_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );

        if (!self_outline_material || !memory::IsValidPointer((uintptr_t)self_outline_material)) return;

        auto apply_to_mesh = [&](uskeletalmeshcomponent* mesh)
            {
                if (!mesh) return;

                auto num_materials = mesh->get_num_materials();
                for (int i = 0; i < num_materials; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, self_outline_material);
                    if (!material_instance_dynamic) continue;

                    auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dynCast) continue;

                    dynCast->set_vector_parameter_value1(silohuette_color_name, outerEdgeColor);
                    dynCast->set_vector_parameter_value1(center_edge_color_name, centerEdgeColor);
                    dynCast->set_vector_parameter_value1(inner_edge_color_name, innerEdgeColor);
                    dynCast->set_vector_parameter_value1(outer_edge_color_name, outerEdgeColor);
                    dynCast->set_scalar_parameter_value(glow_intensity_param, globals::chams::GlowVisible);
                }
            };

        apply_to_mesh(character->get_mesh());
        apply_to_mesh(character->GetCosmeticMesh3P());
    }

    void ApplyCrystalChamsPreset(int preset) {
        switch (preset) {
        case 0: // Red Dark
            globals::visuals::Self_CenterEdgeR = 0.613636f;
            globals::visuals::Self_CenterEdgeG = 0.0f;
            globals::visuals::Self_CenterEdgeB = 0.170455f;
            globals::visuals::Self_InnerEdgeR = 1.32955f;
            globals::visuals::Self_InnerEdgeG = 0.0f;
            globals::visuals::Self_InnerEdgeB = 0.89f;
            globals::visuals::Self_OuterEdgeR = 9.64773f;
            globals::visuals::Self_OuterEdgeG = 11.64f;
            globals::visuals::Self_OuterEdgeB = 0.0f;

            globals::visuals::GlowVisible = 1.5f;
            globals::visuals::AlphaBasePower = 2.0f;
            globals::visuals::AlphaColorMult = 1.2f;
            globals::visuals::DepthBias = 0.1f;
            globals::visuals::AlphaDissolveOpacity = 0.8f;
            globals::visuals::BoundingBox = 1.0f;
            globals::visuals::InnerEdgeThickness = 0.3f;
            globals::visuals::OuterEdgeThickness = 0.2f;
            globals::visuals::RimFresnel = 2.5f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 12.2727f;
            globals::visuals::OcclusionDepth = 0.5f;
            globals::visuals::OcclusionBehindWall = 0.3f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.246591f;
            break;

        case 1: // Dark Green
            globals::visuals::Self_CenterEdgeR = 0.0f;
            globals::visuals::Self_CenterEdgeG = 0.545455f;
            globals::visuals::Self_CenterEdgeB = 0.170455f;
            globals::visuals::Self_InnerEdgeR = 1.32955f;
            globals::visuals::Self_InnerEdgeG = 0.0f;
            globals::visuals::Self_InnerEdgeB = 0.89f;
            globals::visuals::Self_OuterEdgeR = 9.64773f;
            globals::visuals::Self_OuterEdgeG = 9.95455f;
            globals::visuals::Self_OuterEdgeB = 0.0f;

            globals::visuals::GlowVisible = 1.5f;
            globals::visuals::AlphaBasePower = 2.0f;
            globals::visuals::AlphaColorMult = 1.2f;
            globals::visuals::DepthBias = 0.1f;
            globals::visuals::AlphaDissolveOpacity = 0.8f;
            globals::visuals::BoundingBox = 1.0f;
            globals::visuals::InnerEdgeThickness = 0.3f;
            globals::visuals::OuterEdgeThickness = 0.2f;
            globals::visuals::RimFresnel = 2.5f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 12.2727f;
            globals::visuals::OcclusionDepth = 0.5f;
            globals::visuals::OcclusionBehindWall = 0.3f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.246591f;
            break;

        case 2: // Dark Blue
            globals::visuals::Self_CenterEdgeR = 0.0f;
            globals::visuals::Self_CenterEdgeG = 0.0f;
            globals::visuals::Self_CenterEdgeB = 0.477273f;
            globals::visuals::Self_InnerEdgeR = 0.0340909f;
            globals::visuals::Self_InnerEdgeG = 0.0f;
            globals::visuals::Self_InnerEdgeB = 0.0f;
            globals::visuals::Self_OuterEdgeR = 0.0f;
            globals::visuals::Self_OuterEdgeG = 0.0f;
            globals::visuals::Self_OuterEdgeB = 0.0f;

            globals::visuals::GlowVisible = 1.5f;
            globals::visuals::AlphaBasePower = 2.0f;
            globals::visuals::AlphaColorMult = 1.2f;
            globals::visuals::DepthBias = 0.1f;
            globals::visuals::AlphaDissolveOpacity = 0.8f;
            globals::visuals::BoundingBox = 1.0f;
            globals::visuals::InnerEdgeThickness = 0.3f;
            globals::visuals::OuterEdgeThickness = 0.2f;
            globals::visuals::RimFresnel = 2.5f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 12.2727f;
            globals::visuals::OcclusionDepth = 0.5f;
            globals::visuals::OcclusionBehindWall = 0.3f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.246591f;
            break;

        case 3: // Dark Orange
            globals::visuals::Self_CenterEdgeR = 0.647727f;
            globals::visuals::Self_CenterEdgeG = 0.579545f;
            globals::visuals::Self_CenterEdgeB = 0.0f;
            globals::visuals::Self_InnerEdgeR = 0.511364f;
            globals::visuals::Self_InnerEdgeG = 0.27f;
            globals::visuals::Self_InnerEdgeB = 1.0f;
            globals::visuals::Self_OuterEdgeR = 0.04f;
            globals::visuals::Self_OuterEdgeG = 0.23f;
            globals::visuals::Self_OuterEdgeB = 0.21f;

            globals::visuals::GlowVisible = 200.0f;
            globals::visuals::AlphaBasePower = 0.806818f;
            globals::visuals::AlphaColorMult = 0.515227f;
            globals::visuals::DepthBias = 0.106818f;
            globals::visuals::AlphaDissolveOpacity = 0.207412f;
            globals::visuals::BoundingBox = -50.0f;
            globals::visuals::InnerEdgeThickness = 0.1f;
            globals::visuals::OuterEdgeThickness = 0.37f;
            globals::visuals::RimFresnel = 1.0f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 20.0f;
            globals::visuals::OcclusionDepth = 0.0f;
            globals::visuals::OcclusionBehindWall = 1.24545f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.0f;
            break;

        case 4: // Orange
            globals::visuals::Self_CenterEdgeR = 0.681818f;
            globals::visuals::Self_CenterEdgeG = 0.579545f;
            globals::visuals::Self_CenterEdgeB = 0.0f;
            globals::visuals::Self_InnerEdgeR = 0.511364f;
            globals::visuals::Self_InnerEdgeG = 0.27f;
            globals::visuals::Self_InnerEdgeB = 1.0f;
            globals::visuals::Self_OuterEdgeR = 0.04f;
            globals::visuals::Self_OuterEdgeG = 0.23f;
            globals::visuals::Self_OuterEdgeB = 0.21f;

            globals::visuals::GlowVisible = 200.0f;
            globals::visuals::AlphaBasePower = 0.806818f;
            globals::visuals::AlphaColorMult = 0.515227f;
            globals::visuals::DepthBias = 0.106818f;
            globals::visuals::AlphaDissolveOpacity = 0.207412f;
            globals::visuals::BoundingBox = -50.0f;
            globals::visuals::InnerEdgeThickness = 0.1f;
            globals::visuals::OuterEdgeThickness = 0.37f;
            globals::visuals::RimFresnel = 1.0f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 20.0f;
            globals::visuals::OcclusionDepth = 0.0f;
            globals::visuals::OcclusionBehindWall = 1.24545f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.0f;
            break;

        case 5: // Pink
            globals::visuals::Self_CenterEdgeR = 0.647727f;
            globals::visuals::Self_CenterEdgeG = 0.545455f;
            globals::visuals::Self_CenterEdgeB = 0.511364f;
            globals::visuals::Self_InnerEdgeR = 0.511364f;
            globals::visuals::Self_InnerEdgeG = 0.27f;
            globals::visuals::Self_InnerEdgeB = 1.0f;
            globals::visuals::Self_OuterEdgeR = 0.04f;
            globals::visuals::Self_OuterEdgeG = 0.23f;
            globals::visuals::Self_OuterEdgeB = 0.21f;

            globals::visuals::GlowVisible = 200.0f;
            globals::visuals::AlphaBasePower = 0.806818f;
            globals::visuals::AlphaColorMult = 0.515227f;
            globals::visuals::DepthBias = 0.106818f;
            globals::visuals::AlphaDissolveOpacity = 0.207412f;
            globals::visuals::BoundingBox = -50.0f;
            globals::visuals::InnerEdgeThickness = 0.1f;
            globals::visuals::OuterEdgeThickness = 0.37f;
            globals::visuals::RimFresnel = 1.0f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 20.0f;
            globals::visuals::OcclusionDepth = 0.0f;
            globals::visuals::OcclusionBehindWall = 1.24545f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.0f;
            break;

        case 6: // Black Galaxy
            globals::visuals::Self_CenterEdgeR = 0.647727f;
            globals::visuals::Self_CenterEdgeG = 0.579545f;
            globals::visuals::Self_CenterEdgeB = 0.545455f;
            globals::visuals::Self_InnerEdgeR = 0.511364f;
            globals::visuals::Self_InnerEdgeG = 0.27f;
            globals::visuals::Self_InnerEdgeB = 1.0f;
            globals::visuals::Self_OuterEdgeR = 0.04f;
            globals::visuals::Self_OuterEdgeG = 0.23f;
            globals::visuals::Self_OuterEdgeB = 0.21f;

            globals::visuals::GlowVisible = 200.0f;
            globals::visuals::AlphaBasePower = 0.806818f;
            globals::visuals::AlphaColorMult = 0.515227f;
            globals::visuals::DepthBias = 0.106818f;
            globals::visuals::AlphaDissolveOpacity = 0.207412f;
            globals::visuals::BoundingBox = -50.0f;
            globals::visuals::InnerEdgeThickness = 0.1f;
            globals::visuals::OuterEdgeThickness = 0.37f;
            globals::visuals::RimFresnel = 1.0f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 20.0f;
            globals::visuals::OcclusionDepth = 0.0f;
            globals::visuals::OcclusionBehindWall = 1.24545f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.0f;
            break;

        case 7: // Purple
            globals::visuals::Self_CenterEdgeR = 0.613636f;
            globals::visuals::Self_CenterEdgeG = 0.511364f;
            globals::visuals::Self_CenterEdgeB = 0.545455f;
            globals::visuals::Self_InnerEdgeR = 0.511364f;
            globals::visuals::Self_InnerEdgeG = 0.27f;
            globals::visuals::Self_InnerEdgeB = 1.0f;
            globals::visuals::Self_OuterEdgeR = 0.04f;
            globals::visuals::Self_OuterEdgeG = 0.23f;
            globals::visuals::Self_OuterEdgeB = 0.21f;

            globals::visuals::GlowVisible = 200.0f;
            globals::visuals::AlphaBasePower = 0.806818f;
            globals::visuals::AlphaColorMult = 0.515227f;
            globals::visuals::DepthBias = 0.106818f;
            globals::visuals::AlphaDissolveOpacity = 0.207412f;
            globals::visuals::BoundingBox = -50.0f;
            globals::visuals::InnerEdgeThickness = 0.1f;
            globals::visuals::OuterEdgeThickness = 0.37f;
            globals::visuals::RimFresnel = 1.0f;
            globals::visuals::RimMultiply = 1.0f;
            globals::visuals::RimPower = 20.0f;
            globals::visuals::OcclusionDepth = 0.0f;
            globals::visuals::OcclusionBehindWall = 1.24545f;
            globals::visuals::OcclusionState = 1.0f;
            globals::visuals::RefractionDepthBias = 0.0f;
            break;
        }
    }

    void ApplyEnemyGalaxyPreset(int preset) {
        switch (preset) {
        case 0: globals::visuals::EnemyGalaxyR = 0.613636f; globals::visuals::EnemyGalaxyG = 0.0f;      globals::visuals::EnemyGalaxyB = 0.170455f; break; // Red Dark
        case 1: globals::visuals::EnemyGalaxyR = 0.0f;      globals::visuals::EnemyGalaxyG = 0.545455f; globals::visuals::EnemyGalaxyB = 0.170455f; break; // Dark Green
        case 2: globals::visuals::EnemyGalaxyR = 0.0f;      globals::visuals::EnemyGalaxyG = 0.0f;      globals::visuals::EnemyGalaxyB = 0.477273f; break; // Dark Blue
        case 3: globals::visuals::EnemyGalaxyR = 0.647727f; globals::visuals::EnemyGalaxyG = 0.579545f; globals::visuals::EnemyGalaxyB = 0.0f;      break; // Dark Orange
        case 4: globals::visuals::EnemyGalaxyR = 0.681818f; globals::visuals::EnemyGalaxyG = 0.579545f; globals::visuals::EnemyGalaxyB = 0.0f;      break; // Orange
        case 5: globals::visuals::EnemyGalaxyR = 0.647727f; globals::visuals::EnemyGalaxyG = 0.545455f; globals::visuals::EnemyGalaxyB = 0.511364f; break; // Pink
        case 6: globals::visuals::EnemyGalaxyR = 0.647727f; globals::visuals::EnemyGalaxyG = 0.579545f; globals::visuals::EnemyGalaxyB = 0.545455f; break; // Black Galaxy
        case 7: globals::visuals::EnemyGalaxyR = 0.613636f; globals::visuals::EnemyGalaxyG = 0.511364f; globals::visuals::EnemyGalaxyB = 0.545455f; break; // Purple
        case 8: globals::visuals::EnemyGalaxyR = 0.05f;      globals::visuals::EnemyGalaxyG = 0.15f;     globals::visuals::EnemyGalaxyB = 0.85f;     break; // Star Blue
        case 9: globals::visuals::EnemyGalaxyR = 0.45f;      globals::visuals::EnemyGalaxyG = 0.05f;     globals::visuals::EnemyGalaxyB = 0.95f;     break; // Star Purple
        case 10: globals::visuals::EnemyGalaxyR = 0.1f;      globals::visuals::EnemyGalaxyG = 0.05f;     globals::visuals::EnemyGalaxyB = 0.8f;      break; // Sentinal Galaxy
        }
    }

    void apply_galaxy_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) return;
        if (!globals::chams::enemy_galaxy_enabled) return;

        static int last_enemy_preset = -1;
        if (last_enemy_preset != globals::visuals::enemy_crystal_chams_preset) {
            ApplyEnemyGalaxyPreset(globals::visuals::enemy_crystal_chams_preset);
            last_enemy_preset = globals::visuals::enemy_crystal_chams_preset;
        }

        uobject* galaxy_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!galaxy_material || !memory::IsValidPointer((uintptr_t)galaxy_material)) return;

        fname silohuette_color_name = string::string_to_name(L"SilohuetteColor");
        fname center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
        fname inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
        fname outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
        fname glow_intensity_param = string::string_to_name(L"GlowIntensity");

        fname alpha_base_power_name = string::string_to_name(L"Alpha_Base_Power");
        fname depth_bias_name = string::string_to_name(L"DepthBias");
        fname alpha_dissolve_opacity_name = string::string_to_name(L"Alpha_Dissolve_Opacity");
        fname bounding_box_name = string::string_to_name(L"BoundingBox");
        fname inner_edge_thickness_name = string::string_to_name(L"InnerEdgeThickness");
        fname outer_edge_thickness_name = string::string_to_name(L"OuterEdgeThickness");
        fname rim_fresnel_name = string::string_to_name(L"Rim_Fresnel");
        fname rim_multiply_name = string::string_to_name(L"Rim_Multiply");
        fname rim_power_name = string::string_to_name(L"Rim_Power");
        fname occlusion_behind_wall_name = string::string_to_name(L"OcclusionDepth_BehindWall");
        fname occlusion_state_name = string::string_to_name(L"OcclusionState");
        fname refraction_depth_bias_name = string::string_to_name(L"RefractionDepthBias");

        float enemy_glowIntensity = 30.0f;
        float alpha_base_power = 0.8f;
        float depth_bias = 0.1f;
        float alpha_dissolve_opacity = 0.25f;
        float bounding_box = -25.0f;
        float inner_edge_thickness = 0.15f;
        float outer_edge_thickness = 0.35f;
        float rim_fresnel = 1.0f;
        float rim_multiply = 1.0f;
        float rim_power = 10.0f;
        float occlusion_behind_wall = 1.0f;
        float occlusion_state = 1.0f;
        float refraction_depth_bias = 0.0f;

        flinearcolor enemy_centerEdgeColor = flinearcolor(
            globals::visuals::EnemyGalaxyR,
            globals::visuals::EnemyGalaxyG,
            globals::visuals::EnemyGalaxyB,
            18.20f
        );
        flinearcolor enemy_innerEdgeColor = flinearcolor(
            globals::visuals::EnemyGalaxyR,
            globals::visuals::EnemyGalaxyG,
            globals::visuals::EnemyGalaxyB,
            18.20f
        );
        flinearcolor enemy_outerEdgeColor = flinearcolor(
            globals::visuals::EnemyGalaxyR,
            globals::visuals::EnemyGalaxyG,
            globals::visuals::EnemyGalaxyB,
            18.20f
        );

        auto main_mesh = actor->get_mesh();
        if (main_mesh && memory::IsValidPointer((uintptr_t)main_mesh)) {
            auto num_materials = main_mesh->get_num_materials();
            if (num_materials > 0 && num_materials <= 64) {
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)main_mesh, i, galaxy_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(silohuette_color_name, enemy_outerEdgeColor);
                dynCast->set_vector_parameter_value1(center_edge_color_name, enemy_centerEdgeColor);
                dynCast->set_vector_parameter_value1(inner_edge_color_name, enemy_innerEdgeColor);
                dynCast->set_vector_parameter_value1(outer_edge_color_name, enemy_outerEdgeColor);
                dynCast->set_scalar_parameter_value(glow_intensity_param, enemy_glowIntensity);
                dynCast->set_scalar_parameter_value(alpha_base_power_name, alpha_base_power);
                dynCast->set_scalar_parameter_value(depth_bias_name, depth_bias);
                dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, alpha_dissolve_opacity);
                dynCast->set_scalar_parameter_value(bounding_box_name, bounding_box);
                dynCast->set_scalar_parameter_value(inner_edge_thickness_name, inner_edge_thickness);
                dynCast->set_scalar_parameter_value(outer_edge_thickness_name, outer_edge_thickness);
                dynCast->set_scalar_parameter_value(rim_fresnel_name, rim_fresnel);
                dynCast->set_scalar_parameter_value(rim_multiply_name, rim_multiply);
                dynCast->set_scalar_parameter_value(rim_power_name, rim_power);
                dynCast->set_scalar_parameter_value(occlusion_behind_wall_name, occlusion_behind_wall);
                dynCast->set_scalar_parameter_value(occlusion_state_name, occlusion_state);
                dynCast->set_scalar_parameter_value(refraction_depth_bias_name, refraction_depth_bias);
            }
            }
        }

        uskeletalmeshcomponent* mesh_cosmetic_3p = actor->GetCosmeticMesh3P();
        if (mesh_cosmetic_3p && memory::IsValidPointer((uintptr_t)mesh_cosmetic_3p)) {
            auto num_materials = mesh_cosmetic_3p->get_num_materials();
            if (num_materials > 0 && num_materials <= 64) {
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh_cosmetic_3p, i, galaxy_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(silohuette_color_name, enemy_outerEdgeColor);
                dynCast->set_vector_parameter_value1(center_edge_color_name, enemy_centerEdgeColor);
                dynCast->set_vector_parameter_value1(inner_edge_color_name, enemy_innerEdgeColor);
                dynCast->set_vector_parameter_value1(outer_edge_color_name, enemy_outerEdgeColor);
                dynCast->set_scalar_parameter_value(glow_intensity_param, enemy_glowIntensity);
                dynCast->set_scalar_parameter_value(alpha_base_power_name, alpha_base_power);
                dynCast->set_scalar_parameter_value(depth_bias_name, depth_bias);
                dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, alpha_dissolve_opacity);
                dynCast->set_scalar_parameter_value(bounding_box_name, bounding_box);
                dynCast->set_scalar_parameter_value(inner_edge_thickness_name, inner_edge_thickness);
                dynCast->set_scalar_parameter_value(outer_edge_thickness_name, outer_edge_thickness);
                dynCast->set_scalar_parameter_value(rim_fresnel_name, rim_fresnel);
                dynCast->set_scalar_parameter_value(rim_multiply_name, rim_multiply);
                dynCast->set_scalar_parameter_value(rim_power_name, rim_power);
                dynCast->set_scalar_parameter_value(occlusion_behind_wall_name, occlusion_behind_wall);
                dynCast->set_scalar_parameter_value(occlusion_state_name, occlusion_state);
                dynCast->set_scalar_parameter_value(refraction_depth_bias_name, refraction_depth_bias);
            }
            }
        }
    }

    void apply_local_galaxy_chams(acknowledgedpawn* pawn, ashootercharacter* character)
    {
        static bool last_enabled_any = false;
        bool currently_enabled = (globals::chams::self_galaxy_enabled || 
                                 globals::chams::hand_galaxy_enabled || 
                                 globals::chams::gun1p_galaxy_enabled || 
                                 globals::chams::gun3p_galaxy_enabled);

        if (!currently_enabled) {
            if (last_enabled_any && character && memory::IsValidPointer((uintptr_t)character) && IsValidUObject(character) && character->is_alive()) {
                auto reset_mesh = [&](uskeletalmeshcomponent* mesh) {
                    if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
                    auto nm = mesh->get_num_materials();
                    if (nm <= 0 || nm > 64) return;
                    for (int i = 0; i < nm; i++)
                        seh_SetMaterial((uintptr_t)mesh, i, nullptr);
                };
                reset_mesh(character->get_mesh());
                reset_mesh(character->GetCosmeticMesh3P());
                if (pawn && memory::IsValidPointer((uintptr_t)pawn)) {
                    reset_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p));
                    reset_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay));
                }
            }
            last_enabled_any = false;
            return;
        }
        last_enabled_any = true;

        if (!pawn || !character || !character->is_alive()) return;

        static int last_preset = -1;
        if (last_preset != globals::visuals::crystal_chams_preset) {
            ApplyCrystalChamsPreset(globals::visuals::crystal_chams_preset);
            last_preset = globals::visuals::crystal_chams_preset;
        }

        uobject* galaxy_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!galaxy_material || !memory::IsValidPointer((uintptr_t)galaxy_material)) return;

        fname silohuette_color_name = string::string_to_name(L"SilohuetteColor");
        fname center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
        fname inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
        fname outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
        fname glow_intensity_param = string::string_to_name(L"GlowIntensity");

        fname alpha_base_power_name = string::string_to_name(L"Alpha_Base_Power");
        fname depth_bias_name = string::string_to_name(L"DepthBias");
        fname alpha_dissolve_opacity_name = string::string_to_name(L"Alpha_Dissolve_Opacity");
        fname bounding_box_name = string::string_to_name(L"BoundingBox");
        fname inner_edge_thickness_name = string::string_to_name(L"InnerEdgeThickness");
        fname outer_edge_thickness_name = string::string_to_name(L"OuterEdgeThickness");
        fname rim_fresnel_name = string::string_to_name(L"Rim_Fresnel");
        fname rim_multiply_name = string::string_to_name(L"Rim_Multiply");
        fname rim_power_name = string::string_to_name(L"Rim_Power");
        fname occlusion_behind_wall_name = string::string_to_name(L"OcclusionDepth_BehindWall");
        fname occlusion_state_name = string::string_to_name(L"OcclusionState");
        fname refraction_depth_bias_name = string::string_to_name(L"RefractionDepthBias");

        float glow_intensity = globals::visuals::GlowVisible;
        float alpha_base_power = globals::visuals::AlphaBasePower;
        float depth_bias = globals::visuals::DepthBias;
        float alpha_dissolve_opacity = globals::visuals::AlphaDissolveOpacity;
        float bounding_box = globals::visuals::BoundingBox;
        float inner_edge_thickness = globals::visuals::InnerEdgeThickness;
        float outer_edge_thickness = globals::visuals::OuterEdgeThickness;
        float rim_fresnel = globals::visuals::RimFresnel;
        float rim_multiply = globals::visuals::RimMultiply;
        float rim_power = globals::visuals::RimPower;
        float occlusion_behind_wall = globals::visuals::OcclusionBehindWall;
        float occlusion_state = globals::visuals::OcclusionState;
        float refraction_depth_bias = globals::visuals::RefractionDepthBias;

        flinearcolor center_edge_color = flinearcolor(
            globals::visuals::Self_CenterEdgeR,
            globals::visuals::Self_CenterEdgeG,
            globals::visuals::Self_CenterEdgeB,
            18.20f
        );
        flinearcolor inner_edge_color = flinearcolor(
            globals::visuals::Self_InnerEdgeR,
            globals::visuals::Self_InnerEdgeG,
            globals::visuals::Self_InnerEdgeB,
            18.20f
        );
        flinearcolor outer_edge_color = flinearcolor(
            globals::visuals::Self_OuterEdgeR,
            globals::visuals::Self_OuterEdgeG,
            globals::visuals::Self_OuterEdgeB,
            18.20f
        );

        auto apply_on_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            auto num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, galaxy_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(silohuette_color_name, outer_edge_color);
                dynCast->set_vector_parameter_value1(center_edge_color_name, center_edge_color);
                dynCast->set_vector_parameter_value1(inner_edge_color_name, inner_edge_color);
                dynCast->set_vector_parameter_value1(outer_edge_color_name, outer_edge_color);
                dynCast->set_scalar_parameter_value(glow_intensity_param, glow_intensity);
                dynCast->set_scalar_parameter_value(alpha_base_power_name, alpha_base_power);
                dynCast->set_scalar_parameter_value(depth_bias_name, depth_bias);
                dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, alpha_dissolve_opacity);
                dynCast->set_scalar_parameter_value(bounding_box_name, bounding_box);
                dynCast->set_scalar_parameter_value(inner_edge_thickness_name, inner_edge_thickness);
                dynCast->set_scalar_parameter_value(outer_edge_thickness_name, outer_edge_thickness);
                dynCast->set_scalar_parameter_value(rim_fresnel_name, rim_fresnel);
                dynCast->set_scalar_parameter_value(rim_multiply_name, rim_multiply);
                dynCast->set_scalar_parameter_value(rim_power_name, rim_power);
                dynCast->set_scalar_parameter_value(occlusion_behind_wall_name, occlusion_behind_wall);
                dynCast->set_scalar_parameter_value(occlusion_state_name, occlusion_state);
                dynCast->set_scalar_parameter_value(refraction_depth_bias_name, refraction_depth_bias);
            }
        };

        if (globals::chams::self_galaxy_enabled) {
            apply_on_mesh(character->get_mesh());
            apply_on_mesh(character->GetCosmeticMesh3P());
        }

        if (globals::chams::hand_galaxy_enabled) {
            apply_on_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p));
            apply_on_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay));
        }

        if (auto inventory = character->get_inventory()) {
            if (auto current_weapon = inventory->get_current_equippable()) {
                if (globals::chams::gun1p_galaxy_enabled) {
                    apply_on_mesh(current_weapon->GetMesh1P());
                }
                if (globals::chams::gun3p_galaxy_enabled) {
                    apply_on_mesh(current_weapon->GetMesh3P());
                }
            }
        }
    }

    // ========================================================================
    // === CRYSTAL CHAMS (Self / Hand / Enemy) ===
    // ========================================================================
    struct crystal_color_preset {
        float cr;
        float cg;
        float cb;
    };

    crystal_color_preset get_crystal_color_preset(int preset) {
        crystal_color_preset p;
        switch (preset) {
        case 0:  p.cr = 0.00f; p.cg = 0.59f; p.cb = 0.58f; break; // Crystal Teal
        case 1:  p.cr = 0.65f; p.cg = 0.63f; p.cb = 0.58f; break; // Crystal Silver
        default: p.cr = 0.68f; p.cg = 0.63f; p.cb = 0.58f; break; // Crystal Pearl
        }
        return p;
    }

    static void ApplyCrystalChamsToMesh(uskeletalmeshcomponent* mesh, uobject* crystal_material, float r, float g, float b)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
        if (!crystal_material || !memory::IsValidPointer((uintptr_t)crystal_material)) return;

        static fname silohuette_color_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name, glow_intensity_param;
        static fname alpha_base_power_name, depth_bias_name, alpha_dissolve_opacity_name, bounding_box_name;
        static fname inner_edge_thickness_name, outer_edge_thickness_name, rim_fresnel_name, rim_multiply_name, rim_power_name;
        static fname occlusion_behind_wall_name, occlusion_state_name, refraction_depth_bias_name;
        static bool name_init = false;
        if (!name_init) {
            silohuette_color_name = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
            glow_intensity_param = string::string_to_name(L"GlowIntensity");
            alpha_base_power_name = string::string_to_name(L"Alpha_Base_Power");
            depth_bias_name = string::string_to_name(L"DepthBias");
            alpha_dissolve_opacity_name = string::string_to_name(L"Alpha_Dissolve_Opacity");
            bounding_box_name = string::string_to_name(L"BoundingBox");
            inner_edge_thickness_name = string::string_to_name(L"InnerEdgeThickness");
            outer_edge_thickness_name = string::string_to_name(L"OuterEdgeThickness");
            rim_fresnel_name = string::string_to_name(L"Rim_Fresnel");
            rim_multiply_name = string::string_to_name(L"Rim_Multiply");
            rim_power_name = string::string_to_name(L"Rim_Power");
            occlusion_behind_wall_name = string::string_to_name(L"OcclusionDepth_BehindWall");
            occlusion_state_name = string::string_to_name(L"OcclusionState");
            refraction_depth_bias_name = string::string_to_name(L"RefractionDepthBias");
            name_init = true;
        }

        int num_materials = mesh->get_num_materials();
        if (num_materials <= 0 || num_materials > 64) return;

        flinearcolor edge_color = flinearcolor(r, g, b, 18.20f);

        for (int i = 0; i < num_materials; i++) {
            auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, crystal_material);
            if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
            auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
            if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

            dynCast->set_vector_parameter_value1(silohuette_color_name, edge_color);
            dynCast->set_vector_parameter_value1(center_edge_color_name, edge_color);
            dynCast->set_vector_parameter_value1(inner_edge_color_name, edge_color);
            dynCast->set_vector_parameter_value1(outer_edge_color_name, edge_color);
            dynCast->set_scalar_parameter_value(glow_intensity_param, 30.0f);
            dynCast->set_scalar_parameter_value(alpha_base_power_name, 0.8f);
            dynCast->set_scalar_parameter_value(depth_bias_name, 0.1f);
            dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, 0.25f);
            dynCast->set_scalar_parameter_value(bounding_box_name, -25.0f);
            dynCast->set_scalar_parameter_value(inner_edge_thickness_name, 0.15f);
            dynCast->set_scalar_parameter_value(outer_edge_thickness_name, 0.35f);
            dynCast->set_scalar_parameter_value(rim_fresnel_name, 1.0f);
            dynCast->set_scalar_parameter_value(rim_multiply_name, 1.0f);
            dynCast->set_scalar_parameter_value(rim_power_name, 10.0f);
            dynCast->set_scalar_parameter_value(occlusion_behind_wall_name, 1.0f);
            dynCast->set_scalar_parameter_value(occlusion_state_name, 1.0f);
            dynCast->set_scalar_parameter_value(refraction_depth_bias_name, 0.0f);
        }
    }

    static void ResetCrystalChams(uskeletalmeshcomponent* mesh)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
        int num_materials = mesh->get_num_materials();
        if (num_materials <= 0 || num_materials > 64) return;
        for (int i = 0; i < num_materials; i++)
            seh_SetMaterial((uintptr_t)mesh, i, nullptr);
    }

    void apply_crystal_local_chams(acknowledgedpawn* pawn, ashootercharacter* character)
    {
        if (!pawn || !character || !memory::IsValidPointer((uintptr_t)character)) return;
        if (!character->is_alive()) return;

        static bool last_any_enabled = false;
        bool any_enabled = globals::visuals::crystal_chams_self || globals::visuals::crystal_chams_hand;

        if (!any_enabled) {
            if (last_any_enabled && IsValidUObject((uobject*)character)) {
                ResetCrystalChams(character->get_mesh());
                ResetCrystalChams(character->GetCosmeticMesh3P());
                ResetCrystalChams(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p));
                ResetCrystalChams(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay));
            }
            last_any_enabled = false;
            return;
        }
        last_any_enabled = true;

        uobject* crystal_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");
        if (!crystal_material || !memory::IsValidPointer((uintptr_t)crystal_material)) return;

        if (globals::visuals::crystal_chams_self) {
            auto c = get_crystal_color_preset(globals::visuals::crystal_chams_self_preset);
            ApplyCrystalChamsToMesh(character->get_mesh(), crystal_material, c.cr, c.cg, c.cb);
            ApplyCrystalChamsToMesh(character->GetCosmeticMesh3P(), crystal_material, c.cr, c.cg, c.cb);
        }

        if (globals::visuals::crystal_chams_hand) {
            auto c = get_crystal_color_preset(globals::visuals::crystal_chams_hand_preset);
            ApplyCrystalChamsToMesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p), crystal_material, c.cr, c.cg, c.cb);
            ApplyCrystalChamsToMesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay), crystal_material, c.cr, c.cg, c.cb);
        }
    }

    void apply_crystal_enemy_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) return;
        if (!globals::visuals::crystal_chams_enemy) return;

        uobject* crystal_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");
        if (!crystal_material || !memory::IsValidPointer((uintptr_t)crystal_material)) return;

        auto c = get_crystal_color_preset(globals::visuals::crystal_chams_enemy_preset);
        ApplyCrystalChamsToMesh(actor->get_mesh(), crystal_material, c.cr, c.cg, c.cb);
        ApplyCrystalChamsToMesh(actor->GetCosmeticMesh3P(), crystal_material, c.cr, c.cg, c.cb);
    }

    // ========================================================================
    // === NEBULA CHAMS (Type 1 / 2 / 3) - Ported from src ===
    // ========================================================================
    static inline void NebulaApplyPresetVisuals(int preset)
    {
        // Type 1
        if (preset <= 0) {
            globals::visuals::Self_CenterEdgeR = 0.00f;
            globals::visuals::Self_CenterEdgeG = 0.59f;
            globals::visuals::Self_CenterEdgeB = 0.58f;
            globals::visuals::Self_InnerEdgeR = 0.00f;
            globals::visuals::Self_InnerEdgeG = 12.74f;
            globals::visuals::Self_InnerEdgeB = 13.64f;
            globals::visuals::Self_OuterEdgeR = 3.36f;
            globals::visuals::Self_OuterEdgeG = 0.00f;
            globals::visuals::Self_OuterEdgeB = 0.00f;
        }
        // Type 2
        else if (preset == 1) {
            globals::visuals::Self_CenterEdgeR = 0.65f;
            globals::visuals::Self_CenterEdgeG = 0.63f;
            globals::visuals::Self_CenterEdgeB = 0.58f;
            globals::visuals::Self_InnerEdgeR = 0.00f;
            globals::visuals::Self_InnerEdgeG = 12.74f;
            globals::visuals::Self_InnerEdgeB = 13.64f;
            globals::visuals::Self_OuterEdgeR = 3.36f;
            globals::visuals::Self_OuterEdgeG = 0.00f;
            globals::visuals::Self_OuterEdgeB = 0.00f;
        }
        // Type 3
        else {
            globals::visuals::Self_CenterEdgeR = 0.68f;
            globals::visuals::Self_CenterEdgeG = 0.63f;
            globals::visuals::Self_CenterEdgeB = 0.58f;
            globals::visuals::Self_InnerEdgeR = 0.00f;
            globals::visuals::Self_InnerEdgeG = 12.74f;
            globals::visuals::Self_InnerEdgeB = 13.64f;
            globals::visuals::Self_OuterEdgeR = 3.36f;
            globals::visuals::Self_OuterEdgeG = 0.00f;
            globals::visuals::Self_OuterEdgeB = 0.00f;
        }

        globals::visuals::GlowVisible = 1.6f;
        globals::visuals::AlphaBasePower = 2.5f;
        globals::visuals::AlphaColorMult = 0.9f;
        globals::visuals::DepthBias = 0.08f;
        globals::visuals::AlphaDissolveOpacity = 0.85f;
        globals::visuals::BoundingBox = 1.0f;
        globals::visuals::InnerEdgeThickness = 16.43f;
        globals::visuals::OuterEdgeThickness = 2.80f;
        globals::visuals::RimFresnel = 44.23f;
        globals::visuals::RimMultiply = 0.8f;
        globals::visuals::RimPower = 3.32f;
        globals::visuals::OcclusionDepth = 0.7f;
        globals::visuals::OcclusionBehindWall = 2.10f;
        globals::visuals::OcclusionState = 1.0f;
        globals::visuals::RefractionDepthBias = 0.04f;
    }

    // Free helper: matches the existing apply_local_galaxy_chams style (no __try).
    // Validity is enforced by IsValidPointer / IsValidUObject checks at every step.
    static void NebulaApplyToMesh(
        uskeletalmeshcomponent* mesh,
        uobject* nebula_material)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh) || !nebula_material) return;

        static fname silohuette_color_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name, glow_intensity_param;
        static fname alpha_base_power_name, depth_bias_name, alpha_dissolve_opacity_name, bounding_box_name;
        static fname inner_edge_thickness_name, outer_edge_thickness_name, rim_fresnel_name, rim_multiply_name, rim_power_name;
        static fname occlusion_behind_wall_name, occlusion_state_name, refraction_depth_bias_name;
        static bool name_init = false;
        if (!name_init) {
            silohuette_color_name = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
            glow_intensity_param = string::string_to_name(L"GlowIntensity");
            alpha_base_power_name = string::string_to_name(L"Alpha_Base_Power");
            depth_bias_name = string::string_to_name(L"DepthBias");
            alpha_dissolve_opacity_name = string::string_to_name(L"Alpha_Dissolve_Opacity");
            bounding_box_name = string::string_to_name(L"BoundingBox");
            inner_edge_thickness_name = string::string_to_name(L"InnerEdgeThickness");
            outer_edge_thickness_name = string::string_to_name(L"OuterEdgeThickness");
            rim_fresnel_name = string::string_to_name(L"Rim_Fresnel");
            rim_multiply_name = string::string_to_name(L"Rim_Multiply");
            rim_power_name = string::string_to_name(L"Rim_Power");
            occlusion_behind_wall_name = string::string_to_name(L"OcclusionDepth_BehindWall");
            occlusion_state_name = string::string_to_name(L"OcclusionState");
            refraction_depth_bias_name = string::string_to_name(L"RefractionDepthBias");
            name_init = true;
        }

        flinearcolor centerColor(globals::visuals::Self_CenterEdgeR, globals::visuals::Self_CenterEdgeG, globals::visuals::Self_CenterEdgeB, globals::visuals::intensityvisibleoutline);
        flinearcolor innerColor(globals::visuals::Self_InnerEdgeR, globals::visuals::Self_InnerEdgeG, globals::visuals::Self_InnerEdgeB, globals::visuals::intensityvisibleoutline);
        flinearcolor outerColor(globals::visuals::Self_OuterEdgeR, globals::visuals::Self_OuterEdgeG, globals::visuals::Self_OuterEdgeB, globals::visuals::intensityvisibleoutline);

        int num_materials = mesh->get_num_materials();
        if (num_materials <= 0 || num_materials > 64) return;
        for (int i = 0; i < num_materials; i++) {
            auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, nebula_material);
            if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
            auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
            if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

            dynCast->set_vector_parameter_value1(silohuette_color_name,  outerColor);
            dynCast->set_vector_parameter_value1(center_edge_color_name, centerColor);
            dynCast->set_vector_parameter_value1(inner_edge_color_name,  innerColor);
            dynCast->set_vector_parameter_value1(outer_edge_color_name,  outerColor);
            dynCast->set_scalar_parameter_value(glow_intensity_param,        globals::visuals::GlowVisible);
            dynCast->set_scalar_parameter_value(alpha_base_power_name,       globals::visuals::AlphaBasePower);
            dynCast->set_scalar_parameter_value(depth_bias_name,             globals::visuals::DepthBias);
            dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, globals::visuals::AlphaDissolveOpacity);
            dynCast->set_scalar_parameter_value(bounding_box_name,           globals::visuals::BoundingBox);
            dynCast->set_scalar_parameter_value(inner_edge_thickness_name,   globals::visuals::InnerEdgeThickness);
            dynCast->set_scalar_parameter_value(outer_edge_thickness_name,   globals::visuals::OuterEdgeThickness);
            dynCast->set_scalar_parameter_value(rim_fresnel_name,            globals::visuals::RimFresnel);
            dynCast->set_scalar_parameter_value(rim_multiply_name,           globals::visuals::RimMultiply);
            dynCast->set_scalar_parameter_value(rim_power_name,              globals::visuals::RimPower);
            dynCast->set_scalar_parameter_value(occlusion_behind_wall_name,  globals::visuals::OcclusionBehindWall);
            dynCast->set_scalar_parameter_value(occlusion_state_name,        globals::visuals::OcclusionState);
            dynCast->set_scalar_parameter_value(refraction_depth_bias_name,  globals::visuals::RefractionDepthBias);
        }
    }

    void apply_nebula_chams_hand(ashootercharacter* local_player, acknowledgedpawn* pawn)
    {
        if (!local_player || !pawn) return;
        if (!IsValidUObject((uobject*)local_player) || !IsValidUObject((uobject*)pawn)) return;

        static int self_frame_counter = 0;
        static int last_preset = -1;

        self_frame_counter++;
        if (self_frame_counter % 6 != 0) return;

        if (last_preset != globals::chams::nebula_hand_type) {
            NebulaApplyPresetVisuals(globals::chams::nebula_hand_type);
            last_preset = globals::chams::nebula_hand_type;
        }

        uobject* nebula_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!nebula_material || !memory::IsValidPointer((uintptr_t)nebula_material)) return;

        NebulaApplyToMesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p),         nebula_material);
        NebulaApplyToMesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay), nebula_material);
    }

    void apply_nebula_chams_self3p(ashootercharacter* local_player)
    {
        if (!local_player) return;
        if (!IsValidUObject((uobject*)local_player)) return;

        static int self_frame_counter = 0;
        static int last_preset = -1;

        self_frame_counter++;
        if (self_frame_counter % 6 != 0) return;

        if (last_preset != globals::chams::nebula_self_type) {
            NebulaApplyPresetVisuals(globals::chams::nebula_self_type);
            last_preset = globals::chams::nebula_self_type;
        }

        uobject* nebula_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!nebula_material || !memory::IsValidPointer((uintptr_t)nebula_material)) return;

        NebulaApplyToMesh(local_player->GetCosmeticMesh3P(), nebula_material);
    }

    // ========================================================================
    // === GALAXY CHAMS v2 (Hand / Self / Enemy) - Ported from src ===
    // ========================================================================
    static inline void GalaxyV2ApplyHandToMesh(uskeletalmeshcomponent* mesh, uobject* visible_material, int color_preset)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh) || !visible_material) return;

        static fname silo_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name;
        static fname glow_name, rim_power, rim_multiply, outer_edge_thickness, bounding_box_param;
        static bool name_init = false;
        if (!name_init) {
            silo_name                = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name   = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name    = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name    = string::string_to_name(L"OuterEdgeColor");
            glow_name                = string::string_to_name(L"GlowIntensity");
            rim_power                = string::string_to_name(L"Rim_Power");
            rim_multiply             = string::string_to_name(L"Rim_Multiply");
            outer_edge_thickness     = string::string_to_name(L"OuterEdgeThickness");
            bounding_box_param       = string::string_to_name(L"BoundingBox");
            name_init = true;
        }

        flinearcolor center_color(0.0f, 0.0f, 0.29f, 10.0f);
        flinearcolor inner_color(0.69f, 0.63f, 0.60f, 10.0f);
        flinearcolor outer_color(0.0f, 0.62f, 0.59f, 10.0f);
        flinearcolor silo_color = outer_color;
        float glow_intensity = 12.0f;
        float rim_power_val = 3.0f;
        float rim_multiply_val = 2.0f;
        float outer_thickness_val = 0.4f;
        float bounding_box_val = 4097.0f;

        if (color_preset == 0) {
            // Flat white — low alpha/glow so it stays matte, not blown-out bloom.
            center_color = flinearcolor(0.88f, 0.88f, 0.88f, 2.0f);
            inner_color  = flinearcolor(0.92f, 0.92f, 0.92f, 2.5f);
            outer_color  = flinearcolor(0.90f, 0.90f, 0.90f, 2.0f);
            silo_color   = outer_color;
            glow_intensity = 1.5f;
            rim_power_val = 30.0f;
            rim_multiply_val = 0.12f;
            outer_thickness_val = 0.10f;
        } else if (color_preset == 1) {
            center_color = flinearcolor(0.0f, 0.0f, 0.0f, 18.20f);
            inner_color  = flinearcolor(0.0f, 0.0f, 0.0f, 18.20f);
            outer_color  = flinearcolor(0.0f, 0.0f, 0.0f, 18.20f);
            silo_color   = outer_color;
        } else {
            static float hand_galaxy_rgb_time = 0.0f;
            hand_galaxy_rgb_time += 0.014f;

            const flinearcolor rgb_a = GetRainbowColor(hand_galaxy_rgb_time);
            const flinearcolor rgb_b = GetRainbowColor(hand_galaxy_rgb_time + 1.05f);
            const flinearcolor rgb_c = GetRainbowColor(hand_galaxy_rgb_time + 2.10f);
            const float color_alpha = 2.0f;

            silo_color   = rgb_a; silo_color.a   = color_alpha;
            inner_color  = rgb_b; inner_color.a  = color_alpha;
            center_color = rgb_c; center_color.a = color_alpha;
            outer_color  = center_color;
            // Matte RGB — minimal rim bloom from any viewing angle
            glow_intensity = 0.08f;
            rim_power_val = 52.0f;
            rim_multiply_val = 0.015f;
            outer_thickness_val = 0.02f;
            bounding_box_val = 4097.0f;
        }

        if (!is_mesh_component_safe((uintptr_t)mesh)) return;

        const int count = seh_fp_num_materials((uintptr_t)mesh);
        if (count <= 0 || count > 64) return;
        for (int i = 0; i < count; i++)
        {
            auto dynMat = seh_create_dynamic_mat((uintptr_t)mesh, i, visible_material, true);
            if (!dynMat || !memory::IsValidPointer((uintptr_t)dynMat)) continue;

            auto matInst = dynMat->cast<UMaterialInstanceDynamic>();
            if (!matInst || !memory::IsValidPointer((uintptr_t)matInst)) continue;
            if (!IsValidUObject((uobject*)matInst)) continue;

            matInst->set_vector_parameter_value1(silo_name, silo_color);
            matInst->set_vector_parameter_value1(center_edge_color_name, center_color);
            matInst->set_vector_parameter_value1(inner_edge_color_name, inner_color);
            matInst->set_vector_parameter_value1(outer_edge_color_name, outer_color);
            matInst->set_scalar_parameter_value(glow_name, glow_intensity);
            matInst->set_scalar_parameter_value(rim_power, rim_power_val);
            matInst->set_scalar_parameter_value(rim_multiply, rim_multiply_val);
            matInst->set_scalar_parameter_value(outer_edge_thickness, outer_thickness_val);
            matInst->set_scalar_parameter_value(bounding_box_param, bounding_box_val);
        }
    }

    static inline void GalaxyV2ApplyBlackChamsToMesh(uskeletalmeshcomponent* mesh, uobject* visible_material, int chams_type)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh) || !visible_material) return;

        static fname silo_name, center_edge_color_name, inner_edge_color_name, outer_edge_color_name;
        static fname glow_name, rim_power, rim_multiply, outer_edge_thickness, bounding_box_param;
        static bool name_init = false;
        if (!name_init) {
            silo_name              = string::string_to_name(L"SilohuetteColor");
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name  = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name  = string::string_to_name(L"OuterEdgeColor");
            glow_name              = string::string_to_name(L"GlowIntensity");
            rim_power              = string::string_to_name(L"Rim_Power");
            rim_multiply           = string::string_to_name(L"Rim_Multiply");
            outer_edge_thickness   = string::string_to_name(L"OuterEdgeThickness");
            bounding_box_param     = string::string_to_name(L"BoundingBox");
            name_init = true;
        }

        const float intensity_scale = globals::chams::black_chams_intensity / 50.0f;
        flinearcolor center_color(0.0f, 0.0f, 0.0f, 8.0f * intensity_scale);
        flinearcolor inner_color  = center_color;
        flinearcolor outer_color  = center_color;
        flinearcolor silo_color   = center_color;
        float glow_intensity = 1.2f * intensity_scale;
        float rim_power_val = 42.0f;
        float rim_multiply_val = 0.06f;
        float outer_thickness_val = 0.06f;
        float bounding_box_val = 4097.0f;

        if (chams_type == 1) {
            static float black_rgb_time = 0.0f;
            black_rgb_time += 0.016f;

            const flinearcolor rgb_a = GetRainbowColor(black_rgb_time);
            const flinearcolor rgb_b = GetRainbowColor(black_rgb_time + 1.05f);
            const flinearcolor rgb_c = GetRainbowColor(black_rgb_time + 2.10f);
            const float color_alpha = 8.0f * intensity_scale;

            auto clamp01 = [](float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
            auto brighten = [&](const flinearcolor& c, float mul) {
                return flinearcolor(
                    clamp01(c.r * mul),
                    clamp01(c.g * mul),
                    clamp01(c.b * mul),
                    color_alpha);
            };

            silo_color   = brighten(rgb_a, 1.10f);
            inner_color  = brighten(rgb_b, 1.15f);
            center_color = brighten(rgb_c, 1.18f);
            outer_color  = center_color;
            glow_intensity = 6.0f * intensity_scale;
            rim_power_val = 28.0f;
            rim_multiply_val = 0.22f;
            outer_thickness_val = 0.22f;
        }
        else if (chams_type == 2) {
            // Black fill + bright white outer rim (same rim strength as RGB)
            const float inner_alpha = 10.0f * intensity_scale;
            const float outer_alpha = 14.0f * intensity_scale;

            center_color = flinearcolor(0.f, 0.f, 0.f, inner_alpha);
            inner_color  = flinearcolor(0.f, 0.f, 0.f, inner_alpha);
            silo_color   = flinearcolor(0.f, 0.f, 0.f, inner_alpha);
            outer_color  = flinearcolor(1.f, 1.f, 1.f, outer_alpha);

            glow_intensity = 7.5f * intensity_scale;
            rim_power_val = 28.0f;
            rim_multiply_val = 0.24f;
            outer_thickness_val = 0.30f;
        }

        if (!is_mesh_component_safe((uintptr_t)mesh)) return;

        const int count = seh_fp_num_materials((uintptr_t)mesh);
        if (count <= 0 || count > 64) return;
        for (int i = 0; i < count; i++)
        {
            auto dynMat = seh_create_dynamic_mat((uintptr_t)mesh, i, visible_material, true);
            if (!dynMat || !memory::IsValidPointer((uintptr_t)dynMat)) continue;

            auto matInst = dynMat->cast<UMaterialInstanceDynamic>();
            if (!matInst || !memory::IsValidPointer((uintptr_t)matInst)) continue;
            if (!IsValidUObject((uobject*)matInst)) continue;

            matInst->set_vector_parameter_value1(silo_name, silo_color);
            matInst->set_vector_parameter_value1(center_edge_color_name, center_color);
            matInst->set_vector_parameter_value1(inner_edge_color_name, inner_color);
            matInst->set_vector_parameter_value1(outer_edge_color_name, outer_color);
            matInst->set_scalar_parameter_value(glow_name, glow_intensity);
            matInst->set_scalar_parameter_value(rim_power, rim_power_val);
            matInst->set_scalar_parameter_value(rim_multiply, rim_multiply_val);
            matInst->set_scalar_parameter_value(outer_edge_thickness, outer_thickness_val);
            matInst->set_scalar_parameter_value(bounding_box_param, bounding_box_val);
        }
    }

    static void seh_galaxy_v2_apply_black_to_mesh(uskeletalmeshcomponent* mesh, uobject* visible_material, int chams_type) {
        if (!mesh || !visible_material) return;
        if (!memory::IsValidPointer((uintptr_t)mesh) || !memory::IsValidPointer((uintptr_t)visible_material)) return;
        if (!IsValidUObject((uobject*)mesh)) return;
        __try {
            GalaxyV2ApplyBlackChamsToMesh(mesh, visible_material, chams_type);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("GalaxyV2ApplyBlackChamsToMesh EXCEPTION mesh=0x%llX type=%d",
                (unsigned long long)mesh, chams_type);
        }
    }

    static inline void GalaxyV2ApplyToMesh(uskeletalmeshcomponent* mesh, uobject* visible_material)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh) || !visible_material) return;

        static fname center_edge_color_name, inner_edge_color_name, outer_edge_color_name;
        static fname rim_power, Rim_Multiply, OuterEdgeThickness, boundingBoxParam;
        static bool name_init = false;
        if (!name_init) {
            center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
            inner_edge_color_name  = string::string_to_name(L"InnerEdgeColor");
            outer_edge_color_name  = string::string_to_name(L"OuterEdgeColor");
            rim_power              = string::string_to_name(L"Rim_Power");
            Rim_Multiply           = string::string_to_name(L"Rim_Multiply");
            OuterEdgeThickness     = string::string_to_name(L"OuterEdgeThickness");
            boundingBoxParam       = string::string_to_name(L"BoundingBox");
            name_init = true;
        }

        int count = mesh->get_num_materials();
        if (count <= 0 || count > 64) return;
        for (int i = 0; i < count; i++)
        {
            auto dynMat = seh_create_dynamic_mat((uintptr_t)mesh, i, visible_material);
            if (!dynMat || !memory::IsValidPointer((uintptr_t)dynMat)) continue;

            auto matInst = dynMat->cast<UMaterialInstanceDynamic>();
            if (!matInst || !memory::IsValidPointer((uintptr_t)matInst)) continue;

            matInst->set_vector_parameter_value1(center_edge_color_name, flinearcolor(0.0f, 0.0f, 0.29f, 10.0f));
            matInst->set_vector_parameter_value1(inner_edge_color_name,  flinearcolor(0.69f, 0.63f, 0.60f, 10.0f));
            matInst->set_vector_parameter_value1(outer_edge_color_name,  flinearcolor(0.0f, 0.62f, 0.59f, 10.0f));

            matInst->set_scalar_parameter_value(rim_power, 3.0f);
            matInst->set_scalar_parameter_value(Rim_Multiply, 2.0f);
            matInst->set_scalar_parameter_value(OuterEdgeThickness, 0.4f);
            matInst->set_scalar_parameter_value(boundingBoxParam, 4097.0f);
        }
    }

    static void seh_galaxy_v2_apply_hand_to_mesh(uskeletalmeshcomponent* mesh, uobject* visible_material, int color_preset) {
        if (!mesh || !visible_material) return;
        if (!memory::IsValidPointer((uintptr_t)mesh) || !memory::IsValidPointer((uintptr_t)visible_material)) return;
        if (!IsValidUObject((uobject*)mesh)) return;
        __try {
            GalaxyV2ApplyHandToMesh(mesh, visible_material, color_preset);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("GalaxyV2ApplyHandToMesh EXCEPTION mesh=0x%llX preset=%d",
                (unsigned long long)mesh, color_preset);
        }
    }

    void galaxychams_v2_hand(ashootercharacter* actor, acknowledgedpawn* pawn)
    {
        (void)pawn;
        if (!actor) return;
        if (!IsValidUObject((uobject*)actor)) return;

        uobject* visible_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");
        if (!visible_material || !memory::IsValidPointer((uintptr_t)visible_material)) return;

        const int preset = globals::chams::nebula_galaxy_hand_color;
        // 1P meshes are often invalid/hidden while third-person is active — never mix.
        if (globals::misc::tperson) {
            seh_galaxy_v2_apply_hand_to_mesh(actor->mesh3p(), visible_material, preset);
            seh_galaxy_v2_apply_hand_to_mesh(actor->get_mesh(), visible_material, preset);
            seh_galaxy_v2_apply_hand_to_mesh(actor->GetCosmeticMesh3P(), visible_material, preset);
        } else {
            seh_galaxy_v2_apply_hand_to_mesh(actor->getmesh1p(), visible_material, preset);
            seh_galaxy_v2_apply_hand_to_mesh(actor->GetOverlayMesh1P(), visible_material, preset);
        }
    }

    void galaxychams_v2_self3p(ashootercharacter* actor)
    {
        if (!actor) return;
        if (!IsValidUObject((uobject*)actor)) return;

        uobject* visible_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");
        if (!visible_material || !memory::IsValidPointer((uintptr_t)visible_material)) return;

        const int color_preset = globals::chams::nebula_galaxy_self_color;
        auto apply_body = [&](uskeletalmeshcomponent* mesh) {
            if (mesh && memory::IsValidPointer((uintptr_t)mesh))
                seh_galaxy_v2_apply_hand_to_mesh(mesh, visible_material, color_preset);
        };

        apply_body(actor->getmesh1p());
        apply_body(actor->GetOverlayMesh1P());
        // Only touch 3P body meshes when third person is actually on.
        if (globals::misc::tperson) {
            apply_body(actor->mesh3p());
            apply_body(actor->get_mesh());
            apply_body(actor->GetCosmeticMesh3P());
        }
    }

    void galaxychams_v2_enemy(ashootercharacter* actor)
    {
        if (!actor || !memory::IsValidPointer((uintptr_t)actor)) return;
        if (!IsValidUObject((uobject*)actor) || !actor->is_alive()) return;

        uobject* visible_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");
        if (!visible_material || !memory::IsValidPointer((uintptr_t)visible_material)) return;

        GalaxyV2ApplyToMesh(actor->get_mesh(), visible_material);
        GalaxyV2ApplyToMesh(actor->GetCosmeticMesh3P(), visible_material);
    }

    void apply_cool_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) return;
        if (!globals::chams::cool_chams_enabled) return;

        uobject* cool_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!cool_material || !memory::IsValidPointer((uintptr_t)cool_material)) return;

        flinearcolor cool_color = flinearcolor(
            globals::chams::coolChamsR,
            globals::chams::coolChamsG,
            globals::chams::coolChamsB,
            globals::chams::coolChamsGlow
        );

        auto apply_on_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            auto num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, cool_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(string::string_to_name(L"SilohuetteColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"CenterEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"InnerEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"OuterEdgeColor"), cool_color);
                dynCast->set_scalar_parameter_value(string::string_to_name(L"GlowIntensity"), globals::chams::coolChamsGlow);
            }
        };

        apply_on_mesh(actor->get_mesh());
        apply_on_mesh(actor->GetCosmeticMesh3P());
    }

    void apply_hand_cool_chams(acknowledgedpawn* pawn)
    {
        if (!pawn) return;
        if (!globals::chams::hand_cool_chams_enabled) return;

        uobject* cool_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!cool_material || !memory::IsValidPointer((uintptr_t)cool_material)) return;

        flinearcolor cool_color = flinearcolor(
            globals::chams::coolChamsR,
            globals::chams::coolChamsG,
            globals::chams::coolChamsB,
            globals::chams::coolChamsGlow
        );

        auto apply_on_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            auto num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, cool_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(string::string_to_name(L"SilohuetteColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"CenterEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"InnerEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"OuterEdgeColor"), cool_color);
                dynCast->set_scalar_parameter_value(string::string_to_name(L"GlowIntensity"), globals::chams::coolChamsGlow);
            }
        };

        apply_on_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p));
        apply_on_mesh(memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay));
    }

    void apply_self_cool_chams(ashootercharacter* character)
    {
        if (!character || !character->is_alive()) return;
        if (!globals::chams::self_cool_chams_enabled) return;

        uobject* cool_material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
        );
        if (!cool_material || !memory::IsValidPointer((uintptr_t)cool_material)) return;

        flinearcolor cool_color = flinearcolor(
            globals::chams::coolChamsR,
            globals::chams::coolChamsG,
            globals::chams::coolChamsB,
            globals::chams::coolChamsGlow
        );

        auto apply_on_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            auto num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, cool_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(string::string_to_name(L"SilohuetteColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"CenterEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"InnerEdgeColor"), cool_color);
                dynCast->set_vector_parameter_value1(string::string_to_name(L"OuterEdgeColor"), cool_color);
                dynCast->set_scalar_parameter_value(string::string_to_name(L"GlowIntensity"), globals::chams::coolChamsGlow);
            }
        };

        apply_on_mesh(character->get_mesh());
        apply_on_mesh(character->GetCosmeticMesh3P());
    }

    // Sentinel: turuncu mukemmel — mavi ayni HunterReveal + AYNI uc kanal (inv_c/inv_i/inv_o ile ayni rol: merkez / parlak ic / parlak dis kenar). Silo=dis icin koyu "cerceve" vermek maviyi kiriyordu.
    void apply_sentinel_enemy_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor)) return;
        if (!actor->is_alive()) return;
        if (!IsValidUObject((uobject*)actor)) return;

        const bool want_vis = globals::chams::sentinel_chams_visible_enemy;
        const bool want_inv = globals::chams::sentinel_chams_invisible_enemy;
        if (!want_vis && !want_inv) return;

        uobject* mat_hunter = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/VFX/Materials/HunterReveal_MI.HunterReveal_MI"
        );
        if (!mat_hunter || !memory::IsValidPointer((uintptr_t)mat_hunter)) return;

        const bool is_visible = [&]() -> bool {
            __try { return controllers->line_of_sight(actor); }
            __except (EXCEPTION_EXECUTE_HANDLER) { return true; }
        }();

        static fname silo_name, center_name, inner_name, outer_name, glow_name;
        if (!silo_name.comparison_index) {
            silo_name   = string::string_to_name(L"SilohuetteColor");
            center_name = string::string_to_name(L"CenterEdgeColor");
            inner_name  = string::string_to_name(L"InnerEdgeColor");
            outer_name  = string::string_to_name(L"OuterEdgeColor");
            glow_name   = string::string_to_name(L"GlowIntensity");
        }

        flinearcolor vis_c(0.04f, 0.2f, 0.7f, 12.0f);
        flinearcolor vis_i(0.1f, 0.45f, 0.95f, 14.0f);
        flinearcolor vis_o(0.0f, 0.3f, 0.99f, 12.0f);
        flinearcolor inv_c(0.72f, 0.22f, 0.0f, 12.0f);
        flinearcolor inv_i(0.95f, 0.38f, 0.02f, 14.0f);
        flinearcolor inv_o(0.99f, 0.12f, 0.0f, 12.0f);
        const float g_vis = 12.0f;
        const float g_inv = 12.0f;

        auto apply_to_mesh = [&](uskeletalmeshcomponent* mesh, uobject* mat,
            const flinearcolor& center, const flinearcolor& inner, const flinearcolor& silo_and_outer, float gl) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            if (!IsValidUObject((uobject*)mesh)) return;
            if (!mat || !memory::IsValidPointer((uintptr_t)mat)) return;
            __try {
                int n = mesh->get_num_materials();
                if (n <= 0 || n > 50) return;
                for (int i = 0; i < n; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, mat);
                    if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                    auto dyn = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dyn || !memory::IsValidPointer((uintptr_t)dyn)) continue;
                    dyn->set_vector_parameter_value1(silo_name, silo_and_outer);
                    dyn->set_vector_parameter_value1(center_name, center);
                    dyn->set_vector_parameter_value1(inner_name, inner);
                    dyn->set_vector_parameter_value1(outer_name, silo_and_outer);
                    dyn->set_scalar_parameter_value(glow_name, gl);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                LOG_CRASH("sentinel_enemy apply_to_mesh EXCEPTION mesh=0x%llX", (unsigned long long)mesh);
            }
        };

        auto reset_both = [&]() {
            __try {
                if (auto* m = seh_bot_get_mesh(actor); m && memory::IsValidPointer((uintptr_t)m) && IsValidUObject((uobject*)m))
                    actor->reset_character_materials_internal(m);
                if (auto* c = seh_bot_get_cosmetic_mesh(actor); c && memory::IsValidPointer((uintptr_t)c) && IsValidUObject((uobject*)c))
                    actor->reset_character_materials_internal(c);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                LOG_CRASH("sentinel_enemy reset_both EXCEPTION");
            }
        };

        if (want_vis && want_inv) {
            if (is_visible) {
                apply_to_mesh(seh_bot_get_mesh(actor), mat_hunter, vis_c, vis_i, vis_o, g_vis);
                apply_to_mesh(seh_bot_get_cosmetic_mesh(actor), mat_hunter, vis_c, vis_i, vis_o, g_vis);
            } else {
                apply_to_mesh(seh_bot_get_mesh(actor), mat_hunter, inv_c, inv_i, inv_o, g_inv);
                apply_to_mesh(seh_bot_get_cosmetic_mesh(actor), mat_hunter, inv_c, inv_i, inv_o, g_inv);
            }
        } else if (want_vis && !want_inv) {
            if (!is_visible) {
                reset_both();
                return;
            }
            apply_to_mesh(seh_bot_get_mesh(actor), mat_hunter, vis_c, vis_i, vis_o, g_vis);
            apply_to_mesh(seh_bot_get_cosmetic_mesh(actor), mat_hunter, vis_c, vis_i, vis_o, g_vis);
        } else {
            if (is_visible) {
                reset_both();
                return;
            }
            apply_to_mesh(seh_bot_get_mesh(actor), mat_hunter, inv_c, inv_i, inv_o, g_inv);
            apply_to_mesh(seh_bot_get_cosmetic_mesh(actor), mat_hunter, inv_c, inv_i, inv_o, g_inv);
        }
    }

    void apply_sentinel_self_chams(ashootercharacter* character)
    {
        if (!character) return;
        if (!memory::IsValidPointer((uintptr_t)character) || !character->is_alive()) return;
        if (is_weapon_swap_grace_active()) return;
        if (!IsValidUObject((uobject*)character)) return;

        const bool want_vis = globals::chams::sentinel_chams_visible_self;
        const bool want_inv = globals::chams::sentinel_chams_invisible_self;
        if (!want_vis && !want_inv) return;

        uobject* mat_hunter = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/VFX/Materials/HunterReveal_MI.HunterReveal_MI"
        );
        if (!mat_hunter || !memory::IsValidPointer((uintptr_t)mat_hunter)) return;

        static fname silo_name, center_name, inner_name, outer_name, glow_name;
        if (!silo_name.comparison_index) {
            silo_name   = string::string_to_name(L"SilohuetteColor");
            center_name = string::string_to_name(L"CenterEdgeColor");
            inner_name  = string::string_to_name(L"InnerEdgeColor");
            outer_name  = string::string_to_name(L"OuterEdgeColor");
            glow_name   = string::string_to_name(L"GlowIntensity");
        }

        flinearcolor vis_c(0.04f, 0.2f, 0.7f, 12.0f);
        flinearcolor vis_i(0.1f, 0.45f, 0.95f, 14.0f);
        flinearcolor vis_o(0.0f, 0.3f, 0.99f, 12.0f);
        flinearcolor inv_c(0.72f, 0.22f, 0.0f, 12.0f);
        flinearcolor inv_i(0.95f, 0.38f, 0.02f, 14.0f);
        flinearcolor inv_o(0.99f, 0.12f, 0.0f, 12.0f);

        const flinearcolor* center = nullptr;
        const flinearcolor* inner  = nullptr;
        const flinearcolor* outer  = nullptr;
        float glow = 12.0f;

        if (want_vis && !want_inv) {
            center = &vis_c; inner = &vis_i; outer = &vis_o;
        } else if (!want_vis && want_inv) {
            center = &inv_c; inner = &inv_i; outer = &inv_o;
        } else {
            center = &vis_c; inner = &vis_i; outer = &vis_o;
        }

        auto apply_to_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            if (!IsValidUObject((uobject*)mesh)) return;
            __try {
                int n = mesh->get_num_materials();
                if (n <= 0 || n > 50) return;
                for (int i = 0; i < n; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, mat_hunter);
                    if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                    auto dyn = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    if (!dyn || !memory::IsValidPointer((uintptr_t)dyn)) continue;
                    dyn->set_vector_parameter_value1(silo_name, *outer);
                    dyn->set_vector_parameter_value1(center_name, *center);
                    dyn->set_vector_parameter_value1(inner_name, *inner);
                    dyn->set_vector_parameter_value1(outer_name, *outer);
                    dyn->set_scalar_parameter_value(glow_name, glow);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                LOG_CRASH("sentinel_self apply_to_mesh EXCEPTION mesh=0x%llX", (unsigned long long)mesh);
            }
        };

        // 1P arms in first person; 3P body only when third person (never both).
        // SEH getters + mesh safety — raw mesh access during 3P toggle crashes (AV @ +0x60).
        if (globals::misc::tperson) {
            uskeletalmeshcomponent* core = seh_fp_mesh3p(character);
            uskeletalmeshcomponent* body = seh_bot_get_mesh(character);
            uskeletalmeshcomponent* cos = seh_bot_get_cosmetic_mesh(character);
            if (core && is_mesh_component_safe((uintptr_t)core)) apply_to_mesh(core);
            if (body && is_mesh_component_safe((uintptr_t)body)) apply_to_mesh(body);
            if (cos && is_mesh_component_safe((uintptr_t)cos)) apply_to_mesh(cos);
        } else {
            uskeletalmeshcomponent* m1 = seh_fp_getmesh1p(character);
            uskeletalmeshcomponent* ov = seh_fp_get_overlay1p(character);
            if (m1 && is_mesh_component_safe((uintptr_t)m1)) apply_to_mesh(m1);
            if (ov && is_mesh_component_safe((uintptr_t)ov)) apply_to_mesh(ov);
        }
    }

    static void seh_apply_sentinel_self_chams(ashootercharacter* character) {
        if (!character) return;
        __try {
            apply_sentinel_self_chams(character);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("apply_sentinel_self_chams EXCEPTION");
        }
    }

    void apply_sentinel_enemy_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers);
    static void seh_apply_sentinel_enemy_chams(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers) {
        if (!pawn || !actor || !controllers) return;
        __try {
            apply_sentinel_enemy_chams(pawn, actor, controllers);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("apply_sentinel_enemy_chams EXCEPTION actor=0x%llX", (unsigned long long)actor);
        }
    }

    void apply_black_chams_enemy(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers);
    static void seh_apply_black_chams_enemy(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers) {
        if (!pawn || !actor || !controllers) return;
        __try {
            apply_black_chams_enemy(pawn, actor, controllers);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("apply_black_chams_enemy EXCEPTION actor=0x%llX", (unsigned long long)actor);
        }
    }

    void apply_black_chams_enemy(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!pawn || !actor || !controllers) return;
        if (!memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) return;
        if (!IsValidUObject((uobject*)actor)) return;
        if (!globals::chams::black_chams_enemy) return;

        static uobject* mat = nullptr;
        static bool mat_load_failed = false;
        if (!mat && !mat_load_failed) {
            mat = uobject::static_load_object(
                nullptr, nullptr,
                L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
            );
            if (!mat || !memory::IsValidPointer((uintptr_t)mat)) {
                mat_load_failed = true;
                LOG_ERR("black_chams: BountyHunterReveal material load FAILED");
            }
        }
        if (!mat || !memory::IsValidPointer((uintptr_t)mat)) return;

        const int chams_type = globals::chams::black_chams_type > 0
            ? globals::chams::black_chams_type
            : (globals::chams::black_chams_rgb ? 1 : 0);

        auto apply_black = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            if (!IsValidUObject((uobject*)mesh)) return;
            seh_galaxy_v2_apply_black_to_mesh(mesh, mat, chams_type);
        };

        apply_black(seh_bot_get_mesh(actor));
        apply_black(seh_bot_get_cosmetic_mesh(actor));
        apply_black(seh_fp_mesh3p(actor));
    }

    static float enemy_fresnel_rainbow_time = 0.0f;
    
    void apply_enemy_fresnel(acknowledgedpawn* pawn, ashootercharacter* actor, aplayercontroller* controllers)
    {
        if (!actor || !IsValidUObject((uobject*)actor) || !actor->is_alive()) return;
        if (!globals::chams::custom_fresnel) return;

        static fname tint_name = string::string_to_name(L"Fresnel Enemy Tint");
        static fname intensity_name = string::string_to_name(L"Fresnel Intensity");

        flinearcolor colorToApply;
        if (globals::chams::rainbow_fresnel) {
            enemy_fresnel_rainbow_time += globals::chams::enemy_fresnel_rainbow_speed;
            colorToApply = GetRainbowColor(enemy_fresnel_rainbow_time);
        }
        else {
            colorToApply = flinearcolor(globals::chams::enemy_fresnel_r, globals::chams::enemy_fresnel_g, globals::chams::enemy_fresnel_b, 1.0f);
        }

        float intensity = globals::chams::enemy_fresnel_intensity;
        flinearcolor scaledColor = flinearcolor(colorToApply.r * intensity, colorToApply.g * intensity, colorToApply.b * intensity, 1.0f);

        tarray<UMaterialInstanceDynamic*> mids = memory::read<tarray<UMaterialInstanceDynamic*>>((uintptr_t)actor + offsets::Mesh3PMIDs);
        if (mids.size() <= 0 || mids.size() > 64 || !memory::IsValidPointer((uintptr_t)mids.data)) return;
        for (int i = 0; i < mids.size(); i++) {
            if (auto instance = mids[i]) {
                if (memory::IsValidPointer((uintptr_t)instance) && IsValidUObject((uobject*)instance)) {
                    instance->set_vector_parameter_value(tint_name, scaledColor);
                    instance->set_scalar_parameter_value(intensity_name, static_cast<double>(intensity));
                }
            }
        }
    }

    void apply_hand_fresnel(acknowledgedpawn* pawn, ashootercharacter* character)
    {
        if (!character || !IsValidUObject((uobject*)character) || !character->is_alive()) return;
        if (!globals::chams::hand_fresnel_enabled) return;

        static fname ally_tint_name = string::string_to_name(L"Fresnel Ally Tint");
        static fname intensity_name = string::string_to_name(L"Fresnel Intensity");

        float intensity = globals::chams::hand_fresnel_intensity;
        flinearcolor scaledColor = flinearcolor(
            globals::chams::hand_fresnel_r * intensity,
            globals::chams::hand_fresnel_g * intensity,
            globals::chams::hand_fresnel_b * intensity,
            1.0f
        );

        // Hand fresnel uses 1P Overlay MIDs
        tarray<UMaterialInstanceDynamic*> mids = memory::read<tarray<UMaterialInstanceDynamic*>>((uintptr_t)character + offsets::Mesh1POverlayMIDs);
        for (int i = 0; i < mids.size(); i++) {
            if (auto instance = mids[i]) {
                if (IsValidUObject((uobject*)instance)) {
                    instance->set_vector_parameter_value(ally_tint_name, scaledColor);
                    instance->set_scalar_parameter_value(intensity_name, static_cast<double>(intensity));
                }
            }
        }

        // Also apply to weapon mesh
        tarray<UMaterialInstanceDynamic*> weapon_mids = memory::read<tarray<UMaterialInstanceDynamic*>>((uintptr_t)character + offsets::Mesh1PMIDs);
        for (int i = 0; i < weapon_mids.size(); i++) {
            if (auto instance = weapon_mids[i]) {
                if (IsValidUObject((uobject*)instance)) {
                    instance->set_vector_parameter_value(ally_tint_name, scaledColor);
                    instance->set_scalar_parameter_value(intensity_name, static_cast<double>(intensity));
                }
            }
        }
    }

    void apply_fresnel_shi(ashootercharacter* actor)
    {
        if (!actor || !IsValidUObject((uobject*)actor)) return;
        if (!globals::chams::fresnel_shi_enabled) return;
        if (!actor->is_alive()) return;

        uintptr_t fresnelComp = memory::read<uintptr_t>((uintptr_t)actor + offsets::fresnelcomponent);
        if (fresnelComp && IsValidUObject((uobject*)fresnelComp)) {
            static uobject* setIntensityFn = nullptr;
            if (!setIntensityFn)
                setIntensityFn = uobject::find_object<uobject*>(L"ShooterGame.FresnelComponent.SetIntensity");
            if (setIntensityFn) {
                struct { float val; } params;
                params.val = globals::chams::fresnel_shi_intensity;
                __try { ((uobject*)fresnelComp)->process_event(setIntensityFn, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { }
            }
        }
    }

    void apply_self_3p_fresnel(ashootercharacter* character)
    {
        if (!character || !IsValidUObject((uobject*)character) || !character->is_alive()) return;
        if (!globals::chams::self_3p_fresnel_enabled) return;

        static fname ally_tint_name = string::string_to_name(L"Fresnel Ally Tint");
        static fname intensity_name = string::string_to_name(L"Fresnel Intensity");

        float intensity = globals::chams::self_3p_fresnel_intensity;
        flinearcolor scaledColor = flinearcolor(
            globals::chams::self_3p_fresnel_r * intensity,
            globals::chams::self_3p_fresnel_g * intensity,
            globals::chams::self_3p_fresnel_b * intensity,
            1.0f
        );

        tarray<UMaterialInstanceDynamic*> mids = memory::read<tarray<UMaterialInstanceDynamic*>>((uintptr_t)character + offsets::Mesh3PMIDs);
        if (mids.size() <= 0 || mids.size() > 64 || !memory::IsValidPointer((uintptr_t)mids.data)) return;
        for (int i = 0; i < mids.size(); i++) {
            if (auto instance = mids[i]) {
                if (memory::IsValidPointer((uintptr_t)instance) && IsValidUObject((uobject*)instance)) {
                    instance->set_vector_parameter_value(ally_tint_name, scaledColor);
                    instance->set_scalar_parameter_value(intensity_name, static_cast<double>(intensity));
                }
            }
        }
    }






#include <string>

#pragma pack(push, 1)
    union fp_flag_store {
        unsigned char raw;
        struct {
            unsigned char f0 : 1;
            unsigned char f1 : 1;
            unsigned char f2 : 6;
        } bits;
    };
#pragma pack(pop)

    struct ViewModelCache {
        uskeletalmeshcomponent* mesh1p = nullptr;
        uskeletalmeshcomponent* overlayMesh = nullptr;
        uskeletalmeshcomponent* weaponMesh1P = nullptr;
        uskeletalmeshcomponent* cosmeticMesh1P = nullptr;
        uskeletalmeshcomponent* meleeMesh1P = nullptr;
        uskeletalmeshcomponent* offHandMesh = nullptr;
        currentequippable* lastWeapon = nullptr;
        currentequippable* lastMelee = nullptr;
        ULONGLONG lastCacheTime = 0;

        void Clear() {
            mesh1p = overlayMesh = weaponMesh1P = cosmeticMesh1P = nullptr;
            meleeMesh1P = offHandMesh = nullptr;
            lastWeapon = lastMelee = nullptr;
        }

        bool IsValid() const {
            return mesh1p != nullptr && weaponMesh1P != nullptr;
        }
    };

    static bool g_vm_invalidate_cache = false;

    inline void InvalidateViewModelCache() {
        g_vm_invalidate_cache = true;
    }

    inline void NotifyViewModelWeaponChange() {
        InvalidateViewModelCache();
    }

    static bool is_weapon_ready_for_ops(currentequippable* weapon, const std::wstring& name) {
        if (!weapon || !memory::IsValidPointer((uintptr_t)weapon)) return false;
        if (!IsValidUObject((uobject*)weapon)) return false;
        if (name.empty() || name == L"Invalid") return false;
        return true;
    }

    inline bool IsValidViewModelPointer(uintptr_t ptr) {
        if (ptr == 0 || ptr == (uintptr_t)-1 || ptr < 0x10000 || ptr > 0x7FFFFFFFFFFF) {
            return false;
        }

        __try {
            volatile unsigned char test = *(unsigned char*)ptr;
            (void)test;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool IsValidViewModelObject(void* obj) {
        if (!obj) return false;
        return IsValidViewModelPointer((uintptr_t)obj);
    }

    template<typename T>
    inline bool SafeRead(uintptr_t address, T& value) {
        if (!IsValidViewModelPointer(address)) return false;

        __try {
            value = *reinterpret_cast<T*>(address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    template<typename T>
    inline bool SafeWrite(uintptr_t address, const T& value) {
        if (!IsValidViewModelPointer(address)) return false;

        __try {
            *reinterpret_cast<T*>(address) = value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    inline bool SafeProcessFlag(uskeletalmeshcomponent* mesh, uintptr_t flag_offset, bool clear_flag) {
        if (!mesh || !IsValidViewModelObject(mesh)) return false;

        uintptr_t flag_addr = (uintptr_t)mesh + flag_offset;
        if (!IsValidViewModelPointer(flag_addr)) return false;

        fp_flag_store state;
        if (!SafeRead(flag_addr, state.raw)) return false;

        if (clear_flag) {
            state.bits.f0 = 0;
            state.bits.f1 = 0;
            return SafeWrite(flag_addr, state.raw);
        }

        return true;
    }

    // GetChildrenComponents removed: It caused severe memory leaks (ProcessEvent TArray return) leading to round 4 crashes.
    // Instead we will recursively read the AttachChildren property (0xC8) directly.
    // GetChildrenComponentsRecursive removed in favor of engine-native call from 3.txt
    // Forward decl so we can SEH-wrap process_event below (full def is later in file).
    static bool PbtSafeProcessEvent(uobject* obj, uobject* fn, void* params);

    inline tarray<USceneComponent*> GetChildrenComponents(USceneComponent* component, bool bIncludeAllDescendants) {
        tarray<USceneComponent*> result;
        if (!component || !IsValidViewModelObject(component)) return result;

        static uobject* Function = nullptr;
        if (!Function) {
            auto function_name = (L"Engine.SceneComponent.GetChildrenComponents");
            Function = uobject::find_object<uobject*>(function_name);
        }

        if (!Function || !IsValidViewModelObject(Function)) return result;

        struct {
            bool bIncludeAllDescendants;
            tarray<USceneComponent*> Children;
        } Args;

        Args.bIncludeAllDescendants = bIncludeAllDescendants;
        Args.Children.data = nullptr;
        Args.Children.count = 0;
        Args.Children.maxCount = 0;

        // C2712: tarray+__try ayni fonksiyonda yasak; process_event cagrisi (IsValidViewModelObject sonrasi)
        // SEH wrap'i ayri (helper) fonksiyonda yapariz ki tarray donus tipi conflict olmasin.
        if (!component || !memory::IsValidPointer((uintptr_t)component) || !Function) return Args.Children;
        PbtSafeProcessEvent((uobject*)component, Function, &Args);
        return Args.Children;
    }

    static bool PbtSafeProcessEvent(uobject* obj, uobject* fn, void* params) {
        if (!obj || !fn || !params) return false;
        if (!memory::IsValidPointer((uintptr_t)obj) || !memory::IsValidPointer((uintptr_t)fn)) return false;
        __try {
            obj->process_event(fn, params);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    currentequippable* GetLocalMeleeWeapon() {
        static currentequippable* CachedMelee = nullptr;
        static ULONGLONG lastCacheTime = 0;

        ULONGLONG currentTime = GetTickCount64();
        // Cache hit: also revalidate - the object may have been freed between GC runs.
        if (CachedMelee && (currentTime - lastCacheTime) < 2000 &&
            memory::IsValidPointer((uintptr_t)CachedMelee) && IsValidUObject((uobject*)CachedMelee)) {
            return CachedMelee;
        }

        CachedMelee = nullptr;
        if (!UWorldSave) return nullptr;

        tarray<AGameObject*> Objects;
        GameplayStatics::GetAllActorsOfClass2(UWorldSave, Class::Actors(), &Objects);

        for (int i = 0; i < Objects.size(); ++i) {
            AGameObject* Object = Objects[i];
            if (!Object) continue;

            auto name = system::get_object_name(Object);
            if (!name.is_valid()) continue;

            std::string name_str = name.ToString();
            if (name_str.find("Ability_Melee_Base_C") != std::string::npos) {
                CachedMelee = (currentequippable*)Object;
                lastCacheTime = currentTime;
                return CachedMelee;
            }
        }

        return nullptr;
    }


    // Walk AttachChildren (0xC8) without ProcessEvent. Only clear fp flags on
    // components that look like real meshes — writing +0x364 on plain SceneComponents
    // corrupts pointers (AV read 0x00007f00...).
    static int seh_fp_num_materials(uintptr_t obj) {
        if (!obj) return -1;
        __try { return reinterpret_cast<UPrimitiveComponent*>(obj)->get_num_materials(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
    }

    inline void SafeLockDescendants(USceneComponent* component) {
        if (!component || !IsValidViewModelObject(component)) return;

        tarray<USceneComponent*> allChildren = GetChildrenComponents(component, true);

        if (!memory::IsValidPointer((uintptr_t)allChildren.data)) return;
        if (allChildren.count <= 0 || allChildren.count > 1000) return;

        for (int i = 0; i < allChildren.count; i++) {
            USceneComponent* child = nullptr;
            uintptr_t child_ptr_addr = (uintptr_t)allChildren.data + (i * sizeof(void*));
            if (!SafeRead(child_ptr_addr, child)) continue;

            if (child && IsValidViewModelObject(child)) {
                uintptr_t child_flag_addr = (uintptr_t)child + offsets::mesh_fp_flag;
                SafeWrite(child_flag_addr, (unsigned char)0x00);
            }
        }
    }

    // C2712: process_fp_mode icinde tarray/ViewModelCache + __try birlikte yok; cagri SEH baska fonksiyonda
    static bool seh_fp_is_alive(ashootercharacter* s) {
        if (!s) return false;
        __try { return s->is_alive(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static uinventory* seh_fp_get_inventory(ashootercharacter* s) {
        if (!s) return nullptr;
        __try { return s->get_inventory(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static currentequippable* seh_fp_get_equippable(uinventory* inv) {
        if (!inv) return nullptr;
        __try { return inv->get_current_equippable(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static uskeletalmeshcomponent* seh_fp_getmesh1p(ashootercharacter* s) {
        if (!s) return nullptr;
        __try { return s->getmesh1p(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static uskeletalmeshcomponent* seh_fp_get_overlay1p(ashootercharacter* s) {
        if (!s) return nullptr;
        __try { return s->GetOverlayMesh1P(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static bool seh_fp_is_zoomed(ashootercharacter* s) {
        if (!s) return false;
        __try { return s->IsZoomed(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static uskeletalmeshcomponent* seh_fp_mesh3p(ashootercharacter* s) {
        if (!s) return nullptr;
        __try {
            uskeletalmeshcomponent* m = s->mesh3p();
            if (m && memory::IsValidPointerFast((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        return nullptr;
    }
    static uskeletalmeshcomponent* seh_fp_weapon_get_mesh1p(currentequippable* w) {
        if (!w) return nullptr;
        __try { return w->GetMesh1P(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static uskeletalmeshcomponent* seh_fp_weapon_get_equippable_mesh(currentequippable* w) {
        if (!w) return nullptr;
        __try { return w->GetEquippableMesh(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static uskeletalmeshcomponent* seh_fp_read_mesh_at(uintptr_t addr) {
        if (!IsValidViewModelPointer(addr)) return nullptr;
        __try { return memory::read<uskeletalmeshcomponent*>(addr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static bool seh_fp_probe_shooter(uintptr_t shooter_ptr) {
        __try {
            volatile uintptr_t test = *(uintptr_t*)shooter_ptr;
            (void)test;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    struct fp_transform_snap {
        fvector rel_loc{};
        frotator rel_rot{};
        bool valid = false;
    };

    static void SnapReadRelativeTransform(USceneComponent* comp, fp_transform_snap& out) {
        out.valid = false;
        if (!comp || !IsValidViewModelObject(comp)) return;
        const uintptr_t base = (uintptr_t)comp;
        if (!SafeRead(base + 0x128, out.rel_loc)) return;
        if (!SafeRead(base + 0x140, out.rel_rot)) return;
        out.valid = true;
    }

    static void SnapWriteRelativeTransform(USceneComponent* comp, const fp_transform_snap& snap) {
        if (!snap.valid || !comp || !IsValidViewModelObject(comp)) return;
        const uintptr_t base = (uintptr_t)comp;
        SafeWrite(base + 0x128, snap.rel_loc);
        SafeWrite(base + 0x140, snap.rel_rot);
    }

    inline void process_fp_mode(ashootercharacter* shooter) {
        if (!globals::misc::ViewModelChanger) return;
        if (!shooter) return;

        uintptr_t shooter_ptr = (uintptr_t)shooter;
        if (shooter_ptr == 0 || shooter_ptr == (uintptr_t)-1 || shooter_ptr < 0x10000 || shooter_ptr > 0x7FFFFFFFFFFF) {
            return;
        }
        if (!seh_fp_probe_shooter(shooter_ptr)) return;

        const bool isAlive = seh_fp_is_alive(shooter);

        static ViewModelCache cache;

        if (g_vm_invalidate_cache) {
            cache.Clear();
            g_vm_invalidate_cache = false;
        }

        if (!isAlive) {
            cache.Clear();
            return;
        }

        static bool last_force_key_state = false;
        ULONGLONG current_time = GetTickCount64();
        bool force_key_pressed = GetAsyncKeyState(VK_F8) & 0x8000;
        bool force_reapply = force_key_pressed && !last_force_key_state;
        last_force_key_state = force_key_pressed;

        uinventory* inventory = seh_fp_get_inventory(shooter);
        if (!inventory || !IsValidViewModelObject(inventory)) {
            cache.Clear();
            return;
        }

        currentequippable* weapon = seh_fp_get_equippable(inventory);
        if (!weapon || !IsValidViewModelObject(weapon)) {
            cache.Clear();
            return;
        }

        bool needs_refresh = false;
        if (!cache.IsValid() ||
            weapon != cache.lastWeapon ||
            (current_time - cache.lastCacheTime) >= 2000 ||
            force_reapply) {
            needs_refresh = true;
        }

        if (needs_refresh) {
            cache.Clear();
            cache.lastCacheTime = current_time;
            cache.lastWeapon = weapon;

            cache.mesh1p = seh_fp_getmesh1p(shooter);
            if (!cache.mesh1p || !IsValidViewModelObject(cache.mesh1p)) {
                cache.Clear();
                return;
            }

            cache.overlayMesh = seh_fp_get_overlay1p(shooter);

            cache.weaponMesh1P = seh_fp_weapon_get_mesh1p(weapon);
            if (!cache.weaponMesh1P || !IsValidViewModelObject(cache.weaponMesh1P)) {
                cache.Clear();
                return;
            }

            uintptr_t cosmetic_ptr = (uintptr_t)weapon + 0x1180;
            if (IsValidViewModelPointer(cosmetic_ptr)) {
                cache.cosmeticMesh1P = seh_fp_read_mesh_at(cosmetic_ptr);
            }

            auto meleeWeapon = GetLocalMeleeWeapon();
            if (meleeWeapon && IsValidViewModelObject(meleeWeapon)) {
                cache.lastMelee = meleeWeapon;

                cache.meleeMesh1P = seh_fp_weapon_get_mesh1p(meleeWeapon);
                if (cache.meleeMesh1P) {
                    uintptr_t offhand_ptr = (uintptr_t)meleeWeapon + 0x1210;
                    if (IsValidViewModelPointer(offhand_ptr)) {
                        cache.offHandMesh = seh_fp_read_mesh_at(offhand_ptr);
                    }
                }
            }
        }

        if (!cache.IsValid() || !cache.mesh1p || !cache.weaponMesh1P) {
            return;
        }

        static fp_transform_snap s_hip_weapon_tf;
        static fp_transform_snap s_hip_cosmetic_tf;
        static fp_transform_snap s_hip_overlay_tf;

        const uintptr_t flag_offset = offsets::mesh_fp_flag;

        bool should_process_flags = false;
        fp_flag_store main_state;

        const bool custom_scope_lock =
            globals::misc::custom_scope && seh_fp_is_zoomed(shooter);

        if (custom_scope_lock) {
            if (s_hip_weapon_tf.valid)
                SnapWriteRelativeTransform((USceneComponent*)cache.weaponMesh1P, s_hip_weapon_tf);
            if (s_hip_cosmetic_tf.valid && cache.cosmeticMesh1P)
                SnapWriteRelativeTransform((USceneComponent*)cache.cosmeticMesh1P, s_hip_cosmetic_tf);
            if (s_hip_overlay_tf.valid && cache.overlayMesh)
                SnapWriteRelativeTransform((USceneComponent*)cache.overlayMesh, s_hip_overlay_tf);
        } else {
            SnapReadRelativeTransform((USceneComponent*)cache.weaponMesh1P, s_hip_weapon_tf);
            if (cache.cosmeticMesh1P)
                SnapReadRelativeTransform((USceneComponent*)cache.cosmeticMesh1P, s_hip_cosmetic_tf);
            if (cache.overlayMesh)
                SnapReadRelativeTransform((USceneComponent*)cache.overlayMesh, s_hip_overlay_tf);
        }

        uintptr_t mesh_flag_addr = (uintptr_t)cache.mesh1p + flag_offset;
        if (SafeRead(mesh_flag_addr, main_state.raw)) {
            should_process_flags = custom_scope_lock || (main_state.bits.f0 != 0) || force_reapply;
        } else if (custom_scope_lock) {
            should_process_flags = true;
        }

        if (should_process_flags) {
            SafeProcessFlag(cache.mesh1p, flag_offset, true);

            if (cache.overlayMesh && IsValidViewModelObject(cache.overlayMesh)) {
                SafeProcessFlag(cache.overlayMesh, flag_offset, true);
            }

            // Skip weapon mesh flag clearing when custom skins are active
            // so the original weapon mesh stays hidden behind the custom .obj model
            if (!globals::misc::custom_vandal_enabled) {
                if (cache.weaponMesh1P && IsValidViewModelObject(cache.weaponMesh1P)) {
                    SafeProcessFlag(cache.weaponMesh1P, flag_offset, true);
                }

                if (cache.cosmeticMesh1P && IsValidViewModelObject(cache.cosmeticMesh1P)) {
                    SafeProcessFlag(cache.cosmeticMesh1P, flag_offset, true);
                }
            }

            if (cache.meleeMesh1P && IsValidViewModelObject(cache.meleeMesh1P)) {
                SafeProcessFlag(cache.meleeMesh1P, flag_offset, true);
            }

            if (cache.offHandMesh && IsValidViewModelObject(cache.offHandMesh)) {
                SafeProcessFlag(cache.offHandMesh, flag_offset, true);
            }
        }

        if (cache.mesh1p && IsValidViewModelObject(cache.mesh1p)) {
            SafeLockDescendants((USceneComponent*)cache.mesh1p);
        }

        if (!globals::misc::custom_vandal_enabled) {
            if (cache.weaponMesh1P && IsValidViewModelObject(cache.weaponMesh1P)) {
                SafeLockDescendants((USceneComponent*)cache.weaponMesh1P);
            }
        }

        if (cache.meleeMesh1P && IsValidViewModelObject(cache.meleeMesh1P)) {
            SafeLockDescendants((USceneComponent*)cache.meleeMesh1P);
        }
    }

    bool IsValidUObject(uobject* obj)
    {
        if (!obj) return false;
        if (!memory::IsValidPointer((uintptr_t)obj)) return false;


        void* vtable = *(void**)obj;
        return memory::IsValidPointer((uintptr_t)vtable);
    }

    struct USceneComponentHelpers
    {
        static void Detach(void* Target)
        {
            static uobject* fn = nullptr;
            if (!fn)
                fn = uobject::find_object<uobject*>(L"Engine.SceneComponent.K2_DetachFromComponent");

            if (!fn || !Target || !memory::IsValidPointerFast((uintptr_t)Target)) return;

            struct
            {
                int LocationRule;
                int RotationRule;
                int ScaleRule;
                bool bCallModify;
            } params;

            params.LocationRule = 0;
            params.RotationRule = 0;
            params.ScaleRule = 0;
            params.bCallModify = true;

            __try { ((uobject*)Target)->process_event(fn, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        static void K2_DetachFromComponent(void* Target, int LocationRule, int RotationRule, int ScaleRule, bool bCallModify)
        {
            static uobject* Function = nullptr;
            if (!Function)
                Function = uobject::find_object<uobject*>(L"Engine.SceneComponent.K2_DetachFromComponent");

            if (!Function || !Target || !memory::IsValidPointerFast((uintptr_t)Target)) return;

            struct
            {
                int LocationRule;
                int RotationRule;
                int ScaleRule;
                bool bCallModify;
            } params;

            params.LocationRule = LocationRule;
            params.RotationRule = RotationRule;
            params.ScaleRule = ScaleRule;
            params.bCallModify = bCallModify;

            __try { ((uobject*)Target)->process_event(Function, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        static fname GetAttachSocketName(uskeletalmeshcomponent* TargetComponent)
        {
            static uobject* Function = nullptr;
            if (!Function)
                Function = uobject::find_object<uobject*>(L"Engine.SceneComponent.GetAttachSocketName");

            if (!Function || !TargetComponent || !memory::IsValidPointerFast((uintptr_t)TargetComponent)) return fname();

            struct
            {
                fname ReturnValue;
            } params;

            __try { TargetComponent->process_event(Function, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { return fname(); }
            return params.ReturnValue;
        }

        static bool AttachTo(uskeletalmeshcomponent* Target, uskeletalmeshcomponent* Parent, fname SocketName,
            int LocationRule, int RotationRule, int ScaleRule, bool bWeldSimulatedBodies)
        {
            static uobject* Function = nullptr;
            if (!Function)
                Function = uobject::find_object<uobject*>(L"Engine.SceneComponent.K2_AttachToComponent");

            if (!Function || !Target || !memory::IsValidPointerFast((uintptr_t)Target)) return false;

            struct
            {
                void* Parent;
                fname SocketName;
                int LocationRule;
                int RotationRule;
                int ScaleRule;
                bool bWeldSimulatedBodies;
                bool ReturnValue;
            } params;

            params.Parent = Parent;
            params.SocketName = SocketName;
            params.LocationRule = LocationRule;
            params.RotationRule = RotationRule;
            params.ScaleRule = ScaleRule;
            params.bWeldSimulatedBodies = bWeldSimulatedBodies;

            __try { Target->process_event(Function, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
            return params.ReturnValue;
        }

        static void SetRelativeLocation(void* Target, const fvector& Location, bool bSweep = false, bool bTeleport = true)
        {
            static uobject* fn = nullptr;
            if (!fn)
                fn = uobject::find_object<uobject*>(L"Engine.SceneComponent.K2_SetRelativeLocation");

            if (!fn || !Target || !memory::IsValidPointerFast((uintptr_t)Target)) return;

            struct
            {
                fvector NewLocation;
                bool bSweep;
                FHitResult SweepHitResult;
                bool bTeleport;
            } params;

            params.NewLocation = Location;
            params.bSweep = bSweep;
            params.bTeleport = bTeleport;
            memset(&params.SweepHitResult, 0, sizeof(FHitResult));

            __try { ((uobject*)Target)->process_event(fn, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        static void SetRelativeRotation(void* Target, const FRotator& Rotation, bool bSweep = false, bool bTeleport = true)
        {
            static uobject* fn = nullptr;
            if (!fn)
                fn = uobject::find_object<uobject*>(L"Engine.SceneComponent.K2_SetRelativeRotation");

            if (!fn || !Target || !memory::IsValidPointerFast((uintptr_t)Target)) return;

            struct
            {
                FRotator NewRotation;
                bool bSweep;
                FHitResult SweepHitResult;
                bool bTeleport;
            } params;

            params.NewRotation = Rotation;
            params.bSweep = bSweep;
            params.bTeleport = bTeleport;
            memset(&params.SweepHitResult, 0, sizeof(FHitResult));

            __try { ((uobject*)Target)->process_event(fn, &params); } __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

    };


    struct WeaponTransform {
        fvector position;
        frotator rotation;
        fvector scale;
    };
    enum class EAttachmentRule : uint8 {
        KeepRelative = 0,
        KeepWorld = 1,
        SnapToTarget = 2,
        EAttachmentRule_MAX = 3
    };

    WeaponTransform GetTextTransform(const std::wstring& weaponName, int skinIndex) {
        WeaponTransform transform;

        if (weaponName == L"Melee") {
            transform.position = fvector(globals::misc::melee_text_pos_x, globals::misc::melee_text_pos_y, globals::misc::melee_text_pos_z);
            transform.rotation = frotator(globals::misc::melee_text_rot_pitch, globals::misc::melee_text_rot_yaw, globals::misc::melee_text_rot_roll);
            transform.scale = fvector(globals::misc::melee_text_scale_x, globals::misc::melee_text_scale_y, globals::misc::melee_text_scale_z);
        }
        else if (weaponName == L"Vandal") {
            transform.position = fvector(globals::misc::text_pos_x, globals::misc::text_pos_y, globals::misc::text_pos_z);
            transform.rotation = frotator(globals::misc::text_rot_pitch, globals::misc::text_rot_yaw, globals::misc::text_rot_roll);
            transform.scale = fvector(globals::misc::text_scale_x, globals::misc::text_scale_y, globals::misc::text_scale_z);
        }
        else if (weaponName == L"Phantom") {
            transform.position = fvector(globals::misc::phantom_text_pos_x, globals::misc::phantom_text_pos_y, globals::misc::phantom_text_pos_z);
            transform.rotation = frotator(globals::misc::phantom_text_rot_pitch, globals::misc::phantom_text_rot_yaw, globals::misc::phantom_text_rot_roll);
            transform.scale = fvector(globals::misc::phantom_text_scale_x, globals::misc::phantom_text_scale_y, globals::misc::phantom_text_scale_z);
        }
        else if (weaponName == L"Ghost") {
            transform.position = fvector(globals::misc::ghost_text_pos_x, globals::misc::ghost_text_pos_y, globals::misc::ghost_text_pos_z);
            transform.rotation = frotator(globals::misc::ghost_text_rot_pitch, globals::misc::ghost_text_rot_yaw, globals::misc::ghost_text_rot_roll);
            transform.scale = fvector(globals::misc::ghost_text_scale_x, globals::misc::ghost_text_scale_y, globals::misc::ghost_text_scale_z);
        }
        else if (weaponName == L"Frenzy") {
            transform.position = fvector(globals::misc::frenzy_text_pos_x, globals::misc::frenzy_text_pos_y, globals::misc::frenzy_text_pos_z);
            transform.rotation = frotator(globals::misc::frenzy_text_rot_pitch, globals::misc::frenzy_text_rot_yaw, globals::misc::frenzy_text_rot_roll);
            transform.scale = fvector(globals::misc::frenzy_text_scale_x, globals::misc::frenzy_text_scale_y, globals::misc::frenzy_text_scale_z);
        }
        else if (weaponName == L"Spectre") {
            transform.position = fvector(globals::misc::spectre_text_pos_x, globals::misc::spectre_text_pos_y, globals::misc::spectre_text_pos_z);
            transform.rotation = frotator(globals::misc::spectre_text_rot_pitch, globals::misc::spectre_text_rot_yaw, globals::misc::spectre_text_rot_roll);
            transform.scale = fvector(globals::misc::spectre_text_scale_x, globals::misc::spectre_text_scale_y, globals::misc::spectre_text_scale_z);
        }
        else {
            transform.position = fvector(globals::misc::text_pos_x, globals::misc::text_pos_y, globals::misc::text_pos_z);
            transform.rotation = frotator(globals::misc::text_rot_pitch, globals::misc::text_rot_yaw, globals::misc::text_rot_roll);
            transform.scale = fvector(globals::misc::text_scale_x, globals::misc::text_scale_y, globals::misc::text_scale_z);
        }

        return transform;
    }




    static std::map<uintptr_t, bool> WeaponHasCustomMesh;
    static std::map<uintptr_t, UProceduralMeshComponent*> WeaponTextMeshMap;

    static bool text_meshcreated = false;
    static UProceduralMeshComponent* TextMesh = nullptr;

    static int last_vandal_sel = -1;
    static int last_frenzy_sel = -1;
    static int last_ghost_sel = -1;

    struct MeshData {
        tarray<fvector> Vertices;
        tarray<int32_t> Triangles;
        tarray<fvector> Normals;
        tarray<fvector2d> UV0;
        tarray<FColor> VertexColors;
        tarray<FProcMeshTangent> Tangents;
    };

    static std::map<std::string, MeshData> ModelCache;
    static std::map<std::string, uobject*> TextureCache;
    static bool ModelsLoadedToRAM = false;
    static uobject* LastWorldPtr = nullptr;
    static uintptr_t LastWeaponProcessed = 0;


    inline void SetComponentVisibility(USceneComponent* component, bool bNewVisibility, bool bPropagateToChildren) {
        if (!component || !memory::IsValidPointer((uintptr_t)component)) return;
        static uobject* Function = uobject::find_object<uobject*>(L"Engine.SceneComponent.SetVisibility");
        if (!Function) return;
        struct { bool bNewVisibility; bool bPropagateToChildren; } Args = { bNewVisibility, bPropagateToChildren };
        ((uobject*)component)->process_event(Function, &Args);
    }

    inline uskeletalmeshcomponent* FindSightComponent(currentequippable* weapon, uskeletalmeshcomponent* GunMesh1P) {
        if (!weapon || !memory::IsValidPointer((uintptr_t)weapon)) return nullptr;
        if (!GunMesh1P || !memory::IsValidPointer((uintptr_t)GunMesh1P)) return nullptr;
        USceneComponent* sceneComp = reinterpret_cast<USceneComponent*>(GunMesh1P);
        tarray<USceneComponent*> children = GetChildrenComponents(sceneComp, true);
        for (int i = 0; i < children.size(); i++) {
            if (!children[i]) continue;
            fstring childName = system::get_object_name((uobject*)children[i]);
            if (childName.to_str().find("SkeletalMeshComponent_") != std::string::npos) {
                uskeletalmeshcomponent* skelMesh = reinterpret_cast<uskeletalmeshcomponent*>(children[i]);
                UPrimitiveComponent* primComp = reinterpret_cast<UPrimitiveComponent*>(skelMesh);
                if (primComp && memory::IsValidPointer((uintptr_t)primComp)) {
                    if (primComp->get_num_materials() > 0) return skelMesh;
                }
            }
        }
        return nullptr;
    }


#include <thread>
#include <atomic>

    static std::string GetPublicPath() {
        return "C:\\ProgramData\\Microsoft\\Windows\\Caches\\{7D5B-F12A-11E1-B5}\\";
    }

    void DownloadMissingAssets() {
        return;
    }

    const MeshData& ParseOBJFile(const char* filepath) {
        static MeshData empty;
        std::string pathKey(filepath);
        if (ModelCache.count(pathKey)) return ModelCache[pathKey];

        MeshData data;
        std::ifstream file(filepath);
        if (!file.is_open()) return empty;

        std::vector<fvector> temp_vertices;
        std::vector<fvector2d> temp_uvs;
        std::vector<fvector> temp_normals;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            if (type == "v") {
                float x, y, z; iss >> x >> y >> z;
                temp_vertices.push_back(fvector(x * 100.0f, y * 100.0f, z * 100.0f));
            }
            else if (type == "vt") {
                float u, v; iss >> u >> v;
                temp_uvs.push_back(fvector2d(u, 1.0f - v));
            }
            else if (type == "vn") {
                float x, y, z; iss >> x >> y >> z;
                temp_normals.push_back(fvector(x, y, z));
            }
            else if (type == "f") {
                std::string v[3];
                if (!(iss >> v[0] >> v[1] >> v[2])) continue;
                for (int i = 0; i < 3; i++) {
                    std::istringstream vss(v[i]);
                    std::string part;
                    int v_idx = 0, vt_idx = -1, vn_idx = -1;
                    if (std::getline(vss, part, '/')) v_idx = std::stoi(part) - 1;
                    if (std::getline(vss, part, '/')) { if (!part.empty()) vt_idx = std::stoi(part) - 1; }
                    if (std::getline(vss, part, '/')) { if (!part.empty()) vn_idx = std::stoi(part) - 1; }
                    if (v_idx < 0 || v_idx >= (int)temp_vertices.size()) continue;
                    int current_index = data.Vertices.Num();
                    data.Vertices.Add(temp_vertices[v_idx]);
                    data.UV0.Add((vt_idx >= 0 && vt_idx < (int)temp_uvs.size()) ? temp_uvs[vt_idx] : fvector2d(0, 0));
                    data.Normals.Add((vn_idx >= 0 && vn_idx < (int)temp_normals.size()) ? temp_normals[vn_idx] : fvector(0, 0, 1));
                    data.VertexColors.Add(FColor(255, 255, 255, 255));
                    data.Triangles.Add(current_index);
                }
            }
        }
        file.close();

        std::string p = pathKey;
        for (auto& c : p) c = tolower(c);
        if (p.find("sheriff") == std::string::npos) {
            for (int i = 0; i + 2 < data.Triangles.Num(); i += 3) {
                int32_t temp = data.Triangles[i + 1];
                data.Triangles[i + 1] = data.Triangles[i + 2];
                data.Triangles[i + 2] = temp;
            }
        }
        if (data.Vertices.Num() > 0) ModelCache[pathKey] = std::move(data);
        if (ModelCache.count(pathKey)) return ModelCache[pathKey];
        return empty;
    }

    static std::atomic<bool> g_CacheThreadRunning = false;

    void HardCacheModelsFromDisk() {
        if (ModelsLoadedToRAM || !UWorldSave) return;
        if (g_CacheThreadRunning) return;

        g_CacheThreadRunning = true;
        ModelsLoadedToRAM = true;

        std::thread([]() {
            std::string baseDir = GetPublicPath();

            const char* weaponBases[] = { "vandal", "phantom" };
            const char* otherWeapons[] = {
            "bulldog", "guardian", "sheriff", "ghost",
            "bucky", "judge", "frenzy", "bicak",
            "spectre", "stinger", "marshal", "operator", "ares",
            "odin", "classic", "shorty"
            };

            for (auto base : weaponBases) {
                ParseOBJFile((baseDir + base + ".obj").c_str());
                for (int i = 1; i <= 6; i++) {
                    std::string skinPath = baseDir + base + "_skin" + std::to_string(i) + ".obj";
                    if (GetFileAttributesA(skinPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                        ParseOBJFile(skinPath.c_str());
                }
            }

            for (auto name : otherWeapons) {
                std::string objPath = baseDir + name + ".obj";
                if (GetFileAttributesA(objPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                    ParseOBJFile(objPath.c_str());
            }

            std::string textPath = baseDir + "text.obj";
            if (GetFileAttributesA(textPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                ParseOBJFile(textPath.c_str());

            std::string auraPath = baseDir + "aura.obj";
            if (GetFileAttributesA(auraPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                ParseOBJFile(auraPath.c_str());

            g_CacheThreadRunning = false;
            }).detach();
    }

    void PreCacheAllVisuals() {
        if (UWorldSave != LastWorldPtr) {
            TextureCache.clear();
            LastWorldPtr = UWorldSave;
            ModelsLoadedToRAM = false;
            LastWeaponProcessed = 0;
            g_CacheThreadRunning = false;
        }
        HardCacheModelsFromDisk();
    }

    void ReplaceTextMeshWith3DModel(currentequippable* Weapon, const char* objFilePath) {
        if (!Weapon || !memory::IsValidPointer((uintptr_t)Weapon)) return;
        if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) return;

        auto* OriginalMesh = Weapon->GetMesh1P();
        if (!OriginalMesh || !memory::IsValidPointer((uintptr_t)OriginalMesh)) return;

        static uobject* ProcMeshClass = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent");
        static uobject* AddComponentFunc = (uobject*)uobject::find_object<uclass*>(L"ShooterGame.ShooterBlueprintLibrary.AddComponentByClass");
        static uobject* CreateMeshFunc = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent.CreateMeshSection");

        if (!ProcMeshClass || !AddComponentFunc || !CreateMeshFunc) return;

        struct { AActor* Actor; UActorComponent* ComponentClass; UActorComponent* ReturnValue; }
        AddParams{ (AActor*)Weapon, (UActorComponent*)ProcMeshClass, nullptr };
        variables.blueprints->process_event(AddComponentFunc, &AddParams);

        auto* ProcMesh = (uskeletalmeshcomponent*)AddParams.ReturnValue;
        if (!ProcMesh || !memory::IsValidPointer((uintptr_t)ProcMesh)) return;

        const MeshData& mesh = ParseOBJFile(objFilePath);
        if (mesh.Vertices.Num() == 0) return;

        struct {
            int32_t SectionIndex; tarray<fvector> Vertices; tarray<int32_t> Triangles;
            tarray<fvector> Normals; tarray<fvector2d> UV0; tarray<FColor> VertexColors;
            tarray<FProcMeshTangent> Tangents; bool bCreateCollision;
        } CreateParams = { 0, mesh.Vertices, mesh.Triangles, mesh.Normals, mesh.UV0, mesh.VertexColors, mesh.Tangents, false };

        ((uobject*)ProcMesh)->process_event(CreateMeshFunc, &CreateParams);

        uobject* GlowMaterial = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI");           
        if (GlowMaterial && memory::IsValidPointer((uintptr_t)GlowMaterial)) {
            uobject* DynMat = seh_create_dynamic_mat((uintptr_t)ProcMesh, 0, GlowMaterial);
            if (DynMat && memory::IsValidPointer((uintptr_t)DynMat)) {
                auto* matInstance = (UMaterialInstanceDynamic*)DynMat;
                flinearcolor white = flinearcolor(1.0f, 1.0f, 1.0f, 1.0f);
                matInstance->set_vector_parameter_value(string::string_to_name(L"Base Color"), white);
                matInstance->set_vector_parameter_value(string::string_to_name(L"Emissive Color"), white);
                matInstance->set_scalar_parameter_value(string::string_to_name(L"EmissiveIntensity"), 5.0f);
            }
        }

        USceneComponentHelpers::AttachTo(ProcMesh, (uskeletalmeshcomponent*)OriginalMesh,
            string::string_to_name(L"R_WeaponPoint"), 1, 1, 1, false);
        SetComponentVisibility((USceneComponent*)ProcMesh, true, true);

        seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(
            globals::misc::text_scale_x,
            globals::misc::text_scale_y,
            globals::misc::text_scale_z
        ));
        USceneComponentHelpers::SetRelativeRotation(ProcMesh,
            FRotator{ globals::misc::text_rot_pitch,
                      globals::misc::text_rot_yaw,
                      globals::misc::text_rot_roll });
        USceneComponentHelpers::SetRelativeLocation(ProcMesh,
            fvector(globals::misc::text_pos_x,
                globals::misc::text_pos_y,
                globals::misc::text_pos_z));

        TextMesh = (UProceduralMeshComponent*)ProcMesh;
        text_meshcreated = true;
        WeaponTextMeshMap[(uintptr_t)Weapon] = TextMesh;
    }

    void ReplaceWeaponMeshWith3DModel(currentequippable* Weapon, const char* objFilePath, const wchar_t* texFilePath) {
        if (!Weapon || !memory::IsValidPointer((uintptr_t)Weapon)) return;
        if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) return;

        auto* OriginalMesh = Weapon->GetMesh1P();
        if (!OriginalMesh || !memory::IsValidPointer((uintptr_t)OriginalMesh)) return;

        const MeshData& mesh = ParseOBJFile(objFilePath);
        if (mesh.Vertices.Num() == 0) return;

        {
            USceneComponent* sceneComp = reinterpret_cast<USceneComponent*>(OriginalMesh);
            tarray<USceneComponent*> children = GetChildrenComponents(sceneComp, true);
            static uobject* DestroyComponentFunc = uobject::find_object<uobject*>(L"Engine.Actor.DestroyComponent");
            for (int i = 0; i < children.size(); i++) {
                if (!children[i] || !memory::IsValidPointer((uintptr_t)children[i])) continue;
                fstring childName = system::get_object_name((uobject*)children[i]);
                if (childName.to_str().find("ProceduralMesh") != std::string::npos) {
                    if (DestroyComponentFunc) {
                        struct { UActorComponent* Component; } Args = { (UActorComponent*)children[i] };
                        ((uobject*)Weapon)->process_event(DestroyComponentFunc, &Args);
                    }
                    break;
                }
            }
        }

        std::wstring wTexPath(texFilePath);
        std::string  texKey(wTexPath.begin(), wTexPath.end());
        uobject* WeaponTexture = nullptr;

        if (TextureCache.size() > 50) TextureCache.clear();

        if (TextureCache.count(texKey) && IsValidUObject(TextureCache[texKey])) {
            WeaponTexture = TextureCache[texKey];
        }
        else {
            WeaponTexture = system::import_file_as_texture2d(UWorldSave, fstring(texFilePath));
            if (WeaponTexture) TextureCache[texKey] = WeaponTexture;
        }

        static uobject* ProcMeshClass = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent");
        static uobject* AddComponentFunc = (uobject*)uobject::find_object<uclass*>(L"ShooterGame.ShooterBlueprintLibrary.AddComponentByClass");
        static uobject* CreateMeshFunc = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent.CreateMeshSection");

        if (!ProcMeshClass || !AddComponentFunc || !CreateMeshFunc) return;

        struct { AActor* Actor; UActorComponent* ComponentClass; UActorComponent* ReturnValue; }
        AddParams{ (AActor*)Weapon, (UActorComponent*)ProcMeshClass, nullptr };
        variables.blueprints->process_event(AddComponentFunc, &AddParams);

        auto* ProcMesh = (uskeletalmeshcomponent*)AddParams.ReturnValue;
        if (!ProcMesh || !memory::IsValidPointer((uintptr_t)ProcMesh)) return;

        struct {
            int32_t SectionIndex; tarray<fvector> Vertices; tarray<int32_t> Triangles;
            tarray<fvector> Normals; tarray<fvector2d> UV0; tarray<FColor> VertexColors;
            tarray<FProcMeshTangent> Tangents; bool bCreateCollision;
        } CreateParams = { 0, mesh.Vertices, mesh.Triangles, mesh.Normals, mesh.UV0, mesh.VertexColors, mesh.Tangents, false };

        ((uobject*)ProcMesh)->process_event(CreateMeshFunc, &CreateParams);

        if (WeaponTexture) {
            uobject* MasterMat = OriginalMesh->GetMaterial(0);
            if (MasterMat) {
                uobject* DynMat = seh_create_dynamic_mat((uintptr_t)ProcMesh, 0, MasterMat);
                if (DynMat) {
                    auto* matInstance = (UMaterialInstanceDynamic*)DynMat;
                    const wchar_t* pNames[] = { L"BaseColor", L"Diffuse", L"Albedo", L"Texture" };
                    for (auto p : pNames)
                        matInstance->set_texture_parameter_value(string::string_to_name(p), WeaponTexture);
                    matInstance->set_scalar_parameter_value2(string::string_to_name(L"TwoSided"), 1.0f);
                }
            }
        }

        USceneComponentHelpers::AttachTo(ProcMesh, (uskeletalmeshcomponent*)OriginalMesh,
            string::string_to_name(L"R_WeaponPoint"), 1, 1, 1, false);

        uintptr_t cosmetic_ptr = (uintptr_t)Weapon + 0x1188;
        uskeletalmeshcomponent* cosmeticMesh1P = nullptr;
        if (SafeRead(cosmetic_ptr, cosmeticMesh1P)) {
            if (cosmeticMesh1P && IsValidViewModelObject(cosmeticMesh1P)) {
                SetComponentVisibility((USceneComponent*)cosmeticMesh1P, false, true);
            }
        }
        SetComponentVisibility((USceneComponent*)OriginalMesh, false, true);
        SetComponentVisibility((USceneComponent*)ProcMesh, true, true);

        fstring converted_name = helper::convert_weapon_name(system::get_object_name((uobject*)Weapon));
        std::wstring wNameMesh = converted_name.wide();

        if (wNameMesh == L"Melee") {
            seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(1.35f, 1.35f, 1.35f));
            USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{ -46.6f, -103.2f, 93.1f });
            USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(-19.93f, -1.05f, -0.70f));
        }
        else if (wNameMesh == L"Spectre") {
            seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(
                globals::misc::spectre_scale_x,
                globals::misc::spectre_scale_y,
                globals::misc::spectre_scale_z
            ));
            USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{
                globals::misc::spectre_rot_pitch,
                globals::misc::spectre_rot_yaw,
                globals::misc::spectre_rot_roll
                });
            USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(
                globals::misc::spectre_pos_x,
                globals::misc::spectre_pos_y,
                globals::misc::spectre_pos_z
            ));
        }
        else {
            seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(1.5f, 1.5f, 1.5f));
            USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{ 0.f, 90.f, -90.f });
            USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(-0.9434f, 0.943392f, -2.83019f));
        }

        LastWeaponProcessed = (uintptr_t)Weapon;

        // ── 3P mesh: also apply custom skin to third person view ──
        auto* Mesh3P = Weapon->GetMesh3P();
        if (Mesh3P && memory::IsValidPointer((uintptr_t)Mesh3P)) {
            // Remove old 3P procedural mesh if exists
            {
                USceneComponent* sc3p = reinterpret_cast<USceneComponent*>(Mesh3P);
                tarray<USceneComponent*> ch3p = GetChildrenComponents(sc3p, true);
                static uobject* DestroyFunc3P = uobject::find_object<uobject*>(L"Engine.Actor.DestroyComponent");
                for (int i = 0; i < ch3p.size(); i++) {
                    if (!ch3p[i] || !memory::IsValidPointer((uintptr_t)ch3p[i])) continue;
                    fstring cn = system::get_object_name((uobject*)ch3p[i]);
                    if (cn.to_str().find("ProceduralMesh") != std::string::npos) {
                        if (DestroyFunc3P) {
                            struct { UActorComponent* Component; } Args = { (UActorComponent*)ch3p[i] };
                            ((uobject*)Weapon)->process_event(DestroyFunc3P, &Args);
                        }
                        break;
                    }
                }
            }

            struct { AActor* Actor; UActorComponent* ComponentClass; UActorComponent* ReturnValue; }
            Add3P{ (AActor*)Weapon, (UActorComponent*)ProcMeshClass, nullptr };
            variables.blueprints->process_event(AddComponentFunc, &Add3P);

            auto* ProcMesh3P = (uskeletalmeshcomponent*)Add3P.ReturnValue;
            if (ProcMesh3P && memory::IsValidPointer((uintptr_t)ProcMesh3P)) {
                ((uobject*)ProcMesh3P)->process_event(CreateMeshFunc, &CreateParams);

                if (WeaponTexture) {
                    uobject* MasterMat3P = Mesh3P->GetMaterial(0);
                    if (MasterMat3P) {
                        uobject* DynMat3P = seh_create_dynamic_mat((uintptr_t)ProcMesh3P, 0, MasterMat3P);
                        if (DynMat3P) {
                            auto* matInst3P = (UMaterialInstanceDynamic*)DynMat3P;
                            const wchar_t* pNames3P[] = { L"BaseColor", L"Diffuse", L"Albedo", L"Texture" };
                            for (auto p : pNames3P)
                                matInst3P->set_texture_parameter_value(string::string_to_name(p), WeaponTexture);
                            matInst3P->set_scalar_parameter_value2(string::string_to_name(L"TwoSided"), 1.0f);
                        }
                    }
                }

                USceneComponentHelpers::AttachTo(ProcMesh3P, (uskeletalmeshcomponent*)Mesh3P,
                    string::string_to_name(L"R_WeaponPoint"), 1, 1, 1, false);
                SetComponentVisibility((USceneComponent*)Mesh3P, false, true);
                SetComponentVisibility((USceneComponent*)ProcMesh3P, true, true);

                if (wNameMesh == L"Melee") {
                    seh_set_relative_scale_3d((uintptr_t)ProcMesh3P, fvector(1.35f, 1.35f, 1.35f));
                    USceneComponentHelpers::SetRelativeRotation(ProcMesh3P, FRotator{ -46.6f, -103.2f, 93.1f });
                    USceneComponentHelpers::SetRelativeLocation(ProcMesh3P, fvector(-19.93f, -1.05f, -0.70f));
                }
                else if (wNameMesh == L"Spectre") {
                    seh_set_relative_scale_3d((uintptr_t)ProcMesh3P, fvector(
                        globals::misc::spectre_scale_x,
                        globals::misc::spectre_scale_y,
                        globals::misc::spectre_scale_z
                    ));
                    USceneComponentHelpers::SetRelativeRotation(ProcMesh3P, FRotator{
                        globals::misc::spectre_rot_pitch,
                        globals::misc::spectre_rot_yaw,
                        globals::misc::spectre_rot_roll
                        });
                    USceneComponentHelpers::SetRelativeLocation(ProcMesh3P, fvector(
                        globals::misc::spectre_pos_x,
                        globals::misc::spectre_pos_y,
                        globals::misc::spectre_pos_z
                    ));
                }
                else {
                    seh_set_relative_scale_3d((uintptr_t)ProcMesh3P, fvector(1.5f, 1.5f, 1.5f));
                    USceneComponentHelpers::SetRelativeRotation(ProcMesh3P, FRotator{ 0.f, 90.f, -90.f });
                    USceneComponentHelpers::SetRelativeLocation(ProcMesh3P, fvector(-0.9434f, 0.943392f, -2.83019f));
                }
            }
        }
    }











    static float normalize_angle(float angle) {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }





    static void SetCameraCachePOVHook_Impl(uintptr_t PlayerCameraManager, FMinimalViewInfo* ViewInfo)
    {
        if (!SetCameraCachePOVOriginal) return;
        if (!ViewInfo || !memory::IsValidPointer((uintptr_t)ViewInfo)) {
            SetCameraCachePOVOriginal(PlayerCameraManager, ViewInfo);
            return;
        }

        LocalCameraLocation = ViewInfo->Location;
        LocalCameraRotation = ViewInfo->Rotation;

        static bool cam_logged = false;
        if (!cam_logged) {
            LOG_INFO("SetCameraCachePOVHook first call - PCM=0x%llX ViewInfo=0x%llX", (unsigned long long)PlayerCameraManager, (unsigned long long)ViewInfo);
            cam_logged = true;
        }

        const bool ctrl_ok = controllers && memory::IsValidPointer((uintptr_t)controllers);
        bool aimbot_key_current = burat::menu_open ? false : (GetAsyncKeyState(globals::aimbot::a1m_k3y) != 0);

        // === FREE CAM (toggle, smooth, proper) ===
        static bool s_freecam_active = false;
        static bool s_freecam_key_prev = false;
        static fvector s_freecam_loc{};
        static fvector s_freecam_rot{};

        const bool freecam_key_down = (globals::misc::freecam_key != 0) && ((GetAsyncKeyState(globals::misc::freecam_key) & 0x8000) != 0);
        if (globals::misc::freecam_key != 0 && freecam_key_down && !s_freecam_key_prev) {
            globals::misc::freecam = !globals::misc::freecam;
            if (globals::misc::freecam && ViewInfo) {
                s_freecam_active = true;
                s_freecam_loc = ViewInfo->Location;
                s_freecam_rot = ViewInfo->Rotation;
            }
            else {
                s_freecam_active = false;
            }
        }
        s_freecam_key_prev = freecam_key_down;

        if (globals::misc::freecam && ViewInfo && !burat::menu_open && ctrl_ok) {
            if (!s_freecam_active) {
                s_freecam_active = true;
                s_freecam_loc = ViewInfo->Location;
                s_freecam_rot = ViewInfo->Rotation;
            }

            float dx = 0.f, dy = 0.f;
            controllers->GetInputMouseDelta(dx, dy);
            const float sens = controllers->GetMouseSensitivity();
            s_freecam_rot.x = normalize_angle(s_freecam_rot.x + dy * sens);
            s_freecam_rot.y = normalize_angle(s_freecam_rot.y + dx * sens);
            if (s_freecam_rot.x > 89.0f) s_freecam_rot.x = 89.0f;
            if (s_freecam_rot.x < -89.0f) s_freecam_rot.x = -89.0f;

            const float radPitch = s_freecam_rot.x * (M_PI / 180.0f);
            const float radYaw = s_freecam_rot.y * (M_PI / 180.0f);
            fvector forward{ cosf(radPitch) * cosf(radYaw), cosf(radPitch) * sinf(radYaw), sinf(radPitch) };
            fvector right{ -sinf(radYaw), cosf(radYaw), 0.f };
            fvector up{ 0.f, 0.f, 1.f };

            float spd = globals::misc::freecam_speed;
            if (spd < 50.0f) spd = 50.0f;
            if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) spd *= 2.5f;
            if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) spd *= 0.35f; // Alt = slow

            // fixed tick since hook is per-frame; keep stable by clamping large frame spikes
            static LARGE_INTEGER s_freq{ 0 }, s_last{ 0 };
            if (s_freq.QuadPart == 0) QueryPerformanceFrequency(&s_freq);
            LARGE_INTEGER now; QueryPerformanceCounter(&now);
            float dt = 0.016f;
            if (s_last.QuadPart != 0) {
                dt = (float)((double)(now.QuadPart - s_last.QuadPart) / (double)s_freq.QuadPart);
                if (dt < 0.001f) dt = 0.001f;
                if (dt > 0.050f) dt = 0.050f;
            }
            s_last = now;

            fvector move{};
            if ((GetAsyncKeyState('W') & 0x8000) != 0) move = move + forward;
            if ((GetAsyncKeyState('S') & 0x8000) != 0) move = move - forward;
            if ((GetAsyncKeyState('D') & 0x8000) != 0) move = move + right;
            if ((GetAsyncKeyState('A') & 0x8000) != 0) move = move - right;
            if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) move = move + up;
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) move = move - up;

            const float mlen = sqrtf(move.x * move.x + move.y * move.y + move.z * move.z);
            if (mlen > 0.001f) {
                const float inv = 1.0f / mlen;
                move.x *= inv; move.y *= inv; move.z *= inv;
                s_freecam_loc.x += move.x * spd * dt;
                s_freecam_loc.y += move.y * spd * dt;
                s_freecam_loc.z += move.z * spd * dt;
            }

            ViewInfo->Location = s_freecam_loc;
            ViewInfo->Rotation = s_freecam_rot;

            // keep FOV/aspect/thirdperson logic consistent
            if (globals::misc::fovchanger) ViewInfo->FOV = globals::misc::fovchangur;
            if (globals::misc::aspectratio) { ViewInfo->bConstrainAspectRatio = true; ViewInfo->AspectRatio = globals::misc::aspectfloat; }

            SetCameraCachePOVOriginal(PlayerCameraManager, ViewInfo);
            LocalCameraRotation = ViewInfo->Rotation;
            return;
        }

        // === SILENT AIM ===
        if (globals::aimbot::silent && ctrl_ok && !globals::misc::tperson && !aim_check && !burat::menu_open) {
            if (GetAsyncKeyState(globals::aimbot::a1m_k3y)) {
                if (first_location) {
                    aim_check = true;
                    first_camera_location = controllers->get_control_rotation();
                    first_camera_rotation = ViewInfo->Rotation;
                    aim_check = false;
                    first_location = false;
                }
                second_locked_camera = true;
                finished_hook = false;
            }
            else if (!first_location) {
                aim_check = true;
                controllers->set_control_rotation(first_camera_location);
                if (controllers->get_control_rotation() == first_camera_location &&
                    ViewInfo->Rotation == first_camera_rotation) {
                    finished_hook = true;
                    second_locked_camera = false;
                }
                first_location = true;
            }
        }
        else {
            second_locked_camera = false;
            first_camera_location = fvector();
            aim_check = false;
            finished_hook = false;
            first_location = true;
        }

        if (globals::aimbot::silent && second_locked_camera && ViewInfo != nullptr && first_camera_rotation != fvector()) {
            ViewInfo->Rotation = first_camera_rotation;
        }

        if (globals::misc::fovchanger && ViewInfo) {
            ViewInfo->FOV = globals::misc::fovchangur;
        }

        if (globals::misc::aspectratio && ViewInfo) {
            ViewInfo->bConstrainAspectRatio = true;
            ViewInfo->AspectRatio = globals::misc::aspectfloat;
        }

        if (globals::misc::tperson && ViewInfo) {
            float radPitch = ViewInfo->Rotation.x * (M_PI / 180.0f);
            float radYaw = ViewInfo->Rotation.y * (M_PI / 180.0f);

            fvector forward;
            forward.x = cosf(radPitch) * cosf(radYaw);
            forward.y = cosf(radPitch) * sinf(radYaw);
            forward.z = sinf(radPitch);

            ViewInfo->Location.x -= forward.x * globals::misc::PlayerDistance;
            ViewInfo->Location.y -= forward.y * globals::misc::PlayerDistance;
            ViewInfo->Location.z -= forward.z * globals::misc::PlayerDistance;
        }

        if (globals::misc::nightmode && globals::misc::nightmode_postprocess && ViewInfo && memory::IsValidPointer((uintptr_t)ViewInfo)) {
            ViewInfo->PostProcessBlendWeight = 1.0f;
            FPostProcessSettings& PPS = ViewInfo->PostProcessSettings;
            PPS.bOverride_SceneColorTint = true;
            PPS.SceneColorTint = flinearcolor(
                globals::misc::nightmode_pp_tint_r,
                globals::misc::nightmode_pp_tint_g,
                globals::misc::nightmode_pp_tint_b, 1.0f);
            PPS.bOverride_AutoExposureMinBrightness = true;
            PPS.AutoExposureMinBrightness = globals::misc::nightmode_pp_exposure_min;
            PPS.bOverride_AutoExposureMaxBrightness = true;
            PPS.AutoExposureMaxBrightness = globals::misc::nightmode_pp_exposure_max;
            PPS.bOverride_BloomIntensity = true;
            PPS.BloomIntensity = 0.0f;
            PPS.bOverride_VignetteIntensity = true;
            PPS.VignetteIntensity = globals::misc::nightmode_pp_vignette;
            PPS.bOverride_FilmSaturation = true;
            PPS.FilmSaturation = globals::misc::nightmode_pp_saturation;
        }

        SetCameraCachePOVOriginal(PlayerCameraManager, ViewInfo);

        if (ViewInfo) {
            LocalCameraRotation = ViewInfo->Rotation;
        }

        aimbot_key_pressed_last_frame = aimbot_key_current;
    }

    void SetCameraCachePOVHook(uintptr_t PlayerCameraManager, FMinimalViewInfo* ViewInfo)
    {
        __try {
            SetCameraCachePOVHook_Impl(PlayerCameraManager, ViewInfo);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("SetCameraCachePOVHook SEH code=0x%X PCM=0x%llX ViewInfo=0x%llX",
                GetExceptionCode(),
                (unsigned long long)PlayerCameraManager,
                (unsigned long long)ViewInfo);
            if (SetCameraCachePOVOriginal)
                SetCameraCachePOVOriginal(PlayerCameraManager, ViewInfo);
        }
    }





    static flinearcolor accent_color = { 0.6f, 0.6f, 0.6f, 1.0f }; // OUR COLOR (VIBRANT GREY)
    const flinearcolor background_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    const flinearcolor border_color = { 0.08f, 0.08f, 0.08f, 1.0f };
    const flinearcolor panel_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    const flinearcolor text_color = { 0.9f, 0.9f, 0.9f, 1.0f };
    const flinearcolor hover_color = { 0.2f, 0.2f, 0.2f, 1.0f };
    const flinearcolor disabled_color = { 0.5f, 0.5f, 0.5f, 1.0f };
    const flinearcolor dark_grey_color = { 0.15f, 0.15f, 0.15f, 1.0f };  // Dark grey for unfilled part  
    const flinearcolor shadow_color = { 0.1f, 0.1f, 0.1f, 0.6f };        // Subtle shadow effect  
    const flinearcolor dark_border_color = { 0.15f, 0.15f, 0.15f, 1.0f };

    void DrawSmoothFilledCircle(fvector2d center, float radius, flinearcolor color, ucanvas* canvas)
    {
        constexpr float PI = 3.14159265359f;

        fvector2d p0 = { center.x + radius * cosf(0.0f), center.y + radius * sinf(0.0f) };
        fvector2d p1 = { center.x + radius * cosf(PI / 3.0f), center.y + radius * sinf(PI / 3.0f) };
        fvector2d p2 = { center.x + radius * cosf(2.0f * PI / 3.0f), center.y + radius * sinf(2.0f * PI / 3.0f) };
        fvector2d p3 = { center.x + radius * cosf(PI), center.y + radius * sinf(PI) };
        fvector2d p4 = { center.x + radius * cosf(4.0f * PI / 3.0f), center.y + radius * sinf(4.0f * PI / 3.0f) };
        fvector2d p5 = { center.x + radius * cosf(5.0f * PI / 3.0f), center.y + radius * sinf(5.0f * PI / 3.0f) };

        canvas->k2_drawline(center, p0, 1.0f, color);
        canvas->k2_drawline(center, p1, 1.0f, color);
        canvas->k2_drawline(p0, p1, 1.0f, color);

        canvas->k2_drawline(center, p1, 1.0f, color);
        canvas->k2_drawline(center, p2, 1.0f, color);
        canvas->k2_drawline(p1, p2, 1.0f, color);

        canvas->k2_drawline(center, p2, 1.0f, color);
        canvas->k2_drawline(center, p3, 1.0f, color);
        canvas->k2_drawline(p2, p3, 1.0f, color);

        canvas->k2_drawline(center, p3, 1.0f, color);
        canvas->k2_drawline(center, p4, 1.0f, color);
        canvas->k2_drawline(p3, p4, 1.0f, color);

        canvas->k2_drawline(center, p4, 1.0f, color);
        canvas->k2_drawline(center, p5, 1.0f, color);
        canvas->k2_drawline(p4, p5, 1.0f, color);

        canvas->k2_drawline(center, p5, 1.0f, color);
        canvas->k2_drawline(center, p0, 1.0f, color);
        canvas->k2_drawline(p5, p0, 1.0f, color);
    }


    void DrawSimpleCircle(fvector2d center, float radius, flinearcolor color, ucanvas* canvas)
    {
        constexpr float PI = 3.14159265359f;

        float angle0 = 0.0f;
        float angle1 = PI / 3.0f;
        float angle2 = 2.0f * PI / 3.0f;
        float angle3 = PI;
        float angle4 = 4.0f * PI / 3.0f;
        float angle5 = 5.0f * PI / 3.0f;

        fvector2d p0 = { center.x + cosf(angle0) * radius, center.y + sinf(angle0) * radius };
        fvector2d p1 = { center.x + cosf(angle1) * radius, center.y + sinf(angle1) * radius };
        fvector2d p2 = { center.x + cosf(angle2) * radius, center.y + sinf(angle2) * radius };
        fvector2d p3 = { center.x + cosf(angle3) * radius, center.y + sinf(angle3) * radius };
        fvector2d p4 = { center.x + cosf(angle4) * radius, center.y + sinf(angle4) * radius };
        fvector2d p5 = { center.x + cosf(angle5) * radius, center.y + sinf(angle5) * radius };

        canvas->k2_drawline(p0, p1, 1.0f, color);
        canvas->k2_drawline(p1, p2, 1.0f, color);
        canvas->k2_drawline(p2, p3, 1.0f, color);
        canvas->k2_drawline(p3, p4, 1.0f, color);
        canvas->k2_drawline(p4, p5, 1.0f, color);
        canvas->k2_drawline(p5, p0, 1.0f, color);
    }

    namespace radar
    {
        static fvector pRadar;
        /*void DrawCircleRadar(int x, int y, int radius, flinearcolor color, ucanvas* cvs)
        {
            DrawFilledCircle(fvector2d(x, y), radius, color, cvs);
        }*/

        void DrawCircleRadar(int x, int y, int radius, flinearcolor color, ucanvas* canvas)
        {
            // Outer glow
            flinearcolor glowColor = color;
            glowColor.a *= 0.25f;
            DrawSmoothFilledCircle(fvector2d(x, y), radius + 2, glowColor, canvas);

            // Solid core
            DrawSmoothFilledCircle(fvector2d(x, y), radius, color, canvas);
        }


        fvector WorldRadar(fvector srcPos, fvector distPos, float yaw, float radarX, float radarY, float size)
        {
            auto cosYaw = cos(DegreeToRadian(yaw));
            auto sinYaw = sin(DegreeToRadian(yaw));

            auto deltaX = srcPos.x - distPos.x;
            auto deltaY = srcPos.y - distPos.y;

            auto locationX = (float)(deltaY * cosYaw - deltaX * sinYaw) / 45.f;
            auto locationY = (float)(deltaX * cosYaw + deltaY * sinYaw) / 45.f;

            if (locationX > (size - 2.f))
                locationX = (size - 2.f);
            else if (locationX < -(size - 2.f))
                locationX = -(size - 2.f);

            if (locationY > (size - 6.f))
                locationY = (size - 6.f);
            else if (locationY < -(size - 6.f))
                locationY = -(size - 6.f);

            return fvector((int)(-locationX + radarX), (int)(locationY + radarY), 0);
        }

        static float closestDistance = FLT_MAX;
        static ashootercharacter* pulsingActor = nullptr;

        inline float GetPulseScale()
        {
            float time = GetTickCount64() / 1000.f; // system time in seconds
            return 1.0f + 0.25f * sinf(time * 5.0f); // pulsing between 1.0x - 1.25x
        }


        void DrawRadar(fvector EntityPos, acknowledgedpawn* MyPawns, ucanvas* cvs, ashootercharacter* actor)
        {
            if (!actor || !actor->is_alive())
                return; // skip dead enemies

            aplayercameramanager* camerap = controllers->get_camera_manager();
            int radar_posX = pRadar.x + 135;
            int radar_posY = pRadar.y + 135;

            uint64_t LocalRootComp = memory::read<uint64_t>((uint64_t)MyPawns + offsets::Rootcomponent);
            fvector LocalPos = memory::read<fvector>(LocalRootComp + offsets::root_position);
            FMinimalViewInfo camerae = memory::read<FMinimalViewInfo>((uint64_t)camerap + offsets::CameraRadar);

            fvector Radar2D = WorldRadar(LocalPos, EntityPos, camerae.Rotation.y, radar_posX, radar_posY, 135.f);

            float distance = (EntityPos - LocalPos).length();
            bool Visible = controllers->line_of_sight(actor);

            // Determine if this is the closest actor this frame
            if (distance < closestDistance)
            {
                closestDistance = distance;
                pulsingActor = actor;
            }

            float baseSize = 5.0f;
            float pulseSize = (actor == pulsingActor && distance < 1000.f) ? baseSize * GetPulseScale() : baseSize;

            // Shadow effect
            flinearcolor shadowColor = { 0.f, 0.f, 0.f, 0.3f };
            DrawCircleRadar(Radar2D.x + 1, Radar2D.y + 1, pulseSize, shadowColor, cvs);

            // Main radar blip
            if (Visible) {
                static flinearcolor greenColor{ 0.0f, 1.0f, 0.0f, 1.0f };
                DrawCircleRadar(Radar2D.x, Radar2D.y, pulseSize, greenColor, cvs);
            }
            else {
                static flinearcolor redColor{ 1.0f, 0.0f, 0.0f, 1.0f };
                DrawCircleRadar(Radar2D.x, Radar2D.y, pulseSize, redColor, cvs);
            }
        }

        // Call this at the beginning of each frame, before drawing enemies
        void ResetRadarPulse()
        {
            closestDistance = DBL_MAX;
            pulsingActor = nullptr;
        }
    }













    void ReplaceCharacterMeshWith3DModel(ashootercharacter* shooter, const char* objFilePath, const wchar_t* texFilePath) {
        if (!shooter || !memory::IsValidPointer((uintptr_t)shooter)) return;
        if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) return;

        auto* OriginalMesh = shooter->get_mesh();
        if (!OriginalMesh || !memory::IsValidPointer((uintptr_t)OriginalMesh)) return;

        // Destroy existing procedural mesh if any
        {
            USceneComponent* sceneComp = reinterpret_cast<USceneComponent*>(OriginalMesh);
            tarray<USceneComponent*> children = GetChildrenComponents(sceneComp, true);
            static uobject* DestroyComponentFunc = uobject::find_object<uobject*>(L"Engine.Actor.DestroyComponent");
            for (int i = 0; i < children.size(); i++) {
                if (!children[i] || !memory::IsValidPointer((uintptr_t)children[i])) continue;
                fstring childName = system::get_object_name((uobject*)children[i]);
                if (childName.to_str().find("ProceduralMesh") != std::string::npos) {
                    if (DestroyComponentFunc) {
                        struct { UActorComponent* Component; } Args = { (UActorComponent*)children[i] };
                        ((uobject*)shooter)->process_event(DestroyComponentFunc, &Args);
                    }
                    break;
                }
            }
        }

        uobject* CharacterTexture = nullptr;
        std::wstring wTexPath(texFilePath);
        std::string  texKey(wTexPath.begin(), wTexPath.end());

        if (TextureCache.count(texKey) && IsValidUObject(TextureCache[texKey])) {
            CharacterTexture = TextureCache[texKey];
        }
        else {
            CharacterTexture = system::import_file_as_texture2d(UWorldSave, fstring(texFilePath));
            if (CharacterTexture) TextureCache[texKey] = CharacterTexture;
        }

        static uobject* ProcMeshClass = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent");
        static uobject* AddComponentFunc = (uobject*)uobject::find_object<uclass*>(L"ShooterGame.ShooterBlueprintLibrary.AddComponentByClass");
        static uobject* CreateMeshFunc = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent.CreateMeshSection");

        if (!ProcMeshClass || !AddComponentFunc || !CreateMeshFunc) return;

        struct { AActor* Actor; UActorComponent* ComponentClass; UActorComponent* ReturnValue; }
        AddParams{ (AActor*)shooter, (UActorComponent*)ProcMeshClass, nullptr };
        variables.blueprints->process_event(AddComponentFunc, &AddParams);

        auto* ProcMesh = (uskeletalmeshcomponent*)AddParams.ReturnValue;
        if (!ProcMesh || !memory::IsValidPointer((uintptr_t)ProcMesh)) return;

        const MeshData& mesh = ParseOBJFile(objFilePath);
        if (mesh.Vertices.Num() == 0) return;

        struct {
            int32_t SectionIndex; tarray<fvector> Vertices; tarray<int32_t> Triangles;
            tarray<fvector> Normals; tarray<fvector2d> UV0; tarray<FColor> VertexColors;
            tarray<FProcMeshTangent> Tangents; bool bCreateCollision;
        } CreateParams = { 0, mesh.Vertices, mesh.Triangles, mesh.Normals, mesh.UV0, mesh.VertexColors, mesh.Tangents, false };

        ((uobject*)ProcMesh)->process_event(CreateMeshFunc, &CreateParams);

        if (CharacterTexture) {
            uobject* MasterMat = OriginalMesh->GetMaterial(0);
            if (MasterMat) {
                uobject* DynMat = seh_create_dynamic_mat((uintptr_t)ProcMesh, 0, MasterMat);
                if (DynMat) {
                    auto* matInstance = (UMaterialInstanceDynamic*)DynMat;
                    matInstance->set_texture_parameter_value(string::string_to_name(L"BaseColor"), CharacterTexture);
                    matInstance->set_scalar_parameter_value2(string::string_to_name(L"TwoSided"), 1.0f);
                }
            }
        }

        // Attach to character mesh
        USceneComponentHelpers::AttachTo(ProcMesh, (uskeletalmeshcomponent*)OriginalMesh,
            string::string_to_name(L"Root"), 1, 1, 1, false);
        
        seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(1.0f, 1.0f, 1.0f));
        USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{ 0.f, 0.f, 0.f });
        USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(0, 0, 0));

        SetComponentVisibility((USceneComponent*)OriginalMesh, false, true);
        SetComponentVisibility((USceneComponent*)ProcMesh, true, true);
    }

    void ResetCharacterMesh(ashootercharacter* shooter) {
        if (!shooter || !memory::IsValidPointer((uintptr_t)shooter)) return;

        auto* OriginalMesh = shooter->get_mesh();
        if (!OriginalMesh || !memory::IsValidPointer((uintptr_t)OriginalMesh)) return;

        // Destroy existing procedural mesh
        {
            USceneComponent* sceneComp = reinterpret_cast<USceneComponent*>(OriginalMesh);
            tarray<USceneComponent*> children = GetChildrenComponents(sceneComp, true);
            static uobject* DestroyComponentFunc = uobject::find_object<uobject*>(L"Engine.Actor.DestroyComponent");
            for (int i = 0; i < children.size(); i++) {
                if (!children[i] || !memory::IsValidPointer((uintptr_t)children[i])) continue;
                fstring childName = system::get_object_name((uobject*)children[i]);
                if (childName.to_str().find("ProceduralMesh") != std::string::npos) {
                    if (DestroyComponentFunc) {
                        struct { UActorComponent* Component; } Args = { (UActorComponent*)children[i] };
                        ((uobject*)shooter)->process_event(DestroyComponentFunc, &Args);
                    }
                    break;
                }
            }
        }

        SetComponentVisibility((USceneComponent*)OriginalMesh, true, true);
    }

    void meshp1_material1(acknowledgedpawn* pawn, ashootercharacter* shooter_character)
    {

        uobject* material = uobject::static_load_object(
            nullptr,
            nullptr,
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI"
        );

        fname first_name = string::string_to_name(L"Base Color");
        fname second_name = string::string_to_name(L"Emissive Color");

        UPrimitiveComponent* mesh1p = memory::read<UPrimitiveComponent*>(uintptr_t(pawn) + offsets::mesh1p);
        UPrimitiveComponent* meshOverlay = memory::read<UPrimitiveComponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay);

        /*   uskeletalmeshcomponent* mesh1p = pawn->GetMesh11P();
           uskeletalmeshcomponent* meshOverlay = pawn->GetOverlayMesh11P();*/



        if (mesh1p) {
            auto num_materials = mesh1p->get_num_materials();
            for (int i = 0; i < num_materials; i++) {
                uobject* material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh1p, i, material);
                if (material_instance_dynamic) {

                    material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(first_name, { globals::misc::handcolor.r * globals::misc::handbright, globals::misc::handcolor.g * globals::misc::handbright, globals::misc::handcolor.b * globals::misc::handbright });
                    material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(second_name, { globals::misc::handcolor.r * globals::misc::handbright, globals::misc::handcolor.g * globals::misc::handbright, globals::misc::handcolor.b * globals::misc::handbright });
                }
            }
        }

        if (meshOverlay) {
            auto num_materials = meshOverlay->get_num_materials();
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)meshOverlay, i, material);
                if (material_instance_dynamic) {

                    material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(first_name, { globals::misc::handcolor.r * globals::misc::handbright, globals::misc::handcolor.g * globals::misc::handbright, globals::misc::handcolor.b * globals::misc::handbright });
                    material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(second_name, { globals::misc::handcolor.r * globals::misc::handbright, globals::misc::handcolor.g * globals::misc::handbright,globals::misc::handcolor.b * globals::misc::handbright });
                }
            }
        }


        if (globals::misc::HandChamsRbg) {

            rainbowTime += 0.005f;
            flinearcolor rainbow = GetRainbowColor(rainbowTime);

            if (mesh1p) {
                auto num_materials = mesh1p->get_num_materials();
                for (int i = 0; i < num_materials; i++) {
                    uobject* material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh1p, i, material);
                    if (material_instance_dynamic) {

                        material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(first_name, { rainbow.r * globals::misc::handbright, rainbow.g * globals::misc::handbright, rainbow.b * globals::misc::handbright });
                        material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(second_name, { rainbow.r * globals::misc::handbright, rainbow.g * globals::misc::handbright, rainbow.b * globals::misc::handbright });
                    }
                }
            }

            if (meshOverlay) {
                auto num_materials = meshOverlay->get_num_materials();
                for (int i = 0; i < num_materials; i++) {
                    auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)meshOverlay, i, material);
                    if (material_instance_dynamic) {

                        material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(first_name, { rainbow.r * globals::misc::handbright, rainbow.g * globals::misc::handbright, rainbow.b * globals::misc::handbright });
                        material_instance_dynamic->cast<UMaterialInstanceDynamic>()->set_vector_parameter_value1(second_name, { rainbow.r * globals::misc::handbright, rainbow.g * globals::misc::handbright, rainbow.b * globals::misc::handbright });
                    }
                }
            }
        }
    }



    void meshp1_material(acknowledgedpawn* MyPawn, ashootercharacter* characterz)
    {
        UPrimitiveComponent* mesh = memory::read<UPrimitiveComponent*>(uintptr_t(MyPawn) + offsets::mesh1p);
        UPrimitiveComponent* meshOverlay = memory::read<UPrimitiveComponent*>(uintptr_t(MyPawn) + offsets::mesh1p_overlay);

        uobject* material = nullptr;

        switch (globals::misc::materials) {
        case 0:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/3P_Weapon_Translucent_Mat"
            );
            break;
        case 1:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Characters/Vampire/S0/VFX/Ability_X/1P_Vampire_Tattoo_X_S0_MI_VFX.1P_Vampire_Tattoo_X_S0_MI_VFX"
            );
            break;
        case 2:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Gems/1P_Gem_MAT"
            );
            break;
        case 3:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat"
            );
            break;
        case 4:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Sakura/Tritium_Sakura_3P_MI"
            );
            break;
        case 5:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Yellow_MI.Arcade_Emissive_Yellow_MI"
            );
            break;
        case 6:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Red_MI.Arcade_Emissive_Red_MI"
            );
            break;
        case 7:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI"
            );
            break;
        case 8:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Afterglow3/BakedLight/Afterglow3_BakedLight_MI.Afterglow3_BakedLight_MI"
            );
            break;
        case 9:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
            );
            break;
        case 10:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/VFX/Materials/HunterReveal_MI.HunterReveal_MI"
            );
            break;
        case 11:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Warring/1P_Melee_Warring_Emissive_Gradient_LUT_MI.1P_Melee_Warring_Emissive_Gradient_LUT_MI"
            );
            break;
        }

        if (material && memory::IsValidPointer((uintptr_t)material))
        {
            if (mesh && memory::IsValidPointer((uintptr_t)mesh))
                seh_set_material((uintptr_t)mesh, 0, material);

            if (meshOverlay && memory::IsValidPointer((uintptr_t)meshOverlay))
                seh_set_material((uintptr_t)meshOverlay, 0, material);
        }
    }   
    void Gunp1_material(acknowledgedpawn* MyPawn, ashootercharacter* characterz)
    {
        UPrimitiveComponent* mesh = memory::read<UPrimitiveComponent*>(uintptr_t(MyPawn) + offsets::mesh1p);
        UPrimitiveComponent* meshOverlay = memory::read<UPrimitiveComponent*>(uintptr_t(MyPawn) + offsets::mesh1p_overlay);
        UPrimitiveComponent* gun = memory::read<UPrimitiveComponent*>(uintptr_t(MyPawn) + offsets::mesh1pgun);

        uobject* material = nullptr;

        switch (globals::misc::materials) {
        case 0:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/3P_Weapon_Translucent_Mat"
            );
            break;
        case 1:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Characters/Vampire/S0/VFX/Ability_X/1P_Vampire_Tattoo_X_S0_MI_VFX.1P_Vampire_Tattoo_X_S0_MI_VFX"
            );
            break;
        case 2:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Gems/1P_Gem_MAT"
            );
            break;
        case 3:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat"
            );
            break;
        case 4:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Sakura/Tritium_Sakura_3P_MI"
            );
            break;
        case 5:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Yellow_MI.Arcade_Emissive_Yellow_MI"
            );
            break;
        case 6:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Red_MI.Arcade_Emissive_Red_MI"
            );
            break;
        case 7:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI"
            );
            break;
        case 8:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Afterglow3/BakedLight/Afterglow3_BakedLight_MI.Afterglow3_BakedLight_MI"
            );
            break;
        case 9:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI"
            );
            break;
        case 10:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/VFX/Materials/HunterReveal_MI.HunterReveal_MI"
            );
            break;
        case 11:
            material = uobject::static_load_object(
                nullptr,
                nullptr,
                L"/Game/Equippables/_Core/Materials/SpecialMaterials/Warring/1P_Melee_Warring_Emissive_Gradient_LUT_MI.1P_Melee_Warring_Emissive_Gradient_LUT_MI"
            );
            break;

        }

        if (material && memory::IsValidPointer((uintptr_t)material))
        {
            if (gun && memory::IsValidPointer((uintptr_t)gun))
                seh_set_material((uintptr_t)gun, 0, material);
        }
    }



    void use_blind_manager_component(uobject* target_object)
    {
        auto blind_manager = static_cast<UBlindManagerComponent*>(target_object);

        if (blind_manager == nullptr)
            return;

        bool blinded = blind_manager->IsBlinded();

        if (blinded)
        {
            blind_manager->SetBlinded(false);

            blind_manager->ClientCleanseBlinds();
        }
    }



    ucanvas* canvas;

    void draw_rect21(ucanvas* canvas, float x, float y, float width, float height, flinearcolor color) {
        canvas->k2_drawline({ x, y }, { x + width, y }, 1.0f, color);
        canvas->k2_drawline({ x + width, y }, { x + width, y + height }, 1.0f, color);
        canvas->k2_drawline({ x + width, y + height }, { x, y + height }, 1.0f, color);
        canvas->k2_drawline({ x, y + height }, { x, y }, 1.0f, color);
    }

    inline auto DrawBorder(ucanvas* canva, float x, float y, float w, float h, float px, flinearcolor BorderColor) -> void
    {
        draw_rect21(canva, x, (y + h - px), w, px, BorderColor);
        draw_rect21(canva, x, y, px, h, BorderColor);
        draw_rect21(canva, x, y, w, px, BorderColor);
        draw_rect21(canva, (x + w - px), y, px, h, BorderColor);
    }

    void DrawLineCanvas(ucanvas* canvas, int x1, int y1, int x2, int y2, flinearcolor color, int thickness)
    {
        canvas->k2_drawline(fvector2d(x1, y1), fvector2d(x2, y2), thickness, color);
    }


    void draw_line_2(ucanvas* canvas, fvector2d from, fvector2d to, int thickness, flinearcolor color)
    {
        canvas->k2_drawline(from, to, static_cast<float>(thickness), color);
    }









    void meshp3_material12(acknowledgedpawn* pawn, ashootercharacter* shooter_character)
    {
        // All your materials as a selectable array
        static const wchar_t* material_paths[] =
        {
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/Ninja/AK_Ninja_Shuriken_MI.AK_Ninja_Shuriken_MI",
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/CosmosShader/CyberCity/CyberCity_MI.CyberCity_MI",
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/CosmosShader/SovWorld/3p_SovWorld_MI.3p_SovWorld_MI",
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/CosmosShader/UnderTheSea/UnderTheSea_MI.UnderTheSea_MI",
        };

        int material_count = sizeof(material_paths) / sizeof(material_paths[0]);


        int selected_material = globals::misc::chams_material_index;

        if (selected_material < 0 || selected_material >= material_count)
            selected_material = 0;

        // Load selected material dynamically
        uobject* material = uobject::static_load_object(
            nullptr,
            nullptr,
            material_paths[selected_material]
        );

        if (!material) return;

        fname first_name = string::string_to_name(L"Base Color");
        fname second_name = string::string_to_name(L"Emissive Color");

        uskeletalmeshcomponent* myselfchams = shooter_character->GetCosmeticMesh3P();

        if (globals::misc::playerchamsself && myselfchams)
        {
            auto num_materials = myselfchams->get_num_materials();
            for (int i = 0; i < num_materials; i++) {
                uobject* material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)myselfchams, i, material);
                if (material_instance_dynamic)
                {
                    static float t = 0.0f;
                    t += 0.0004f;
                    if (t > 1.0f) t = 0.0f;

                    float r = fabsf(sinf(t * 6.2831f));
                    float g = fabsf(sinf((t + 0.33f) * 6.2831f));
                    float b = fabsf(sinf((t + 0.66f) * 6.2831f));

                    flinearcolor rainbow_color = { r, g, b };

                    auto dynamic_mat = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                    dynamic_mat->set_vector_parameter_value1(first_name, rainbow_color);
                    dynamic_mat->set_vector_parameter_value1(second_name, rainbow_color);
                }
            }
        }
    }

    //void ashen_crystal_material(acknowledgedpawn* pawn, ashootercharacter* shooter_character)
    //{
    //    if (!shooter_character) return;

    //    UPrimitiveComponent* myselfchams = memory::read<UPrimitiveComponent*>(uintptr_t(pawn) + offsets::mesh_cosmetic_3p);

    //    if (globals::misc::ashen_crystal_enabled && myselfchams)
    //    {
    //        // Load Ashen Crystal material
    //        uobject* material = uobject::static_load_object(nullptr, nullptr,
    //            L"/Game/Equippables/_Core/Materials/SpecialMaterials/Ashen/Ashen_Crystal_v3_MI.Ashen_Crystal_v3_MI");

    //        if (!material) return;

    //        auto num_materials = myselfchams->get_num_materials();

    //        for (int i = 0; i < num_materials; i++) {
    //            auto material_instance_dynamic = myselfchams->create_and_set_material_instance_dynamic_from_material(i, material);

    //            if (material_instance_dynamic)
    //            {
    //                auto dynamic_mat = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
    //                if (!dynamic_mat) continue;

    //                // ParamÃ¨tres Ashen Crystal
    //                fname diffuse_overlay = string::string_to_name(L"Diffuse Overlay Intensity");
    //                fname inner_b_emissive = string::string_to_name(L"Inner (B) Emissive Intenstiy");
    //                fname inner_g_emissive = string::string_to_name(L"Inner (G) Emissive Intenstiy");
    //                fname bump_offset = string::string_to_name(L"Bump Offset Height");
    //                fname texture_tiling = string::string_to_name(L"Texture Tiling");
    //                fname reflection_vector = string::string_to_name(L"Reflection Vector Intensity");
    //                fname flow_map_speed = string::string_to_name(L"Flow Map Speed Y");
    //                fname panner_y = string::string_to_name(L"Panner Y");
    //                fname diffuse_multiply = string::string_to_name(L"Diffuse Multiply");
    //                fname diffuse_power = string::string_to_name(L"Diffuse Power");
    //                fname refraction_bias = string::string_to_name(L"RefractionDepthBias");

    //                // Animation
    //                static float animation_time = 0.0f;
    //                animation_time += 0.08f;
    //                if (animation_time > 6.283f) animation_time = 0.0f;

    //                float animated_flow = 0.1f + sinf(animation_time * 2.0f) * 1.5f;
    //                float animated_panner = 0.05f + cosf(animation_time * 1.5f) * 1.2f;

    //                // ParamÃ¨tres galaxy
    //                dynamic_mat->set_scalar_parameter_value(diffuse_overlay, 12.0f);
    //                dynamic_mat->set_scalar_parameter_value(inner_b_emissive, 8.0f);
    //                dynamic_mat->set_scalar_parameter_value(inner_g_emissive, 1.0f);
    //                dynamic_mat->set_scalar_parameter_value(bump_offset, -5.0f);
    //                dynamic_mat->set_scalar_parameter_value(texture_tiling, 250.0f);
    //                dynamic_mat->set_scalar_parameter_value(reflection_vector, 6.0f);
    //                dynamic_mat->set_scalar_parameter_value(flow_map_speed, animated_flow);
    //                dynamic_mat->set_scalar_parameter_value(panner_y, animated_panner);
    //                dynamic_mat->set_scalar_parameter_value(diffuse_multiply, 5.0f);
    //                dynamic_mat->set_scalar_parameter_value(diffuse_power, 2.0f);
    //                dynamic_mat->set_scalar_parameter_value(refraction_bias, 2.5f);

    //                // Couleurs par dÃ©faut
    //                fname emissive_surface = string::string_to_name(L"Emissive Surface Color");
    //                fname diffuse_tint = string::string_to_name(L"Diffuse Tint");

    //                flinearcolor default_color = { 1.0f, 1.0f, 1.0f, 1.0f };

    //                dynamic_mat->set_vector_parameter_value1(emissive_surface, default_color);
    //                dynamic_mat->set_vector_parameter_value1(diffuse_tint, default_color);
    //            }
    //        }
    //    }
    //}















    bool HasVisibleEnemy(tarray<ashootercharacter*>& actors, aplayercontroller* controllers, ashootercharacter* character) {
        if (!controllers || !memory::IsValidPointer((uintptr_t)controllers)) return false;
        if (!character || !memory::IsValidPointer((uintptr_t)character)) return false;
        if (actors.count <= 0 || actors.count > 10000) return false;
        if (!memory::IsValidPointer((uintptr_t)actors.data)) return false;

        for (int32_t idx = 0; idx < actors.count; ++idx) {
            ashootercharacter* actor = actors[idx];
            if (!actor || !memory::IsValidPointer((uintptr_t)actor)) continue;
            if (actor == character) continue;
            if (!actor->is_alive()) continue;

            if (controllers->line_of_sight(actor)) {
                return true;
            }
        }
        return false;
    }


    bool GetAllActorsSafely(uworld* world, uobject* actor_class, tarray<AGameObject*>* Objects) {
        if (!world || !memory::IsValidPointer((uintptr_t)world)) {
            return false;
        }
        if (!actor_class || !memory::IsValidPointer((uintptr_t)actor_class)) {
            return false;
        }

        memset(Objects, 0, sizeof(*Objects));
        GameplayStatics::GetAllActorsOfClass2(world, actor_class, Objects);

        if (!memory::IsValidPointer((uintptr_t)Objects->data) && Objects->Num() > 0) {
            return false;
        }

        return true;
    }

    void apply_custom_hand_texture(acknowledgedpawn* pawn, ashootercharacter* character, uworld* world)
    {
        if (!pawn || !character || !world) return;
        if (!character->is_alive()) return;

        UPrimitiveComponent* mesh1p = memory::read<UPrimitiveComponent*>(uintptr_t(pawn) + offsets::mesh1p);
        UPrimitiveComponent* meshOverlay = memory::read<UPrimitiveComponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay);

        if (!mesh1p || !meshOverlay) return;
        if (!memory::IsValidPointer((uintptr_t)mesh1p) || !memory::IsValidPointer((uintptr_t)meshOverlay)) return;

        fstring customHandTexturePath = fstring(L"C:/hand.jpg");
        uobject* CustomHandTexture = system::import_file_as_texture2d(world, customHandTexturePath);

        if (!CustomHandTexture || !memory::IsValidPointer((uintptr_t)CustomHandTexture)) return;

        auto handMatPath = L"/Game/Equippables/_Core/Materials/SpecialMaterials/CosmosShader/Winter/Winter_MI.Winter_MI";
        uobject* handMaterial = uobject::find_object<uobject*>(handMatPath);

        if (!handMaterial) {
            handMaterial = uobject::static_load_object(nullptr, nullptr, handMatPath);
        }

        if (!handMaterial || !memory::IsValidPointer((uintptr_t)handMaterial)) return;

        if (mesh1p && memory::IsValidPointer((uintptr_t)mesh1p)) {
            seh_set_material((uintptr_t)mesh1p, 0, handMaterial);
            uobject* HandDynamicMat = seh_create_dynamic_mat((uintptr_t)mesh1p, 0, handMaterial);

            if (HandDynamicMat && memory::IsValidPointer((uintptr_t)HandDynamicMat)) {
                auto* handMat = HandDynamicMat->cast<UMaterialInstanceDynamic>();
                if (handMat) {
                    fname handParam1 = string::string_to_name(crypt(L"Image 1").decrypt());
                    fname handParam2 = string::string_to_name(crypt(L"Image 2").decrypt());

                    handMat->set_texture_parameter_value(handParam1, CustomHandTexture);
                    handMat->set_texture_parameter_value(handParam2, CustomHandTexture);

                    fname baseColorParam = string::string_to_name(crypt(L"Base Color").decrypt());
                    fname diffuseParam = string::string_to_name(crypt(L"Diffuse").decrypt());
                    fname textureParam = string::string_to_name(crypt(L"Texture").decrypt());

                    handMat->set_texture_parameter_value(baseColorParam, CustomHandTexture);
                    handMat->set_texture_parameter_value(diffuseParam, CustomHandTexture);
                    handMat->set_texture_parameter_value(textureParam, CustomHandTexture);
                }
            }
        }

        if (meshOverlay && memory::IsValidPointer((uintptr_t)meshOverlay)) {
            seh_set_material((uintptr_t)meshOverlay, 0, handMaterial);
            uobject* OverlayDynamicMat = seh_create_dynamic_mat((uintptr_t)meshOverlay, 0, handMaterial);

            if (OverlayDynamicMat && memory::IsValidPointer((uintptr_t)OverlayDynamicMat)) {
                auto* overlayMat = OverlayDynamicMat->cast<UMaterialInstanceDynamic>();
                if (overlayMat) {
                    fname overlayParam1 = string::string_to_name(crypt(L"Image 1").decrypt());
                    fname overlayParam2 = string::string_to_name(crypt(L"Image 2").decrypt());

                    overlayMat->set_texture_parameter_value(overlayParam1, CustomHandTexture);
                    overlayMat->set_texture_parameter_value(overlayParam2, CustomHandTexture);

                    fname baseColorParam = string::string_to_name(crypt(L"Base Color").decrypt());
                    fname diffuseParam = string::string_to_name(crypt(L"Diffuse").decrypt());
                    fname textureParam = string::string_to_name(crypt(L"Texture").decrypt());

                    overlayMat->set_texture_parameter_value(baseColorParam, CustomHandTexture);
                    overlayMat->set_texture_parameter_value(diffuseParam, CustomHandTexture);
                    overlayMat->set_texture_parameter_value(textureParam, CustomHandTexture);
                }
            }
        }
    }





    //using FinisherFn = void* (__fastcall*)(uintptr_t);

    //inline void* PlayFinisherEffect(uintptr_t effect)
    //{
    //    SPOOF_FUNC;
    //    static void* (__fastcall * fn)(uintptr_t) = nullptr;
    //    if (!fn)
    //        fn = reinterpret_cast<FinisherFn>(memory::module_base + offsets::player_finisher);

    //    return fn(effect);
    //}

    //void hk_death(ashootercharacter* shooter_character, UDamageResponse* a2) {
    //    try {
    //        if (!shooter_character || !memory::IsValidPointer((uintptr_t)shooter_character)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Get local player context
    //        acknowledgedpawn* pawn = controllers->get_acknowledged_pawn();
    //        ashootercharacter* character_context = character;
    //        acknowledgedpawn* local_pawn_context = pawn;
    //        auto damage_response = a2;

    //        if (!character_context || !local_pawn_context || !damage_response) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Validate pointers
    //        if (!memory::IsValidPointer((uintptr_t)character_context) ||
    //            !memory::IsValidPointer((uintptr_t)local_pawn_context) ||
    //            !memory::IsValidPointer((uintptr_t)damage_response)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Get death reaction component
    //        auto component = (uintptr_t)memory::read<uobject*>((uintptr_t)shooter_character + offsets::death_reaction_component_offset);

    //        if (!component || !memory::IsValidPointer(component)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Check death reaction flags
    //        BYTE b1 = memory::read<BYTE>(component + 0x15A);
    //        BYTE b2 = memory::read<BYTE>(component + 0x168);

    //        if (!(b1 == 0 || b2 == 1)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Get killer and weapon info
    //        auto killer = damage_response->get_damage_causer();
    //        auto equippable = damage_response->get_equippable_used();

    //        if (!killer || !equippable ||
    //            !memory::IsValidPointer((uintptr_t)killer) ||
    //            !memory::IsValidPointer((uintptr_t)equippable)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Get world context
    //        uworld* world_save = nullptr;
    //        uintptr_t* uworld_state_ptr = *(uintptr_t**)(memory::module_base + offsets::gworld);
    //        if (uworld_state_ptr) {
    //            world_save = *(uworld**)uworld_state_ptr;
    //        }

    //        if (!world_save || !memory::IsValidPointer((uintptr_t)world_save)) {
    //            return oHkDeath(shooter_character, a2);
    //        }

    //        // Find enemies
    //        tarray<ashootercharacter*> enemies = blueprints::find_all_shooters_with_alliance(
    //            world_save, character, earesalliance::enemy, false, true);

    //        currentequippable* my_weapon = character->get_inventory()->get_current_equippable();

    //        // Check if finisher should be processed
    //        bool process_finisher = globals::misc::finisher &&
    //            character->is_alive() &&
    //            character->health() > 0 &&
    //            character &&
    //            memory::IsValidPointer((uintptr_t)character);

    //        if (process_finisher) {
    //            if (killer == local_pawn_context) {
    //                int num_enemies = enemies.count;

    //                // Remove the killed enemy from count
    //                for (int i = 0; i < enemies.count; ++i) {
    //                    if (enemies[i] == shooter_character) {
    //                        num_enemies -= 1;
    //                        break;
    //                    }
    //                }

    //                // Check finisher conditions
    //                bool should_play_finisher = globals::misc::only_last_kill ? (num_enemies == 0) : true;

    //                if (globals::misc::finisher && should_play_finisher) {
    //                    std::string weapon_name = system::get_object_name(my_weapon).to_str();

    //                    auto apply_finisher = [&]() {
    //                       /* std::wstring skin = get_chosen_skin(weapon_name);
    //                        printf("[DEBUG] get_chosen_skin weapon_name: %s\n", weapon_name.c_str());
    //                        wprintf(L"[DEBUG] Chosen skin: %ls\n", skin.empty() ? L"(none)" : skin.c_str());
    //                        uobject* finisher = get_finisher_from_skin(skin.c_str());*/

    //                        /*    if (!finisher) {
    //                                return;
    //                            }*/

    //                            // Set up finisher override
    //                        static uobject* dummy_finisher = uobject::find_object<uobject*>(L"FXC_Finisher_Champions_Victim_C", (uobject*)-1);

    //                        // Clear existing overrides
    //               /*         memory::write<uobject*>(component + offsets::montage_effect_override_offset, dummy_finisher);
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);*/

    //                        memory::write<uobject*>(component + offsets::montage_effect_override_offset, nullptr);
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);

    //                        // Set new finisher
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_offset, dummy_finisher);
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, local_pawn_context);


    //                        PlayFinisherEffect(component); // btw it's not crashing just freezing so it's a func
    //                        };

    //                    // Check weapon type and apply finisher
    //                    if (weapon_name.find("AssaultRifle_AK_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("AssaultRifle_ACR_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("BoltSniper_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("AssaultRifle_Burst_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("AutomaticPistol_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("DMR_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("RevolverPistol_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("LugerPistol_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("SubMachineGun_MP5_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("BasePistol_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("LeverSniperRifle_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("DS_Gun_C") != std::string::npos) {
    //                        apply_finisher();
    //                    }
    //                    else if (weapon_name.find("Ability_Melee_Base_C") != std::string::npos) {
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_offset, nullptr);
    //                        memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);
    //                    }
    //                }
    //            }
    //        }
    //    }
    //    catch (...) {
    //        // Silent catch
    //    }

    //    return oHkDeath(shooter_character, a2);
    //}



    static inline uobject* GetExponentialHeightFogClass()
    {
        static uobject* cls = nullptr;
        if (!cls)
            cls = uobject::find_object<uobject*>(crypt(L"Engine.ExponentialHeightFog").decrypt());
        return cls;
    }

    void apply_crystal_chams_to_self(ashootercharacter* local_player, ugameinstance* gameinstance)
    {
        if (!local_player || !gameinstance) return;

        static int self_frame_counter = 0;
        static int last_preset = -1;

        self_frame_counter++;
        if (self_frame_counter % 6 != 0) return;

        if (last_preset != globals::visuals::crystal_chams_preset) {
            switch (globals::visuals::crystal_chams_preset) {
            case 0:
                globals::visuals::Self_CenterEdgeR = 0.53f;
                globals::visuals::Self_CenterEdgeG = 0.58f;
                globals::visuals::Self_CenterEdgeB = 0.52f;
                globals::visuals::Self_InnerEdgeR = 12.64f;
                globals::visuals::Self_InnerEdgeG = 15.00f;
                globals::visuals::Self_InnerEdgeB = 0.89f;
                globals::visuals::Self_OuterEdgeR = 2.47f;
                globals::visuals::Self_OuterEdgeG = 11.64f;
                globals::visuals::Self_OuterEdgeB = 0.00f;

                globals::visuals::GlowVisible = 1.5f;
                globals::visuals::AlphaBasePower = 2.0f;
                globals::visuals::AlphaColorMult = 1.2f;
                globals::visuals::DepthBias = 0.1f;
                globals::visuals::AlphaDissolveOpacity = 0.8f;
                globals::visuals::BoundingBox = 1.0f;
                globals::visuals::InnerEdgeThickness = 0.3f;
                globals::visuals::OuterEdgeThickness = 0.2f;
                globals::visuals::RimFresnel = 2.5f;
                globals::visuals::RimMultiply = 1.0f;
                globals::visuals::RimPower = 3.0f;
                globals::visuals::OcclusionDepth = 0.5f;
                globals::visuals::OcclusionBehindWall = 0.3f;
                globals::visuals::OcclusionState = 1.0f;
                globals::visuals::RefractionDepthBias = 0.05f;
                break;

            case 1:
                globals::visuals::Self_CenterEdgeR = 0.58f;
                globals::visuals::Self_CenterEdgeG = 0.63f;
                globals::visuals::Self_CenterEdgeB = 0.52f;
                globals::visuals::Self_InnerEdgeR = 0.00f;
                globals::visuals::Self_InnerEdgeG = 0.00f;
                globals::visuals::Self_InnerEdgeB = 0.00f;
                globals::visuals::Self_OuterEdgeR = 2.67f;
                globals::visuals::Self_OuterEdgeG = 0.00f;
                globals::visuals::Self_OuterEdgeB = 0.00f;

                globals::visuals::GlowVisible = 1.8f;
                globals::visuals::AlphaBasePower = 2.2f;
                globals::visuals::AlphaColorMult = 1.0f;
                globals::visuals::DepthBias = 0.15f;
                globals::visuals::AlphaDissolveOpacity = 0.9f;
                globals::visuals::BoundingBox = 1.0f;
                globals::visuals::InnerEdgeThickness = 0.4f;
                globals::visuals::OuterEdgeThickness = 0.25f;
                globals::visuals::RimFresnel = 3.0f;
                globals::visuals::RimMultiply = 1.2f;
                globals::visuals::RimPower = 2.5f;
                globals::visuals::OcclusionDepth = 0.6f;
                globals::visuals::OcclusionBehindWall = 0.4f;
                globals::visuals::OcclusionState = 1.0f;
                globals::visuals::RefractionDepthBias = 0.08f;
                break;

            case 2:
                globals::visuals::Self_CenterEdgeR = 0.63f;
                globals::visuals::Self_CenterEdgeG = 0.63f;
                globals::visuals::Self_CenterEdgeB = 0.58f;
                globals::visuals::Self_InnerEdgeR = 0.00f;
                globals::visuals::Self_InnerEdgeG = 12.74f;
                globals::visuals::Self_InnerEdgeB = 13.64f;
                globals::visuals::Self_OuterEdgeR = 3.36f;
                globals::visuals::Self_OuterEdgeG = 0.00f;
                globals::visuals::Self_OuterEdgeB = 0.00f;

                globals::visuals::GlowVisible = 2.0f;
                globals::visuals::AlphaBasePower = 1.8f;
                globals::visuals::AlphaColorMult = 1.5f;
                globals::visuals::DepthBias = 0.12f;
                globals::visuals::AlphaDissolveOpacity = 0.7f;
                globals::visuals::BoundingBox = 1.0f;
                globals::visuals::InnerEdgeThickness = 0.35f;
                globals::visuals::OuterEdgeThickness = 0.18f;
                globals::visuals::RimFresnel = 2.8f;
                globals::visuals::RimMultiply = 1.1f;
                globals::visuals::RimPower = 3.2f;
                globals::visuals::OcclusionDepth = 0.4f;
                globals::visuals::OcclusionBehindWall = 0.2f;
                globals::visuals::OcclusionState = 1.0f;
                globals::visuals::RefractionDepthBias = 0.06f;
                break;

            case 3:
                globals::visuals::Self_CenterEdgeR = 0.68f;
                globals::visuals::Self_CenterEdgeG = 0.63f;
                globals::visuals::Self_CenterEdgeB = 0.58f;
                globals::visuals::Self_InnerEdgeR = 0.00f;
                globals::visuals::Self_InnerEdgeG = 12.74f;
                globals::visuals::Self_InnerEdgeB = 13.64f;
                globals::visuals::Self_OuterEdgeR = 3.36f;
                globals::visuals::Self_OuterEdgeG = 0.00f;
                globals::visuals::Self_OuterEdgeB = 0.00f;

                globals::visuals::GlowVisible = 1.6f;
                globals::visuals::AlphaBasePower = 2.5f;
                globals::visuals::AlphaColorMult = 0.9f;
                globals::visuals::DepthBias = 0.08f;
                globals::visuals::AlphaDissolveOpacity = 0.85f;
                globals::visuals::BoundingBox = 1.0f;
                globals::visuals::InnerEdgeThickness = 0.25f;
                globals::visuals::OuterEdgeThickness = 0.15f;
                globals::visuals::RimFresnel = 2.2f;
                globals::visuals::RimMultiply = 0.8f;
                globals::visuals::RimPower = 2.8f;
                globals::visuals::OcclusionDepth = 0.7f;
                globals::visuals::OcclusionBehindWall = 0.5f;
                globals::visuals::OcclusionState = 1.0f;
                globals::visuals::RefractionDepthBias = 0.04f;
                break;

            case 4:
                globals::visuals::Self_CenterEdgeR = 0.00f;
                globals::visuals::Self_CenterEdgeG = 0.63f;
                globals::visuals::Self_CenterEdgeB = 0.47f;
                globals::visuals::Self_InnerEdgeR = 0.00f;
                globals::visuals::Self_InnerEdgeG = 12.74f;
                globals::visuals::Self_InnerEdgeB = 13.64f;
                globals::visuals::Self_OuterEdgeR = 3.25f;
                globals::visuals::Self_OuterEdgeG = 0.00f;
                globals::visuals::Self_OuterEdgeB = 0.00f;

                globals::visuals::GlowVisible = 2.2f;
                globals::visuals::AlphaBasePower = 1.9f;
                globals::visuals::AlphaColorMult = 1.3f;
                globals::visuals::DepthBias = 0.18f;
                globals::visuals::AlphaDissolveOpacity = 0.75f;
                globals::visuals::BoundingBox = 1.0f;
                globals::visuals::InnerEdgeThickness = 0.45f;
                globals::visuals::OuterEdgeThickness = 0.28f;
                globals::visuals::RimFresnel = 3.5f;
                globals::visuals::RimMultiply = 1.4f;
                globals::visuals::RimPower = 2.6f;
                globals::visuals::OcclusionDepth = 0.35f;
                globals::visuals::OcclusionBehindWall = 0.25f;
                globals::visuals::OcclusionState = 1.0f;
                globals::visuals::RefractionDepthBias = 0.09f;
                break;
            }
            last_preset = globals::visuals::crystal_chams_preset;
        }

        if (!globals::visuals::crystal_chams_enabled) {
            auto self_main_mesh = local_player->get_mesh();
            if (self_main_mesh) {
                local_player->reset_character_materials_internal(self_main_mesh);
            }
            uskeletalmeshcomponent* self_cosmetic_mesh = local_player->GetCosmeticMesh3P();
            if (self_cosmetic_mesh) {
                local_player->reset_character_materials_internal(self_cosmetic_mesh);
            }

            auto mesh1p = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + 0x0F10);
            auto meshOverlay = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + 0x0F18);
            if (mesh1p) local_player->reset_character_materials_internal(mesh1p);
            if (meshOverlay) local_player->reset_character_materials_internal(meshOverlay);
            return;
        }

        uobject* crystal_material = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Characters/BountyHunter/S0/VFX/Materials/BountyHunterReveal_MI.BountyHunterReveal_MI");

        if (!crystal_material || !memory::IsValidPointer((uintptr_t)crystal_material)) return;


        fname silohuette_color_name = string::string_to_name(L"SilohuetteColor");
        fname center_edge_color_name = string::string_to_name(L"CenterEdgeColor");
        fname inner_edge_color_name = string::string_to_name(L"InnerEdgeColor");
        fname outer_edge_color_name = string::string_to_name(L"OuterEdgeColor");
        fname glow_intensity_param = string::string_to_name(L"GlowIntensity");


        fname alpha_base_power_name = string::string_to_name(L"Alpha_Base_Power");
        fname depth_bias_name = string::string_to_name(L"DepthBias");
        fname alpha_dissolve_opacity_name = string::string_to_name(L"Alpha_Dissolve_Opacity");
        fname bounding_box_name = string::string_to_name(L"BoundingBox");
        fname inner_edge_thickness_name = string::string_to_name(L"InnerEdgeThickness");
        fname outer_edge_thickness_name = string::string_to_name(L"OuterEdgeThickness");
        fname rim_fresnel_name = string::string_to_name(L"Rim_Fresnel");
        fname rim_multiply_name = string::string_to_name(L"Rim_Multiply");
        fname rim_power_name = string::string_to_name(L"Rim_Power");
        fname occlusion_behind_wall_name = string::string_to_name(L"OcclusionDepth_BehindWall");
        fname occlusion_state_name = string::string_to_name(L"OcclusionState");
        fname refraction_depth_bias_name = string::string_to_name(L"RefractionDepthBias");


        float self_glowIntensity = globals::visuals::GlowVisible;
        float alpha_base_power = globals::visuals::AlphaBasePower;
        float alpha_colormult = globals::visuals::AlphaColorMult;
        float depth_bias = globals::visuals::DepthBias;
        float alpha_dissolve_opacity = globals::visuals::AlphaDissolveOpacity;
        float bounding_box = globals::visuals::BoundingBox;
        float inner_edge_thickness = globals::visuals::InnerEdgeThickness;
        float outer_edge_thickness = globals::visuals::OuterEdgeThickness;
        float rim_fresnel = globals::visuals::RimFresnel;
        float rim_multiply = globals::visuals::RimMultiply;
        float rim_power = globals::visuals::RimPower;
        float occlusion_behind_wall = globals::visuals::OcclusionBehindWall;
        float occlusion_state = globals::visuals::OcclusionState;
        float refraction_depth_bias = globals::visuals::RefractionDepthBias;

        flinearcolor self_centerEdgeColor = flinearcolor(
            globals::visuals::Self_CenterEdgeR,
            globals::visuals::Self_CenterEdgeG,
            globals::visuals::Self_CenterEdgeB,
            globals::visuals::intensityvisibleoutline
        );
        flinearcolor self_innerEdgeColor = flinearcolor(
            globals::visuals::Self_InnerEdgeR,
            globals::visuals::Self_InnerEdgeG,
            globals::visuals::Self_InnerEdgeB,
            globals::visuals::intensityvisibleoutline
        );
        flinearcolor self_outerEdgeColor = flinearcolor(
            globals::visuals::Self_OuterEdgeR,
            globals::visuals::Self_OuterEdgeG,
            globals::visuals::Self_OuterEdgeB,
            globals::visuals::intensityvisibleoutline
        );


        auto apply_material = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;

            int num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto material_instance_dynamic = seh_create_dynamic_mat((uintptr_t)mesh, i, crystal_material);
                if (!material_instance_dynamic || !memory::IsValidPointer((uintptr_t)material_instance_dynamic)) continue;
                auto dynCast = material_instance_dynamic->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;

                dynCast->set_vector_parameter_value1(silohuette_color_name, self_outerEdgeColor);
                dynCast->set_vector_parameter_value1(center_edge_color_name, self_centerEdgeColor);
                dynCast->set_vector_parameter_value1(inner_edge_color_name, self_innerEdgeColor);
                dynCast->set_vector_parameter_value1(outer_edge_color_name, self_outerEdgeColor);
                dynCast->set_scalar_parameter_value(glow_intensity_param, self_glowIntensity);
                dynCast->set_scalar_parameter_value(alpha_base_power_name, alpha_base_power);
                dynCast->set_scalar_parameter_value(depth_bias_name, depth_bias);
                dynCast->set_scalar_parameter_value(alpha_dissolve_opacity_name, alpha_dissolve_opacity);
                dynCast->set_scalar_parameter_value(bounding_box_name, bounding_box);
                dynCast->set_scalar_parameter_value(inner_edge_thickness_name, inner_edge_thickness);
                dynCast->set_scalar_parameter_value(outer_edge_thickness_name, outer_edge_thickness);
                dynCast->set_scalar_parameter_value(rim_fresnel_name, rim_fresnel);
                dynCast->set_scalar_parameter_value(rim_multiply_name, rim_multiply);
                dynCast->set_scalar_parameter_value(rim_power_name, rim_power);
                dynCast->set_scalar_parameter_value(occlusion_behind_wall_name, occlusion_behind_wall);
                dynCast->set_scalar_parameter_value(occlusion_state_name, occlusion_state);
                dynCast->set_scalar_parameter_value(refraction_depth_bias_name, refraction_depth_bias);
            }
            };

        apply_material(local_player->get_mesh());
        apply_material(local_player->GetCosmeticMesh3P());

        auto mesh1p = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + offsets::mesh1p);
        auto meshOverlay = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + offsets::mesh1p_overlay);

        apply_material(mesh1p);
        apply_material(meshOverlay);
    }


    static std::unordered_map<uobject*, std::string> objectNameCache;
    inline std::string get_cached_name(uobject* obj) {
        auto it = objectNameCache.find(obj);
        if (it != objectNameCache.end()) return it->second;
        std::string name = system::get_object_name(obj).ToString();
        objectNameCache[obj] = name;
        return name;
    }
    static const std::pair<const char*, const char*> kWeaponToFamily[] = {
        {"AssaultRifle_AK_C",           "AssaultRifle_AK"},
        {"AssaultRifle_ACR_C",          "AssaultRifle_ACR"},
        {"BoltSniper_C",                "BoltSniper"},
        {"AssaultRifle_Burst_C",        "AssaultRifle_Burst"},
        {"AutomaticPistol_C",           "AutomaticPistol"},
        {"DMR_C",                        "DMR"},
        {"RevolverPistol_C",            "Revolver"},
        {"LugerPistol_C",               "LugerPistol"},
        {"SubMachineGun_MP5_C",         "SubMachineGun_MP5"},
        {"Vector_C",                    "SubMachineGun_Vector"},
        {"BasePistol_C",                "BasePistol"},
        {"LeverSniperRifle_C",          "LeverSniper"},
        {"DS_Gun_C",                    "DS_Gun"},
        {"Ability_Melee_Base_C",        "Melee"},
        {"HeavyMachineGun_C",           "HeavyMachineGun"},
        {"LightMachineGun_C",           "LightMachineGun"},
        {"SawedOffShotgun_C",           "SawedOffShotgun"},
        {"AutomaticShotgun_C",          "AutomaticShotgun"},
        {"PumpShotgun_C",               "PumpShotgun"},
    };
    struct SkinItem {
        std::wstring name;
    };
    static std::unordered_map<std::string, std::vector<SkinItem>> g_byFamily;
    static std::unordered_map<std::string, int> g_selectedIndexForFamily;
    std::wstring get_chosen_skin(const std::string& weapon_name) {


        std::string family;
        for (const auto& [key, fam] : kWeaponToFamily) {
            if (weapon_name == key) {
                family = fam;
                break;
            }
        }

        if (family.empty()) {

            return L"";
        }



        auto it = g_byFamily.find(family);
        if (it == g_byFamily.end()) {
            return L"";
        }

        int index = g_selectedIndexForFamily[family];
        auto& skins = it->second;

        if (index < 0 || index >= static_cast<int>(skins.size())) {

            return L"";
        }

        return skins[index].name;
    }
    static std::string family_from_logged_name(const std::wstring& wname) {
        std::string s(wname.begin(), wname.end());

        const std::string pre = "Default__";
        const std::string suf = "_PrimaryAsset_C";
        if (s.rfind(pre, 0) == 0) s.erase(0, pre.size());
        if (s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0)
            s.erase(s.size() - suf.size());

        if (s.rfind("AK_", 0) == 0) return "AssaultRifle_AK";
        if (s.rfind("Melee_", 0) == 0) return "Melee";
        if (s.rfind("Vector_", 0) == 0) return "SubMachineGun_Vector";
        if (s.rfind("Luger_", 0) == 0) return "LugerPistol";

        auto u1 = s.find('_');
        if (u1 == std::string::npos) return s;
        auto u2 = s.find('_', u1 + 1);
        return (u2 == std::string::npos) ? s.substr(0, u1) : s.substr(0, u2);
    }
    static void store_skin_by_name(const std::wstring& fullName) {
        std::string fam = family_from_logged_name(fullName);
        auto& vec = g_byFamily[fam];

        if (std::none_of(vec.begin(), vec.end(), [&](const SkinItem& it) { return it.name == fullName; })) {
            vec.push_back(SkinItem{ fullName });

            if (vec.size() == 1) {
                g_selectedIndexForFamily[fam] = 0;
            }
        }
    }
    static std::unordered_map<equippable_skin_data_asset*, std::wstring> g_skinNameCache;
    static const std::wstring& get_skin_name_cached(equippable_skin_data_asset* p, bool refresh = false) {
        static const std::wstring kEmpty;
        if (!p) return kEmpty;

        if (!refresh) {
            auto it = g_skinNameCache.find(p);
            if (it != g_skinNameCache.end()) return it->second;
        }

        // Query once, then copy into a stable std::wstring
        fstring f = system::get_object_name(p);
        auto [it, _] = g_skinNameCache.emplace(p, std::wstring(f.c_str()));
        if (!_) it->second.assign(f.c_str()); // if already existed & refresh==true
        return it->second;
    }
    std::string normalize_weapon_class(const std::string& weapon) {
        size_t pos = weapon.find_last_of('_');
        if (pos != std::string::npos && pos + 1 < weapon.size() &&
            std::all_of(weapon.begin() + pos + 1, weapon.end(), ::isdigit)) {
            return weapon.substr(0, pos);
        }
        return weapon;
    }
    uobject* get_finisher_from_skin(std::wstring skinData) {
        std::wstring assetName = skinData;
        const std::wstring defaultPrefix = L"Default__";
        if (assetName.find(defaultPrefix) == 0) {
            assetName = assetName.substr(defaultPrefix.length());
        }

        size_t firstUnderscore = assetName.find(L'_');
        size_t lastUnderscore = assetName.rfind(L"_PrimaryAsset_C");

        if (firstUnderscore == std::wstring::npos || lastUnderscore == std::wstring::npos || lastUnderscore <= firstUnderscore) {
            return nullptr;
        }

        std::wstring skinNameW = assetName.substr(firstUnderscore + 1, lastUnderscore - firstUnderscore - 1);
        std::string skinName(skinNameW.begin(), skinNameW.end());

        std::string obj = "FXC_Finisher_" + skinName + "_Victim_C";
        std::wstring wobj(obj.begin(), obj.end());
        uobject* effect = uobject::find_object<uobject*>(wobj.c_str(), reinterpret_cast<uobject*>(-1));

        if (!effect && !skinName.empty()) {
            while (!skinName.empty() && std::isdigit(skinName.back())) {
                skinName.pop_back();
            }

            obj = "FXC_Finisher_" + skinName + "_Victim_C";
            wobj = std::wstring(obj.begin(), obj.end());
            effect = uobject::find_object<uobject*>(wobj.c_str(), reinterpret_cast<uobject*>(-1));
        }

        return effect;
    }




    using FinisherFn = void* (__fastcall*)(uintptr_t);

    inline void* PlayFinisherEffect(uintptr_t effect)
    {
        SPOOF_FUNC;
        static void* (__fastcall * fn)(uintptr_t) = nullptr;
        if (!fn)
            fn = reinterpret_cast<FinisherFn>(memory::module_base + offsets::play_finisher_effect);
        return fn(effect);
    }

    void hk_death(ashootercharacter* shooter_character, UDamageResponse* a2) {
        try {
            printf("[DEATH] hk_death called! shooter=0x%p a2=0x%p\n", (void*)shooter_character, (void*)a2);

            if (!shooter_character || !memory::IsValidPointer((uintptr_t)shooter_character)) {
                printf("[DEATH] shooter_character invalid, passthrough\n");
                return oHkDeath(shooter_character, a2);
            }

            acknowledgedpawn* pawn = controllers->get_acknowledged_pawn();
            ashootercharacter* character_context = (ashootercharacter*)character;
            acknowledgedpawn* local_pawn_context = (acknowledgedpawn*)pawn;
            auto damage_response = a2;

            printf("[DEATH] character_context=0x%p local_pawn=0x%p damage_response=0x%p\n",
                (void*)character_context, (void*)local_pawn_context, (void*)damage_response);

            if (!character_context || !local_pawn_context || !damage_response) {
                printf("[DEATH] FAIL: one of context/pawn/dmg is null\n");
                return oHkDeath(shooter_character, a2);
            }

            if (!memory::IsValidPointer((uintptr_t)character_context) ||
                !memory::IsValidPointer((uintptr_t)local_pawn_context) ||
                !memory::IsValidPointer((uintptr_t)damage_response)) {
                printf("[DEATH] FAIL: pointer validation failed\n");
                return oHkDeath(shooter_character, a2);
            }

            auto component = (uintptr_t)memory::read<uobject*>((uintptr_t)shooter_character + offsets::death_reaction_component_offset);
            printf("[DEATH] component=0x%p (offset=0x%X)\n", (void*)component, (int)offsets::death_reaction_component_offset);

            if (!component || !memory::IsValidPointer(component)) {
                printf("[DEATH] FAIL: component null/invalid\n");
                return oHkDeath(shooter_character, a2);
            }

            BYTE b1 = memory::read<BYTE>(component + 0x15A);
            BYTE b2 = memory::read<BYTE>(component + 0x168);
            printf("[DEATH] b1=0x%X b2=0x%X | condition=(b1==0 || b2==1) = %d\n", b1, b2, (int)(b1 == 0 || b2 == 1));

            if (!(b1 == 0 || b2 == 1)) {
                printf("[DEATH] FAIL: b1/b2 condition not met\n");
                return oHkDeath(shooter_character, a2);
            }

            auto killer = damage_response->get_damage_causer();
            auto equippable = damage_response->get_equippable_used();
            printf("[DEATH] killer=0x%p local_pawn=0x%p equippable=0x%p\n",
                (void*)killer, (void*)local_pawn_context, (void*)equippable);
            printf("[DEATH] killer==local_pawn? %d\n", (int)(killer == local_pawn_context));

            if (!killer || !equippable ||
                !memory::IsValidPointer((uintptr_t)killer) ||
                !memory::IsValidPointer((uintptr_t)equippable)) {
                printf("[DEATH] FAIL: killer or equippable null/invalid\n");
                return oHkDeath(shooter_character, a2);
            }

            if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) {
                printf("[DEATH] FAIL: UWorldSave null/invalid\n");
                return oHkDeath(shooter_character, a2);
            }

            tarray<ashootercharacter*> enemies = blueprints::find_all_shooters_with_alliance(
                UWorldSave, character, earesalliance::enemy, false, true);
            printf("[DEATH] enemies.count=%d\n", enemies.count);

            myweapon = character->get_inventory()->get_current_equippable();
            printf("[DEATH] myweapon=0x%p\n", (void*)myweapon);

            if (globals::misc::finisher &&
                character_context->is_alive() &&
                character_context->health() > 0 &&
                memory::IsValidPointer((uintptr_t)character_context))
            {
                printf("[DEATH] finisher enabled, checking killer match\n");

                if (killer == local_pawn_context)
                {
                    printf("[DEATH] killer matches local pawn -> applying finisher\n");

                    int num_enemies = enemies.count;
                    for (int i = 0; i < enemies.size(); ++i) {
                        if (enemies[i] == shooter_character) {
                            num_enemies -= 1;
                            break;
                        }
                    }
                    printf("[DEATH] num_enemies after exclude=%d only_last_kill=%d\n",
                        num_enemies, (int)globals::misc::only_last_kill);

                    bool should_play = globals::misc::only_last_kill ? (num_enemies == 0) : true;
                    printf("[DEATH] should_play_finisher=%d\n", (int)should_play);

                    if (should_play)
                    {
                        std::string raw_weapon = get_cached_name(myweapon);
                        std::string weapon_name = normalize_weapon_class(raw_weapon.c_str());
                        printf("[DEATH] raw_weapon=%s normalized=%s\n", raw_weapon.c_str(), weapon_name.c_str());

                        auto apply_finisher = [&]() {
                            std::wstring skin = get_chosen_skin(weapon_name);
                            printf("[DEATH] chosen skin=%ws\n", skin.empty() ? L"(empty)" : skin.c_str());

                            uobject* finisher = get_finisher_from_skin(skin.c_str());
                            printf("[DEATH] finisher object=0x%p\n", (void*)finisher);

                            static uobject* dummy_finisher = uobject::find_object<uobject*>(
                                L"FXC_Finisher_Invalid_Victim_C", reinterpret_cast<uobject*>(-1));
                            printf("[DEATH] dummy_finisher=0x%p\n", (void*)dummy_finisher);

                            printf("[DEATH] writing to component=0x%p offset_override=0x%X offset_ctx=0x%X\n",
                                (void*)component,
                                (int)offsets::montage_effect_override_offset,
                                (int)offsets::montage_effect_override_context_offset);

                            memory::write<uobject*>(component + offsets::montage_effect_override_offset, dummy_finisher);
                            memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);
                            memory::write<uobject*>(component + offsets::montage_effect_override_offset, nullptr);
                            memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);
                            memory::write<uobject*>(component + offsets::montage_effect_override_offset, finisher);
                            memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, local_pawn_context);

                            printf("[DEATH] calling PlayFinisherEffect(0x%p)\n", (void*)component);
                            PlayFinisherEffect(component);
                            printf("[DEATH] PlayFinisherEffect done\n");
                            };

                        bool weapon_matched = false;
                        if (weapon_name.find("AssaultRifle_AK_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("AssaultRifle_ACR_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("BoltSniper_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("AssaultRifle_Burst_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("AutomaticPistol_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("DMR_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("RevolverPistol_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("LugerPistol_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("SubMachineGun_MP5_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("BasePistol_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("LeverSniperRifle_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("DS_Gun_C") != std::string::npos) { weapon_matched = true; apply_finisher(); }
                        else if (weapon_name.find("Ability_Melee_Base_C") != std::string::npos) {
                            weapon_matched = true;
                            memory::write<uobject*>(component + offsets::montage_effect_override_offset, nullptr);
                            memory::write<uobject*>(component + offsets::montage_effect_override_context_offset, nullptr);
                        }

                        if (!weapon_matched)
                            printf("[DEATH] FAIL: weapon_name '%s' matched NO weapon case!\n", weapon_name.c_str());
                    }
                }
                else {
                    printf("[DEATH] killer != local_pawn, not our kill\n");
                }
            }
            else {
                printf("[DEATH] finisher check failed: enabled=%d alive=%d health=%d\n",
                    (int)globals::misc::finisher,
                    (int)character_context->is_alive(),
                    (int)character_context->health());
            }
        }
        catch (...) {
            printf("[DEATH] EXCEPTION caught\n");
        }

        return oHkDeath(shooter_character, a2);
    }



    namespace kismentsystemlibrary {
        static bool line_trace_single(
            uworld* world_context_object,
            fvector start,
            fvector end,
            ETraceTypeQuery trace_channel,
            bool b_trace_complex,
            tarray<AActor*> actors_to_ignore,
            EDrawDebugTrace draw_debug_type,
            FHitResult& out_hit,
            bool b_ignore_self,
            flinearcolor trace_color,
            flinearcolor trace_hit_color,
            float draw_time)
        {
            if (!world_context_object || !memory::IsValidPointer((uintptr_t)world_context_object))
                return false;

            static uobject* function = nullptr;
            if (!function)
                function = uobject::find_object<uobject*>(L"Engine.KismetSystemLibrary.LineTraceSingle");
            if (!function) return false;

            uobject* kismet = variables.kismet_system;
            if (!kismet || !memory::IsValidPointer((uintptr_t)kismet))
                kismet = uobject::find_object<uobject*>(L"Engine.Default__KismetSystemLibrary");
            if (!kismet || !memory::IsValidPointer((uintptr_t)kismet))
                return false;

            struct {
                uworld* world_context_object;
                fvector start;
                fvector end;
                ETraceTypeQuery trace_channel;
                bool b_trace_complex;
                tarray<AActor*> actors_to_ignore;
                char draw_debug_type;
                FHitResult out_hit;
                bool b_ignore_self;
                flinearcolor trace_color;
                flinearcolor trace_hit_color;
                float draw_time;
                bool return_value;
            } params{};

            params.world_context_object = world_context_object;
            params.start = start;
            params.end = end;
            params.trace_channel = trace_channel;
            params.b_trace_complex = b_trace_complex;
            params.actors_to_ignore = actors_to_ignore;
            params.draw_debug_type = (char)draw_debug_type;
            params.b_ignore_self = b_ignore_self;
            params.trace_color = trace_color;
            params.trace_hit_color = trace_hit_color;
            params.draw_time = draw_time;

            __try {
                kismet->process_event(function, &params);
                out_hit = params.out_hit;
                return params.return_value;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
    }

    inline uworld* GetWorldSafe() {
        uintptr_t* uworld_state_ptr = *(uintptr_t**)(memory::module_base + offsets::gworld);
        if (uworld_state_ptr) {
            return *(uworld**)uworld_state_ptr;
        }
        return nullptr;
    }

    namespace ShooterGameBlueprints {
        static uintptr_t GetAresGlobals() {
            static uobject* Function = nullptr;
            if (!Function) {
                Function = uobject::find_object<uobject*>(L"ShooterGame.ShooterBlueprintLibrary.GetAresGlobals");
            }
            if (!Function) return 0;

            static uobject* DefaultObj = nullptr;
            if (!DefaultObj) {
                DefaultObj = uobject::find_object<uobject*>(L"ShooterGame.Default__ShooterBlueprintLibrary");
            }
            if (!DefaultObj) return 0;

            struct {
                uintptr_t ReturnValue;
            } params = { 0 };

            DefaultObj->process_event(Function, &params);
            return params.ReturnValue;
        }

        static void GetWallPenetrationSpans(
            uworld* World,
            const fvector& Start,
            const fvector& End,
            const tarray<AActor*>& IgnoreActors,
            ECollisionChannel TraceChannel,
            float IgnoreGapTolerance,
            FWallSpanList& OutWallSpans)
        {
            static uobject* Function = nullptr;
            if (!Function) {
                Function = uobject::find_object<uobject*>(L"ShooterGame.ShooterBlueprintLibrary.GetWallPenetrationSpans");
            }

            if (!Function || !World) {
                return;
            }

            struct {
                uworld* World;
                fvector Start;
                fvector End;
                tarray<AActor*> IgnoreActors;
                ECollisionChannel TraceChannel;
                float IgnoreGapTolerance;
                FWallSpanList OutWallSpans;
            } params;

            params.World = World;
            params.Start = Start;
            params.End = End;
            params.IgnoreActors = IgnoreActors;
            params.TraceChannel = TraceChannel;
            params.IgnoreGapTolerance = IgnoreGapTolerance;

            World->process_event(Function, &params);
            OutWallSpans = params.OutWallSpans;
        }

        static uint8_t ConvertToAresSurfaceType(uint8_t SurfaceType) {
            static uobject* Function = nullptr;
            if (!Function) {
                Function = uobject::find_object<uobject*>(L"ShooterGame.ShooterBlueprintLibrary.ConvertToAresSurfaceType");
            }
            if (!Function) return 0;

            static uobject* DefaultObj = nullptr;
            if (!DefaultObj) {
                DefaultObj = uobject::find_object<uobject*>(L"ShooterGame.Default__ShooterBlueprintLibrary");
            }
            if (!DefaultObj) return 0;

            struct {
                uint8_t SurfaceType;
                uint8_t ReturnValue;
            } params;

            params.SurfaceType = SurfaceType;
            DefaultObj->process_event(Function, &params);
            return params.ReturnValue;
        }
    }


#include <string>
#include <map>







    class BoneHelper {
    public:
        static int32_t GetBonePriorityByIndex(int32_t bi, int32_t bc)
        {
            switch (bc) {
            case 101:
                if (bi == 20) return 100; if (bi == 21) return 90;
                if (bi >= 17 && bi <= 19) return 80; if (bi >= 15 && bi <= 16) return 70;
                if (bi >= 13 && bi <= 14) return 60; if (bi == 3)  return 50;
                if (bi >= 23 && bi <= 25) return 30; if (bi >= 49 && bi <= 51) return 30;
                if (bi >= 75 && bi <= 78) return 25; if (bi >= 82 && bi <= 85) return 25;
                return 10;
            case 103:
                if (bi == 8)  return 100; if (bi == 9)  return 90;
                if (bi >= 5 && bi <= 7)   return 80; if (bi == 3)  return 50;
                if (bi >= 30 && bi <= 33) return 30; if (bi >= 55 && bi <= 58) return 30;
                if (bi >= 63 && bi <= 69) return 25; if (bi >= 77 && bi <= 83) return 25;
                return 10;
            case 104:
                if (bi == 20) return 100; if (bi == 21) return 90;
                if (bi >= 17 && bi <= 19) return 80; if (bi >= 15 && bi <= 16) return 70;
                if (bi >= 13 && bi <= 14) return 60; if (bi == 3)  return 50;
                if (bi >= 23 && bi <= 25) return 30; if (bi >= 49 && bi <= 51) return 30;
                if (bi >= 77 && bi <= 80) return 25; if (bi >= 84 && bi <= 87) return 25;
                return 10;
            default:
                if (bi <= 10) return 80; if (bi <= 20) return 60; if (bi <= 30) return 40; return 20;
            }
        }

        static inline void GetCriticalBones(int32_t bc, int32_t* out, int32_t& cnt)
        {
            if (bc == 101 || bc == 104) { static const int32_t b[] = { 20,21,19,18,17,3 }; memcpy(out, b, sizeof(b)); cnt = 6; }
            else if (bc == 103) { static const int32_t b[] = { 8,9,7,6,5,3 };     memcpy(out, b, sizeof(b)); cnt = 6; }
            else cnt = 0;
        }
    };




    // ======================== AUTOWALL SYSTEM (akctwo method - gun-native, no settings) ========================

    struct WeaponPenetrationData {
        float HeadDamage;
        float BodyDamage;
        float LegDamage;
        float PenetrationPower;
        bool CanPenetrateWalls;
    };

    static std::map<std::wstring, WeaponPenetrationData> WeaponDatabase = {
        { L"Vandal",     { 160.0f,  40.0f,  34.0f, 1.00f, true  } },
        { L"Phantom",    { 156.0f,  39.0f,  33.0f, 0.95f, true  } },
        { L"Guardian",   { 195.0f,  65.0f,  49.0f, 1.05f, true  } },
        { L"Operator",   { 255.0f, 150.0f, 127.0f, 1.20f, true  } },
        { L"Marshal",    { 202.0f, 101.0f,  85.0f, 1.10f, true  } },
        { L"Outlaw",     { 238.0f, 140.0f, 119.0f, 1.15f, true  } },
        { L"Bulldog",    { 116.0f,  35.0f,  30.0f, 0.85f, true  } },
        { L"Odin",       {  95.0f,  38.0f,  32.0f, 0.80f, true  } },
        { L"Ares",       {  72.0f,  30.0f,  25.0f, 0.75f, true  } },
        { L"Spectre",    {  78.0f,  26.0f,  22.0f, 0.60f, true  } },
        { L"Stinger",    {  67.0f,  27.0f,  23.0f, 0.55f, true  } },
        { L"Sheriff",    { 159.0f,  55.0f,  47.0f, 0.70f, true  } },
        { L"Ghost",      { 105.0f,  30.0f,  26.0f, 0.65f, true  } },
        { L"Frenzy",     {  78.0f,  26.0f,  22.0f, 0.50f, false } },
        { L"Classic",    {  78.0f,  26.0f,  22.0f, 0.45f, false } },
        { L"Shorty",     {  24.0f,  12.0f,  10.0f, 0.20f, false } },
        { L"Bucky",      {  44.0f,  22.0f,  19.0f, 0.25f, false } },
        { L"Judge",      {  34.0f,  17.0f,  14.0f, 0.30f, false } }
    };

    static WeaponPenetrationData GetWeaponData(ashootercharacter* Player) {
        WeaponPenetrationData default_data = { 140.0f, 35.0f, 30.0f, 0.80f, true };
        if (!Player) return default_data;

        auto inv = Player->get_inventory();
        if (!inv) return default_data;

        auto eq = inv->get_current_equippable();
        if (!eq) return default_data;

        fstring obj_name = helper::convert_weapon_name(system::get_object_name(eq));
        std::wstring weaponKey = obj_name.c_str();

        auto it = WeaponDatabase.find(weaponKey);
        if (it != WeaponDatabase.end()) return it->second;

        return default_data;
    }

    // ======================== ULTIMATE RESOLVER (ported from src_remaded) ========================

    namespace resolver_math {
        __forceinline float AngleDiff(float a, float b) {
            float diff = fmod(b - a + 180.0f, 360.0f) - 180.0f;
            return diff < -180.0f ? diff + 360.0f : diff;
        }
        __forceinline float r_clamp(float value, float min_val, float max_val) {
            return value < min_val ? min_val : (value > max_val ? max_val : value);
        }
        __forceinline float lerp(float a, float b, float t) { return a + t * (b - a); }
    }

    template<typename T, size_t N>
    struct UltimateCircularBuffer {
        std::array<T, N> data;
        size_t head = 0;
        size_t count = 0;
        __forceinline void push_back(const T& value) { data[head] = value; head = (head + 1) % N; count = (count < N) ? count + 1 : N; }
        __forceinline T& operator[](size_t index) { return data[(head + N - count + index) % N]; }
        __forceinline const T& operator[](size_t index) const { return data[(head + N - count + index) % N]; }
        __forceinline T& back() { return (*this)[count - 1]; }
        __forceinline bool empty() const { return count == 0; }
        __forceinline void clear() { count = 0; head = 0; }
    };

    struct UltimateResolverData {
        UltimateCircularBuffer<frotator, 48> rotation_history;
        UltimateCircularBuffer<fvector, 32> position_history;
        UltimateCircularBuffer<fvector, 24> velocity_history;
        UltimateCircularBuffer<float, 96> yaw_pattern;
        UltimateCircularBuffer<float, 32> movement_angles;
        UltimateCircularBuffer<bool, 64> visibility_history;

        float base_yaw = 0.0f, real_yaw = 0.0f, fake_yaw = 0.0f;
        float pattern_frequency = 0.0f, pattern_amplitude = 0.0f;
        float resolver_confidence = 1.0f, desync_range = 58.0f;
        uint32_t pattern_analysis_ticks = 0, shots_fired = 0, shots_hit = 0, resolver_updates = 0;
        int8_t pattern_type = 0, resolved_side = 0;
        bool was_moving = false, is_desyncing = false;

        fvector last_resolved_head_pos, last_original_head_pos;
        float last_confidence = 0.0f;
        int last_pattern_type = 0;
    };

    static std::array<UltimateResolverData, 64> ultimate_resolver_states;
    static std::array<ashootercharacter*, 64> resolver_target_map = {};
    static uint32_t resolver_frame_counter = 0;

    class UltimateValorantResolver {
    private:
        ashootercharacter* m_target;
        ashootercharacter* m_local;
        UltimateResolverData* m_data;
        uint32_t m_target_index;
        float m_current_time;
        float m_yaw_variance = 0.0f;

    public:
        UltimateValorantResolver(ashootercharacter* target, ashootercharacter* local)
            : m_target(target), m_local(local) {
            m_target_index = FindOrCreateTargetIndex(target);
            m_data = &ultimate_resolver_states[m_target_index];
            m_current_time = GetTickCount64() * 0.001f;
        }

        fvector ResolveHeadPosition(fvector current_head, fvector velocity) {
            if (!m_target || !m_local || !m_target->is_alive() || !m_local->is_alive()) return current_head;
            m_current_time = GetTickCount64() * 0.001f;
            UpdateTargetData();
            AnalyzeMovement();
            AnalyzeRotation();
            AnalyzeStrafing();
            m_data->resolver_updates++;
            return ApplyResolution(current_head, velocity);
        }

        void RegisterShotFired() { m_data->shots_fired++; UpdateConfidence(); }
        void RegisterShotHit() {
            m_data->shots_hit++;
            UpdateConfidence();
            if (m_data->shots_hit > 5) {
                float hr = float(m_data->shots_hit) / float(m_data->shots_fired);
                m_data->resolver_confidence = (std::min)(1.0f, hr + 0.1f);
            }
        }
        float GetConfidence() const { return m_data->resolver_confidence; }
        UltimateResolverData* GetResolverData() const { return m_data; }
        void UpdateVisualizationData(fvector orig, fvector resolved) {
            m_data->last_original_head_pos = orig;
            m_data->last_resolved_head_pos = resolved;
            m_data->last_confidence = m_data->resolver_confidence;
            m_data->last_pattern_type = m_data->pattern_type;
        }

    private:
        uint32_t FindOrCreateTargetIndex(ashootercharacter* target) {
            for (uint32_t i = 0; i < 64; i++)
                if (resolver_target_map[i] == target) return i;
            uint32_t idx = resolver_frame_counter % 64;
            for (uint32_t i = 0; i < 64; i++)
                if (!resolver_target_map[i]) { idx = i; break; }
            resolver_target_map[idx] = target;
            ultimate_resolver_states[idx] = UltimateResolverData();
            return idx;
        }

        void UpdateTargetData() {
            uintptr_t rootComp = memory::read<uintptr_t>((uintptr_t)m_target + offsets::Rootcomponent);
            frotator rot = { 0, 0, 0 };
            if (rootComp && memory::IsValidPointer(rootComp))
                rot = memory::read<frotator>(rootComp + offsets::relative_rotation);
            fvector pos = m_target->k2_get_actor_location();
            fvector vel = m_target->get_velocity();
            m_data->rotation_history.push_back(rot);
            m_data->position_history.push_back(pos);
            m_data->velocity_history.push_back(vel);
            m_data->yaw_pattern.push_back(rot.yaw);
            m_data->pattern_analysis_ticks++;
            resolver_frame_counter++;
        }

        void AnalyzeMovement() {
            if (m_data->velocity_history.count < 3) return;
            fvector cv = m_data->velocity_history.back();
            float speed = cv.size();
            m_data->was_moving = (speed > 10.0f);
            if (speed > 1.0f) {
                float ma = atan2f(cv.y, cv.x) * (180.0f / 3.14159265f);
                m_data->movement_angles.push_back(ma);
            }
        }

        void AnalyzeStrafing() {
            if (m_data->velocity_history.count < 8) return;
            int changes = 0;
            fvector prev = m_data->velocity_history[0];
            for (size_t i = 1; i < m_data->velocity_history.count; i++) {
                fvector cur = m_data->velocity_history[i];
                float ps = prev.size(), cs = cur.size();
                if (ps > 0.01f && cs > 0.01f && prev.dot(cur) / (ps * cs) < -0.5f) changes++;
                prev = cur;
            }
            m_data->is_desyncing = (changes > (int)m_data->velocity_history.count / 3);
        }

        void AnalyzeRotation() {
            if (m_data->yaw_pattern.count < 8) return;
            float sum = 0.0f, sum_sq = 0.0f, mn = FLT_MAX, mx = -FLT_MAX;
            for (size_t i = 0; i < m_data->yaw_pattern.count; i++) {
                float y = m_data->yaw_pattern[i];
                sum += y; sum_sq += y * y;
                mn = (std::min)(mn, y); mx = (std::max)(mx, y);
            }
            float mean = sum / m_data->yaw_pattern.count;
            m_yaw_variance = (sum_sq / m_data->yaw_pattern.count) - (mean * mean);
            m_data->pattern_amplitude = mx - mn;
            m_data->base_yaw = mean;

            if (m_yaw_variance > 5000.0f) m_data->pattern_type = 1;
            else if (m_yaw_variance > 500.0f && m_data->pattern_amplitude < 180.0f) m_data->pattern_type = 2;
            else if (m_yaw_variance > 100.0f && m_data->pattern_amplitude > 45.0f) m_data->pattern_type = 3;
            else if (m_yaw_variance < 50.0f && m_data->was_moving) m_data->pattern_type = 4;
            else m_data->pattern_type = 0;

            CalculateDesyncRange();
            CalculatePatternFrequency();
        }

        void CalculateDesyncRange() {
            if (m_data->yaw_pattern.count < 12) return;
            std::map<float, int> bins;
            for (size_t i = 0; i < m_data->yaw_pattern.count; i++) {
                float r = std::round(m_data->yaw_pattern[i] / 5.0f) * 5.0f;
                bins[r]++;
            }
            float primary = 0.0f; int mc = 0;
            for (auto& p : bins) if (p.second > mc) { mc = p.second; primary = p.first; }
            float secondary = 0.0f; int sc = 0;
            for (auto& p : bins) if (p.second > sc && std::abs(p.first - primary) > 10.0f) { sc = p.second; secondary = p.first; }
            m_data->desync_range = std::abs(resolver_math::AngleDiff(primary, secondary));
        }

        void CalculatePatternFrequency() {
            if (m_data->yaw_pattern.count < 16) return;
            int best_lag = 1; float max_corr = -1.0f;
            for (int lag = 1; lag <= 8; lag++) {
                float corr = 0.0f; int cnt = 0;
                for (size_t i = lag; i < m_data->yaw_pattern.count; i++) {
                    corr += std::cos(resolver_math::AngleDiff(m_data->yaw_pattern[i], m_data->yaw_pattern[i - lag]) * (3.14159265f / 180.0f));
                    cnt++;
                }
                if (cnt > 0) { corr /= cnt; if (corr > max_corr) { max_corr = corr; best_lag = lag; } }
            }
            m_data->pattern_frequency = (best_lag > 0) ? (1.0f / best_lag) : 0.0f;
        }

        fvector ApplyResolution(fvector head, fvector velocity) {
            switch (m_data->pattern_type) {
            case 1: { // Spin
                float phase = fmod(m_current_time * m_data->pattern_frequency * 360.0f, 360.0f);
                float r = m_data->desync_range * 0.5f;
                head.x += cos(phase * (3.14159265f / 180.0f)) * r;
                head.y += sin(phase * (3.14159265f / 180.0f)) * r;
                return head + velocity * 0.1f;
            }
            case 2: { // Jitter - average valid positions
                if (m_data->position_history.count < 4) return head;
                fvector sum(0, 0, 0); int cnt = 0;
                for (size_t i = 0; i < m_data->position_history.count; i++) {
                    if ((m_data->position_history[i] - head).size() < 50.0f) { sum = sum + m_data->position_history[i]; cnt++; }
                }
                return cnt > 0 ? sum / (float)cnt : head;
            }
            case 3: { // Desync
                float side = 0.0f;
                if (!m_data->velocity_history.empty()) {
                    fvector v = m_data->velocity_history.back();
                    if (v.size() > 5.0f) {
                        fvector right = m_target->GetActorRightVector();
                        side = (v.dot(right) / v.size() > 0.0f) ? 1.0f : -1.0f;
                    }
                }
                fvector right = m_target->GetActorRightVector();
                fvector corrected = head + right * (m_data->desync_range * 0.1f * side);
                if (velocity.size() > 5.0f) corrected = corrected + velocity * 0.05f;
                return corrected;
            }
            case 4: { // Static while moving
                if (m_data->was_moving && !m_data->movement_angles.empty()) {
                    float a = m_data->movement_angles.back() * (3.14159265f / 180.0f);
                    head.x += cos(a) * 5.0f;
                    head.y += sin(a) * 5.0f;
                }
                return head;
            }
            default: return (velocity.size() > 2.0f) ? head + velocity * 0.05f : head;
            }
        }

        void UpdateConfidence() {
            if (m_data->shots_fired > 0) {
                float hr = float(m_data->shots_hit) / float(m_data->shots_fired);
                if (hr > 0.7f) m_data->resolver_confidence = (std::min)(1.0f, m_data->resolver_confidence + 0.05f);
                else if (hr < 0.3f) m_data->resolver_confidence = (std::max)(0.1f, m_data->resolver_confidence - 0.1f);
            }
        }
    };

    class ResolverVisualizer {
    public:
        static float m_pulse_time;
        static float m_rainbow_hue;

        static void DrawModernResolverInfo(ucanvas* canvas, aplayercontroller* controller,
            ashootercharacter* target, UltimateResolverData* rd) {
            if (!canvas || !controller || !target || !rd) return;
            m_pulse_time += 0.02f;
            if (m_pulse_time > 6.28f) m_pulse_time = 0.0f;
            m_rainbow_hue += 0.001f;
            if (m_rainbow_hue > 1.0f) m_rainbow_hue = 0.0f;

            fvector2d orig_screen, resolved_screen;
            bool ov = controller->project_world_location_to_screen(rd->last_original_head_pos, orig_screen, true);
            bool rv = controller->project_world_location_to_screen(rd->last_resolved_head_pos, resolved_screen, true);

            if (ov && rv) {
                flinearcolor cc = LerpColor({1,0,0,1}, {0,1,0,1}, rd->last_confidence);
                if (rd->last_confidence > 0.8f) { cc = HueToRGB(m_rainbow_hue); cc.a = 0.9f; }
                DrawMarker(canvas, orig_screen, {1,1,1,0.8f});
                DrawMarker(canvas, resolved_screen, cc);
            }

            if (ov)
                DrawResolverHUD(canvas, rd, orig_screen);
        }

    private:
        static void DrawMarker(ucanvas* c, const fvector2d& p, const flinearcolor& col) {
            float s = 4.0f;
            c->k2_drawline({p.x-s, p.y}, {p.x+s, p.y}, 1.5f, col);
            c->k2_drawline({p.x, p.y-s}, {p.x, p.y+s}, 1.5f, col);
        }

        static void DrawResolverHUD(ucanvas* canvas, UltimateResolverData* d, const fvector2d& sp) {
            fvector2d hp = {sp.x - 60.0f, sp.y - 155.0f};
            flinearcolor hc = HueToRGB(m_rainbow_hue);

            canvas->k2_drawtext(menu::font, fstring(L"RESOLVER"), {hp.x + 22.0f, hp.y + 5.0f}, fvector2d(0.8f,0.8f), hc, 0, {0,0,0,0}, {0,0}, true, true, true, {0,0,0,0});
            canvas->k2_drawline({hp.x+10, hp.y+22}, {hp.x+110, hp.y+22}, 1.2f, {0.3f,0.3f,0.4f,0.7f});

            DrawBar(canvas, {hp.x+10, hp.y+28}, 100.0f, d->last_confidence, L"CONF");
            float vp = 0.0f;
            if (!d->velocity_history.empty()) vp = resolver_math::r_clamp(d->velocity_history.back().size() / 500.0f, 0.0f, 1.0f);
            DrawBar(canvas, {hp.x+10, hp.y+43}, 100.0f, vp, L"VEL");
            DrawBar(canvas, {hp.x+10, hp.y+58}, 100.0f, d->pattern_frequency, L"PRED");

            wchar_t dt[32]; swprintf_s(dt, L"DESYNC: %d\xB0", (int)d->desync_range);
            canvas->k2_drawtext(menu::font, fstring(dt), {hp.x+10, hp.y+73}, fvector2d(0.5f,0.5f), {0.8f,0.8f,1,1}, 0, {0,0,0,0}, {0,0}, false, true, true, {0,0,0,0});

            const wchar_t* ptn[] = {L"LEGIT", L"SPIN", L"JITTER", L"DESYNC", L"STATIC"};
            int pi = d->last_pattern_type; if (pi < 0 || pi > 4) pi = 0;
            canvas->k2_drawtext(menu::font, fstring(ptn[pi]), {hp.x+70, hp.y+73}, fvector2d(0.5f,0.5f), {1,1,1,1}, 0, {0,0,0,0}, {0,0}, false, true, true, {0,0,0,0});

            wchar_t st[32]; swprintf_s(st, L"SHOTS: %d", d->shots_fired);
            canvas->k2_drawtext(menu::font, fstring(st), {hp.x+10, hp.y+85}, fvector2d(0.45f,0.45f), {0.9f,0.9f,0.9f,0.9f}, 0, {0,0,0,0}, {0,0}, false, true, true, {0,0,0,0});
            if (d->shots_fired > 0) {
                float hr = (float)d->shots_hit / (float)d->shots_fired * 100.0f;
                wchar_t ht[32]; swprintf_s(ht, L"HIT: %d%%", (int)hr);
                canvas->k2_drawtext(menu::font, fstring(ht), {hp.x+70, hp.y+85}, fvector2d(0.45f,0.45f), {0.7f,1,0.7f,0.9f}, 0, {0,0,0,0}, {0,0}, false, true, true, {0,0,0,0});
            }

            const wchar_t* state = d->resolver_confidence > 0.7f ? L"OPTIMAL" : L"LEARNING";
            flinearcolor sc = d->resolver_confidence > 0.7f ? flinearcolor{0.2f,0.8f,0.2f,0.9f} : flinearcolor{0.8f,0.8f,0.2f,0.9f};
            canvas->k2_drawtext(menu::font, fstring(state), {hp.x+10, hp.y+97}, fvector2d(0.45f,0.45f), sc, 0, {0,0,0,0}, {0,0}, false, true, true, {0,0,0,0});
        }

        static void DrawBar(ucanvas* c, const fvector2d& p, float w, float prog, const wchar_t* lbl) {
            c->k2_drawline(p, {p.x+w, p.y}, 4.0f, {0.2f,0.2f,0.2f,0.9f});
            flinearcolor pc;
            if (prog < 0.3f) pc = {1,0,0,0.9f};
            else if (prog < 0.7f) pc = {1,1,0,0.9f};
            else pc = {0,1,0,0.9f};
            c->k2_drawline(p, {p.x + w * prog, p.y}, 4.0f, pc);
            canvas_safe_text(c, lbl, {p.x, p.y - 8.0f}, {1,1,1,1}, 0.5f);
            wchar_t vt[8]; swprintf_s(vt, L"%d%%", int(prog * 100));
            canvas_safe_text(c, vt, {p.x + w + 2, p.y - 8.0f}, {1,1,1,1}, 0.5f);
        }

        static void canvas_safe_text(ucanvas* c, const wchar_t* t, const fvector2d& p, const flinearcolor& col, float s) {
            c->k2_drawtext(menu::font, fstring(t), p, fvector2d(s,s), col, 0, {0,0,0,0}, {0,0}, true, true, true, {0,0,0,0});
        }

        static flinearcolor HueToRGB(float h) {
            float r = std::abs(h * 6.0f - 3.0f) - 1.0f;
            float g = 2.0f - std::abs(h * 6.0f - 2.0f);
            float b = 2.0f - std::abs(h * 6.0f - 4.0f);
            return {resolver_math::r_clamp(r,0,1), resolver_math::r_clamp(g,0,1), resolver_math::r_clamp(b,0,1), 1.0f};
        }

        static flinearcolor LerpColor(const flinearcolor& a, const flinearcolor& b, float t) {
            return {resolver_math::lerp(a.r,b.r,t), resolver_math::lerp(a.g,b.g,t), resolver_math::lerp(a.b,b.b,t), resolver_math::lerp(a.a,b.a,t)};
        }
    };
    float ResolverVisualizer::m_pulse_time = 0.0f;
    float ResolverVisualizer::m_rainbow_hue = 0.0f;

    static std::unordered_map<uintptr_t, std::unique_ptr<UltimateValorantResolver>> resolver_instances;
    static DWORD last_resolver_cleanup = 0;

    // ======================== AUTOWALL (akctwo method - native material energy system) ========================

    static float NativeMaterialMultipliers[40];
    static bool bNativeMaterialsCached = false;

    static void CacheNativeMaterialData() {
        if (bNativeMaterialsCached) return;

        for (int i = 0; i < 40; i++) NativeMaterialMultipliers[i] = 0.5f;

        uintptr_t aresGlobals = ShooterGameBlueprints::GetAresGlobals();
        if (!aresGlobals || !memory::IsValidPointer(aresGlobals)) return;

        uintptr_t wallPenGlobals = *(uintptr_t*)(aresGlobals + 0x820);
        if (!wallPenGlobals || !memory::IsValidPointer(wallPenGlobals)) return;

        for (int i = 0; i < 39; i++) {
            uintptr_t penTypeCDO = *(uintptr_t*)(wallPenGlobals + 0x788 + (i * 8));
            if (!penTypeCDO || !memory::IsValidPointer(penTypeCDO)) continue;

            float energyReduction = *(float*)(penTypeCDO + 0x30);
            if (energyReduction > 0.001f) {
                NativeMaterialMultipliers[i] = energyReduction;
            }
        }
        bNativeMaterialsCached = true;
    }

    class TraceHelper {
    public:
        static inline bool IsValidActorPtr(uintptr_t ptr) {
            return ptr != 0 && ptr != 0xFFFFFFFF && ptr != 0xFFFFFFFFFFFFFFFF && ptr > 0x10000;
        }

        static bool CanPenetrateTrace(
            fvector trace_start, fvector trace_end,
            ashootercharacter* shooter_char, ashootercharacter* target_enemy,
            const WeaponPenetrationData& wpn)
        {
            tarray<AActor*> ignore_actors;
            ignore_actors.Add((AActor*)shooter_char);
            ignore_actors.Add((AActor*)target_enemy);

            FWallSpanList wall_spans;
            memset(&wall_spans, 0, sizeof(FWallSpanList));

            ShooterGameBlueprints::GetWallPenetrationSpans(
                UWorldSave, trace_start, trace_end,
                ignore_actors, ECollisionChannel::ECC_Visibility,
                2.0f, wall_spans
            );

            if (wall_spans.bLastPointInWall) return false;
            if (wall_spans.Spans.count <= 0 || !wall_spans.Spans.data) return true;
            if (!memory::IsValidPointer((uintptr_t)wall_spans.Spans.data)) return false;

            float weapon_energy = wpn.PenetrationPower * 193.0f;
            float remaining_energy = weapon_energy;
            int walls_penetrated = 0;

            constexpr int SPAN_SIZE = 0x1E0;
            uint8_t* span_base = (uint8_t*)wall_spans.Spans.data;

            for (int i = 0; i < wall_spans.Spans.count && i < 8; i++) {
                uint8_t* span = span_base + (i * SPAN_SIZE);

                FHitResult* entrance_hr = (FHitResult*)(span);
                FHitResult* exit_hr = (FHitResult*)(span + 0xF0);

                fvector entrance_loc = entrance_hr->Location;
                fvector exit_loc = exit_hr->Location;

                if (exit_loc.is_null() || !exit_loc.is_valid()) return false;

                double dx = exit_loc.x - entrance_loc.x;
                double dy = exit_loc.y - entrance_loc.y;
                double dz = exit_loc.z - entrance_loc.z;
                float thickness = (float)sqrt(dx * dx + dy * dy + dz * dz);

                if (thickness < 1.0f) continue;

                uint8_t phys_surface = GameplayStatics::GetSurfaceType(*entrance_hr);
                uint8_t ares_surface = ShooterGameBlueprints::ConvertToAresSurfaceType(phys_surface);

                float material_mult = 1.0f;
                if (ares_surface < 40) {
                    material_mult = NativeMaterialMultipliers[ares_surface];
                }

                float energy_lost = thickness * material_mult;

                if (thickness < 15.0f) {
                    energy_lost *= 0.75f;
                }

                remaining_energy -= energy_lost;

                if (remaining_energy <= 0.0f) return false;

                walls_penetrated++;
                if (walls_penetrated >= 4) return false;
            }

            float energy_ratio = remaining_energy / weapon_energy;
            float remaining_damage = wpn.HeadDamage * energy_ratio;

            return remaining_damage > 1.0f;
        }

        static bool CanShootThrough(aplayercontroller* controller, ashootercharacter* shooter_char,
            ashootercharacter* target_enemy, int aim_bone = 8)
        {
            if (!controller || !shooter_char || !target_enemy) return false;
            if (!memory::IsValidPointer((uintptr_t)controller) ||
                !memory::IsValidPointer((uintptr_t)shooter_char) ||
                !memory::IsValidPointer((uintptr_t)target_enemy)) return false;
            if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) return false;

            uskeletalmeshcomponent* enemy_mesh = target_enemy->get_mesh();
            if (!enemy_mesh || !memory::IsValidPointer((uintptr_t)enemy_mesh)) return false;

            fvector trace_start;
            if (globals::misc::tperson) {
                uskeletalmeshcomponent* local_mesh = shooter_char->get_mesh();
                if (local_mesh && memory::IsValidPointer((uintptr_t)local_mesh))
                    trace_start = local_mesh->get_bone_location(8);
            }
            if (!trace_start.is_valid()) {
                aplayercameramanager* cam = controller->get_camera_manager();
                if (cam && memory::IsValidPointer((uintptr_t)cam))
                    trace_start = cam->get_camera_location();
                else return false;
            }

            auto wpn = GetWeaponData(shooter_char);
            if (!wpn.CanPenetrateWalls) return false;

            CacheNativeMaterialData();

            int trace_bone = (aim_bone == 0) ? 8 : aim_bone;
            fvector trace_end = GetBoneMatrix(enemy_mesh, trace_bone);

            if (trace_end.is_valid() && CanPenetrateTrace(trace_start, trace_end, shooter_char, target_enemy, wpn))
                return true;

            if (trace_bone != 8) {
                fvector head_end = GetBoneMatrix(enemy_mesh, 8);
                if (head_end.is_valid() && CanPenetrateTrace(trace_start, head_end, shooter_char, target_enemy, wpn))
                    return true;
            }

            if (trace_bone != 4) {
                fvector chest_end = GetBoneMatrix(enemy_mesh, 4);
                if (chest_end.is_valid() && CanPenetrateTrace(trace_start, chest_end, shooter_char, target_enemy, wpn))
                    return true;
            }

            return false;
        }
    };

    static inline int AutowallComboToUeBone(int combo)
    {
        switch (combo) {
        case 0: return 8;
        case 1: return 6;
        case 2: return 4;
        default: return 8;
        }
    }

    static inline fvector GetAutowallTraceStart(aplayercontroller* controller, ashootercharacter* shooter_char)
    {
        if (!controller || !shooter_char) return fvector();
        if (globals::misc::tperson) {
            uskeletalmeshcomponent* sm = shooter_char->get_mesh();
            if (!sm || !memory::IsValidPointer((uintptr_t)sm)) return fvector();
            fvector h = sm->get_bone_location(8);
            return h.is_valid() ? h : fvector();
        }
        aplayercameramanager* cam = controller->get_camera_manager();
        if (!cam || !memory::IsValidPointer((uintptr_t)cam)) return fvector();
        fvector loc = cam->get_camera_location();
        return loc.is_valid() ? loc : fvector();
    }

    // ===========================================================================
    // == Metrix Targeting & Third-Person Integration (STABLE VERSION)          ==
    // == Ported from Gothic to UE5. Uses spoofcall_stub for crash prevention. ==
    // ===========================================================================
    namespace MetrixTargeting {

        static const int core_bones[] = { 8, 7, 6, 60 };
        static constexpr int core_bone_count = 4;

        __forceinline fvector GetBoneMatrix(void* Mesh, int BoneIndex) {
            if (!Mesh || !memory::IsValidPointer((uintptr_t)Mesh))
                return fvector(0.f, 0.f, 0.f);
            if (BoneIndex < 0)
                return fvector(0.f, 0.f, 0.f);

            FMatrix BoneMatrix;
            reinterpret_cast<FMatrix* (__cdecl*)(void*, FMatrix*, int, uintptr_t, void*)>(spoofcall_stub)(
                Mesh,
                &BoneMatrix,
                BoneIndex,
                0x46C4660,
                (void*)(memory::module_base + offsets::bone_matrix)
            );
            return fvector(BoneMatrix.WPlane.X, BoneMatrix.WPlane.Y, BoneMatrix.WPlane.Z);
        }

        static inline fvector GetAimOrigin(aplayercontroller* controller, ashootercharacter* local_char) {
            if (globals::misc::tperson && local_char && memory::IsValidPointer((uintptr_t)local_char)) {
                uskeletalmeshcomponent* lm = local_char->get_mesh();
                if (lm && memory::IsValidPointer((uintptr_t)lm)) {
                    fvector head = lm->get_bone_location(8);
                    if (head.is_valid()) return head;
                }
            }
            if (controller && memory::IsValidPointer((uintptr_t)controller)) {
                aplayercameramanager* cam = controller->get_camera_manager();
                if (cam && memory::IsValidPointer((uintptr_t)cam)) {
                    fvector loc = cam->get_camera_location();
                    if (loc.is_valid()) return loc;
                }
            }
            return fvector();
        }

        static int GetClosestBoneIndex(uskeletalmeshcomponent* mesh, aplayercontroller* controller) {
            if (!mesh || !controller || !memory::IsValidPointer((uintptr_t)mesh)) return 8;

            fvector2d screen_center = {
                (float)GetSystemMetrics(SM_CXSCREEN) / 2.0f,
                (float)GetSystemMetrics(SM_CYSCREEN) / 2.0f
            };

            int best_bone = 8;
            float closest_distance = FLT_MAX;

            for (int bone : core_bones) {
                fvector bone_world = GetBoneMatrix((void*)mesh, bone);
                if (!bone_world.is_valid()) continue;

                fvector2d bone_screen;
                if (!controller->project_world_location_to_screen(bone_world, bone_screen, false))
                    continue;
                if (!bone_screen.is_valid()) continue;

                float dist = math::distance_2d(bone_screen, screen_center);
                if (dist < closest_distance) {
                    closest_distance = dist;
                    best_bone = bone;
                }
            }
            return best_bone;
        }

        static int GetBestShootableBone(
            aplayercontroller* controller,
            ashootercharacter* shooter_char,
            ashootercharacter* target_enemy)
        {
            if (!target_enemy || !controller || !memory::IsValidPointer((uintptr_t)target_enemy))
                return 8;

            uskeletalmeshcomponent* mesh = target_enemy->get_mesh();
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return 8;

            fvector2d crosshair = {
                (float)GetSystemMetrics(SM_CXSCREEN) / 2.0f,
                (float)GetSystemMetrics(SM_CYSCREEN) / 2.0f
            };

            int best_bone = 8;
            float best_distance = FLT_MAX;
            bool found_any = false;

            for (int bone : core_bones) {
                fvector bone_world = GetBoneMatrix((void*)mesh, bone);
                if (!bone_world.is_valid()) continue;

                fvector2d bone_screen;
                if (!controller->project_world_location_to_screen(bone_world, bone_screen, false))
                    continue;
                if (!bone_screen.is_valid()) continue;

                bool shootable = false;
                if (!globals::aimbot::v1sh_ch3ck) {
                    shootable = true;
                }
                else {
                    bool visible = false;
                    if (globals::aimbot::v1sh_ch3ck_3p && globals::misc::tperson) {
                        uskeletalmeshcomponent* local_mesh = shooter_char ? shooter_char->get_mesh() : nullptr;
                        if (local_mesh && memory::IsValidPointer((uintptr_t)local_mesh)) {
                            fvector head_pos = local_mesh->get_bone_location(8);
                            visible = controller->line_of_sight_from_position(target_enemy, head_pos);
                        }
                    }
                    else {
                        visible = controller->line_of_sight(target_enemy);
                    }

                    if (visible) {
                        shootable = true;
                    }
                    else if (globals::aimbot::auto_wall && shooter_char && memory::IsValidPointer((uintptr_t)shooter_char)) {
                        shootable = TraceHelper::CanShootThrough(controller, shooter_char, target_enemy, bone);
                    }
                }

                if (!shootable) continue;

                float dist = math::distance_2d(bone_screen, crosshair);
                if (dist < best_distance) {
                    best_distance = dist;
                    best_bone = bone;
                    found_any = true;
                }
            }
            return found_any ? best_bone : 8;
        }

    } // namespace MetrixTargeting

    static int ResolveAutowallUeBone(
        aplayercontroller* controller,
        ashootercharacter* shooter_char,
        ashootercharacter* target_enemy,
        bool /*target_visible_for_pick*/ )
    {
        if (!target_enemy || !memory::IsValidPointer((uintptr_t)target_enemy)) return 8;

        if (globals::aimbot::closest_bone) {
            return MetrixTargeting::GetBestShootableBone(controller, shooter_char, target_enemy);
        }

        int combo = globals::aimbot::autowall_b0ne;
        if (combo > 2) combo = 0;
        return AutowallComboToUeBone(combo);
    }

    static inline fvector GetTargetWorldFromAutowallUeBone(uskeletalmeshcomponent* mesh, int ue_bone)
    {
        if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return fvector();
        fvector p = MetrixTargeting::GetBoneMatrix((void*)mesh, ue_bone);
        return p.is_valid() ? p : fvector();
    }

    // ======================== PLAYER TRAIL ========================
    namespace PlayerTrail {
        constexpr int MAX_POINTS = 64;
        constexpr float MIN_DIST_SQ = 12.f * 12.f;

        struct Point {
            fvector world;
            float time;
            bool valid;
        };

        static Point points[MAX_POINTS];
        static int write_idx = 0;
        static fvector last_added{};
        static bool has_last = false;

        static float GetTime() {
            static uint64_t base = GetTickCount64();
            return (GetTickCount64() - base) * 0.001f;
        }

        static float Lifetime() {
            float len = globals::misc::player_trail_length;
            if (!std::isfinite(len)) len = 1.6f;
            if (len < 0.2f) len = 0.2f;
            if (len > 6.0f) len = 6.0f;
            return len;
        }

        static void Reset() {
            for (auto& p : points) p.valid = false;
            has_last = false;
        }

        static void PushPoint(const fvector& pos) {
            if (!pos.is_valid()) return;
            if (has_last) {
                const fvector d = pos - last_added;
                const float dist_sq = (float)(d.x * d.x + d.y * d.y + d.z * d.z);
                if (dist_sq < MIN_DIST_SQ) return;
            }
            points[write_idx].world = pos;
            points[write_idx].time = GetTime();
            points[write_idx].valid = true;
            write_idx = (write_idx + 1) % MAX_POINTS;
            last_added = pos;
            has_last = true;
        }

        static void DrawGlowSegment(ucanvas* canvas, const fvector2d& a, const fvector2d& b, float alpha) {
            if (!canvas || alpha <= 0.01f) return;
            // Fixed thin light-blue trail
            constexpr float size = 0.55f;
            constexpr float r = 0.45f;
            constexpr float g = 0.78f;
            constexpr float bcol = 1.00f;
            canvas->k2_drawline(a, b, size, flinearcolor(r, g, bcol, alpha * 0.90f));
        }

        static void Render(ucanvas* canvas, aplayercontroller* controller) {
            if (!canvas || !controller) return;
            const float now = GetTime();
            const float lifetime = Lifetime();
            fvector2d prev_screen{};
            bool has_prev = false;

            for (int step = 0; step < MAX_POINTS; ++step) {
                const int idx = (write_idx - 1 - step + MAX_POINTS) % MAX_POINTS;
                Point& pt = points[idx];
                if (!pt.valid) continue;

                const float age = now - pt.time;
                if (age >= lifetime || age < 0.0f) {
                    pt.valid = false;
                    continue;
                }

                fvector2d screen{};
                if (!controller->project_world_location_to_screen(pt.world, screen, false) || !screen.is_valid())
                    continue;

                const float t = age / lifetime;
                const float alpha = (1.0f - t) * (1.0f - t);

                if (has_prev)
                    DrawGlowSegment(canvas, prev_screen, screen, alpha);

                has_prev = true;
                prev_screen = screen;
            }
        }
    }

    // ======================== BACKTRACK (enemy tick ghosts) ========================
    // Valorant-safe visual lag-comp: record enemy poses and draw faded skeletons.
    namespace BacktrackVis {
        constexpr int MAX_PLAYERS = 64;
        constexpr int MAX_TICKS = 12;
        constexpr int BONE_COUNT = 14;

        static const int kBoneIds[BONE_COUNT] = { 3, 21, 23, 24, 25, 49, 50, 51, 75, 76, 78, 82, 83, 85 };
        static const int kPairs[][2] = {
            {0,1},{1,2},{2,3},{3,4},{1,5},{5,6},{6,7},
            {0,8},{8,9},{9,10},{0,11},{11,12},{12,13}
        };
        constexpr int PAIR_COUNT = 13;

        struct TickRecord {
            fvector bones[BONE_COUNT];
            fvector origin;
            fvector head;
            float   simtime;
            bool    valid;
        };

        struct PlayerTrack {
            uintptr_t actor_key;
            TickRecord ticks[MAX_TICKS];
            int write_idx;
            int count;
            float last_push;
        };

        static PlayerTrack tracks[MAX_PLAYERS];
        static bool inited;

        static float GetTime() {
            static uint64_t base = GetTickCount64();
            return (GetTickCount64() - base) * 0.001f;
        }

        static void EnsureInit() {
            if (inited) return;
            for (auto& t : tracks) {
                t.actor_key = 0;
                t.write_idx = 0;
                t.count = 0;
                t.last_push = 0.f;
                for (auto& r : t.ticks) r.valid = false;
            }
            inited = true;
        }

        static void Reset() {
            inited = false;
            EnsureInit();
            inited = true;
        }

        static fvector RotateYaw(const fvector& v, float yaw_rad) {
            const float c = cosf(yaw_rad);
            const float s = sinf(yaw_rad);
            return fvector(v.x * c - v.y * s, v.x * s + v.y * c, v.z);
        }

        static PlayerTrack* FindOrAlloc(uintptr_t key) {
            EnsureInit();
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (tracks[i].actor_key == key)
                    return &tracks[i];
            }
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (tracks[i].actor_key == 0) {
                    tracks[i].actor_key = key;
                    tracks[i].write_idx = 0;
                    tracks[i].count = 0;
                    tracks[i].last_push = 0.f;
                    return &tracks[i];
                }
            }
            // reuse oldest slot
            int oldest = 0;
            float oldest_t = 1e9f;
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                if (tracks[i].last_push < oldest_t) {
                    oldest_t = tracks[i].last_push;
                    oldest = i;
                }
            }
            tracks[oldest] = {};
            tracks[oldest].actor_key = key;
            return &tracks[oldest];
        }

        static void PushEnemy(ashootercharacter* actor, uskeletalmeshcomponent* mesh, const fvector& origin) {
            if (!globals::misc::backtrack) return;
            if (!actor || !mesh) return;
            if (!memory::IsValidPointer((uintptr_t)actor) || !memory::IsValidPointer((uintptr_t)mesh)) return;

            const float now = GetTime();
            PlayerTrack* track = FindOrAlloc((uintptr_t)actor);
            if (!track) return;
            if (now - track->last_push < 0.033f) return;
            track->last_push = now;

            TickRecord rec{};
            rec.origin = origin;
            rec.simtime = now;
            rec.valid = true;
            rec.head = origin;
            for (int i = 0; i < BONE_COUNT; ++i) {
                fvector b = mesh->get_bone_location(kBoneIds[i]);
                if (!b.is_valid()) b = origin;
                rec.bones[i] = b;
            }
            // head approx = neck bone index 1 in our remap (bone 21)
            rec.head = rec.bones[1];

            // skip duplicate pose (same origin)
            if (track->count > 0) {
                int prev = (track->write_idx - 1 + MAX_TICKS) % MAX_TICKS;
                if (track->ticks[prev].valid) {
                    fvector d = track->ticks[prev].origin - origin;
                    float dist2 = (float)(d.x * d.x + d.y * d.y + d.z * d.z);
                    if (dist2 < 1.0f) return;
                }
            }

            track->ticks[track->write_idx] = rec;
            track->write_idx = (track->write_idx + 1) % MAX_TICKS;
            if (track->count < MAX_TICKS) track->count++;
        }

        static void Render(ucanvas* canvas, aplayercontroller* controller, const fvector& player_origin) {
            if (!canvas || !controller) return;
            if (!globals::misc::backtrack) return;
            EnsureInit();

            float lifetime = globals::misc::backtrack_length;
            if (!std::isfinite(lifetime)) lifetime = 1.2f;
            if (lifetime < 0.2f) lifetime = 0.2f;
            if (lifetime > 4.0f) lifetime = 4.0f;

            const float now = GetTime();

            for (int p = 0; p < MAX_PLAYERS; ++p) {
                PlayerTrack& track = tracks[p];
                if (track.actor_key == 0 || track.count <= 0) continue;

                for (int i = 0; i < MAX_TICKS; ++i) {
                    TickRecord& rec = track.ticks[i];
                    if (!rec.valid) continue;

                    const float age = now - rec.simtime;
                    if (age >= lifetime || age < 0.f) {
                        rec.valid = false;
                        continue;
                    }

                    const float t = age / lifetime;
                    const float alpha = (1.0f - t) * 0.90f;
                    if (alpha < 0.08f) continue;

                    flinearcolor col(1.0f, 0.70f, 0.20f, alpha);

                    fvector draw_bones[BONE_COUNT];
                    for (int b = 0; b < BONE_COUNT; ++b) {
                        draw_bones[b] = rec.bones[b];
                    }

                    for (int pair = 0; pair < PAIR_COUNT; ++pair) {
                        fvector2d s0, s1;
                        if (!controller->project_world_location_to_screen(draw_bones[kPairs[pair][0]], s0, false) || !s0.is_valid())
                            continue;
                        if (!controller->project_world_location_to_screen(draw_bones[kPairs[pair][1]], s1, false) || !s1.is_valid())
                            continue;
                        canvas->k2_drawline(s0, s1, 1.6f, col);
                    }

                    // head dot
                    fvector2d hs;
                    if (controller->project_world_location_to_screen(draw_bones[1], hs, false) && hs.is_valid()) {
                        flinearcolor dot(1.f, 1.f, 0.4f, alpha);
                        canvas->k2_drawline(fvector2d(hs.x - 2, hs.y), fvector2d(hs.x + 2, hs.y), 2.0f, dot);
                        canvas->k2_drawline(fvector2d(hs.x, hs.y - 2), fvector2d(hs.x, hs.y + 2), 2.0f, dot);
                    }
                }
            }
        }
    }

    // ======================== BULLET TRACE — FiringStateComp polling + particle beam ========================
    namespace BulletBeamTrace {
        constexpr int MAX_TRACERS = 64;
        constexpr float DEFAULT_LIFETIME = 3.5f;
        constexpr float MAX_TRACE_RANGE = 50000.0f;
        constexpr ULONGLONG MIN_SHOT_INTERVAL_MS = 25;

        struct Tracer {
            fvector worldStart;
            fvector worldEnd;
            ULONGLONG timeCreatedMs;
            flinearcolor color;
            bool isActive;
            UParticleSystemComponent* particleComp;
        };

        struct FireWatch {
            currentequippable* weapon = nullptr;
            uintptr_t firing_state = 0;
            int error_retries = 0;
            uint64_t seed_sample = 0;
            bool primed = false;
        };

        static Tracer g_tracers[MAX_TRACERS] = {};
        static int g_currentTracerIndex = 0;
        static ULONGLONG g_lastShotTimeMs = 0;
        static FireWatch g_fireWatch = {};
        static int g_colorCounter = 0;

        static const flinearcolor kColors[8] = {
            flinearcolor(1.0f, 0.0f, 0.0f, 1.0f),
            flinearcolor(0.0f, 1.0f, 0.0f, 1.0f),
            flinearcolor(0.0f, 0.5f, 1.0f, 1.0f),
            flinearcolor(1.0f, 1.0f, 0.0f, 1.0f),
            flinearcolor(1.0f, 0.0f, 1.0f, 1.0f),
            flinearcolor(0.0f, 1.0f, 1.0f, 1.0f),
            flinearcolor(1.0f, 0.5f, 0.0f, 1.0f),
            flinearcolor(0.5f, 0.0f, 1.0f, 1.0f)
        };

        static bool PtrOk(void* p) {
            return p && memory::IsValidPointer((uintptr_t)p) && (uintptr_t)p != (uintptr_t)-1;
        }

        static float Lifetime() {
            float life = globals::misc::pulse_trace_lifetime;
            if (!std::isfinite(life) || life < 0.4f) life = DEFAULT_LIFETIME;
            return life;
        }

        static flinearcolor NextColor() {
            if (globals::misc::pulse_trace_rgb) {
                flinearcolor c = kColors[g_colorCounter % 8];
                g_colorCounter++;
                return c;
            }
            return globals::misc::pulse_trace_color;
        }

        static UParticleSystemComponent* SpawnTracerBeam(const fvector& start, const fvector& end) {
            if (!UWorldSave || !PtrOk(UWorldSave)) return nullptr;

            const wchar_t* path =
                L"/Game/Equippables/Guns/SniperRifles/Boltsniper/Raygun/Chromas/RWB/VFX/"
                L"Trail_TracerSniperBlot_Raygun_VFX_RWB.Trail_TracerSniperBlot_Raygun_VFX_RWB";

            static UParticleSystem* tmpl = nullptr;
            if (!tmpl) {
                tmpl = uobject::find_object<UParticleSystem*>(path);
                if (!tmpl)
                    tmpl = reinterpret_cast<UParticleSystem*>(uobject::static_load_object(nullptr, nullptr, path));
            }
            if (!tmpl) return nullptr;

            static fname tracerAlpha = string::string_to_name(fstring(L"TracerAlpha"));

            UParticleSystemComponent* comp = GameplayStatics::SpawnEmitterAtLocationWithWorld(
                (uobject*)UWorldSave, tmpl, start,
                frotator(0.f, 0.f, 0.f),
                fvector(1.f, 1.f, 1.f),
                true, EPSCPoolMethod::None, true, false, 0.f
            );
            if (!comp) return nullptr;

            comp->SetBeamSourcePoint(0, start, 0);
            comp->SetBeamTargetPoint(0, end, 0);
            comp->SetCustomTimeDilation(0.24f);
            comp->SetFloatParameter(tracerAlpha, 1.0f);

            return comp;
        }

        static void AddTracer(const fvector& start, const fvector& end, const flinearcolor& color) {
            Tracer& t = g_tracers[g_currentTracerIndex];
            if (t.isActive && t.particleComp)
                t.particleComp = nullptr;
            t.worldStart = start;
            t.worldEnd = end;
            t.timeCreatedMs = GetTickCount64();
            t.color = color;
            t.color.a = 1.0f;
            t.isActive = true;
            t.particleComp = SpawnTracerBeam(start, end);
            g_currentTracerIndex = (g_currentTracerIndex + 1) % MAX_TRACERS;
        }

        static bool seh_camera_ray(aplayercameramanager* camera, fvector* outLoc, fvector* outDir) {
            if (!camera || !PtrOk(camera) || !outLoc || !outDir) return false;
            __try {
                fvector loc = camera->get_camera_location();
                fvector rot = camera->get_camera_rotation();
                if (loc.size() < 1.0) return false;

                const double pitch = rot.x * (M_PI / 180.0);
                const double yaw = rot.y * (M_PI / 180.0);
                const double cp = cos(pitch);
                fvector dir(cp * cos(yaw), cp * sin(yaw), sin(pitch));
                dir.normalize();
                if (dir.size() < 0.1) return false;

                *outLoc = loc + (dir * 12.0);
                *outDir = dir;
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }

        static bool seh_PollActualShot(currentequippable* weapon) {
            if (!weapon || !PtrOk(weapon)) {
                g_fireWatch = {};
                return false;
            }

            uintptr_t fs = 0;
            int retries = 0;
            uint64_t seed_sample = 0;
            __try {
                fs = memory::read<uintptr_t>((uintptr_t)weapon + offsets::FiringStateComp);
                if (!fs || !memory::IsValidPointer(fs)) {
                    g_fireWatch = {};
                    return false;
                }
                retries = memory::read<int>(fs + offsets::error_retries);
                uintptr_t seed_ptr = memory::read<uintptr_t>(fs + offsets::seed_data);
                if (seed_ptr && memory::IsValidPointer(seed_ptr))
                    seed_sample = memory::read<uint64_t>(seed_ptr + offsets::seed_dataadd);
                else
                    seed_sample = memory::read<uint64_t>(fs + offsets::seed_data);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                g_fireWatch = {};
                return false;
            }

            if (!g_fireWatch.primed ||
                g_fireWatch.weapon != weapon ||
                g_fireWatch.firing_state != fs)
            {
                g_fireWatch.weapon = weapon;
                g_fireWatch.firing_state = fs;
                g_fireWatch.error_retries = retries;
                g_fireWatch.seed_sample = seed_sample;
                g_fireWatch.primed = true;
                return false;
            }

            bool shot = false;
            if (retries > g_fireWatch.error_retries)
                shot = true;
            else if (seed_sample != g_fireWatch.seed_sample && retries >= g_fireWatch.error_retries)
                shot = true;

            g_fireWatch.error_retries = retries;
            g_fireWatch.seed_sample = seed_sample;
            return shot;
        }

        static void TryAddShotFromCamera(aplayercameramanager* camera, currentequippable* weapon) {
            if (!seh_PollActualShot(weapon))
                return;

            ULONGLONG now = GetTickCount64();
            if (now - g_lastShotTimeMs < MIN_SHOT_INTERVAL_MS)
                return;

            fvector loc{}, dir{};
            if (!seh_camera_ray(camera, &loc, &dir)) return;

            fvector end = loc + (dir * MAX_TRACE_RANGE);

            if (UWorldSave && PtrOk(UWorldSave)) {
                FHitResult hit;
                tarray<AActor*> ignore;
                bool bHit = kismentsystemlibrary::line_trace_single(
                    UWorldSave, loc, end,
                    ETraceTypeQuery::TraceTypeQuery1, true, ignore,
                    EDrawDebugTrace::None, hit, true,
                    flinearcolor(0, 0, 0, 0), flinearcolor(0, 0, 0, 0), 0.0f);
                if (bHit) end = hit.Location;
            }

            AddTracer(loc, end, NextColor());
            g_lastShotTimeMs = now;
        }

        static void seh_TryAddShotFromCamera(aplayercameramanager* camera, currentequippable* weapon) {
            __try { TryAddShotFromCamera(camera, weapon); }
            __except (EXCEPTION_EXECUTE_HANDLER) { }
        }

        void UpdateTracers() {
            const ULONGLONG now = GetTickCount64();
            const float life = Lifetime();

            for (int i = 0; i < MAX_TRACERS; ++i) {
                Tracer& tracer = g_tracers[i];
                if (!tracer.isActive) continue;

                const float age = (float)(now - tracer.timeCreatedMs) * 0.001f;
                if (age < 0.f || age >= life) {
                    tracer.particleComp = nullptr;
                    tracer.isActive = false;
                }
            }
        }

        void ProcessBulletTracers(aplayercameramanager* camera, aplayercontroller* controller,
            ashootercharacter* character, currentequippable* weapon, const std::wstring& weaponName)
        {
            if (!globals::misc::pulse_trace) {
                g_fireWatch = {};
                return;
            }
            if (!controller || !PtrOk(controller)) return;
            if (!camera || !PtrOk(camera)) return;

            bool allow_gun_trace = true;
            if (!weaponName.empty()) {
                if (weaponName.find(L"Melee") != std::wstring::npos ||
                    weaponName.find(L"melee") != std::wstring::npos ||
                    weaponName.find(L"Knife") != std::wstring::npos ||
                    weaponName.find(L"Ability_Melee") != std::wstring::npos)
                    allow_gun_trace = false;
            }

            if (allow_gun_trace && weapon && PtrOk(weapon))
                seh_TryAddShotFromCamera(camera, weapon);
            else
                g_fireWatch = {};

            UpdateTracers();
        }
    }
    // ======================== EMOTE LOAD ========================
    // Cycle: off -> GunKata -> Arcade Victim -> Fortnite Dance -> off
    namespace EmoteLoad {
        static bool s_active = false;
        static bool s_montage_playing = false;
        static float s_montage_replay_at = 0.f;
        static uintptr_t s_last_char = 0;
        static int s_slot = 0; // 0 = GunKata, 1 = Arcade Victim, 2 = Fortnite Dance

        static constexpr wchar_t kGunKata[] =
            L"/Game/Characters/_Core/Emotes/3P/Anims/TP_Core_Emotes_GunKata_Montage.TP_Core_Emotes_GunKata_Montage";
        static constexpr wchar_t kArcadeVictim[] =
            L"/Game/Equippables/Finishers/Arcade/v01/Anims/TP_Core_Arcade_Finisher_TP_Core_Arcade_Finisher_DanceFever_Victim_Montage.TP_Core_Arcade_Finisher_TP_Core_Arcade_Finisher_DanceFever_Victim_Montage";
        static constexpr wchar_t kFortniteDance[] =
            L"/Game/Characters/_Core/Emotes/3P/Anims/TP_Core_Emotes_Floss_Montage.TP_Core_Emotes_Floss_Montage";

        static uobject* resolve_emote_asset(int slot)
        {
            if (slot == 0) {
                uobject* obj = uobject::static_load_object(nullptr, nullptr, kGunKata);
                if (obj && memory::IsValidPointer((uintptr_t)obj))
                    return obj;
                obj = uobject::static_load_object(nullptr, nullptr,
                    L"/Game/Characters/_Core/Emotes/3P/Anims/TP_Core_Emotes_GunKata_Montage.0");
                if (obj && memory::IsValidPointer((uintptr_t)obj))
                    return obj;
                return nullptr;
            }

            if (slot == 1) {
                uobject* obj = uobject::static_load_object(nullptr, nullptr, kArcadeVictim);
                if (obj && memory::IsValidPointer((uintptr_t)obj))
                    return obj;
                return nullptr;
            }

            uobject* obj = uobject::static_load_object(nullptr, nullptr, kFortniteDance);
            if (obj && memory::IsValidPointer((uintptr_t)obj))
                return obj;
            obj = uobject::static_load_object(nullptr, nullptr,
                L"/Game/Characters/_Core/Emotes/3P/Anims/TP_Core_Emotes_Floss_Montage.0");
            if (obj && memory::IsValidPointer((uintptr_t)obj))
                return obj;
            return nullptr;
        }

        static void restore(ashootercharacter* ch)
        {
            if (ch && memory::IsValidPointer((uintptr_t)ch)) {
                if (s_montage_playing) {
                    __try {
                        ch->StopAnimMontage(nullptr);
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        LOG_CRASH("EmoteLoad::restore StopAnimMontage EXCEPTION");
                    }
                    s_montage_playing = false;
                }
                ch->Set3pMeshVisible(globals::misc::tperson);
            }

            s_active = false;
            s_montage_replay_at = 0.f;
            s_last_char = 0;
        }

        static void start_montage(ashootercharacter* ch)
        {
            if (!ch || !memory::IsValidPointer((uintptr_t)ch))
                return;

            __try {
                uobject* montage = resolve_emote_asset(s_slot);
                if (!montage) {
                    s_montage_playing = false;
                    return;
                }

                const float len = ch->PlayAnimMontage(montage, 1.0f);
                if (len > 0.f) {
                    s_montage_playing = true;
                    s_montage_replay_at = (GetTickCount64() / 1000.0f) + len * 0.95f;
                } else {
                    s_montage_playing = false;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                s_montage_playing = false;
                LOG_CRASH("EmoteLoad::start_montage EXCEPTION");
            }
        }

        static bool s_pending_switch = false;

        // 1. basış = GunKata | 2. basış = Arcade | 3. basış = Fortnite Dance | 4. basış = kapat
        static void CycleEmote()
        {
            PakLoader::LoadEmotePak();

            if (!globals::misc::emote_load) {
                s_slot = 0;
                globals::misc::emote_load = true;
                s_pending_switch = true;
                return;
            }

            if (s_slot == 0) {
                s_slot = 1;
                s_pending_switch = true;
                return;
            }

            if (s_slot == 1) {
                s_slot = 2;
                s_pending_switch = true;
                return;
            }

            globals::misc::emote_load = false;
            s_slot = 0;
            s_pending_switch = false;
        }

        static void ToggleEmote() { CycleEmote(); }
    }

    void HandleEmoteLoad(ashootercharacter* character)
    {
        using namespace EmoteLoad;

        if (!globals::misc::emote_load) {
            if (s_active)
                restore(character);
            return;
        }

        if (!character || !memory::IsValidPointer((uintptr_t)character) || !character->is_alive()) {
            if (s_active)
                restore(character);
            return;
        }

        // Slot değişince eski montajı kes, yenisini başlat
        if (s_pending_switch) {
            if (s_montage_playing) {
                __try {
                    character->StopAnimMontage(nullptr);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                }
            }
            s_montage_playing = false;
            s_active = false;
            s_pending_switch = false;
        }

        const uintptr_t char_ptr = (uintptr_t)character;
        if (s_active && char_ptr != s_last_char)
            restore(character);

        const float now = GetTickCount64() / 1000.0f;

        if (!s_active) {
            character->Set3pMeshVisible(true);
            start_montage(character);
            s_active = true;
            s_last_char = char_ptr;
        }

        if (s_montage_playing && now >= s_montage_replay_at)
            start_montage(character);
    }


    static UParticleSystem* g_HellFireParticles[2] = { nullptr, nullptr };
    static bool             g_HellFireParticlesLoaded = false;
    static bool             g_HellFireAttached = false;
    static ashootercharacter* g_LastHellFireCharacter = nullptr;
    static std::map<uintptr_t, UParticleSystemComponent*> g_ActiveHellfireEffects;

    void LoadHellFireParticles()
    {
        if (g_HellFireParticlesLoaded) return;
        g_HellFireParticlesLoaded = true;
        const wchar_t* paths[] = {
            L"/Game/Equippables/Finishers/Hellfire/VFX/Finisher_Hellfire_Debris_ENV_Chroma.Finisher_Hellfire_Debris_ENV_Chroma",
            L"/Game/Equippables/Finishers/Hellfire/VFX/Finisher_Hellfire_Debris_ENV.Finisher_Hellfire_Debris_ENV",
        };
        for (int i = 0; i < 2; i++)
            g_HellFireParticles[i] = reinterpret_cast<UParticleSystem*>(uobject::static_load_object(nullptr, nullptr, paths[i]));
    }

    void ResetHellFire()
    {
        for (auto it = g_ActiveHellfireEffects.begin(); it != g_ActiveHellfireEffects.end(); ++it) {
            if (it->second && memory::IsValidPointer((uintptr_t)it->second) && IsValidUObject((uobject*)it->second)) {
                it->second->DeactivateSystem();
                it->second->K2_DestroyComponent(nullptr);
            }
        }
        g_ActiveHellfireEffects.clear();
        g_HellFireAttached = false;
        g_LastHellFireCharacter = nullptr;
    }

    void AttachHellFireToPlayerFollowing(ashootercharacter* Player)
    {
        if (!Player || !IsValidUObject((uobject*)Player)) return;
        if (!UWorldSave || !IsValidUObject((uobject*)UWorldSave)) return;

        static fvector LastPlayerLocation = fvector(0, 0, 0);
        fvector CurrentLocation = Player->k2_get_actor_location();

        float distance = sqrtf(
            (CurrentLocation.x - LastPlayerLocation.x) * (CurrentLocation.x - LastPlayerLocation.x) +
            (CurrentLocation.y - LastPlayerLocation.y) * (CurrentLocation.y - LastPlayerLocation.y) +
            (CurrentLocation.z - LastPlayerLocation.z) * (CurrentLocation.z - LastPlayerLocation.z));

        if (distance < 1000.0f && g_HellFireAttached) return;

        LoadHellFireParticles();
        if (!g_HellFireParticles[0] && !g_HellFireParticles[1]) return;

        for (int i = 0; i < 3; i++)
        {
            float randomAngle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
            float randomDistance = 600.0f + ((float)rand() / RAND_MAX) * 2000.0f;

            fvector SpawnLoc;
            SpawnLoc.x = CurrentLocation.x + cosf(randomAngle) * randomDistance;
            SpawnLoc.y = CurrentLocation.y + sinf(randomAngle) * randomDistance;
            SpawnLoc.z = CurrentLocation.z + (-50.0f + ((float)rand() / RAND_MAX) * 200.0f);

            UParticleSystem* particle = g_HellFireParticles[rand() % 2];
            if (!particle || !IsValidUObject((uobject*)particle)) continue;

            auto result = GameplayStatics::SpawnEmitterAtLocationWithWorld(
                (uobject*)UWorldSave,
                particle,
                SpawnLoc,
                frotator(0, 0, 0),
                fvector(2.5f, 2.5f, 2.5f),
                true, EPSCPoolMethod::None, true, false, 0.0f
            );

            if (result && IsValidUObject((uobject*)result))
                g_ActiveHellfireEffects[(uintptr_t)result] = result;
        }

        LastPlayerLocation = CurrentLocation;
        g_HellFireAttached = true;
        g_LastHellFireCharacter = Player;
    }

    // ── Lightning (src_remaded) ─────────────────────────────────────
    static UParticleSystem* g_ParticlesNinja[1] = { nullptr };
    static bool g_ParticlesLoaded2 = false;
    static bool g_NinjaAttached = false;
    static std::map<uintptr_t, UParticleSystemComponent*> g_ActiveLightningEffects;

    void SkyLightPaths() {
        if (g_ParticlesLoaded2) return;
        g_ParticlesLoaded2 = true;

        const wchar_t* path = L"/Game/Equippables/Finishers/Ninja/VFX/Particles/P_Ninja_finisher_Lighting_Loop.P_Ninja_finisher_Lighting_Loop";
        g_ParticlesNinja[0] = (UParticleSystem*)uobject::static_load_object(nullptr, nullptr, path);
    }

    void ResetSkyLightFire() {
        for (auto& pair : g_ActiveLightningEffects) {
            if (pair.second && IsValidUObject((uobject*)pair.second)) {
                pair.second->DeactivateSystem();
                pair.second->K2_DestroyComponent(pair.second);
            }
        }
        g_ActiveLightningEffects.clear();
    }

    void AttachNinjaToPlayerFollowing(ashootercharacter* Player) {
        if (!Player) return;

        static fvector LastPlayerLocation = fvector(0, 0, 0);
        fvector CurrentLocation = Player->k2_get_actor_location();

        float distance = sqrtf(
            (float)(pow(CurrentLocation.x - LastPlayerLocation.x, 2) +
            pow(CurrentLocation.y - LastPlayerLocation.y, 2) +
            pow(CurrentLocation.z - LastPlayerLocation.z, 2)));

        if (distance < 1000.0f) return;

        SkyLightPaths();
        if (!g_ParticlesNinja[0]) return;

        float randomAngle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
        float randomDistance = 600.0f + ((float)rand() / RAND_MAX) * 2000.0f;
        float heightOffset = 1000.0f + ((float)rand() / RAND_MAX) * 500.0f;

        fvector SpawnLoc;
        SpawnLoc.x = CurrentLocation.x + cosf(randomAngle) * randomDistance;
        SpawnLoc.y = CurrentLocation.y + sinf(randomAngle) * randomDistance;
        SpawnLoc.z = CurrentLocation.z + heightOffset;

        float scale = 50.0f;

        auto* result = GameplayStatics::SpawnEmitterAtLocationWithWorld(
            (uobject*)UWorldSave,
            g_ParticlesNinja[0],
            SpawnLoc,
            frotator(0, 0, 0),
            fvector(scale, scale, scale),
            true, EPSCPoolMethod::None, true, false, 0.0f);

        if (result) {
            g_ActiveLightningEffects[(uintptr_t)result] = result;
        }

        LastPlayerLocation = CurrentLocation;
    }

    void HandleLightning(ashootercharacter* shooter, uworld* world)
    {
        if (!globals::misc::lightningmode) {
            if (g_NinjaAttached) {
                ResetSkyLightFire();
                g_NinjaAttached = false;
            }
            return;
        }

        if (!world || !shooter) return;
        if (!memory::IsValidPointer((uintptr_t)world) || !memory::IsValidPointer((uintptr_t)shooter)) return;

        static uintptr_t LastWorldPtr = 0;
        static uintptr_t LastCharPtr = 0;

        if ((uintptr_t)world != LastWorldPtr ||
            (uintptr_t)shooter != LastCharPtr)
        {
            ResetSkyLightFire();
            g_ParticlesLoaded2 = false;
            g_ParticlesNinja[0] = nullptr;
            g_NinjaAttached = false;
            LastWorldPtr = (uintptr_t)world;
            LastCharPtr = (uintptr_t)shooter;
        }

        if (!memory::IsValidPointer((uintptr_t)shooter) || !IsValidUObject((uobject*)shooter)) {
            ResetSkyLightFire();
            return;
        }

        AttachNinjaToPlayerFollowing(shooter);
        g_NinjaAttached = true;
    }

    // ── Apocalypse Mode ──────────────────────────────────────────
    static bool g_ApocalypseAttached = false;
    static bool g_ApocalypseLoaded = false;
    static UParticleSystem* g_ApocalypseParticle = nullptr;
    static UParticleSystemComponent* g_ApocalypseParticleComp = nullptr;
    static uobject* g_ApocalypsePPMaterial = nullptr;
    static uobject* g_ApocalypseCollection = nullptr;

    void LoadApocalypseAssets() {
        if (g_ApocalypseLoaded) return;
        g_ApocalypseLoaded = true;

        g_ApocalypseParticle = reinterpret_cast<UParticleSystem*>(
            uobject::static_load_object(nullptr, nullptr,
                L"/Game/Equippables/Finishers/Champions/VFX/Champions_SkySphere_PP_VFX.Champions_SkySphere_PP_VFX")
        );

        g_ApocalypsePPMaterial = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Equippables/Finishers/Champions/VFX/Champions_BrimsExp_PP_MI.Champions_BrimsExp_PP_MI"
        );

        g_ApocalypseCollection = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Equippables/Finishers/Champions/VFX/Champions_Finisher_COL.Champions_Finisher_COL"
        );
    }

    void PPVolume_AddOrUpdateBlendable(uobject* PPVolume, uobject* InBlendableObject, float InWeight) {
        static uobject* fn = uobject::find_object<uobject*>(L"Engine.PostProcessVolume.AddOrUpdateBlendable");
        if (!fn) return;
        struct {
            uobject* ObjectPointer;
            uobject* InterfacePointer;
            float InWeight;
        } params;
        params.ObjectPointer = InBlendableObject;
        params.InterfacePointer = InBlendableObject;
        params.InWeight = InWeight;
        PPVolume->process_event(fn, &params);
    }

    void PPVolume_RemoveBlendable(uobject* PPVolume, uobject* InBlendableObject) {
        static uobject* fn = uobject::find_object<uobject*>(L"Engine.PostProcessVolume.RemoveBlendable");
        if (!fn) return;
        struct {
            uobject* ObjectPointer;
            uobject* InterfacePointer;
        } params;
        params.ObjectPointer = InBlendableObject;
        params.InterfacePointer = InBlendableObject;
        PPVolume->process_event(fn, &params);
    }

    static uobject* g_CachedPPVolume = nullptr;

    uobject* FindPostProcessVolume(uworld* world) {
        if (g_CachedPPVolume && memory::IsValidPointer((uintptr_t)g_CachedPPVolume))
            return g_CachedPPVolume;

        if (!world || !memory::IsValidPointer((uintptr_t)world)) return nullptr;

        tarray<AGameObject*> Objects;
        GameplayStatics::GetAllActorsOfClass2(world, Class::Actors(), &Objects);

        for (int i = 0; i < Objects.size(); i++) {
            AGameObject* Obj = Objects[i];
            if (!Obj || !memory::IsValidPointer((uintptr_t)Obj)) continue;

            fstring name = system::get_object_name((uobject*)Obj);
            if (!name.c_str()) continue;

            if (string_utils::contains(name, L"PostProcessVolume")) {
                g_CachedPPVolume = (uobject*)Obj;
                return g_CachedPPVolume;
            }
        }

        return nullptr;
    }

    void SetCollectionScalarParam(uobject* world, uobject* Collection, const wchar_t* ParamName, float Value) {
        static uobject* fn = nullptr;
        static uobject* lib = nullptr;
        if (!fn) fn = uobject::find_object<uobject*>(L"Engine.KismetMaterialLibrary.SetScalarParameterValue");
        if (!lib) lib = uobject::find_object<uobject*>(L"Default__KismetMaterialLibrary");
        if (!fn || !lib) return;
        struct {
            uobject* WorldContextObject;
            uobject* Collection;
            fname ParameterName;
            float ParameterValue;
        } params;
        params.WorldContextObject = world;
        params.Collection = Collection;
        params.ParameterName = string::string_to_name(fstring(ParamName));
        params.ParameterValue = Value;
        lib->process_event(fn, &params);
    }

    void SetCollectionVectorParam(uobject* world, uobject* Collection, const wchar_t* ParamName, flinearcolor Value) {
        static uobject* fn = nullptr;
        static uobject* lib = nullptr;
        if (!fn) fn = uobject::find_object<uobject*>(L"Engine.KismetMaterialLibrary.SetVectorParameterValue");
        if (!lib) lib = uobject::find_object<uobject*>(L"Default__KismetMaterialLibrary");
        if (!fn || !lib) return;
        struct {
            uobject* WorldContextObject;
            uobject* Collection;
            fname ParameterName;
            flinearcolor ParameterValue;
        } params;
        params.WorldContextObject = world;
        params.Collection = Collection;
        params.ParameterName = string::string_to_name(fstring(ParamName));
        params.ParameterValue = Value;
        lib->process_event(fn, &params);
    }

    void ResetApocalypse() {
        if (g_ApocalypseParticleComp && memory::IsValidPointer((uintptr_t)g_ApocalypseParticleComp)) {
            g_ApocalypseParticleComp->DeactivateSystem();
            g_ApocalypseParticleComp->K2_DestroyComponent(nullptr);
        }
        if (g_CachedPPVolume && g_ApocalypsePPMaterial && memory::IsValidPointer((uintptr_t)g_CachedPPVolume)) {
            PPVolume_RemoveBlendable(g_CachedPPVolume, g_ApocalypsePPMaterial);
        }
        g_ApocalypseAttached = false;
        g_ApocalypseParticleComp = nullptr;
        g_ApocalypseLoaded = false;
        g_ApocalypseParticle = nullptr;
        g_ApocalypsePPMaterial = nullptr;
        g_ApocalypseCollection = nullptr;
        g_CachedPPVolume = nullptr;
    }

    void HandleApocalypseMode(ashootercharacter* shooter, uworld* world) {
        if (!world || !shooter) return;
        if (!memory::IsValidPointer((uintptr_t)world) || !memory::IsValidPointer((uintptr_t)shooter)) return;

        if (!globals::misc::apocalypse_mode) {
            if (g_ApocalypseAttached) {
                ResetApocalypse();
            }
            return;
        }

        if (g_ApocalypseAttached) return;

        // Load all assets via static_load_object (like Lightning/Hellfire)
        LoadApocalypseAssets();

        // Spawn Champions skybox particle
        if (g_ApocalypseParticle && memory::IsValidPointer((uintptr_t)g_ApocalypseParticle)) {
            fvector PlayerPos = shooter->k2_get_actor_location();
            fvector spawnLoc = { PlayerPos.x, PlayerPos.y, PlayerPos.z + 5000.0f };
            g_ApocalypseParticleComp = GameplayStatics::SpawnEmitterAtLocation(
                (uobject*)world, g_ApocalypseParticle,
                spawnLoc,
                FRotator{ 0.f, 0.f, 0.f },
                fvector(500.f, 500.f, 500.f),
                false, EPSCPoolMethod::None, true
            );
        }

        // Find PostProcessVolume and attach PP material
        uobject* PPVolume = FindPostProcessVolume(world);
        if (PPVolume && g_ApocalypsePPMaterial && memory::IsValidPointer((uintptr_t)g_ApocalypsePPMaterial)) {
            memory::write<bool>((uintptr_t)PPVolume + 0x258, true);  // bEnabled
            memory::write<bool>((uintptr_t)PPVolume + 0x259, true);  // bUnbound
            memory::write<float>((uintptr_t)PPVolume + 0x25C, 1.0f); // BlendWeight
            PPVolume_AddOrUpdateBlendable(PPVolume, g_ApocalypsePPMaterial, 1.0f);
        }

        // Setup material parameter collection
        if (g_ApocalypseCollection && memory::IsValidPointer((uintptr_t)g_ApocalypseCollection)) {
            fvector PlayerPos = shooter->k2_get_actor_location();
            SetCollectionVectorParam((uobject*)world, g_ApocalypseCollection, L"Location", { (float)PlayerPos.x, (float)PlayerPos.y, (float)PlayerPos.z, 0.f });
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"Radius", 99999.0f);
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"MainOpacity", 1.f);
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"Global Opacity", 1.0f);
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"R", 1.0f);
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"G", 0.0f);
            SetCollectionScalarParam((uobject*)world, g_ApocalypseCollection, L"B", 0.0f);
        }

        g_ApocalypseAttached = true;
    }

    // ===== ROTATING TEXT =====
    constexpr int AURA_MAX = 8;
    static UProceduralMeshComponent* AuraMeshes[AURA_MAX] = {};
    static UMaterialInstanceDynamic* AuraDynMats[AURA_MAX] = {};
    static int       AuraSpawnedCount = 0;
    static uintptr_t AuraLastCharPtr = 0;
    static float     aura_angle = 0.0f;
    static float     aura_last_time = 0.0f;

    tarray<fvector> CurveVertices(const tarray<fvector>& src)
    {
        tarray<fvector> out;
        if (src.Num() == 0) return out;

        constexpr float cdeg = 110.0f;

        if (fabsf(cdeg) < 1.f) {
            for (int i = 0; i < src.Num(); i++) out.add(src[i]);
            return out;
        }

        float Xmin = (float)src[0].x, Xmax = (float)src[0].x;
        for (int i = 1; i < src.Num(); i++) {
            float x = (float)src[i].x;
            if (x < Xmin) Xmin = x;
            if (x > Xmax) Xmax = x;
        }
        float Xc = (Xmin + Xmax) * 0.5f;
        float halfW = (Xmax - Xmin) * 0.5f;
        if (halfW < 0.001f) {
            for (int i = 0; i < src.Num(); i++) out.add(src[i]);
            return out;
        }

        float half_arc = cdeg * (3.14159265f / 180.f) * 0.5f;
        float R = halfW / sinf(half_arc);

        for (int i = 0; i < src.Num(); i++) {
            fvector v = src[i];
            float t = ((float)v.x - Xc) / halfW;
            float ang = t * half_arc;
            v.x = (double)(Xc + R * sinf(ang));
            v.z += (double)(R * (cosf(ang) - 1.f));
            out.add(v);
        }
        return out;
    }

    void AuraSpawnOneMesh(ashootercharacter* Character, int idx,
        const MeshData& mesh, uobject* GlowMat,
        uobject* AddComponentFunc, uobject* CreateMeshFunc,
        USceneComponent* rootComp)
    {
        if (idx < 0 || idx >= AURA_MAX) return;
        AuraMeshes[idx] = nullptr; AuraDynMats[idx] = nullptr;

        struct { AActor* Actor; UActorComponent* ComponentClass; UActorComponent* ReturnValue; }
        AP{ (AActor*)Character,
            (UActorComponent*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent"),
            nullptr };
        variables.blueprints->process_event(AddComponentFunc, &AP);

        auto* PM = (uskeletalmeshcomponent*)AP.ReturnValue;
        if (!PM || !memory::IsValidPointer((uintptr_t)PM)) return;

        tarray<fvector> curvedVerts = CurveVertices(mesh.Vertices);
        struct {
            int32_t SectionIndex; tarray<fvector> Vertices; tarray<int32_t> Triangles;
            tarray<fvector> Normals; tarray<fvector2d> UV0; tarray<FColor> VertexColors;
            tarray<FProcMeshTangent> Tangents; bool bCreateCollision;
        } CP = { 0, curvedVerts, mesh.Triangles, mesh.Normals, mesh.UV0,
                  mesh.VertexColors, mesh.Tangents, false };
        ((uobject*)PM)->process_event(CreateMeshFunc, &CP);

        if (GlowMat && memory::IsValidPointer((uintptr_t)GlowMat)) {
            uobject* DR = seh_create_dynamic_mat((uintptr_t)PM, 0, GlowMat);
            if (DR && memory::IsValidPointer((uintptr_t)DR)) {
                AuraDynMats[idx] = (UMaterialInstanceDynamic*)DR;
                flinearcolor col(1.f, 1.f, 1.f, 1.f);
                AuraDynMats[idx]->set_vector_parameter_value(string::string_to_name(L"Base Color"), col);
                AuraDynMats[idx]->set_vector_parameter_value(string::string_to_name(L"Emissive Color"), col);
                AuraDynMats[idx]->set_scalar_parameter_value(string::string_to_name(L"EmissiveIntensity"), 12.0);
            }
        }

        if (rootComp && memory::IsValidPointer((uintptr_t)rootComp))
            USceneComponentHelpers::AttachTo(PM, (uskeletalmeshcomponent*)rootComp,
                string::string_to_name(L""), 2, 2, 2, false);

        SetComponentVisibility((USceneComponent*)PM, true, true);

        seh_set_relative_scale_3d((uintptr_t)PM, fvector(
            globals::misc::aura_scale_x, globals::misc::aura_scale_y, globals::misc::aura_scale_z));

        AuraMeshes[idx] = (UProceduralMeshComponent*)PM;
    }

    void SpawnAuraMesh(ashootercharacter* Character) {
        if (!Character || !memory::IsValidPointer((uintptr_t)Character)) return;
        if (!UWorldSave || !memory::IsValidPointer((uintptr_t)UWorldSave)) return;

        std::string auraPath = GetPublicPath() + "aura.obj";
        if (GetFileAttributesA(auraPath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

        static uobject* ProcMeshClass = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent");
        static uobject* AddComponentFunc = (uobject*)uobject::find_object<uclass*>(L"ShooterGame.ShooterBlueprintLibrary.AddComponentByClass");
        static uobject* CreateMeshFunc = (uobject*)uobject::find_object<uclass*>(L"ProceduralMeshComponent.ProceduralMeshComponent.CreateMeshSection");
        if (!ProcMeshClass || !AddComponentFunc || !CreateMeshFunc) return;

        for (int i = 0; i < AuraSpawnedCount; i++) {
            if (AuraMeshes[i] && memory::IsValidPointer((uintptr_t)AuraMeshes[i]))
                SetComponentVisibility((USceneComponent*)AuraMeshes[i], false, true);
            AuraMeshes[i] = nullptr; AuraDynMats[i] = nullptr;
        }
        AuraSpawnedCount = 0;

        const MeshData& mesh = ParseOBJFile(auraPath.c_str());
        if (mesh.Vertices.Num() == 0) return;

        uobject* GlowMat = uobject::static_load_object(nullptr, nullptr,
            L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI");
        USceneComponent* rootComp = memory::read<USceneComponent*>((uintptr_t)Character + offsets::Rootcomponent);

        int n = globals::misc::aura_copies;
        if (n < 1) n = 1;
        if (n > AURA_MAX) n = AURA_MAX;

        for (int i = 0; i < n; i++) {
            AuraSpawnOneMesh(Character, i, mesh, GlowMat, AddComponentFunc, CreateMeshFunc, rootComp);
            if (AuraMeshes[i]) AuraSpawnedCount++;
        }
    }

    void TickAuraMesh() {
        if (AuraSpawnedCount == 0) return;

        float now = GetTickCount64() / 1000.0f;
        float dt = now - aura_last_time;
        aura_last_time = now;
        if (dt > 0.1f) dt = 0.1f;

        aura_angle -= globals::misc::aura_speed * dt * 2.0f * 3.14159265f;
        if (aura_angle > 2.0f * 3.14159265f) aura_angle -= 2.0f * 3.14159265f;

        int n = AuraSpawnedCount;
        float step = (n > 1) ? (2.0f * 3.14159265f / (float)n) : 0.0f;

        for (int i = 0; i < n; i++) {
            auto* m = AuraMeshes[i];
            if (!m || !memory::IsValidPointer((uintptr_t)m)) continue;

            float a = aura_angle + (float)i * step;

            USceneComponentHelpers::SetRelativeLocation((uskeletalmeshcomponent*)m,
                fvector(globals::misc::aura_radius * cosf(a),
                        globals::misc::aura_radius * sinf(a),
                        globals::misc::aura_height));

            USceneComponentHelpers::SetRelativeRotation((uskeletalmeshcomponent*)m,
                FRotator{ 0.0f, a * (180.f / 3.14159265f) + globals::misc::aura_yaw_offset, globals::misc::aura_roll });

            seh_set_relative_scale_3d((uintptr_t)m, fvector(
                globals::misc::aura_scale_x, globals::misc::aura_scale_y, globals::misc::aura_scale_z));

            if (AuraDynMats[i] && memory::IsValidPointer((uintptr_t)AuraDynMats[i])) {
                float hue = fmodf(now * 2.0f + (float)i / (float)n, 1.0f);
                float r, g, b;
                float h6 = hue * 6.0f;
                int hi = (int)h6;
                float f = h6 - hi;
                switch (hi % 6) {
                case 0: r = 1; g = f; b = 0; break;
                case 1: r = 1 - f; g = 1; b = 0; break;
                case 2: r = 0; g = 1; b = f; break;
                case 3: r = 0; g = 1 - f; b = 1; break;
                case 4: r = f; g = 0; b = 1; break;
                case 5: r = 1; g = 0; b = 1 - f; break;
                default: r = g = b = 1; break;
                }
                flinearcolor col(r, g, b, 1.f);
                AuraDynMats[i]->set_vector_parameter_value(string::string_to_name(L"Emissive Color"), col);
                AuraDynMats[i]->set_scalar_parameter_value(string::string_to_name(L"EmissiveIntensity"), (double)globals::misc::aura_emissive);
            }
        }
    }

    void HandleAuraOrbit(ashootercharacter* Character) {
        if (!globals::misc::aura_orbit) {
            if (AuraSpawnedCount > 0) {
                for (int i = 0; i < AuraSpawnedCount; i++) {
                    if (AuraMeshes[i] && memory::IsValidPointer((uintptr_t)AuraMeshes[i]))
                        SetComponentVisibility((USceneComponent*)AuraMeshes[i], false, true);
                    AuraMeshes[i] = nullptr; AuraDynMats[i] = nullptr;
                }
                AuraSpawnedCount = 0;
                AuraLastCharPtr = 0;
            }
            return;
        }

        if (!Character || !memory::IsValidPointer((uintptr_t)Character)) return;

        uintptr_t charPtr = (uintptr_t)Character;
        int desired = globals::misc::aura_copies;
        if (desired < 1) desired = 1;
        if (desired > AURA_MAX) desired = AURA_MAX;
        if (charPtr != AuraLastCharPtr || AuraSpawnedCount == 0 || AuraSpawnedCount != desired) {
            SpawnAuraMesh(Character);
            AuraLastCharPtr = charPtr;
        }

        TickAuraMesh();
    }

    namespace NoSmoke
    {
        static std::vector<AGameObject*> cachedSmokes;
        static double lastSmokeUpdate = 0.0;
        static uintptr_t lastSmokeWorldPtr = 0;

        static const std::vector<std::string> smoke_identifiers = {
            "SmokeGrenade", "ZedAbility_Smoke", "GrenadeSmoke",
            "SmokeScreen", "AbilitySmoke", "Brimstone_Smoke",
            "Viper_Smoke", "Omen_Smoke", "Harbor_Smoke"
        };

        void RemoveSmokes(uworld* world)
        {
            if (!globals::misc::no_smoke) {
                if (!cachedSmokes.empty()) cachedSmokes.clear();
                return;
            }
            if (!world || !memory::IsValidPointer((uintptr_t)world)) return;

            if ((uintptr_t)world != lastSmokeWorldPtr) {
                cachedSmokes.clear();
                lastSmokeUpdate = 0.0;
                lastSmokeWorldPtr = (uintptr_t)world;
            }

            double currentTime = GameplayStatics::GetTimeSeconds((uobject*)world);
            if (currentTime < 0.0 || currentTime > 1000000.0) return;

            if (currentTime - lastSmokeUpdate > 0.5)
            {
                cachedSmokes.clear();

                tarray<AGameObject*> Objects;
                GameplayStatics::GetAllActorsOfClass2(world, Class::Actors(), &Objects);

                int32_t objectCount = Objects.Num();
                if (objectCount <= 0 || objectCount > 10000) return;

                for (int32_t index = 0; index < objectCount; index++)
                {
                    if (!Objects.IsValidIndex(index)) continue;

                    AGameObject* Object = Objects[index];
                    if (!Object || !memory::IsValidPointer((uintptr_t)Object)) continue;

                    std::string ObjectName = system::get_object_name((uobject*)Object).to_str();
                    if (ObjectName.empty()) continue;

                    char firstChar = ObjectName[0];
                    if (firstChar != 'S' && firstChar != 'Z' && firstChar != 'G' &&
                        firstChar != 'B' && firstChar != 'V' && firstChar != 'O' && firstChar != 'H') continue;

                    for (const auto& identifier : smoke_identifiers) {
                        if (ObjectName.find(identifier) != std::string::npos) {
                            cachedSmokes.push_back(Object);
                            break;
                        }
                    }

                    if (cachedSmokes.size() >= 50) break;
                }

                lastSmokeUpdate = currentTime;
            }

            for (auto it = cachedSmokes.begin(); it != cachedSmokes.end();)
            {
                AGameObject* smoke = *it;
                if (!smoke || !memory::IsValidPointer((uintptr_t)smoke)) {
                    it = cachedSmokes.erase(it);
                    continue;
                }
                smoke->SetActorHiddenInGame(true);
                smoke->SetActorEnableCollision(false);
                ++it;
            }
        }
    }


    static AGameObject* CachedSkyDome = nullptr;
    static bool SkyDomeCached = false;
    static AGameObject* SkyDome = nullptr;

    static int seh_sky_num_materials(UPrimitiveComponent* mesh) {
        if (!mesh) return 0;
        __try { return mesh->get_num_materials(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
    }

    static void SkyBoxMeshImpl()
    {
        if (!SkyDome || !IsValidUObject((uobject*)SkyDome)) return;

        UPrimitiveComponent* mesh = memory::read<UPrimitiveComponent*>((uintptr_t)SkyDome + offsets::skymeshcomponent);
        if (!mesh || !IsValidUObject((uobject*)mesh)) return;

        // Cache original material per-mesh; reset when mesh pointer changes (e.g. round end actor reload).
        static uobject* originalMaterial = nullptr;
        static UPrimitiveComponent* originalMaterialOwner = nullptr;
        if (originalMaterialOwner != mesh) {
            originalMaterial = nullptr;
            originalMaterialOwner = mesh;
        }
        if (!originalMaterial || !IsValidUObject(originalMaterial)) {
            originalMaterial = mesh->get_material(0);
            if (!originalMaterial) return;
        }

        if (!globals::misc::skybox && !globals::misc::nightmode) {
            if (originalMaterial && IsValidUObject(originalMaterial) && mesh->get_material(0) != originalMaterial) {
                seh_set_material((uintptr_t)mesh, 0, originalMaterial);
            }
            return;
        }

        fname first_name = string::string_to_name(L"Horizon color");
        fname second_name = string::string_to_name(L"Zenith Color");
        fname third_name = string::string_to_name(L"Overall Color");
        fname cloud_color = string::string_to_name(L"Cloud Color");
        fname cloud_speed = string::string_to_name(L"Cloud Speed");
        fname Stars_Brightness = string::string_to_name(L"Stars Brightness");
        fname cloud_op = string::string_to_name(L"Cloud Opacity");
        fname noise_power2 = string::string_to_name(L"NoisePower2");
        fname noise_power1 = string::string_to_name(L"NoisePower1");
        fname sun_radius = string::string_to_name(L"Sun Radius");
        fname horizon_falloff = string::string_to_name(L"Horizon Falloff");
        fname sun_brightness = string::string_to_name(L"Sun Brightness");
        fname sun_height = string::string_to_name(L"Sun Height");
        fname sun_color = string::string_to_name(L"Sun Color");

        auto matPath = L"/Engine/EngineSky/M_Sky_Panning_Clouds2.M_Sky_Panning_Clouds2";

        uobject* material = uobject::find_object<uobject*>(matPath);
        if (!material)
            uobject::static_load_object(nullptr, nullptr, matPath);
        material = uobject::find_object<uobject*>(matPath);
        if (!material || !IsValidUObject(material)) return;

        seh_set_material((uintptr_t)mesh, 0, material);
        uobject* dynMat = seh_create_dynamic_mat((uintptr_t)mesh, 0, material);

        if (dynMat && IsValidUObject(dynMat))
        {
            auto mat = dynMat->cast<UMaterialInstanceDynamic>();
            if (mat && IsValidUObject((uobject*)mat))
            {
                if (globals::misc::nightmode) {
                    mat->set_vector_parameter_value1(first_name, { 0.0f, 0.0f, 0.0f });
                    mat->set_vector_parameter_value1(second_name, { 0.0f, 0.0f, 0.0f });
                    mat->set_vector_parameter_value1(third_name, { 0.0f, 0.0f, 0.0f });
                    mat->set_vector_parameter_value1(cloud_color, { 0.0f, 0.0f, 0.0f });
                    mat->set_vector_parameter_value1(sun_color, { 0.0f, 0.0f, 0.0f });
                    mat->set_scalar_parameter_value1(cloud_speed, 0.0f);
                    mat->set_scalar_parameter_value1(Stars_Brightness, 0.0f);
                    mat->set_scalar_parameter_value1(cloud_op, 0.0f);
                    mat->set_scalar_parameter_value1(sun_brightness, 0.0f);
                    mat->set_scalar_parameter_value1(sun_height, 0.0f);
                    mat->set_scalar_parameter_value1(noise_power1, 0.0f);
                    mat->set_scalar_parameter_value1(noise_power2, 0.0f);
                    mat->set_scalar_parameter_value1(sun_radius, 0.0f);
                    mat->set_scalar_parameter_value1(horizon_falloff, 0.0f);
                }
                else {
                    struct SkyPreset {
                        flinearcolor horizon, zenith, overall, cloud, sun;
                        float cloud_speed, stars, cloud_op, noise2, noise1, sun_rad, hor_fall, sun_bright, sun_h;
                    };

                    SkyPreset p{};
                    switch (globals::misc::skybox_preset) {
                    default:
                    case 0: // Neon Aurora
                        p = { {0.15f,2.5f,3.2f}, {1.8f,0.2f,3.5f}, {0.05f,1.8f,2.8f}, {0.3f,2.2f,3.0f}, {1.5f,0.4f,3.5f},
                              0.55f, 10.0f, 15.0f, 4.0f, 3.2f, 1.8f, 0.55f, 22.0f, 0.65f };
                        break;
                    case 1: // Solar Flare
                        p = { {2.8f,0.55f,0.05f}, {3.5f,1.2f,0.15f}, {2.2f,0.35f,0.02f}, {3.0f,0.9f,0.1f}, {4.0f,1.5f,0.2f},
                              0.35f, 6.0f, 12.0f, 3.5f, 2.8f, 2.5f, 0.4f, 18.0f, 0.85f };
                        break;
                    case 2: // Purple Haze
                        p = { {1.6f,0.15f,2.8f}, {2.4f,0.05f,3.5f}, {1.1f,0.08f,2.2f}, {1.8f,0.2f,3.0f}, {2.6f,0.3f,3.8f},
                              0.42f, 9.0f, 14.0f, 3.8f, 3.0f, 2.0f, 0.5f, 20.0f, 0.7f };
                        break;
                    case 3: // Ocean Blue
                        p = { {0.05f,0.8f,2.8f}, {0.1f,1.4f,3.6f}, {0.02f,0.5f,2.2f}, {0.15f,1.1f,3.2f}, {0.2f,1.8f,4.0f},
                              0.48f, 8.5f, 13.5f, 3.6f, 2.9f, 1.9f, 0.52f, 19.0f, 0.68f };
                        break;
                    case 4: // RGB — colors applied below
                        p = { {1.0f,1.0f,1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f,1.0f}, {1.0f,1.0f,1.0f},
                              0.55f, 10.0f, 15.0f, 4.0f, 3.2f, 1.8f, 0.55f, 22.0f, 0.65f };
                        break;
                    case 5: // Emerald Glow
                        p = { {0.05f,2.2f,0.6f}, {0.1f,3.0f,0.9f}, {0.02f,1.6f,0.4f}, {0.12f,2.6f,0.75f}, {0.2f,3.4f,1.0f},
                              0.4f, 8.0f, 13.0f, 3.4f, 2.7f, 1.7f, 0.48f, 17.0f, 0.72f };
                        break;
                    case 6: // Crimson Sky
                        p = { {2.6f,0.08f,0.12f}, {3.2f,0.05f,0.2f}, {2.0f,0.03f,0.08f}, {2.8f,0.15f,0.18f}, {3.6f,0.25f,0.3f},
                              0.38f, 7.5f, 12.5f, 3.3f, 2.6f, 2.2f, 0.45f, 16.0f, 0.78f };
                        break;
                    case 7: // Rose Pink
                        p = { {2.4f,0.35f,1.4f}, {3.0f,0.2f,2.0f}, {1.8f,0.15f,1.0f}, {2.6f,0.4f,1.6f}, {3.2f,0.55f,2.2f},
                              0.45f, 8.0f, 13.0f, 3.5f, 2.8f, 1.9f, 0.5f, 18.0f, 0.74f };
                        break;
                    case 8: // Midnight Void
                        p = { {0.15f,0.05f,0.45f}, {0.25f,0.02f,0.65f}, {0.08f,0.02f,0.35f}, {0.2f,0.08f,0.55f}, {0.35f,0.12f,0.75f},
                              0.3f, 12.0f, 16.0f, 4.2f, 3.4f, 1.5f, 0.6f, 24.0f, 0.55f };
                        break;
                    }

                    mat->set_vector_parameter_value1(first_name, p.horizon);
                    mat->set_vector_parameter_value1(second_name, p.zenith);
                    mat->set_vector_parameter_value1(third_name, p.overall);
                    mat->set_vector_parameter_value1(cloud_color, p.cloud);
                    mat->set_vector_parameter_value1(sun_color, p.sun);
                    mat->set_scalar_parameter_value1(cloud_speed, p.cloud_speed);
                    mat->set_scalar_parameter_value1(Stars_Brightness, p.stars);
                    mat->set_scalar_parameter_value1(cloud_op, p.cloud_op);
                    mat->set_scalar_parameter_value1(noise_power2, p.noise2);
                    mat->set_scalar_parameter_value1(noise_power1, p.noise1);
                    mat->set_scalar_parameter_value1(sun_radius, p.sun_rad);
                    mat->set_scalar_parameter_value1(horizon_falloff, p.hor_fall);
                    mat->set_scalar_parameter_value1(sun_brightness, p.sun_bright);
                    mat->set_scalar_parameter_value1(sun_height, p.sun_h);
                }
            }
        }

        if ((globals::misc::skybox_preset == 4 || globals::misc::skyboxrgb) && !globals::misc::nightmode)
        {
            static float rainbowTime = 0.0f;
            rainbowTime += 0.006f;
            flinearcolor rainbow = GetRainbowColor(rainbowTime);

            int num_materials = seh_sky_num_materials(mesh);
            if (num_materials <= 0) return;
            if (num_materials > 8) num_materials = 8;
            for (int i = 0; i < num_materials; i++)
            {
                uobject* mid = seh_create_dynamic_mat((uintptr_t)mesh, i, material);
                if (!mid || !IsValidUObject(mid)) continue;

                auto mat = mid->cast<UMaterialInstanceDynamic>();
                if (!mat || !IsValidUObject((uobject*)mat)) continue;

                mat->set_vector_parameter_value1(first_name, { rainbow.r * 3.0f, rainbow.g * 3.0f, rainbow.b * 3.0f });
                mat->set_vector_parameter_value1(second_name, { rainbow.r * 3.5f, rainbow.g * 3.5f, rainbow.b * 3.5f });
                mat->set_vector_parameter_value1(third_name, { rainbow.r * 2.5f, rainbow.g * 2.5f, rainbow.b * 2.5f });
                mat->set_vector_parameter_value1(cloud_color, { rainbow.r * 3.0f, rainbow.g * 3.0f, rainbow.b * 3.0f });
            }
        }
    }

    void SkyBoxMesh()
    {
        __try {
            SkyBoxMeshImpl();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("SkyBoxMesh EXCEPTION");
            SkyDomeCached = false;
            CachedSkyDome = nullptr;
            SkyDome = nullptr;
        }
    }




    // ── Cool Wireframe ─────────────────────────────────────────────────────
    // Translucent weapon mat + wireframe bit (ported from working base).
    static void seh_crystal_wireframe(uintptr_t ptr) {
        if (!ptr || !memory::IsValidPointerFast(ptr)) return;
        __try {
            *(char*)(ptr + offsets::WireFrame) = *(char*)(ptr + offsets::WireFrame) | (1 << 5);
            *(char*)(ptr + offsets::WireFrame2) = (char)offsets::WireFrame3;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static uobject* seh_static_load_mw(const wchar_t* path) {
        __try {
            uobject* m = uobject::static_load_object(nullptr, nullptr, path);
            if (m && memory::IsValidPointer((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        return nullptr;
    }

    static void moscawireframe_impl(ashootercharacter* local_player);
    void moscawireframe(ashootercharacter* local_player)
    {
        if (!globals::misc::MoscaWireframe) return;
        __try { moscawireframe_impl(local_player); } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    static void moscawireframe_impl(ashootercharacter* local_player)
    {
        if (!local_player || !local_player->is_alive()) return;

        static uobject* wire_mat = nullptr;
        if (!wire_mat || !memory::IsValidPointer((uintptr_t)wire_mat)) {
            wire_mat = seh_static_load_mw(L"/Game/Equippables/_Core/Materials/3P_Weapon_Translucent_Mat.3P_Weapon_Translucent_Mat");
        }
        if (!wire_mat || !memory::IsValidPointer((uintptr_t)wire_mat)) return;

        static fname n_basecolor  = string::string_to_name(L"Base Color");
        static fname n_emissive   = string::string_to_name(L"Emissive Color");
        static fname n_center     = string::string_to_name(L"CenterEdgeColor");
        static fname n_inner      = string::string_to_name(L"InnerEdgeColor");
        static fname n_outer      = string::string_to_name(L"OuterEdgeColor");
        static fname n_silo       = string::string_to_name(L"SilohuetteColor");
        static fname n_glow       = string::string_to_name(L"GlowIntensity");

        float R = globals::misc::MoscaWireColor[0];
        float G = globals::misc::MoscaWireColor[1];
        float B = globals::misc::MoscaWireColor[2];
        float glow = globals::misc::MoscaGlow;

        flinearcolor base_col(R * glow, G * glow, B * glow, 1.0f);
        flinearcolor emissive_col(R * glow * 3.0f, G * glow * 3.0f, B * glow * 3.0f, 1.0f);

        auto apply_on_mesh = [&](uskeletalmeshcomponent* mesh) {
            if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) return;
            auto num_materials = mesh->get_num_materials();
            if (num_materials <= 0 || num_materials > 64) return;
            for (int i = 0; i < num_materials; i++) {
                auto mid_obj = seh_create_dynamic_mat((uintptr_t)mesh, i, wire_mat);
                if (!mid_obj || !memory::IsValidPointer((uintptr_t)mid_obj)) continue;
                auto dynCast = mid_obj->cast<UMaterialInstanceDynamic>();
                if (!dynCast || !memory::IsValidPointer((uintptr_t)dynCast)) continue;
                seh_set_vector_param(dynCast, n_basecolor, base_col);
                seh_set_vector_param(dynCast, n_emissive, emissive_col);
                seh_set_vector_param(dynCast, n_center, base_col);
                seh_set_vector_param(dynCast, n_inner, base_col);
                seh_set_vector_param(dynCast, n_outer, emissive_col);
                seh_set_vector_param(dynCast, n_silo, base_col);
                seh_set_scalar_param(dynCast, n_glow, (double)glow);
            }
            seh_crystal_wireframe((uintptr_t)mesh);
        };

        auto mesh1p = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + offsets::mesh1p);
        auto meshOverlay = memory::read<uskeletalmeshcomponent*>((uintptr_t)local_player + offsets::mesh1p_overlay);
        if (mesh1p) apply_on_mesh(mesh1p);
        if (meshOverlay) apply_on_mesh(meshOverlay);

        auto mesh3p = local_player->get_mesh();
        auto cosmetic3p = local_player->GetCosmeticMesh3P();
        if (mesh3p) apply_on_mesh(mesh3p);
        if (cosmetic3p) apply_on_mesh(cosmetic3p);
    }

    static float smooth_fps = 60.0f;

    // Returns: 0 = failed/invalid, otherwise (before_byte | (after_byte << 8) | 0x10000)
    // NOTE: Do NOT write WireFrame2 (0xc0)=0xff — that offset sits on component/object
    // fields and corrupts pointers (crash signature: read 0x00007f00...).
    static int seh_wireframe_write(uintptr_t ptr, bool enable) {
        __try {
            volatile unsigned char* flag = (volatile unsigned char*)(ptr + offsets::WireFrame);
            unsigned char before = *flag;
            if (enable) {
                *flag = (unsigned char)(before | (1 << 5));
            } else {
                *flag = (unsigned char)(before & ~(1 << 5));
            }
            unsigned char after = *flag;
            return 0x10000 | before | (after << 8);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static void seh_set_wireframe(uintptr_t ptr, bool enable) {
        if (!ptr || !memory::IsValidPointerFast(ptr)) return;

        int r = seh_wireframe_write(ptr, enable);

        if (r == 0) {
            static int fail_logged = 0;
            if (fail_logged < 5) {
                fail_logged++;
                LOG_CRASH("seh_set_wireframe: WRITE FAILED ptr=0x%llX offset=0x%llX",
                    (unsigned long long)ptr, (unsigned long long)offsets::WireFrame);
            }
            return;
        }

        if (!enable) return;

        const unsigned char before = (unsigned char)(r & 0xFF);
        const unsigned char after = (unsigned char)((r >> 8) & 0xFF);
        const bool bit_set = (after & (1 << 5)) != 0;

        static int enable_logged = 0;
        if (enable_logged < 8) {
            enable_logged++;
            LOG_INFO("wireframe write #%d: ptr=0x%llX off=0x%llX before=0x%02X after=0x%02X bit5=%d",
                enable_logged, (unsigned long long)ptr, (unsigned long long)offsets::WireFrame,
                before, after, bit_set ? 1 : 0);
        }

        // If the bit reads back clear, the offset is wrong or the write is being rejected.
        if (!bit_set) {
            static int reject_logged = 0;
            if (reject_logged < 5) {
                reject_logged++;
                LOG_WARN("wireframe: bit5 did NOT stick (before=0x%02X after=0x%02X) - offset 0x%llX likely WRONG",
                    before, after, (unsigned long long)offsets::WireFrame);
            }
            return;
        }

        // Bit stuck this frame; report whether the game resets it between frames.
        static int verify_counter = 0;
        if (++verify_counter % 600 == 0) {
            LOG_DBG("wireframe periodic: ptr=0x%llX incoming_before=0x%02X (game %s clearing bit5)",
                (unsigned long long)ptr, before,
                (before & (1 << 5)) ? "is NOT" : "IS");
        }
    }

    // C2712: SEH-only mini helpers (no fvector / unwinding in same def as __try)
    static bool seh_qs_is_alive(ashootercharacter* ply) {
        if (!ply || !memory::IsValidPointer((uintptr_t)ply)) return false;
        __try { return ply->is_alive(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static bool seh_qs_get_velocity(ashootercharacter* ply, fvector* out_vel) {
        if (!ply || !out_vel) return false;
        __try { *out_vel = ply->get_velocity(); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static USceneComponent* seh_qs_get_root(ashootercharacter* ply) {
        if (!ply) return nullptr;
        __try { return ply->K2_GetRootComponent(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    static void seh_qs_zero_root_vel(USceneComponent* root) {
        if (!root) return;
        const uintptr_t a = reinterpret_cast<uintptr_t>(root) + 0x170;
        __try {
            memory::write<float>(a, 0.f);
            memory::write<float>(a + 4, 0.f);
            memory::write<float>(a + 8, 0.f);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    static void ApplyQuickStopMovement(ashootercharacter* ply)
    {
        // Quick stop (no extra settings): when shooting/aiming, hard-stop horizontal velocity.
        if (!globals::misc::quick_stop || !ply || !memory::IsValidPointer((uintptr_t)ply)) return;
        if (!seh_qs_is_alive(ply)) return;

        // Only engage while user is actively aiming or shooting.
        const bool want_stop =
            ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) ||
            ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        if (!want_stop) return;

        static DWORD s_last_ms = 0;
        const DWORD now_ms = GetTickCount();
        if (now_ms - s_last_ms < 12) return; // ~80Hz max, prevents spam
        s_last_ms = now_ms;

        fvector vel{};
        if (!seh_qs_get_velocity(ply, &vel)) return;

        const float horiz = sqrtf(vel.x * vel.x + vel.y * vel.y);
        if (horiz < 8.0f) return;
        if (fabsf(vel.z) > 240.f) return; // don't fight airtime / jumps

        USceneComponent* root = seh_qs_get_root(ply);
        if (!root || !memory::IsValidPointer(reinterpret_cast<uintptr_t>(root))) return;
        seh_qs_zero_root_vel(root);
    }

    // hk_draw_transition (binlerce satir, C++ nesneleri) icinde __try yasak — kemik okuma burada
    static bool seh_rotating_text_bone0(uskeletalmeshcomponent* mesh_3p, fvector* out_center) {
        if (!mesh_3p || !out_center) return false;
        __try {
            *out_center = mesh_3p->get_bone_location(0);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void ApplyAutoPeek(ashootercharacter* ply, aplayercontroller* controller, ucanvas* can)
    {
        if (!globals::autopeek::enabled) return;
        if (!ply || !memory::IsValidPointer((uintptr_t)ply) || !controller || !can) return;

        const int key = globals::autopeek::peek_key;
        if (key == 0) return;

        bool alive = ply->is_alive();
        if (!alive) return;

        // Hold behavior: while key is down we anchor a position, allow small peek, then return.
        static bool s_active = false;
        static bool s_returning = false;
        static bool s_lmb_prev = false;
        static fvector s_anchor{};

        const bool key_down = (GetAsyncKeyState(key) & 0x8000) != 0;

        if (key_down && !s_active) {
            s_anchor = ply->k2_get_actor_location();
            s_active = true;
            s_returning = false;
        }

        if (!s_active) {
            s_lmb_prev = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
            return;
        }

        // Start returning when key released or when a shot is fired.
        const bool lmb_now = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
        const bool shot_edge = (lmb_now && !s_lmb_prev);
        s_lmb_prev = lmb_now;

        if (!key_down || shot_edge)
            s_returning = true;

        fvector cur = ply->k2_get_actor_location();

        // Visuals + movement.
        const float max_dist = 85.0f; // intentionally small peek (no settings)
        const float dx = cur.x - s_anchor.x;
        const float dy = cur.y - s_anchor.y;
        const float dist2d = sqrtf(dx * dx + dy * dy);
        const float fill = clamp_value(dist2d / max_dist, 0.0f, 1.0f);

        // Anchor marker: small 2D square + cross (looks clean, no huge 3D box).
        if (globals::autopeek::draw_position) {
            fvector2d a2d{};
            if (controller->project_world_location_to_screen(s_anchor, a2d, 0) && a2d.is_valid()) {
                const float sz = 10.0f;
                const float x = a2d.x - (sz * 0.5f);
                const float y = a2d.y - (sz * 0.5f);
                const flinearcolor red = flinearcolor(1.f, 0.15f, 0.15f, 0.95f);
                const flinearcolor shadow = flinearcolor(0.f, 0.f, 0.f, 0.65f);

                // shadow
                draw_rect21(can, x - 1.f, y - 1.f, sz + 2.f, sz + 2.f, shadow);
                // fill
                draw_rect21(can, x, y, sz, sz, red);
                // crosshair-like cross
                can->k2_drawline({ a2d.x - 8.f, a2d.y }, { a2d.x + 8.f, a2d.y }, 1.6f, shadow);
                can->k2_drawline({ a2d.x, a2d.y - 8.f }, { a2d.x, a2d.y + 8.f }, 1.6f, shadow);
                can->k2_drawline({ a2d.x - 7.f, a2d.y }, { a2d.x + 7.f, a2d.y }, 1.2f, red);
                can->k2_drawline({ a2d.x, a2d.y - 7.f }, { a2d.x, a2d.y + 7.f }, 1.2f, red);
            }
        }

        {
            fvector2d ss = can->get_screen_size();
            if (ss.is_valid()) {
                const float bar_w = 180.0f;
                const float bar_h = 7.0f;
                const float x0 = (ss.x * 0.5f) - (bar_w * 0.5f);
                const float y0 = (ss.y * 0.64f);

                const flinearcolor bg = flinearcolor(0.f, 0.f, 0.f, 0.50f);
                const flinearcolor fg = flinearcolor(1.f, 0.18f, 0.18f, 0.92f);
                const flinearcolor br = flinearcolor(0.f, 0.f, 0.f, 0.80f);

                // background + border
                draw_rect21(can, x0, y0, bar_w, bar_h, bg);
                DrawBorder(can, x0, y0, bar_w, bar_h, 1.0f, br);
                // fill
                draw_rect21(can, x0 + 1.f, y0 + 1.f, (bar_w - 2.f) * fill, bar_h - 2.f, fg);
            }
        }

        // Movement enforcement: while holding, keep peek small; while returning, pull back to anchor.
        USceneComponent* root = nullptr;
        root = ply->K2_GetRootComponent();
        if (!root || !memory::IsValidPointer(reinterpret_cast<uintptr_t>(root))) return;

        if (s_returning) {
            if (dist2d < 6.0f) {
                s_active = false;
                s_returning = false;
                return;
            }

            const float inv = (dist2d > 1.0f) ? (1.0f / dist2d) : 1.0f;
            const fvector dir(-(dx * inv), -(dy * inv), 0.f);
            // Guaranteed return: set velocity + gently step position back (prevents "no return" cases).
            const fvector vel(dir.x * 900.f, dir.y * 900.f, 0.f);
            memory::write<fvector>(reinterpret_cast<uintptr_t>(root) + 0x170, vel);

            const float step = clamp_value(dist2d * 0.35f, 12.0f, 55.0f);
            fvector newPos(cur.x + dir.x * step, cur.y + dir.y * step, cur.z);
            USceneComponentHelpers::SetRelativeLocation(root, newPos, false, true);
            return;
        }

        // Holding: clamp the peek radius (clean behavior, not jittery).
        if (dist2d > max_dist) {
            const float inv = 1.0f / (dist2d > 1.0f ? dist2d : 1.0f);
            const float nx = dx * inv;
            const float ny = dy * inv;
            fvector clamped(s_anchor.x + nx * max_dist, s_anchor.y + ny * max_dist, cur.z);
            USceneComponentHelpers::SetRelativeLocation(root, clamped, false, true);
        }
    }

    static bool seh_get_weapon_name_safe(currentequippable* weapon, wchar_t* out_buf, int buf_len) {
        __try {
            if (!weapon || !memory::IsValidPointer((uintptr_t)weapon)) return false;
            fstring obj_name = system::get_object_name(weapon);
            fstring converted = helper::convert_weapon_name(obj_name);
            const wchar_t* src = converted.c_str();
            if (!src) return false;
            wcsncpy_s(out_buf, buf_len, src, _TRUNCATE);
            return true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("seh_get_weapon_name: EXCEPTION reading weapon name");
            return false;
        }
    }

    static tarray<ashootercharacter*> seh_find_shooters(uworld* world, ashootercharacter* character) {
        tarray<ashootercharacter*> result = {};
        __try {
            result = blueprints::find_all_shooters_with_alliance(world, character, earesalliance::enemy, false, false);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("seh_find_shooters: EXCEPTION in find_all_shooters");
            memset(&result, 0, sizeof(result));
        }
        return result;
    }

    static bool seh_bot_slot_has_ptr(uintptr_t base, uintptr_t off) {
        if (!base || !memory::IsValidPointerFast(base)) return false;
        __try {
            uintptr_t v = *(uintptr_t*)(base + off);
            if (!v || v < 0x10000 || v > 0x7FFFFFFFFFFF) return false;
            volatile unsigned char probe = *(unsigned char*)v;
            (void)probe;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static uskeletalmeshcomponent* seh_bot_get_cosmetic_mesh(ashootercharacter* actor) {
        if (!actor) return nullptr;
        __try {
            uskeletalmeshcomponent* m = actor->GetCosmeticMesh3P();
            if (m && memory::IsValidPointerFast((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        return nullptr;
    }

    static uskeletalmeshcomponent* seh_bot_get_mesh(ashootercharacter* actor) {
        if (!actor) return nullptr;
        __try {
            uskeletalmeshcomponent* m = actor->get_mesh();
            if (m && memory::IsValidPointerFast((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        // Fallback: direct mesh offset (bots sometimes settle getters later)
        __try {
            uskeletalmeshcomponent* m = memory::read<uskeletalmeshcomponent*>((uintptr_t)actor + offsets::mesh);
            if (m && memory::IsValidPointerFast((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        __try {
            uskeletalmeshcomponent* m = memory::read<uskeletalmeshcomponent*>((uintptr_t)actor + offsets::mesh3p);
            if (m && memory::IsValidPointerFast((uintptr_t)m)) return m;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        return nullptr;
    }

    static bool seh_bot_bone0_impl(uskeletalmeshcomponent* mesh) {
        fvector r = mesh->get_bone_location(0);
        return r.is_valid() && !r.is_null();
    }

    static bool seh_bot_bone0_ok(uskeletalmeshcomponent* mesh) {
        if (!mesh) return false;
        __try { return seh_bot_bone0_impl(mesh); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    // True when a freshly spawned bot is safe to touch (mesh + bone ready).
    static bool seh_bot_actor_ready(ashootercharacter* actor) {
        if (!actor || !memory::IsValidPointerFast((uintptr_t)actor)) return false;
        __try {
            if (!IsValidUObject((uobject*)actor)) return false;
            if (!actor->is_alive()) return false;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
        uskeletalmeshcomponent* mesh = seh_bot_get_mesh(actor);
        if (!mesh || !IsValidUObject((uobject*)mesh)) return false;
        return seh_bot_bone0_ok(mesh);
    }

    static void seh_probe_bot_actor_ptrs(ashootercharacter* actor, int actor_index) {
        if (!actor || !memory::IsValidPointerFast((uintptr_t)actor)) return;
        int hits = 0;
        char hitbuf[512] = {};
        int hitlen = 0;
        for (int i = 0; i < offsets::bot_actor_ptrs_count; i++) {
            uintptr_t off = offsets::bot_actor_ptrs[i];
            if (!seh_bot_slot_has_ptr((uintptr_t)actor, off)) continue;
            hits++;
            if (hitlen < (int)sizeof(hitbuf) - 16)
                hitlen += sprintf_s(hitbuf + hitlen, sizeof(hitbuf) - hitlen, " 0x%llX", (unsigned long long)off);
        }
        uskeletalmeshcomponent* mesh = seh_bot_get_mesh(actor);
        LOG_INFO("BOT PROBE[#%d] actor=0x%llX ready=%d mesh=0x%llX hits=%d/%d offs:%s",
            actor_index,
            (unsigned long long)actor,
            seh_bot_actor_ready(actor) ? 1 : 0,
            (unsigned long long)mesh,
            hits, offsets::bot_actor_ptrs_count,
            hitbuf);
    }

    static void seh_moscawireframe(ashootercharacter* character) {
        __try {
            moscawireframe(character);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("moscawireframe EXCEPTION");
        }
    }

    static void seh_apply_chams(acknowledgedpawn* pawn, ashootercharacter* character) {
        __try {
            apply_local_galaxy_chams(pawn, character);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("apply_local_galaxy_chams EXCEPTION");
        }
    }

    static void seh_apply_fresnel(acknowledgedpawn* pawn, ashootercharacter* character) {
        __try {
            apply_hand_fresnel(pawn, character);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("apply_hand_fresnel EXCEPTION");
        }
    }

    static void seh_set_3p_mesh_visible(ashootercharacter* character, bool visible) {
        if (!character || !memory::IsValidPointer((uintptr_t)character)) return;
        __try {
            character->Set3pMeshVisible(visible);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("Set3pMeshVisible EXCEPTION");
        }
    }

    static void seh_process_fp_mode(ashootercharacter* character) {
        static bool last_menu_open = false;

        if (burat::menu_open != last_menu_open) {
            last_menu_open = burat::menu_open;
            InvalidateViewModelCache();
        }

        if (!character || !memory::IsValidPointer((uintptr_t)character))
            return;

        __try {
            process_fp_mode(character);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("process_fp_mode EXCEPTION");
            InvalidateViewModelCache();
        }
    }

    static void seh_apply_no_aspect_viewmodel(ashootercharacter* character) {
        if (!globals::misc::no_aspect_viewmodel || !globals::misc::aspectratio) return;
        if (globals::misc::ViewModelChanger) return;
        if (!character || !memory::IsValidPointer((uintptr_t)character)) return;
        if (!character->is_alive()) return;

        __try {
            auto mesh1p = character->getmesh1p();
            if (!mesh1p || !memory::IsValidPointer((uintptr_t)mesh1p)) return;

            float ar = globals::misc::aspectfloat;
            if (ar < 0.01f) return;
            fvector scale(1.0 / ar, 1.0, 1.0);

            seh_set_relative_scale_3d((uintptr_t)mesh1p, scale);

            auto overlay = character->GetOverlayMesh1P();
            if (overlay && memory::IsValidPointer((uintptr_t)overlay))
                seh_set_relative_scale_3d((uintptr_t)overlay, scale);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void seh_reset_no_aspect_viewmodel(ashootercharacter* character) {
        if (!character || !memory::IsValidPointer((uintptr_t)character)) return;
        __try {
            auto mesh1p = character->getmesh1p();
            if (mesh1p && memory::IsValidPointer((uintptr_t)mesh1p))
                seh_set_relative_scale_3d((uintptr_t)mesh1p, fvector(1.0, 1.0, 1.0));
            auto overlay = character->GetOverlayMesh1P();
            if (overlay && memory::IsValidPointer((uintptr_t)overlay))
                seh_set_relative_scale_3d((uintptr_t)overlay, fvector(1.0, 1.0, 1.0));
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void seh_set_cinematic_mode(aplayercontroller* ctrl, bool enable) {
        if (!ctrl || !memory::IsValidPointer((uintptr_t)ctrl)) return;
        if (!IsValidUObject((uobject*)ctrl)) return;

        static uobject* fn_cinematic = nullptr;
        if (!fn_cinematic)
            fn_cinematic = uobject::find_object<uobject*>(crypt(L"Engine.PlayerController.SetCinematicMode"));
        if (!fn_cinematic || !memory::IsValidPointer((uintptr_t)fn_cinematic)) return;

        __try {
            struct {
                bool bInCinematicMode;
                bool bHidePlayer;
                bool bAffectsHUD;
                bool bAffectsMovement;
                bool bAffectsTurning;
            } params = { enable, false, true, false, false };
            ctrl->process_event(fn_cinematic, &params);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("SetCinematicMode EXCEPTION enable=%d", (int)enable);
        }
    }

    static void seh_menu_render(ucanvas* canvas) {
        __try {
            (burat::Menu)(canvas);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("burat::Menu crashed! exception in menu rendering");
        }
    }

    static int unlock_resolve_max_level(equippable_skin_data_asset* s_data, uintptr_t skin_inv_obj = 0) {
        int lvl = 1;
        if (skin_inv_obj && memory::IsValidPointer(skin_inv_obj)) {
            tarray<uobject*> pl = memory::read<tarray<uobject*>>(skin_inv_obj + offsets::possible_levels);
            if (pl.size() > 0 && pl.size() < 50)
                lvl = pl.size();
        }
        if (s_data && memory::IsValidPointer((uintptr_t)s_data)) {
            const int asset_lvls = (int)s_data->get_skin_levels().size();
            if (asset_lvls > lvl) lvl = asset_lvls;
        }
        return lvl > 0 ? lvl : 1;
    }

    static bool unlock_try_apply_skin(uworld* world, currentequippable* local_weapon,
        equippable_skin_data_asset* s_data, uobject* chroma_asset, int max_lvl, const char* src) {
        if (!world || !local_weapon || !s_data || !chroma_asset) return false;
        if (!memory::IsValidPointer((uintptr_t)s_data)) return false;
        if (!memory::IsValidPointer((uintptr_t)chroma_asset)) return false;
        if (max_lvl <= 0) max_lvl = 1;
        LOG_DBG("unlock_all in-hand (%s): type=%d lvl=%d skin=0x%llX chroma=0x%llX",
            src, s_data->get_type(), max_lvl,
            (unsigned long long)s_data, (unsigned long long)chroma_asset);
        skin_changer::unlock_all_apply(world, local_weapon, s_data, chroma_asset, max_lvl, nullptr, -1);
        return true;
    }

    static bool unlock_inhand_via_inventory(uworld* world, currentequippable* local_weapon) {
        if (!world || !local_weapon) return false;

        equippable_skin_data_asset* current_skin = local_weapon->get_skin_data_asset();
        if (!current_skin) return false;

        const int32_t current_type = current_skin->get_type();
        if (current_type == 0) return false;

        auto client = ares_instance::get_ares_client_game_instance(world);
        if (!client) return false;

        inventorymanager* manager = client->get_inventory_manager();
        if (!manager) return false;

        tarray<uobject*> equippable_models = memory::read<tarray<uobject*>>(
            std::uintptr_t(manager) + offsets::equippable_models);
        if (equippable_models.size() <= 0 || equippable_models.size() > 500) return false;

        // Pass 1: equipped skin on matching weapon model (what the collection UI shows).
        for (int32_t i = 0; i < equippable_models.size(); i++) {
            auto model = equippable_models[i];
            if (!model || !memory::IsValidPointer((uintptr_t)model)) continue;

            auto* inv_model = reinterpret_cast<equippable_inventory_model*>(model);
            equippable_skin_inventory_model* equipped = inv_model->get_equipped_skin_model();
            if (!equipped || !memory::IsValidPointer((uintptr_t)equipped)) continue;

            equippable_skin_data_asset* s_data = equipped->get_skin_data_asset();
            if (!s_data || !memory::IsValidPointer((uintptr_t)s_data)) continue;
            if (s_data->get_type() != current_type) continue;

            uobject* chroma_asset = nullptr;
            if (auto chroma_inv = equipped->get_skin_inventory_chroma_asset();
                chroma_inv && memory::IsValidPointer((uintptr_t)chroma_inv))
                chroma_asset = chroma_inv->get_skin_chroma_data_asset();

            if (!chroma_asset || !memory::IsValidPointer((uintptr_t)chroma_asset)) continue;

            const int max_lvl = unlock_resolve_max_level(s_data, (uintptr_t)equipped);
            return unlock_try_apply_skin(world, local_weapon, s_data, chroma_asset, max_lvl, "equipped model");
        }

        // Pass 2: first owned/unlocked skin for this weapon type.
        for (int32_t i = 0; i < equippable_models.size(); i++) {
            auto model = equippable_models[i];
            if (!model) continue;

            tarray<uobject*> skins = memory::read<tarray<uobject*>>(std::uintptr_t(model) + offsets::skins);
            if (skins.size() <= 0 || skins.size() > 200) continue;

            for (int32_t a = 0; a < skins.size(); a++) {
                auto skin_obj = skins[a];
                if (!skin_obj || !memory::IsValidPointer((uintptr_t)skin_obj)) continue;

                const bool owned =
                    memory::read<bool>(std::uintptr_t(skin_obj) + offsets::bIsOwned) ||
                    memory::read<bool>(std::uintptr_t(skin_obj) + offsets::bIsUnlockedByConfig);
                if (!owned) continue;

                auto s_data = memory::read<equippable_skin_data_asset*>(
                    std::uintptr_t(skin_obj) + offsets::get_skin_data_asset);
                if (!s_data || !memory::IsValidPointer((uintptr_t)s_data)) continue;
                if (s_data->get_type() != current_type) continue;

                uobject* chroma_asset = nullptr;
                tarray<uobject*> chromas = memory::read<tarray<uobject*>>(
                    std::uintptr_t(skin_obj) + offsets::possible_chromas);
                if (chromas.size() > 0 && chromas.size() < 50 && chromas[0]) {
                    chroma_asset = memory::read<uobject*>(
                        std::uintptr_t(chromas[0]) + offsets::get_skin_chroma_data_asset);
                }
                if (!chroma_asset || !memory::IsValidPointer((uintptr_t)chroma_asset)) continue;

                const int max_lvl = unlock_resolve_max_level(s_data, (uintptr_t)skin_obj);
                return unlock_try_apply_skin(world, local_weapon, s_data, chroma_asset, max_lvl, "owned inventory");
            }
        }

        // Pass 3: legacy fallback — any skin matching weapon type.
        for (int32_t i = 0; i < equippable_models.size(); i++) {
            auto model = equippable_models[i];
            if (!model) continue;

            tarray<uobject*> skins = memory::read<tarray<uobject*>>(std::uintptr_t(model) + offsets::skins);
            if (skins.size() <= 0 || skins.size() > 200) continue;

            for (int32_t a = 0; a < skins.size(); a++) {
                auto skin_obj = skins[a];
                if (!skin_obj) continue;

                auto s_data = memory::read<equippable_skin_data_asset*>(
                    std::uintptr_t(skin_obj) + offsets::get_skin_data_asset);
                if (!s_data || !memory::IsValidPointer((uintptr_t)s_data)) continue;
                if (s_data->get_type() != current_type) continue;

                uobject* chroma_asset = nullptr;
                tarray<uobject*> chromas = memory::read<tarray<uobject*>>(
                    std::uintptr_t(skin_obj) + offsets::possible_chromas);
                if (chromas.size() > 0 && chromas.size() < 50 && chromas[0]) {
                    chroma_asset = memory::read<uobject*>(
                        std::uintptr_t(chromas[0]) + offsets::get_skin_chroma_data_asset);
                }
                if (!chroma_asset || !memory::IsValidPointer((uintptr_t)chroma_asset)) continue;

                const int max_lvl = unlock_resolve_max_level(s_data, (uintptr_t)skin_obj);
                return unlock_try_apply_skin(world, local_weapon, s_data, chroma_asset, max_lvl, "fallback inventory");
            }
        }
        return false;
    }

    static bool seh_unlock_inhand(uworld* world, currentequippable* local_weapon, uinventory* inventory) {
        bool applied = false;
        __try {
            equippable_skin_data_asset* skin_data_asset = local_weapon->get_skin_data_asset();
            if (!skin_data_asset)
                return false;

            const int32_t current_type = skin_data_asset->get_type();
            if (current_type == 0)
                return false;

            // Inventory equipped skin first — works in-match; arsenal VM is menu-only.
            if (unlock_inhand_via_inventory(world, local_weapon)) {
                return true;
            }

            auto client_instance = ares_instance::get_ares_client_game_instance(world);
            arsenal_view_controller* arsenal_ctrl = client_instance ? client_instance->get_aresnal_view_controller() : nullptr;

            if (arsenal_ctrl)
            {
                arsenal_view_model* arsenal_model = arsenal_ctrl->get_view_model();
                if (arsenal_model)
                {
                    auto models = arsenal_model->get_gun_models();
                    LOG_DBG("unlock_all in-hand: type=%d models.count=%d", current_type, models.count);
                    for (int i = 0; i < models.count; i++) {
                        equippable_inventory_model* mod = models[i];
                        if (!mod) continue;

                        equippable_skin_inventory_model* skin_mod = mod->get_equipped_skin_model();
                        if (!skin_mod) continue;

                        equippable_skin_data_asset* s_data = skin_mod->get_skin_data_asset();
                        if (!s_data || s_data->get_type() != current_type) continue;

                        int max_lvl = unlock_resolve_max_level(s_data, (uintptr_t)skin_mod);
                        auto chroma_inv = skin_mod->get_skin_inventory_chroma_asset();
                        uobject* chroma_asset = chroma_inv ? chroma_inv->get_skin_chroma_data_asset() : nullptr;

                        if (chroma_asset) {
                            applied = unlock_try_apply_skin(world, local_weapon, s_data, chroma_asset, max_lvl, "arsenal vm");
                            if (applied) break;
                        }
                    }
                }
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LOG_CRASH("unlock_all in-hand: CRASH during skin apply");
            return false;
        }
        return applied;
    }

    void hk_draw_transition(ugameviewportclient* viewportclient, ucanvas* canvas_, std::uintptr_t a3) {
        static bool first_frame = true;
        if (first_frame) {
            LOG_INFO("hk_draw_transition first call - viewportclient=0x%llX canvas_=0x%llX", (unsigned long long)viewportclient, (unsigned long long)canvas_);
            first_frame = false;
        }
        if (!viewportclient) { return draw_transition_o(viewportclient, canvas_, a3); }
        if (!canvas) { canvas = uobject::find_object<ucanvas*>(L"/Engine/Transient.DebugCanvasObject", (uobject*)-1); }

        static bool tperson_key_was_down = false;
        if (globals::misc::thirdperson_bind != 0) {
            bool tperson_key_down = (GetAsyncKeyState(globals::misc::thirdperson_bind) & 0x8000) != 0;
            if (tperson_key_down && !tperson_key_was_down) {
                globals::misc::tperson = !globals::misc::tperson;
            }
            tperson_key_was_down = tperson_key_down;
        }

        static bool emote_key_was_down = false;
        if (globals::misc::emote_load_bind != 0) {
            bool emote_key_down = (GetAsyncKeyState(globals::misc::emote_load_bind) & 0x8000) != 0;
            if (emote_key_down && !emote_key_was_down) {
                EmoteLoad::ToggleEmote();
            }
            emote_key_was_down = emote_key_down;
        }

        if (globals::misc::emote_toggle_request) {
            EmoteLoad::ToggleEmote();
            globals::misc::emote_toggle_request = false;
        }

        int target_id = -1;
        static ashootercharacter* last_aim_target = nullptr; 
        static bool auto_shot_fired_flag = false; // 100% Flag for Auto-Shoot tracers
        bool unlock_skin = false;
        bool skins_updated = false;
        double closest_distance = DBL_MAX;
        ashootercharacter* target_actor = nullptr;

        static ULONGLONG s_frame_counter = 0;
        static ULONGLONG s_inject_time = 0;
        s_frame_counter++;
        if (s_inject_time == 0) {
            s_inject_time = GetTickCount64();
            LOG_INFO("INJECT time recorded - camera hook deferred (min 120 frames / 3s)");
        }

        do
        {
            uworld* world = reinterpret_cast<uworld*>(viewportclient->get_world());
            if (!world || !memory::IsValidPointer((uintptr_t)world)) continue;
            UWorldSave = world;

            static uintptr_t s_last_world_ptr = 0;
            static ULONGLONG s_world_change_time = 0;
            static ULONGLONG s_bot_spawn_grace_until = 0;
            static ULONGLONG s_inject_grace_until = 0;
            static bool s_inject_grace_init = false;
            if (!s_inject_grace_init) {
                s_inject_grace_init = true;
                s_inject_grace_until = GetTickCount64() + 5000;
                LOG_INFO("INJECT grace 5s - mesh ops disabled until stable");
            }
            if ((uintptr_t)world != s_last_world_ptr) {
                LOG_INFO("World changed: old=0x%llX new=0x%llX", (unsigned long long)s_last_world_ptr, (unsigned long long)world);
                s_last_world_ptr = (uintptr_t)world;
                s_world_change_time = GetTickCount64();
                s_bot_spawn_grace_until = 0;
                target_actor = nullptr;
                last_aim_target = nullptr;
                CachedSkyDome = nullptr;
                SkyDomeCached = false;
                SkyDome = nullptr;
                NoSmoke::cachedSmokes.clear();
                NoSmoke::lastSmokeUpdate = 0.0;
                NoSmoke::lastSmokeWorldPtr = 0;
                lastweapon = nullptr;
                myweapon = nullptr;
                resolver_instances.clear();
                last_resolver_cleanup = 0;
                PruneMidCache("world change");
                globals::misc::unlock_all_active = false;
                globals::misc::unlock_all_pending = false;
            }
            const bool g_mesh_ops_safe = (GetTickCount64() - s_world_change_time) > 3000
                && (GetTickCount64() >= s_bot_spawn_grace_until)
                && (GetTickCount64() >= s_inject_grace_until);
            const bool g_local_chams_safe = g_mesh_ops_safe && !is_weapon_swap_grace_active();

            static ULONGLONG s_last_mid_prune = 0;
            if (g_mesh_ops_safe) {
                const ULONGLONG now_prune = GetTickCount64();
                if (now_prune - s_last_mid_prune > 15000) {
                    s_last_mid_prune = now_prune;
                    PruneMidCache("periodic");
                }
            }

            static tarray<ashootercharacter*> cached_world_objects = {};
            static DWORD last_cache_time = 0;
            static uintptr_t last_cached_world_ptr = 0;
            if ((uintptr_t)world != last_cached_world_ptr) {
                memset(&cached_world_objects, 0, sizeof(cached_world_objects));
                last_cache_time = 0;
                last_cached_world_ptr = (uintptr_t)world;
            }
            if (globals::visuals::spike && (GetTickCount() - last_cache_time > 1000)) {
                cached_world_objects = blueprints::find_all_game_objects(world);
                last_cache_time = GetTickCount();
            }

            PreCacheAllVisuals();


            controllers = blueprints::get_player_controller(world);
            if (!controllers || !memory::IsValidPointer((uintptr_t)controllers)) continue;

            negrumdaddy::insta_lock::run(controllers);

            seh_menu_render(canvas);
            
            if (globals::misc::nightmode && canvas && memory::IsValidPointer((uintptr_t)canvas)) {
                float screen_width = canvas->get_screen_size().x;
                canvas->K2_DrawTransparentLine(
                    fvector2d(-5000.0f, 0.0f),
                    fvector2d(screen_width + 5000.0f, 0.0f),
                    20000.0f,
                    flinearcolor(globals::misc::nightmode_r, globals::misc::nightmode_g, globals::misc::nightmode_b, globals::misc::nightmode_intensity)
                );
            }


            if (globals::misc::sk1n_chang3r) {
                LOG_DBG("unlock_all_skins triggered, world=0x%llX", (unsigned long long)world);
                skin_changer::unlock_all_skins(world);
            }

            aplayercameramanager* camera = controllers->get_camera_manager();
            if (!camera || !memory::IsValidPointer((uintptr_t)camera)) continue;

            pawn = controllers->get_acknowledged_pawn();
            if (!pawn || !memory::IsValidPointer((uintptr_t)pawn)) continue;

            character = controllers->get_shooter_character();
            if (!character || !memory::IsValidPointer((uintptr_t)character)) continue;

            uinventory* inventory = nullptr;
            currentequippable* local_weapon = nullptr;
            std::wstring local_weapon_name = L"";
            
            {
                auto current_inv = character->get_inventory();
                if (current_inv && memory::IsValidPointer((uintptr_t)current_inv)) {
                    inventory = current_inv;
                    local_weapon = inventory->get_current_equippable();
                    if (local_weapon && memory::IsValidPointer((uintptr_t)local_weapon)) {
                        wchar_t wpn_buf[128] = {};
                        if (seh_get_weapon_name_safe(local_weapon, wpn_buf, 128))
                            local_weapon_name = wpn_buf;

                        static currentequippable* s_last_weapon_ptr = nullptr;
                        static std::wstring s_last_weapon_name;
                        static ULONGLONG s_last_chams_grace_time = 0;
                        if (local_weapon != s_last_weapon_ptr) {
                            s_last_weapon_ptr = local_weapon;
                            if (is_weapon_ready_for_ops(local_weapon, local_weapon_name)) {
                                InvalidateViewModelCache();
                                const bool name_changed = (local_weapon_name != s_last_weapon_name);
                                if (name_changed) {
                                    s_last_weapon_name = local_weapon_name;
                                    // Unlock All swaps weapons often — do not nuke MID cache for that.
                                    if (!globals::misc::unlock_all_active) {
                                        const ULONGLONG now_wpn = GetTickCount64();
                                        if (now_wpn - s_last_chams_grace_time >= 300) {
                                            s_last_chams_grace_time = now_wpn;
                                            notify_weapon_swap_grace("weapon change");
                                        }
                                    }
                                    LOG_INFO("WEAPON CHANGED: ptr=0x%llX name=%ls (frame=%llu)",
                                        (unsigned long long)local_weapon,
                                        local_weapon_name.empty() ? L"<unreadable>" : local_weapon_name.c_str(),
                                        s_frame_counter);
                                } else {
                                    LOG_DBG("WEAPON ptr refresh: ptr=0x%llX name=%ls (frame=%llu)",
                                        (unsigned long long)local_weapon,
                                        local_weapon_name.empty() ? L"<unreadable>" : local_weapon_name.c_str(),
                                        s_frame_counter);
                                }
                            } else {
                                LOG_DBG("WEAPON transition (unstable): ptr=0x%llX name=%ls (frame=%llu)",
                                    (unsigned long long)local_weapon,
                                    local_weapon_name.empty() ? L"<unreadable>" : local_weapon_name.c_str(),
                                    s_frame_counter);
                            }
                        }
                    } else {
                        local_weapon = nullptr;
                    }
                }
            }
            myweapon = local_weapon;

            if (g_local_chams_safe && character->is_alive()) {
                seh_moscawireframe(character);
                
                if (globals::chams::self_galaxy_enabled) {
                    seh_apply_chams(pawn, character);
                }
                if (globals::chams::hand_galaxy_enabled) {
                }
                seh_apply_fresnel(pawn, character);
            }

            static bool char_custom_mesh_active = false;
            if (globals::misc::custom_char_enabled && character->is_alive()) {
                static uintptr_t last_char_applied = 0;
                static uworld* last_char_world = nullptr;

                if (character && (last_char_applied != (uintptr_t)character || last_char_world != world)) {
                    std::string b = GetPublicPath();
                    std::wstring wb(b.begin(), b.end());
                    ReplaceCharacterMeshWith3DModel(character, (b + "naked.obj").c_str(), (wb + L"naked_tex.png").c_str());
                    last_char_applied = (uintptr_t)character;
                    last_char_world = world;
                    char_custom_mesh_active = true;
                }
            }
            else if (char_custom_mesh_active && character->is_alive()) {
                ResetCharacterMesh(character);
                char_custom_mesh_active = false;
            }



            uskeletalmeshcomponent* mesh3p = character->mesh3p();
            if (!mesh3p || !memory::IsValidPointer((uintptr_t)mesh3p)) continue;

            if (globals::misc::wukong && character->is_alive()) {
                uskeletalmeshcomponent* mesh_cosmetic_3p = character->GetCosmeticMesh3P();
                if (mesh_cosmetic_3p && memory::IsValidPointer((uintptr_t)mesh_cosmetic_3p)) {
                    static uobject* wukong_mesh = nullptr;
                    if (!wukong_mesh) {
                        const wchar_t* meshPath = crypt(L"/Game/Equippables/Finishers/MonkeyKing/Character/Models/MonkeyKing_Finisher_Skelmesh.MonkeyKing_Finisher_Skelmesh").decrypt();
                        wukong_mesh = uobject::find_object<uobject*>(meshPath);
                        if (!wukong_mesh)
                            wukong_mesh = uobject::StaticLoadObject(nullptr, nullptr, meshPath);
                    }

                    if (wukong_mesh && memory::IsValidPointer((uintptr_t)wukong_mesh)) {
                        mesh_cosmetic_3p->SetSkinnedAssetAndUpdate(wukong_mesh, true);
                    }
                }
            }

            ApplyQuickStopMovement(character);
            ApplyAutoPeek(character, controllers, canvas);

            // Managed in SetCameraCachePOVHook
            camera_cache = camera;

            if (pawn != nullptr && local_weapon != nullptr && character->is_alive()) {

                static bool s_unlock_prev_pending = false;
                static std::wstring s_unlock_last_applied_name;
                static ULONGLONG s_unlock_last_apply_ms = 0;

                const bool need_unlock_apply =
                    globals::misc::unlock_all_active || globals::misc::unlock_all_pending;

                if (need_unlock_apply && inventory && is_weapon_ready_for_ops(local_weapon, local_weapon_name)) {
                    const ULONGLONG now = GetTickCount64();

                    if (globals::misc::unlock_all_pending && !s_unlock_prev_pending) {
                        s_unlock_last_applied_name.clear();
                        globals::misc::unlock_all_pending = false;
                        LOG_DBG("unlock_all: pending consumed — re-apply current weapon once");
                    }
                    s_unlock_prev_pending = globals::misc::unlock_all_pending;

                    const bool needs_apply = !local_weapon_name.empty()
                        && local_weapon_name != s_unlock_last_applied_name;

                    if (needs_apply && (now - s_unlock_last_apply_ms >= 400)) {
                        s_unlock_last_apply_ms = now;
                        if (seh_unlock_inhand(world, local_weapon, inventory)) {
                            s_unlock_last_applied_name = local_weapon_name;
                            LOG_DBG("unlock_all: applied once for %ls", local_weapon_name.c_str());
                        }
                    }
                    lastweapon = local_weapon;
                }
                else {
                    s_unlock_prev_pending = globals::misc::unlock_all_pending;
                    if (globals::misc::sk1n_chang3r && local_weapon != lastweapon)
                    {
                        if (inventory && is_weapon_ready_for_ops(local_weapon, local_weapon_name)) {
                            seh_unlock_inhand(world, local_weapon, inventory);
                        }
                        lastweapon = local_weapon;
                    }
                }
            }

            fvector2d screen_size = canvas->get_screen_size();
            if (!screen_size.is_valid()) continue;

            screen_center_x = (double)canvas->get_screen_size().x / 2.f;
            screen_center_y = (double)canvas->get_screen_size().y / 2.f;

            if (s_frame_counter % 500 == 0) {
                LOG_DBG("heartbeat frame=%llu weapon=%ls alive=%d", s_frame_counter,
                    local_weapon_name.empty() ? L"none" : local_weapon_name.c_str(),
                    character ? character->is_alive() : 0);
            }

            {
                static ULONGLONG last_feature_log = 0;
                ULONGLONG now = GetTickCount64();
                if (now - last_feature_log > 30000) {
                    last_feature_log = now;
                    LOG_INFO("=== FEATURE STATE (frame=%llu) ===", s_frame_counter);
                    LOG_INFO("Aimbot: en=%d silent=%d vis=%d recoil=%d fov=%.1f smooth=%.1f",
                        globals::aimbot::a1mbot, globals::aimbot::silent, globals::aimbot::v1sh_ch3ck, 
                        globals::aimbot::reco1l_contr0l, globals::aimbot::a1m_f0v, globals::aimbot::a1m_sm00th);
                    LOG_INFO("ESP: box=%d skeleton=%d health=%d snap=%d head=%d filled=%d",
                        globals::visuals::b0x, globals::visuals::sk3let0n, globals::visuals::h3althbar,
                        globals::visuals::snapl1ne, globals::visuals::headb0x, globals::visuals::filled_box);
                    LOG_INFO("Misc: bhop=%d unlock=%d wireE=%d wireS=%d wireH=%d vm=%d ar=%d(%.2f)",
                        globals::misc::bhop, globals::misc::sk1n_chang3r,
                        globals::misc::Wireframe, globals::misc::self_wireframe, globals::misc::HandWire,
                        globals::misc::ViewModelChanger, globals::misc::aspectratio, globals::misc::aspectfloat);
                    LOG_INFO("Extra: skybox=%d chinahat=%d trace=%d nightmode=%d",
                        globals::misc::skybox, globals::visuals::chinahat,
                        globals::misc::pulse_trace, globals::misc::nightmode);
                    LOG_INFO("Chams: black=%d type=%d sentVis=%d sentInvis=%d galEnemy=%d handGal=%d handGalV2=%d",
                        globals::chams::black_chams_enemy,
                        globals::chams::black_chams_type,
                        globals::chams::sentinel_chams_visible_enemy,
                        globals::chams::sentinel_chams_invisible_enemy,
                        globals::chams::enemy_galaxy_enabled,
                        globals::chams::hand_galaxy_enabled,
                        globals::chams::nebula_galaxy_hand);
                    LOG_INFO("MID cache: entries=%zu created=%llu reused=%llu",
                        g_mid_cache.size(), g_mid_created, g_mid_reused);
                    LOG_PTR("character", character);
                    LOG_PTR("world", world);
                    LOG_INFO("screen=%.0fx%.0f mesh_safe=%d", screen_size.x, screen_size.y, g_mesh_ops_safe);
                    LOG_INFO("=================================");
                }
            }

            if (false && globals::Watermark) {
                static LARGE_INTEGER freq = {0};
                if (freq.QuadPart == 0) {
                    QueryPerformanceFrequency(&freq);
                }

                static LARGE_INTEGER last_frame_time = {0};
                LARGE_INTEGER now_time;
                QueryPerformanceCounter(&now_time);

                float frame_ms = 0.0f;
                if (last_frame_time.QuadPart > 0) {
                    frame_ms = static_cast<float>((now_time.QuadPart - last_frame_time.QuadPart) * 1000.0 / freq.QuadPart);
                }
                last_frame_time = now_time;

                if (frame_ms > 0.0f && frame_ms < 1000.0f) {
                    float instant_fps = 1000.0f / frame_ms;
                    smooth_fps = (smooth_fps <= 0.0f) ? instant_fps : (smooth_fps * 0.95f + instant_fps * 0.05f);
                }

                SYSTEMTIME local_time{};
                GetLocalTime(&local_time);

                int hour12 = local_time.wHour % 12;
                if (hour12 == 0) hour12 = 12;
                const wchar_t* ampm = (local_time.wHour >= 12) ? L"PM" : L"AM";

                int fps_value = static_cast<int>(smooth_fps + 0.5f);
                int tick_value = 128;

                wchar_t watermark_line1[96]{};
                wchar_t watermark_line2[128]{};
                swprintf_s(watermark_line1, L"POWERED BY VIP");
                swprintf_s(watermark_line2, L"FPS | %d  TICK | %d  %02d:%02d:%02d %s", 
                    fps_value, tick_value, hour12, local_time.wMinute, local_time.wSecond, ampm);

                const float wm_x = 18.0f;
                const float wm_y = 18.0f;
                flinearcolor white_color = { 1.0f, 1.0f, 1.0f, 1.0f };
                flinearcolor accent_color = menu::RGBtoFLC(255, 48, 55);

                canvas->k2_drawtext(menu::font, fstring(watermark_line1), { wm_x, wm_y }, { 1.0f, 1.0f }, 
                    white_color, 0.f, { 0, 0, 0, 0.5f }, { 0, 0 }, false, true, false, { 0, 0, 0, 0.5f });

                canvas->k2_drawtext(menu::font, fstring(watermark_line2), { wm_x, wm_y + 18.0f }, { 0.78f, 0.78f }, 
                    accent_color, 0.f, { 0, 0, 0, 0.5f }, { 0, 0 }, false, true, false, { 0, 0, 0, 0.5f });
            }

            if (globals::crosshair::rainbow_crosshair)
            {
                static float rainbowTimeZCH = 0.0f;
                rainbowTimeZCH += 0.005f;
                flinearcolor RainbowColor = GetRainbowColor(rainbowTimeZCH);
                fvector2d ScreenSize = canvas->get_screen_size();

                float CenterX = ScreenSize.x / 2.0f;
                float CenterY = ScreenSize.y / 2.0f;

                float Offset = 5.0f;
                float Thickness = 2.0f;

                canvas->k2_drawline(
                    fvector2d(CenterX - Offset, CenterY),
                    fvector2d(CenterX + Offset, CenterY),
                    Thickness,
                    RainbowColor
                );

                canvas->k2_drawline(
                    fvector2d(CenterX, CenterY - Offset),
                    fvector2d(CenterX, CenterY + Offset),
                    Thickness,
                    RainbowColor
                );
            }

            drawings::draw_f0v({ screen_center_x, screen_center_y }, globals::aimbot::a1m_f0v, 100.0f, burat::fovcolor, canvas);

            if (globals::misc::sk1ptut0rial) {
                controllers->disconnect_server();
                globals::misc::sk1ptut0rial = false;
            }

            fvector2d pos = { ((double)GetSystemMetrics(SM_CXSCREEN) / 2) - 500, ((double)GetSystemMetrics(SM_CYSCREEN) / 2) - 475 };

            if (!character || !memory::IsValidPointer((uintptr_t)character) || !controllers || !memory::IsValidPointer((uintptr_t)controllers)) {
                continue;
            }

            if (!character->is_alive()) {
                continue;
            }

            tarray<ashootercharacter*> actors = seh_find_shooters(world, character);

            {
                static int s_last_actor_count = -1;
                if (actors.count != s_last_actor_count) {
                    const bool had_prior_count = (s_last_actor_count >= 0);
                    PruneMidCache("actor count change");

                    const ULONGLONG now_ac = GetTickCount64();
                    const ULONGLONG actor_grace_until = now_ac + 5000;
                    const ULONGLONG mid_block_until = now_ac + 2000;
                    if (actor_grace_until > s_bot_spawn_grace_until)
                        s_bot_spawn_grace_until = actor_grace_until;
                    if (mid_block_until > g_mid_create_blocked_until)
                        g_mid_create_blocked_until = mid_block_until;

                    if (had_prior_count && actors.count < s_last_actor_count) {
                        PruneMidCache("round reset actor despawn");
                        InvalidateViewModelCache();
                        LOG_INFO("ROUND RESET grace 5s + MID pruned (count %d -> %d frame=%llu)",
                            s_last_actor_count, actors.count, s_frame_counter);
                    }
                    else if (had_prior_count && actors.count > s_last_actor_count) {
                        LOG_INFO("BOT SPAWN grace 5s + MID create blocked (count %d -> %d frame=%llu)",
                            s_last_actor_count, actors.count, s_frame_counter);
                    }

                    if (had_prior_count) {
                        LOG_INFO("ACTOR COUNT CHANGED: %d -> %d (frame=%llu data=0x%llX mesh_safe=%d slots=%d)",
                            s_last_actor_count, actors.count, s_frame_counter,
                            (unsigned long long)actors.data, g_mesh_ops_safe ? 1 : 0,
                            offsets::bot_actor_ptrs_count);

                        int probe_n = actors.count;
                        if (probe_n < 0) probe_n = 0;
                        if (probe_n > 32) probe_n = 32;
                        if (probe_n > 0 && memory::IsValidPointer((uintptr_t)actors.data)) {
                            for (int i = 0; i < probe_n; i++) {
                                ashootercharacter* a = actors[i];
                                if (!a || a == character) continue;
                                seh_probe_bot_actor_ptrs(a, i);
                            }
                        }
                    }
                    else {
                        LOG_INFO("ACTOR COUNT initial: %d (frame=%llu) - skip probe/grace",
                            actors.count, s_frame_counter);
                    }
                    s_last_actor_count = actors.count;
                }
            }

            if (actors.count > 0 && !memory::IsValidPointer((uintptr_t)actors.data)) {
                LOG_WARN("actors.data INVALID (count=%d) - skipping frame %llu", actors.count, s_frame_counter);
                continue;
            }
            if (actors.count < 0 || actors.count > 200) {
                LOG_WARN("actors.count out of range (%d) - skipping frame %llu", actors.count, s_frame_counter);
                continue;
            }

                
            


            bool mbutton_currently_pressed = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
            if (mbutton_currently_pressed && !globals::misc::mbutton_key_pressed_last_frame) {
                globals::misc::tperson = !globals::misc::tperson;
            }
            globals::misc::mbutton_key_pressed_last_frame = mbutton_currently_pressed;



            if (globals::misc::customhand && character->is_alive())
            {
                if (auto handmesh = character->GetOverlayMesh1P()) {
                    apply_custom_hand_texture(pawn, character, world);
                    globals::misc::customhand = false;
                }
            }

            if (camera_engine != uintptr_t(camera))
            {
                camera_engine = uintptr_t(camera);
                should_hook_gay = true;
            }
            if (!camera_engine) break;

            static bool last_tperson = false;
            static ULONGLONG tperson_grace_until = 0;
            if (globals::misc::tperson != last_tperson) {
                last_tperson = globals::misc::tperson;
                // Apply mesh visibility immediately — do not wait (fixes 1–2s lag)
                seh_set_3p_mesh_visible(character, globals::misc::tperson);
                tperson_grace_until = GetTickCount64() + 120; // short grace for chams only
                InvalidateViewModelCache();
            }
            // Keep 3P visibility in sync every frame
            seh_set_3p_mesh_visible(character, globals::misc::tperson);
            const bool tperson_stable = (GetTickCount64() >= tperson_grace_until);

            HandleEmoteLoad(character);

            if (globals::misc::fastcrouch) {
                if (character) {
                    character->SetCrouchTimeOverride(0.001f);
                    globals::misc::meshmofiedfastcrouch = true;
                }
            }
            else if (character) {
                if (globals::misc::meshmofiedfastcrouch) {
                    character->SetCrouchTimeOverride(1.0f);
                    globals::misc::meshmofiedfastcrouch = false;
                }
            }

            // Only write when enabled. Writing "disable" every frame on wrong offsets
            // corrupts mesh/component memory and crashes the game on weapon/bot spawn.
            if (g_local_chams_safe && globals::misc::self_wireframe && character->is_alive()) {
                if (auto body = character->get_mesh();
                    body && memory::IsValidPointer((uintptr_t)body))
                    seh_set_wireframe((uintptr_t)body, true);
                if (auto cosmetic = character->GetCosmeticMesh3P();
                    cosmetic && memory::IsValidPointer((uintptr_t)cosmetic))
                    seh_set_wireframe((uintptr_t)cosmetic, true);
                if (globals::misc::tperson) {
                    if (auto core = character->mesh3p();
                        core && memory::IsValidPointer((uintptr_t)core))
                        seh_set_wireframe((uintptr_t)core, true);
                }
            }

            // Only reset local materials for chams that leak enemy tint onto self.
            const bool enemy_chams_self_reset =
                globals::chams::outline_enabled ||
                globals::chams::cool_chams_enabled ||
                globals::chams::custom_fresnel;

            if (g_local_chams_safe && enemy_chams_self_reset && character && character->is_alive()) {
                if (auto* self_mesh = character->get_mesh())
                    character->reset_character_materials_internal(self_mesh);
                if (auto* self_cos = character->GetCosmeticMesh3P())
                    character->reset_character_materials_internal(self_cos);
            }

            if (g_local_chams_safe && globals::chams::self_chams && character->is_alive())
            {
                apply_self_outline_chams(character);
            }

            if (globals::visuals::partyhat_self && character->is_alive()) {
                auto my_mesh_3p = character->mesh3p();
                if (my_mesh_3p) {
                    drawings::partyhat(controllers, my_mesh_3p, canvas);
                }
            }

            if (globals::visuals::rotating_text_enabled &&
                character && memory::IsValidPointer((uintptr_t)character) &&
                controllers && memory::IsValidPointer((uintptr_t)controllers) &&
                canvas && memory::IsValidPointer((uintptr_t)canvas) &&
                menu::font && memory::IsValidPointer((uintptr_t)menu::font) &&
                character->is_alive())
            {
                auto mesh_3p = character->mesh3p();
                if (mesh_3p && memory::IsValidPointer((uintptr_t)mesh_3p)) {
                    fvector center{};
                    if (!seh_rotating_text_bone0(mesh_3p, &center) || !center.is_valid())
                        goto skip_rotating_text;

                    center.z += 118.0f;

                    static const wchar_t* ring_text = L"cortexsoftware.xyz";

                    float orbit_rad = globals::visuals::rotating_text_radius;
                    if (!std::isfinite(orbit_rad)) orbit_rad = 135.0f;
                    orbit_rad = clamp_value(orbit_rad, 100.0f, 2000.0f);

                    float spin_speed = globals::visuals::rotating_text_speed;
                    if (!std::isfinite(spin_speed)) spin_speed = 0.12f;
                    spin_speed = clamp_value(spin_speed, 0.01f, 3.0f);

                    const float rot_offset =
                        (float)(GetTickCount64() % 3600000ULL) * 0.000001f * spin_speed * 6.283185307f;

                    constexpr int SEGMENTS = 64;
                    fvector2d ring_pts[64];
                    bool ring_ok[64];
                    int ring_count = 0;

                    for (int s = 0; s < SEGMENTS && ring_count < 64; ++s) {
                        const float seg_angle = rot_offset + ((float)s / (float)SEGMENTS) * 6.283185307f;
                        fvector point_3d;
                        point_3d.x = center.x + cosf(seg_angle) * orbit_rad;
                        point_3d.y = center.y + sinf(seg_angle) * orbit_rad;
                        point_3d.z = center.z;

                        ring_ok[ring_count] = point_3d.is_valid() &&
                            controllers->project_world_location_to_screen(point_3d, ring_pts[ring_count], false) &&
                            ring_pts[ring_count].is_valid();
                        ring_count++;
                    }

                    for (int i = 0; i < ring_count; ++i) {
                        if (!ring_ok[i]) continue;
                        const int j = (i + 1) % ring_count;
                        if (!ring_ok[j]) continue;
                        const fvector2d& a = ring_pts[i];
                        const fvector2d& b = ring_pts[j];
                        canvas->k2_drawline(a, b, 22.0f, flinearcolor(1.0f, 1.0f, 1.0f, 0.14f));
                        canvas->k2_drawline(a, b, 17.5f, flinearcolor(1.0f, 1.0f, 1.0f, 0.28f));
                        canvas->k2_drawline(a, b, 13.5f, flinearcolor(1.0f, 1.0f, 1.0f, 0.46f));
                        canvas->k2_drawline(a, b, 9.5f, flinearcolor(1.0f, 1.0f, 1.0f, 0.68f));
                        canvas->k2_drawline(a, b, 6.0f, flinearcolor(1.0f, 1.0f, 1.0f, 0.94f));
                    }

                    const float text_angle = rot_offset + 1.570796326f;
                    fvector text_pos_3d;
                    text_pos_3d.x = center.x + cosf(text_angle) * orbit_rad;
                    text_pos_3d.y = center.y + sinf(text_angle) * orbit_rad;
                    text_pos_3d.z = center.z;

                    fvector2d text_screen{};
                    if (text_pos_3d.is_valid() &&
                        controllers->project_world_location_to_screen(text_pos_3d, text_screen, false) &&
                        text_screen.is_valid())
                    {
                        const fvector2d text_scale(3.45f, 3.45f);
                        const flinearcolor text_main(0.06f, 0.06f, 0.08f, 1.0f);
                        const flinearcolor text_shadow(0.02f, 0.02f, 0.03f, 0.92f);
                        const flinearcolor text_outline(1.0f, 1.0f, 1.0f, 0.98f);
                        canvas->k2_drawtext(menu::font, ring_text, text_screen, text_scale,
                            text_main, 0.0f, text_shadow, { 2.2f, 2.2f }, true, true, true, text_outline);
                    }
                }
            }
        skip_rotating_text:;

            if (globals::misc::player_trail && character && character->is_alive()) {
                fvector trail_pos = character->k2_get_actor_location();
                trail_pos.z += 8.0f;
                PlayerTrail::PushPoint(trail_pos);
                PlayerTrail::Render(canvas, controllers);
            } else {
                PlayerTrail::Reset();
            }

            if (globals::misc::backtrack && character && character->is_alive()) {
                fvector origin = character->k2_get_actor_location();
                BacktrackVis::Render(canvas, controllers, origin);
            } else {
                BacktrackVis::Reset();
            }

            // ViewModel is 1P-only — applying it in third person crashes (stale 1P meshes).
            if (globals::misc::ViewModelChanger && character->is_alive() && !globals::misc::tperson && tperson_stable)
                seh_process_fp_mode(character);

            if (!globals::misc::tperson && tperson_stable && character->is_alive())
                seh_apply_no_aspect_viewmodel(character);

            if (g_local_chams_safe) {
                if (globals::misc::playerchamsself && character->is_alive())
                {
                    meshp3_material12(pawn, character);
                }
            }

            if (g_local_chams_safe && character->is_alive() &&
                (globals::chams::self_galaxy_enabled ||
                    globals::chams::hand_galaxy_enabled ||
                    globals::chams::gun1p_galaxy_enabled ||
                    globals::chams::gun3p_galaxy_enabled)) {
                apply_local_galaxy_chams(pawn, character);
            }

            if (g_local_chams_safe && character->is_alive() &&
                (globals::visuals::crystal_chams_self || globals::visuals::crystal_chams_hand)) {
                apply_crystal_local_chams(pawn, character);
            }

            // === Nebula Chams (Type 1/2/3) - Self & Hand ===
            if (g_local_chams_safe && character->is_alive()) {
                if (globals::chams::nebula_chams_self) {
                    apply_nebula_chams_self3p(character);
                }
                if (globals::chams::nebula_chams_hand) {
                    apply_nebula_chams_hand(character, pawn);
                }
            }

            if (g_mesh_ops_safe && character->is_alive() && tperson_stable) {
                if (globals::chams::nebula_galaxy_self) {
                    galaxychams_v2_self3p(character);
                }
                if (globals::chams::nebula_galaxy_hand) {
                    galaxychams_v2_hand(character, pawn);
                }
            }

            if (g_local_chams_safe && character->is_alive() && tperson_stable) {
                if (globals::chams::sentinel_chams_visible_self || globals::chams::sentinel_chams_invisible_self) {
                    seh_apply_sentinel_self_chams(character);
                }
            }

            if (g_local_chams_safe) {
                if (globals::chams::hand_cool_chams_enabled && character->is_alive())
                {
                    apply_hand_cool_chams(pawn);
                }

                if (globals::chams::self_cool_chams_enabled && character->is_alive())
                {
                    apply_self_cool_chams(character);
                }
            }

            if (globals::chams::hand_fresnel_enabled && character->is_alive())
            {
                apply_hand_fresnel(pawn, character);
            }

            if (g_local_chams_safe && globals::chams::self_3p_fresnel_enabled && character && IsValidUObject((uobject*)character) && character->is_alive())
            {
                apply_self_3p_fresnel(character);
            }

            if (g_local_chams_safe && globals::chams::fresnel_shi_enabled && character && IsValidUObject((uobject*)character) && character->is_alive())
            {
                apply_fresnel_shi(character);
            }

            // 1P hand wire — skip while third person (1P meshes not safe).
            if (g_local_chams_safe && character->is_alive() && globals::misc::HandWire && !globals::misc::tperson) {
                if (auto hand = character->GetOverlayMesh1P();
                    hand && memory::IsValidPointer((uintptr_t)hand))
                    seh_set_wireframe((uintptr_t)hand, true);
                if (auto mesh1p = character->getmesh1p();
                    mesh1p && memory::IsValidPointer((uintptr_t)mesh1p))
                    seh_set_wireframe((uintptr_t)mesh1p, true);
            }

            if (g_local_chams_safe && globals::misc::handglow && character->is_alive()) {
                if (auto handmesh = character->GetOverlayMesh1P())
                    meshp1_material(pawn, character);
            }

            if (g_local_chams_safe && globals::misc::handchams && character->is_alive()) {
                if (auto handmesh = character->GetOverlayMesh1P())
                    meshp1_material1(pawn, character);
            }

            if (g_local_chams_safe && globals::chams::hand_outline_enabled && character->is_alive())
                apply_hand_outline_chams(pawn);

            if (g_mesh_ops_safe && globals::misc::WireframeGun && character->is_alive()) {
                auto wg_inv = character->get_inventory();
                if (wg_inv && memory::IsValidPointer((uintptr_t)wg_inv)) {
                    auto get_weapon = wg_inv->get_current_equippable();
                    if (get_weapon && memory::IsValidPointer((uintptr_t)get_weapon)) {
                        auto weapon_mesh_1p = get_weapon->GetMesh1P();
                        seh_set_wireframe((uintptr_t)weapon_mesh_1p, true);
                    }
                }
            }

            static uintptr_t    last_applied_weapon_ptr = 0;
            static bool         is_processing_model = false;
            static std::wstring last_processed_weapon_type = L"";
            static uintptr_t    last_character_ptr = 0;
            static int          model_load_counter = 0;

            static uintptr_t saved_vandal_ptr = 0;
            static uintptr_t saved_phantom_ptr = 0;
            static uintptr_t saved_ghost_ptr = 0;
            static uintptr_t saved_frenzy_ptr = 0;
            static uintptr_t saved_melee_ptr = 0;

            static int vandal_cycle_index = 0;
            static int phantom_cycle_index = 0;
            static int ghost_cycle_index = 0;
            static int frenzy_cycle_index = 0;

            if (UWorldSave != LastWorldPtr ||
                (character && (uintptr_t)character != last_character_ptr))
            {
                TextureCache.clear();
                LastWorldPtr = UWorldSave;
                last_applied_weapon_ptr = 0;
                is_processing_model = false;
                last_processed_weapon_type = L"";
                saved_vandal_ptr = 0;
                saved_phantom_ptr = 0;
                saved_ghost_ptr = 0;
                saved_frenzy_ptr = 0;
                saved_melee_ptr = 0;
                if (character) last_character_ptr = (uintptr_t)character;
                model_load_counter = 0;
            }

            if (globals::misc::custom_vandal_enabled && character)
            {
                if (character && (uintptr_t)character != last_character_ptr)
                {
                    TextureCache.clear();
                    last_applied_weapon_ptr = 0;
                    last_processed_weapon_type = L"";
                    is_processing_model = false;
                    saved_vandal_ptr = 0;
                    saved_phantom_ptr = 0;
                    saved_ghost_ptr = 0;
                    saved_frenzy_ptr = 0;
                    saved_melee_ptr = 0;
                    last_character_ptr = (uintptr_t)character;
                    model_load_counter = 0;
                }

                if (!character || !controllers || !character->is_alive()
                    || !memory::IsValidPointer((uintptr_t)character) || !IsValidUObject((uobject*)character))
                {
                    last_applied_weapon_ptr = 0;
                    last_processed_weapon_type = L"";
                    is_processing_model = false;
                    last_character_ptr = 0;
                    goto skip_mesh;
                }

                {
                    auto inv = character->get_inventory();
                    if (!inv || !memory::IsValidPointer((uintptr_t)inv) || !IsValidUObject((uobject*)inv))
                    {
                        last_applied_weapon_ptr = 0;
                        last_processed_weapon_type = L"";
                        is_processing_model = false;
                        last_character_ptr = (uintptr_t)character;
                        goto skip_mesh;
                    }

                    auto weapon = inv->get_current_equippable();
                    if (weapon && !is_processing_model)
                    {
                        if (!memory::IsValidPointer((uintptr_t)weapon) ||
                            (uintptr_t)weapon < 0x10000 || (uintptr_t)weapon > 0x7FFFFFFFFFFF ||
                            !IsValidUObject((uobject*)weapon))
                        {
                            last_applied_weapon_ptr = 0;
                            last_processed_weapon_type = L"";
                            is_processing_model = false;
                            last_character_ptr = (uintptr_t)character;
                            goto skip_mesh;
                        }

                        fstring      obj_name = system::get_object_name(weapon);
                        fstring      converted_name = helper::convert_weapon_name(obj_name);
                        std::wstring wName = converted_name.wide();
                        uintptr_t    wPtr = (uintptr_t)weapon;

                        bool should_process = false;

                        if ((uintptr_t)weapon != last_applied_weapon_ptr)
                        {
                            if (last_applied_weapon_ptr != 0)
                                model_load_counter = 0;

                            if (wName == L"Vandal")
                            {
                                if (saved_vandal_ptr != 0 && saved_vandal_ptr != wPtr)
                                    vandal_cycle_index = (vandal_cycle_index + 1) % 7;
                                saved_vandal_ptr = wPtr;
                            }
                            else if (wName == L"Phantom")
                            {
                                if (saved_phantom_ptr != 0 && saved_phantom_ptr != wPtr)
                                    phantom_cycle_index = (phantom_cycle_index + 1) % 3;
                                saved_phantom_ptr = wPtr;
                            }
                            else if (wName == L"Ghost")
                            {
                                if (saved_ghost_ptr != 0 && saved_ghost_ptr != wPtr)
                                    ghost_cycle_index = (ghost_cycle_index + 1) % 1;
                                saved_ghost_ptr = wPtr;
                            }
                            else if (wName == L"Frenzy")
                            {
                                if (saved_frenzy_ptr != 0 && saved_frenzy_ptr != wPtr)
                                    frenzy_cycle_index = (frenzy_cycle_index + 1) % 1;
                                saved_frenzy_ptr = wPtr;
                            }
                            else if (wName == L"Melee")
                            {
                                saved_melee_ptr = wPtr;
                            }

                            should_process = true;
                        }
                        else if (wName != last_processed_weapon_type)
                            should_process = true;

                        if (should_process)
                        {
                            last_applied_weapon_ptr = wPtr;
                            last_processed_weapon_type = wName;
                            last_character_ptr = (uintptr_t)character;
                            is_processing_model = true;

                            std::string  b = GetPublicPath();
                            std::wstring wb(b.begin(), b.end());

                            try
                            {
                                if (wName == L"Vandal")
                                {
                                    int s = vandal_cycle_index;
                                    if (s == 0)      ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal.obj").c_str(), (wb + L"vandal_tex.png").c_str());
                                    else if (s == 1) ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal_skin1.obj").c_str(), (wb + L"vandal_skin1_tex.png").c_str());
                                    else if (s == 2) ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal_skin3.obj").c_str(), (wb + L"vandal_skin3_tex.png").c_str());
                                    else if (s == 3) ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal_skin4.obj").c_str(), (wb + L"vandal_skin4_tex.png").c_str());
                                    else if (s == 4) ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal_skin5.obj").c_str(), (wb + L"vandal_skin5_tex.png").c_str());
                                    else if (s == 5) ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal_skin6.obj").c_str(), (wb + L"vandal_skin6_tex.png").c_str());
                                    else             ReplaceWeaponMeshWith3DModel(weapon, (b + "vandal.obj").c_str(), (wb + L"vandal_tex.png").c_str());
                                }
                                else if (wName == L"Phantom")
                                {
                                    int s = phantom_cycle_index;
                                    if (s == 0)      ReplaceWeaponMeshWith3DModel(weapon, (b + "phantom_skin1.obj").c_str(), (wb + L"phantom_skin1_tex.png").c_str());
                                    else if (s == 1) ReplaceWeaponMeshWith3DModel(weapon, (b + "phantom_skin2.obj").c_str(), (wb + L"phantom_skin2_tex.png").c_str());
                                    else             ReplaceWeaponMeshWith3DModel(weapon, (b + "phantom_skin3.obj").c_str(), (wb + L"phantom_skin3_tex.png").c_str());
                                }
                                else if (wName == L"Ghost")   ReplaceWeaponMeshWith3DModel(weapon, (b + "ghost.obj").c_str(), (wb + L"ghost.png").c_str());
                                else if (wName == L"Frenzy")  ReplaceWeaponMeshWith3DModel(weapon, (b + "frenzy.obj").c_str(), (wb + L"frenzy_tex.png").c_str());
                                else if (wName == L"Melee")   ReplaceWeaponMeshWith3DModel(weapon, (b + "bicak.obj").c_str(), (wb + L"bicak_tex.png").c_str());
                                else if (wName == L"Bulldog") ReplaceWeaponMeshWith3DModel(weapon, (b + "bulldog.obj").c_str(), (wb + L"bulldog_tex.png").c_str());
                                else if (wName == L"Guardian")ReplaceWeaponMeshWith3DModel(weapon, (b + "guardian.obj").c_str(), (wb + L"guardian_tex.png").c_str());
                                else if (wName == L"Sheriff") ReplaceWeaponMeshWith3DModel(weapon, (b + "sheriff.obj").c_str(), (wb + L"sheriff_tex.png").c_str());
                                else if (wName == L"Spectre") ReplaceWeaponMeshWith3DModel(weapon, (b + "spectre.obj").c_str(), (wb + L"spectre.png").c_str());
                                else if (wName == L"Stinger") ReplaceWeaponMeshWith3DModel(weapon, (b + "stinger.obj").c_str(), (wb + L"stinger_tex.png").c_str());
                                else if (wName == L"Bucky")   ReplaceWeaponMeshWith3DModel(weapon, (b + "bucky.obj").c_str(), (wb + L"bucky_tex.png").c_str());
                                else if (wName == L"Judge")   ReplaceWeaponMeshWith3DModel(weapon, (b + "judge.obj").c_str(), (wb + L"judge_tex.png").c_str());
                                else if (wName == L"Marshal") ReplaceWeaponMeshWith3DModel(weapon, (b + "marshal.obj").c_str(), (wb + L"marshal_tex.png").c_str());
                                else if (wName == L"Operator")ReplaceWeaponMeshWith3DModel(weapon, (b + "operator.obj").c_str(), (wb + L"operator_tex.png").c_str());
                                else if (wName == L"Ares")    ReplaceWeaponMeshWith3DModel(weapon, (b + "ares.obj").c_str(), (wb + L"ares_tex.png").c_str());
                                else if (wName == L"Odin")    ReplaceWeaponMeshWith3DModel(weapon, (b + "odin.obj").c_str(), (wb + L"odin_tex.png").c_str());
                                else if (wName == L"Classic") ReplaceWeaponMeshWith3DModel(weapon, (b + "classic.obj").c_str(), (wb + L"classic_tex.png").c_str());
                                else if (wName == L"Shorty")  ReplaceWeaponMeshWith3DModel(weapon, (b + "shorty.obj").c_str(), (wb + L"shorty_tex.png").c_str());
                            }
                            catch (...)
                            {
                                last_applied_weapon_ptr = 0;
                                last_processed_weapon_type = L"";
                                is_processing_model = false;
                                last_character_ptr = (uintptr_t)character;
                            }

                            is_processing_model = false;
                        }
                    }
                }
            }
        skip_mesh:;

            if (globals::misc::custom_vandal_enabled && character && controllers) {
                auto inv = character->get_inventory();
                if (inv) {
                    auto weapon = inv->get_current_equippable();
                    if (weapon && memory::IsValidPointer((uintptr_t)weapon) && IsValidUObject((uobject*)weapon)) {
                        fstring      obj_name = system::get_object_name(weapon);
                        fstring      converted_name = helper::convert_weapon_name(obj_name);
                        std::wstring wName = converted_name.wide();

                        // ── 1P mesh transform update ──
                        USceneComponent* sceneComp = reinterpret_cast<USceneComponent*>(weapon->GetMesh1P());
                        if (sceneComp && memory::IsValidPointer((uintptr_t)sceneComp)) {
                            tarray<USceneComponent*> children = GetChildrenComponents(sceneComp, true);
                            for (int i = 0; i < children.size(); i++) {
                                if (!children[i] || !memory::IsValidPointer((uintptr_t)children[i])) continue;
                                fstring childName = system::get_object_name((uobject*)children[i]);
                                if (childName.to_str().find("ProceduralMesh") != std::string::npos) {
                                    auto* ProcMesh = (uskeletalmeshcomponent*)children[i];

                                    if (wName == L"Spectre") {
                                        seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(
                                            globals::misc::spectre_scale_x,
                                            globals::misc::spectre_scale_y,
                                            globals::misc::spectre_scale_z
                                        ));
                                        USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{
                                            globals::misc::spectre_rot_pitch,
                                            globals::misc::spectre_rot_yaw,
                                            globals::misc::spectre_rot_roll
                                            });
                                        USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(
                                            globals::misc::spectre_pos_x,
                                            globals::misc::spectre_pos_y,
                                            globals::misc::spectre_pos_z
                                        ));
                                    }
                                    else {
                                        seh_set_relative_scale_3d((uintptr_t)ProcMesh, fvector(1.5f, 1.5f, 1.5f));
                                        USceneComponentHelpers::SetRelativeRotation(ProcMesh, FRotator{ 0.f, 90.f, -90.f });
                                        USceneComponentHelpers::SetRelativeLocation(ProcMesh, fvector(-0.9434f, 0.943392f, -2.83019f));
                                    }
                                    break;
                                }
                            }
                        }

                        // ── 3P mesh transform update ──
                        USceneComponent* sceneComp3P = reinterpret_cast<USceneComponent*>(weapon->GetMesh3P());
                        if (sceneComp3P && memory::IsValidPointer((uintptr_t)sceneComp3P)) {
                            tarray<USceneComponent*> children3P = GetChildrenComponents(sceneComp3P, true);
                            for (int i = 0; i < children3P.size(); i++) {
                                if (!children3P[i] || !memory::IsValidPointer((uintptr_t)children3P[i])) continue;
                                fstring childName3P = system::get_object_name((uobject*)children3P[i]);
                                if (childName3P.to_str().find("ProceduralMesh") != std::string::npos) {
                                    auto* ProcMesh3P = (uskeletalmeshcomponent*)children3P[i];

                                    if (wName == L"Spectre") {
                                        seh_set_relative_scale_3d((uintptr_t)ProcMesh3P, fvector(
                                            globals::misc::spectre_scale_x,
                                            globals::misc::spectre_scale_y,
                                            globals::misc::spectre_scale_z
                                        ));
                                        USceneComponentHelpers::SetRelativeRotation(ProcMesh3P, FRotator{
                                            globals::misc::spectre_rot_pitch,
                                            globals::misc::spectre_rot_yaw,
                                            globals::misc::spectre_rot_roll
                                            });
                                        USceneComponentHelpers::SetRelativeLocation(ProcMesh3P, fvector(
                                            globals::misc::spectre_pos_x,
                                            globals::misc::spectre_pos_y,
                                            globals::misc::spectre_pos_z
                                        ));
                                    }
                                    else {
                                        seh_set_relative_scale_3d((uintptr_t)ProcMesh3P, fvector(1.5f, 1.5f, 1.5f));
                                        USceneComponentHelpers::SetRelativeRotation(ProcMesh3P, FRotator{ 0.f, 90.f, -90.f });
                                        USceneComponentHelpers::SetRelativeLocation(ProcMesh3P, fvector(-0.9434f, 0.943392f, -2.83019f));
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            static uintptr_t    lastProcessedTextWeapon = 0;
            static std::wstring lastProcessedTextType = L"";

            if (globals::misc::custom_text_enabled && character && controllers) {
                auto inv = character->get_inventory();
                if (inv) {
                    auto weapon = inv->get_current_equippable();
                    if (weapon) {
                        fstring obj_name = system::get_object_name(weapon);
                        fstring converted_name = helper::convert_weapon_name(obj_name);
                        std::wstring wName = converted_name.wide();

                        if (wName == L"Vandal" || wName == L"Frenzy" || wName == L"Ghost" ||
                            wName == L"Melee" || wName == L"Phantom" || wName == L"Spectre")
                        {
                            uintptr_t weaponPtr = (uintptr_t)weapon;

                            if (lastProcessedTextWeapon != 0 && lastProcessedTextWeapon != weaponPtr) {
                                if (WeaponTextMeshMap.count(lastProcessedTextWeapon)) {
                                    auto* oldMesh = WeaponTextMeshMap[lastProcessedTextWeapon];
                                    if (oldMesh && memory::IsValidPointer((uintptr_t)oldMesh)) {
                                        static uobject* DestroyFunc = uobject::find_object<uobject*>(L"Engine.Actor.DestroyComponent");
                                        if (DestroyFunc) {
                                            struct { UActorComponent* Component; } Args = { (UActorComponent*)oldMesh };

                                            ((uobject*)oldMesh)->process_event(DestroyFunc, &Args);
                                        }
                                        WeaponTextMeshMap.erase(lastProcessedTextWeapon);
                                    }
                                }
                            }

                            lastProcessedTextWeapon = weaponPtr;
                            lastProcessedTextType = wName;

                            if (WeaponTextMeshMap.count(weaponPtr) && WeaponTextMeshMap[weaponPtr]) {
                                TextMesh = WeaponTextMeshMap[weaponPtr];

                                if (memory::IsValidPointer((uintptr_t)TextMesh)) {
                                    uskeletalmeshcomponent* TextSkelMesh = (uskeletalmeshcomponent*)TextMesh;
                                    if (TextSkelMesh) {
                                        bool isVandalSkin4 = (wName == L"Vandal" && vandal_cycle_index == 3);
                                        if (isVandalSkin4) {
                                            seh_set_relative_scale_3d((uintptr_t)TextSkelMesh, fvector(
                                                globals::misc::text_scale_x,
                                                globals::misc::text_scale_y,
                                                globals::misc::text_scale_z
                                            ));
                                            USceneComponentHelpers::SetRelativeRotation(TextSkelMesh,
                                                FRotator{ 0.0f, 90.3396f, -88.9811f });
                                            USceneComponentHelpers::SetRelativeLocation(TextSkelMesh,
                                                fvector(0.000f, -1.333f, -2.667f));
                                        }
                                        else {
                                            WeaponTransform transform = GetTextTransform(wName, vandal_cycle_index);
                                            seh_set_relative_scale_3d((uintptr_t)TextSkelMesh, transform.scale);
                                            USceneComponentHelpers::SetRelativeRotation(TextSkelMesh,
                                                FRotator{
                                                    transform.rotation.pitch,
                                                    transform.rotation.yaw,
                                                    transform.rotation.roll
                                                });
                                            USceneComponentHelpers::SetRelativeLocation(TextSkelMesh, transform.position);
                                        }
                                    }
                                    static uobject* SetVisFunc = uobject::find_object<uobject*>(L"Engine.SceneComponent.SetVisibility");
                                    if (SetVisFunc) {
                                        struct { bool bNewVisibility; bool bPropagateToChildren; } VisArgs = { true, true };
                                        ((uobject*)TextMesh)->process_event(SetVisFunc, &VisArgs);
                                    }
                                }
                                else {
                                    WeaponTextMeshMap.erase(weaponPtr);
                                    ReplaceTextMeshWith3DModel(weapon, (GetPublicPath() + "text.obj").c_str());
                                }
                            }
                            else {
                                ReplaceTextMeshWith3DModel(weapon, (GetPublicPath() + "text.obj").c_str());
                            }
                        }
                    }
                }
            }







            if (globals::misc::pulse_trace)
            {
                aplayercameramanager* camera = controllers ? controllers->get_camera_manager() : nullptr;
                BulletBeamTrace::ProcessBulletTracers(camera, controllers, character, local_weapon, local_weapon_name);
            }


            if (g_mesh_ops_safe && globals::misc::antiflash && character->is_alive())
            {
                auto test1 = memory::read<UBlindManagerComponent*>((uintptr_t)character + offsets::BlindManagerComponent);
                use_blind_manager_component(test1);
            }

            if (g_local_chams_safe && globals::visuals::gunmaterial1p && character->is_alive())
            {
                auto gm1p_inv = character->get_inventory();
                if (gm1p_inv && memory::IsValidPointer((uintptr_t)gm1p_inv)) {
                    auto gm1p_weapon = gm1p_inv->get_current_equippable();
                    if (gm1p_weapon && memory::IsValidPointer((uintptr_t)gm1p_weapon)) {
                        auto weapon_mesh_1p = gm1p_weapon->GetMesh1P();
                        if (weapon_mesh_1p && memory::IsValidPointer((uintptr_t)weapon_mesh_1p)) {
                            uobject* Material = nullptr;
                            switch (globals::visuals::typegun1p) {
                            case 0: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat").decrypt()); break;
                            case 1: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Red_MI.Arcade_Emissive_Red_MI").decrypt()); break;
                            case 2: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI").decrypt()); break;
                            case 3: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Yellow_MI.Arcade_Emissive_Yellow_MI").decrypt()); break;
                            }
                            if (Material && memory::IsValidPointer((uintptr_t)Material))
                                seh_SetMaterial((uintptr_t)weapon_mesh_1p, 0, Material);
                        }
                    }
                }
            }

            if (g_local_chams_safe && globals::visuals::gunmaterial3p && character->is_alive())
            {
                auto gm3p_inv = character->get_inventory();
                if (gm3p_inv && memory::IsValidPointer((uintptr_t)gm3p_inv)) {
                    auto gm3p_weapon = gm3p_inv->get_current_equippable();
                    if (gm3p_weapon && memory::IsValidPointer((uintptr_t)gm3p_weapon)) {
                        auto weapon_mesh_3p = gm3p_weapon->GetMesh3P();
                        if (weapon_mesh_3p && memory::IsValidPointer((uintptr_t)weapon_mesh_3p)) {
                            uobject* Material = nullptr;
                            switch (globals::visuals::typegun3d) {
                            case 0: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat").decrypt()); break;
                            case 1: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Red_MI.Arcade_Emissive_Red_MI").decrypt()); break;
                            case 2: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI").decrypt()); break;
                            case 3: Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Yellow_MI.Arcade_Emissive_Yellow_MI").decrypt()); break;
                            }
                            if (Material && memory::IsValidPointer((uintptr_t)Material))
                                seh_SetMaterial((uintptr_t)weapon_mesh_3p, 0, Material);
                        }
                    }
                }
            }

            if (g_mesh_ops_safe && globals::visuals::hand_with_material && character->is_alive()) {
                uskeletalmeshcomponent* mesh_first_person = memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p);
                if (!mesh_first_person || !memory::IsValidPointer((uintptr_t)mesh_first_person)) {
                    goto skip_hand_material;
                }

                uskeletalmeshcomponent* mesh_overlay_first_person = memory::read<uskeletalmeshcomponent*>(uintptr_t(pawn) + offsets::mesh1p_overlay);
                if (!mesh_overlay_first_person || !memory::IsValidPointer((uintptr_t)mesh_overlay_first_person)) {
                    goto skip_hand_material;
                }

                uobject* Material = nullptr;

                switch (globals::visuals::typehand) {
                case 0:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/3P_Weapon_Translucent_Mat.3P_Weapon_Translucent_Mat").decrypt());
                    break;
                case 1:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Characters/Vampire/S0/VFX/Ability_X/1P_Vampire_Tattoo_X_S0_MI_VFX.1P_Vampire_Tattoo_X_S0_MI_VFX").decrypt());
                    break;
                case 2:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Gems/1P_Gem_MAT.1P_Gem_MAT").decrypt());
                    break;
                case 3:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat").decrypt());
                    break;
                case 4:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Sakura/Tritium_Sakura_3P_MI.Tritium_Sakura_3P_MI").decrypt());
                    break;
                case 5:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Yellow_MI.Arcade_Emissive_Yellow_MI").decrypt());
                    break;
                case 6:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Red_MI.Arcade_Emissive_Red_MI").decrypt());
                    break;
                case 7:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Arcade/Arcade_Emissive_Blue_MI.Arcade_Emissive_Blue_MI").decrypt());
                    break;
                case 8:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/SpecialMaterials/Afterglow3/BakedLight/Afterglow3_BakedLight_MI.Afterglow3_BakedLight_MI").decrypt());
                    break;
                default:
                    Material = uobject::StaticLoadObject(nullptr, nullptr, crypt(L"/Game/Equippables/_Core/Materials/1P_Weapon_Glass_Mat.1P_Weapon_Glass_Mat").decrypt());
                    break;
                }

                if (Material && memory::IsValidPointer((uintptr_t)Material)) {
                    seh_SetMaterial((uintptr_t)mesh_first_person, 0, Material);
                    seh_SetMaterial((uintptr_t)mesh_overlay_first_person, 0, Material);
                }
            }
        skip_hand_material:;

            if (globals::misc::customgun && character->is_alive() && UWorldSave)
            {
                auto gun_inv = character->get_inventory();
                if (!gun_inv || !memory::IsValidPointer((uintptr_t)gun_inv)) goto skip_customgun;
                {
                    currentequippable* Equippable = gun_inv->get_current_equippable();
                    if (!Equippable || !memory::IsValidPointer((uintptr_t)Equippable)) goto skip_customgun;

                    UPrimitiveComponent* GunMesh = memory::read<UPrimitiveComponent*>(uintptr_t(Equippable) + offsets::mesh1pgun);
                    if (!GunMesh || !memory::IsValidPointer((uintptr_t)GunMesh)) goto skip_customgun;

                    fstring customTexturePath = fstring(L"C:/gun.jpg");
                    uobject* CustomTexture = system::import_file_as_texture2d(UWorldSave, customTexturePath);
                    if (!CustomTexture || !memory::IsValidPointer((uintptr_t)CustomTexture)) goto skip_customgun;

                    auto matPath = L"/Game/Equippables/_Core/Materials/SpecialMaterials/CosmosShader/Winter/Winter_MI.Winter_MI";
                    uobject* material = uobject::find_object<uobject*>(matPath);
                    if (!material)
                        material = uobject::static_load_object(nullptr, nullptr, matPath);

                    if (!material || !memory::IsValidPointer((uintptr_t)material)) goto skip_customgun;

                    seh_set_material((uintptr_t)GunMesh, 0, material);
                    uobject* GunDynamicMat = seh_create_dynamic_mat((uintptr_t)GunMesh, 0, material);
                    if (!GunDynamicMat || !memory::IsValidPointer((uintptr_t)GunDynamicMat)) goto skip_customgun;

                    auto* mat = GunDynamicMat->cast<UMaterialInstanceDynamic>();
                    if (!mat) goto skip_customgun;

                    fname param1 = string::string_to_name(crypt(L"Image 1").decrypt());
                    fname param2 = string::string_to_name(crypt(L"Image 2").decrypt());

                    mat->set_texture_parameter_value(param1, CustomTexture);
                    mat->set_texture_parameter_value(param2, CustomTexture);

                    globals::misc::customgun = false;
                }
            skip_customgun:;
            }





            {
                static uintptr_t lastSkyWorld = 0;
                static DWORD lastSkyRevalidate = 0;
                DWORD now_sky = GetTickCount();
                if ((uintptr_t)world != lastSkyWorld || (now_sky - lastSkyRevalidate) > 2500) {
                    CachedSkyDome = nullptr;
                    SkyDomeCached = false;
                    SkyDome = nullptr;
                    lastSkyWorld = (uintptr_t)world;
                    lastSkyRevalidate = now_sky;
                }
            }

            if (g_mesh_ops_safe && (globals::misc::skybox || globals::misc::nightmode))
            {
                if (!SkyDomeCached || !CachedSkyDome || !IsValidUObject((uobject*)CachedSkyDome))
                {
                    CachedSkyDome = nullptr;
                    SkyDomeCached = false;

                    tarray<AGameObject*> Objects;
                    GameplayStatics::GetAllActorsOfClass2(world, Class::Actors(), &Objects);

                    for (int i = 0; i < Objects.Num(); i++)
                    {
                        if (!Objects.IsValidIndex(i)) continue;

                        AGameObject* Object = Objects[i];
                        if (!Object || !IsValidUObject((uobject*)Object)) continue;

                        auto name = system::get_object_name((uobject*)Object);
                        if (!name.is_valid()) continue;

                        if (name.to_str() == "shared_SkyDomeB_0")
                        {
                            CachedSkyDome = Object;
                            SkyDomeCached = true;
                            break;
                        }
                    }
                }

                if (CachedSkyDome && IsValidUObject((uobject*)CachedSkyDome))
                {
                    SkyDome = CachedSkyDome;
                    SkyBoxMesh();
                }
            }
            else if (g_mesh_ops_safe && SkyDomeCached && CachedSkyDome && IsValidUObject((uobject*)CachedSkyDome))
            {
                SkyDome = CachedSkyDome;
                SkyBoxMesh();
            }

            if (globals::misc::Fog)
            {
                static AGameObject* CachedFogActor = nullptr;
                static uintptr_t lastFogWorld = 0;
                static DWORD lastFogRevalidate = 0;

                DWORD now_fog = GetTickCount();
                if ((uintptr_t)world != lastFogWorld || (now_fog - lastFogRevalidate) > 2500) {
                    CachedFogActor = nullptr;
                    lastFogWorld = (uintptr_t)world;
                    lastFogRevalidate = now_fog;
                }

                if (!CachedFogActor || !IsValidUObject((uobject*)CachedFogActor))
                {
                    CachedFogActor = nullptr;

                    tarray<AGameObject*> Objects;
                    GameplayStatics::GetAllActorsOfClass2(world, GetExponentialHeightFogClass(), &Objects);
                    int32_t count = Objects.Num();
                    if (count > 0 && count < 10000)
                    {
                        for (int32_t i = 0; i < count; i++)
                        {
                            if (!Objects.IsValidIndex(i)) continue;
                            AGameObject* obj = Objects[i];
                            if (!obj || !IsValidUObject((uobject*)obj)) continue;
                            CachedFogActor = obj;
                            break;
                        }
                    }
                }

                if (CachedFogActor && IsValidUObject((uobject*)CachedFogActor))
                {
                    UExponentialHeightFogComponent* fogComponent =
                        memory::read<UExponentialHeightFogComponent*>((uintptr_t)CachedFogActor + 0x0460);
                    if (fogComponent && IsValidUObject((uobject*)fogComponent))
                    {
                        if (globals::misc::FogRGB)
                        {
                            static float fogRainbowTime = 0.0f;
                            fogRainbowTime += 0.004f;
                            flinearcolor rainbow = GetRainbowColor(fogRainbowTime);
                            globals::misc::FogColor = flinearcolor{ rainbow.r, rainbow.g, rainbow.b, 1.0f };
                        }

                        fogComponent->SetFogDensity(globals::misc::FogDensity);
                        fogComponent->SetFogHeightFalloff(globals::misc::FogHeightFalloff);
                        fogComponent->SetFogInscatteringColor(globals::misc::FogColor);
                        fogComponent->SetFogMaxOpacity(globals::misc::FogMaxOpacity);
                        fogComponent->SetStartDistance(globals::misc::FogStartDistance);
                        fogComponent->SetFogCutoffDistance(globals::misc::FogCutoffDistance);
                        fogComponent->SetVolumetricFog(globals::misc::bEnableVolumetricFog);
                        fogComponent->SetVolumetricFogDistance(globals::misc::VolumetricFogDistance);
                    }
                }
            }
        skip_fog:;



            if (g_mesh_ops_safe && (globals::misc::BigGun || globals::misc::BigGun3D_enabled) && character->is_alive()) {
                auto bg_inv = character->get_inventory();
                if (bg_inv && memory::IsValidPointer((uintptr_t)bg_inv))
                if (auto get_weapon = bg_inv->get_current_equippable())
                if (memory::IsValidPointer((uintptr_t)get_weapon))
                {
                    if (globals::misc::BigGun) {
                        fvector scale = fvector(globals::misc::BigGunFloat, globals::misc::BigGunFloat, globals::misc::BigGunFloat);
                        if (auto weapon_mesh_1p = get_weapon->GetMesh1P())
                            seh_set_relative_scale_3d((uintptr_t)weapon_mesh_1p, scale);
                        if (auto weapon_mesh_3p = get_weapon->GetMesh3P())
                            seh_set_relative_scale_3d((uintptr_t)weapon_mesh_3p, scale);
                    }
                    if (globals::misc::BigGun3D_enabled) {
                        fvector scale_3p = fvector(globals::misc::BigGun3DFloat, globals::misc::BigGun3DFloat, globals::misc::BigGun3DFloat);
                        if (auto weapon_mesh_3p = get_weapon->GetMesh3P())
                            seh_set_relative_scale_3d((uintptr_t)weapon_mesh_3p, scale_3p);
                    }
                }
            }

            if (g_mesh_ops_safe && globals::misc::BigSelf) {
                if (auto main_mesh_3p = character->mesh3p()) {
                    fvector new_scale = fvector(globals::misc::BigSelfFloat, globals::misc::BigSelfFloat, globals::misc::BigSelfFloat);
                    seh_set_relative_scale_3d((uintptr_t)main_mesh_3p, new_scale);
                }
            }

            if (g_mesh_ops_safe && globals::misc::BigGun3DWireframe && character->is_alive()) {
                auto bw_inv = character->get_inventory();
                if (bw_inv && memory::IsValidPointer((uintptr_t)bw_inv)) {
                    if (auto get_weapon = bw_inv->get_current_equippable()) {
                        if (memory::IsValidPointer((uintptr_t)get_weapon)) {
                            if (auto weapon_mesh_3p = get_weapon->GetMesh3P()) {
                                if (memory::IsValidPointer((uintptr_t)weapon_mesh_3p)) {
                                    fvector new_scale = fvector(globals::misc::BigGunFloat, globals::misc::BigGunFloat, globals::misc::BigGunFloat);
                                    seh_set_relative_scale_3d((uintptr_t)weapon_mesh_3p, new_scale);
                                    seh_set_wireframe((uintptr_t)weapon_mesh_3p, true);
                                }
                            }
                        }
                    }
                }
            }

            if (globals::misc::bullet_spawn && character->is_alive())
            {
                static DWORD last_shell_tick = 0;
                static DWORD last_fired_time = 0; // Son ateş etme zamanını tutar

                bool is_shooting_now = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0 || globals::stop_for_auto_shoot;

                if (is_shooting_now)
                    last_fired_time = GetTickCount();

                bool should_eject = is_shooting_now || (GetTickCount() - last_fired_time < 5000);

                if (should_eject && GetTickCount() - last_shell_tick > 50) // 50ms
                {
                    auto bs_inv = character->get_inventory();
                    if (bs_inv && memory::IsValidPointer((uintptr_t)bs_inv))
                    {
                        auto get_weapon = bs_inv->get_current_equippable();
                        if (get_weapon && memory::IsValidPointer((uintptr_t)get_weapon))
                        {
                            int spawn_amount = globals::misc::bullet_spawn_amount > 0 ? globals::misc::bullet_spawn_amount : 1;
                            for (int i = 0; i < spawn_amount; i++) {
                                seh_eject_shell_casing((uintptr_t)get_weapon);
                            }
                            last_shell_tick = GetTickCount();
                        }
                    }
                }
            }

            if (globals::misc::gun_3p_wireframe && character && character->is_alive())
            {
                auto gw_inv = character->get_inventory();
                if (gw_inv && memory::IsValidPointer((uintptr_t)gw_inv)) {
                    if (auto weapon = gw_inv->get_current_equippable()) {
                        if (memory::IsValidPointer((uintptr_t)weapon)) {
                            if (auto gun_mesh_3p = weapon->GetMesh3P()) {
                                seh_set_wireframe((uintptr_t)gun_mesh_3p, true);
                            }
                        }
                    }
                }
            }



            if (globals::misc::bhop && !burat::menu_open) {
                fkey Space;
                Space = fkey{ fname { string_utils::string_to_name(crypt(L"SpaceBar").decrypt())} };

                if (character->CanJump()) {
                    if ((GetAsyncKeyState)(VK_SPACE) & 0x8000) {
                        controllers->SimulateInputKey(Space, true);
                        (Sleep)(10);
                        controllers->SimulateInputKey(Space, false);
                    }
                }
            }



            // ── Hellfire Mode ──────────────────────────────────────
            if (globals::misc::hellfiremode)
            {
                if (!world || !IsValidUObject((uobject*)world)) {
                    ResetHellFire();
                    goto skip_hellfire;
                }

                if (!character || !IsValidUObject((uobject*)character)) {
                    ResetHellFire();
                    goto skip_hellfire;
                }

                if (!character->is_alive()) {
                    if (g_HellFireAttached) ResetHellFire();
                    goto skip_hellfire;
                }

                static uintptr_t last_hf_world_ptr = 0;
                static uintptr_t last_hf_char_ptr = 0;
                static DWORD last_hf_revalidate = 0;
                DWORD now_hf = GetTickCount();

                if ((uintptr_t)world != last_hf_world_ptr ||
                    (uintptr_t)character != last_hf_char_ptr ||
                    (now_hf - last_hf_revalidate) > 5000)
                {
                    ResetHellFire();
                    last_hf_world_ptr = (uintptr_t)world;
                    last_hf_char_ptr = (uintptr_t)character;
                    last_hf_revalidate = now_hf;
                }

                if (g_LastHellFireCharacter != character)
                    ResetHellFire();

                AttachHellFireToPlayerFollowing(character);
            }
            else
            {
                if (g_HellFireAttached)
                    ResetHellFire();
            }
        skip_hellfire:;

            // ── Lightning Mode ────────────────────────────────────────
            if (character && character->is_alive())
            {
                HandleLightning(character, world);
            }

            // ── Apocalypse Mode ──────────────────────────────────────
            if (character && character->is_alive())
            {
                HandleApocalypseMode(character, world);
            }

            // ── Rotating Text ────────────────────────────────────────
            if (g_mesh_ops_safe && character && character->is_alive())
            {
                HandleAuraOrbit(character);
            }

            int safe_actor_count = actors.count;
            if (safe_actor_count < 0 || safe_actor_count > 200) safe_actor_count = 0;
            if (safe_actor_count > 0 && !memory::IsValidPointer((uintptr_t)actors.data)) safe_actor_count = 0;

            if (globals::aimbot::enable_360_fov && safe_actor_count > 0)
            {
                float fov_closest = FLT_MAX;
                target_id = -1;

                for (int32_t idx = 0; idx < safe_actor_count; ++idx) {
                    ashootercharacter* actor = actors[idx];
                    if (!actor || !memory::IsValidPointer((uintptr_t)actor)) continue;
                    if (actor == character || !seh_bot_actor_ready(actor)) continue;
                    if (actor->health() == 0) continue;
                    if (!controllers->dormant_server(actor)) continue;
                    if (basecomponent::is_ally(actor, character)) continue;

                    bool is_visible = false;
                    if (!globals::aimbot::v1sh_ch3ck) {
                        is_visible = true;
                    }
                    else if (globals::aimbot::v1sh_ch3ck_3p && globals::misc::tperson) {
                        auto* local_mesh_360 = character->get_mesh();
                        if (local_mesh_360 && memory::IsValidPointer((uintptr_t)local_mesh_360)) {
                            fvector head_pos = local_mesh_360->get_bone_location(8);
                            is_visible = controllers->line_of_sight_from_position(actor, head_pos);
                        } else {
                            is_visible = controllers->line_of_sight(actor);
                        }
                    }
                    else {
                        is_visible = controllers->line_of_sight(actor);
                    }
                    
                    bool can_autowall_360 = false;
                    if (!is_visible)
                    {
                        const int ue_autowall = ResolveAutowallUeBone(controllers, character, actor, is_visible);
                        if (globals::aimbot::auto_wall)
                            can_autowall_360 = TraceHelper::CanShootThrough(controllers, character, actor, ue_autowall);
                    }
                    if (!is_visible && !can_autowall_360) continue;

                    fvector actor_pos = actor->k2_get_actor_location();
                    fvector local_pos = character->k2_get_actor_location();
                    float dist = (actor_pos - local_pos).size();

                    // 360 FOV: convert screen FOV to world distance (FOV * 100 for proper scaling)
                    float max_360_distance = globals::aimbot::a1m_f0v * 100.0f;
                    if (dist < max_360_distance && dist < fov_closest) {
                        fov_closest = dist;
                        target_id = idx;
                        target_actor = actor;
                    }
                }
            }


            // Damage tracker static variables (shared between tracking and rendering)
            static struct DmgEvent {
                float damage;
                float timeCreated;
                fvector worldPos;
                flinearcolor color;
                bool isVisible;
            } dmg_events[100][4];
            static int dmg_event_idx[100] = {};
            static float dmg_last_total_hp[100] = {};
            static uintptr_t dmg_actor_slot[100] = {};

            bool hasTarget = false;

            for (int32_t idx = 0; idx < safe_actor_count; ++idx)
            {
                ashootercharacter* actor = actors[idx];
                if (!actor || actor == character) continue;
                if (!memory::IsValidPointer((uintptr_t)actor)) continue;
                if (!IsValidUObject((uobject*)actor)) continue;
                if (basecomponent::is_ally(actor, character)) continue;

                // Filter out projectiles (Jett blades, etc.) to prevent crash
                auto actor_name = system::get_object_name((uobject*)actor);
                if (string_utils::contains(actor_name, L"Projectile") || 
                    string_utils::contains(actor_name, L"Blade") ||
                    string_utils::contains(actor_name, L"Grenade")) continue;

                if (g_mesh_ops_safe && globals::misc::finisher && actor->is_alive() && memory::IsValidPointer((uintptr_t)actor)) {
                    static shadow_vmt death_hook;
                    death_hook.hook<decltype(oHkDeath)>(
                        memory::module_base,
                        (uintptr_t)actor,
                        0x159,
                        (void*)hk_death,
                        &oHkDeath
                    );
                }

                uskeletalmeshcomponent* mesh = seh_bot_get_mesh(actor);
                if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) continue;
                if (!seh_bot_actor_ready(actor)) continue;

                if (!pawn) continue;

                if (globals::misc::backtrack && actor->is_alive()) {
                    BacktrackVis::PushEnemy(actor, mesh, actor->k2_get_actor_location());
                }

                fvector head_location = mesh->get_bone_location(8);
                if (!head_location.is_valid()) continue;

                fvector root_location = mesh->get_bone_location(0);
                if (!root_location.is_valid()) continue;


                bool bVisible_local = false;
                if (!globals::aimbot::v1sh_ch3ck) {
                    bVisible_local = true;
                }
                else if (globals::aimbot::v1sh_ch3ck_3p && globals::misc::tperson) {
                    auto* local_mesh_vis = character->get_mesh();
                    if (local_mesh_vis && memory::IsValidPointer((uintptr_t)local_mesh_vis)) {
                        fvector head_pos = local_mesh_vis->get_bone_location(8);
                        bVisible_local = controllers->line_of_sight_from_position(actor, head_pos);
                    } else {
                        bVisible_local = controllers->line_of_sight(actor);
                    }
                }
                else {
                    bVisible_local = controllers->line_of_sight(actor);
                }
                
                bool CanAutoWall_local = false;
                if (!bVisible_local)
                {
                    const int ue_autowall_local = ResolveAutowallUeBone(controllers, character, actor, bVisible_local);
                    if (globals::aimbot::auto_wall)
                        CanAutoWall_local = TraceHelper::CanShootThrough(controllers, character, actor, ue_autowall_local);
                }

                bool visible_check_local = bVisible_local || CanAutoWall_local;





                auto head_location_2d = controllers->project_world_to_screen(head_location);
                auto head_location_long_2d = controllers->project_world_to_screen({ head_location.x, head_location.y, head_location.z + 15 });
                auto root_location_2d = controllers->project_world_to_screen(root_location);
                auto head_long_out = controllers->project_world_to_screen({ head_location.x - 10.0, head_location.y, head_location.z + 75 });

                if (!root_location_2d.is_valid() || !head_location_2d.is_valid() || !head_location_long_2d.is_valid() || !head_long_out.is_valid())
                    continue;

                auto relative_location = actor->k2_get_actor_location();
                auto my_shooter_relative_location = character->k2_get_actor_location();
                auto distance = my_shooter_relative_location.distance(relative_location);
                if (distance <= 0) continue;

                auto [box_width, box_height] = calculate_box_dimensions(head_location_long_2d, root_location_2d);
                if (box_width <= 0 || box_height <= 0) continue;

                double x = head_location_long_2d.x - (box_width / 2), y = head_location_long_2d.y;
                double lineW = (box_width / 7);
                double lineH = (box_height / 7);

                flinearcolor bobbercol;
                flinearcolor boxcolor = defines::Invisible_ESPColor;
                flinearcolor snapcolor = defines::InvisibleSnapColor;
                flinearcolor skeletoncolor = defines::InvisibleSkeletonColor;
                flinearcolor ChamsColor = defines::Invisible;

                if (!controllers->dormant_server(actor)) continue;

                if (globals::visuals::vischeck) {
                    if (controllers->line_of_sight(actor)) {
                        boxcolor = defines::VisibleBox_ESPColor;
                        snapcolor = defines::VisibleSnapColor;
                        skeletoncolor = defines::VisibleSkeletonColor;
                    }
                    else
                    {
                        boxcolor = defines::Invisible_ESPColor;
                        snapcolor = defines::InvisibleSnapColor;
                        skeletoncolor = defines::InvisibleSkeletonColor;
                    }
                }

                if (globals::visuals::h3althbar) {
                    float health = (actor->is_alive() ? actor->health() : 0);
                    float shield = (actor->is_alive() ? actor->shield() : 0);

                    defines::health_color = health >= 75 ? defines::high_health :
                        (health <= 74 && health >= 44) ? defines::normal_health : defines::low_heath;

                    if (health <= 100)
                        drawings::draw_health_and_shield(health, shield, root_location_2d, box_width, box_height, distance, defines::health_color, canvas);
                }

                if (globals::misc::damage_tracker) {
                    uintptr_t actor_ptr = (uintptr_t)actor;
                    int slot = -1;
                    for (int s = 0; s < 100; s++) {
                        if (dmg_actor_slot[s] == actor_ptr) {
                            slot = s;
                            break;
                        }
                    }
                    if (slot == -1) {
                        for (int s = 0; s < 100; s++) {
                            if (dmg_actor_slot[s] == 0) {
                                slot = s;
                                dmg_actor_slot[s] = actor_ptr;
                                float current_health = actor->health();
                                float current_shield = actor->shield();
                                dmg_last_total_hp[s] = current_health + current_shield;
                                dmg_event_idx[s] = 0;
                                for (int d = 0; d < 4; d++) dmg_events[s][d] = { 0.f, 0.f, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f, 1.f}, false };
                                break;
                            }
                        }
                    }

                    if (slot != -1) {
                        float current_health = actor->is_alive() ? actor->health() : 0.0f;
                        float current_shield = actor->is_alive() ? actor->shield() : 0.0f;
                        float total_hp = current_health + current_shield;
                        
                        if (dmg_last_total_hp[slot] > total_hp && dmg_last_total_hp[slot] > 0.f) {
                            float dmg = dmg_last_total_hp[slot] - total_hp;
                            if (dmg > 0.1f && dmg <= 250.f) {
                                float t = GetTickCount64() / 1000.0f;
                                int di = dmg_event_idx[slot] % 4;
                                
                                static flinearcolor damage_colors[] = {
                                    { 1.0f, 0.2f, 0.2f, 1.0f },
                                    { 1.0f, 0.5f, 0.0f, 1.0f },
                                    { 1.0f, 1.0f, 0.0f, 1.0f },
                                    { 0.0f, 1.0f, 0.5f, 1.0f },
                                    { 0.3f, 0.7f, 1.0f, 1.0f },
                                    { 1.0f, 0.0f, 1.0f, 1.0f },
                                    { 1.0f, 0.4f, 0.7f, 1.0f },
                                    { 0.5f, 1.0f, 0.5f, 1.0f }
                                };
                                int color_idx = (int)(t * 1000) % 8;
                                
                                dmg_events[slot][di] = { dmg, t, head_location, damage_colors[color_idx], bVisible_local };
                                dmg_event_idx[slot]++;
                            }
                        }
                        
                        dmg_last_total_hp[slot] = total_hp;
                    }
                }
                
                if (!actor->is_alive()) continue;

                if (globals::visuals::headb0x) {
                    fvector2d position = { head_location_long_2d.x, head_location_long_2d.y };
                    drawings::head_box(position, lineW, lineH, skeletoncolor, canvas);
                }

                if (globals::visuals::sk3let0n) {
                    drawings::draw_skeleton(controllers, mesh, memory::read<int32_t>((uintptr_t)mesh + offsets::bone_cout), skeletoncolor, canvas);
                }

                if (globals::visuals::b00ms) {
                    fvector2d position1 = { root_location_2d.x - 10, root_location_2d.y + 28 };
                    drawings::agent_icon(actor, position1, distance, canvas);
                }

                if (globals::visuals::chinahat)
                {
                    drawings::partyhat(controllers, mesh, canvas);
                }






                flinearcolor BoxColor = defines::Invisible_ESPColor;

                if (globals::visuals::box2d && actor->is_alive()) {
                    if (globals::visuals::vischeck) {
                        if (controllers->line_of_sight(actor)) {

                            DrawAdaptiveBoundingBox(canvas, controllers, mesh, defines::VisibleBox_ESPColor);
                        }
                        else {

                            DrawAdaptiveBoundingBox(canvas, controllers, mesh, defines::Invisible_ESPColor);
                        }
                    }
                    else {

                        DrawAdaptiveBoundingBox(canvas, controllers, mesh, defines::visuals_color);
                    }
                }

                if (globals::visuals::corner && actor->is_alive()) {
                    if (globals::visuals::vischeck) {
                        if (controllers->line_of_sight(actor)) {

                            DrawAdaptiveCornerBox(canvas, controllers, mesh, defines::VisibleBox_ESPColor, ESPThickness);
                        }
                        else {

                            DrawAdaptiveCornerBox(canvas, controllers, mesh, defines::Invisible_ESPColor, ESPThickness);
                        }
                    }
                    else {

                        DrawAdaptiveCornerBox(canvas, controllers, mesh, defines::Invisible_ESPColor, ESPThickness);
                    }
                }

                if (globals::visuals::box2d5 && actor->is_alive())
                {
                    if (globals::visuals::vischeck)
                    {
                        if (controllers->line_of_sight(actor))
                        {
                            DrawGraham3D(canvas, controllers, mesh, defines::VisibleBox_ESPColor);
                        }
                        else
                        {
                            DrawGraham3D(canvas, controllers, mesh, defines::Invisible_ESPColor);
                        }
                    }
                    else
                    {
                        DrawGraham3D(canvas, controllers, mesh, defines::visuals_color);
                    }
                }

                if (globals::visuals::box3d)
                {
                    fvector origin = actor->k2_get_actor_location();
                    fvector extent = fvector(40.f, 40.f, 100.f);
                    bool isVisible = controllers->line_of_sight(actor);

                    if (globals::visuals::vischeck)
                    {
                        if (isVisible)
                        {
                            Draw3DBox(canvas, controllers, origin, extent, defines::VisibleBox_ESPColor);
                        }
                        else
                        {
                            Draw3DBox(canvas, controllers, origin, extent, defines::Invisible_ESPColor);
                        }
                    }
                    else
                    {
                        Draw3DBox(canvas, controllers, origin, extent, defines::visuals_color);
                    }
                }

                if (globals::visuals::filled_box && actor->is_alive()) {
                    flinearcolor fb_color = defines::visuals_color;
                    if (globals::visuals::vischeck && controllers->line_of_sight(actor))
                        fb_color = defines::VisibleBox_ESPColor;
                    else if (globals::visuals::vischeck)
                        fb_color = defines::Invisible_ESPColor;

                    fvector2d h2d = head_location_long_2d;
                    fvector2d r2d = root_location_2d;
                    float ht = fabsf((float)(r2d.y - h2d.y));
                    float wd = ht * 0.55f;
                    fvector2d fb_tl = { h2d.x - wd * 0.5, h2d.y };
                    fvector2d fb_br = { h2d.x + wd * 0.5, r2d.y };
                    DrawFilledBox(canvas, fb_tl, fb_br, fb_color, globals::visuals::filled_box_alpha);
                }

                if (globals::visuals::box_health && actor->is_alive()) {
                    flinearcolor bh_color = defines::visuals_color;
                    if (globals::visuals::vischeck && controllers->line_of_sight(actor))
                        bh_color = defines::VisibleBox_ESPColor;
                    else if (globals::visuals::vischeck)
                        bh_color = defines::Invisible_ESPColor;
                    DrawBoxHealth(canvas, controllers, mesh, actor, bh_color);
                }

                if ((globals::visuals::outline_esp || globals::visuals::mosca_glow_chams) && actor->is_alive()) {
                    flinearcolor m_color = MoscaOutlinePickColor(controllers, character, actor);
                    DrawOutlineESP(canvas, controllers, mesh, m_color, ESPThickness, actor);
                }

                if (globals::visuals::mosca_gun_glow && actor->is_alive()) {
                    uinventory* inv = actor->get_inventory();
                    currentequippable* eq = nullptr;
                    if (inv && memory::IsValidPointer((uintptr_t)inv))
                        eq = inv->get_current_equippable();
                    uskeletalmeshcomponent* weapon_mesh = nullptr;
                    if (eq && memory::IsValidPointer((uintptr_t)eq))
                        weapon_mesh = eq->GetMesh3P();
                    if (weapon_mesh && memory::IsValidPointer((uintptr_t)weapon_mesh)) {
                        flinearcolor wg_color = MoscaOutlinePickColor(controllers, character, actor);
                        DrawOutlineWeaponGlowESP(canvas, controllers, weapon_mesh, wg_color, ESPThickness);
                    }
                }

                if (g_mesh_ops_safe && globals::misc::Wireframe && !globals::chams::black_chams_enemy)
                {
                    seh_set_wireframe((uintptr_t)mesh, true);
                    if (auto* cos = actor->GetCosmeticMesh3P();
                        cos && memory::IsValidPointer((uintptr_t)cos))
                        seh_set_wireframe((uintptr_t)cos, true);
                }

                if (g_mesh_ops_safe && seh_bot_actor_ready(actor) && actor->is_alive() && actor->health() > 0
                    && IsValidUObject((uobject*)actor)) {
                    // Enemy-only chams: never apply to self or allies.
                    const bool is_self = (character && actor == character);
                    const bool is_ally = (!is_self && character) ? basecomponent::is_ally(actor, character) : false;
                    if (!is_self && !is_ally) {
                        if (globals::chams::outline_enabled) {
                            apply_outline_chams(pawn, actor, controllers);
                        }
                        if (globals::chams::enemy_galaxy_enabled) {
                            apply_galaxy_chams(pawn, actor, controllers);
                        }
                        if (globals::visuals::crystal_chams_enemy) {
                            apply_crystal_enemy_chams(pawn, actor, controllers);
                        }
                        if (globals::chams::nebula_galaxy_enemy) {
                            galaxychams_v2_enemy(actor);
                        }
                        if (globals::chams::cool_chams_enabled) {
                            apply_cool_chams(pawn, actor, controllers);
                        }
                        if (g_mesh_ops_safe && (globals::chams::sentinel_chams_visible_enemy || globals::chams::sentinel_chams_invisible_enemy)) {
                            seh_apply_sentinel_enemy_chams(pawn, actor, controllers);
                        }
                        if (globals::chams::black_chams_enemy) {
                            // Always strip enemy wire bit — wire + chams together causes AV / invisible chams
                            seh_set_wireframe((uintptr_t)mesh, false);
                            if (auto* cos = seh_bot_get_cosmetic_mesh(actor))
                                seh_set_wireframe((uintptr_t)cos, false);
                            seh_apply_black_chams_enemy(pawn, actor, controllers);
                        }
                        if (globals::chams::custom_fresnel) {
                            apply_enemy_fresnel(pawn, actor, controllers);
                        }
                        if (globals::chams::fresnel_shi_enabled) {
                            apply_fresnel_shi(actor);
                        }
                    }
                }

                /* if (globals::visuals::crystal_chams_enabled && character && character->is_alive()) {

                     ugameinstance* gameinstance = world->game_instance();
                     if (gameinstance) {
                         apply_crystal_chams_to_self(character, gameinstance);
                     }
                 }*/






                if (globals::visuals::b11ms)
                {
                    fvector Origin, Extend;
                    fvector2d rel2d, footPos;
                    fvector2d position2 = { head_location_long_2d.x + 5.0f, head_location_long_2d.y };

                    auto RelativeLocation = actor->k2_get_actor_location();

                    if (controllers->project_world_location_to_screen({ RelativeLocation.x, RelativeLocation.y, RelativeLocation.z + (Extend.z / 2) }, footPos, 0))
                    {
                        if (controllers->project_world_location_to_screen(actor->k2_get_actor_location(), rel2d, true))
                        {
                            auto enemy_inv = static_cast<ashootercharacter*>(actor)->get_inventory();
                            currentequippable* CurrentWeapon = nullptr;
                            if (enemy_inv && memory::IsValidPointer((uintptr_t)enemy_inv)) {
                                CurrentWeapon = enemy_inv->get_current_equippable();
                            }
                            if (CurrentWeapon != nullptr && memory::IsValidPointer((uintptr_t)CurrentWeapon))
                            {
                                auto enemy_weapon_name = helper::convert_weapon_name(system::get_object_name(CurrentWeapon));
                                std::wstring enemy_wname = enemy_weapon_name.wide();
                                if (enemy_wname.size() > 0)
                                {
                                    const wchar_t* weaponNameWChar = enemy_wname.c_str();
                                    float adjustedY = footPos.y - (30 * 1.7);

                                    fvector2d position1 = { head_location_long_2d.x + 5.0f, head_location_long_2d.y + 25.f };


                                    draw_text(canvas, menu::font, weaponNameWChar, maincolor, { footPos.x, adjustedY });
                                }
                            }
                        }
                    }

                    if (globals::visuals::speed_esp) {
                        fvector vel = actor->get_velocity();
                        float current_speed = sqrt(vel.x * vel.x + vel.y * vel.y);
                        int display_speed = (int)current_speed;

                        fvector root_pos = actor->k2_get_actor_location();
                        fvector2d speed_pos;
                        if (controllers->project_world_location_to_screen({ root_pos.x, root_pos.y, root_pos.z + 55.0f }, speed_pos, false)) {
                            wchar_t speed_buf[64];
                            swprintf_s(speed_buf, L"%d u/s", display_speed);
                            flinearcolor speedColor = { 1.0f, 0.8f, 0.0f, 1.0f };
                            draw_text_rgb_string(canvas, menu::font, speed_buf, speed_pos.x + 40.f, speed_pos.y, speedColor, 1);
                        }
                    }
                }





                if (globals::aimbot::resolver && globals::aimbot::a1mbot && actor->is_alive()) {
                    DWORD now_r = GetTickCount64();
                    if (now_r - last_resolver_cleanup > 10000) {
                        resolver_instances.clear();
                        last_resolver_cleanup = now_r;
                    }

                    uintptr_t target_key = (uintptr_t)actor;
                    if (resolver_instances.find(target_key) == resolver_instances.end())
                        resolver_instances[target_key] = std::make_unique<UltimateValorantResolver>(actor, character);

                    UltimateValorantResolver& resolver = *resolver_instances[target_key];
                    fvector enemy_vel = actor->get_velocity();
                    fvector head_bone = actor->get_mesh() ? actor->get_mesh()->get_bone_location(8) : fvector(0,0,0);
                    fvector resolved = resolver.ResolveHeadPosition(head_bone, enemy_vel);
                    resolver.UpdateVisualizationData(head_bone, resolved);

                    ResolverVisualizer::DrawModernResolverInfo(canvas, controllers, actor, resolver.GetResolverData());
                }

                if (globals::aimbot::silent && !globals::aimbot::auto_shot && target_id != -1 && target_id < safe_actor_count) {
                    ashootercharacter* actor = actors[target_id];
                    if (!actor || !memory::IsValidPointer((uintptr_t)actor) || actor == character) continue;

                    uskeletalmeshcomponent* mesh = actor->get_mesh();
                    if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) continue;

                    bool silent_target_visible = false;
                    if (!globals::aimbot::v1sh_ch3ck) {
                        silent_target_visible = true;
                    }
                    else if (globals::aimbot::v1sh_ch3ck_3p && globals::misc::tperson) {
                        auto* local_mesh_silent = character->get_mesh();
                        if (local_mesh_silent && memory::IsValidPointer((uintptr_t)local_mesh_silent)) {
                            fvector head_pos = local_mesh_silent->get_bone_location(8);
                            silent_target_visible = controllers->line_of_sight_from_position(actor, head_pos);
                        } else {
                            silent_target_visible = controllers->line_of_sight(actor);
                        }
                    }
                    else {
                        silent_target_visible = controllers->line_of_sight(actor);
                    }

                    const int ue_autowall = ResolveAutowallUeBone(controllers, character, actor, silent_target_visible);

                    if (!silent_target_visible) {
                        if (globals::aimbot::auto_wall &&
                            TraceHelper::CanShootThrough(controllers, character, actor, ue_autowall))
                        {
                            silent_target_visible = true;
                        }
                    }

                    fvector Target = globals::aimbot::closest_bone
                        ? GetTargetWorldFromAutowallUeBone(mesh, ue_autowall)
                        : (silent_target_visible
                            ? get_target_bone_matrix(mesh, globals::aimbot::a1m_b0ne)
                            : GetTargetWorldFromAutowallUeBone(mesh, ue_autowall));

                    if (!Target.is_valid()) continue;

                    if (globals::aimbot::prediction) {
                        // Silent Aim has 0 smoothing delay because the rotation instantly sets to the target.
                        // For hitscan, adding velocity prediction here will cause misses due to over-leading. 
                        // The server's lag compensation will register the hit exactly where we currently see it.
                    }

                    fvector2d head_screen;
                    if (controllers->project_world_location_to_screen(Target, head_screen, false) && head_screen.is_valid()) {

                        bool hasTarget = false;

                        auto ProcessSilentWeapon = [&](fstring obj_name) {

                            if (globals::aimbot::silent && second_locked_camera && !hasTarget && !burat::menu_open &&
                                GetAsyncKeyState(globals::aimbot::a1m_k3y) &&
                                is_target_in_fov(screen_center_x, screen_center_y, head_screen) &&
                                (globals::aimbot::v1sh_ch3ck && silent_target_visible || !globals::aimbot::v1sh_ch3ck))
                            {
                                uintptr_t cmanager = *(uintptr_t*)((uintptr_t)controllers + offsets::cameramaneger);
                                if (!cmanager || !memory::IsValidPointer(cmanager)) return;
                                fvector CameraPos = globals::misc::tperson
                                    ? MetrixTargeting::GetAimOrigin(controllers, character)
                                    : *(fvector*)(cmanager + offsets::camerapos);
                                fvector CameraRot = *(fvector*)(cmanager + offsets::camerarot);
                                fvector DeltaRotation;

                                fvector ConvertRotation = {
                                    CameraRot.x < 0.0 ? 360.0 + CameraRot.x : CameraRot.x,
                                    CameraRot.y < 0.0 ? 360.0 + CameraRot.y : CameraRot.y,
                                    0.0
                                };

                                fvector ControlRotation = controllers->get_control_rotation();

                                fvector Delta = {
                                    CameraPos.x - Target.x,
                                    CameraPos.y - Target.y,
                                    CameraPos.z - Target.z
                                };

                                double hyp = sqrt(Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z);

                                const double PI_PRECISE = 3.1415926535897932384626433832795028841971693993751;

                                fvector Rotation = {
                                    acos(Delta.z / hyp) * (180.0 / PI_PRECISE),
                                    atan2(Delta.y, Delta.x) * (180.0 / PI_PRECISE),
                                    0.0
                                };

                                Rotation.x += 270.0;

                                if (Delta.x >= 0.0) Rotation.y += 180.0;

                                if (Rotation.y < 0.0) Rotation.y += 360.0;

                                DeltaRotation.x = fmod(ConvertRotation.x - ControlRotation.x, 360.0);
                                DeltaRotation.y = fmod(ConvertRotation.y - ControlRotation.y, 360.0);

                                ConvertRotation.x = fmod(Rotation.x - DeltaRotation.x - DeltaRotation.x, 360.0);
                                ConvertRotation.y = fmod(Rotation.y - DeltaRotation.y - DeltaRotation.y, 360.0);

                                if (ConvertRotation.x < 0.0) ConvertRotation.x = 360.0 + ConvertRotation.x;
                                if (ConvertRotation.y < 0.0) ConvertRotation.y = 360.0 + ConvertRotation.y;

                                if (globals::aimbot::nospread) {
                                    fvector direction = RotationToVector(ConvertRotation);

                                    auto current_inv = character->get_inventory();
                                    if (current_inv) {
                                        auto current_equip = current_inv->get_current_equippable();
                                        if (current_equip && memory::IsValidPointer((uintptr_t)current_equip)) {
                                            auto firing_state = current_equip->get_firing_state();
                                            if (firing_state && memory::IsValidPointer((uintptr_t)firing_state)) {
                                                fvector spread_angle = calc_spread(character, (uintptr_t)firing_state, current_equip, direction);

                                                if (spread_angle.size() > 0.001 && spread_angle.is_valid()) {
                                                    ConvertRotation = ConvertRotation - spread_angle;

                                                    ConvertRotation.x = fmod(ConvertRotation.x + 360.0, 360.0);
                                                    ConvertRotation.y = fmod(ConvertRotation.y + 360.0, 360.0);

                                                    if (ConvertRotation.x < 0.0) ConvertRotation.x = 360.0 + ConvertRotation.x;
                                                    if (ConvertRotation.y < 0.0) ConvertRotation.y = 360.0 + ConvertRotation.y;
                                                }
                                            }
                                        }
                                    }
                                }

                                if (ConvertRotation.is_valid()) {
                                    controllers->set_control_rotation(ConvertRotation);
                                    hasTarget = true;
                                }
                            }
                            };

                        if (globals::aimbot::nospread && myweapon) {
                            fstring obj_name = helper::convert_weapon_name(system::get_object_name(myweapon));
                            std::wstring name = obj_name.wide();

                            if (name == L"Bulldog" || name == L"Phantom" || name == L"Vandal" ||
                                name == L"Operator" || name == L"Marshal" || name == L"Sheriff" ||
                                name == L"Spectre" || name == L"Outlaw" || name == L"Classic" ||
                                name == L"Shorty" || name == L"Frenzy" || name == L"Ghost" ||
                                name == L"Stinger" || name == L"Bucky" || name == L"Judge" ||
                                name == L"Guardian" || name == L"Ares" || name == L"Odin") {

                                ProcessSilentWeapon(obj_name);
                            }
                            else {
                                if (globals::aimbot::silent && second_locked_camera && !hasTarget && !burat::menu_open &&
                                    GetAsyncKeyState(globals::aimbot::a1m_k3y) &&
                                    is_target_in_fov(screen_center_x, screen_center_y, head_screen) &&
                                    (globals::aimbot::v1sh_ch3ck && silent_target_visible || !globals::aimbot::v1sh_ch3ck))
                                {
                                    uintptr_t cmanager = *(uintptr_t*)((uintptr_t)controllers + offsets::cameramaneger);
                                    if (!cmanager || !memory::IsValidPointer(cmanager)) continue;
                                    fvector CameraPos = globals::misc::tperson
                                        ? MetrixTargeting::GetAimOrigin(controllers, character)
                                        : *(fvector*)(cmanager + offsets::camerapos);
                                    fvector CameraRot = *(fvector*)(cmanager + offsets::camerarot);
                                    fvector DeltaRotation;

                                    fvector ConvertRotation = {
                                        CameraRot.x < 0.0 ? 360.0 + CameraRot.x : CameraRot.x,
                                        CameraRot.y < 0.0 ? 360.0 + CameraRot.y : CameraRot.y,
                                        0.0
                                    };

                                    fvector ControlRotation = controllers->get_control_rotation();

                                    fvector Delta = {
                                        CameraPos.x - Target.x,
                                        CameraPos.y - Target.y,
                                        CameraPos.z - Target.z
                                    };

                                    double hyp = sqrt(Delta.x * Delta.x + Delta.y * Delta.y + Delta.z * Delta.z);

                                    const double PI_PRECISE = 3.1415926535897932384626433832795028841971693993751;

                                    fvector Rotation = {
                                        acos(Delta.z / hyp) * (180.0 / PI_PRECISE),
                                        atan2(Delta.y, Delta.x) * (180.0 / PI_PRECISE),
                                        0.0
                                    };

                                    Rotation.x += 270.0;

                                    if (Delta.x >= 0.0) Rotation.y += 180.0;
                                    if (Rotation.y < 0.0) Rotation.y += 360.0;

                                    DeltaRotation.x = fmod(ConvertRotation.x - ControlRotation.x, 360.0);
                                    DeltaRotation.y = fmod(ConvertRotation.y - ControlRotation.y, 360.0);

                                    ConvertRotation.x = fmod(Rotation.x - DeltaRotation.x - DeltaRotation.x, 360.0);
                                    ConvertRotation.y = fmod(Rotation.y - DeltaRotation.y - DeltaRotation.y, 360.0);

                                    if (ConvertRotation.x < 0.0) ConvertRotation.x = 360.0 + ConvertRotation.x;
                                    if (ConvertRotation.y < 0.0) ConvertRotation.y = 360.0 + ConvertRotation.y;

                                    if (ConvertRotation.is_valid()) {
                                        controllers->set_control_rotation(ConvertRotation);
                                        hasTarget = true;
                                    }
                                }
                            }
                        }
                    }
                }





                if (g_mesh_ops_safe && actor->is_alive() && actor->health() > 0) {
                    bool visible_check_ch = controllers->line_of_sight(actor);

                    if (globals::chams::rchamsespall && visible_check_ch) {
                        seh_set_ares_outline_mode((uintptr_t)mesh, 4, true);
                        seh_set_ares_outline_mode((uintptr_t)mesh3p, 4, true);

                        ares_outline::setoutlinemode1(world,
                            { globals::chams::ChamsColor.r * globals::chams::Glow,
                              globals::chams::ChamsColor.g * globals::chams::Glow,
                              globals::chams::ChamsColor.b * globals::chams::Glow,
                              globals::chams::ChamsColor.a * globals::chams::Glow });
                    }
                    else if (globals::chams::rchamsesp) {
                        seh_set_ares_outline_mode((uintptr_t)mesh, 1, true);
                        seh_set_ares_outline_mode((uintptr_t)mesh3p, 1, true);

                        ares_outline::setoutlinemode1(world,
                            { globals::chams::ChamsColorvni.r * globals::chams::Glowvni,
                              globals::chams::ChamsColorvni.g * globals::chams::Glowvni,
                              globals::chams::ChamsColorvni.b * globals::chams::Glowvni,
                              globals::chams::ChamsColorvni.a * globals::chams::Glowvni });
                    }
                    else {
                        seh_set_ares_outline_mode((uintptr_t)mesh, 0, true);
                        seh_set_ares_outline_mode((uintptr_t)mesh3p, 0, true);
                    }
                }
                if (globals::visuals::dstc)
                {

                    wchar_t distance_text[256];

                    swprintf_s(distance_text, L"[ %.fm ]", distance);
                    fvector2d meow = { head_location_2d.x, head_location_2d.y - 45 };

                    draw_text(canvas, menu::font, distance_text, maincolor, meow);

                }




                if (globals::visuals::spike && cached_world_objects.count > 0) {

                    SPOOF_FUNC;
                    for (int32_t i = 0; i < cached_world_objects.count; i++) {
                        auto object = cached_world_objects[i];
                        if (!object || !IsValidUObject((uobject*)object)) continue;

                        fvector relative_location22 = object->k2_get_actor_location();

                        fvector2d rel_loc_w2s = controllers->project_world_to_screen(relative_location22);

                        auto name = system::get_object_name(object);

                        if (string_utils::contains(name, L"TimedBomb_C")) {

                            auto defuse_time = memory::read<float>(uintptr_t(object) + 0x5D0);
                            auto remaining_time = memory::read<float>(uintptr_t(object) + 0x5A8);

#define DefusePercentageMax 6.984602
#define CurrentDefuseSectionMax 2

                            float DefusePercentage = defuse_time * 100 / DefusePercentageMax;

                            std::string ep7 = crypt("Bomb (Remaining Time: ").decrypt() + std::to_string((int)remaining_time) + crypt(" Sec").decrypt();
                            std::string ep72 = crypt("Bomb (Defuse Progress: ").decrypt() + std::to_string((int)DefusePercentage) + crypt(" %)").decrypt();
                            std::wstring ep7_long(ep7.begin(), ep7.end());
                            std::wstring ep72_long(ep72.begin(), ep72.end());

                            draw_text_rgb_string(canvas, menu::font, ep7_long.c_str(), rel_loc_w2s.x, rel_loc_w2s.y, Name_Color, 1);
                            draw_text_rgb_string(canvas, menu::font, ep72_long.c_str(), rel_loc_w2s.x, rel_loc_w2s.y - 15, Name_Color, 1);
                        }
                    }
                }

                if (globals::visuals::snapl1ne) {
                    drawings::draw_snapline(globals::visuals::snapos, character, head_location_2d, snapcolor, canvas);
                }

                if (globals::aimbot::a1mbot) {
                    double delta_x = head_location_2d.x - screen_center_x;
                    double delta_y = head_location_2d.y - screen_center_y;

                    double distance = sqrt(delta_x * delta_x + delta_y * delta_y);
                    double screen_distance = math::distance_2d(head_location_2d, { screen_center_x, screen_center_y });

                    if (distance < closest_distance && screen_distance < globals::aimbot::a1m_f0v) {
                        if (visible_check_local) {
                            target_id = idx;
                            target_actor = actor;
                            closest_distance = screen_distance;
                        }
                    }
                }
            }
            
            // Finalize Aimbot Target tracker for tracers
            if (globals::aimbot::a1mbot && target_actor) {
                last_aim_target = target_actor;
            } else if (GetTickCount64() % 100 == 0) { 
                last_aim_target = nullptr;
            }


            // Render damage tracker numbers independently of actor existence
            if (globals::misc::damage_tracker && controllers) {
                float now = GetTickCount64() / 1000.0f;
                for (int s = 0; s < 100; s++) {
                    for (int d = 0; d < 4; d++) {
                        auto& ev = dmg_events[s][d];
                        float age = now - ev.timeCreated;
                        if (age < 2.5f && ev.damage > 0.f) {
                            fvector worldPosWithOffset = { ev.worldPos.x, ev.worldPos.y, ev.worldPos.z + 15.0f + (age * 40.0f) };
                            fvector2d currentScreenPos = controllers->project_world_to_screen(worldPosWithOffset);
                            
                            if (!currentScreenPos.is_valid()) continue;
                            
                            if (globals::misc::damage_tracker_vis_check && !ev.isVisible) continue;
                            
                            float alpha = 1.0f - (age / 2.5f);
                            wchar_t dmgText[32];
                            swprintf(dmgText, 32, L"-%.0f", ev.damage);
                            flinearcolor dmgColor = { ev.color.r, ev.color.g, ev.color.b, alpha };
                            flinearcolor outlineColor = { 0.0f, 0.0f, 0.0f, alpha };
                            canvas->k2_drawtext(menu::font, fstring(dmgText), currentScreenPos, { 1.5f, 1.5f }, dmgColor, 1.0f, outlineColor, { 0, 0 }, true, true, true, outlineColor);
                        }
                    }
                }
            }

            if (character && controllers) {
                const ULONGLONG since_inject = GetTickCount64() - s_inject_time;
                if (camera_engine == uintptr_t(camera) && should_hook_gay
                    && s_frame_counter >= 120 && since_inject >= 3000) {
                    static shadow_vmt camera_hook;

                    bool hook_success = camera_hook.hook<decltype(hooks::SetCameraCachePOVOriginal)>(
                        memory::module_base,
                        (uintptr_t)camera_engine,
                        0xf2,
                        (void*)hooks::SetCameraCachePOVHook,
                        &hooks::SetCameraCachePOVOriginal
                    );

                    if (hook_success) {
                        should_hook_gay = false;
                        LOG_INFO("Camera hook installed frame=%llu +%llums", s_frame_counter, since_inject);
                    }
                }



                static float current_health[100] = { 100.0f };
                static int sound_index = 0;
                static bool was_visible[100] = { false };
                static bool kill_message_sent[100] = { false };

                auto send_chat_message_all = [&](const std::string& message) -> bool {
                    try {
                        if (!world || message.empty()) {
                            return false;
                        }

                        // Mesaj uzunluğunu sınırla - çok uzun mesajlar crash yapıyor
                        std::string safe_message = message;
                        if (safe_message.size() > 200) {
                            safe_message.resize(200);
                        }

                        // Boş veya sadece boşluk olan mesajları engelle
                        bool has_content = false;
                        for (char c : safe_message) {
                            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                                has_content = true;
                                break;
                            }
                        }
                        if (!has_content) return false;

                        auto chat_manager = UThreadedChatManager::GetThreadedChatManager(world);
                        if (!chat_manager || !memory::IsValidPointer((uintptr_t)chat_manager)) {
                            return false;
                        }

                        // Güvenli string dönüşümü - MultiByteToWideChar kullan
                        int wide_len = MultiByteToWideChar(CP_UTF8, 0, safe_message.c_str(), (int)safe_message.size(), nullptr, 0);
                        if (wide_len <= 0 || wide_len > 256) {
                            return false;
                        }

                        std::wstring wide_message(wide_len, L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, safe_message.c_str(), (int)safe_message.size(), &wide_message[0], wide_len);

                        if (wide_message.empty()) {
                            return false;
                        }

                        fstring message_string = fstring(wide_message.c_str());
                        if (!message_string.c_str() || message_string.count <= 0) {
                            return false;
                        }

                        ftext message_text = text::string_to_text(message_string);
                        if (!message_text.data) {
                            return false;
                        }

                        chat_manager->send_chat_message_v2(EChatRoomType::All, message_text);
                        return true;
                    }
                    catch (...) {
                        return false;
                    }
                };

                const int tracked_actor_count = (safe_actor_count > 100) ? 100 : safe_actor_count;
                for (int i = 0; i < tracked_actor_count; i++) {
                    if (!memory::IsValidPointer((uintptr_t)actors.data)) break;
                    auto actor = actors[i];
                    if (actor && actor != character && memory::IsValidPointer((uintptr_t)actor)) {
                        float health = actor->health();
                        bool currently_visible = controllers->line_of_sight(actor);

                        // Detect kills regardless of visibility (works for both visible and autowall kills)
                        if (current_health[i] > 0 && health <= 0) {
                            bool should_trigger = true;
                            if (globals::misc::kill_visible_check && !currently_visible && !was_visible[i]) {
                                should_trigger = false;
                            }

                            if (should_trigger) {
                                if (globals::misc::killsound) {
                                    static const wchar_t* kill_sounds[] = {
                                        L"C:\\Sounds\\kill1.wav",
                                        L"C:\\Sounds\\kill2.wav",
                                        L"C:\\Sounds\\kill3.wav",
                                        L"C:\\Sounds\\kill4.wav",
                                        L"C:\\Sounds\\kill5.wav"
                                    };

                                    PlaySoundW(kill_sounds[sound_index], NULL, SND_FILENAME | SND_ASYNC);
                                    sound_index = (sound_index + 1) % 5;
                                }

                                if (globals::misc::killsays && !kill_message_sent[i]) {
                                    if (send_chat_message_all(globals::misc::chat_message)) {
                                        kill_message_sent[i] = true;
                                    }
                                }
                            }
                        }

                        if (health > 0) {
                            kill_message_sent[i] = false;
                        }

                        current_health[i] = health;
                        was_visible[i] = currently_visible;
                    }
                }


                static bool spam_key_was_down = false;
                const int spam_key = (globals::misc::spam_key != 0) ? globals::misc::spam_key : VK_F4;
                const bool spam_key_down = (GetAsyncKeyState(spam_key) & 0x8000) != 0;

                if (globals::misc::chat_spammer) {
                    if (spam_key_down && !spam_key_was_down) {
                        int burst_count = (globals::misc::spam_count > 0) ? globals::misc::spam_count : 1;
                        if (burst_count > 50) burst_count = 50;

                        for (int i = 0; i < burst_count; ++i) {
                            if (!send_chat_message_all(globals::misc::chat_message)) {
                                break;
                            }
                            if (i < burst_count - 1) {
                                Sleep(5);
                            }
                        }
                    }
                    spam_key_was_down = spam_key_down;
                }
                else {
                    spam_key_was_down = false;
                }






                if (target_id != -1 && target_id < safe_actor_count && globals::aimbot::a1mbot &&
                    safe_actor_count > 0 && memory::IsValidPointer((uintptr_t)actors.data))
                {

                    ashootercharacter* actor = actors[target_id];
                    if (!actor || !memory::IsValidPointer((uintptr_t)actor) || actor == character) continue;

                    uskeletalmeshcomponent* mesh = actor->get_mesh();
                    if (!mesh || !memory::IsValidPointer((uintptr_t)mesh)) continue;

                    if (actor->is_alive()) {

                        static const fkey lmb_key{ fname{ string::string_to_name(crypt(L"LeftMouseButton").decrypt()) } };
                        bool is_blade_storm = (local_weapon_name == L"Blade storm");

                        // Recompute visibility for the actual selected target (not stale from the loop)
                        bool target_bVisible = false;
                        if (!globals::aimbot::v1sh_ch3ck) {
                            target_bVisible = true;
                        }
                        else if (globals::aimbot::v1sh_ch3ck_3p && globals::misc::tperson) {
                            auto* local_mesh_aim = character->get_mesh();
                            if (local_mesh_aim && memory::IsValidPointer((uintptr_t)local_mesh_aim)) {
                                fvector head_pos = local_mesh_aim->get_bone_location(8);
                                target_bVisible = controllers->line_of_sight_from_position(actor, head_pos);
                            } else {
                                target_bVisible = controllers->line_of_sight(actor);
                            }
                        }
                        else {
                            target_bVisible = controllers->line_of_sight(actor);
                        }

                        const int ue_autowall = ResolveAutowallUeBone(controllers, character, actor, target_bVisible);

                        fvector2d head_screen;
                        fvector target = globals::aimbot::closest_bone
                            ? GetTargetWorldFromAutowallUeBone(mesh, ue_autowall)
                            : (target_bVisible
                                ? get_target_bone_matrix(mesh, globals::aimbot::a1m_b0ne)
                                : GetTargetWorldFromAutowallUeBone(mesh, ue_autowall));
                        fvector spread_angle;

                        if (!target.is_valid()) continue;

                        if (globals::aimbot::resolver && target.is_valid()) {
                            DWORD now_rc = GetTickCount64();
                            if (now_rc - last_resolver_cleanup > 10000) { resolver_instances.clear(); last_resolver_cleanup = now_rc; }
                            uintptr_t rk = (uintptr_t)actor;
                            if (resolver_instances.find(rk) == resolver_instances.end())
                                resolver_instances[rk] = std::make_unique<UltimateValorantResolver>(actor, character);
                            fvector rv = resolver_instances[rk]->ResolveHeadPosition(target, actor->get_velocity());
                            resolver_instances[rk]->UpdateVisualizationData(target, rv);
                            target = rv;
                        }

                        if (globals::aimbot::prediction) {
                            fvector velocity = actor->get_velocity();
                            
                            // Auto-Resolver: The only delay factor for hitscan is the aimbot's own smoothing.
                            // We use actual frame time for prediction to remain consistent across different FPS.
                            float travel_time = (smooth_fps > 5.0f) ? (globals::aimbot::a1m_sm00th / smooth_fps) : 0.05f; 
                            
                            target.x += (velocity.x * travel_time);
                            target.y += (velocity.y * travel_time);
                            target.z += (velocity.z * travel_time);
                        }

                        bool aim_key_pressed = GetAsyncKeyState(globals::aimbot::a1m_k3y);
                        
                        bool target_can_auto_wall = false;
                        if (globals::aimbot::auto_wall && !target_bVisible && !is_blade_storm)
                        {
                            target_can_auto_wall = TraceHelper::CanShootThrough(controllers, character, actor, ue_autowall);
                        }

                        bool visible_check = target_bVisible || target_can_auto_wall;
                        static int hvh_autoshot_last_target = -1;
                        static bool hvh_autoshot_fired_this_peek = false;
                        static bool hvh_onetap_reset_spread = false;
                        if (globals::aimbot::auto_shot && globals::aimbot::auto_shot_delay_mode == 3) {
                            if (target_id != hvh_autoshot_last_target) {
                                hvh_autoshot_last_target = target_id;
                                hvh_autoshot_fired_this_peek = false;
                            }
                            if (!visible_check) {
                                hvh_autoshot_fired_this_peek = false;
                                hvh_onetap_reset_spread = true;
                            }
                        }
                        bool can_shoot = (!globals::aimbot::v1sh_ch3ck || visible_check);
                        bool is_valid_weapon = (
                            local_weapon_name == L"Bulldog" || local_weapon_name == L"Phantom" || local_weapon_name == L"Vandal" ||
                            local_weapon_name == L"Operator" || local_weapon_name == L"Marshal" || local_weapon_name == L"Sheriff" ||
                            local_weapon_name == L"Spectre" || local_weapon_name == L"Outlaw" || local_weapon_name == L"Classic" ||
                            local_weapon_name == L"Shorty" || local_weapon_name == L"Frenzy" || local_weapon_name == L"Ghost" ||
                            local_weapon_name == L"Stinger" || local_weapon_name == L"Bucky" || local_weapon_name == L"Judge" ||
                            local_weapon_name == L"Guardian" || local_weapon_name == L"Ares" || local_weapon_name == L"Odin" ||
                            is_blade_storm
                        );

                        if (aim_key_pressed && can_shoot && is_valid_weapon) {

                            fvector CameraPos;
                            if (globals::misc::tperson) {
                                auto* local_mesh_cam = character->get_mesh();
                                CameraPos = (local_mesh_cam && memory::IsValidPointer((uintptr_t)local_mesh_cam))
                                    ? local_mesh_cam->get_bone_location(8)
                                    : camera->get_camera_location();
                            } else {
                                CameraPos = camera->get_camera_location();
                            }
                            fvector ControlRotation = controllers->get_control_rotation();
                            fvector vector_pos = target - CameraPos;
                            double distance = vector_pos.size();

                            if (distance <= 0) continue;

                            double normalized_z = vector_pos.z / distance;
                            normalized_z = max(-1.0, min(1.0, normalized_z));

                            double x = -(acos(normalized_z) * (180.0 / M_PI) - 90.0);
                            double y = atan2(vector_pos.y, vector_pos.x) * (180.0 / M_PI);

                            fvector target_rotation(x, y, 0.0);
                            fvector new_aim_rotation;

                            if (globals::aimbot::reco1l_contr0l) {
                                fvector recoil = camera->get_camera_rotation() - ControlRotation;
                                
                                fvector normalizedRecoil = recoil;
                                while (normalizedRecoil.y > 180.0f) normalizedRecoil.y -= 360.0f;
                                while (normalizedRecoil.y < -180.0f) normalizedRecoil.y += 360.0f;
                                while (normalizedRecoil.x > 180.0f) normalizedRecoil.x -= 360.0f;
                                while (normalizedRecoil.x < -180.0f) normalizedRecoil.x += 360.0f;

                                new_aim_rotation.x = x - normalizedRecoil.x;
                                new_aim_rotation.y = y - normalizedRecoil.y;
                                new_aim_rotation.z = 0;
                            }
                            else {
                                new_aim_rotation = target_rotation;
                            }

                            double safe_smooth = globals::aimbot::a1m_sm00th;
                            if (safe_smooth < 1.0) safe_smooth = 1.0;
                            fvector new_rotation = smooth(new_aim_rotation, ControlRotation, safe_smooth);
                            new_rotation.x = fmod(new_rotation.x + 360.0, 360.0);
                            new_rotation.y = fmod(new_rotation.y + 360.0, 360.0);

                            if (globals::aimbot::nospread && !is_blade_storm && character->is_alive()) {
                                auto current_inv = character->get_inventory();
                                if (current_inv) {
                                    auto current_equip = current_inv->get_current_equippable();
                                    if (current_equip && memory::IsValidPointer((uintptr_t)current_equip)) {
                                        auto firing_state_comp = memory::read<uint64_t>(uintptr_t(current_equip) + offsets::FiringStateComp);
                                        if (firing_state_comp && memory::IsValidPointer(firing_state_comp)) {
                                            fvector spread_v = calc_spread(character, (uintptr_t)firing_state_comp, current_equip, new_rotation);
                                            if (!spread_v.is_null() && spread_v.is_valid()) {
                                                new_rotation = new_rotation - spread_v;
                                            }
                                        }
                                    }
                                }
                            }

                            if (!new_rotation.is_valid()) continue;
                            controllers->set_control_rotation(new_rotation);
                        }


                        bool auto_shot_active = globals::aimbot::auto_shot || globals::aimbot::AutoShoot;
                        if (globals::aimbot::auto_shot_hold_key) {
                            auto_shot_active = auto_shot_active && aim_key_pressed;
                        }

                        if (globals::aimbot::nospread && auto_shot_active && visible_check && is_valid_weapon) {
                            if (!actor || !memory::IsValidPointer((uintptr_t)actor) || !actor->is_alive()) continue;
                            if (!character || !memory::IsValidPointer((uintptr_t)character) || !character->is_alive()) continue;
                            fvector CameraPos = fvector(0, 0, 0);
                            if (is_blade_storm) {
                                if (globals::misc::tperson) {
                                    auto* local_mesh_bs = character->get_mesh();
                                    CameraPos = (local_mesh_bs && memory::IsValidPointer((uintptr_t)local_mesh_bs))
                                        ? seh_get_bone(local_mesh_bs, 8)
                                        : camera->get_camera_location();
                                } else {
                                    CameraPos = camera->get_camera_location();
                                }
                            }
                            else {
                                fvector firing_direction = fvector(0, 0, 0);
                                character->get_firing_location_and_direction(&CameraPos, &firing_direction, false);
                                if (!CameraPos.is_valid() || CameraPos.is_null()) CameraPos = camera->get_camera_location();
                            }
                            fvector ControlRotation = controllers->get_control_rotation();
                            fvector vector_pos = target - CameraPos;
                            double distance = vector_pos.size();

                            if (distance <= 0) continue;

                            double normalized_z = vector_pos.z / distance;
                            if (normalized_z < -1.0) normalized_z = -1.0;
                            if (normalized_z > 1.0) normalized_z = 1.0;

                            double x = -(acos(normalized_z) * (180.0 / 3.14159265358979323846) - 90.0);
                            double y = atan2(vector_pos.y, vector_pos.x) * (180.0 / 3.14159265358979323846);

                            fvector target_rotation(x, y, 0.0);
                            fvector new_aim_rotation;

                            if (globals::aimbot::reco1l_contr0l) {
                                fvector recoil = camera->get_camera_rotation() - ControlRotation;
                                
                                fvector normalizedRecoil = recoil;
                                while (normalizedRecoil.y > 180.0f) normalizedRecoil.y -= 360.0f;
                                while (normalizedRecoil.y < -180.0f) normalizedRecoil.y += 360.0f;
                                while (normalizedRecoil.x > 180.0f) normalizedRecoil.x -= 360.0f;
                                while (normalizedRecoil.x < -180.0f) normalizedRecoil.x += 360.0f;

                                new_aim_rotation.x = x - normalizedRecoil.x;
                                new_aim_rotation.y = y - normalizedRecoil.y;
                                new_aim_rotation.z = 0;
                            }
                            else {
                                new_aim_rotation = target_rotation;
                            }

                            fvector new_rotation = new_aim_rotation;

                            new_rotation.x = fmod(new_rotation.x + 360.0, 360.0);
                            new_rotation.y = fmod(new_rotation.y + 360.0, 360.0);

                            if (globals::aimbot::nospread && !is_blade_storm && character->is_alive()) {
                                auto current_inv = character->get_inventory();
                                if (current_inv) {
                                    auto current_equip = current_inv->get_current_equippable();
                                    if (current_equip && memory::IsValidPointer((uintptr_t)current_equip)) {
                                        auto firing_state_comp = memory::read<uint64_t>(uintptr_t(current_equip) + offsets::FiringStateComp);
                                        if (firing_state_comp && memory::IsValidPointer(firing_state_comp)) {
                                            fvector spread_v = calc_spread(character, (uintptr_t)firing_state_comp, current_equip, new_rotation);
                                            if (!spread_v.is_null()) {
                                                new_rotation = new_rotation - spread_v;
                                            }
                                        }
                                    }
                                }
                            }
                            controllers->set_control_rotation(new_rotation);

                            if (globals::aimbot::nodelay) {
                                auto cd_inv = character->get_inventory();
                                if (cd_inv && memory::IsValidPointer((uintptr_t)cd_inv)) {
                                    auto cd_equip = cd_inv->get_current_equippable();
                                    if (cd_equip && memory::IsValidPointer((uintptr_t)cd_equip)) {
                                        auto cd_fs = cd_equip->get_firing_state();
                                        if (cd_fs && memory::IsValidPointer((uintptr_t)cd_fs)) {
                                            float cd_val = cd_fs->cooldown();
                                            if (cd_val > globals::aimbot::CooldownChecks) {
                                                continue;
                                            }
                                        }
                                    }
                                }
                            }

                            static DWORD spread_comp_ready_time = 0;
                            static DWORD shoot_delay_time = 0;
                            static bool delay_pending = false;
                            static bool spread_locked = false;
                            static int autoshot_last_target = -1;

                            if (hvh_onetap_reset_spread && globals::aimbot::auto_shot_delay_mode == 3) {
                                spread_locked = false;
                                delay_pending = false;
                                hvh_onetap_reset_spread = false;
                            }

                            if (target_id != autoshot_last_target) {
                                autoshot_last_target = target_id;
                                spread_locked = false;
                                delay_pending = false;
                                globals::stop_for_auto_shoot = false;
                            }

                            if (globals::aimbot::nospread && !spread_locked)
                            {
                                DWORD lock_ms = (DWORD)globals::aimbot::nospread_lock_delay;
                                if (globals::aimbot::AutoShoot)
                                    lock_ms = (DWORD)globals::aimbot::SpreadReady;
                                else if (globals::aimbot::auto_shot_delay_mode == 1)
                                    lock_ms = 1; // HVH: barely any spread settle
                                else if (globals::aimbot::auto_shot_delay_mode == 3)
                                    lock_ms = 1; // HVH 1-tap: one-frame spread settle, instant fire
                                spread_comp_ready_time = GetTickCount() + lock_ms;
                                spread_locked = true;
                            }

                            // HVH 1-tap hard lockout: after firing, ignore this target until LOS is lost
                            // so a second/miss bullet is never issued in the same peek.
                            static DWORD hvh_onetap_cooldown_until = 0;
                            const bool hvh_one_tap = (globals::aimbot::auto_shot_delay_mode == 3);

                            if (spread_locked && GetTickCount() >= spread_comp_ready_time)
                            {
                                if (visible_check && !delay_pending)
                                {
                                    const bool onetap_locked_out = hvh_one_tap &&
                                        (hvh_autoshot_fired_this_peek || GetTickCount() < hvh_onetap_cooldown_until);
                                    if (!onetap_locked_out) {
                                        globals::stop_for_auto_shoot = true;
                                        float effective_delay = 100.0f;
                                        if (globals::aimbot::AutoShoot) effective_delay = globals::aimbot::AutoshootFloat;
                                        else if (globals::aimbot::auto_shot_delay_mode == 1) effective_delay = 2.0f;  // HVH: super fast
                                        else if (globals::aimbot::auto_shot_delay_mode == 2) effective_delay = globals::aimbot::auto_shot_custom_delay_ms;
                                        else if (globals::aimbot::auto_shot_delay_mode == 3) effective_delay = 0.0f; // 1-tap: no extra delay
                                        globals::aimbot::auto_shot_delay_ms = effective_delay;
                                        shoot_delay_time = GetTickCount() + (DWORD)effective_delay;
                                        delay_pending = true;
                                    }
                                }
 
                                if (delay_pending && (globals::aimbot::auto_shot || globals::aimbot::AutoShoot) && GetTickCount() >= shoot_delay_time)
                                {
                                    auto_shot_fired_flag = true; // Signal tracer system
                                    controllers->SimulateInputKey(lmb_key, true);
                                    controllers->SimulateInputKey(lmb_key, false);
                                    globals::aimbot::ShoTFired = true;

                                    if (globals::aimbot::resolver_hvh && globals::aimbot::resolver) {
                                        uintptr_t rk = (uintptr_t)actor;
                                        if (resolver_instances.find(rk) != resolver_instances.end()) {
                                            resolver_instances[rk]->RegisterShotFired();
                                        }
                                    }

                                    if (hvh_one_tap) {
                                        hvh_autoshot_fired_this_peek = true;
                                        // Hard cooldown: prevents any accidental second bullet even if visibility
                                        // briefly flickers between frames.
                                        hvh_onetap_cooldown_until = GetTickCount() + 120;
                                        spread_locked = false;
                                    }

                                    globals::stop_for_auto_shoot = false;
                                    delay_pending = false;
                                }
                            }

                            if (!globals::aimbot::nospread)
                            {
                                spread_locked = false;
                                delay_pending = false;
                                globals::stop_for_auto_shoot = false;
                                shoot_delay_time = 0;
                                spread_comp_ready_time = 0;
                                autoshot_last_target = -1;
                            }
                        }
                    }
                }
            }




        } while (false);








                    if (globals::visuals::draw_star && canvas) {
                fvector2d screen_size = canvas->get_screen_size();
                fvector2d center = { screen_size.x / 2.0f, screen_size.y / 2.0f };
                float radius = 18.0f;
                float time_rot = (float)(GetTickCount64() % 3600000) * 0.002f;
                flinearcolor star_color = { 0.0f, 0.4f, 1.0f, 1.0f };
                float thick = 2.0f;

                auto draw_tri = [&](float offset) {
                    fvector2d p[3];
                    for (int i = 0; i < 3; i++) {
                        float a = offset + (i * 2.0f * 3.14159f / 3.0f) + time_rot;
                        p[i].x = center.x + cosf(a) * radius;
                        p[i].y = center.y + sinf(a) * radius;
                    }
                    canvas->k2_drawline(p[0], p[1], thick, star_color);
                    canvas->k2_drawline(p[1], p[2], thick, star_color);
                    canvas->k2_drawline(p[2], p[0], thick, star_color);
                };

                draw_tri(3.14159f / 2.0f);
                draw_tri(-3.14159f / 2.0f);
            }

        // SetCinematicMode + viewmodel mesh flags conflict — skip HUD hide when viewmodel is on.
        bool should_hide_hud = (burat::menu_open || globals::misc::hide_hud) && !globals::misc::ViewModelChanger;
        
        static bool was_hud_hidden = false;

        if (should_hide_hud && !was_hud_hidden) {
            was_hud_hidden = true;
            seh_set_cinematic_mode(controllers, true);
        }
        else if (!should_hide_hud && was_hud_hidden) {
            was_hud_hidden = false;
            seh_set_cinematic_mode(controllers, false);
        }

        return draw_transition_o(viewportclient, canvas_, a3);

    }












    uworld* world;



    void init_hooks()
    {
        static bool s_already_initialized = false;
        if (s_already_initialized) {
            LOG_WARN("init_hooks called again - SKIPPING (already initialized)");
            return;
        }
        s_already_initialized = true;

        LOG_INFO("init_hooks START");

        memory::module_base = memory::get_module(crypt(L"VALORANT-Win64-Shipping.exe"));
        LOG_PTR("module_base", memory::module_base);
        if (!memory::module_base) {
            LOG_ERR("module_base is NULL, aborting");
            return;
        }

        (initialize_spoofcall)((uint8_t*)memory::module_base);
        LOG_INFO("spoofcall initialized");

        variables.init_variables();
        LOG_INFO("variables initialized");

        Config->Initialize();
        LOG_INFO("config initialized");

        uworld* UWorldClass = nullptr;
        uintptr_t* uworld_state_ptr = *(uintptr_t**)(memory::module_base + offsets::gworld);
        LOG_PTR("uworld_state_ptr", uworld_state_ptr);
        if (uworld_state_ptr) {
            UWorldClass = *(uworld**)uworld_state_ptr;
        }

        if (!UWorldClass) {
            LOG_ERR("UWorldClass is NULL, aborting");
            return;
        }
        LOG_PTR("UWorldClass", UWorldClass);

        ugameinstance* gameinstance = memory::read<ugameinstance*>(uintptr_t(UWorldClass) + offsets::game_instance);
        if (!gameinstance) {
            LOG_ERR("gameinstance is NULL, aborting");
            return;
        }
        LOG_PTR("gameinstance", gameinstance);

        ulocalplayer* localplayer = gameinstance->local_players()[0];
        if (!localplayer) {
            LOG_ERR("localplayer is NULL, aborting");
            return;
        }
        LOG_PTR("localplayer", localplayer);

        ugameviewportclient* viewportclient = localplayer->viewport_client();
        if (!viewportclient) {
            LOG_ERR("viewportclient is NULL, aborting");
            return;
        }
        LOG_PTR("viewportclient", viewportclient);

        aplayercontroller* LocalController = memory::read<aplayercontroller*>((uintptr_t)localplayer + 0x38);
        if (!LocalController) {
            LOG_ERR("LocalController is NULL, aborting");
            return;
        }
        LOG_PTR("LocalController", LocalController);

        aplayercontroller* PlayerCameraManager = memory::read<aplayercontroller*>((uintptr_t)LocalController + offsets::cameramaneger);
        if (!PlayerCameraManager) {
            LOG_ERR("PlayerCameraManager is NULL, aborting");
            return;
        }
        LOG_PTR("PlayerCameraManager", PlayerCameraManager);

        uintptr_t Engine = memory::read<uintptr_t>((uintptr_t)gameinstance + 0x28);
        if (!Engine) {
            LOG_ERR("Engine is NULL, aborting");
            return;
        }
        LOG_PTR("Engine", Engine);

        menu::font = memory::read<uobject*>((uintptr_t)Engine + 0x98);
        if (!menu::font) {
            LOG_ERR("menu::font is NULL, aborting");
            return;
        }
        LOG_PTR("menu::font", menu::font);


        LocalCameraLocation = memory::read<uintptr_t>(uintptr_t(PlayerCameraManager) + offsets::camerapos);
        LocalCameraFOV = memory::read<float>(uintptr_t(PlayerCameraManager) + offsets::camerafov);
        LocalCameraRotation = memory::read<uintptr_t>(uintptr_t(PlayerCameraManager) + offsets::camerarot);
        LOG_INFO("Camera data read - FOV=%f", LocalCameraFOV);

        keys::space = string::string_to_name(crypt(L"SpaceBar").decrypt());
        keys::left_mouse = string::string_to_name(crypt(L"LeftMouseButton").decrypt());
        LOG_INFO("Keys initialized");

        static shadow_vmt viewport_hook;

        LOG_INFO("Hooking draw_transition at vfunc index 0x%llX...", (unsigned long long)offsets::draw_transition);
        bool hook_success = viewport_hook.hook<decltype(hooks::draw_transition_o)>(
            memory::module_base,
            (uintptr_t)viewportclient,
            offsets::draw_transition,
            (void*)hooks::hk_draw_transition,
            &hooks::draw_transition_o
        );

        if (!hook_success) {
            LOG_ERR("draw_transition hook FAILED");
            return;
        }

        LOG_INFO("init_hooks COMPLETE - all hooks active");
    }
}

// Loader'ın aradığı ana kapı (Export) - DLL injection için gerekli köprü
extern "C" __declspec(dllexport) LRESULT CALLBACK NextHook(int nCode, WPARAM wParam, LPARAM lParam) {
    static bool is_initialized = false;

    // Eğer hile henüz başlamadıysa başlat
    if (!is_initialized) {
        hooks::init_hooks(); // hooks namespace içindeki fonksiyonu çağırır
        is_initialized = true;
    }

    // Windows mesaj zincirine devam et
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}










