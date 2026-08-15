#include "utils.hpp"

int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
    CustomizeConsoleWindow();

    wstring dllPath;
    if (argc > 1) {
        dllPath = argv[1];
        cout << "[+] dll from argument: ";
        wcout << dllPath << endl;
    } else {
        cout << "[+] please select ur dll..." << endl;
        dllPath = SelectDLL();
    }
    if (dllPath.empty()) {
        cout << "[-] error: no dll selected" << endl;
        system("pause");
        return EXIT_FAILURE;
    }

    cout << "[+] searching for exported functions..." << endl;
    wstring functionName = GetExportedFunctionName(dllPath);
    if (functionName.empty()) {
        cout << "[-] error: no valid export function found" << endl;
        system("pause");
        return EXIT_FAILURE;
    }
    cout << "[+] found export: ";
    wcout << functionName << endl;

    cout << "[+] locating target window..." << endl;
    HWND hwnd = FindWindowW(L"VALORANTUnrealWindow", NULL);
    if (!hwnd) {
        cout << "[-] error: target window not found" << endl;
        system("pause");
        return EXIT_FAILURE;
    }

    DWORD pid = NULL;
    DWORD tid = GetWindowThreadProcessId(hwnd, &pid);
    if (!tid) {
        cout << "[-] error: failed to get thread id" << endl;
        system("pause");
        return EXIT_FAILURE;
    }

    if (!SetupHook(dllPath, functionName, tid)) {
        system("pause");
        return EXIT_FAILURE;
    }

    cout << "[+] hook active (function: ";
    wcout << functionName;
    cout << ")" << endl;

    if (argc > 1) {
        cout << "[+] injection completed, hiding..." << endl;
        Sleep(200);
        HWND consoleWnd = GetConsoleWindow();
        if (consoleWnd) ShowWindow(consoleWnd, SW_HIDE);
        Sleep(INFINITE);
    }

    cout << "[>] press any key to unhook" << endl;
    system("pause > nul");

    cout << "[+] removing hook..." << endl;
    if (!RemoveHook()) {
        cout << "[-] error: failed to remove hook" << endl;
        system("pause");
        return EXIT_FAILURE;
    }

    cout << "[+] operation completed" << endl;
    cout << "[>] press any key to exit" << endl;
    system("pause > nul");
    return EXIT_SUCCESS;
}