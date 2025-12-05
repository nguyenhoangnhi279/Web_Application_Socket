#pragma once

#include <string>
#include <iostream>
#include "../Network/ServerNetwork.h" // Để hiểu kiểu dữ liệu ServerNetwork&
#include "../ThirdParty/json.hpp"     // Thư viện JSON

using namespace std;

class CommandHandler {
public:
    // Input: Chuỗi JSON nhận từ Web, và Biến Server để gửi phản hồi lại
    static void Process(string jsonMessage, ServerNetwork& server);
};