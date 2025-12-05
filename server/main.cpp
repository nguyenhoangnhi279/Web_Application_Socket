#include "Network/ServerNetwork.h"
#include "Core/CommandHandler.h"
#include <iostream>

using namespace std;

int main() {
    ServerNetwork server;
    
    // Nếu máy mày bị trùng port, đổi 8080 thành 9000 (nhớ đổi cả trong index.html)
    if (!server.Init(8080)) {
        cout << ">> Loi: Khong the mo Port 8080. (Co the do Skype/Discord dang chiem?)" << endl;
        cin.get();
        return 1;
    }

    cout << "============================================" << endl;
    cout << "   REMOTE CONTROL SERVER IS RUNNING...      " << endl;
    cout << "   Waiting for Web Client...                " << endl;
    cout << "============================================" << endl;

    while (true) {
        if (server.AcceptClient()) {
            cout << ">> [Main] Client connected!" << endl;
            
            while (true) {
                string msg = server.ReceiveMessage();
                
                if (msg == "" || msg == "DISCONNECT") {
                    cout << ">> [Main] Client disconnected." << endl;
                    break;
                }

                CommandHandler::Process(msg, server);
            }
        }
        cout << ">> [Main] Waiting for new connection..." << endl;
    }
    return 0;
}