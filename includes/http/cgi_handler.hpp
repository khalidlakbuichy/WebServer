#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include "../types/reqTypes.hpp"

class RequestCgi
{
private:
	std::string _request_method;
	std::string _script_name;
	std::string _query_string;
	std::string _content_length;
	std::string _content_type;
	std::string _body;
	std::string _cookies;
	std::string _path_info;
	std::string _interpreter;

public:
	// Constructors:
	RequestCgi();

	RequestCgi(const std::string &request_method,
			   const std::string &script_name,
			   const std::string &query_string,
			   const std::string &content_length,
			   const std::string &content_type,
			   const std::string &body,
			   const std::string &cookies,
			   const std::string &path_info,// PATH_INFO: identifies the resource or sub-resource to be returned by the CGI script, and it is derived from the portion of the URI path following the script name but preceding any query data.
			   const std::string &interpreter);

	~RequestCgi();

	// Setters:
	void setRequestMethod(const std::string &request_method);
	void setScriptName(const std::string &script_name);
	void setQueryString(const std::string &query_string);
	void setContentLength(const std::string &content_length);
	void setContentType(const std::string &content_type);
	void setBody(const std::string &body);
	void setCookies(const std::string &cookies);
	void setPathInfo(const std::string &path_info);
	void setInterpreter(const std::string &interpreter);

	// Getters:
	const std::string &getRequestMethod() const;
	const std::string &getScriptName() const;
	const std::string &getQueryString() const;
	const std::string &getContentLength() const;
	const std::string &getContentType() const;
	const std::string &getBody() const;
	const std::string &getCookies() const;
	const std::string &getPathInfo() const;
	const std::string &getInterpreter() const;
};
class ResponseCgi
{
public:
	ResponseCgi();
	~ResponseCgi();

	void setStatus(int status);
	int getStatus() const;

	void setHeader(const std::string &key, const std::string &value);
	const std::map<std::string, std::string> &getHeaders() const;

	void setBody(const std::string &body);
	const std::string &getBody() const;

	// New method: store the file path where the CGI body is saved.
	void setBodyFile(const std::string &filePath);
	const std::string &getBodyFile() const;

private:
	int _status;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _bodyFile;
};

// Main
void handleCGI(RequestCgi &request, ResponseCgi &response);

// Serving
RequestCgi setupCgiRequest(HttpRequestData &req, std::string interpreter);


#endif