#pragma once
#include <vector>
#include "xorstr.h"

#if defined(_MT) && defined(_DLL)
// Multithreaded DLL (/MD or /MDd)
#pragma comment(lib, "TrashAuth_x64_1.0.3_MD.lib")
#elif defined(_MT)
// Multithreaded static (/MT or /MTd)
#pragma comment(lib, "TrashAuth_x64_1.0.3_MT.lib")
#else
#error "Unknown or unsupported runtime configuration"
#endif

namespace TrashAuth {
    bool init_protection(bool advanced_mode); // Details regarding Advanced Mode are listed below.
    bool check_protection();
    bool check_license(std::string app_id, std::string serial_key);
    std::vector<unsigned char> get_file(std::string file_id);
    std::string get_variable(std::string variable_id);
    extern std::string get_version;
    extern std::string get_expiredate;
    extern std::string get_reason;
    extern bool initialized; // Its use is mandatory.
}

// Advanced Mode:
// Using Advanced Mode adds an extra layer of security to your code, but it has some incompatibilities.
// Known incompatibilities are listed below.
// 1. Using the "Files" tab in VMProtect may cause the program to fail to open.
// 2. Using "Advanced API-Wrapping" in Themida may cause the program to fail to open.
// If you do not use these features, it is recommended that you activate Advanced Mode.

// https://trashauth.com/
// Version: 1.0.3
