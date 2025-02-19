#include "../../includes/http/response.hpp"

void Response::Http301(int client_socket, std::string redirection_link)
{
	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(301)
		.setDefaultHeaders()
		.WithHeader("Location", redirection_link)
		.WithHeader("connection", "keep-alive")
		.Generate()
		.Send(client_socket);
}
bool Response::OpenFile(const std::string &resolvedPath, HttpRequestData &req, int client_socket)
{
	(void)client_socket;
	(void)req;

	this->_file.open(resolvedPath.c_str(), std::ios::in | std::ios::binary);
	if (!this->_file.is_open())
	{
		// File not found
		NotFound(client_socket, req);
		return false;
	}
	this->_file.seekg(0, std::ios::end);
	_fileSize = this->_file.tellg();
	this->_file.seekg(0, std::ios::beg);
	return true;
}
int Response::Serve(int client_socket, HttpRequestData &req)
{
	const size_t chunk_threshold = 2 * 1024 * 1024; // 2mb
	const size_t buffer_size = 4096;				// 4kb
	std::string resolvedPath = req._location_res["root"] + req._uri.host;

	// ***CGI***
	std::string ext = (resolvedPath.find_last_of('.') != std::string::npos) ? resolvedPath.substr(resolvedPath.find_last_of('.'))
																			: ""; // [ ".php" || ""]
	if (req._location_res.find("cgi") && !req._location_res[ext.data()].empty())
		return (ServeCGI(client_socket, req._location_res[ext.data()], req));
	// *********

	// If GET method supported.
	if (!req._location_res.check("GET"))
	{
		MethodNotAllowed(client_socket, req);
		return (1);
	}
	// If should be redirected.
	if (req._location_res.find("redirect"))
	{
		Http301(client_socket, req._location_res["redirect"]);
		return 1;
	}
	// check if a req is for a dir
	if (isDirectory(resolvedPath))
	{
		// as NGINX do, if a req of dir doesnt end with /, it performs a redirect call.
		if (resolvedPath[resolvedPath.length() - 1] != '/')
			return (Http301(client_socket, req._uri.host + '/'), 1);
		// check if the dir has the main index.
		if (access((resolvedPath + req._location_res["index"]).c_str(), F_OK) == 0)
			resolvedPath += req._location_res["index"]; // ResolvedPath is completed somehow.
		else
		{
			// TODO :Check if autoindex ON, later. ELSE, 403 FORBIDEN.
			return ServeDirectory(client_socket, resolvedPath, req);
		}
	}
	if (!_file.is_open())
	{
		if (!OpenFile(resolvedPath, req, client_socket))
			return 1; // Error, file not found

		if (_fileSize < chunk_threshold)
			return ServeFile(client_socket, resolvedPath, req);
		else
		{
			(this)->WithHttpVersion(Version::toString(req._version)).WithStatus(200).setDefaultHeaders().WithHeader("Content-Type", GetMimeType(resolvedPath)).WithHeader("Transfer-Encoding", "chunked").Generate(1).Send(client_socket);
			_ChunkedState = RESPONSE::CHUNKED_BODY;
			return 0;
		}
	}
	// Read and send the next chunk
	char buffer[buffer_size];
	_file.read(buffer, buffer_size);
	std::string bufferStr(buffer, _file.gcount());
	if (_file.eof() && bufferStr.empty())
	{
		// End of file, send the final chunk
		_ChunkedState = RESPONSE::CHUNKED_END;
		this->Generate(1)
			.Send(client_socket);
		_file.close();
		return 1; // Done
	}
	else
	{
		// Send the current chunk
		(this)->WithBody(bufferStr).Generate(1).Send(client_socket);
		return 0; // Not done, continue through epoll
	}
}
int Response::ServeFile(int client_socket, std::string resolvedPath, HttpRequestData &req)
{
	std::string body;
	(void)req;

	body.assign((std::istreambuf_iterator<char>(_file)), std::istreambuf_iterator<char>()); // TODO May fail here, in case of file not found or other errors.

	(this)->WithHttpVersion(Version::toString(Version::HTTP_1_1)).WithStatus(200).setDefaultHeaders().WithHeader("Content-Type", GetMimeType(resolvedPath)).WithBody(body).Generate().Send(client_socket);

	_file.close();
	return 1;
}
int Response::ServeDirectory(int client_socket, std::string DirPath, HttpRequestData &req)
{
	DIR *dir;
	struct dirent *ent;
	std::ostringstream html;

	dir = opendir(DirPath.c_str());
	if (dir == NULL)
	{
		InternalServerError(client_socket, req);
		return 1;
	}

	html << "<html><head><title>Index of " << DirPath << "</title></head><body><h1>Index of " << DirPath << "</h1><hr><pre>";

	while ((ent = readdir(dir)) != NULL)
	{
		std::string name = ent->d_name;
		if (name == "." || name == "..")
			continue; // Skip current and parent directory links

		if (isDirectory(DirPath + name))
			name += "/"; // Append a slash to indicate a directory
		// Generate relative path safely
		std::string relativePath = DirPath;
		if (relativePath.compare(0, 4, "www/") == 0) // Ensure "www/" prefix exists
			relativePath = relativePath.substr(4);

		html << "<a href=\"" << name << "\">" << name << "</a><br>";
	}

	html << "</pre><hr></body></html>";
	closedir(dir);

	(this)->WithHttpVersion("HTTP/1.1").WithStatus(200).setDefaultHeaders().WithHeader("Content-Type", "text/html").WithBody(html.str()).Generate().Send(client_socket);

	return 1;
}
int Response::ServeCGI(int client_socket, std::string ext, HttpRequestData &req)
{
	std::cout << "CGI" << std::endl;
	RequestCgi req_cgi = setupCgiRequest(req, ext);
	ResponseCgi res_cgi;
	handleCGI(req_cgi, res_cgi);
	std::ifstream generated_file;

	if (res_cgi.getStatus() > 500) // TODO: I WILL DETAIL THE 5.. RES LATER
	{
		std::cout << "bec of here" << std::endl;
		return (InternalServerError(client_socket, req), 1);
	}
	else
	{
		generated_file.open(res_cgi.getBodyFile().c_str(), std::ios::in | std::ios::binary);
		if (!generated_file.is_open())
			return (InternalServerError(client_socket, req), 1);

		Response custom_res;
		custom_res.WithStatus(res_cgi.getStatus()).WithHttpVersion("HTTP/1.1").setDefaultHeaders();
		for (std::map<std::string, std::string>::const_iterator it = res_cgi.getHeaders().begin();
			 it != res_cgi.getHeaders().end();
			 ++it)
		{
			custom_res.WithHeader(it->first, it->second);
		}
		std::string body;
		body.assign((std::istreambuf_iterator<char>(generated_file)), std::istreambuf_iterator<char>());
		custom_res.WithBody(body).Generate().Send(client_socket);
		remove(res_cgi.getBodyFile().c_str());
	}
	return (1);
}
