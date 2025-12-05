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
        string cameraName = "Integrated Camera"; // Thay tên cam của bạn vào đây nếu cần
        
        if (!std::filesystem::exists("Tools\\ffmpeg.exe")) {
            cout << ">> [LOI] Thieu Tools\\ffmpeg.exe" << endl;
            return "";
        }

        // --- CẤU HÌNH LỆNH FFMPEG SIÊU NHẸ ---
        // -vf scale=640:480 : Giảm độ phân giải xuống SD (nhẹ hơn HD rất nhiều)
        // -r 15             : Giảm xuống 15 khung hình/giây (vẫn nhìn rõ, dung lượng giảm 1/2)
        // -b:v 250k         : Ép Bitrate xuống 250k (Video sẽ hơi mờ nhưng cực nhẹ)
        // -preset ultrafast : Nén cực nhanh để không bị trễ
        
        // Thêm -loglevel quiet để nó câm miệng hoàn toàn
        // Thêm > NUL 2>&1 ở cuối để chặn mọi output ra console
        string cmd = "Tools\\ffmpeg.exe -f dshow -i video=\"" + cameraName + "\" "
             "-t " + to_string(duration) + " " 
             "-vf scale=480:360 -r 20 -b:v 300k -c:v libx264 -preset ultrafast "
             "-threads 2 "       // Giảm tải CPU
             "-loglevel quiet "  // Tắt log đỡ rác
             "-y webcam_out.mp4 > NUL 2>&1";

        cout << ">> [WebcamMgr] Dang quay.." << endl;
        
        int result = system(cmd.c_str());
        
        // Kiểm tra file
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
            // In ra dung lượng để bạn kiểm tra (Nếu < 500KB là đẹp)
            cout << ">> [WebcamMgr] Video size: " << size / 1024 << " KB." << endl;
            return Utils::Base64Encode((unsigned char*)buffer.data(), (int)size);
        }
        
        return "";
    }
};