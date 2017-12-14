#define UNICODE
#define WIN_32_CHAT_APP_SERVER

//#include <Windows.h>   freaks out if i include this
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Buffer.h"
#include <iostream>
#include <sstream>
#include <map>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5000"	//was 8899
#define DEFAULT_BUFFER_LENGTH 1024
//socket info structure to store all the individual socket information

//User sockets and buffer struct
struct userInfo
{
	SOCKET userSocket;
	Buffer userBuffer;
} newUser;

//Globel variables
enum message_ID { JOINROOM, LEAVEROOM, SENDMESSAGE, RECEIVEMESSAGE };
std::map<char, std::vector<userInfo*>> roomMap;
std::vector<userInfo> usersInServer;
fd_set master;
SOCKET ListenSocket;
Buffer* g_theBuffer = new Buffer();
std::string parseMessage(int messageLength, Buffer& userBuffer);

//Protocols method headers
void sendMessage(SOCKET* sendingUser, std::string message);
void joinRoom(userInfo joinUser, char &roomName);
void leaveRoom(userInfo leaveUserInfo, char &roomName);
std::vector<std::string> readPacket(userInfo& theUser,int packetlength);
void buildMessage(userInfo& theUser,std::string& message);
userInfo getClient(SOCKET& theSock);
void sendServerMessage(SOCKET* sendingUser, std::string message);


int g_IDCounter = 0;

int main()
{
	SOCKET AcceptSocket;
	fd_set readSet;
	fd_set writeSet;
	FD_ZERO(&readSet);
	WSADATA wsaData;
	struct addrinfo* result = 0;
	struct addrinfo addressInfo;
	int iResult = 0;
	int totalSocketsInSet = 0;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SendBytes;

	//populating the roomName with rooms (a-z)
	char *alpha = "abcdefghijklmnopqrstuvwxyz";
	for (int i = 0; alpha[i] != '\0'; i++)
	{
		roomMap[alpha[i]];
	}

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
	//socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)

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

	//for debugging
	char tempBreak;
	bool running = true;

	while (running)
	{
		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages.

		fd_set copy = master;
		//delay value for the select call.
		timeval t_Delay;
		t_Delay.tv_sec = 1; // seconds
		t_Delay.tv_usec = 0; // micro seconds

							 // See who's talking to us
		int socketCount = select(0, &copy, nullptr, nullptr, &t_Delay);

		// Loop through all the current connections / potential connect
		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copy.fd_array[i];

			// Is it an inbound communication?
			if (sock == ListenSocket)
			{
				// Accept a new connection
				SOCKET client = accept(ListenSocket, nullptr, nullptr);

				//Create the userInfo struct and add them to the list of users
				newUser.userBuffer = Buffer();
				newUser.userSocket = client;

				//Assigns the new user to the hub room.
				usersInServer.push_back(newUser);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				// Send a welcome message to the connected client
				std::string welcomeMsg = "Welcome to the Awesome Chat Server!";
				//send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
				sendServerMessage(&newUser.userSocket, welcomeMsg);
			}
			else // It's an inbound message
			{
				//g_theBuffer = new Buffer();
				userInfo currInfo = getClient(sock);

				// Receive message
				int bytesIn;
				bytesIn = recv(sock, currInfo.userBuffer.getBufferAsCharArray(), currInfo.userBuffer.GetBufferLength(), 0);

				if (bytesIn < 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					// Send message to other clients, and definately NOT the listening socket
					std::vector<std::string> results = readPacket(currInfo,bytesIn);
					

					if (results.size() > 1)
					{
						//if (theCommands[0] == "SM" || theCommands[0] == "sm")
						if (results[0] == "SM" || results[0] == "sm")
						{
							sendMessage(&sock, results[1]);
						}
						else if (results[0] == "JR" || results[0] == "jr")
						{
							
							joinRoom(currInfo, results[1][0]);
						}
						else if (results[0] == "LR" || results[0] == "lr")
						{
							leaveRoom(currInfo, results[1][0]);
						}
					}
				}
			}
			//clear the buffer for the next set of info
			g_theBuffer = new Buffer();
		}
	}

	//clean up
	closesocket(ListenSocket);
	WSACleanup();
}

userInfo getClient(SOCKET& theSock) {
	userInfo currInfo;
	for (int i = 0; i < usersInServer.size(); i++)
	{
		if (usersInServer[i].userSocket == theSock)
		{
			currInfo = usersInServer[i];
		}
	}
	return currInfo;
}

std::string parseMessage(int messageLength, Buffer& userBuffer) {
	std::string tempMessage = "";
	tempMessage += userBuffer.ReadStringBE(messageLength);
	return tempMessage;
}

std::vector<std::string> readPacket(userInfo& theUser,int packetLength)
{
	std::string message = "";
	std::string command = "";
	int messageId = 0;
	int messageLength = 0;
	int commandLength = 0;
	std::vector<std::string> receviedMessages;

	////if the message is just a messg
	//if (packetLength == 3)
	//{
	//	message = "";
	//	messageId = g_theBuffer->ReadInt32BE();
	//	messageLength = g_theBuffer->ReadInt32BE();

	//	message = parseMessage(messageLength);
	//}

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

void sendServerMessage(SOCKET* sendingUser, std::string message) {
	//server message happens when client joins the server?
	userInfo theUser = getClient(*sendingUser);
	buildMessage(theUser, message);

	int res = send(*sendingUser, theUser.userBuffer.getBufferAsCharArray(), theUser.userBuffer.GetBufferLength(), 0);
	if (res == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", res);
	}
}

void sendMessage(SOCKET* sendingUser, std::string message)
{
	userInfo theUser = getClient(*sendingUser);
	buildMessage(theUser,message);

	for (int i = 0; i < master.fd_count; i++)
	{
		SOCKET outSock = master.fd_array[i];
		if (outSock != ListenSocket && outSock != *sendingUser)
		{
			int res = send(outSock, theUser.userBuffer.getBufferAsCharArray(), theUser.userBuffer.GetBufferLength(), 0);
			if (res == SOCKET_ERROR)
			{
				printf("Send failed with error: %ld\n", res);
			}
		}
	}
}

void buildMessage(userInfo& theUser,std::string& message)
{
	theUser.userBuffer = Buffer(); 
	theUser.userBuffer.WriteInt32BE(message.size());
	theUser.userBuffer.WriteStringBE(message);
}

void joinRoom(userInfo joinUser, char &roomName)
{
	for (std::map<char, std::vector<userInfo*>>::iterator it = roomMap.begin(); it != roomMap.end(); ++it)
	{
		if (roomName == it->first)
		{
			roomMap[roomName].push_back(&joinUser);
		}
	}

	userInfo tempInfo;

	for (int i = 0; i < master.fd_count; i++)
	{
		SOCKET outSock = master.fd_array[i];
		std::string message = "A New User has joined the room: ";
		message += roomName;
		tempInfo = getClient(outSock);

		buildMessage(tempInfo,message);

		//send all other clients in that room a message that a user joined
		if (outSock != ListenSocket && outSock != joinUser.userSocket)
		{
			int res = send(outSock, tempInfo.userBuffer.getBufferAsCharArray(), tempInfo.userBuffer.GetBufferLength(), 0);
			if (res == SOCKET_ERROR)
			{
				printf("Send failed with error: %ld\n", res);
			}
		}

		
	}

	std::string joinConfirmation = "You successfully joined room: ";
	joinConfirmation += roomName;
	buildMessage(tempInfo, joinConfirmation);

	//send the client a message back about joining the room
	int res = send(joinUser.userSocket, tempInfo.userBuffer.getBufferAsCharArray(), tempInfo.userBuffer.GetBufferLength(), 0);
	if (res == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", res);
	}
}

void leaveRoom(userInfo leaveUserInfo, char &roomName)
{
	for (std::map<char, std::vector<userInfo*>>::iterator it = roomMap.begin(); it != roomMap.end(); ++it)
	{
		if (roomName == it->first)
		{
			for (std::vector<userInfo*>::iterator iter = it->second.begin(); iter != it->second.end(); ++iter)
			{
				//TODO: if it is the same userInfo (this might not work)
				if (*iter == &leaveUserInfo)
				{
					//DELETES THE USER FROM THE ROOM, AS LONG AS THEY ARE IN ANOTHER ROOM THE DATA IS SAVED. (pointer to user in lobby)
					it->second.erase(iter);
				}
			}
		}
	}

	for (int i = 0; i < master.fd_count; i++)
	{
		g_theBuffer = new Buffer();
		SOCKET outSock = master.fd_array[i];
		std::string message = "A User has Left the room : " + roomName;
		if (outSock != ListenSocket && outSock != leaveUserInfo.userSocket)
		{
			send(outSock, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength(), 0);
		}
	}
}