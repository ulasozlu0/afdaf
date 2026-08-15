#include "Settings.hpp"
#include "../Core/Globals.hpp"
#include <ShlObj.h>
#include <filesystem>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <climits>
#include <cmath>

using namespace std;

namespace {

std::string trim_ws(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) a++;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
    return s.substr(a, b - a);
}

size_t find_json_scalar_end(const std::string& content, size_t pos) {
    while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\t')) pos++;
    size_t end = pos;
    while (end < content.length()) {
        char c = content[end];
        if (c == ',' || c == '}' || c == '\n' || c == '\r') break;
        end++;
    }
    return end;
}

void ClampConfigGlobalsAfterLoad() {
    using namespace globals;
    auto clamp_i = [](int& v, int lo, int hi) {
        if (v < lo) v = lo;
        else if (v > hi) v = hi;
    };
    clamp_i(misc::hellfire_amount, 1, 40);
    clamp_i(misc::lightning_count, 1, 15);
    clamp_i(misc::spam_count, 1, 50);
    clamp_i(misc::bullet_spawn_amount, 1, 50);
    clamp_i(buddy::index, 0, 658);
    clamp_i(aimbot::auto_shot_delay_mode, 0, 3);
}

} // namespace

CConfig* Config = new CConfig();

void CConfig::AddItem(void* pointer, const char* name, const std::string& type) {
    items.push_back(new C_ConfigItem(std::string(name), pointer, type));
}

void CConfig::setup_item(int* pointer, int value, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("int"));
    *pointer = value;
}

void CConfig::setup_item(bool* pointer, bool value, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("bool"));
    *pointer = value;
}

void CConfig::setup_item(float* pointer, float value, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("float"));
    *pointer = value;
}

void CConfig::setup_item(double* pointer, double value, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("double"));
    *pointer = value;
}

void CConfig::setup_item(std::vector< int >* pointer, int size, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("vector<int>"));
    pointer->clear();
    for (int i = 0; i < size; i++)
        pointer->push_back(FALSE);
}

void CConfig::setup_item(std::vector< std::string >* pointer, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("vector<string>"));
}

void CConfig::setup_item(std::string* pointer, const std::string& value, const std::string& name)
{
    AddItem(pointer, name.c_str(), ("string"));
    *pointer = value;
}



void CConfig::SetupAimbot() {
    // AIMBOT TAB - Main subtab
    setup_item(&globals::aimbot::a1mbot, false, ("aimbot_enabled"));
    setup_item(&globals::aimbot::auto_wall, false, ("aimbot_autowall"));
    
    setup_item(&globals::aimbot::silent, false, ("aimbot_silent"));
    setup_item(&globals::aimbot::v1sh_ch3ck, false, ("aimbot_visible_check"));
    setup_item(&globals::aimbot::v1sh_ch3ck_3p, false, ("aimbot_visible_check_3p"));
    setup_item(&globals::aimbot::reco1l_contr0l, false, ("aimbot_recoil"));
    setup_item(&globals::aimbot::nospread, false, ("aimbot_nospread"));
    setup_item(&globals::aimbot::draw_f0v, false, ("aimbot_draw_fov"));
    setup_item(&globals::aimbot::enable_360_fov, false, ("aimbot_360_fov"));
    setup_item(&globals::aimbot::a1m_f0v, 70.0f, ("aimbot_fov"));
    setup_item(&globals::aimbot::a1m_sm00th, 15.0f, ("aimbot_smooth"));
    setup_item(&globals::aimbot::a1m_b0ne, 0, ("aimbot_bone"));
    setup_item(&globals::aimbot::autowall_b0ne, 0, ("aimbot_autowall_bone"));
    setup_item(&globals::aimbot::closest_bone, false, ("aimbot_closest_bone"));
    setup_item(&globals::aimbot::a1m_k3y, 0, ("aimbot_key"));

    // AIMBOT TAB - Auto Shot subtab
    setup_item(&globals::aimbot::auto_shot, false, ("aimbot_auto_shot"));
    setup_item(&globals::aimbot::auto_shot_hold_key, false, ("aimbot_auto_shot_hold_key"));
    setup_item(&globals::aimbot::nospread_lock_delay, 50.0f, ("aimbot_nospread_lock_delay"));
    setup_item(&globals::aimbot::auto_shot_delay_ms, 0.0f, ("aimbot_auto_shot_delay_ms"));

    // AIMBOT TAB - Cooldown gate
    setup_item(&globals::aimbot::nodelay, false, ("aimbot_nodelay"));
    setup_item(&globals::aimbot::CooldownChecks, 0.05f, ("aimbot_cooldown_checks"));

    // AIMBOT TAB - Prediction
    setup_item(&globals::aimbot::prediction, false, ("aimbot_prediction"));
    setup_item(&globals::aimbot::resolver, false, ("aimbot_resolver"));
    setup_item(&globals::aimbot::aim_lock, false, ("aimbot_lock"));
    setup_item(&globals::aimbot::auto_shot_delay_mode, 0, ("aimbot_auto_shot_delay_mode"));
    setup_item(&globals::aimbot::auto_shot_custom_delay_ms, 100.0f, ("aimbot_auto_shot_custom_delay_ms"));
    setup_item(&globals::aimbot::max_shots_per_second, 4.0f, ("aimbot_max_shots_per_second"));
    setup_item(&globals::aimbot::advanced_timing, false, ("aimbot_advanced_timing"));
}

void CConfig::SetupVisuals() {
    // VISUAL TAB - ESP subtab
    setup_item(&globals::visuals::filled_box, false, ("visuals_filled_box"));
    setup_item(&globals::visuals::filled_box_alpha, 0.15f, ("visuals_filled_box_alpha"));
    setup_item(&globals::visuals::box_health, false, ("visuals_box_health"));
    setup_item(&globals::visuals::box2d, false, ("visuals_box2d"));
    setup_item(&globals::visuals::box2d5, false, ("visuals_box2d5"));
    setup_item(&globals::misc::Wireframe, false, ("misc_wireframe"));
    setup_item(&globals::misc::self_wireframe, false, ("misc_self_wireframe"));
    setup_item(&globals::visuals::box3d, false, ("visuals_box3d"));
    setup_item(&globals::misc::HandWire, false, ("misc_handwire"));
    setup_item(&globals::visuals::hand_with_material, false, ("visuals_hand_material"));
    setup_item(&globals::visuals::corner, false, ("visuals_corner"));
    setup_item(&globals::misc::WireframeGun, false, ("misc_wireframe_gun"));
    setup_item(&globals::visuals::gunmaterial1p, false, ("visuals_gunmaterial1p"));
    setup_item(&globals::visuals::vischeck, false, ("visuals_vischeck"));
    setup_item(&globals::visuals::gunmaterial3p, false, ("visuals_gunmaterial3p"));
    setup_item(&globals::misc::bullet_tracers, false, ("misc_bullettracer"));
    setup_item(&globals::misc::pulse_trace, false, ("misc_pulse_trace"));
    setup_item(&globals::misc::pulse_trace_rgb, false, ("misc_pulse_trace_rgb"));
    setup_item(&globals::misc::pulse_trace_lifetime, 2.7f, ("misc_pulse_trace_life"));
    setup_item(&globals::misc::emote_load, false, ("misc_emote_load"));
    setup_item(&globals::misc::emote_load_bind, 0, ("misc_emote_load_bind"));
    setup_item(&globals::misc::emote2_load, false, ("misc_emote2_load"));
    setup_item(&globals::misc::emote2_load_bind, 0, ("misc_emote2_load_bind"));
    setup_item(&globals::misc::emote3_load, false, ("misc_emote3_load"));
    setup_item(&globals::misc::emote3_load_bind, 0, ("misc_emote3_load_bind"));
    setup_item(&globals::visuals::headb0x, false, ("visuals_headbox"));
    setup_item(&globals::misc::playerchamsself, false, ("misc_player_chams_self"));
    setup_item(&globals::visuals::chinahat, false, ("visuals_chinahat"));
    setup_item(&globals::visuals::b00ms, false, ("visuals_agent_icon"));
    setup_item(&globals::visuals::partyhat_self, false, ("visuals_partyhat_self"));
    setup_item(&globals::visuals::spike, false, ("visuals_spike"));
    setup_item(&globals::visuals::h3althbar, false, ("visuals_healthbar"));
    setup_item(&globals::visuals::snapl1ne, false, ("visuals_snapline"));
    setup_item(&globals::visuals::sk3let0n, false, ("visuals_skeleton"));
    setup_item(&globals::misc::gun_3p_wireframe, false, ("misc_gun_3p_wireframe"));
    setup_item(&globals::misc::MoscaWireframe, false, ("misc_mosca_wireframe"));
    setup_item(&globals::misc::MoscaRed, 1.0f, ("misc_mosca_red"));
    setup_item(&globals::misc::MoscaGreen, 1.0f, ("misc_mosca_green"));
    setup_item(&globals::misc::MoscaBlue, 1.0f, ("misc_mosca_blue"));
    setup_item(&globals::misc::MoscaGlow, 1.0f, ("misc_mosca_glow"));
    setup_item(&globals::misc::MoscaOpacity, 0.5f, ("misc_mosca_opacity"));
    setup_item(&globals::misc::MoscaWireColor[0], 0.0f, ("misc_mosca_wire_r"));
    setup_item(&globals::misc::MoscaWireColor[1], 0.5f, ("misc_mosca_wire_g"));
    setup_item(&globals::misc::MoscaWireColor[2], 1.0f, ("misc_mosca_wire_b"));
    setup_item(&globals::visuals::outline_esp, false, ("visuals_outline_esp"));
    setup_item(&globals::visuals::outline_style, 1, ("visuals_outline_style"));
    setup_item(&globals::visuals::outline_hull_padding, 8.0f, ("visuals_outline_hull_pad"));
    setup_item(&globals::misc::MoscaFresnel, 1.0f, ("misc_mosca_fresnel"));
    setup_item(&globals::misc::MoscaMetallic, 1.0f, ("misc_mosca_metallic"));
    setup_item(&globals::misc::MoscaSpecTile, 1.0f, ("misc_mosca_spectile"));
    setup_item(&globals::misc::MoscaAOClamp, 1.0f, ("misc_mosca_aoclamp"));
    setup_item(&globals::visuals::b11ms, false, ("visuals_weapon_info"));
    setup_item(&globals::visuals::dstc, false, ("visuals_distance"));

    // VISUAL TAB - Materials subtab
    setup_item(&globals::visuals::typehand, 0, ("visuals_typehand"));
    setup_item(&globals::visuals::typegun1p, 0, ("visuals_typegun1p"));
    setup_item(&globals::visuals::typegun3d, 0, ("visuals_typegun3d"));
    setup_item(&globals::misc::chams_material_index, 0, ("misc_chams_material_index"));

    // VISUAL TAB - World subtab
    setup_item(&globals::misc::skybox, false, ("misc_skybox"));
    setup_item(&globals::misc::skyboxrgb, false, ("misc_skybox_rgb"));
    setup_item(&globals::misc::StarsBrightness, 1.0f, ("misc_stars_brightness"));
    setup_item(&globals::misc::CloudSpeed, 1.0f, ("misc_cloud_speed"));
    setup_item(&globals::misc::CloudOpacity, 1.0f, ("misc_cloud_opacity"));
    setup_item(&globals::misc::SkySharedR, 1.0f, ("misc_sky_shared_r"));
    setup_item(&globals::misc::SkySharedG, 1.0f, ("misc_sky_shared_g"));
    setup_item(&globals::misc::SkySharedB, 1.0f, ("misc_sky_shared_b"));

    // VISUAL TAB - Fog subtab
    setup_item(&globals::misc::Fog, false, ("misc_fog"));
    setup_item(&globals::misc::FogRGB, false, ("misc_fog_rgb"));
    setup_item(&globals::misc::bEnableVolumetricFog, false, ("misc_volumetric_fog"));
    setup_item(&globals::misc::FogDensity, 0.02f, ("misc_fog_density"));
    setup_item(&globals::misc::FogHeightFalloff, 0.2f, ("misc_fog_height_falloff"));
    setup_item(&globals::misc::FogMaxOpacity, 1.0f, ("misc_fog_max_opacity"));
    setup_item(&globals::misc::FogStartDistance, 0.0f, ("misc_fog_start_distance"));
    setup_item(&globals::misc::FogCutoffDistance, 0.0f, ("misc_fog_cutoff_distance"));
    setup_item(&globals::misc::VolumetricFogDistance, 6000.0f, ("misc_volumetric_fog_distance"));
    setup_item(&globals::misc::FogColor.r, 0.5f, ("misc_fog_color_r"));
    setup_item(&globals::misc::FogColor.g, 0.5f, ("misc_fog_color_g"));
    setup_item(&globals::misc::FogColor.b, 0.5f, ("misc_fog_color_b"));

    // VISUAL TAB - Extras
    setup_item(&globals::visuals::draw_star, false, ("visuals_draw_star"));
    setup_item(&globals::visuals::draw_checkmate, false, ("visuals_draw_checkmate"));
    setup_item(&globals::visuals::b0x, false, ("visuals_box_esp"));
    setup_item(&globals::visuals::b0x_type, 1, ("visuals_box_type"));
    setup_item(&globals::visuals::snapos, 0, ("visuals_snap_pos"));
    setup_item(&globals::visuals::radar, false, ("visuals_radar"));
    setup_item(&globals::visuals::rotating_text_enabled, false, ("visuals_rotating_text"));
    setup_item(&globals::visuals::rotating_text_speed, 0.12f, ("visuals_rotating_text_speed"));
    setup_item(&globals::visuals::rotating_text_radius, 135.0f, ("visuals_rotating_text_radius"));
}

void CConfig::SetupChams() {
    // CHAMS TAB - Outline Chams subtab
    setup_item(&globals::chams::outline_enabled, false, ("chams_outline_enabled"));
    setup_item(&globals::chams::hand_outline_enabled, false, ("chams_hand_outline_enabled"));

    setup_item(&globals::chams::self_chams, false, ("chams_self_chams"));
    setup_item(&globals::chams::gun_outline3P_enabled, false, ("chams_gun3P_outline_enabled"));
    setup_item(&globals::chams::gun_outline1P_enabled, false, ("chams_gun1P_outline_enabled"));

    setup_item(&globals::chams::intensityvisibleoutline, 50.0f, ("chams_intensity_visible"));
    setup_item(&globals::chams::intensityinvisbleoutline, 50.0f, ("chams_intensity_invisible"));
    setup_item(&globals::chams::outlinetype, 0, ("chams_outline_type"));
    setup_item(&globals::chams::visible_outline_preset, 0, ("chams_visible_outline_preset"));
    setup_item(&globals::chams::invisible_outline_preset, 0, ("chams_invisible_outline_preset"));

    // CHAMS TAB - Visible/Invisible Chams toggle
    setup_item(&globals::chams::chamsesp, false, ("chams_visible_chams"));
    setup_item(&globals::chams::rchamsesp, false, ("chams_invisible_chams"));
    setup_item(&globals::chams::visible_chams_color_index, 0, ("chams_visible_color_index"));
    setup_item(&globals::chams::invisible_chams_color_index, 7, ("chams_invisible_color_index"));

    // CHAMS TAB - Visible Chams subtab
    setup_item(&globals::chams::rchamsespall, false, ("chams_rainbow_all"));
    setup_item(&globals::chams::Glow, 1.0f, ("chams_glow"));
    setup_item(&globals::chams::ChamsColor.r, 1.0f, ("chams_glow_visible_r"));
    setup_item(&globals::chams::ChamsColor.g, 0.0f, ("chams_glow_visible_g"));
    setup_item(&globals::chams::ChamsColor.b, 0.0f, ("chams_glow_visible_b"));

    // CHAMS TAB - Invisible Chams subtab
    setup_item(&globals::chams::rchamsesp, false, ("chams_rainbow"));
    setup_item(&globals::chams::Glowvni, 1.0f, ("chams_glow_invisible"));
    setup_item(&globals::chams::ChamsColorvni.r, 1.0f, ("chams_glow_invisible_r"));
    setup_item(&globals::chams::ChamsColorvni.g, 0.5f, ("chams_glow_invisible_g"));
    setup_item(&globals::chams::ChamsColorvni.b, 0.0f, ("chams_glow_invisible_b"));

    // CHAMS TAB - Hand Chams subtab
    setup_item(&globals::misc::materials, 0, ("misc_materials"));
    setup_item(&globals::misc::handchams, false, ("misc_handchams"));
    setup_item(&globals::misc::HandChamsRbg, false, ("misc_handchams_rgb"));
    setup_item(&globals::misc::handbright, 1.0f, ("misc_handbright"));
    setup_item(&globals::misc::handglow, false, ("misc_handglow"));
    setup_item(&globals::visuals::crystal_chams_enabled, false, ("chams_crystal_enabled"));
    setup_item(&globals::visuals::crystal_chams_preset, 5, ("chams_crystal_preset"));
    setup_item(&globals::visuals::enemy_crystal_chams_preset, 6, ("chams_enemy_crystal_preset"));
    setup_item(&globals::visuals::crystal_chams_self, false, ("chams_crystal_self"));
    setup_item(&globals::visuals::crystal_chams_hand, false, ("chams_crystal_hand"));
    setup_item(&globals::visuals::crystal_chams_enemy, false, ("chams_crystal_enemy"));
    setup_item(&globals::visuals::crystal_chams_self_preset, 0, ("chams_crystal_self_preset"));
    setup_item(&globals::visuals::crystal_chams_hand_preset, 0, ("chams_crystal_hand_preset"));
    setup_item(&globals::visuals::crystal_chams_enemy_preset, 0, ("chams_crystal_enemy_preset"));

    // CHAMS TAB - Galaxy Chams toggles
    setup_item(&globals::chams::hand_galaxy_enabled, false, ("chams_hand_galaxy_enabled"));
    setup_item(&globals::chams::self_galaxy_enabled, false, ("chams_self_galaxy_enabled"));
    setup_item(&globals::chams::enemy_galaxy_enabled, false, ("chams_enemy_galaxy_enabled"));

    // CHAMS TAB - Nebula Chams (Type 1/2/3/4/5 selectable)
    setup_item(&globals::chams::nebula_chams_self, false, ("chams_nebula_self"));
    setup_item(&globals::chams::nebula_chams_hand, false, ("chams_nebula_hand"));
    setup_item(&globals::chams::nebula_chams_enemy, false, ("chams_nebula_enemy"));
    setup_item(&globals::chams::nebula_self_type, 0, ("chams_nebula_self_type"));
    setup_item(&globals::chams::nebula_hand_type, 0, ("chams_nebula_hand_type"));
    setup_item(&globals::chams::nebula_enemy_type, 0, ("chams_nebula_enemy_type"));

    // CHAMS TAB - Galaxy Chams v2 (BountyHunter material based)
    setup_item(&globals::chams::nebula_galaxy_hand, false, ("chams_galaxy_v2_hand"));
    setup_item(&globals::chams::nebula_galaxy_hand_color, 0, ("chams_galaxy_v2_hand_color"));
    setup_item(&globals::chams::nebula_galaxy_self, false, ("chams_galaxy_v2_self"));
    setup_item(&globals::chams::nebula_galaxy_self_color, 0, ("chams_galaxy_v2_self_color"));
    setup_item(&globals::chams::nebula_galaxy_enemy, false, ("chams_galaxy_v2_enemy"));

    // CHAMS TAB - Galaxy colors
    setup_item(&globals::visuals::LocalGalaxyR, 0.53f, ("chams_local_galaxy_r"));
    setup_item(&globals::visuals::LocalGalaxyG, 0.27f, ("chams_local_galaxy_g"));
    setup_item(&globals::visuals::LocalGalaxyB, 0.47f, ("chams_local_galaxy_b"));
    setup_item(&globals::visuals::EnemyGalaxyR, 1.0f, ("chams_enemy_galaxy_r"));
    setup_item(&globals::visuals::EnemyGalaxyG, 0.15f, ("chams_enemy_galaxy_g"));
    setup_item(&globals::visuals::EnemyGalaxyB, 0.15f, ("chams_enemy_galaxy_b"));

    // CHAMS TAB - Sentinel material enemy (visible = cyan/blue, invisible = orange-green, BountyHunter)
    setup_item(&globals::chams::sentinel_chams_visible_enemy, false, ("chams_sentinel_visible_enemy"));
    setup_item(&globals::chams::sentinel_chams_invisible_enemy, false, ("chams_sentinel_invisible_enemy"));
    setup_item(&globals::chams::sentinel_chams_visible_self, false, ("chams_sentinel_visible_self"));
    setup_item(&globals::chams::sentinel_chams_invisible_self, false, ("chams_sentinel_invisible_self"));
    setup_item(&globals::chams::black_chams_enemy, false, ("chams_black_enemy"));
    setup_item(&globals::chams::black_chams_rgb, false, ("chams_black_rgb"));
    setup_item(&globals::chams::black_chams_type, 0, ("chams_black_type"));
    setup_item(&globals::chams::black_chams_intensity, 50.0f, ("chams_black_intensity"));

    // CHAMS TAB - Cool Chams
    setup_item(&globals::chams::cool_chams_enabled, false, ("chams_cool_enabled"));
    setup_item(&globals::chams::hand_cool_chams_enabled, false, ("chams_hand_cool_enabled"));
    setup_item(&globals::chams::self_cool_chams_enabled, false, ("chams_self_cool_enabled"));
    setup_item(&globals::chams::coolChamsR, 2.093f, ("chams_cool_r"));
    setup_item(&globals::chams::coolChamsG, 0.019f, ("chams_cool_g"));
    setup_item(&globals::chams::coolChamsB, 20.0f, ("chams_cool_b"));
    setup_item(&globals::chams::coolChamsGlow, 10.0f, ("chams_cool_glow"));

    // CHAMS TAB - Fresnel Chams
    setup_item(&globals::chams::custom_fresnel, false, ("chams_custom_fresnel"));
    setup_item(&globals::chams::rainbow_fresnel, false, ("chams_rainbow_fresnel"));
    setup_item(&globals::chams::enemy_fresnel_r, 20.0f, ("chams_enemy_fresnel_r"));
    setup_item(&globals::chams::enemy_fresnel_g, 0.0f, ("chams_enemy_fresnel_g"));
    setup_item(&globals::chams::enemy_fresnel_b, 20.0f, ("chams_enemy_fresnel_b"));
    setup_item(&globals::chams::enemy_fresnel_intensity, 30.0f, ("chams_enemy_fresnel_intensity"));
    setup_item(&globals::chams::enemy_fresnel_rainbow_speed, 0.005f, ("chams_enemy_fresnel_rainbow_speed"));

    // Hand Fresnel
    setup_item(&globals::chams::hand_fresnel_enabled, false, ("chams_hand_fresnel_enabled"));
    setup_item(&globals::chams::hand_fresnel_r, 20.0f, ("chams_hand_fresnel_r"));
    setup_item(&globals::chams::hand_fresnel_g, 0.0f, ("chams_hand_fresnel_g"));
    setup_item(&globals::chams::hand_fresnel_b, 20.0f, ("chams_hand_fresnel_b"));
    setup_item(&globals::chams::hand_fresnel_intensity, 30.0f, ("chams_hand_fresnel_intensity"));

    // Self Fresnel
    setup_item(&globals::chams::self_fresnel_enabled, false, ("chams_self_fresnel_enabled"));
    setup_item(&globals::chams::self_fresnel_r, 0.53f, ("chams_self_fresnel_r"));
    setup_item(&globals::chams::self_fresnel_g, 0.27f, ("chams_self_fresnel_g"));
    setup_item(&globals::chams::self_fresnel_b, 0.47f, ("chams_self_fresnel_b"));
    setup_item(&globals::chams::self_fresnel_intensity, 5.0f, ("chams_self_fresnel_intensity"));

    // FresnelShi (native component)
    setup_item(&globals::chams::fresnel_shi_enabled, false, ("chams_fresnel_shi_enabled"));
    setup_item(&globals::chams::fresnel_shi_intensity, 500.0f, ("chams_fresnel_shi_intensity"));

    // Self 3P Fresnel
    setup_item(&globals::chams::self_3p_fresnel_enabled, false, ("chams_self_3p_fresnel_enabled"));
    setup_item(&globals::chams::self_3p_fresnel_r, 0.53f, ("chams_self_3p_fresnel_r"));
    setup_item(&globals::chams::self_3p_fresnel_g, 0.27f, ("chams_self_3p_fresnel_g"));
    setup_item(&globals::chams::self_3p_fresnel_b, 0.47f, ("chams_self_3p_fresnel_b"));
    setup_item(&globals::chams::self_3p_fresnel_intensity, 5.0f, ("chams_self_3p_fresnel_intensity"));

    // Rainbow Crosshair
    setup_item(&globals::crosshair::rainbow_crosshair, false, ("misc_rainbow_crosshair"));
}

void CConfig::SetupExploits() {
    // EXPLOIT TAB - General subtab
    setup_item(&globals::misc::bhop, false, ("misc_bhop"));
    setup_item(&globals::misc::quick_stop, false, ("misc_quick_stop"));
    setup_item(&globals::misc::quick_stop_near_death_only, true, ("misc_quick_stop_near_death"));
    setup_item(&globals::misc::quick_stop_death_band_max, 55.0f, ("misc_quick_stop_death_band"));
    setup_item(&globals::misc::quick_stop_hp_effective, 28.0f, ("misc_quick_stop_hp"));
    setup_item(&globals::misc::quick_stop_shield_weight, 0.45f, ("misc_quick_stop_shield_w"));
    setup_item(&globals::misc::quick_stop_burst_damage, 35.0f, ("misc_quick_stop_burst"));
    setup_item(&globals::misc::quick_stop_burst_max_hp, 50.0f, ("misc_quick_stop_burst_max_hp"));
    setup_item(&globals::misc::quick_stop_burst_cooldown_ms, 700, ("misc_quick_stop_burst_cd"));
    setup_item(&globals::misc::quick_stop_min_speed, 42.0f, ("misc_quick_stop_min_spd"));
    setup_item(&globals::misc::fastcrouch, false, ("misc_fastcrouch"));
    setup_item(&globals::misc::fovchanger, false, ("misc_fov_changer"));
    setup_item(&globals::misc::fovchangur, 105.0f, ("misc_fov_value"));
    setup_item(&globals::misc::player_trail, false, ("misc_player_trail"));
    setup_item(&globals::misc::player_trail_length, 1.6f, ("misc_player_trail_len"));
    setup_item(&globals::misc::player_trail_size, 1.0f, ("misc_player_trail_size"));
    setup_item(&globals::misc::player_trail_r, 0.20f, ("misc_player_trail_r"));
    setup_item(&globals::misc::player_trail_g, 0.48f, ("misc_player_trail_g"));
    setup_item(&globals::misc::player_trail_b, 1.00f, ("misc_player_trail_b"));
    setup_item(&globals::misc::backtrack, false, ("misc_backtrack"));
    setup_item(&globals::misc::backtrack_length, 1.2f, ("misc_backtrack_len"));
    setup_item(&globals::misc::aspectratio, false, ("misc_aspectratio"));
    setup_item(&globals::misc::aspectfloat, 1.0f, ("misc_aspect_float"));
    setup_item(&globals::misc::PlayerDistance, 100.0f, ("misc_player_distance"));
    setup_item(&globals::misc::tperson, false, ("misc_thirdperson"));
    setup_item(&globals::misc::thirdperson_bind, 0, ("misc_thirdperson_bind"));
    setup_item(&globals::misc::custom_vandal_enabled, false, ("misc_custom_vandal_enabled"));
    setup_item(&globals::misc::custom_text_enabled, false, ("misc_custom_text_enabled"));
    setup_item(&globals::buddy::enabled, false, ("misc_buddy_enabled"));
    //setup_item(&globals::misc::buddy, 0, ("misc_buddy"));
    // LINEUP HELPER
    setup_item(&globals::lineup::enabled, false, ("lineup_enabled"));
    setup_item(&globals::lineup::show_guides, true, ("lineup_guides"));
    setup_item(&globals::lineup::auto_aim, false, ("lineup_auto_aim"));
    setup_item(&globals::lineup::projectile_velocity, 2000.0f, ("lineup_velocity"));
    setup_item(&globals::lineup::gravity_scale, 1.0f, ("lineup_gravity"));
    setup_item(&globals::lineup::render_distance, 5000.0f, ("lineup_render_distance"));
    
    // AUTO PEEK
    setup_item(&globals::autopeek::enabled, false, ("autopeek_enabled"));
    setup_item(&globals::autopeek::peek_key, 0, ("autopeek_key"));
    setup_item(&globals::autopeek::draw_position, true, ("autopeek_draw"));

    // FREE CAM
    setup_item(&globals::misc::freecam, false, ("misc_freecam"));
    setup_item(&globals::misc::freecam_key, 0, ("misc_freecam_key"));
    setup_item(&globals::misc::freecam_speed, 1200.0f, ("misc_freecam_speed"));
    
    // EXPLOITS
   // setup_item(&globals::misc::spam, false, ("misc_spam"));
    setup_item(&globals::misc::BigSelf, false, ("misc_bigself_toggle"));
    setup_item(&globals::misc::BigSelfFloat, 1.0f, ("misc_bigself_float"));
    setup_item(&globals::misc::BigGun, false, ("misc_biggun"));
    setup_item(&globals::misc::BigGunFloat, 1.0f, ("misc_biggun_float"));
    setup_item(&globals::misc::BigGun3D_enabled, false, ("misc_biggun3d_enabled"));
    setup_item(&globals::misc::BigGun3DFloat, 1.0f, ("misc_biggun3d_float"));
    setup_item(&globals::misc::BigGun3D, false, ("misc_biggun3d"));
    setup_item(&globals::misc::BigGun3DWireframe, false, ("misc_biggun3d_wire"));
    setup_item(&globals::misc::customgun, false, ("misc_custom_gun"));
    setup_item(&globals::misc::customhand, false, ("misc_custom_hand"));

    // EXPLOIT TAB - General Extras
    setup_item(&globals::misc::agent_roulette, false, ("misc_agent_roulette"));
    setup_item(&globals::misc::insta_lock, false, ("misc_insta_lock"));
    setup_item(&globals::misc::agent_to_instalock, 0, ("misc_agent_to_instalock"));
    setup_item(&globals::misc::bullet_spawn, false, ("misc_bullet_spawn"));
    setup_item(&globals::misc::bullet_spawn_amount, 1, ("misc_bullet_spawn_amount"));
}

void CConfig::SetupMisc() {
    // MISC TAB - General subtab
    setup_item(&globals::Watermark, true, ("watermark_enabled"));
    setup_item(&globals::misc::antiflash, false, ("misc_antiflash"));
    setup_item(&globals::misc::hide_hud, false, ("misc_hide_hud"));
    setup_item(&globals::misc::custom_char_enabled, false, ("misc_custom_char_enabled"));
    setup_item(&globals::misc::ViewModelChanger, false, ("misc_viewmodel_changer"));
    setup_item(&globals::misc::no_aspect_viewmodel, false, ("misc_no_aspect_viewmodel"));
    setup_item(&globals::misc::wukong, false, ("misc_wukong_mode"));

    // MISC TAB - Gun Buddy subtab
    setup_item(&globals::buddy::enabled, false, ("buddy_enabled"));
    setup_item(&globals::buddy::index, 0, ("buddy_index"));

    // MISC TAB - Kill Say/Sound subtabs
    setup_item(&globals::misc::killsays, false, ("misc_killsays"));
    setup_item(&globals::misc::killsound, false, ("misc_killsound"));
    setup_item(&globals::misc::kill_visible_check, false, ("misc_kill_visible_check"));
    setup_item(&globals::misc::chat_spammer, false, ("misc_chat_spammer"));
    setup_item(&globals::misc::spam_key, 0, ("misc_spam_key"));
    setup_item(&globals::misc::spam_count, 10, ("misc_spam_count"));
    setup_item(&globals::misc::chat_message, "ez", ("misc_chat_message"));
    
    // MISC TAB - Damage Tracker
    setup_item(&globals::misc::damage_tracker, false, ("misc_damage_tracker"));
    setup_item(&globals::misc::damage_tracker_vis_check, false, ("misc_damage_tracker_vis_check"));

    // MISC TAB - Skins
    setup_item(&globals::misc::finisher, false, ("misc_finisher"));
    setup_item(&globals::misc::only_last_kill, false, ("misc_only_last_kill"));

    // MISC TAB - VFX Effects
    setup_item(&globals::misc::hellfire_enabled, false, ("misc_hellfire_enabled"));
    setup_item(&globals::misc::hellfiremode, false, ("misc_hellfiremode"));
    setup_item(&globals::misc::hellfire_amount, 4, ("misc_hellfire_amount"));
    setup_item(&globals::misc::hellfire_interval, 15, ("misc_hellfire_interval"));
    setup_item(&globals::misc::lightningmode, false, ("misc_lightningmode"));
    setup_item(&globals::misc::lightning_count, 5, ("misc_lightning_count"));
    setup_item(&globals::misc::apocalypse_mode, false, ("misc_apocalypse_mode"));
    setup_item(&globals::misc::aura_orbit, false, ("misc_aura_orbit"));
    setup_item(&globals::misc::aura_radius, 76.8f, ("misc_aura_radius"));
    setup_item(&globals::misc::aura_height, 44.3f, ("misc_aura_height"));
    setup_item(&globals::misc::aura_speed, 0.34f, ("misc_aura_speed"));
    setup_item(&globals::misc::aura_copies, 2, ("misc_aura_copies"));
    setup_item(&globals::misc::aura_scale_x, -0.84f, ("misc_aura_scale_x"));
    setup_item(&globals::misc::aura_scale_y, -1.33f, ("misc_aura_scale_y"));
    setup_item(&globals::misc::aura_scale_z, -1.40f, ("misc_aura_scale_z"));
    setup_item(&globals::misc::aura_yaw_offset, -43.2f, ("misc_aura_yaw_offset"));
    setup_item(&globals::misc::aura_roll, 270.0f, ("misc_aura_roll"));
    setup_item(&globals::misc::aura_emissive, 50.0f, ("misc_aura_emissive"));
    setup_item(&globals::misc::nightmode, false, ("misc_nightmode"));
    setup_item(&globals::misc::nightmode_r, 0.0f, ("misc_nightmode_r"));
    setup_item(&globals::misc::nightmode_g, 0.0f, ("misc_nightmode_g"));
    setup_item(&globals::misc::nightmode_b, 0.0f, ("misc_nightmode_b"));
    setup_item(&globals::misc::nightmode_intensity, 0.40f, ("misc_nightmode_intensity"));
    setup_item(&globals::misc::nightmode_postprocess, true, ("misc_nightmode_postprocess"));
    setup_item(&globals::misc::nightmode_pp_tint_r, 0.4f, ("misc_nightmode_pp_tint_r"));
    setup_item(&globals::misc::nightmode_pp_tint_g, 0.4f, ("misc_nightmode_pp_tint_g"));
    setup_item(&globals::misc::nightmode_pp_tint_b, 0.6f, ("misc_nightmode_pp_tint_b"));
    setup_item(&globals::misc::nightmode_pp_exposure_min, 0.03f, ("misc_nightmode_pp_exposure_min"));
    setup_item(&globals::misc::nightmode_pp_exposure_max, 0.2f, ("misc_nightmode_pp_exposure_max"));
    setup_item(&globals::misc::nightmode_pp_saturation, 0.6f, ("misc_nightmode_pp_saturation"));
    setup_item(&globals::misc::nightmode_pp_vignette, 0.1f, ("misc_nightmode_pp_vignette"));


    // MENU COLORS
    setup_item(&globals::menu_colors::accent_r, 140.0f, ("menu_accent_r"));
    setup_item(&globals::menu_colors::accent_g, 100.0f, ("menu_accent_g"));
    setup_item(&globals::menu_colors::accent_b, 230.0f, ("menu_accent_b"));
    setup_item(&globals::menu_colors::bg_r, 10.0f, ("menu_bg_r"));
    setup_item(&globals::menu_colors::bg_g, 10.0f, ("menu_bg_g"));
    setup_item(&globals::menu_colors::bg_b, 16.0f, ("menu_bg_b"));
    setup_item(&globals::menu_colors::border_r, 32.0f, ("menu_border_r"));
    setup_item(&globals::menu_colors::border_g, 35.0f, ("menu_border_g"));
    setup_item(&globals::menu_colors::border_b, 48.0f, ("menu_border_b"));
    setup_item(&globals::menu_colors::slider_r, 140.0f, ("menu_slider_r"));
    setup_item(&globals::menu_colors::slider_g, 100.0f, ("menu_slider_g"));
    setup_item(&globals::menu_colors::slider_b, 230.0f, ("menu_slider_b"));
    setup_item(&globals::menu_colors::checkbox_r, 140.0f, ("menu_checkbox_r"));
    setup_item(&globals::menu_colors::checkbox_g, 100.0f, ("menu_checkbox_g"));
    setup_item(&globals::menu_colors::checkbox_b, 230.0f, ("menu_checkbox_b"));
}

void CConfig::Initialize() {
    SetupAimbot();
    SetupVisuals();
    SetupChams();
    SetupExploits();
    SetupMisc();
}

void CConfig::SaveSettings(const std::string& szIniFile) {
    std::ofstream file(szIniFile);

    if (!file.is_open()) {
        MessageBoxA(NULL, ("Cannot open file: " + szIniFile).c_str(), "Error", MB_OK);
        return;
    }

    file << "{\n";

    bool first = true;
    for (auto item : items) {
        if (!first) file << ",\n";
        first = false;

        file << "  \"" << item->name << "\": ";

        if (item->type == "bool") {
            file << (*(bool*)item->pointer ? "true" : "false");
        }
        else if (item->type == "int") {
            file << *(int*)item->pointer;
        }
        else if (item->type == "float") {
            file << std::fixed << std::setprecision(6) << *(float*)item->pointer;
        }
        else if (item->type == "double") {
            file << std::fixed << std::setprecision(6) << *(double*)item->pointer;
        }
        else if (item->type == "string") {
            file << "\"" << *(std::string*)item->pointer << "\"";
        }
    }

    file << "\n}";
    file.close();
}

void CConfig::LoadSettings(const std::string& szIniFile) {
    std::ifstream file(szIniFile);

    if (!file.is_open()) {
        MessageBoxA(NULL, ("Cannot open file: " + szIniFile).c_str(), "Error", MB_OK);
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    for (auto item : items) {
        std::string search = "\"" + item->name + "\":";
        size_t pos = content.find(search);

        if (pos != std::string::npos) {
            pos += search.length();

            // Skip whitespace
            while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\t')) pos++;

            if (item->type == "bool") {
                if (content.substr(pos, 4) == "true") {
                    *(bool*)item->pointer = true;
                }
                else if (content.substr(pos, 5) == "false") {
                    *(bool*)item->pointer = false;
                }
            }
            else if (item->type == "int") {
                size_t end = find_json_scalar_end(content, pos);
                std::string value = trim_ws(content.substr(pos, end - pos));
                if (!value.empty()) {
                    try {
                        long long v = std::stoll(value);
                        if (v <= INT_MAX && v >= INT_MIN)
                            *(int*)item->pointer = static_cast<int>(v);
                    } catch (...) {
                    }
                }
            }
            else if (item->type == "float") {
                size_t end = find_json_scalar_end(content, pos);
                std::string value = trim_ws(content.substr(pos, end - pos));
                if (!value.empty()) {
                    try {
                        float v = std::stof(value);
                        if (std::isfinite(static_cast<double>(v)))
                            *(float*)item->pointer = v;
                    } catch (...) {
                    }
                }
            }
            else if (item->type == "double") {
                size_t end = find_json_scalar_end(content, pos);
                std::string value = trim_ws(content.substr(pos, end - pos));
                if (!value.empty()) {
                    try {
                        double v = std::stod(value);
                        if (std::isfinite(v))
                            *(double*)item->pointer = v;
                    } catch (...) {
                    }
                }
            }
            else if (item->type == "string") {
                if (pos < content.length() && content[pos] == '"') {
                    pos++;
                    std::string value;
                    while (pos < content.length() && content[pos] != '"') {
                        value += content[pos++];
                    }
                    *(std::string*)item->pointer = value;
                }
            }
        }
    }

    ClampConfigGlobalsAfterLoad();
}

