//Authentication server for the chat server to connect to.
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>

#include "Buffer.h"
#include "SQL_Wrapper.h"

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "6000"	//was 8899
//User sockets and buffer struct
struct userInfo
{
	SOCKET userSocket;
	Buffer userBuffer;
} newUser;

fd_set master;
SOCKET ListenSocket;
Buffer* g_theBuffer = new Buffer();
userInfo g_chatServerInfo;

//functions
void buildMessage(userInfo& theUser, std::string& message);
void sendAuthenticationServerMessage(userInfo& sendingUser, std::string message);
std::vector<std::string> readPacket(userInfo& theUser, int packetLength);
std::string parseMessage(int messageLength, Buffer& userBuffer);

int main() {
	//rework the server code to be here.
	fd_set readSet;
	FD_ZERO(&readSet);
	WSADATA wsaData;
	struct addrinfo* result = 0;
	struct addrinfo addressInfo;
	int iResult = 0;

	//create a socket for the server with the port 8899
	ZeroMemory(&addressInfo, sizeof(addressInfo));
	addressInfo.ai_family = AF_INET;
	addressInfo.ai_socktype = SOCK_STREAM;
	addressInfo.ai_protocol = IPPROTO_TCP;
	addressInfo.ai_flags = AI_PASSIVE;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	// Socket()
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	printf("Created Listen Socket\n");

	// Bind()
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &addressInfo, &result);
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind() failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Bind Listen Socket\n");

	// Listen()
	if (listen(ListenSocket, 5)) {
		printf("listen() failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("Listen for incoming requests\n");

	ULONG nonBlock = 1;
	//zero out master
	FD_ZERO(&master);
	FD_SET(ListenSocket, &master);

	bool running = true;
	std::cout << "Authentication Server\nStatus: Running\n" << std::endl;

	while (running)
	{
		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages.

		fd_set copy = master; 
		int socketCount = select(0, &copy, nullptr, nullptr, 0);

		//should only be one.
		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copy.fd_array[i];

			// Is it an inbound communication?
			if (sock == ListenSocket)
			{
				std::vector<std::string> results;
				// Accept a new connection
				g_chatServerInfo.userSocket = accept(ListenSocket, nullptr, nullptr);

				//Create the userInfo struct and make a new buffer for it
				g_chatServerInfo.userBuffer = Buffer();

				// Add the new connection to the list of connected clients
				FD_SET(g_chatServerInfo.userSocket, &master);

				std::cout << "Chat server successfully connected!";
				
				// Receive message
				int bytesIn;
				bytesIn = recv(sock, g_chatServerInfo.userBuffer.getBufferAsCharArray(), g_chatServerInfo.userBuffer.GetBufferLength(), 0);
				if (bytesIn > 0)
				{
					//do the conversion
					std::vector<std::string> results = readPacket(g_chatServerInfo, bytesIn);
				}
				else if (bytesIn == -1) {//print error message

				}
				else if (bytesIn == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {//print error message
					printf("receive failed with error: %i", WSAGetLastError());
				}
				// Send message to other clients, and definately NOT the listening socket
				

				if (results.size() > 1)
				{
					//if (theCommands[0] == "SM" || theCommands[0] == "sm")
					if (results[0] == "REGISTER" || results[0] == "register")
					{
						//TODO::
						//register the user
					
					}
				}//end if
			}//end if
		}//end for 
	}//end else

}


std::vector<std::string> readPacket(userInfo& theUser, int packetLength)
{
	std::string message = "";
	std::string command = "";
	int messageId = 0;
	int messageLength = 0;
	int commandLength = 0;
	std::vector<std::string> receviedMessages;

	//if the message is specific
	if (packetLength > 3)
	{
		message = "";
		//read the packet id and the command length
		messageId = theUser.userBuffer.ReadInt32BE();
		//get the command length
		commandLength = theUser.userBuffer.ReadInt32BE();
		//read the command 
		command = parseMessage(commandLength, theUser.userBuffer);
		//get message length
		messageLength = theUser.userBuffer.ReadInt32BE();
		//get message
		message = parseMessage(messageLength, theUser.userBuffer);
		//push back the messages
		receviedMessages.push_back(command);
		receviedMessages.push_back(message);
	}

	return receviedMessages;
}

void sendAuthenticationServerMessage(userInfo& sendingUser, std::string message) {
	//server message happens when client joins the server?
	//userInfo theUser = getClient(*sendingUser);
	buildMessage(sendingUser, message);

	int res = send(sendingUser.userSocket, sendingUser.userBuffer.getBufferAsCharArray(), sendingUser.userBuffer.GetBufferLength(), 0);
	if (res == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", res);
	}
}


void buildMessage(userInfo& theUser, std::string& message)
{
	theUser.userBuffer = Buffer();
	theUser.userBuffer.WriteInt32BE(message.size());
	theUser.userBuffer.WriteStringBE(message);
}


std::string parseMessage(int messageLength, Buffer& userBuffer) {
	std::string tempMessage = "";
	tempMessage += userBuffer.ReadStringBE(messageLength);
	return tempMessage;
}