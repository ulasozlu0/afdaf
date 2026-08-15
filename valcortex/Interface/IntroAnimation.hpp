#pragma once
#include "Drawing.hpp"
#include <Windows.h>
#include <cmath>

extern class uworld* UWorldSave;

namespace menu {
    class IntroAnimator {
    private:
        static inline ULONGLONG start_time = 0;
        static inline bool is_started = false;
        static inline utexture2d* intro_logo = nullptr;
        static inline ULONGLONG last_load_attempt = 0;
        static inline ULONGLONG first_time_opened = 0;

        static float EaseInOut(float t) {
            return t * t * (3.0f - 2.0f * t);
        }
        static float EaseOut(float t) {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }
        static float EaseIn(float t) {
            return t * t;
        }

    public:
        static void RenderIntro(ucanvas* canvas, fvector2d pos, fvector2d size) {
            ULONGLONG current_time = GetTickCount64();

            if (first_time_opened == 0) first_time_opened = current_time;

            if (!intro_logo) {
                if (current_time - last_load_attempt > 500) {
                    last_load_attempt = current_time;
                    if (UWorldSave) {
                        intro_logo = system::import_file_as_texture2d((uobject*)UWorldSave, fstring(L"C:\\ProgramData\\Microsoft\\Windows\\Caches\\{7D5B-F12A-11E1-B5}\\logow.png"));
                    }
                }
                
                if (!intro_logo) {
                    if (current_time - first_time_opened > 1000 && !is_started) {
                        is_started = true;
                        start_time = current_time;
                    }
                    if (!is_started) return;
                } 
                else if (!is_started) { 
                    start_time = current_time;
                    is_started = true;
                }
            }

            if (!is_started) return;

            float elapsed = (current_time - start_time) / 1000.0f; 
            
            if (elapsed > 4.5f) return;

            fvector2d center = { pos.x + size.x * 0.5f, pos.y + size.y * 0.5f };
            float alpha = 1.0f;
            float scale = 1.0f;
            
            if (elapsed < 1.5f) {
                float t = elapsed / 1.5f;
                alpha = EaseInOut(t);
                scale = 0.8f + (0.2f * EaseOut(t));
            } 
            else if (elapsed < 3.0f) {
                alpha = 1.0f;
                scale = 1.0f + 0.04f * sinf((elapsed - 1.5f) * 3.14159f);
            } 
            else {
                float t = (elapsed - 3.0f) / 1.5f;
                alpha = 1.0f - EaseInOut(t);
                scale = 1.0f - (0.4f * EaseIn(t));
            }

            if (alpha < 0.01f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;

            float base_w = 560.0f * scale; 
            float base_h = 560.0f * scale;
            
            if (intro_logo) {
                canvas->k2_drawtexture(
                    (uobject*)intro_logo, 
                    fvector2d(center.x - base_w * 0.5f, center.y - base_h * 0.5f), 
                    fvector2d(base_w, base_h),                  
                    fvector2d(0.0f, 0.0f),                     
                    fvector2d(1.0f, 1.0f),                     
                    flinearcolor(1.0f, 1.0f, 1.0f, alpha),     
                    e_blend_mode::translucent,           
                    0.0f,                                      
                    fvector2d(0.0f, 0.0f)                      
                );
            }
        }

        static void RenderCover(fvector2d pos, fvector2d size) {
            (void)pos;
            (void)size;
        }

        static bool ShouldDrawItems() {
            return true;
        }
    };
}
