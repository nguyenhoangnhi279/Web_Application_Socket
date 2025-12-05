#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <iostream>
#include "../ThirdParty/json.hpp"
#include "Utils.h"

#pragma comment(lib, "psapi.lib")

using namespace std;
using json = nlohmann::json;

static json globalAppList;

class AppManager {
private:
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == 0) {
            wchar_t title[256];
            GetWindowTextW(hwnd, title, 256);
            if (wcslen(title) > 0) {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                string strTitle = Utils::WStringToString(title);
                
                bool exists = false;
                for (auto& item : globalAppList) {
                    if (item["pid"] == pid) { exists = true; break; }
                }
                if (!exists) {
                    globalAppList.push_back({
                        {"name", strTitle}, {"pid", (int)pid}, {"status", "Running"}
                    });
                }
            }
        }
        return TRUE;
    }

    static string GetMemoryUsage(DWORD pid) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (NULL == hProcess) return "N/A";
        PROCESS_MEMORY_COUNTERS pmc;
        string mem = "N/A";
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            char buffer[32];
            sprintf_s(buffer, "%.1f MB", pmc.WorkingSetSize / (1024.0 * 1024.0));
            mem = string(buffer);
        }
        CloseHandle(hProcess);
        return mem;
    }

public:
    static json GetRunningApps() {
        globalAppList = json::array();
        EnumWindows(EnumWindowsProc, 0);
        return globalAppList;
    }

    static json GetAllProcesses() {
        json processArray = json::array();
        HANDLE hProcessSnap;
        PROCESSENTRY32W pe32; // Dùng PROCESSENTRY32W (Wide char) thay vì PROCESSENTRY32

        // TH32CS_SNAPPROCESS vẫn dùng bình thường
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) return processArray;

        pe32.dwSize = sizeof(PROCESSENTRY32W);

        // Dùng Process32FirstW để lấy dữ liệu dạng Unicode (WCHAR)
        if (Process32FirstW(hProcessSnap, &pe32)) {
            do {
                // Lúc này szExeFile là WCHAR, hàm Utils sẽ chịu nhận
                string name = Utils::WStringToString(pe32.szExeFile);
                int pid = pe32.th32ProcessID;
                string ram = GetMemoryUsage(pid);

                processArray.push_back({
                    {"name", name},
                    {"pid", pid},
                    {"mem", ram}, 
                    {"cpu", ""} 
                });

            } while (Process32NextW(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);
        return processArray;
    }

    static void KillProcess(int pid) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess != NULL) {
            TerminateProcess(hProcess, 9);
            CloseHandle(hProcess);
        }
    }

    static void StartApp(string path) {
        int len = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
        wstring wPath(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wPath[0], len);
        ShellExecuteW(NULL, L"open", wPath.c_str(), NULL, NULL, SW_SHOW);
    }
};