#include <cstdlib>
#include <string>
#include <iostream>

using namespace std;

class PowerControl {
public:
    static void Execute(string action) {
        if (action == "shutdown") {
            //cout << ">> Executing: SHUTDOWN" << endl;
            system("shutdown /s /t 0");
        }
        else if (action == "restart") {
            //cout << ">> Executing: RESTART" << endl;
            system("shutdown /r /t 0");
        }
        else if (action == "logoff") {
            //cout << ">> Executing: LOGOFF" << endl;
            system("shutdown /l");
        }
    }
};