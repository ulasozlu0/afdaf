#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

namespace cortex_log {

    inline FILE* g_file = nullptr;
    inline CRITICAL_SECTION g_cs;
    inline bool g_initialized = false;
    inline ULONGLONG g_start_tick = 0;
    inline LPTOP_LEVEL_EXCEPTION_FILTER g_old_filter = nullptr;
    inline char g_log_path[MAX_PATH] = {};

    inline void write(const char* level, const char* func, const char* fmt, ...) {
        if (!g_file) return;
        EnterCriticalSection(&g_cs);

        ULONGLONG elapsed = GetTickCount64() - g_start_tick;
        unsigned int sec = (unsigned int)(elapsed / 1000);
        unsigned int ms  = (unsigned int)(elapsed % 1000);

        fprintf(g_file, "[%u.%03u][T:%lu][%s] %s: ", sec, ms, GetCurrentThreadId(), level, func);

        va_list args;
        va_start(args, fmt);
        vfprintf(g_file, fmt, args);
        va_end(args);

        fprintf(g_file, "\n");
        fflush(g_file);
        LeaveCriticalSection(&g_cs);
    }

    inline void write_stacktrace(CONTEXT* ctx) {
        if (!g_file || !ctx) return;
        EnterCriticalSection(&g_cs);

        fprintf(g_file, "\n=== CRASH STACK TRACE ===\n");
        fprintf(g_file, "RIP=0x%llX RSP=0x%llX RBP=0x%llX\n",
            (unsigned long long)ctx->Rip, (unsigned long long)ctx->Rsp, (unsigned long long)ctx->Rbp);
        fprintf(g_file, "RAX=0x%llX RBX=0x%llX RCX=0x%llX RDX=0x%llX\n",
            (unsigned long long)ctx->Rax, (unsigned long long)ctx->Rbx,
            (unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx);
        fprintf(g_file, "RSI=0x%llX RDI=0x%llX R8=0x%llX R9=0x%llX\n",
            (unsigned long long)ctx->Rsi, (unsigned long long)ctx->Rdi,
            (unsigned long long)ctx->R8, (unsigned long long)ctx->R9);

        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        SymInitialize(process, NULL, TRUE);

        STACKFRAME64 frame = {};
        frame.AddrPC.Offset = ctx->Rip;
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrFrame.Offset = ctx->Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = ctx->Rsp;
        frame.AddrStack.Mode = AddrModeFlat;

        fprintf(g_file, "\nCall Stack:\n");
        for (int i = 0; i < 32; i++) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, (PVOID)ctx,
                NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                break;

            DWORD64 addr = frame.AddrPC.Offset;
            if (addr == 0) break;

            char sym_buf[sizeof(SYMBOL_INFO) + 256];
            SYMBOL_INFO* sym = (SYMBOL_INFO*)sym_buf;
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = 255;

            DWORD64 displacement = 0;
            HMODULE hMod = NULL;
            char mod_name[MAX_PATH] = "???";
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)addr, &hMod)) {
                GetModuleFileNameA(hMod, mod_name, MAX_PATH);
                char* slash = strrchr(mod_name, '\\');
                if (slash) memmove(mod_name, slash + 1, strlen(slash + 1) + 1);
            }

            if (SymFromAddr(process, addr, &displacement, sym)) {
                fprintf(g_file, "  [%d] %s!%s+0x%llX (0x%llX)\n", i, mod_name, sym->Name,
                    (unsigned long long)displacement, (unsigned long long)addr);
            } else {
                DWORD64 mod_base = hMod ? (DWORD64)(uintptr_t)hMod : 0;
                fprintf(g_file, "  [%d] %s+0x%llX (0x%llX)\n", i, mod_name,
                    (unsigned long long)(addr - mod_base), (unsigned long long)addr);
            }
        }
        fprintf(g_file, "=== END STACK TRACE ===\n\n");
        fflush(g_file);
        SymCleanup(process);
        LeaveCriticalSection(&g_cs);
    }

    inline LONG WINAPI crash_handler(EXCEPTION_POINTERS* ep) {
        if (!g_file) {
            if (g_old_filter) return g_old_filter(ep);
            return EXCEPTION_CONTINUE_SEARCH;
        }

        const char* exc_name = "UNKNOWN";
        switch (ep->ExceptionRecord->ExceptionCode) {
            case EXCEPTION_ACCESS_VIOLATION:     exc_name = "ACCESS_VIOLATION"; break;
            case EXCEPTION_STACK_OVERFLOW:        exc_name = "STACK_OVERFLOW"; break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:    exc_name = "DIVIDE_BY_ZERO"; break;
            case EXCEPTION_ILLEGAL_INSTRUCTION:   exc_name = "ILLEGAL_INSTRUCTION"; break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:    exc_name = "FLT_DIVIDE_BY_ZERO"; break;
            case EXCEPTION_BREAKPOINT:            exc_name = "BREAKPOINT"; break;
            case EXCEPTION_SINGLE_STEP:           exc_name = "SINGLE_STEP"; break;
        }

        write("CRASH", "UNHANDLED_EXCEPTION",
            "Exception 0x%08X (%s) at RIP=0x%llX",
            ep->ExceptionRecord->ExceptionCode, exc_name,
            (unsigned long long)ep->ContextRecord->Rip);

        if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
            ep->ExceptionRecord->NumberParameters >= 2) {
            const char* op = ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" : "WRITE";
            write("CRASH", "UNHANDLED_EXCEPTION",
                "%s access to address 0x%llX",
                op, (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
        }

        HMODULE hMod = NULL;
        char mod_name[MAX_PATH] = "???";
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (LPCSTR)ep->ContextRecord->Rip, &hMod)) {
            GetModuleFileNameA(hMod, mod_name, MAX_PATH);
            char* slash = strrchr(mod_name, '\\');
            if (slash) memmove(mod_name, slash + 1, strlen(slash + 1) + 1);
            write("CRASH", "UNHANDLED_EXCEPTION",
                "Faulting module: %s (base=0x%llX, offset=0x%llX)",
                mod_name, (unsigned long long)(uintptr_t)hMod,
                (unsigned long long)(ep->ContextRecord->Rip - (uintptr_t)hMod));
        }

        write_stacktrace(ep->ContextRecord);

        if (g_old_filter) return g_old_filter(ep);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    inline void init() {
        if (g_initialized) return;
        InitializeCriticalSection(&g_cs);
        g_start_tick = GetTickCount64();

        CreateDirectoryA("C:\\cortex_log", nullptr);

        SYSTEMTIME st;
        GetLocalTime(&st);
        sprintf_s(g_log_path, "C:\\cortex_log\\cortex_%04d%02d%02d_%02d%02d%02d.log",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        g_file = fopen(g_log_path, "w");
        if (g_file) {
            fprintf(g_file, "=== CORTEX LOG STARTED %04d-%02d-%02d %02d:%02d:%02d ===\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            fprintf(g_file, "PID: %lu  |  TID: %lu\n", GetCurrentProcessId(), GetCurrentThreadId());
            fprintf(g_file, "Log file: %s\n", g_log_path);
            fprintf(g_file, "================================================\n\n");
            fflush(g_file);
        }

        g_old_filter = SetUnhandledExceptionFilter(crash_handler);

        g_initialized = true;
    }

    inline void shutdown() {
        if (g_old_filter) {
            SetUnhandledExceptionFilter(g_old_filter);
            g_old_filter = nullptr;
        }
        if (g_file) {
            write("INFO", "SHUTDOWN", "Logger shutting down");
            fclose(g_file);
            g_file = nullptr;
        }
        if (g_initialized) {
            DeleteCriticalSection(&g_cs);
            g_initialized = false;
        }
    }
}

#define LOG_INFO(fmt, ...)  cortex_log::write("INFO",  __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  cortex_log::write("WARN",  __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   cortex_log::write("ERROR", __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   cortex_log::write("DEBUG", __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_CRASH(fmt, ...) cortex_log::write("CRASH", __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_PTR(name, ptr) cortex_log::write("DEBUG", __FUNCTION__, "%s = 0x%llX", name, (unsigned long long)(uintptr_t)(ptr))
