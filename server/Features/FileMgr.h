#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "Utils.h"

using namespace std;

class FileMgr {
public:
    static bool SaveFile(string fileName, string base64Data) {
        try {
            char* userProfile = getenv("USERPROFILE");
            
            if (userProfile == nullptr) {
                cout << ">> [LOI] Khong tim thay bien moi truong USERPROFILE." << endl;
                return false;
            }

            string fullPath = string(userProfile) + "\\Downloads\\" + fileName;

            string binaryData = Utils::Base64Decode(base64Data);

            ofstream outFile(fullPath, ios::binary);
            
            if (outFile.is_open()) {
                outFile.write(binaryData.data(), binaryData.size());
                outFile.close();
                cout << ">> [FILE] Da luu tai: " << fullPath << endl;
                return true;
            } else {
                cout << ">> [LOI] Khong the ghi file tai: " << fullPath << endl;
                return false;
            }
        } catch (...) {
            return false;
        }
    }
};