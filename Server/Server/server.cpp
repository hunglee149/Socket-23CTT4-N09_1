#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>

#define PORT 23444
#define BUFFER_SIZE 1024

std::mutex consoleMutex;

void handleClient(SOCKET acceptedSocket)
{
	char buffer[BUFFER_SIZE] = { 0 };

	{
		std::lock_guard<std::mutex> lock(consoleMutex);
		std::cout << "Sending file list to client socket " << acceptedSocket << std::endl;
	}
	
	std::fstream fileList("file_list.txt");
	if (!fileList.is_open())
	{
		std::cout << "Error opeing file" << std::endl;
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	std::string line;
	
	while (getline(fileList, line))
	{
		strcpy(buffer, line.c_str());
		send(acceptedSocket, buffer, sizeof(buffer), 0);
		memset(buffer, 0, sizeof(buffer));
	}
	fileList.close();
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "ACK");
	send(acceptedSocket, buffer, sizeof(buffer), 0);
	memset(buffer, 0, sizeof(buffer));
	
	std::string test;
	int nRet = 1;
	while (1)
	{
		nRet = recv(acceptedSocket, buffer, sizeof(buffer), 0);
		if (nRet <= 0)
		{
			{
				std::lock_guard<std::mutex> lock(consoleMutex);
				std::cout << "Client " << acceptedSocket << " disconnected" << std::endl;
			}
			break;
		}
		test = buffer;
		if (test == "ACK") 
		{
			{
				std::lock_guard<std::mutex> lock(consoleMutex);
				std::cout << "Client " << acceptedSocket << " finished" << std::endl;
			}
			break;
		}

		{
			std::lock_guard<std::mutex> lock(consoleMutex);
			std::cout << "Sending file: " << test << " to client " << acceptedSocket << std::endl;
		}

		std::ifstream fileSend(test, std::ios::binary);
		if (!fileSend)
		{
			std::cout << "Error opening file" << std::endl;
			WSACleanup();
			continue;
		}

		while (!fileSend.eof())
		{
			fileSend.read(buffer, BUFFER_SIZE);
			nRet = fileSend.gcount();
			send(acceptedSocket, buffer, nRet, 0);
		}
		{
			std::lock_guard<std::mutex> lock(consoleMutex);
			std::cout << "File " << test << " sent successfully" << std::endl;
		}
		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer, "ACK");
		send(acceptedSocket, buffer, sizeof(buffer), 0);
		memset(buffer, 0, sizeof(buffer));

		fileSend.close();
	}
	closesocket(acceptedSocket);
}

int main() 
{
	int nRet = 1;
	WSADATA ws;
	WORD wVersionRequest = MAKEWORD(2, 2);
	nRet = WSAStartup(wVersionRequest, &ws);

	if (nRet)
	{
		std::cout << "The winsock dll not found" << std::endl;
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "The winsock dll found" << std::endl;
		std::cout << "The status: " << ws.szSystemStatus << std::endl;
	}

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "socket() is OK" << std::endl;
	}
	
	struct sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(PORT);
	service.sin_addr.s_addr = INADDR_ANY;
	memset(&(service.sin_zero), 0, 8);
	nRet = bind(serverSocket, (sockaddr*)&service, sizeof(service));
	if (nRet != 0)
	{
		std::cout << "bind() failed: " << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "bind() is OK" << std::endl;
	}
	nRet = listen(serverSocket, 5);
	if (nRet != 0)
	{
		std::cout << "listen() failed: Error listening on socket" << WSAGetLastError() << std::endl;
	}
	else
	{
		std::cout << "listen() is OK" << std::endl;
		std::cout << "Server is waiting for connections..." << std::endl;
	}
	while (true)
	{
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cout << "Failed in accepting socket: " << WSAGetLastError() << std::endl;
			continue;
		}

		std::thread clientThread(handleClient, clientSocket);
		clientThread.detach();
	}
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}