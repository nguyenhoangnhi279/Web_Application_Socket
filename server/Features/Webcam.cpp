#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include "Utils.h"

using namespace std;

class WebcamMgr {
public:
    static string RecordVideo(int duration) {
        string cameraName = "Integrated Camera"; //Camera's name in system
        
        if (!std::filesystem::exists("Tools\\ffmpeg.exe")) {
            cout << ">> [LOI] Thieu Tools\\ffmpeg.exe" << endl;
            return "";
        }
        string cmd = "Tools\\ffmpeg.exe -f dshow -i video=\"" + cameraName + "\" "
             "-t " + to_string(duration) + " " 
             "-vf scale=480:360 -r 20 -b:v 300k -c:v libx264 -preset ultrafast "
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