#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <map>

#pragma comment(lib,"ws2_32.lib")

static int PORT = 6668;

SOCKET serverSocket;

std::map<int, SOCKADDR_IN> current;

std::string NormalizedIPString(SOCKADDR_IN addr) {
	char host[16];
	ZeroMemory(host, 16);
	inet_ntop(AF_INET, &addr.sin_addr, host, 16);

	USHORT port = ntohs(addr.sin_port);

	int realLen = 0;
	for (int i = 0; i < 16; i++) {
		if (host[i] == '\00') {
			break;
		}
		realLen++;
	}

	std::string res(host, realLen);
	res += ":" + std::to_string(port);

	return res;
}

void SendResponse(SOCKADDR_IN addr, SOCKADDR_IN receiver) {
	int size = sizeof(addr);

	std::string msg = NormalizedIPString(addr);

	sendto(serverSocket, msg.c_str(), msg.length(), 0, (sockaddr*)&receiver, sizeof(receiver));
}

int main() {	
	SetConsoleTitleA(("Server [" + std::to_string(PORT) + "]").c_str());

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 0;
	}

	SOCKADDR_IN sockAddr;
	sockAddr.sin_port = htons(PORT);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if (bind(serverSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
		return 0;
	}

	int val = 64 * 1024;
	setsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, (char*)&val, sizeof(val));
	setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, (char*)&val, sizeof(val));

	listen(serverSocket, 1000);

	while (true) {
		SOCKADDR_IN clientAddr;
		int clientSize = sizeof(clientAddr);

		char buffer[1024];
		int bufferLength = 1024;

		int iResult = recvfrom(serverSocket, buffer, bufferLength, 0, (sockaddr*)&clientAddr, &clientSize);

		if (iResult > 0) {
			try {
				int id = stoi(std::string(buffer, buffer + iResult));

				if (current.find(id) != current.end()) {
					SOCKADDR_IN other = current[id];

					SendResponse(other, clientAddr);
					SendResponse(clientAddr, other);

					std::cout << "Linked" << std::endl << "   ID: " << id << std::endl
						<< "   Endpoint1: " << NormalizedIPString(clientAddr) << std::endl
						<< "   Endpoint2: " << NormalizedIPString(other) << std::endl << std::endl;

					current.erase(id);
				}
				else {
					current.insert(std::make_pair(id, clientAddr));

					std::cout << "Registered" << std::endl << "   ID: " << id << std::endl 
						<< "   Endpoint: " << NormalizedIPString(clientAddr) << std::endl << std::endl;
				}
			}
			catch (std::invalid_argument& e) {}
			catch (std::out_of_range& e) {}
			catch (...) {}
		}

		Sleep(1);
	}

	closesocket(serverSocket);
	WSACleanup();
}