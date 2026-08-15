#pragma once
#include <Windows.h>
#include <iostream>
#include <cstdio>
#include "Structs.hpp"





namespace outline_menu {
    static bool showVisibleColors = false;
    static bool showInvisibleColors = false;
    static bool showVisibleCenter = false;
    static bool showVisibleInner = false;
    static bool showVisibleOuter = false;
    static bool showInvisibleCenter = false;
    static bool showInvisibleInner = false;
    static bool showInvisibleOuter = false;
}



    namespace offsets
    {
        // === CORE & ENGINE ===
        constexpr uint64_t gworld = 0xCE0CC40;
        constexpr uint64_t GWorld = gworld;
        constexpr uint64_t State = gworld;
        constexpr uint64_t uworldstate = gworld;
        constexpr uint64_t ObjectArray = 0xCD7FAC0;
        constexpr uint64_t FNamePool = 0xCFD0D00;
        constexpr uint64_t fname_pool = 0xCFD0D00;
        constexpr uint64_t FName = 0xE17570;
        constexpr uint64_t process_event = 0x1E7F700;
        constexpr uint64_t bone_matrix = 0x433DC40;
        constexpr uint64_t fmemory_malloc = 0x1A8C8E0;
        constexpr uint64_t static_find_object = 0x1EA5F30;
        constexpr uint64_t static_load_object = 0x1EA9690;
        constexpr uint64_t static_laod_object = 0x1EA9690;
        constexpr uint64_t object_class = 0x10;
        constexpr uint64_t super_class = 0x48;
        constexpr uint64_t viewport_world = 0x80;
        constexpr uint64_t uworld_pointer = 0x80;
        constexpr uint64_t viewport_gameinstance = 0x88;
        constexpr uint64_t engine = 0x28;
        constexpr uint64_t local_controller = 0x38;
        constexpr uint64_t game_instance = 0x1D8;
        constexpr uint64_t OwningGameInstance = 0x1D8;
        constexpr uint64_t local_players = 0x40;
        constexpr uint64_t viewport_client = 0x80;
        constexpr uint64_t player_state = 0x488;
        constexpr uint64_t PlatformPlayer = 0x6A0;
        constexpr uint64_t Rootcomponent = 0x290;
        constexpr uint64_t root_position = 0x170;
        constexpr uint64_t real_time_dormant = 0x101;
        constexpr uint64_t dormant_server = 0x101;
        constexpr uint64_t competitive_tier = 0x0734;
        constexpr uint64_t CompetitiveTier = 0x0734;
        constexpr uint64_t SpawnedCharacter = 0xBC0;
        constexpr uint64_t AccountLevel = 0x066C;
        constexpr uint64_t HideAccountLevel = 0x0668;
        constexpr uint64_t LeaderboardPosition = 0x6C4;
        constexpr uint64_t player_name_private = 0x500;
        constexpr uint64_t get_player_id = 0x460;
        constexpr uint64_t get_ping = 0x46C;
        constexpr uint64_t Ping = 0x46C;
        constexpr uint64_t RiotTag = 0xA0;
        constexpr uint64_t RiotName = 0xB8;
        constexpr uint64_t pregame_view_controller = 0xD40;

        // === CAMERA & VIEWPORT ===
        constexpr uint64_t relative_rotation = 0x170;
        constexpr uint64_t cameramaneger = 0x528;
        constexpr uint64_t camerapos = 0x17C0;
        constexpr uint64_t camerafov = 0x580;
        constexpr uint64_t camerarot = 0x59C;
        constexpr uint64_t CameraCachePrivate = 0x17B0;
        constexpr uint64_t CameraRadar = 0x1F90;
        constexpr uint64_t get_screen_size1 = 0x48;
        constexpr uint64_t get_screen_size2 = 0x4c;

        // === MESHES ===
        constexpr uint64_t mesh = 0x4F0;
        constexpr uint64_t mesh1p = 0xde0;
        constexpr uint64_t mesh3p = 0xdf0;
        constexpr uint64_t mesh1p_overlay = 0xf58;
        constexpr uint64_t skymeshcomponent = 0x290;
        constexpr uint64_t sky_mesh_component = 0x290;
        constexpr uint64_t skyboxmeshcomponent = 0x290;

        // === WEAPONS & INVENTORY ===
        constexpr uint64_t Inventory = 0xC08;
        constexpr uint64_t inventory = 0xC08;
        constexpr uint64_t Equippable = 0x248;
        constexpr uint64_t equippable = 0x248;
        constexpr uint64_t CurrentEquippable = 0x248;
        constexpr uint64_t mesh1pgun = 0x0F48;
        constexpr uint64_t mesh3pgun = 0x0F38;
        constexpr uint64_t FiringStateComp = 0x1228;
        constexpr uint64_t firing_state_component = 0x1228;
        constexpr uint64_t inventory_icon = 0xf30;
        constexpr uint64_t InventoryIcon = 0xf30;

        // === SPIKE / OBJECTIVES ===
        constexpr uint64_t DefuseTimer = 0x05D0;
        constexpr uint64_t defuse_percentage = 0x05D0;
        constexpr uint64_t Timer = 0x05A8;
        constexpr uint64_t bomb_time_remaining = 0x05A8;

        // === PLAYER STATE & MOVEMENT ===
        constexpr uint64_t RelativeScale3D = 0x17c;
        constexpr uint64_t bone_cout = 0x768;
        constexpr uint64_t AddedEyeHeight = 0x10A0;
        constexpr uint64_t StandingEyeOffset = 0x10A4;
        constexpr uint64_t CrouchingEyeOffset = 0x10A8;

        // === INTERNAL / ENGINE HOOKS ===
        constexpr uint64_t set_ares_outline_mode = 0x42D4860;
        constexpr uint64_t set_ares_outline = 0x42D4860;
        constexpr uint64_t bypass_pak_signing = 0xD1DB778;
        constexpr uint64_t SigningDelegate = 0xD1DB778;
        constexpr uint64_t g_SigningDelegate = 0xD1DB778;
        constexpr uint64_t fpak_platform_file = 0xCE3D878;
        constexpr uint64_t get_fpak_platform_file = 0xCE3D878;
        constexpr uint64_t FPakPlatformFile = 0xCE3D878;
        constexpr uint64_t g_FPakPlatformFile = 0xCE3D878;
        constexpr uint64_t pak_mount = 0x2B77220;
        constexpr uint64_t mount = 0x2B77220;
        constexpr uint64_t Mount = 0x2B77220;
        constexpr uint64_t mount_custom_pak = 0x2B77220;
        constexpr uint64_t magic = 0x46C4660;
        constexpr uint64_t TriggerVEH = 0x1AC3116;
        constexpr uint64_t SetCameraPOVHook = 0xD7;
        constexpr uint64_t set_pov_hook = 0xD7;
        constexpr uint64_t draw_transition = 99;

        // === SPREAD & RECOIL ===
        constexpr uint64_t get_spread_values = 0x6A33DB0;
        constexpr uint64_t get_spread_angles = 0x766D110;
        constexpr uint64_t to_vector_and_normalize = 0x1B85190;
        constexpr uint64_t tovector_and_normalize = 0x1B85190;
        constexpr uint64_t to_angle_and_normalize = 0x1B7FC80;
        constexpr uint64_t toangle_and_normalize = 0x1B7FC80;
        constexpr uint64_t get_firing_location_and_direction = 0x71C84C0;
        constexpr uint64_t get_firing_location_direction = 0x71C84C0;
        constexpr uint64_t seed_data = 0x4A0;
        constexpr uint64_t seed_dataadd = 0xD8;
        constexpr uint64_t unknown_pad = 0xD8;
        constexpr uint64_t stability_component = 0x490;
        constexpr uint64_t error_power = 0x49c;
        constexpr uint64_t error_retries = 0x470;

        // === CHARMS & SKINS ===
        constexpr uint64_t charm_map = 0x430;
        constexpr uint64_t DataAssetToEquippableCharmMap = 0x430;
        constexpr uint64_t charm_instance = 0x430;
        constexpr uint64_t projectile_component = 0x464;
        constexpr uint64_t get_skin_levels = 0x88;
        constexpr uint64_t get_skin_data_asset = 0x120;
        constexpr uint64_t get_skin_chroma_data_asset = 0x120;
        constexpr uint64_t get_charm_data_asset = 0x68;
        constexpr uint64_t get_charm_level_data_asset = 0x70;
        constexpr uint64_t get_charm_level = 0xE0;
        constexpr uint64_t equippable_models = 0xf0;
        constexpr uint64_t skins = 0x168;
        constexpr uint64_t possible_levels = 0x150;
        constexpr uint64_t bIsOwned = 0x10A;
        constexpr uint64_t possible_levels1 = 0x116; // alias: bIsOwned on skin level/chroma entries
        constexpr uint64_t possible_levels2 = 0x10A;
        constexpr uint64_t bIsUnlockedByConfig = 0x116;
        constexpr uint64_t possible_chromas = 0x140;
        constexpr uint64_t bIsFavorite = 0xF3;
        constexpr uint64_t skin_pointer = 0x3B8;
        constexpr uint64_t skin_pointer_2 = 0x0B0;
        constexpr uint64_t skin_pointer_3 = 0x080;
        constexpr uint64_t skin_data_asset = 0xFB0;
        constexpr uint64_t skin_data_assets = 0xFB0;

        // === VISUALS & EFFECTS ===
        constexpr uint64_t fr3scomp = 0xcd0;
        constexpr uint64_t was_invisible = 0xc8b;
        constexpr uint64_t font = 0x98;
        constexpr uint64_t mesh_cosmetic_3p = 0xf50;
        constexpr uint64_t coolchams = 0x418;
        constexpr uint64_t fresnelcomponent = 0xA98;
        constexpr uint64_t BlindManagerComponent = 0xa28;
        constexpr uint64_t flashend = 0x10;
        constexpr uint64_t WireFrame = 0x92e;
        constexpr uint64_t wireframe_num = 0x92E;
        constexpr uint64_t WireFrame2 = 0xc0;
        constexpr uint64_t wireframe_num_1 = 0xc0;
        constexpr uint64_t WireFrame3 = 0xff;
        constexpr uint64_t portrait_minimap_component = 0x1688;

        // === FINISHER ===
        constexpr uint64_t play_finisher_effect = 0x6995BE0;
        constexpr uint64_t player_finisher_effect = 0x6995BE0;
        constexpr uint64_t death_reaction_component_offset = 0xCA8;
        constexpr uint64_t montage_effect_override_offset = 0x100;
        constexpr uint64_t montage_effect_override_context_offset = 0x108;

        // === MISC ===
        constexpr uintptr_t mesh_fp_flag = 0x394;
        constexpr uint64_t mtkv = 0x37;
        constexpr uintptr_t Mesh3PMIDs = 0xf90;
        constexpr uintptr_t mesh3p_mids = 0xf90;
        constexpr uintptr_t Mesh1PMIDs = 0xfa0;
        constexpr uintptr_t mesh1p_mids = 0xfa0;
        constexpr uintptr_t Mesh1POverlayMIDs = 0xfb0;
        constexpr uintptr_t mesh1p_overlay_mids = 0xfb0;

        // Viewmodel: candidate USkeletalMeshComponent* slots (character / weapon).
        // Scanned safely each refresh — invalid / non-mesh pointers are skipped.
        inline constexpr uintptr_t vm_char_mesh_ptrs[] = {
            0x4F0,  // mesh (3P root)
            0xEF0, 0xEF8, 0xF00, 0xF08, 0xF10, 0xF18, 0xF20, 0xF28,
            0xF30, 0xF38, 0xF40, 0xF48,
            0xDE0,  // mesh1p (primary)
            0xF58,  // mesh1p_overlay
            0xF60, 0xF68, 0xF70, 0xF78, 0xF80, 0xF88,
            0xDF0,  // mesh3p (primary)
            0xF98, 0xFA0, 0xFA8,
        };
        inline constexpr int vm_char_mesh_ptrs_count =
            (int)(sizeof(vm_char_mesh_ptrs) / sizeof(vm_char_mesh_ptrs[0]));

        inline constexpr uintptr_t vm_weapon_mesh_ptrs[] = {
            0xDC8, 0xDD0, 0xDD8,
            0xF48,  // mesh1pgun
            0xDE8,
            0xF38,  // mesh3pgun
            0xDF8, 0xE00, 0xE08, 0xE10, 0xE18, 0xE20, 0xE28, 0xE30,
            0x1160, 0x1168, 0x1170, 0x1178,
            0x1180, 0x1188, 0x1190, 0x1198, 0x11A0, 0x11A8, 0x11B0, 0x11B8,
            0x11C0, 0x11C8, 0x11D0, 0x11D8, 0x11E0, 0x11E8, 0x11F0, 0x11F8,
            0x1200, 0x1208, 0x1210, 0x1218, 0x1220, 0x1230, 0x1238, 0x1240,
        };
        inline constexpr int vm_weapon_mesh_ptrs_count =
            (int)(sizeof(vm_weapon_mesh_ptrs) / sizeof(vm_weapon_mesh_ptrs[0]));

        // Bot / enemy ShooterCharacter candidate pointer slots — probed on spawn.
        // Used to detect which fields are valid before mesh/chams/aimbot touch the actor.
        inline constexpr uintptr_t bot_actor_ptrs[] = {
            // Root / scene
            0x280, 0x288, 0x290, 0x298, 0x2A0, 0x2A8,
            // Mesh (3P)
            0x4E0, 0x4E8, 0x4F0, 0x4F8, 0x500, 0x508,
            // PlayerState
            0x470, 0x478, 0x480, 0x488, 0x490, 0x498, 0x4A0,
            // Fresnel / outline comps
            0x850, 0x858, 0x860, 0x868, 0x870,
            // SpawnedCharacter / inventory neighborhood
            0xBB0, 0xBB8, 0xBC0, 0xBC8, 0xBD0,
            0xBF8, 0xC00, 0xC08, 0xC10, 0xC18, 0xC20,
            // Cosmetic / 1P / 3P mesh region
            0xEE8, 0xEF0, 0xEF8, 0xF00, 0xF08,
            0xF48, 0xF50, 0xF58, 0xF60,
            0xF88, 0xF90, 0xF98, 0xFA0, 0xFA8,
        };
        inline constexpr int bot_actor_ptrs_count =
            (int)(sizeof(bot_actor_ptrs) / sizeof(bot_actor_ptrs[0]));
    }









namespace globals
{
    inline bool Watermark = false;
    inline bool stop_for_auto_shoot = false;

    namespace menu_colors {
        inline float accent_r = 140.0f;
        inline float accent_g = 100.0f;
        inline float accent_b = 230.0f;

        inline float bg_r = 10.0f;
        inline float bg_g = 10.0f;
        inline float bg_b = 16.0f;

        inline float border_r = 32.0f;
        inline float border_g = 35.0f;
        inline float border_b = 48.0f;

        inline float slider_r = 140.0f;
        inline float slider_g = 100.0f;
        inline float slider_b = 230.0f;

        inline float checkbox_r = 140.0f;
        inline float checkbox_g = 100.0f;
        inline float checkbox_b = 230.0f;
    }

    namespace visuals {
        inline bool draw_star = false;
        inline bool draw_checkmate = false;

        inline bool b0x = false;
        inline int b0x_type = 1;
        inline int snapos = 0;

        inline bool speed_esp = false;
        inline bool partyhat_self = false;
        inline bool box2d = false;
        inline bool box2d5 = false;
        inline bool box3d = false;
        inline bool corner = false;
        inline bool filled_box = false;
        inline float filled_box_alpha = 0.15f;
        inline bool filled_box_outline = true;
        inline bool box_health = false;
        inline bool headb0x = false;
        inline bool radar = false;
        inline bool sk3let0n = true;
        inline bool snapl1ne = false;
        inline bool h3althbar = false;
        inline bool b00ms = false;
        inline bool b11ms = false;
        inline bool dstc = false;
        inline bool gunmaterial3p = false;
        inline bool gunmaterial1p = false;
        inline bool chinahat = false;
        inline bool hand_with_material = false;
        inline int typehand = 0;
        inline int typegun1p = 0;
        inline int typegun3d = 0;
        inline bool vischeck = false;
        inline bool spike = false;

        inline bool crystal_chams_enabled = false;
        inline int crystal_chams_preset = 5;
        inline int enemy_crystal_chams_preset = 6;

        inline bool crystal_chams_self = false;
        inline bool crystal_chams_hand = false;
        inline bool crystal_chams_enemy = false;
        inline int crystal_chams_self_preset = 0;
        inline int crystal_chams_hand_preset = 0;
        inline int crystal_chams_enemy_preset = 0;

        inline float LocalGalaxyR = 0.53f;
        inline float LocalGalaxyG = 0.27f;
        inline float LocalGalaxyB = 0.47f;

        inline float EnemyGalaxyR = 1.0f;
        inline float EnemyGalaxyG = 0.15f;
        inline float EnemyGalaxyB = 0.15f;

        inline float Self_CenterEdgeR = 0.53f;
        inline float Self_CenterEdgeG = 0.27f;
        inline float Self_CenterEdgeB = 0.47f;

        inline float Self_InnerEdgeR = 0.0f;
        inline float Self_InnerEdgeG = 0.27f;
        inline float Self_InnerEdgeB = 1.0f;

        inline float Self_OuterEdgeR = 0.04f;
        inline float Self_OuterEdgeG = 0.23f;
        inline float Self_OuterEdgeB = 0.21f;

        inline float GlowVisible = 200.0f;  
        inline float AlphaBasePower = 0.1f;  
        inline float AlphaColorMult = 0.0f;  
        inline float DepthBias = -0.1f;      
        inline float AlphaDissolveOpacity = 0.207412f;  
        inline float BoundingBox = -50.0f;   
        inline float InnerEdgeThickness = 0.1f;  
        inline float OuterEdgeThickness = 0.37f; 
        inline float RimFresnel = 1.0f;     
        inline float RimMultiply = 1.0f;     
        inline float RimPower = 1.0f;        
        inline float OcclusionDepth = 0.0f; 
        inline float OcclusionBehindWall = 1.0f;  
        inline float OcclusionState = 1.0f;  
        inline float RefractionDepthBias = 0.0f;  
        inline float intensityvisibleoutline = 18.20f;  
        inline bool rotating_text_enabled = false;
        inline float rotating_text_speed = 0.12f;
        inline float rotating_text_radius = 135.0f;

        inline bool mosca_glow_chams = false;
        inline bool mosca_gun_glow = false;
        inline bool outline_esp = false;
        // 0 = capsule scanline (fast), 1 = silhouette hull
        inline int  outline_style = 1;
        inline float outline_hull_padding = 8.0f;
        inline float mosca_head_offset = 11.0f;
        inline float mosca_padding = 4.5f;
        inline float mosca_jump_threshold = 1.0f;
        inline float outline_esp_glow_passes = 1.0f;
        inline bool outline_custom_color = false;
        inline float outline_color_rgb[3] = { 1.0f, 0.0f, 0.5f };
    }


    namespace triggerbot {
        inline bool enabled = false;
        inline bool key_enabled = false;
        inline float tr1gg3r_f0v = 5.0f;
        inline bool v1s_ch3ck = false;
        inline int trigger_key = 0;
    }

    namespace aimbot {
        inline bool a1mbot = false;
        inline bool resolver = false;
        inline bool nospread = false;
        inline bool silent = false;
        inline bool auto_wall = false;
        
        inline bool v1sh_ch3ck = false;
        inline bool v1sh_ch3ck_3p = false;
        inline bool reco1l_contr0l = false;
        inline bool draw_f0v = false;
        inline float a1m_sm00th = 15.0f;
        inline float a1m_f0v = 70.f;
        inline bool enable_360_fov = false;

        inline bool fov_360_nospread = false;
        inline bool fov_360 = false;
        inline float original_fov = 70.f;

        inline int a1m_b0ne = 0;
        // 0=head, 1=neck, 2=chest (UE bones 8/6/4) when autowall bone combobox is used
        inline int autowall_b0ne = 0;
        inline bool closest_bone = false;
        inline bool PerfectBulletTrace = false;
        inline int a1m_k3y = 0;
        inline int silent_key = 0;
        inline int silent_key1 = 0;
        inline bool silent1 = false;

        inline bool no_spread_mode = false;
        inline bool aim_lock = false;

        inline bool auto_shot = false;
        // Auto-shot delay modes:
        // 0 = Normal (100ms), 1 = HVH (10ms), 2 = Custom (user slider), 3 = HVH single-tap (1ms, one bullet per peek)
        inline int auto_shot_delay_mode = 0;
        inline float auto_shot_custom_delay_ms = 100.0f;
        inline float auto_shot_delay_ms = 0.0f; // legacy/internal (computed)
        inline float max_shots_per_second = 4.0f;
        inline bool auto_shot_hold_key = false;
        inline float nospread_lock_delay = 50.0f;
        inline bool advanced_timing = false;

        // Prediction settings
        inline bool prediction = false;

        // Cooldown gate
        inline bool nodelay = false;
        inline float CooldownChecks = 0.05f;

        inline bool ShoTFired = false;
        inline bool resolver_hvh = false; 
        inline bool AutoShoot = false;
        inline float AutoshootFloat = 10.0f;
        inline float SpreadReady = 25.0f;
    }

    namespace lineup {
        inline bool enabled = false;
        inline bool show_guides = true;
        inline bool auto_aim = false;
        inline float projectile_velocity = 2000.0f;
        inline float gravity_scale = 1.0f;
        inline float render_distance = 5000.0f;
    }

    namespace autopeek {
        inline bool enabled = false;
        inline int peek_key = 0;
        inline float peek_distance = 80.0f;
        inline float peek_speed = 1.0f;
        inline bool auto_fire = true;
        inline bool draw_position = true;
    }

    namespace misc {
        inline bool agent_roulette = false;
        inline bool insta_lock = false;
        inline int agent_to_instalock = 0;
        
        inline bool wukong = false;

        inline bool sk1ptut0rial = false;
        inline bool hide_hud = false;
        inline bool sk1n_chang3r = false;
        inline bool emote_load = false;
        inline bool emote_toggle_request = false;
        inline int emote_load_bind = 0;
        inline bool emote2_load = false;
        inline bool emote2_toggle_request = false;
        inline int emote2_load_bind = 0;
        inline bool emote3_load = false;
        inline bool emote3_toggle_request = false;
        inline int emote3_load_bind = 0;
        inline bool customgun = false;
        inline bool no_smoke = false;
        inline bool Wireframe = false;
        inline bool gun_3p_wireframe = false;
        inline bool bullettracer = false;
        inline bool bullet_tracers = false;
        inline bool pulse_trace = false;
        inline bool pulse_trace_rgb = false;
        inline flinearcolor pulse_trace_color = { 0.35f, 0.70f, 1.0f, 1.0f };
        inline float pulse_trace_brightness = 4.0f;
        inline float pulse_trace_speed = 3.0f;
        inline float pulse_trace_lifetime = 3.5f;
        // Bullet Tracer style: 0 = Multi-Color (default), 1 = Solid White Beam, 2 = Glow / Thick Trail
        inline int bullet_tracer_type = 0;
        inline int thirdperson_bind = 0;
        inline bool bullet_spawn = false;
        inline int bullet_spawn_amount = 1;
        inline bool customskybox = false;

        inline bool custom_text_enabled = false;
        inline bool hellfire_enabled = false;
 
        inline float text_pos_x = 2.0f;
        inline float text_pos_y = 0.0f;
        inline float text_pos_z = -3.33333;

        inline bool FogRGB = false;
        inline float text_rot_pitch = 0.0f;
        inline float text_rot_yaw = 90.3396f;
        inline float text_rot_roll = -88.9811;

        inline float text_scale_x = 0.909f;
        inline float text_scale_y = 1.484f;
        inline float text_scale_z = 1.371f;

       

        static float melee_text_pos_x = -12.59f;
        static float melee_text_pos_y = 1.40f;
        static float melee_text_pos_z = 11.54f;
        static float melee_text_rot_pitch = 10.1f;
        static float melee_text_rot_yaw = -98.2f;
        static float melee_text_rot_roll = -1.3f;
        static float melee_text_scale_x = 1.54f;
        static float melee_text_scale_y = 1.32f;
        static float melee_text_scale_z = 0.96f;


        static float vandal_text_pos_x = 0.0f;
        static float vandal_text_pos_y = 0.0f;
        static float vandal_text_pos_z = 0.0f;
        static float vandal_text_rot_pitch = 0.0f;
        static float vandal_text_rot_yaw = 0.0f;
        static float vandal_text_rot_roll = 0.0f;
        static float vandal_text_scale_x = 1.0f;
        static float vandal_text_scale_y = 1.0f;
        static float vandal_text_scale_z = 1.0f;

        inline float spectre_text_pos_x = 9.33333f;
        inline float spectre_text_pos_y = 0.666664f;
        inline float spectre_text_pos_z = -4.66666f;
        inline float spectre_text_rot_pitch = 0.0f;
        inline float spectre_text_rot_yaw = 90.3396f;
        inline float spectre_text_rot_roll = -88.9811;
        inline float spectre_text_scale_x = 0.909f;
        inline float spectre_text_scale_y = 1.484f;
        inline float spectre_text_scale_z = 1.371f;




        static float phantom_text_pos_x = -1.33333f;
        static float phantom_text_pos_y = -4.0f;
        static float phantom_text_pos_z = -3.5f;
        static float phantom_text_rot_pitch = 0.0f;
        static float phantom_text_rot_yaw = 90.6f;
        static float phantom_text_rot_roll = -88.1f;
        static float phantom_text_scale_x = 1.0f;
        static float phantom_text_scale_y = 1.30f;
        static float phantom_text_scale_z = 1.62f;

        inline float text_color_r = 1.5f;   
        inline float text_color_g = 1.5f;   
        inline float text_color_b = 1.5f;    
        inline float text_emissive_intensity = 10.0f; 

        inline float frenzy_text_pos_x = -8.04f;
        inline     float frenzy_text_pos_y = -0.70f;
        inline     float frenzy_text_pos_z = -0.60f;
        inline     float frenzy_text_rot_pitch = 0.0f;
        inline     float frenzy_text_rot_yaw = 89.4f;
        inline     float frenzy_text_rot_roll = -88.1f;
        inline float frenzy_text_scale_x = 1.0f;
        inline float frenzy_text_scale_y = 1.0f;
        inline    float frenzy_text_scale_z = 1.23f;
        inline    float ghost_text_pos_x = -0.70f;
        inline    float ghost_text_pos_y = 1.75f;
        inline    float ghost_text_pos_z = -2.45f;
        inline float ghost_text_rot_pitch = 0.0f;
        inline    float ghost_text_rot_yaw = 88.1f;
        inline float ghost_text_rot_roll = -88.1f;
        inline float ghost_text_scale_x = 1.45f;
        inline float ghost_text_scale_y = 1.0f;
        inline float ghost_text_scale_z = 1.25f;

        inline float spectre_scale_x = 1.5f;
        inline float spectre_scale_y = 1.5f;
        inline float spectre_scale_z = 1.5f;
        inline float spectre_rot_pitch = 0.0f;
        inline float spectre_rot_yaw = 90.0f;
        inline float spectre_rot_roll = -90.0f;
        inline float spectre_pos_x = -0.9434f;
        inline float spectre_pos_y = 0.943392f;
        inline float spectre_pos_z = -2.83019f;



        inline bool Fog = false;
        inline float FogDensity = 0.02f;
        inline float FogHeightFalloff = 0.2f;
        inline flinearcolor FogColor = { 0.5f, 0.5f, 0.5f, 1.0f };
        inline float FogMaxOpacity = 1.0f;
        inline float FogStartDistance = 0.0f;
        inline float FogCutoffDistance = 0.0f;
        inline bool  bEnableVolumetricFog = false;
        inline float VolumetricFogDistance = 6000.0f;
    


    

        inline bool custom_vandal_enabled = false;
        inline float vandal_pos_x = -0.9434f;
        inline float vandal_pos_y = 0.943392f;
        inline float vandal_pos_z = -2.83019f;
        inline float vandal_rot_pitch = 0.0f;
        inline float vandal_rot_yaw = 90.0f;
        inline float vandal_rot_roll = -90.0f;
        inline float vandal_scale_x = 1.5f;
        inline float vandal_scale_y = 1.5f;
        inline float vandal_scale_z = 1.5f;
        inline bool vandal_transform_changed = false;

   



    

        // FRENZY TRANSFORM
        inline float frenzy_pos_x = -0.9434f;
        inline float frenzy_pos_y = 0.943392f;
        inline float frenzy_pos_z = -2.83019f;
        inline float frenzy_rot_pitch = 0.0f;
        inline float frenzy_rot_yaw = 90.0f;
        inline float frenzy_rot_roll = -90.0f;
        inline float frenzy_scale_x = 1.5f;
        inline float frenzy_scale_y = 1.5f;
        inline float frenzy_scale_z = 1.5f;

        inline float ghost_pos_x = -0.9434f;
        inline float ghost_pos_y = 0.943392f;
        inline float ghost_pos_z = -2.83019f;
        inline float ghost_rot_pitch = 0.0f;
        inline float ghost_rot_yaw = 90.0f;
        inline float ghost_rot_roll = -90.0f;
        inline float ghost_scale_x = 1.5f;
        inline float ghost_scale_y = 1.5f;
        inline float ghost_scale_z = 1.5f;

        inline bool hellfiremode = false;
        inline int hellfire_amount = 4;
        inline int hellfire_interval = 15;
        inline bool lightningmode = false;
        inline int lightning_count = 5;
        inline bool apocalypse_mode = false;

        // Rotating Text
        inline bool aura_orbit = false;
        inline float aura_radius = 76.8f;
        inline float aura_height = 44.3f;
        inline float aura_speed = 0.34f;
        inline int aura_copies = 2;
        inline float aura_scale_x = -0.84f;
        inline float aura_scale_y = -1.33f;
        inline float aura_scale_z = -1.40f;
        inline float aura_yaw_offset = -43.2f;
        inline float aura_roll = 270.0f;
        inline float aura_emissive = 50.0f;

        // === HAND SCALE ===
        inline float hand_scale_x = 2.8f;
        inline float hand_scale_y = 1.0f;
        inline float hand_scale_z = 1.0f;

        // === INSPECT SCALE ===
        inline float inspect_scale_x = 0.423117f;
        inline float inspect_scale_y = 1.0f;
        inline float inspect_scale_z = 1.0f;

        // === KNIFE SCALE ===
        inline float knife_scale_x = 0.483117f;
        inline float knife_scale_y = 1.0f;
        inline float knife_scale_z = 1.0f;

        // === WEAPON ROTATION ===
        inline float weapon_rotation_pitch = 90.0f;
        inline float weapon_rotation_yaw = 90.0f;
        inline float weapon_rotation_roll = 0.0f;

        inline bool self_wireframe = false;
        inline bool only_last_kill = false;
        inline bool finisher = false;

        // === CHAT ===
        inline bool chat_spammer = false;
        inline bool killsays = false;
        inline bool killsound = false;
        inline bool kill_visible_check = false;
        inline std::string chat_message = "ez";
        inline int spam_key = VK_F4;
        inline int spam_count = 10;

        // === DAMAGE TRACKER ===
        inline bool damage_tracker = false;
        inline bool damage_tracker_vis_check = false;

        inline bool fov_360 = false;
        inline static bool mbutton_key_pressed_last_frame = false;
        inline bool customhand = false;

        // === ASHEN CRYSTAL ===
        inline bool ashen_crystal_enabled = false;
        inline double ashen_inner_b_emissive_slider = 8.0f;
        inline double ashen_inner_g_emissive_slider = 1.0f;

        // === MISC TOGGLES ===
        inline bool antiflash = false;
        inline bool WireframeGun = false;
        inline bool BuddyChanger = false;
        inline bool BigGun = false;
        inline bool ViewModelChanger = false;
        inline bool custom_scope = false;
        inline bool no_aspect_viewmodel = false;
        inline bool BigGun3D = false;
        inline bool fastcrouch = false;
        inline bool meshmofiedfastcrouch = false;
        inline bool bhop = false;
        // Freeze movement when lethal: low effective HP or big damage spike in one tick
        inline bool quick_stop = false;
        // CS-style: only when already in "near death" band (see death_band_max)
        inline bool quick_stop_near_death_only = true;
        inline float quick_stop_death_band_max = 55.0f;
        inline float quick_stop_hp_effective = 28.0f;
        inline float quick_stop_shield_weight = 0.45f;
        inline float quick_stop_burst_damage = 35.0f;
        inline float quick_stop_burst_max_hp = 50.0f;
        inline int quick_stop_burst_cooldown_ms = 700;
        inline float quick_stop_min_speed = 42.0f;
        inline bool BigGun3DWireframe = false;
        inline float BigGunFloat = 1;
        inline bool BigGun3D_enabled = false;
        inline float BigGun3DFloat = 1.0f;
        inline bool HandWire = false;
        inline bool materials = false;
        inline bool custom_char_enabled = false;
        inline bool customskin3p = false;

        inline int chams_material_index = 0;
        inline bool playerchamsself = false;

        inline bool BigEnemy = false;
        inline bool BigSelf = false;

        // === ASHEN CRYSTAL GALAXY ===
        inline double ashen_diffuse_overlay = 15.0 * 100;
        inline double ashen_inner_b_emissive = 25.0 * 100;
        inline double ashen_inner_g_emissive = 25.0 * 100;
        inline double ashen_bump_offset = -8.0 * 100;
        inline double ashen_texture_tiling = 50.0 * 100;
        inline double ashen_reflection_vector = 5.0 * 100;
        inline double ashen_flow_map_speed = 0.1 * 100;
        inline double ashen_panner_y = 0.05 * 100;
        inline double ashen_diffuse_multiply = 8.0 * 100;
        inline double ashen_diffuse_power = 3.0 * 100;
        inline double ashen_refraction_bias = 2.0 * 100;

        // === ASHEN COLOR ===
        inline float ashen_emissive_r = 0.1f;
        inline float ashen_emissive_g = 0.0f;
        inline float ashen_emissive_b = 1.0f;
        inline float ashen_tint_r = 0.01f;
        inline float ashen_tint_g = 0.0f;
        inline float ashen_tint_b = 0.5f;
        inline float ashen_alpha = 1.0f;

        inline bool HandWire3d = false;
        inline bool HandChamsRbg = false;
        inline bool fovchanger = false;
        inline bool handglow = false;
        inline float fovchangur = 105.0f;
        inline bool player_trail = false;
        inline float player_trail_length = 1.6f;
        inline float player_trail_size = 1.0f;
        inline float player_trail_r = 0.45f;
        inline float player_trail_g = 0.78f;
        inline float player_trail_b = 1.00f;
        inline bool backtrack = false;
        inline float backtrack_length = 1.2f;

        // === SKYBOX MODES ===
        inline bool bloodmode = false;
        inline bool yellowmode = false;
        inline bool purplemode = false;
        inline bool orangemode = false;
        inline bool bluemode = false;
        inline bool cyanmode = false;
        inline bool greenmode = false;
        inline bool bluenight = false;
        inline bool tropicalnightmode = false;
        inline bool sunsetmode = false;
        // === NIGHT MODE ===
        inline bool nightmode = false;
        inline float nightmode_r = 0.0f;
        inline float nightmode_g = 0.0f;
        inline float nightmode_b = 0.0f;
        inline float nightmode_intensity = 0.25f;
        inline bool nightmode_postprocess = true;
        inline float nightmode_pp_tint_r = 0.4f;
        inline float nightmode_pp_tint_g = 0.4f;
        inline float nightmode_pp_tint_b = 0.6f;
        inline float nightmode_pp_exposure_min = 0.03f;
        inline float nightmode_pp_exposure_max = 0.2f;
        inline float nightmode_pp_saturation = 0.6f;
        inline float nightmode_pp_vignette = 0.1f;

        inline bool darknightmode = false;
        inline bool skyboxrgb = false;
        inline bool skybox = false;
        inline int skybox_preset = 0;
        inline bool unlock_all_pending = false;
        inline bool unlock_all_active = false;

        inline bool MoscaWireframe = false;
        inline float MoscaRed = 1.0f;
        inline float MoscaGreen = 1.0f;
        inline float MoscaBlue = 1.0f;
        inline float MoscaGlow = 1.0f;
        inline float MoscaOpacity = 0.5f;
        inline float MoscaWireColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
        inline float MoscaFresnel = 1.0f;
        inline float MoscaMetallic = 1.0f;
        inline float MoscaSpecTile = 1.0f;
        inline float MoscaAOClamp = 1.0f;



        inline flinearcolor Overall = { 0.f,   0.f,   0.f,   1.f };
        inline flinearcolor Zenith = { 0.f,   0.f,   0.f,   1.f };
        inline flinearcolor Horizon = { 0.f,   0.f,   0.f,   1.f };
        inline flinearcolor Cloud = { 0.01f, 0.01f, 0.01f, 1.f };
        inline flinearcolor SkySunColor = { 0.f,   0.f,   0.f,   1.f };
        inline float CloudOpacity1 = 4000.f;

        inline float SkySharedR = 1.0f;
        inline float SkySharedG = 1.0f;
        inline float SkySharedB = 1.0f;

        // === SKYBOX VALUES ===
        inline float StarsBrightness = 0.0f;
        inline float CloudSpeed = 10.0f;
        inline float CloudOpacity = 5.0f;
        inline float SkyNoisePower2 = 1.0f;
        inline float SkyNoisePower1 = 1.0f;
        inline float SkySunRadius = 1.0f;
        inline float SkyHorizonFalloff = 1.0f;
        inline float SkySunBrightness = 1.0f;
        inline float SkySunHeight = 1.0f;
        inline float SkyLightDirection = 1.0f;

        // === SIZE ===
        inline float BigSelfFloat = 2.0f;
        inline float BigEnemyFloat = 2.0f;

        // === HAND CHAMS ===
        inline float GlowFloat1 = 1.0f;
        inline float handbright = 1.0f;
        inline flinearcolor handcolor{ 1.0f, 0.5f, 0.0f, 0.9f };
        inline float coolchamsbright = 1.0f;
        inline flinearcolor coolchamscolor{ 1.0f, 0.5f, 0.0f, 0.9f };
        inline bool handchams = false;

        // === SCALE ===
        inline float ScaleX = 1.0f;
        inline float ScaleY = 1.0f;
        inline float ScaleZ = 1.0f;
        inline float WScaleX = 1.0f;
        inline float WScaleY = 1.0f;
        inline float WScaleZ = 1.0f;

        // === HAND CHAMS SETTINGS ===
        inline float handchams_intensity = 2.0f;
        inline float handchams_speed = 1.0f;
        inline float time = 1.0f;
        inline float view = 1;

        // === THIRD PERSON ===
        inline bool tperson = false;
        inline float PlayerDistance = 100;

        // === FREE CAM ===
        inline bool freecam = false;
        inline int freecam_key = 0;
        inline float freecam_speed = 1200.0f;

        // === ASPECT RATIO ===
        inline bool aspectratio = false;
        inline float aspectfloat = 1;
    }


    namespace chams {

        inline bool outline_enabled = false;
        inline bool hand_outline_enabled = false;
        inline bool gun_outline3P_enabled = false;
        inline bool gun_outline1P_enabled = false;
        inline int outlinetype = 0; // 0 = toujours visible, 1 = seulement invisible
        inline float GlowVisible = 100.f;
        inline float GlowInvisible = 10.f;
  

        inline bool enemy_galaxy_enabled = false;
        inline bool self_galaxy_enabled = false;
        inline bool hand_galaxy_enabled = false;
        inline bool gun1p_galaxy_enabled = false;
        inline bool gun3p_galaxy_enabled = false;


        inline int global_galaxy_preset = 0; // 0=Custom, 1=Red Dark, 2=Dark Green, 3=Dark Blue, 4=Dark Orange, 5=Orange, 6=Pink, 7=Black Galaxy, 8=Star Blue, 9=Star Purple

        // === Nebula Chams (Type 1/2/3/4/5 selectable) ===
        inline bool nebula_chams_self = false;
        inline bool nebula_chams_hand = false;
        inline bool nebula_chams_enemy = false;
        inline int nebula_self_type = 0;   // 0 = Type 1, 1 = Type 2, 2 = Type 3, 3 = Type 4 (Invisible Orange-Green), 4 = Type 5 (Visible Blue)
        inline int nebula_hand_type = 0;   // 0 = Type 1, 1 = Type 2, 2 = Type 3, 3 = Type 4 (Invisible Orange-Green), 4 = Type 5 (Visible Blue)
        inline int nebula_enemy_type = 0;  // 0 = Type 1, 1 = Type 2, 2 = Type 3, 3 = Type 4 (Invisible Orange-Green), 4 = Type 5 (Visible Blue)

        // === Galaxy Chams v2 (BountyHunter material based) ===
        inline bool nebula_galaxy_hand = false;
        inline int nebula_galaxy_hand_color = 0; // 0=White, 1=Black, 2=RGB
        inline bool nebula_galaxy_self = false;
        inline int nebula_galaxy_self_color = 0; // 0=White, 1=Black, 2=RGB
        inline bool nebula_galaxy_enemy = false;


        // Sentinel (material) chams — BountyHunter / HunterReveal (enemy + local self)
        inline bool sentinel_chams_visible_enemy = false;
        inline bool sentinel_chams_invisible_enemy = false;
        inline bool sentinel_chams_visible_self = false;
        inline bool sentinel_chams_invisible_self = false;

        // Black Chams — type: 0=Black, 1=RGB, 2=White (black fill + bright white rim)
        inline bool black_chams_enemy = false;
        inline bool black_chams_rgb = false; // legacy config; synced from black_chams_type
        inline int black_chams_type = 0;
        inline float black_chams_intensity = 50.0f;

        // Cool Chams
        inline bool cool_chams_enabled = false;
        inline bool hand_cool_chams_enabled = false;
        inline bool self_cool_chams_enabled = false;
        inline float coolChamsR = 2.093f;
        inline float coolChamsG = 0.019f;
        inline float coolChamsB = 20.0f;
        inline float coolChamsGlow = 10.0f;

        // Fresnel Chams
        inline bool custom_fresnel = false;
        inline bool rainbow_fresnel = false;
        inline float enemy_fresnel_r = 2.093f;
        inline float enemy_fresnel_g = 0.019f;
        inline float enemy_fresnel_b = 20.0f;
        inline float enemy_fresnel_intensity = 10.0f;
        inline float enemy_fresnel_rainbow_speed = 0.005f;

        // Hand Fresnel
        inline bool hand_fresnel_enabled = false;
        inline float hand_fresnel_r = 2.093f;
        inline float hand_fresnel_g = 0.019f;
        inline float hand_fresnel_b = 20.0f;
        inline float hand_fresnel_intensity = 10.0f;

        // Self Fresnel
        inline bool self_fresnel_enabled = false;
        inline float self_fresnel_r = 0.53f;
        inline float self_fresnel_g = 0.27f;
        inline float self_fresnel_b = 0.47f;
        inline float self_fresnel_intensity = 5.0f;

        // FresnelShi (native UFresnelComponent intensity boost)
        inline bool fresnel_shi_enabled = false;
        inline float fresnel_shi_intensity = 500.0f;

        // Self 3P Fresnel
        inline bool self_3p_fresnel_enabled = false;
        inline float self_3p_fresnel_r = 0.53f;
        inline float self_3p_fresnel_g = 0.27f;
        inline float self_3p_fresnel_b = 0.47f;
        inline float self_3p_fresnel_intensity = 5.0f;

        inline float  Glow = 5.0f;
        inline float  Glowvni = 5.0f;

        inline flinearcolor ChamsColor{ 1.0f, 0.0f, 0.0f, 1.0f }; // Red
        inline flinearcolor ChamsColorvni{ 0.5f, 0.0f, 1.0f, 0.9f }; // Purple

        inline int visible_chams_color_index = 0; // 0=Red, 1=Yellow, 2=Green, 3=Blue, 4=Pink, 5=White, 6=Black, 7=Purple
        inline int invisible_chams_color_index = 7;

        inline void apply_simple_chams_colors() {
            auto get_color = [](int index) -> flinearcolor {
                switch (index) {
                case 0: return { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
                case 1: return { 1.0f, 1.0f, 0.0f, 1.0f }; // Yellow
                case 2: return { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
                case 3: return { 0.0f, 0.0f, 1.0f, 1.0f }; // Blue
                case 4: return { 1.0f, 0.0f, 1.0f, 1.0f }; // Pink
                case 5: return { 1.0f, 1.0f, 1.0f, 1.0f }; // White
                case 6: return { 0.05f, 0.05f, 0.05f, 1.0f }; // Black
                case 7: return { 0.5f, 0.0f, 1.0f, 1.0f }; // Purple
                default: return { 1.0f, 0.0f, 0.0f, 1.0f };
                }
            };
            ChamsColor = get_color(visible_chams_color_index);
            ChamsColorvni = get_color(invisible_chams_color_index);
        }

        inline float intensityvisibleoutline = 10.0f * 100.0f;

        inline float intensityinvisbleoutline = 10.0f;

        inline int visible_outline_preset = 0;     // Index du preset visible
        inline int invisible_outline_preset = 0;   // Index du preset invisible

        // Preset 0 - Galaxy Chams (tes valeurs exactes)
        inline float Galaxy_CenterEdgeR_Visible = 0.4f;
        inline float Galaxy_CenterEdgeG_Visible = 0.0f;
        inline float Galaxy_CenterEdgeB_Visible = 0.5f;
        inline float Galaxy_InnerEdgeR_Visible = 0.2f;
        inline float Galaxy_InnerEdgeG_Visible = 0.1f;
        inline float Galaxy_InnerEdgeB_Visible = 0.6f;
        inline float Galaxy_OuterEdgeR_Visible = 0.0f;
        inline float Galaxy_OuterEdgeG_Visible = 0.6f;
        inline float Galaxy_OuterEdgeB_Visible = 1.0f;
        inline float Galaxy_GlowVisible = 20.0f;

        // Preset 1 - Blue Full Outline
        inline float BlueOutline_CenterEdgeR_Visible = 0.0f;
        inline float BlueOutline_CenterEdgeG_Visible = 0.3f;
        inline float BlueOutline_CenterEdgeB_Visible = 0.8f;
        inline float BlueOutline_InnerEdgeR_Visible = 0.1f;
        inline float BlueOutline_InnerEdgeG_Visible = 0.4f;
        inline float BlueOutline_InnerEdgeB_Visible = 0.9f;
        inline float BlueOutline_OuterEdgeR_Visible = 0.0f;
        inline float BlueOutline_OuterEdgeG_Visible = 0.5f;
        inline float BlueOutline_OuterEdgeB_Visible = 1.0f;
        inline float BlueOutline_GlowVisible = 20.0f;

        // Preset 2 - Green Full Outline
        inline float GreenOutline_CenterEdgeR_Visible = 0.1f;
        inline float GreenOutline_CenterEdgeG_Visible = 0.5f;
        inline float GreenOutline_CenterEdgeB_Visible = 0.2f;
        inline float GreenOutline_InnerEdgeR_Visible = 0.2f;
        inline float GreenOutline_InnerEdgeG_Visible = 0.7f;
        inline float GreenOutline_InnerEdgeB_Visible = 0.1f;
        inline float GreenOutline_OuterEdgeR_Visible = 0.0f;
        inline float GreenOutline_OuterEdgeG_Visible = 0.8f;
        inline float GreenOutline_OuterEdgeB_Visible = 0.3f;
        inline float GreenOutline_GlowVisible = 20.0f;

        // Preset 3 - Orange Full Outline
        inline float OrangeOutline_CenterEdgeR_Visible = 0.6f;
        inline float OrangeOutline_CenterEdgeG_Visible = 0.3f;
        inline float OrangeOutline_CenterEdgeB_Visible = 0.1f;
        inline float OrangeOutline_InnerEdgeR_Visible = 0.8f;
        inline float OrangeOutline_InnerEdgeG_Visible = 0.4f;
        inline float OrangeOutline_InnerEdgeB_Visible = 0.0f;
        inline float OrangeOutline_OuterEdgeR_Visible = 1.0f;
        inline float OrangeOutline_OuterEdgeG_Visible = 0.5f;
        inline float OrangeOutline_OuterEdgeB_Visible = 0.2f;
        inline float OrangeOutline_GlowVisible = 20.0f;

        // Preset 4 - Pink Full Outline
        inline float PinkOutline_CenterEdgeR_Visible = 0.7f;
        inline float PinkOutline_CenterEdgeG_Visible = 0.2f;
        inline float PinkOutline_CenterEdgeB_Visible = 0.5f;
        inline float PinkOutline_InnerEdgeR_Visible = 0.8f;
        inline float PinkOutline_InnerEdgeG_Visible = 0.1f;
        inline float PinkOutline_InnerEdgeB_Visible = 0.6f;
        inline float PinkOutline_OuterEdgeR_Visible = 0.9f;
        inline float PinkOutline_OuterEdgeG_Visible = 0.3f;
        inline float PinkOutline_OuterEdgeB_Visible = 0.7f;
        inline float PinkOutline_GlowVisible = 20.0f;

        // Preset 5 - White Full Outline
        inline float WhiteOutline_CenterEdgeR_Visible = 0.7f;
        inline float WhiteOutline_CenterEdgeG_Visible = 0.7f;
        inline float WhiteOutline_CenterEdgeB_Visible = 0.7f;
        inline float WhiteOutline_InnerEdgeR_Visible = 0.8f;
        inline float WhiteOutline_InnerEdgeG_Visible = 0.8f;
        inline float WhiteOutline_InnerEdgeB_Visible = 0.8f;
        inline float WhiteOutline_OuterEdgeR_Visible = 0.9f;
        inline float WhiteOutline_OuterEdgeG_Visible = 0.9f;
        inline float WhiteOutline_OuterEdgeB_Visible = 0.9f;
        inline float WhiteOutline_GlowVisible = 20.0f;

        inline float GalaxyBlackBlue_CenterEdgeR_Visible = 0.0f;     // Noir pur
        inline float GalaxyBlackBlue_CenterEdgeG_Visible = 0.0f;
        inline float GalaxyBlackBlue_CenterEdgeB_Visible = 0.0f;
        inline float GalaxyBlackBlue_InnerEdgeR_Visible = 0.4f;      // Bleu dark plus intense
        inline float GalaxyBlackBlue_InnerEdgeG_Visible = 0.3f;
        inline float GalaxyBlackBlue_InnerEdgeB_Visible = 0.7f;
        inline float GalaxyBlackBlue_OuterEdgeR_Visible = 0.0f;      // Noir
        inline float GalaxyBlackBlue_OuterEdgeG_Visible = 0.0f;
        inline float GalaxyBlackBlue_OuterEdgeB_Visible = 0.0f;
        inline float GalaxyBlackBlue_GlowVisible = 65.0f;            // Glow moyenne

        // Preset 7 - Purple Full Outline (Visible)
        inline float PurpleOutline_CenterEdgeR_Visible = 0.5f;
        inline float PurpleOutline_CenterEdgeG_Visible = 0.0f;
        inline float PurpleOutline_CenterEdgeB_Visible = 1.0f;
        inline float PurpleOutline_InnerEdgeR_Visible = 0.8f;
        inline float PurpleOutline_InnerEdgeG_Visible = 0.2f;
        inline float PurpleOutline_InnerEdgeB_Visible = 1.0f;
        inline float PurpleOutline_OuterEdgeR_Visible = 0.5f;
        inline float PurpleOutline_OuterEdgeG_Visible = 0.0f;
        inline float PurpleOutline_OuterEdgeB_Visible = 1.0f;
        inline float PurpleOutline_GlowVisible = 30.0f;

        // === PRESETS POUR ENNEMIS INVISIBLES ===

        // Preset 0 - Red Full Outline (tes valeurs exactes)
        inline float RedOutline_CenterEdgeR_Invisible = 0.01f;
        inline float RedOutline_CenterEdgeG_Invisible = 0.0f;
        inline float RedOutline_CenterEdgeB_Invisible = 0.0f;
        inline float RedOutline_InnerEdgeR_Invisible = 0.1f;
        inline float RedOutline_InnerEdgeG_Invisible = 0.0f;
        inline float RedOutline_InnerEdgeB_Invisible = 0.0f;
        inline float RedOutline_OuterEdgeR_Invisible = 1.0f;
        inline float RedOutline_OuterEdgeG_Invisible = 0.0f;
        inline float RedOutline_OuterEdgeB_Invisible = 0.0f;
        inline float RedOutline_GlowInvisible = 15.0f;

        // Preset 1 - Orange Full Outline
        inline float OrangeOutline_CenterEdgeR_Invisible = 0.3f;
        inline float OrangeOutline_CenterEdgeG_Invisible = 0.1f;
        inline float OrangeOutline_CenterEdgeB_Invisible = 0.0f;
        inline float OrangeOutline_InnerEdgeR_Invisible = 0.5f;
        inline float OrangeOutline_InnerEdgeG_Invisible = 0.2f;
        inline float OrangeOutline_InnerEdgeB_Invisible = 0.0f;
        inline float OrangeOutline_OuterEdgeR_Invisible = 0.8f;
        inline float OrangeOutline_OuterEdgeG_Invisible = 0.4f;
        inline float OrangeOutline_OuterEdgeB_Invisible = 0.1f;
        inline float OrangeOutline_GlowInvisible = 15.0f;

        // Preset 2 - Yellow Full Outline
        inline float YellowOutline_CenterEdgeR_Invisible = 0.4f;
        inline float YellowOutline_CenterEdgeG_Invisible = 0.4f;
        inline float YellowOutline_CenterEdgeB_Invisible = 0.0f;
        inline float YellowOutline_InnerEdgeR_Invisible = 0.6f;
        inline float YellowOutline_InnerEdgeG_Invisible = 0.6f;
        inline float YellowOutline_InnerEdgeB_Invisible = 0.1f;
        inline float YellowOutline_OuterEdgeR_Invisible = 0.8f;
        inline float YellowOutline_OuterEdgeG_Invisible = 0.8f;
        inline float YellowOutline_OuterEdgeB_Invisible = 0.2f;
        inline float YellowOutline_GlowInvisible = 15.0f;

        // Preset 3 - Green Full Outline
        inline float GreenOutline_CenterEdgeR_Invisible = 0.1f;
        inline float GreenOutline_CenterEdgeG_Invisible = 0.3f;
        inline float GreenOutline_CenterEdgeB_Invisible = 0.0f;
        inline float GreenOutline_InnerEdgeR_Invisible = 0.2f;
        inline float GreenOutline_InnerEdgeG_Invisible = 0.5f;
        inline float GreenOutline_InnerEdgeB_Invisible = 0.1f;
        inline float GreenOutline_OuterEdgeR_Invisible = 0.3f;
        inline float GreenOutline_OuterEdgeG_Invisible = 0.7f;
        inline float GreenOutline_OuterEdgeB_Invisible = 0.2f;
        inline float GreenOutline_GlowInvisible = 15.0f;

        // Preset 4 - Pink Full Outline
        inline float PinkOutline_CenterEdgeR_Invisible = 0.4f;
        inline float PinkOutline_CenterEdgeG_Invisible = 0.1f;
        inline float PinkOutline_CenterEdgeB_Invisible = 0.3f;
        inline float PinkOutline_InnerEdgeR_Invisible = 0.6f;
        inline float PinkOutline_InnerEdgeG_Invisible = 0.0f;
        inline float PinkOutline_InnerEdgeB_Invisible = 0.4f;
        inline float PinkOutline_OuterEdgeR_Invisible = 0.8f;
        inline float PinkOutline_OuterEdgeG_Invisible = 0.2f;
        inline float PinkOutline_OuterEdgeB_Invisible = 0.6f;
        inline float PinkOutline_GlowInvisible = 15.0f;

        // Preset 5 - Gray Full Outline
        inline float GrayOutline_CenterEdgeR_Invisible = 0.2f;
        inline float GrayOutline_CenterEdgeG_Invisible = 0.2f;
        inline float GrayOutline_CenterEdgeB_Invisible = 0.2f;
        inline float GrayOutline_InnerEdgeR_Invisible = 0.4f;
        inline float GrayOutline_InnerEdgeG_Invisible = 0.4f;
        inline float GrayOutline_InnerEdgeB_Invisible = 0.4f;
        inline float GrayOutline_OuterEdgeR_Invisible = 0.6f;
        inline float GrayOutline_OuterEdgeG_Invisible = 0.6f;
        inline float GrayOutline_OuterEdgeB_Invisible = 0.6f;
        inline float GrayOutline_GlowInvisible = 15.0f;

        inline float GalaxyBlackBlue_CenterEdgeR_Invisible = 0.0f;   // Noir pur
        inline float GalaxyBlackBlue_CenterEdgeG_Invisible = 0.0f;
        inline float GalaxyBlackBlue_CenterEdgeB_Invisible = 0.0f;
        inline float GalaxyBlackBlue_InnerEdgeR_Invisible = 0.15f;   // Bleu dark un peu plus brillant
        inline float GalaxyBlackBlue_InnerEdgeG_Invisible = 0.35f;
        inline float GalaxyBlackBlue_InnerEdgeB_Invisible = 0.8f;
        inline float GalaxyBlackBlue_OuterEdgeR_Invisible = 0.0f;    // Noir
        inline float GalaxyBlackBlue_OuterEdgeG_Invisible = 0.0f;
        inline float GalaxyBlackBlue_OuterEdgeB_Invisible = 0.0f;
        inline float GalaxyBlackBlue_GlowInvisible = 40.0f;          // Glow moyenne

        // Preset 7 - Purple Full Outline (Invisible)
        inline float PurpleOutline_CenterEdgeR_Invisible = 0.5f;
        inline float PurpleOutline_CenterEdgeG_Invisible = 0.0f;
        inline float PurpleOutline_CenterEdgeB_Invisible = 1.0f;
        inline float PurpleOutline_InnerEdgeR_Invisible = 0.8f;
        inline float PurpleOutline_InnerEdgeG_Invisible = 0.2f;
        inline float PurpleOutline_InnerEdgeB_Invisible = 1.0f;
        inline float PurpleOutline_OuterEdgeR_Invisible = 0.5f;
        inline float PurpleOutline_OuterEdgeG_Invisible = 0.0f;
        inline float PurpleOutline_OuterEdgeB_Invisible = 1.0f;
        inline float PurpleOutline_GlowInvisible = 30.0f;

        // === VARIABLES ACTIVES (celles utilisÃ©es par apply_outline_chams) ===
        inline float CenterEdgeR_Visible = 0.5f;
        inline float CenterEdgeG_Visible = 0.0f;
        inline float CenterEdgeB_Visible = 1.0f;
        inline float InnerEdgeR_Visible = 0.8f;
        inline float InnerEdgeG_Visible = 0.2f;
        inline float InnerEdgeB_Visible = 1.0f;
        inline float OuterEdgeR_Visible = 0.2f;
        inline float OuterEdgeG_Visible = 0.0f;
        inline float OuterEdgeB_Visible = 0.8f;

        inline float CenterEdgeR_Invisible = 1.0f;
        inline float CenterEdgeG_Invisible = 0.0f;
        inline float CenterEdgeB_Invisible = 0.0f;
        inline float InnerEdgeR_Invisible = 0.8f;
        inline float InnerEdgeG_Invisible = 0.1f;
        inline float InnerEdgeB_Invisible = 0.0f;
        inline float OuterEdgeR_Invisible = 1.0f;
        inline float OuterEdgeG_Invisible = 0.2f;
        inline float OuterEdgeB_Invisible = 0.0f;

        inline void apply_visible_preset() {
            switch (visible_outline_preset) {
            case 0: // Galaxy Chams (tes valeurs exactes)
                CenterEdgeR_Visible = Galaxy_CenterEdgeR_Visible;
                CenterEdgeG_Visible = Galaxy_CenterEdgeG_Visible;
                CenterEdgeB_Visible = Galaxy_CenterEdgeB_Visible;
                InnerEdgeR_Visible = Galaxy_InnerEdgeR_Visible;
                InnerEdgeG_Visible = Galaxy_InnerEdgeG_Visible;
                InnerEdgeB_Visible = Galaxy_InnerEdgeB_Visible;
                OuterEdgeR_Visible = Galaxy_OuterEdgeR_Visible;
                OuterEdgeG_Visible = Galaxy_OuterEdgeG_Visible;
                OuterEdgeB_Visible = Galaxy_OuterEdgeB_Visible;
                GlowVisible = Galaxy_GlowVisible;
                break;
            case 1: // Blue Full Outline
                CenterEdgeR_Visible = BlueOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = BlueOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = BlueOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = BlueOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = BlueOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = BlueOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = BlueOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = BlueOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = BlueOutline_OuterEdgeB_Visible;
                GlowVisible = BlueOutline_GlowVisible;
                break;
            case 2: // Green Full Outline
                CenterEdgeR_Visible = GreenOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = GreenOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = GreenOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = GreenOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = GreenOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = GreenOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = GreenOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = GreenOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = GreenOutline_OuterEdgeB_Visible;
                GlowVisible = GreenOutline_GlowVisible;
                break;
            case 3: // Orange Full Outline
                CenterEdgeR_Visible = OrangeOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = OrangeOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = OrangeOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = OrangeOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = OrangeOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = OrangeOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = OrangeOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = OrangeOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = OrangeOutline_OuterEdgeB_Visible;
                GlowVisible = OrangeOutline_GlowVisible;
                break;
            case 4: // Pink Full Outline
                CenterEdgeR_Visible = PinkOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = PinkOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = PinkOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = PinkOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = PinkOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = PinkOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = PinkOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = PinkOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = PinkOutline_OuterEdgeB_Visible;
                GlowVisible = PinkOutline_GlowVisible;
                break;
            case 5: // White Full Outline
                CenterEdgeR_Visible = WhiteOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = WhiteOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = WhiteOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = WhiteOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = WhiteOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = WhiteOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = WhiteOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = WhiteOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = WhiteOutline_OuterEdgeB_Visible;
                GlowVisible = WhiteOutline_GlowVisible;
                break;
            case 6: // NEW: Galaxy Black Blue (Visible)
                CenterEdgeR_Visible = GalaxyBlackBlue_CenterEdgeR_Visible;
                CenterEdgeG_Visible = GalaxyBlackBlue_CenterEdgeG_Visible;
                CenterEdgeB_Visible = GalaxyBlackBlue_CenterEdgeB_Visible;
                InnerEdgeR_Visible = GalaxyBlackBlue_InnerEdgeR_Visible;
                InnerEdgeG_Visible = GalaxyBlackBlue_InnerEdgeG_Visible;
                InnerEdgeB_Visible = GalaxyBlackBlue_InnerEdgeB_Visible;
                OuterEdgeR_Visible = GalaxyBlackBlue_OuterEdgeR_Visible;
                OuterEdgeG_Visible = GalaxyBlackBlue_OuterEdgeG_Visible;
                OuterEdgeB_Visible = GalaxyBlackBlue_OuterEdgeB_Visible;
                GlowVisible = GalaxyBlackBlue_GlowVisible;
                break;
            case 7: // Purple Full Outline
                CenterEdgeR_Visible = PurpleOutline_CenterEdgeR_Visible;
                CenterEdgeG_Visible = PurpleOutline_CenterEdgeG_Visible;
                CenterEdgeB_Visible = PurpleOutline_CenterEdgeB_Visible;
                InnerEdgeR_Visible = PurpleOutline_InnerEdgeR_Visible;
                InnerEdgeG_Visible = PurpleOutline_InnerEdgeG_Visible;
                InnerEdgeB_Visible = PurpleOutline_InnerEdgeB_Visible;
                OuterEdgeR_Visible = PurpleOutline_OuterEdgeR_Visible;
                OuterEdgeG_Visible = PurpleOutline_OuterEdgeG_Visible;
                OuterEdgeB_Visible = PurpleOutline_OuterEdgeB_Visible;
                GlowVisible = PurpleOutline_GlowVisible;
                break;
            }
        }

        inline void apply_invisible_preset() {
            switch (invisible_outline_preset) {
            case 0: // Red Full Outline (tes valeurs exactes)
                CenterEdgeR_Invisible = RedOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = RedOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = RedOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = RedOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = RedOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = RedOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = RedOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = RedOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = RedOutline_OuterEdgeB_Invisible;
                GlowInvisible = RedOutline_GlowInvisible;
                break;
            case 1: // Orange Full Outline
                CenterEdgeR_Invisible = OrangeOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = OrangeOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = OrangeOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = OrangeOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = OrangeOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = OrangeOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = OrangeOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = OrangeOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = OrangeOutline_OuterEdgeB_Invisible;
                GlowInvisible = OrangeOutline_GlowInvisible;
                break;
            case 2: // Yellow Full Outline
                CenterEdgeR_Invisible = YellowOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = YellowOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = YellowOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = YellowOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = YellowOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = YellowOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = YellowOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = YellowOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = YellowOutline_OuterEdgeB_Invisible;
                GlowInvisible = YellowOutline_GlowInvisible;
                break;
            case 3: // Green Full Outline
                CenterEdgeR_Invisible = GreenOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = GreenOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = GreenOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = GreenOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = GreenOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = GreenOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = GreenOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = GreenOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = GreenOutline_OuterEdgeB_Invisible;
                GlowInvisible = GreenOutline_GlowInvisible;
                break;
            case 4: // Pink Full Outline
                CenterEdgeR_Invisible = PinkOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = PinkOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = PinkOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = PinkOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = PinkOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = PinkOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = PinkOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = PinkOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = PinkOutline_OuterEdgeB_Invisible;
                GlowInvisible = PinkOutline_GlowInvisible;
                break;
            case 5: // Gray Full Outline
                CenterEdgeR_Invisible = GrayOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = GrayOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = GrayOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = GrayOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = GrayOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = GrayOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = GrayOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = GrayOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = GrayOutline_OuterEdgeB_Invisible;
                GlowInvisible = GrayOutline_GlowInvisible;
                break;
            case 6: // NEW: Galaxy Black Blue (Invisible)
                CenterEdgeR_Invisible = GalaxyBlackBlue_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = GalaxyBlackBlue_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = GalaxyBlackBlue_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = GalaxyBlackBlue_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = GalaxyBlackBlue_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = GalaxyBlackBlue_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = GalaxyBlackBlue_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = GalaxyBlackBlue_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = GalaxyBlackBlue_OuterEdgeB_Invisible;
                GlowInvisible = GalaxyBlackBlue_GlowInvisible;
                break;
            case 7: // Purple Full Outline
                CenterEdgeR_Invisible = PurpleOutline_CenterEdgeR_Invisible;
                CenterEdgeG_Invisible = PurpleOutline_CenterEdgeG_Invisible;
                CenterEdgeB_Invisible = PurpleOutline_CenterEdgeB_Invisible;
                InnerEdgeR_Invisible = PurpleOutline_InnerEdgeR_Invisible;
                InnerEdgeG_Invisible = PurpleOutline_InnerEdgeG_Invisible;
                InnerEdgeB_Invisible = PurpleOutline_InnerEdgeB_Invisible;
                OuterEdgeR_Invisible = PurpleOutline_OuterEdgeR_Invisible;
                OuterEdgeG_Invisible = PurpleOutline_OuterEdgeG_Invisible;
                OuterEdgeB_Invisible = PurpleOutline_OuterEdgeB_Invisible;
                GlowInvisible = PurpleOutline_GlowInvisible;
                break;
            }
        }

        inline bool self_chams = false;
        inline bool chamsesp = false;
        inline bool enable_glow = false;
        inline bool rchamsesp = false;
        inline bool rchamsespall = false;
        inline float glow = 1.f;
        inline float Intenisty_fresnelz = 50;
        inline float GlowFloat = 1.0f;
        inline flinearcolor fresnel_color = flinearcolor(0.f, 26.0f, 0.f, GlowFloat);
        inline bool fresnelz = false;
    }

    namespace buddy {
        inline bool enabled = false;
        inline int index = 0;
    }

    namespace crosshair {
        inline bool rainbow_crosshair = false;
    }
}



