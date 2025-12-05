#pragma once
#include <windows.h>
#include <string>
#include <vector>

using namespace std;

class Utils {
public:
    // Chuyển chuỗi rộng (Windows WCHAR) sang chuỗi thường (UTF-8 cho JSON)
    static string WStringToString(const wstring& wstr) {
        if (wstr.empty()) return string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    // Mã hóa Base64 (Dùng để gửi ảnh và video qua WebSocket)
    static string Base64Encode(const unsigned char* bytes, int len) {
        static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        string out;
        int val = 0, valb = -6;
        for (int i = 0; i < len; i++) {
            val = (val << 8) + bytes[i];
            valb += 8;
            while (valb >= 0) {
                out.push_back(b64[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }
};