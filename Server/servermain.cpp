#define UNICODE
#define WIN_32_CHAT_APP_SERVER

//#include <Windows.h>   freaks out if i include this
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "Buffer.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <map>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5000"	//was 8899
#define DEFAULT_AUTHENTICATION_PORT "6000"	//was 8899
#define DEFAULT_BUFFER_LENGTH 512
//socket info structure to store all the individual socket information

//User sockets and buffer struct
struct userInfo
{
	SOCKET userSocket;
	Buffer userBuffer;
	std::vector<int> requests;
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
void registerUser(SOCKET connectSock, std::string userEmail, std::string userPlainTextPassword, int& requestID);
void authenticateUser(SOCKET connectSock, std::string userEmail, std::string userPlainTextPassword,int& requestID);
std::vector<std::string> readPacket(userInfo& theUser,int packetlength);
void buildMessage(userInfo& theUser,std::string& message);
userInfo getClient(SOCKET& theSock);
void sendServerMessage(SOCKET* sendingUser, std::string message);
int receiveAuthMessage(SOCKET& sock, userInfo& theinfo);
int parseAuthMessage(userInfo& theinfo);
userInfo& findUserByRequestID(int requestId);

int g_IDCounter = 0;

int main()
{
	//seed the rand (for random request id)
	srand(time(0));

	SOCKET AcceptSocket;
	fd_set readSet;
	fd_set writeSet;
	FD_ZERO(&readSet);
	WSADATA wsaData;
	struct addrinfo* result = 0;
	struct addrinfo* ptr = NULL;
	struct addrinfo addressInfo;
	struct addrinfo hints;
	int iResult = 0;
	int totalSocketsInSet = 0;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SendBytes;
	//for authentication server
	SOCKET ConnectSocket = INVALID_SOCKET;

	std::cout << "Chat Server" << std::endl;

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

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	//get the address info for the authentication server
	iResult = getaddrinfo(NULL, DEFAULT_AUTHENTICATION_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//connect to the authentication server  
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket() failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
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

			if (sock == ConnectSocket) //if its the authentication server
			{
				userInfo tempInfo;
				tempInfo.userSocket = ConnectSocket;
				userInfo userWhoSentRequest;
				//TODO::
				//handle the messages from the authentication server.
				int requestId = receiveAuthMessage(sock, tempInfo);

				if(requestId == -1)//no request id somthing went wrong
				{
					std::cout << "Something went wrong with the request id\n (From if (sock == ConnectSocket) //if its the authentication server)" << std::endl;
				}
				else {
					userWhoSentRequest = findUserByRequestID(requestId);
				
					if (userWhoSentRequest.requests.size() > 0)
					{
						//send the message in the buffer to the user;
						//get the user
						int res =send(userWhoSentRequest.userSocket, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength(), 0);

						if (res == SOCKET_ERROR)
						{
							printf("Send failed at line:226 Chat Server with error: %ld\n", res);
						}
					}
				}

			}			
			else if (sock == ListenSocket)// Is it an inbound connection?
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
					int requestId = rand() % 1000;

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
						else if (results[0] == "REGISTER" || results[0] == "register")
						{
							registerUser(ConnectSocket, results[1], results[2], requestId);
						}
						else if (results[0] == "AUTHENTICATE" || results[0] == "authenticate")
						{
							authenticateUser(ConnectSocket, results[1], results[2], requestId);
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
	std::string email = "";
	std::string password = "";
	std::string command = "";
	int messageId = 0;
	int messageLength = 0;
	int commandLength = 0;
	int emailLength = 0;
	int passwordLength = 0;
	std::vector<std::string> receviedMessages;

	//if the message is specific
	if (packetLength > 3)
	{
		//read the packet id doesnt matter what it is becaus ethis checks what it is
		messageId = theUser.userBuffer.ReadInt32BE();

		//get the command length
		commandLength = theUser.userBuffer.ReadInt32BE();
		//read the command 
		command = parseMessage(commandLength, theUser.userBuffer);

		message = "";
		if (messageId <= 3)
		{
			//get message length
			messageLength = theUser.userBuffer.ReadInt32BE();
			//get message
			message = parseMessage(messageLength, theUser.userBuffer);
			//push back the messages
			receviedMessages.push_back(command);
			receviedMessages.push_back(message);
		}
		else //(register and auth)
		{
			//randomly generate a request id
			//theUser.requests.push_back(requestId);
			//get message length
			emailLength = theUser.userBuffer.ReadInt32BE();
			//get email
			email = parseMessage(emailLength, theUser.userBuffer);
			//get the password length
			passwordLength = theUser.userBuffer.ReadInt32BE();
			//get password
			password = parseMessage(passwordLength, theUser.userBuffer);
			//push back the messages
			receviedMessages.push_back(command);
			receviedMessages.push_back(email);
			receviedMessages.push_back(password);
		}
		
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
	userInfo* valueToErase;
	std::vector<userInfo*> roomUsers;

	//Go Through the rooms
	for (std::map<char, std::vector<userInfo*>>::iterator it = roomMap.begin(); it != roomMap.end(); ++it)
	{
		//Check if the the current room is the room the user specified to leave
		if (roomName == it->first)
		{
			//Pull the vector out of the map
			roomUsers = roomMap[roomName];

			//go through the users in the room
			for (int i = 0; i < roomUsers.size(); i++)
			{
				//check if the requester to leave is in the room
				if (&leaveUserInfo == roomUsers.at(i))
				{
					//remove the user from the room.
					valueToErase = roomMap[roomName].at(i);
					roomMap[roomName].erase(std::remove(roomMap[roomName].begin(), roomMap[roomName].end(), valueToErase), roomMap[roomName].end());
				}
			}//for
		}//if roomName == it->first
	}//for

	//Send the leave room status update to all of the users in the room that was left
	for (int i = 0; i < master.fd_count; i++)
	{
		bool userInRoom = false;
		g_theBuffer = new Buffer();
		SOCKET outSock = master.fd_array[i];
		std::string message = "A User has Left the room : " + roomName;

		for (int i = 0; i < roomUsers.size(); i++)
		{
			if (outSock == roomUsers[i]->userSocket)
			{
				userInRoom = true;
			}
		}

		if (outSock != ListenSocket && userInRoom && outSock != leaveUserInfo.userSocket)
		{
			send(outSock, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength(), 0);
		}
	}
}

#include "AccountAuthentication.pb.h"

void registerUser(SOCKET connectSock, std::string userEmail, std::string userPlainTextPassword,int& requestId)
{
	AccountAuthentication::AuthenticateAccount userAccount;
	g_theBuffer->getBuffer();
	
	//set the messageID
	g_theBuffer->WriteInt32BE(4);

	//Add the user credentials to the userAccount Object
	userAccount.set_requestid(requestId);
	userAccount.set_email(userEmail);
	userAccount.set_plaintextpassword(userPlainTextPassword);

	//Serialize the userAccount to a string
	std::string serializedString = "";
	serializedString = userAccount.SerializeAsString();

	//write the string length
	g_theBuffer->WriteInt32BE(serializedString.size());
	//Add the serialized string to the globel buffer
	g_theBuffer->WriteStringBE(serializedString);

	//Send the populated buffer to the auth server
	send(connectSock, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength(), 0);
}

void authenticateUser(SOCKET connectSock, std::string userEmail, std::string userPlainTextPassword,int& requestId)
{
	AccountAuthentication::AuthenticateAccount userAccount;
	g_theBuffer->getBuffer();

	//set the messageID
	g_theBuffer->WriteInt32BE(5);

	//Add the user credentials to the userAccount Object
	userAccount.set_requestid(requestId);
	userAccount.set_email(userEmail);
	userAccount.set_plaintextpassword(userPlainTextPassword);

	//Serialize the userAccount to a string
	std::string serializedString;
	userAccount.SerializeToString(&serializedString);

	//write the string length
	g_theBuffer->WriteInt32BE(serializedString.size());
	//Add the serialized string to the globel buffer
	g_theBuffer->WriteStringBE(serializedString);

	//Send the populated buffer to the auth server
	send(connectSock, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength(), 0);
}



int receiveAuthMessage(SOCKET& sock, userInfo& theinfo) {
	int bytesIn;
	bytesIn = recv(sock, theinfo.userBuffer.getBufferAsCharArray(), theinfo.userBuffer.GetBufferLength(), 0);
	int requestId = 0;
	if (bytesIn < 0)
	{
		// Drop the client
		closesocket(sock);
		FD_CLR(sock, &master);
	}
	else
	{
		if (bytesIn >= 4)
		{
			requestId = parseAuthMessage(theinfo);

		}
		else
		{
			//cant read anything 
			return requestId;
		}
	}

	return requestId;
}

int parseAuthMessage(userInfo& theinfo) {
	
	int messageId = -1;
	int messageLength = 0;
	std::string message;
	int requestId = -1;
	
	//message id
	messageId = theinfo.userBuffer.ReadInt32BE();
	//message length
	messageLength = theinfo.userBuffer.ReadInt32BE();
	//message
	message = theinfo.userBuffer.ReadStringBE(messageLength);

	if (messageId == 11)
	{
		//add success
		std::string tempStr = "Account Successfully Registered!";
		//populate the create 
		AccountAuthentication::CreateAccount create;
		create.ParseFromString(message);
		//get the request id
		requestId = create.requestid();

		//write the message id to the buffer
		g_theBuffer->WriteInt32BE(messageId);
		//packet length
		g_theBuffer->WriteInt32BE(tempStr.size());
		//message 
		g_theBuffer->WriteStringBE(tempStr);

		//return the request id
		return requestId;
	}
	else if(messageId == 12)
	{
		//add fail
		std::string tempStr = "Failed to register account: ";
		//populate the create 
		AccountAuthentication::CreateAccountFailure createFail;
		createFail.ParseFromString(message);
		//get the request id
		requestId = createFail.requestid();
		//add the reason to the end of the message
		tempStr += createFail.reason();
		//write the message id to the buffer
		g_theBuffer->WriteInt32BE(messageId);
		//packet length
		g_theBuffer->WriteInt32BE(tempStr.size());
		//message 
		g_theBuffer->WriteStringBE(tempStr);
		//return the request id
		return requestId;
	}
	else if (messageId == 13)
	{
		//auth success
		std::string tempStr = "Successfully authenticated the account!";
		//populate the create 
		AccountAuthentication::AuthenticateAccount authenticate;
		authenticate.ParseFromString(message);
		//get the request id
		requestId = authenticate.requestid();
		//write the message id to the buffer
		g_theBuffer->WriteInt32BE(messageId);
		//packet length
		g_theBuffer->WriteInt32BE(tempStr.size());
		//message 
		g_theBuffer->WriteStringBE(tempStr);
		//return the request id
		return requestId;
	}
	else if (messageId == 14)
	{
		//auth fail
		std::string tempStr = "Failed to authenticate account: ";
		//populate the create 
		AccountAuthentication::AuthenticateAccountFailure authenticateFail;
		authenticateFail.ParseFromString(message);
		//get the request id
		requestId = authenticateFail.requestid();
		//add the reason to the end of the message
		tempStr += authenticateFail.reason();
		//write the message id to the buffer
		g_theBuffer->WriteInt32BE(messageId);
		//packet length
		g_theBuffer->WriteInt32BE(tempStr.size());
		//message 
		g_theBuffer->WriteStringBE(tempStr);
		//return the request id
		return requestId;
	}
	else
		std::cout << "Invalid authserver message id" << std::endl;


	return -1;
}

userInfo& findUserByRequestID(int requestId) {

	//search the users in the userinserver vector
	for (int i = 0; i < usersInServer.size(); i++)
	{
		if (usersInServer[i].requests.size() > 0)
		{
			for (int request = 0; request < usersInServer[i].requests.size(); request++)
			{
				if (usersInServer[i].requests[request] == requestId)
				{
					return usersInServer[i];
				}
			}
		}
	}
	return userInfo();
}
