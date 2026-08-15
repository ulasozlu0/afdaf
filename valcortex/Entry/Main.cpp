#include "../Hooks/Engine.hpp"
#include "../Core/Logger.hpp"
#include <windows.h>
#include "../Security/Spoof.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#pragma warning(disable: 4996)

static DWORD WINAPI InitHooksThread(LPVOID) {
    Sleep(1000);
    //LOG_INFO("InitHooksThread: calling init_hooks after 1s delay");
    hooks::init_hooks();
    //LOG_INFO("InitHooksThread: init_hooks returned");
    return 0;
}

extern "C" __declspec(dllexport) int val33BA7ABAD2AFCAE4EB50BBE29AD3C98EC9CC67458BD8305989cortex(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(NULL, code, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {

#ifndef DEV
        HMODULE handle = NULL;
        BOOL ret = SPOOF_CALL(GetModuleHandleExW)(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPCWSTR)DllMain, &handle);
        if (!ret) return FALSE;
#endif

        SPOOF_CALL(DisableThreadLibraryCalls)(hModule);

        cortex_log::init();
        //LOG_INFO("DllMain called - DLL_PROCESS_ATTACH");
        //LOG_INFO("hModule = 0x%llX", (unsigned long long)hModule);

        //LOG_INFO("Starting delayed init_hooks thread (1s)...");
        CreateThread(nullptr, 0, InitHooksThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
