#pragma once
#include <windows.h>
#include <string>

namespace PakLoader {
    void LoadAllPaks();
    bool MountPakFile(const wchar_t* path, int priority = 10000);
    void LoadEmotePak();
}
