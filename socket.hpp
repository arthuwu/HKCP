#pragma once
#include "EuroScopePlugIn.h"
#include "stdafx.h"
#include "winsock2.h"
#include "windows.h"
#include "thread"
#include "iostream"
#include "socket.hpp"
#include "Constant.hpp"
#include "ws2tcpip.h"
#include <nlohmann/json.hpp>

using namespace std;
using namespace EuroScopePlugIn;
using json = nlohmann::json;

class TcpServer :
    public EuroScopePlugIn::CPlugIn
{
public:
    TcpServer(int port = 9000);
    ~TcpServer();

    void start();
    void stop();

private:
    void serverLoop();
    void handleClient(SOCKET clientSocket);
    void pushMessageToES(string msg);
    string handleRequests(json req);

    int port_;
    bool running_;
    SOCKET serverSocket_;
    std::thread serverThread_;
};
