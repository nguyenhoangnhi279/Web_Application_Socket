#include <windows.h>
#include <objidl.h> 
#include <gdiplus.h>
#include <vector>
#include <iostream>
#include "Utils.h"
#include <shellscalingapi.h>

#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib, "gdi32.lib")
#pragma comment(lib, "Shcore.lib")



using namespace std;
using namespace Gdiplus;

struct GdiPlusManager {
    ULONG_PTR gdiplusToken;
    GdiPlusManager() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~GdiPlusManager() {
        GdiplusShutdown(gdiplusToken);
    }
};


class ScreenCapture {
public:
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
        UINT num = 0, size = 0;
        GetImageEncodersSize(&num, &size);
        if (size == 0) return -1;

        ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
        if (!pImageCodecInfo) return -1;

        GetImageEncoders(num, size, pImageCodecInfo);
        for (UINT j = 0; j < num; ++j) {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j;
            }
        }
        free(pImageCodecInfo);
        return -1;
    }

 static string TakeScreenshot() {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    static GdiPlusManager gdiManager;

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return "";

    int vLeft   = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vTop    = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vWidth  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (vWidth <= 0 || vHeight <= 0) {
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    int physW = GetDeviceCaps(hdcScreen, DESKTOPHORZRES);
    int physH = GetDeviceCaps(hdcScreen, DESKTOPVERTRES);

    double scaleX = (vWidth > 0) ? (double)physW / (double)vWidth : 1.0;
    double scaleY = (vHeight > 0) ? (double)physH / (double)vHeight : 1.0;

    int width  = (int)round(vWidth * scaleX);
    int height = (int)round(vHeight * scaleY);
    if (width <= 0 || height <= 0) {
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBitmap) {
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        DeleteObject(hBitmap);
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    HGDIOBJ oldBmp = SelectObject(hdcMem, hBitmap);

    int oldMode = SetStretchBltMode(hdcMem, HALFTONE);
    SetBrushOrgEx(hdcMem, 0, 0, NULL);

    BOOL ok = StretchBlt(
        hdcMem,
        0, 0, width, height,
        hdcScreen,
        vLeft, vTop, vWidth, vHeight,
        SRCCOPY
    );

    if (!ok) {
        int copyW = min(width, vWidth);
        int copyH = min(height, vHeight);
        BitBlt(hdcMem, 0, 0, copyW, copyH, hdcScreen, vLeft, vTop, SRCCOPY);
    }

    Bitmap bmp(width, height, width * 4, PixelFormat32bppARGB, (BYTE*)pBits);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK) {
        SelectObject(hdcMem, oldBmp);
        SetStretchBltMode(hdcMem, oldMode);
        DeleteDC(hdcMem);
        DeleteObject(hBitmap);
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    CLSID clsid;
    if (GetEncoderClsid(L"image/jpeg", &clsid) < 0) {
        pStream->Release();
        SelectObject(hdcMem, oldBmp);
        SetStretchBltMode(hdcMem, oldMode);
        DeleteDC(hdcMem);
        DeleteObject(hBitmap);
        ReleaseDC(NULL, hdcScreen);
        return "";
    }

    EncoderParameters params;
    params.Count = 1;
    ULONG quality = 70;
    params.Parameter[0].Guid = EncoderQuality;
    params.Parameter[0].Type = EncoderParameterValueTypeLong;
    params.Parameter[0].NumberOfValues = 1;
    params.Parameter[0].Value = &quality;

    bmp.Save(pStream, &clsid, &params);

    LARGE_INTEGER liZero = {};
    ULARGE_INTEGER ulSize = {};
    pStream->Seek(liZero, STREAM_SEEK_END, &ulSize);
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);

    vector<unsigned char> buffer((size_t)ulSize.QuadPart);
    ULONG bytesRead = 0;
    pStream->Read(buffer.data(), (ULONG)buffer.size(), &bytesRead);

    pStream->Release();
    SelectObject(hdcMem, oldBmp);
    SetStretchBltMode(hdcMem, oldMode);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
    ReleaseDC(NULL, hdcScreen);

    return Utils::Base64Encode(buffer.data(), buffer.size());
}
};