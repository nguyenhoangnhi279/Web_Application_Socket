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

    bool AcceptClient();

    string ReceiveMessage();
    void SendFrame(string message);

private:
    bool WebSocketHandshake(SOCKET client);
};