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

#pragma comment(lib, "Ws2_32.lib")

using namespace std;
using namespace EuroScopePlugIn;

TcpServer::TcpServer(int port)
    : port_(port), running_(false), serverSocket_(INVALID_SOCKET), CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, MY_PLUGIN_NAME, MY_PLUGIN_VERSION, MY_PLUGIN_DEVELOPER, MY_PLUGIN_COPYRIGHT) {}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start() {
    if (running_) return;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        string msg = "WSAStartup failed\n";
        pushMessageToES(msg);
        return;
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == INVALID_SOCKET) {
        string msg = "Failed to create socket\n";
        pushMessageToES(msg);
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    InetPton(AF_INET, "127.0.0.1", &serverAddr.sin_addr.s_addr);

    if (::bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        string msg = "Bind failed\n";
        pushMessageToES(msg);
        closesocket(serverSocket_);
        return;
    }

    if (listen(serverSocket_, 3) == SOCKET_ERROR) {
        string msg = "Listen failed\n";
        pushMessageToES(msg);
        closesocket(serverSocket_);
        return;
    }

    running_ = true;
    serverThread_ = thread(&TcpServer::serverLoop, this);
    serverThread_.detach();

    string msg = "TCP Server started on port " + port_;
    pushMessageToES(msg);
}

void TcpServer::stop() {
    running_ = false;
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
    WSACleanup();
}

void TcpServer::serverLoop() {
    while (running_) {
        SOCKET clientSocket = accept(serverSocket_, nullptr, nullptr);
        if (clientSocket != INVALID_SOCKET) {
            thread(&TcpServer::handleClient, this, clientSocket).detach();
        }
    }
}

void TcpServer::handleClient(SOCKET clientSocket) {
    char buffer[512];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        string request(buffer, bytesReceived);

        string debugRes = "RECIEVED MESSAGE: " + request;
        pushMessageToES(debugRes);

        json req = json::parse(request);
        string res = handleRequests(req);

        send(clientSocket, res.c_str(), res.length(), 0);
    }
    closesocket(clientSocket);
}

void TcpServer::pushMessageToES(string msg) {
    DisplayUserMessage("HKCP", "HKCP", msg.c_str(), true, true, true, true, false);
}

string TcpServer::handleRequests(json req) {
    //TODO: Make this not the worst selection statement youve seen in your life
    string type = req["type"];

    if (type == "SET") {
        try {
            string callsign = req["callsign"];
            CFlightPlan fp = FlightPlanSelect(callsign.c_str());

            if (req["request"] == "STATUS: STUP") {
                fp.GetControllerAssignedData().SetScratchPadString("ST-UP");
            }
            else if (req["request"] == "STATUS: PUSH") {
                fp.GetControllerAssignedData().SetScratchPadString("PUSH");
            }
            else if (req["request"] == "STATUS: TAXI") {
                fp.GetControllerAssignedData().SetScratchPadString("TAXI");
            }
            else if (req["request"] == "STATUS: DEPA") {
                fp.GetControllerAssignedData().SetScratchPadString("DEPA");
            }
            else if (req["request"] == "FREETEXT") {
                string ft = req["contents"];
                fp.GetControllerAssignedData().SetScratchPadString(ft.c_str());
            }
            return "OK";
        }
        catch (...) {
            return "FAILED";
        }
    }
    else if (type == "GET") {
        try {
            string callsign = req["callsign"];
            CFlightPlan fp = FlightPlanSelect(callsign.c_str());
            return fp.GetGroundState();
        }
        catch (...) {
            return "OK";
        }
    }
    else {
        return "FAILED";
    }
}