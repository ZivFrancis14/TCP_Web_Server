#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "httpHandler.h"

bool addSocket(SocketState sockets[], int& socketsCount, SOCKET id, int what);
void removeSocket(SocketState sockets[], int& socketsCount, int index);
void acceptConnection(SocketState sockets[], int& socketsCount, int index);
void receiveMessage(SocketState sockets[], int& socketsCount, int index);
void sendMessage(SocketState sockets[], int& socketsCount, int index);

void main()
{
	SocketState sockets[MAX_SOCKETS] = { 0 };  // Array of SocketState structs
	int socketsCount = 0;  // Number of active sockets


	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(HTTP_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(sockets,socketsCount,listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "HTTP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(sockets, socketsCount, i);
					break;

				case RECEIVE:
					receiveMessage(sockets, socketsCount, i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(sockets, socketsCount, i);
					break;
				}
			}
		}
		// if there are open sockets (non listener) well check if they reached the max time 
		if (socketsCount > 1)
		{
			int socketsLeft = socketsCount - 1;
			time_t currTime;
			time(&currTime);
			time_t timeToClose = currTime - MAX_SEC_FOR_SOCKET;
			for (int i = 1; i < MAX_SOCKETS && socketsLeft != 0; i++)
			{
				// if the id is valid and the last time used was before time to close, well close the connection
				if (sockets[i].id != 0 && sockets[i].lastTimeUsed <= timeToClose && sockets[i].lastTimeUsed != 0)
				{
					closesocket(sockets[i].id);
					removeSocket(sockets, socketsCount,i);
					socketsLeft -= 1;
				}
				// if the time is still valid and also the id is valid (different than zero)
				else if (sockets[i].id != 0)
					socketsLeft -= 1;
			}


		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SocketState sockets[], int& socketsCount, SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);

			//
			// Set the socket to be in non-blocking mode.
			//
			unsigned long flag = 1;
			if (ioctlsocket(id, FIONBIO, &flag) != 0)
			{
				cout << "HTTP Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
			}
		}
	}
	return (false);
}

void removeSocket(SocketState sockets[], int& socketsCount, int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].id = 0;
	sockets[index].buffer.clear();
	sockets[index].len = 0;
	sockets[index].lastTimeUsed = 0;
	socketsCount--;
}

void acceptConnection(SocketState sockets[], int& socketsCount, int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;


	if (addSocket(sockets, socketsCount, msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(SocketState sockets[], int& socketsCount, int index)
{
	SOCKET msgSocket = sockets[index].id;

	char buffer[SOCKET_BUFFER_SIZE];
	int bytesRecv = recv(msgSocket, buffer, SOCKET_BUFFER_SIZE - 1, 0);
	buffer[bytesRecv] = '\0'; // Null-terminate the received data
	sockets[index].buffer = buffer; // Append received data to the socket's buffer
	sockets[index].len = sockets[index].buffer.length(); // Update the buffer length

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(sockets, socketsCount, index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(sockets, socketsCount, index);
		return;
	}
	else
	{

		if (sockets[index].len > 0)
		{
			if (strncmp(sockets[index].buffer.c_str(), "OPTIONS", 7) == 0)
			{
				updateSocketAfterRequest(sockets,index, OPTIONS, 7);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "GET", 3) == 0)
			{
				updateSocketAfterRequest(sockets, index, GET, 3);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "HEAD", 4) == 0)
			{
				updateSocketAfterRequest(sockets, index, HEAD, 4);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "POST", 4) == 0)
			{
				updateSocketAfterRequest(sockets, index, POST, 4);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "PUT", 3) == 0)
			{
				updateSocketAfterRequest(sockets, index, PUT, 3);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "DELETE", 6) == 0)
			{
				updateSocketAfterRequest(sockets, index, DELETE_M, 6);
			}
			else if (strncmp(sockets[index].buffer.c_str(), "TRACE", 5) == 0)
			{
				updateSocketAfterRequest(sockets, index, TRACE, 5);
			}
			else
			{
				updateSocketAfterRequest(sockets, index, NOT_ALLOWED, 0);
			}
		}
		
	}
	// Clear the temporary buffer
	memset(buffer, 0, SOCKET_BUFFER_SIZE);
}

void sendMessage(SocketState sockets[], int& socketsCount, int index)
{
	bool closeConnection = false;

	size_t bytesSent = 0;

	string response;

	SOCKET msgSocket = sockets[index].id;

	METHOD methodType = sockets[index].method;

	time_t currectTime;

	switch (methodType)
	{
	case OPTIONS:
		prepareOptionsResponse(response);
		break;
	case GET:
		prepareGetResponse(response, sockets[index].buffer , false);
		break;
	case HEAD:
		prepareHeadResponse(response, sockets[index].buffer);
		break;
	case POST:
		preparePostResponse(response, sockets[index].buffer);
		break;
	case PUT:
		preparePutResponse(response, sockets[index].buffer);
		break;
	case DELETE_M:
		prepareDeleteResponse(response, sockets[index].buffer);
		break;
	case TRACE:
		prepareTraceResponse(response, sockets[index].buffer);
		break;
	case NOT_ALLOWED:
		prepareNotAllowedResponse(response);
		closeConnection = true;
		break;
	}


	bytesSent = send(msgSocket, response.c_str(), response.length(), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "HTTP Server: Sent: " << bytesSent << "\\" << response.length() << " bytes of \"" << response;

	time(&currectTime);
	sockets[index].lastTimeUsed = currectTime; // update the last time used.

	if (closeConnection)
	{
		closesocket(msgSocket);
		removeSocket(sockets,socketsCount,index);
	}
	else 
	{
		
	  // Update buffer and length if not all data sent
		if (bytesSent < response.length())
		{
		// If not all data was sent, update the buffer to contain the unsent data
		sockets[index].buffer = response.substr(bytesSent);
		}
		else
		{
		// Clear the buffer if all data was sent
		sockets[index].buffer.clear();
		sockets[index].send = IDLE;
		}	
	}
}
