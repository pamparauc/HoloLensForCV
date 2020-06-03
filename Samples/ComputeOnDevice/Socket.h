#pragma once
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

void start_socket()
{
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOK = WSAStartup(ver, &wsData);
	if (wsOK != 0)
	{
		return;
	}
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		return;
	}
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(listening, (sockaddr*)&hint, sizeof(hint));
	listen(listening, SOMAXCONN);

	sockaddr_in client;
	int clientSize = sizeof(client);
	SOCKET clientsocket = accept(listening, (sockaddr*)&client, &clientSize);
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	ZeroMemory(host, NI_MAXSERV);
	ZeroMemory(service, NI_MAXHOST);
	
	closesocket(listening);

	char buf[4096];
	while (true)
	{
		ZeroMemory(buf, 4096);
		int bytesReceived = recv(clientsocket, buf, 4096, 0);
		if (bytesReceived == 0)
		{
			break;
		}
		send(clientsocket, buf, bytesReceived + 1, 0);


	}
	closesocket(clientsocket);
	WSACleanup();
}