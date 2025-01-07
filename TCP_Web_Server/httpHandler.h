#ifndef HTTP_HANDLER__H
#define HTTP_HANDLER__H

#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <filesystem>

using namespace std; 
namespace fs = filesystem;

const int HTTP_PORT = 80;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SOCKET_BUFFER_SIZE = 4096;
const int MAX_SEC_FOR_SOCKET = 120;  // time in seconds
const timeval MAX_SEC_FOR_SELECT = { MAX_SEC_FOR_SOCKET, 0 };
const int MAX_SOCKETS = 100;  // Example maximum number of sockets


enum METHOD 
{
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE_M,
    TRACE,
    NOT_ALLOWED
};

enum fileType {
	TEXT_PLAIN,
	BINARY
};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType; // Sending sub-type
    METHOD method;
    string buffer;       // Buffer for the socket
    size_t len;          // Length of the buffer
    time_t lastTimeUsed; // Last time the socket was used
};


void updateSocketAfterRequest(SocketState* sockets, int index, METHOD methodType, int size);


void prepareOptionsResponse(string& response);
void prepareGetResponse(string& response, string& socketBuffer, bool headReq);
void prepareHeadResponse(string& response,string& socketBuffer);
void preparePostResponse(string& response,string& socketBuffer);
void preparePutResponse(string& response,string& socketBuffer);
void prepareDeleteResponse(string& response,string& socketBuffer);
void prepareTraceResponse(string& response,string& socketBuffer);
void prepareNotAllowedResponse(string& response);

#endif 
