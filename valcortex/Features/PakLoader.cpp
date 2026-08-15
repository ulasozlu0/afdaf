#include "PakLoader.hpp"
#include "../Core/Globals.hpp"
#include <filesystem>
#include <cstdint>
#include <cwctype>
#include <windows.h>

namespace PakLoader {

    static int64_t get_fpak_platform_file()
    {
        auto base = (uintptr_t)GetModuleHandleA(nullptr);
        return *(int64_t*)(base + offsets::FPakPlatformFile);
    }

    static void bypass_pak_signing()
    {
        auto base = (uintptr_t)GetModuleHandleA(nullptr);
        auto delegate = (uint8_t*)(base + offsets::SigningDelegate);
        DWORD old_protect;
        VirtualProtect(delegate, 0x20, PAGE_READWRITE, &old_protect);
        memset(delegate, 0, 0x20);
        VirtualProtect(delegate, 0x20, old_protect, &old_protect);

        auto fpak = get_fpak_platform_file();
        if (fpak) {
            auto bsigned_ptr = (uint8_t*)(fpak + 48);
            VirtualProtect(bsigned_ptr, 1, PAGE_READWRITE, &old_protect);
            *bsigned_ptr = 0;
            VirtualProtect(bsigned_ptr, 1, old_protect, &old_protect);
        }
    }

    static void mount_custom_pak(int64_t fpak, const wchar_t* path, int priority)
    {
        if (!fpak || !path || !path[0]) return;
        if (!std::filesystem::exists(path)) return;

        typedef bool(__fastcall* fn_mount)(int64_t, const wchar_t*, int, const wchar_t*, bool, bool);
        auto base = (uintptr_t)GetModuleHandleA(nullptr);
        auto mount = (fn_mount)(base + offsets::Mount);
        mount(fpak, path, priority, nullptr, true, false);
    }

    bool MountPakFile(const wchar_t* path, int priority)
    {
        bypass_pak_signing();
        auto fpak = get_fpak_platform_file();
        if (!fpak || !path || !path[0]) return false;
        if (!std::filesystem::exists(path)) return false;
        mount_custom_pak(fpak, path, priority);
        return true;
    }

    void LoadEmotePak()
    {
        bypass_pak_signing();
        auto fpak = get_fpak_platform_file();
        if (!fpak) return;

        const wchar_t* direct_paths[] = {
            L"C:\\mods\\ObjectPatch.pak",
            L"C:\\mods\\objectpatch.pak",
            L"C:\\mods\\ObjectPatch_P.pak",
            L"C:\\mods\\EmotePatch.pak",
        };

        bool mounted = false;
        for (const wchar_t* path : direct_paths) {
            if (std::filesystem::exists(path)) {
                mount_custom_pak(fpak, path, 10000);
                mounted = true;
            }
        }

        std::wstring modsPath = L"C:\\mods";
        if (std::filesystem::exists(modsPath) && std::filesystem::is_directory(modsPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(modsPath)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != L".pak") continue;

                std::wstring filename = entry.path().filename().wstring();
                for (auto& c : filename) c = (wchar_t)towlower(c);

                if (filename.find(L"objectpatch") != std::wstring::npos ||
                    filename.find(L"emote") != std::wstring::npos)
                {
                    mount_custom_pak(fpak, entry.path().c_str(), 10000);
                    mounted = true;
                }
            }
        }

        if (!mounted)
            LoadAllPaks();
    }

    void LoadAllPaks()
    {
        bypass_pak_signing();

        auto fpak = get_fpak_platform_file();
        if (!fpak) return;

        std::wstring modsPath = L"C:\\mods";
        if (!std::filesystem::exists(modsPath) || !std::filesystem::is_directory(modsPath)) {
            return;
        }

        int priority = 10000;
        for (const auto& entry : std::filesystem::directory_iterator(modsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == L".pak") {
                mount_custom_pak(fpak, entry.path().c_str(), priority++);
            }
        }
    }
}
