#include "httpHandler.h"
using namespace std;


void updateSocketAfterRequest(SocketState* sockets, int index, METHOD methodType, int size)
{
    // Set the socket METHOD and change Status to SEND
    sockets[index].send = SEND;
    sockets[index].method = methodType;

    // Remove processed part of the buffer
    if (size > 0)
    {
        sockets[index].buffer = sockets[index].buffer.substr(size);
        sockets[index].len -= size;
    }
}

void prepareOptionsResponse(string& response)
{
	response = "HTTP/1.1 204 No Content\r\n";
	response += "Server: HTTP Web Server\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
	response += "Content-Length: 0\r\n";
	response += "\r\n";  // End of headers
}

void prepareGetResponse(string& response, string& socketBuffer, bool headReq)
{
	// Extract the requested path from the request data
	size_t startPos = socketBuffer.find("GET ") + 3;
	size_t endPos = socketBuffer.find(" ", startPos);
	std::string requestedPath = socketBuffer.substr(startPos, endPos - startPos);

	// Default to index.html if no specific file is requested
	if (requestedPath == "/") {
		requestedPath = "index.html";
	}

	// Determine the language from the query string (if any)
	std::string lang = "en"; // default to English
	size_t langPos = requestedPath.find("lang=");
	if (langPos != std::string::npos) {
		lang = requestedPath.substr(langPos + 5, 2);
		requestedPath = requestedPath.substr(0, langPos - 1); // remove query string
	}

	// Construct the full file path
	std::string filePath = "C://temp";
	if (lang == "he") {
		filePath += "/he/" + requestedPath;
	}
	else if (lang == "fr") {
		filePath += "/fr/" + requestedPath;
	}
	else {
		filePath += "/en/" + requestedPath;
	}


	// Open the file and read its content
	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	if (!file) {
		// File not found, respond with 404
		response = "HTTP/1.1 404 Not Found\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: 0\r\n";
		response += "\r\n";
		return;
	}

	std::ostringstream fileContent;
	fileContent << file.rdbuf();
	std::string content = fileContent.str();
	file.close();

	// Construct the response
	response = "HTTP/1.1 200 OK\r\n";
	response += "Server: HTTP Web Server\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
	response += "\r\n"; // End of headers
	if (!headReq)
		response += content;
}

void prepareHeadResponse(string& response, string& socketBuffer)
{
    prepareGetResponse(response, socketBuffer, true);
}
void preparePostResponse(string& response, string& socketBuffer)
{
	unordered_map<string, string> headers;
	fileType dataType = TEXT_PLAIN;
	string status;
	ofstream file;

	// Parse headers and determine content type
	size_t headerEnd = socketBuffer.find("\r\n\r\n");
	if (headerEnd != string::npos) {
		string headerSection = socketBuffer.substr(0, headerEnd);
		stringstream ss(headerSection);
		string line;

		// Read headers into unordered_map
		while (getline(ss, line, '\n')) {
			size_t pos = line.find(": ");
			if (pos != string::npos) {
				headers[line.substr(0, pos)] = line.substr(pos + 2);
			}
		}

		// Determine content type from headers
		auto contentTypeIt = headers.find("Content-Type");
		if (contentTypeIt != headers.end()) {
			string contentType = contentTypeIt->second;
			if (contentType.find("application/octet-stream") != string::npos) {
				dataType = BINARY;
			}
		}
	}

	// Extract file content from body
	size_t bodyStart = headerEnd + 4;
	string fileContent = socketBuffer.substr(bodyStart);

	std::cout << "Content length: " << fileContent.size() << " bytes" << std::endl;

	// Determine file path based on language and requested resource
	string resourcePath = "/uploaded_file.txt"; // Example resource path
	string filePath = "C:/temp/";
	filePath += resourcePath;

	// Open file for writing
	if (dataType == BINARY)
		file.open(filePath, ios::binary);
	else
		file.open(filePath);

	if (!file.is_open()) {
		status = "500 Internal Server Error";
		response = "HTTP/1.1 " + status + "\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: 0\r\n";
		response += "\r\n";
		return;
	}

	// Write file content
	file << fileContent;
	file.close();

	// Prepare successful response
	status = "201 Created";
	response = "HTTP/1.1 " + status + "\r\n";
	response += "Server: HTTP Web Server\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: 0\r\n";
	response += "\r\n";

	// Print file contents to console
	ifstream readFile(filePath, ios::binary);
	if (readFile)
	{
		ostringstream fileContents;
		fileContents << readFile.rdbuf();
		string content = fileContents.str();
		cout << "File Contents:\n" << content << endl;
	}
	else
	{
		cout << "Failed to open file to read contents." << endl;
	}
}
void preparePutResponse(string& response, string& socketBuffer)
{
	size_t startPath = socketBuffer.find("PUT ") + 3; // Adjusted to skip "PUT "
	size_t endPath = socketBuffer.find(" HTTP/1.1", startPath);
	std::string requestedPath = socketBuffer.substr(startPath, endPath - startPath);

	// Construct full file path based on ROOT_DIRECTORY and requestedPath
	std::string filePath = "C:/temp/" + requestedPath;

	// Extract content length from headers
	size_t contentLengthPos = socketBuffer.find("Content-Length: ") + 16;
	size_t contentLengthEndPos = socketBuffer.find("\r\n", contentLengthPos);
	std::string contentLengthStr = socketBuffer.substr(contentLengthPos, contentLengthEndPos - contentLengthPos);
	int contentLength = std::stoi(contentLengthStr);

	// Extract file content from body
	size_t bodyStart = socketBuffer.find("\r\n\r\n") + 4;
	std::string fileContent = socketBuffer.substr(bodyStart, contentLength);

	// Open the file for writing
	std::ofstream file(filePath, std::ios::out | std::ios::binary);
	if (!file.is_open()) {
		// Error opening file for writing
		response = "HTTP/1.1 500 Internal Server Error\r\n";
		response += "Server: HTTP Web Server\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: 0\r\n";
		response += "\r\n"; // End of headers

		// Print a message to the console for logging
		std::cout << "HTTP Server: PUT request failed. Could not open file for writing: " << filePath << std::endl;
		return;
	}

	// Write file content
	file << fileContent;
	file.close();

	// Prepare successful response
	response = "HTTP/1.1 200 OK\r\n";
	response += "Server: HTTP Web Server\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: 0\r\n";
	response += "\r\n"; // End of headers

	// Print a message to the console for logging
	std::cout << "HTTP Server: PUT request handled successfully. File created/modified: " << filePath << std::endl;

}
void prepareDeleteResponse(string& response, string& socketBuffer)
{
	size_t startPath = socketBuffer.find("DELETE ") + 3;
	size_t endPath = socketBuffer.find(" HTTP/1.1", startPath);
	string requestedPath = socketBuffer.substr(startPath, endPath - startPath);

	// Construct full file path based on ROOT_DIRECTORY
	string filePath = "C:/temp/" + requestedPath;

	// Attempt to delete the file
	if (fs::exists(filePath)) 
	{
		if (fs::remove(filePath)) 
		{
			// Deletion successful
			response = "HTTP/1.1 204 No Content\r\n";
			response += "Server: HTTP Web Server\r\n";
			response += "Content-Length: 0\r\n";
			response += "\r\n"; // End of headers

			// Print a message to the console for logging
			cout << "HTTP Server: DELETE request handled successfully. File deleted: " << filePath << endl;
		}
		else 
		{
			// Error occurred while deleting the file
			response = "HTTP/1.1 500 Internal Server Error\r\n";
			response += "Server: HTTP Web Server\r\n";
			response += "Content-Type: text/html\r\n";
			response += "Content-Length: 0\r\n";
			response += "\r\n"; // End of headers

			// Print a message to the console for logging
			cout << "HTTP Server: DELETE request failed. Failed to delete file: " << filePath << endl;
		}
	}
	else 
	{
		// File not found
		response = "HTTP/1.1 404 Not Found\r\n";
		response += "Server: HTTP Web Server\r\n";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: 0\r\n";
		response += "\r\n"; // End of headers

		// Print a message to the console for logging
		cout << "HTTP Server: DELETE request failed. File not found: " << filePath << endl;
	}

}
void prepareTraceResponse(string& response, string& socketBuffer)
{
	// Concatenate "TRACE" with the received request
	string echoString = "TRACE " + socketBuffer;

	// Make the header
	string status("200 OK");
	string contentType("message/http");
	response = "HTTP/1.1 " + status + "\r\n";
	response += "Server: HTTP Web Server\r\n";
	response += "Content-Type: " + contentType + "\r\n";

	// Calculate and append Content-Length
	response += "Content-Length: " + to_string(echoString.length()) + "\r\n";

	// Add empty line to mark end of headers
	response += "\r\n";

	// Append the echoed request to the response body
	response += echoString;
}


void prepareNotAllowedResponse(string& response)
{
    response = "HTTP/1.1 405 Method Not Allowed\r\n";
    response += "Server: HTTP Web Server\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: Closed\r\n";
    response += "\r\n";  // End of headers
}
