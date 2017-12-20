#include <string>
#include <iostream>
#include <conio.h>
#include <list>
#include "Buffer.h"
#include "Utils.h"

//#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_PORT "5000"
#define DEFAULT_BUFFER_LENGTH 512

class Header {
public:
	//[packet_length][message_id]
	int32_t packet_length;			//in bytes
	int32_t message_id;				//What user is trying to do
};

//global buffer 
Buffer* g_theBuffer;
Header* g_theHeader;

//Protocols method headers
std::string receiveMessage(Buffer& theBuffer);
void readInput(std::vector<std::string>& theStrings, std::string input);
void processCommands(std::vector<std::string>& theCommands);
std::vector<std::string> theCommands;
std::list<std::string> screenMessages;
void populateScreenData(std::string message);
void printScreen();
int requestId = 0;

//TO DO: Client side connection
int main(int argc, char** argv) {

	g_theBuffer = new Buffer();
	g_theHeader = new Header();
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;
	int iResult;
	ULONG iMode = 0;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		//return 1;
	}
	printf("Winsock Initialized\n");


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//get the address info 
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	//set up the socket
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

	unsigned long nonBlock = 1;
	//set socket to non-blocking for recieving messages
	if (ioctlsocket(ConnectSocket, FIONBIO, &nonBlock) == SOCKET_ERROR) {
		printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
		return 1;
	}

	//iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);
	//if (iResult != NO_ERROR)
	//	printf("ioctlsocket failed with error: %ld\n", iResult);

	//display commands before loop
	print_text("=======================================");
	print_text("		Commands:              ");
	print_text("=======================================");
	print_text("Join Room: JR (a-z)");
	print_text("Leave Room: LR (a-z)");
	print_text("Send Message: SM (followed by message)");
	print_text("Register Account: REGISTER (Email) (Password)");
	print_text("Login: AUTHENTICATE (Email) (Password)");
	print_text("Connected to Server");

	//string for user input
	std::string userInput;
	std::string command;
	std::string roomName;
	std::string email;
	std::string password;
	bool isMessagePopulated = false;
	int smFind = -1;
	int lrFind = -1;
	int jrFind = -1;
	int ruFind = -1;
	int auFind = -1;
	int bytesReceived = 0;

	while (true)
	{
		start_text();

		if (_kbhit()) {
			char c = _getch();
			if (c == '\r')
			{

				//no error checking for the room after (right now assume that its correct)
				if (userInput.length() > 3)
				{
					smFind = userInput.find("SM "); //Send Message
					jrFind = userInput.find("JR "); //Join Room
					lrFind = userInput.find("LR "); //Leave Room
					ruFind = userInput.find("REGISTER"); //Register Account
					auFind = userInput.find("AUTHENTICATE");	//Authenticate User

																//Get the commands for Join Room
					if (jrFind >= 0)
					{
						command = userInput.substr(0, 2);
						roomName = userInput.substr(3, 1);

						theCommands.push_back(command);
						theCommands.push_back(roomName);
						isMessagePopulated = true;
						userInput = "";
					}

					//Get the commands for Send Message
					if (smFind >= 0)
					{
						command = userInput.substr(0, 2);
						roomName = userInput.substr(3, userInput.size() - 3);

						theCommands.push_back(command);
						theCommands.push_back(roomName);
						isMessagePopulated = true;
						userInput = "";
					}

					//Get the commands for Leave Room
					if (lrFind >= 0)
					{
						command = userInput.substr(0, 2);
						roomName = userInput.substr(3, 1);

						theCommands.push_back(command);
						theCommands.push_back(roomName);
						isMessagePopulated = true;
						userInput = "";
					}

					//Get the commands for Register Account
					if (ruFind >= 0)
					{
						int registerPos = userInput.find("REGISTER");
						std::string comm = "";
						std::string emailString = "";
						std::string pass = "";

						//build the command 
						for (int i = 0; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' ')
							{
								comm += userInput[i];
							}
							else
							{
								break;
							}
						}

						//build the email
						for (int i = comm.size() + 1; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' '){
								emailString += userInput[i];
							}
							else{
								break;
							}
						}

						//build password
						for (int i = (emailString.size() + comm.size()) + 2; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' '){
								pass += userInput[i];
							}
							else{
								break;
							}
						}
						theCommands.push_back(comm);
						theCommands.push_back(emailString);
						theCommands.push_back(pass);
						isMessagePopulated = true;
						userInput = "";
					}

					//Get the commands for Authenticate User
					if (auFind >= 0)
					{
						std::string comm = "";
						std::string emailString = "";
						std::string pass = "";

						//build the command 
						for (int i = 0; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' '){
								comm += userInput[i];
							}
							else{
								break;
							}
						}

						//build the email
						for (int i = comm.size() + 1; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' '){
								emailString += userInput[i];
							}
							else{
								break;
							}
						}

						//build password
						for (int i = (emailString.size() + comm.size()) + 2; i < userInput.size(); i++)
						{
							//start at the beginning of the string 
							if (userInput[i] != ' '){
								pass += userInput[i];
							}
							else{
								break;
							}
						}
						theCommands.push_back(comm);
						theCommands.push_back(emailString);
						theCommands.push_back(pass);
						isMessagePopulated = true;
						userInput = "";
					}
				}
				populateScreenData(command + " " + roomName);
			}
			if (c == '\b')
			{
				userInput = userInput.substr(0, userInput.length()-1);
			}
			else
			{
				userInput += c;
			}
		}

		if (isMessagePopulated)
		{
			isMessagePopulated = false;
			//read the user input
			//readInput(theCommands, userInput);
			//process the commands from input 
			processCommands(theCommands);

			//send command
			int sendResult = send(ConnectSocket, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength() + 1, 0);
			//check for error
			if (sendResult == SOCKET_ERROR)
			{
				print_text("Send failed with error: %s", sendResult);
			}

			//clear the buffer for the next set of info
			g_theBuffer = new Buffer();
		}

		bytesReceived = recv(ConnectSocket, g_theBuffer->getBufferAsCharArray(), g_theBuffer->GetBufferLength() + 1, 0);
		if (bytesReceived > 0)
		{
			//do the conversion
			std::string receivedPhrase = receiveMessage(*g_theBuffer);
			if (receivedPhrase.size() > 0)
				populateScreenData(receivedPhrase);

			g_theBuffer = new Buffer();
		}
		else if (bytesReceived == -1) {//print error message

		}
		else if (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {//print error message
			print_text("receive failed with error: %s", WSAGetLastError());
		}

		//clear the commands so they don't build up
		theCommands.clear();
		
		printScreen();

		std::string textOnScreen = "-> " + userInput;

		print_text("%-75s", textOnScreen.c_str());

		end_text();
	}

	//cleanup
	closesocket(ConnectSocket);
	freeaddrinfo(ptr);
	WSACleanup();
}

//Name:			receiveMessage
//Purpose:		takes in the message from the server and processes it
std::string receiveMessage(Buffer& theBuffer) {
	Header tempHeader;
	tempHeader.packet_length = theBuffer.ReadInt32BE();
	std::string message = theBuffer.ReadStringBE(tempHeader.packet_length);
	return message;
}

void populateScreenData(std::string message) {
	screenMessages.push_back(message);
	if (screenMessages.size() >= 15) {
		screenMessages.pop_front();
	}
}

void printScreen()
{
	std::list<std::string>::const_iterator iterator;
	std::string temp = "";
	for (iterator = screenMessages.begin(); iterator != screenMessages.end(); ++iterator) {
		temp = *iterator;
		print_text("->%s", temp.c_str());
	}
}


//Name:			processCommands
//Purpose:		processes the user commands and populates the buffer according to message type
void processCommands(std::vector<std::string>& theCommands) {
	if (theCommands.size() > 0)
	{
		//LR = 3
		//JR = 2
		//SM = 1
		//REGISTER = 4
		//AUTHENTICATE = 5

		//if the command is to leave room
		if (theCommands[0] == "LR" || theCommands[0] == "lr")
		{

			//Create the header with massageType
			g_theHeader = new Header();
			g_theHeader->message_id = 3;
			//write header
			g_theBuffer->WriteInt32BE(g_theHeader->message_id);
			g_theHeader->packet_length = theCommands[0].size();
			g_theBuffer->WriteInt32BE(g_theHeader->packet_length);
			g_theBuffer->WriteStringBE(theCommands[0]);
			//write second data
			g_theBuffer->WriteInt32BE(theCommands[1].size());
			g_theBuffer->WriteStringBE(theCommands[1]);
		}


		//if the command is to join room
		if (theCommands[0] == "JR" || theCommands[0] == "jr")
		{

			g_theHeader = new Header();
			g_theHeader->message_id = 2;
			//write message id
			g_theBuffer->WriteInt32BE(g_theHeader->message_id);
			//get packet length
			g_theHeader->packet_length = 2;
			//write packet length
			g_theBuffer->WriteInt32BE(g_theHeader->packet_length);
			//write the command
			g_theBuffer->WriteStringBE(theCommands[0]);
			//write the packet length
			g_theBuffer->WriteInt32BE(theCommands[1].size());
			//write the 
			g_theBuffer->WriteStringBE(theCommands[1]);
		}

		//if the command is to send message
		if (theCommands[0] == "SM" || theCommands[0] == "sm")
		{
			g_theHeader = new Header();
			g_theHeader->message_id = 1;
			g_theBuffer->WriteInt32BE(g_theHeader->message_id);
			g_theHeader->packet_length = theCommands[0].size();
			g_theBuffer->WriteInt32BE(g_theHeader->packet_length);
			g_theBuffer->WriteStringBE(theCommands[0]);
			g_theBuffer->WriteInt32BE(theCommands[1].size());
			g_theBuffer->WriteStringBE(theCommands[1]);
		}

		if (theCommands[0] == "REGISTER" || theCommands[0] == "register")
		{
			//Registration functionality
			g_theHeader = new Header();
			g_theHeader->message_id = 4; //messageid
			g_theBuffer->WriteInt32BE(g_theHeader->message_id);//write to buffer

			g_theHeader->packet_length = theCommands[0].size();
			g_theBuffer->WriteInt32BE(g_theHeader->packet_length);//write the packet length to the buffer
			g_theBuffer->WriteStringBE(theCommands[0]);//write command
			g_theBuffer->WriteInt32BE(theCommands[1].size());//write first length
			g_theBuffer->WriteStringBE(theCommands[1]);//write first
			g_theBuffer->WriteInt32BE(theCommands[2].size());//write second length
			g_theBuffer->WriteStringBE(theCommands[2]);//write second 
		}

		if (theCommands[0] == "AUTHENTICATE" || theCommands[0] == "authenticate")
		{
			//Authentication functionality
			g_theHeader = new Header();
			g_theHeader->message_id = 5;
			g_theBuffer->WriteInt32BE(g_theHeader->message_id);
			g_theHeader->packet_length = theCommands[0].size();
			g_theBuffer->WriteInt32BE(g_theHeader->packet_length);
			g_theBuffer->WriteStringBE(theCommands[0]);
			g_theBuffer->WriteInt32BE(theCommands[1].size());
			g_theBuffer->WriteStringBE(theCommands[1]);
			g_theBuffer->WriteInt32BE(theCommands[2].size());
			g_theBuffer->WriteStringBE(theCommands[2]);
		}
	}
}

//Name:			readInput
//Purpose:		parses the input from the user and separates the first and second strings
void readInput(std::vector<std::string>& theStrings, std::string input) {
	std::string tempString = "";
	for (int i = 0; i < input.size(); i++)
	{
		//if theres a space at the start
		if (input[i] == ' '&& i == 0)
		{
			continue;
		}

		//if theres a space after the first two letters and the temp string has letter already
		if (input[i] == ' '&& i == 3 && tempString.size() >0)
		{
			theStrings.push_back(tempString);
			continue;
		}

		//otherwise just add all characters and spaces
		tempString += input[i];
		if (tempString != "")
			theStrings.push_back(tempString);
	}
}