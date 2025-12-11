#pragma once

#include <string>
#include <iostream>
#include "../Network/ServerNetwork.h" 
#include "../ThirdParty/json.hpp"    

using namespace std;

class CommandHandler {
public:
    static void Process(string jsonMessage, ServerNetwork& server);
};