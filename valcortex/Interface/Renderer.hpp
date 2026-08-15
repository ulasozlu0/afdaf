#pragma once
#include "Drawing.hpp"
#include "IntroAnimation.hpp"
#include "Settings.hpp"
#include <ShlObj.h>

extern uworld* UWorldSave;

namespace burat {

    constexpr float kMenuW = 455.0f;
    constexpr float kMenuH = 470.0f;
    fvector2d pos = { 960.0f - (kMenuW / 2.0f), 540.0f - (kMenuH / 2.0f) };
    bool menu_open = true;
    static flinearcolor fovcolor = { 255.0f, 255.0f, 255.0f, 1.0f };

    void Menu(ucanvas* canvas)
    {
        menu::SetupCanvas(canvas);
        menu::input::handle();
        menu::input::ResetFrame();

        if (GetAsyncKeyState(VK_INSERT) & 1) menu_open = !menu_open;

        // Removed features — keep off even if old config had them
        globals::misc::backtrack = false;
        globals::misc::bullet_tracers = false;
        globals::visuals::rotating_text_enabled = false;

        static int tab = 0;

        if (menu::Window(L"", &pos, fvector2d(kMenuW, kMenuH), menu_open))
        {
            const float header_h = 30.0f;          // top black strip
            const float tab_x = 10.0f;
            const float tab_y = header_h + 16.0f;  // categories ~1.5cm below header
            const float tab_gap = 4.0f;
            const float tab_h = 28.0f;
            const float tab_w = (kMenuW - 20.0f - tab_gap * 3.0f) / 4.0f;
            const float tab_area_bottom = tab_y + tab_h + tab_gap + tab_h + 10.0f;
            const float content_y = tab_area_bottom + 6.0f;
            const float panel_h = kMenuH - content_y - 10.0f;
            const float panel_w = kMenuW - 20.0f;

            // Full solid black header bar (also the drag strip)
            menu::drawFilledRect(
                fvector2d(pos.x, pos.y),
                kMenuW, header_h,
                flinearcolor(0.f, 0.f, 0.f, 1.f));

            // Title centered in the header bar
            menu::PostRenderer::Text(L"CORTEX SOFTWARE",
                fvector2d(pos.x + kMenuW / 2.0f, pos.y + header_h / 2.0f),
                fvector2d(1.0f, 1.0f),
                flinearcolor(1.f, 1.f, 1.f, 1.f),
                0.0f,
                flinearcolor(0.f, 0.f, 0.f, 1.f),
                fvector2d(0.f, 0.f),
                true, true,
                flinearcolor(0.f, 0.f, 0.f, 1.f));

            // Dim band behind category rows
            menu::drawFilledRectTransparent(
                fvector2d(pos.x, pos.y + header_h),
                kMenuW, tab_area_bottom - header_h,
                flinearcolor(0.f, 0.f, 0.f, 0.35f));

            menu::Draw_Line(
                fvector2d(pos.x + 1.f, pos.y + tab_area_bottom),
                fvector2d(pos.x + kMenuW - 1.f, pos.y + tab_area_bottom),
                1.f, flinearcolor(1.f, 1.f, 1.f, 0.18f));

            // Row 1 — Aimbot | Esp | World | Misc
            menu::offset_x = tab_x;
            menu::offset_y = tab_y;
            menu::sameLine = false;
            if (menu::ButtonTab2(L"Aimbot", fvector2d(tab_w, tab_h), tab == 0)) tab = 0;
            menu::SameLine();
            if (menu::ButtonTab2(L"Esp", fvector2d(tab_w, tab_h), tab == 1)) tab = 1;
            menu::SameLine();
            if (menu::ButtonTab2(L"World", fvector2d(tab_w, tab_h), tab == 2)) tab = 2;
            menu::SameLine();
            if (menu::ButtonTab2(L"Misc", fvector2d(tab_w, tab_h), tab == 3)) tab = 3;

            // Row 2 — Skin | Thirdperson | Effects | Chams
            menu::offset_x = tab_x;
            menu::offset_y = tab_y + tab_h + tab_gap;
            menu::sameLine = false;
            if (menu::ButtonTab2(L"Skin", fvector2d(tab_w, tab_h), tab == 4)) tab = 4;
            menu::SameLine();
            if (menu::ButtonTab2(L"Thirdperson", fvector2d(tab_w, tab_h), tab == 5)) tab = 5;
            menu::SameLine();
            if (menu::ButtonTab2(L"Effects", fvector2d(tab_w, tab_h), tab == 6)) tab = 6;
            menu::SameLine();
            if (menu::ButtonTab2(L"Chams", fvector2d(tab_w, tab_h), tab == 7)) tab = 7;

            if (tab == 0)
            {
                float half_w = (panel_w - 8.0f) / 2.0f;

                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"SETTINGS", fvector2d(half_w, panel_h));

                menu::Checkbox(L"Enable", &globals::aimbot::a1mbot);
                menu::SliderFloat(L"FOV", &globals::aimbot::a1m_f0v, 5.0f, 2000.0f, "%.1f");
                menu::SliderFloat(L"Smoothing", &globals::aimbot::a1m_sm00th, 1.0f, 20.0f, "%.0f");
                menu::KeybindRow(L"Aim Key", fvector2d(half_w - 20, 22), &globals::aimbot::a1m_k3y);
                menu::Checkbox(L"Draw FOV", &globals::aimbot::draw_f0v);
                menu::Checkbox(L"360° FOV", &globals::aimbot::enable_360_fov);
                // draw_f0v
                menu::offset_y += 2;
                menu::FieldLabel(L"Target Bones");
                menu::Combobox(fvector2d(half_w - 22, 24), &globals::aimbot::a1m_b0ne,
                    L"Head", L"Neck", L"Chest", NULL);

                menu::offset_x = 10 + half_w + 8;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"FEATURES", fvector2d(half_w, panel_h));

                menu::Checkbox(L"Auto Shoot", &globals::aimbot::AutoShoot);
                menu::Checkbox(L"Silent Aim", &globals::aimbot::silent);
                menu::Checkbox(L"No Spread", &globals::aimbot::nospread);
                menu::Checkbox(L"Visible Check", &globals::aimbot::v1sh_ch3ck);
                menu::Checkbox(L"Recoil Control", &globals::aimbot::reco1l_contr0l);

                if (globals::aimbot::nospread)
                    globals::aimbot::reco1l_contr0l = false;
            }
            else if (tab == 1)
            {
                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"PLAYER ESP", fvector2d(panel_w, panel_h));

                menu::Checkbox(L"2D Box", &globals::visuals::box2d);
                menu::Checkbox(L"3D Box", &globals::visuals::box3d);
                menu::Checkbox(L"Corner Box", &globals::visuals::corner);
                menu::Checkbox(L"Outline ESP", &globals::visuals::outline_esp);
                if (globals::visuals::outline_esp) {
                    menu::FieldLabel(L"Outline Style");
                    menu::Combobox(fvector2d(panel_w - 22, 24), &globals::visuals::outline_style,
                        L"Capsule (fast)", L"Silhouette Hull", NULL);
                    if (globals::visuals::outline_style == 1)
                        menu::SliderFloat(L"Hull Pad", &globals::visuals::outline_hull_padding, 2.f, 30.f, "%.1f");
                }
                menu::Checkbox(L"Vis Check", &globals::visuals::vischeck);
                menu::Checkbox(L"Head ESP", &globals::visuals::headb0x);
                menu::Checkbox(L"Health Bar", &globals::visuals::h3althbar);
                menu::Checkbox(L"Skeleton", &globals::visuals::sk3let0n);
                menu::Checkbox(L"Filled Box", &globals::visuals::filled_box);
                menu::Checkbox(L"Box + HP", &globals::visuals::box_health);
                menu::Checkbox(L"Snapline", &globals::visuals::snapl1ne);

                if (globals::visuals::corner) { globals::visuals::box2d = false; globals::visuals::box3d = false; }
                if (globals::visuals::box3d) { globals::visuals::corner = false; globals::visuals::box2d = false; }
                if (globals::visuals::box2d) { globals::visuals::corner = false; globals::visuals::box3d = false; }
            }
            else if (tab == 2)
            {
                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"WORLD", fvector2d(panel_w, panel_h));

                menu::Checkbox(L"Skybox", &globals::misc::skybox);
                if (globals::misc::skybox) {
                    menu::FieldLabel(L"Sky Preset");
                    menu::Combobox(fvector2d(panel_w - 22, 24), &globals::misc::skybox_preset,
                        L"Neon Aurora", L"Solar Flare", L"Purple Haze", L"Ocean Blue", L"RGB",
                        L"Emerald Glow", L"Crimson Sky", L"Rose Pink", L"Midnight Void", NULL);
                }

                menu::Checkbox(L"Fog", &globals::misc::Fog);
                if (globals::misc::Fog) {
                    menu::SliderFloat(L"Fog Density", &globals::misc::FogDensity, 0.0f, 10.0f, "%.3f");
                    menu::Checkbox(L"Fog RGB Cycle", &globals::misc::FogRGB);
                    if (!globals::misc::FogRGB) {
                        menu::SliderFloat(L"Fog R", &globals::misc::FogColor.r, 0.0f, 1.0f, "%.2f");
                        menu::SliderFloat(L"Fog G", &globals::misc::FogColor.g, 0.0f, 1.0f, "%.2f");
                        menu::SliderFloat(L"Fog B", &globals::misc::FogColor.b, 0.0f, 1.0f, "%.2f");
                    }
                }

                menu::Checkbox(L"Hellfire", &globals::misc::hellfire_enabled);
                if (globals::misc::hellfire_enabled) {
                    menu::Checkbox(L"Hellfire Mode", &globals::misc::hellfiremode);
                    menu::SliderInt(L"Amount", &globals::misc::hellfire_amount, 1, 40, "%d");
                    menu::SliderInt(L"Interval", &globals::misc::hellfire_interval, 1, 60, "%d");
                }

                menu::Checkbox(L"Night Mode", &globals::misc::nightmode);
                if (globals::misc::nightmode) {
                    menu::SliderFloat(L"Intensity", &globals::misc::nightmode_intensity, 0.0f, 1.0f, "%.2f");
                }
            }
            else if (tab == 3)
            {
                float half_w = (panel_w - 8.0f) / 2.0f;

                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"MISC", fvector2d(half_w, panel_h));

                menu::Checkbox(L"Bunny Hop", &globals::misc::bhop);
                menu::Checkbox(L"Skip Tutorial", &globals::misc::sk1ptut0rial);

                menu::offset_x = 10 + half_w + 8;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"SETUP", fvector2d(half_w, panel_h));

                menu::Checkbox(L"Viewmodel", &globals::misc::ViewModelChanger);
                menu::Checkbox(L"Aspect Ratio", &globals::misc::aspectratio);
                if (globals::misc::aspectratio)
                    //menu::Checkbox(L"Not Aspect VM", &globals::misc::no_aspect_viewmodel);
                menu::SliderFloat(L"AR Value", &globals::misc::aspectfloat, 0.5f, 3.0f, "%.2f");
                menu::Checkbox(L"Custom FOV", &globals::misc::fovchanger);
                if (globals::misc::fovchanger)
                    menu::SliderFloat(L"FOV Value", &globals::misc::fovchangur, 60.0f, 140.0f, "%.0f");
            }
            else if (tab == 4)
            {
                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"UNLOCK ALL", fvector2d(panel_w, panel_h));

                if (menu::WideButton(L"Unlock All Skins", panel_w - 22.f, 34.f))
                    globals::misc::sk1n_chang3r = true;
            }
            else if (tab == 5)
            {
                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"THIRD PERSON", fvector2d(panel_w, panel_h));

                menu::Checkbox(L"Third Person", &globals::misc::tperson);
                menu::offset_y += 6;
                menu::HotkeyBoxed("", fvector2d(54, 20), &globals::misc::thirdperson_bind);
                menu::offset_y += 6;
                menu::SliderFloat(L"Distance", &globals::misc::PlayerDistance, 50.0f, 500.0f, "%.0f");
            }
            else if (tab == 6)
            {
                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"EFFECTS", fvector2d(panel_w, panel_h));

                //if (menu::WideButton(L"Load Emote", panel_w - 22.f, 32.f))
                //    globals::misc::emote_toggle_request = true;
                //menu::KeybindRow(L"Emote Key", fvector2d(panel_w - 22, 22), &globals::misc::emote_load_bind);
                menu::offset_y += 4;

                menu::Checkbox(L"Player Trail", &globals::misc::player_trail);
                if (globals::misc::player_trail)
                    menu::SliderFloat(L"Trail Length", &globals::misc::player_trail_length, 0.4f, 4.0f, "%.1f");

                menu::Checkbox(L"Aura Orbit", &globals::misc::aura_orbit);
                if (globals::misc::aura_orbit) {
                    menu::SliderFloat(L"Orbit Speed", &globals::misc::aura_speed, 0.01f, 2.0f, "%.2f");
                    menu::SliderFloat(L"Orbit Radius", &globals::misc::aura_radius, 10.0f, 200.0f, "%.1f");
                    menu::SliderFloat(L"Orbit Height", &globals::misc::aura_height, 0.0f, 100.0f, "%.1f");
                    menu::SliderInt(L"Copies", &globals::misc::aura_copies, 1, 8, "%d");
                }
            }
            else if (tab == 7)
            {
                float col_gap = 8.0f;
                float col_w = (panel_w - col_gap) / 2.0f;

                menu::offset_x = 10;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"SENTINEL", fvector2d(col_w, panel_h));

                menu::Checkbox(L"Sentinel Vis (Enemy)", &globals::chams::sentinel_chams_visible_enemy);
                menu::Checkbox(L"Sentinel Invis (Enemy)", &globals::chams::sentinel_chams_invisible_enemy);
                menu::Checkbox(L"Sentinel Vis (Self)", &globals::chams::sentinel_chams_visible_self);
                menu::Checkbox(L"Sentinel Invis (Self)", &globals::chams::sentinel_chams_invisible_self);

                menu::SectionWrapper(L"CRYSTAL CHAMS", fvector2d(col_w, panel_h));

                menu::Checkbox(L"Crystal Self", &globals::visuals::crystal_chams_self);
                menu::Combobox(fvector2d(col_w - 22, 24), &globals::visuals::crystal_chams_self_preset,
                    L"Teal", L"Silver", L"Pearl", NULL);

                menu::Checkbox(L"Crystal Hand", &globals::visuals::crystal_chams_hand);
                menu::Combobox(fvector2d(col_w - 22, 24), &globals::visuals::crystal_chams_hand_preset,
                    L"Teal", L"Silver", L"Pearl", NULL);

                menu::Checkbox(L"Crystal Enemy", &globals::visuals::crystal_chams_enemy);
                menu::Combobox(fvector2d(col_w - 22, 24), &globals::visuals::crystal_chams_enemy_preset,
                    L"Teal", L"Silver", L"Pearl", NULL);

                menu::SectionWrapper(L"OUTLINE CHAMS", fvector2d(col_w, panel_h));

                menu::Checkbox(L"Outline Chams", &globals::chams::outline_enabled);
                if (globals::chams::outline_enabled) {
                    menu::FieldLabel(L"Visible Preset");
                    menu::Combobox(fvector2d(col_w - 22, 24), &globals::chams::visible_outline_preset,
                        L"Galaxy", L"Blue", L"Green", L"Orange", L"Pink", L"White", L"Black Blue", L"Purple", NULL);
                    menu::FieldLabel(L"Invisible Preset");
                    menu::Combobox(fvector2d(col_w - 22, 24), &globals::chams::invisible_outline_preset,
                        L"Red", L"Orange", L"Yellow", L"Green", L"Pink", L"Gray", L"Black Blue", L"Purple", NULL);
                    menu::Checkbox(L"Hand Outline", &globals::chams::hand_outline_enabled);
                    menu::Checkbox(L"Gun 1P Outline", &globals::chams::gun_outline1P_enabled);
                    menu::Checkbox(L"Gun 3P Outline", &globals::chams::gun_outline3P_enabled);
                }

                menu::offset_x = 10 + col_w + col_gap;
                menu::offset_y = content_y;
                menu::SectionWrapper(L"HAND / WIRE / BLACK", fvector2d(col_w, panel_h));

                menu::Checkbox(L"Hand Chams", &globals::chams::nebula_galaxy_hand);
                if (globals::chams::nebula_galaxy_hand) {
                    globals::misc::HandWire = false;
                    globals::misc::self_wireframe = false;
                    menu::FieldLabel(L"Hand Color");
                    menu::Combobox(fvector2d(col_w - 22, 24), &globals::chams::nebula_galaxy_hand_color,
                        L"White", L"Black", L"RGB", NULL);
                }

                menu::Checkbox(L"Enemy Chams", &globals::chams::black_chams_enemy);
                menu::FieldLabel(L"Style");
                menu::Combobox(fvector2d(col_w - 22, 24), &globals::chams::black_chams_type,
                    L"Black", L"RGB", L"White", NULL);
                globals::chams::black_chams_rgb = (globals::chams::black_chams_type == 1);
                if (globals::chams::black_chams_type != 0)
                    globals::chams::black_chams_enemy = true;
                if (globals::chams::black_chams_enemy) {
                    globals::misc::Wireframe = false;
                    globals::misc::self_wireframe = false;
                    menu::SliderFloat(L"Intensity", &globals::chams::black_chams_intensity, 1.0f, 120.0f, "%.0f");
                }
                menu::Checkbox(L"Hand Wire", &globals::misc::HandWire);
                menu::Checkbox(L"Enemy Wire", &globals::misc::Wireframe);
                menu::Checkbox(L"Self Wire", &globals::misc::self_wireframe);
                menu::Checkbox(L"Cool Wireframe", &globals::misc::MoscaWireframe);
                if (globals::misc::MoscaWireframe) {
                    menu::SliderFloat(L"Wire Glow", &globals::misc::MoscaGlow, 0.1f, 10.f, "%.1f");
                    menu::SliderFloat(L"Wire R", &globals::misc::MoscaWireColor[0], 0.f, 1.f, "%.2f");
                    menu::SliderFloat(L"Wire G", &globals::misc::MoscaWireColor[1], 0.f, 1.f, "%.2f");
                    menu::SliderFloat(L"Wire B", &globals::misc::MoscaWireColor[2], 0.f, 1.f, "%.2f");
                }
            }

            fvector2d cursorPos = menu::CursorPos();
            menu::drawFilledRect(fvector2d(cursorPos.x - 2, cursorPos.y - 2), 4, 4, flinearcolor(1, 1, 1, 1));

            menu::IntroAnimator::RenderCover(pos, fvector2d(kMenuW, kMenuH));
            menu::IntroAnimator::RenderIntro(canvas, pos, fvector2d(kMenuW, kMenuH));
        }

        menu::Render();
    }
}
