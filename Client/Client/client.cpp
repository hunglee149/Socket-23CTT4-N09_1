#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>	
#include <stdio.h>
#include <set>
#include <fstream>
#include <string>
#include <cstdlib>
#include <signal.h>

using namespace std;
#define PORT 23444
#define BUFFER_SIZE 1024

struct sockaddr_in clientService;
int nRet;
char buffer[BUFFER_SIZE] = { 0 };

atomic<bool> stopLoop(false);

void signal_callback_handler(int signum) 
{
    cout << "Caught signal " << signum << endl;
    stopLoop = true;
}

void downloadFile(string fileName, SOCKET clientSocket) 
{
    strcpy(buffer, fileName.c_str());
    send(clientSocket, buffer, sizeof(buffer), 0);
    memset(buffer, 0, sizeof(buffer));

    ofstream receivedFile(fileName, ios::binary);
    if (!receivedFile)
    {
        cout << "Error opening file" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    while ((nRet = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        string test = buffer;
        if (test == "ACK") {
            break;
        }
        receivedFile.write(buffer, nRet);
    }
    receivedFile.close();
}

int main() 
{
    signal(SIGINT, signal_callback_handler);
    WSADATA ws;
    WORD wVersionRequest = MAKEWORD(2, 2);
    nRet = WSAStartup(wVersionRequest, &ws);
    if (nRet)
    {
        cout << "The winsock dll not found" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "The winsock dll found" << endl;
        cout << "the status: " << ws.szSystemStatus << endl;
    }

    SOCKET clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "socket() is OK" << endl;
    }

    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(PORT);
    clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(clientService.sin_zero), 0, 8);
    nRet = connect(clientSocket, (sockaddr*)&clientService, sizeof(clientService));
    if (nRet != 0)
    {
        cout << "Client: connect() failed" << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << "Client: connect() is OK" << endl;
        cout << "client: start sending and receiving data..." << endl;
    }

    string test;
    while ((nRet = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        test = buffer;
        if (test == "ACK")
        {
            cout << "Received file list successfully" << endl;
            break;
        }
        cout << buffer << endl;
        memset(buffer, 0, sizeof(buffer));
    }

    set<string> downloadedFiles;

    while (!stopLoop)
    {
        fstream input("input.txt");
        if (!input.is_open())
        {
            cout << "Error opening input file" << endl;
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        string temp;
        while (getline(input, temp))
        {
            temp.erase(0, temp.find_first_not_of(" \t"));
            temp.erase(temp.find_last_not_of(" \t") + 1);

            if (!temp.empty() && downloadedFiles.find(temp) == downloadedFiles.end())
            {
                cout << "Downloading " << temp << endl;
                downloadFile(temp, clientSocket);
                cout << "Downloaded " << temp << endl;
                downloadedFiles.insert(temp);
            }
        }
        input.close();
        Sleep(5000);
    }

    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "ACK");
    send(clientSocket, buffer, sizeof(buffer), 0);
    memset(buffer, 0, sizeof(buffer));

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}