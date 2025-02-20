#include "../../includes/http/response.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

Response::Response() : _version(""), _Status(""), _Headers(), _Body(""), _Resp(""), _fileSize(0)
{
	_ChunkedState = RESPONSE::CHUNKED_HEADERS;
}
Response &Response::WithHttpVersion(std::string version)
{
	(void)version;
	this->_version = "HTTP/1.1";
	return *this;
}
Response &Response::WithStatus(int status)
{
	this->_Status = std::string(NumberToString(status).c_str()) + " " + RESPONSE::toString(static_cast<RESPONSE::ResponseCode>(status));
	return *this;
}
Response &Response::setDefaultHeaders()
{
	_Headers["Server"] = "OneServe";
	_Headers["Date"] = get_http_date();
	return *this;
}
Response &Response::WithHeader(std::string key, std::string value)
{
	_Headers[key] = value;
	return *this;
}
Response &Response::WithBody(std::string body)
{
	_Body = body;
	(*this).WithHeader("content-length", NumberToString(body.size()));
	return *this;
}
void Response::Http200(int client_socket)
{
	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(200)
		.setDefaultHeaders()
		.Generate()
		.Send(client_socket);
}
void Response::BadRequest(int client_socket, HttpRequestData &req)
{
	std::ifstream DefFile;
	std::ifstream file(req._config_res["400"].c_str());
	std::string LastGoBody = "<!DOCTYPE html>\n<html>\n<head>\n<title>400 Bad Request</title>\n</head>\n<body>\n<h1>Bad Request</h1>\n<p>Your browser sent a request that this server could not understand.</p>\n</body>\n</html>\n";

	std::string body;

	if (!file.is_open())
		body = LastGoBody;
	else
		body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(400)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/html")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
void Response::NotFound(int client_socket, HttpRequestData &req)
{
	std::ifstream file(req._config_res["404"].c_str()); 
	std::cout << req._config_res["404"].c_str() << std::endl; //TODO HADI katjib 400.html !!
	std::string LastGoBody = "<!DOCTYPE html>\n<html>\n<head>\n<title>404 Not Found</title>\n</head>\n<body>\n<h1>Not Found</h1>\n<p>The requested URL was not found on this server.</p>\n</body>\n</html>\n";
	std::string body;
	

	if (!file.is_open())
		body = LastGoBody;
	else
		body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(404)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/html")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
void Response::MethodNotAllowed(int client_socket, HttpRequestData &req)
{
	std::ifstream file(req._config_res["405"].c_str());
	std::string LastGoBody = "<!DOCTYPE html>\n<html>\n<head>\n<title>405 Method Not Allowed</title>\n</head>\n<body>\n<h1>Method Not Allowed</h1>\n<p>The method is not allowed for the requested URL.</p>\n</body>\n</html>\n";
	std::string body;

	if (!file.is_open())
		body = LastGoBody;
	else
		body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(405)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/html")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
void Response::InternalServerError(int client_socket, HttpRequestData &req)
{
	std::ifstream file(req._config_res["500"].c_str());
	std::string body;
	std::string LastGoBody = "<!DOCTYPE html>\n<html>\n<head>\n<title>500 Internal Server Error</title>\n</head>\n<body>\n<h1>Internal Server Error</h1>\n<p>The server encountered an internal error or misconfiguration and was unable to complete your request.</p>\n</body>\n</html>\n";

	if (!file.is_open())
		body = LastGoBody;
	else
		body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(500)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/html")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
void Response::NotImplemented(int client_socket, HttpRequestData &req)
{
	std::ifstream file(req._config_res["501"].c_str());
	std::string body;
	std::string LastGoBody = "<!DOCTYPE html>\n<html>\n<head>\n<title>501 Not Implemented</title>\n</head>\n<body>\n<h1>Not Implemented</h1>\n<p>The server does not support the action requested by the browser.</p>\n</body>\n</html>\n";

	if (!file.is_open())
		body = LastGoBody;
	else
		body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(501)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/html")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
Response &Response::Generate(int isChunked)
{
	if (isChunked)
	{
		switch (this->_ChunkedState)
		{
		case RESPONSE::CHUNKED_HEADERS:
		{
			this->_Resp = _version + " " + _Status + "\r\n";
			for (std::map<std::string, std::string>::iterator it = _Headers.begin(); it != _Headers.end(); it++)
				this->_Resp += it->first + ": " + it->second + "\r\n";
			this->_Resp += "\r\n";
			break;
		}
		case RESPONSE::CHUNKED_BODY:
		{
			std::stringstream hexStream;
			hexStream << std::hex << this->_Body.size();
			std::string hexSize = hexStream.str();
			this->_Resp = hexSize + "\r\n";
			this->_Resp += this->_Body + "\r\n";
			break;
		}
		case RESPONSE::CHUNKED_END:
		{
			this->_Resp = "0\r\n\r\n";
			break;
		}
		default:
			break;
		}
	}
	else
	{
		this->_Resp = _version + " " + _Status + "\r\n";
		for (std::map<std::string, std::string>::iterator it = _Headers.begin(); it != _Headers.end(); it++)
		{
			this->_Resp += it->first + ": " + it->second + "\r\n";
		}
		this->_Resp += "\r\n";
		this->_Resp += _Body;
	}
	return *this;
}
int Response::Send(int client_socket)
{
	ssize_t sent = send(client_socket, this->_Resp.c_str(), this->_Resp.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
	if (sent == -1)
		std::cout << "error sending." << std::endl;
	Clear();
	return 0;
}
void Response::Clear()
{
	_Resp = "";
	_Body = "";
	_Status = "";
	_Headers.clear();
}
