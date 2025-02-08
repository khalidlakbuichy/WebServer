#include "../../includes/http/response.hpp"

void Response::Http204(int client_socket)
{
	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(204)
		.setDefaultHeaders()
		.Generate()
		.Send(client_socket);
}

int Response::Delete(int client_socket, HttpRequestData &req)
{
	std::string Upload_dir = req._location_res["upload"];
	std::string Root = req._location_res["root"];
	Root += (Root[Root.length() - 1] == '/' ? "" : "/");
	Upload_dir += (Upload_dir[Upload_dir.length() - 1] == '/' ? "" : "/");

	std::string fullDir = Root + Upload_dir + req._uri.host;

	// Check if DELETE allowed
	if (!req._location_res.check("DELETE"))
	{
		MethodNotAllowed(client_socket, req);
		return (1);
	}

	// Check if the file exists
	if (access(fullDir.c_str(), F_OK) == -1)
	{
		NotFound(client_socket, req);
		return 1;
	}

	// Delete the file
	int status = remove(fullDir.c_str());
	if (status != 0)
	{
		InternalServerError(client_socket, req);
		return 1;
	}
	else
		Http204(client_socket);
	return 1;
}