#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")

using namespace std;

class ServerNetwork {
public:
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    // Khởi tạo Server tại Port (ví dụ 8080)
    bool Init(int port);

    // Chờ Client kết nối (Hàm này sẽ dừng chương trình đến khi có người kết nối)
    bool AcceptClient();

    // Nhận tin nhắn từ Client (Trả về chuỗi JSON)
    string ReceiveMessage();

    // Gửi tin nhắn về Client (Gửi chuỗi JSON)
    void SendFrame(string message);

private:
    // Hàm xử lý bắt tay WebSocket (Chỉ dùng nội bộ)
    bool WebSocketHandshake(SOCKET client);
};