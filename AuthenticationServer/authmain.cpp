//Authentication server for the chat server to connect to.
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>

#include "Buffer.h"
#include "SQL_Wrapper.h"
#include "AccountAuthentication.pb.h"

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "6000"
#define DEFAULT_CHAT_SERVER_PORT "5000"
//User sockets and buffer struct
struct userInfo
{
	SOCKET userSocket;
	Buffer userBuffer;
} newUser;

fd_set master;
SOCKET ListenSocket;
SOCKET ConnectSocket;
Buffer* g_theBuffer = new Buffer();
userInfo g_chatServerInfo;

//functions
void buildMessage(userInfo& theUser, std::string& message, int messageType);
void sendAuthenticationServerMessage(userInfo& sendingUser, std::string message, int messageType);
void readPacket(userInfo& theUser, int packetLength);
std::string parseMessage(int messageLength, Buffer& userBuffer);
void connectToChatServer();
SQL_Wrapper* pTheSQLWrapper;

int main() {
	//singleton SQL_Wrapper
	//pTheSQLWrapper = pTheSQLWrapper->getInstance();
	pTheSQLWrapper = new SQL_Wrapper();
	//connect to the db.
	pTheSQLWrapper->connectToDB();
	g_chatServerInfo.userSocket = NULL;

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
	freeaddrinfo(result);

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

		if (g_chatServerInfo.userSocket == NULL)
		{
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
					connectToChatServer();
				}//end if
			}//end for 
		}
		else {
			// Receive message
			int bytesIn;

			bytesIn = recv(g_chatServerInfo.userSocket, g_chatServerInfo.userBuffer.getBufferAsCharArray(), g_chatServerInfo.userBuffer.GetBufferLength(), 0);
			if (bytesIn > 0)
			{
				readPacket(g_chatServerInfo, bytesIn);
			}
			else if (bytesIn == -1) {//print error message

			}
			else if (bytesIn == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {//print error message
				printf("receive failed with error: %i", WSAGetLastError());
			}

		}
		//should only be one.

	}//end else
	closesocket(ListenSocket);
	closesocket(ConnectSocket);
	WSACleanup();
}

//Name:			readPacket
//Purpose:		Reads the packet sent from the chat servers
void readPacket(userInfo& theUser, int packetLength)
{
	std::string message = "";
	std::string command = "";
	int messageId = 0;
	int messageLength = 0;
	std::vector<std::string> receviedMessages;
	std::pair<int, int> addAccountInfo(-2, -1);
	std::pair<std::pair<int, int>, std::string> authAccountInfo(std::pair<int, int>(-2, -1), "");

	//if the message is specific
	if (packetLength > 4)
	{
		message = "";
		//read the packet id and the command length
		messageId = theUser.userBuffer.ReadInt32BE();

		//if the id is 4(Register) or 5(Authenticate)
		if (messageId == 4)//Register
		{
			//get the message length
			messageLength = theUser.userBuffer.ReadInt32BE();
			//parse the message
			message = parseMessage(messageLength, theUser.userBuffer);

			//create the CreateAccount and parse the message
			AccountAuthentication::CreateAccount account;
			account.ParseFromString(message);

			//get the add account info from the db
			addAccountInfo = pTheSQLWrapper->addAccount(account.email(), account.plaintextpassword());

			//returns -1 for server error 
			//returns 1 for exists

			if (addAccountInfo.first == 0)//success
			{
				//create the AuthenticateAccountSuccess object
				AccountAuthentication::CreateAccountSuccess success;
				//populate
				success.set_requestid(account.requestid());
				success.set_userid(addAccountInfo.second);
				//serialize the object
				std::string successSerialize;
				successSerialize = success.SerializeAsString();
				//send the success
				sendAuthenticationServerMessage(g_chatServerInfo, successSerialize, 11);
			}
			else
			{
				//create the AuthenticateAccountFailure object and populate it
				AccountAuthentication::CreateAccountFailure failure;
				failure.set_requestid(account.requestid());

				if (addAccountInfo.first == 1)
				{
					//already exists
					failure.set_reason(failure.ACCOUNT_ALREADY_EXISTS);
				}
				else //server error
					failure.set_reason(failure.INTERNAL_SERVER_ERROR);

				//serialize and send
				std::string addFailureSerialize;
				addFailureSerialize = failure.SerializeAsString();
				sendAuthenticationServerMessage(g_chatServerInfo, addFailureSerialize, 12);

			}
		}
		else if (messageId == 5) //Authenticate
		{
			//get the message length
			messageLength = theUser.userBuffer.ReadInt32BE();
			//get the message
			message = parseMessage(messageLength, theUser.userBuffer);
			//deserialize the message
			AccountAuthentication::AuthenticateAccount account;
			account.ParseFromString(message);

			//try to authenticate the account
			authAccountInfo = pTheSQLWrapper->authenticateAccount(account.email(), account.plaintextpassword());

			//authentication success
			if (authAccountInfo.first.first == 0)
			{
				//success
				AccountAuthentication::AuthenticateAccountSuccess authSuccess;
				authSuccess.set_requestid(account.requestid());
				authSuccess.set_userid(authAccountInfo.first.first);
				authSuccess.set_creationdate(authAccountInfo.second);

				std::string authSerial = authSuccess.SerializeAsString();
				//send the message
				sendAuthenticationServerMessage(g_chatServerInfo, authSerial, 13);
			}
			else // authentication failure
			{
				//create and populate failure message
				AccountAuthentication::AuthenticateAccountFailure authFailure;
				authFailure.set_requestid(account.requestid());

				if (authAccountInfo.first.first == 1) {
					//invalid credentials
					authFailure.set_reason(authFailure.INVALID_CREDENTIALS);
				}
				else//server error
					authFailure.set_reason(authFailure.INTERNAL_SERVER_ERROR);

				//create a string for serialization
				std::string authFailureMessage;
				authFailureMessage = authFailure.SerializeAsString();
				//send the failure
				sendAuthenticationServerMessage(g_chatServerInfo, authFailureMessage, 14);
			}

		}

		//get the command length
	}
}

//Name:			sendAuthenticationServerMessage
//Purpose:		Sends and authentication message to the chat server.
//Return:		void
void sendAuthenticationServerMessage(userInfo& sendingUser, std::string message, int messageType) {
	//server message happens when client joins the server?
	//userInfo theUser = getClient(*sendingUser);
	buildMessage(sendingUser, message, messageType);

	int res = send(ConnectSocket, sendingUser.userBuffer.getBufferAsCharArray(), sendingUser.userBuffer.GetBufferLength(), 0);
	if (res == SOCKET_ERROR)
	{
		printf("Send failed with error: %ld\n", res);
	}
}


//Name:			buildMessage
//Purpose:		Builds the message to be sent.
//Return:		void
void buildMessage(userInfo& theUser, std::string& message, int messageType)
{
	theUser.userBuffer = Buffer();
	//write the id
	theUser.userBuffer.WriteInt32BE(messageType);
	//write the packet size
	theUser.userBuffer.WriteInt32BE(message.size());
	//write the message
	theUser.userBuffer.WriteStringBE(message);
}

//Name:			parseMessage
//Purpose:		Parse the message from the chat server.
//Return:		std::string
std::string parseMessage(int messageLength, Buffer& userBuffer) {
	std::string tempMessage = "";
	tempMessage += userBuffer.ReadStringBE(messageLength);
	return tempMessage;
}

//Name:			connectToChatServer
//Purpose:		After the chat server has connected to us connect to it for message sending purposes.
//Return:		void
void connectToChatServer()
{
	int iResult = 0;
	struct addrinfo* result = 0;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//get the address info for the authentication server
	iResult = getaddrinfo(NULL, DEFAULT_CHAT_SERVER_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return;
	}

	//connect to the authentication server  
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket() failed with error: %d\n", iResult);
			WSACleanup();
			return;
		}

		//connect to the socket
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);
	//Check if the Connected socket is valid
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server\n");
		WSACleanup();
		return;
	}
}