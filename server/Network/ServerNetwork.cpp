#include "ServerNetwork.h"
#include <wincrypt.h> 

string base64_encode(const unsigned char* bytes, int len) {
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

bool ServerNetwork::Init(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    string portStr = to_string(port);
    if (getaddrinfo(NULL, portStr.c_str(), &hints, &result) != 0) return false;

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) return false;

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) return false;
    freeaddrinfo(result);

    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) return false;
    
    cout << ">> Server dang chay tai PORT: " << port << endl;
    return true;
}

bool ServerNetwork::AcceptClient() {
    cout << ">> Dang cho Client ket noi..." << endl;
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) return false;

    return WebSocketHandshake(ClientSocket);
}

bool ServerNetwork::WebSocketHandshake(SOCKET client) {
    char buffer[1024];
    int received = recv(client, buffer, 1024, 0);
    if (received <= 0) return false;

    string req(buffer, received);
    string keyHeader = "Sec-WebSocket-Key: ";
    size_t pos = req.find(keyHeader);
    if (pos == string::npos) return false;

    size_t end = req.find("\r\n", pos);
    string key = req.substr(pos + keyHeader.length(), end - (pos + keyHeader.length()));
    string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    string acceptKey = key + magic;

    HCRYPTPROV hProv = 0; HCRYPTHASH hHash = 0;
    BYTE hashBytes[20]; DWORD hashLen = 20;

    CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    CryptHashData(hHash, (BYTE*)acceptKey.c_str(), acceptKey.length(), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashLen, 0);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    string encoded = base64_encode(hashBytes, 20);
    string response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + encoded + "\r\n\r\n";
    
    send(client, response.c_str(), response.length(), 0);
    cout << ">> Handshake thanh cong!" << endl;
    return true;
}

string ServerNetwork::ReceiveMessage() {
    unsigned char buffer[8192];
    int received = recv(ClientSocket, (char*)buffer, 8192, 0);
    if (received <= 0) return ""; 

    if ((buffer[0] & 0x0F) == 0x08) return "DISCONNECT";

    unsigned long long payloadLen = buffer[1] & 127;
    int maskOffset = 2;
    
    if (payloadLen == 126) maskOffset = 4;
    else if (payloadLen == 127) maskOffset = 10;

    unsigned char mask[4];
    for (int i = 0; i < 4; i++) mask[i] = buffer[maskOffset + i];

    string output = "";
    for (int i = maskOffset + 4; i < received; i++) {
        output += (char)(buffer[i] ^ mask[(i - (maskOffset + 4)) % 4]);
    }
    return output;
}

void ServerNetwork::SendFrame(string msg) {
    vector<unsigned char> frame;
    frame.push_back(0x81); // Text Frame

    unsigned long long len = msg.size();

    if (len <= 125) {
        frame.push_back((unsigned char)len);
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        // Hỗ trợ gói tin cực lớn (64-bit length)
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (8 * i)) & 0xFF);
        }
    }

    frame.insert(frame.end(), msg.begin(), msg.end());
   
    const char* dataPtr = (const char*)frame.data();
    size_t totalSent = 0;
    size_t dataLeft = frame.size();

    while (totalSent < frame.size()) {
        int sent = send(ClientSocket, dataPtr + totalSent, (int)dataLeft, 0);
        if (sent == SOCKET_ERROR) {
            cout << ">> [Loi] Khong gui duoc du lieu (Socket Error)!" << endl;
            break;
        }
        totalSent += sent;
        dataLeft -= sent;
    }
}