#include "../../includes/http/response.hpp"

void Response::Http413(int client_socket)
{
	Response res;

	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(413)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "text/plain")
		.WithHeader("Connection", "close")
		.WithBody("413 Payload Too Large")
		.Generate()
		.Send(client_socket);
}
void Response::Http201(int client_socket)
{
	// Create the response body
	std::string body = "{\"status\": \"created\", \"filename\": \"filename.txt\"}";

	std::string content_length = NumberToString(body.size());

	// Build the response
	Response res;
	res.WithHttpVersion("HTTP/1.1")
		.WithStatus(201)
		.setDefaultHeaders()
		.WithHeader("Content-Type", "application/json")
		.WithBody(body)
		.Generate()
		.Send(client_socket);
}
std::string Response::FieldsMapJsonify(std::map<std::string, std::string> Fields)
{
	std::string data;
	data += "{";

	for (std::map<std::string, std::string>::iterator it = Fields.begin(); it != Fields.end(); it++)
	{
		data += "\"" + it->first + "\": \"" + it->second + "\"";
		// Add comma if not last element
		if (it != --Fields.end()) // todo -___-
			data += ", ";
	}
	data += "}";
	return data;
}

int Response::ParseMultiPartFormData(HttpRequestData &req, int client_socket)
{
	if (!req._initialized)
	{
		std::string Root = req._location_res["root"];
		// Seting up tmp file
		if (!_tmp_file.is_open())
		{
			_tmp_file.open(req._tmp_file_name.c_str(), std::ios::binary);
			if (!_tmp_file.is_open())
				return (req._Error_msg = "Could not open tmp file", -1);
		}

		// Seting up upload dir
		std::string Upload_dir = req._location_res["upload"];
		Root += (Root[Root.length() - 1] == '/' ? "" : "/");
		Upload_dir += (Upload_dir[Upload_dir.length() - 1] == '/' ? "" : "/");
		req._fullDir = Root + Upload_dir;

		req._initialized = true;
		req.multipart_state = PARSE::STATE_BOUNDARY;
		// if upload dirr is exist
		if (!isDirectory(req._fullDir))
			return (req._Error_msg = "Unacessible upload folder", -1);
	}

	int ChunkSize = 16384;
	char Buffer[ChunkSize];
	const char *ptr = Buffer;
	const char *end;

	_tmp_file.clear();
	_tmp_file.seekg(req._curr_tmpfile_pos);
	_tmp_file.read(Buffer, ChunkSize);

	std::streamsize bytesRead = _tmp_file.gcount();
	if (bytesRead <= 0)
		return (req._Error_msg = "End of file or read error", -1);

	end = ptr + bytesRead;

	while (ptr != end)
	{
		switch (req.multipart_state)
		{
		case PARSE::STATE_BOUNDARY:
		{
			std::string Boundary = "--" + req._boundary + "\r\n";
			std::string BoundaryEnd = "--" + req._boundary + "--\r\n";


			if (memcmp(ptr, Boundary.c_str(), Boundary.length()) == 0)
			{
				ptr += (Boundary.length());
				req.multipart_state = PARSE::STATE_HEADERS;
			}
			else if (memcmp(ptr, BoundaryEnd.c_str(), BoundaryEnd.length()) == 0)
			{
				ptr += (Boundary.length());
				_tmp_file.close();
				remove(req._tmp_file_name.c_str());

				std::string data = FieldsMapJsonify(req._Fields);
				Response res;
				res.WithHttpVersion("HTTP/1.1")
					.WithStatus(201)
					.setDefaultHeaders()
					.WithHeader("Content-Type", "application/json")
					.WithBody("{\"data\": " + data + "}")
					.Generate()
					.Send(client_socket);
				return (1);
			}
			else
			{
				return (req._Error_msg = "Invalid Boundary", -1);
			}
			break;
		}
		case PARSE::STATE_HEADERS:
		{
			std::string should_find = "Content-Disposition: form-data;";

			if (memcmp(ptr, should_find.c_str(), should_find.length()) == 0)
				ptr += should_find.length();
			else
				return (req._Error_msg = "Invalid Content-Disposition", -1);

			if (memcmp(ptr, " name=", 6) == 0)
			{
				ptr += 6;
				std::string rest = std::string(++ptr);
				req._curr_field_name = rest.substr(0, rest.find("\""));
				ptr += req._curr_field_name.length() + 1;
			}
			else
				return (req._Error_msg = "Invalid Field Name", -1);
			if (memcmp(ptr, "; filename=", 11) == 0)
			{
				req.multipart_state = PARSE::STATE_FILE_DATA_SETUP;
				ptr += 12; // plus 1 for "
				std::string rest = std::string(ptr);
				req._curr_uploaded_file = rest.substr(0, rest.find("\""));

				req._Fields[req._curr_field_name] = req._curr_uploaded_file;

				ptr += req._curr_uploaded_file.length() + 1;
			}
			else
				req.multipart_state = PARSE::STATE_FIELD_VALUE;

			if (memcmp(ptr, "\r\n", 2) == 0)
				ptr += 2;
			else
				return (req._Error_msg = "invalid end of line.", -1);
			break;
		}
		case PARSE::STATE_FIELD_VALUE:
		{
			if (memcmp(ptr, "\r\n", 2) == 0)
				ptr += 2;
			else
				return (req._Error_msg = "invalid line break.", -1);

			std::string rest = std::string(ptr);
			req._curr_field_value = rest.substr(0, rest.find("\r\n"));

			// Add the field to the fields map
			req._Fields[req._curr_field_name] = req._curr_field_value;
			ptr += req._curr_field_value.length() + 2;

			req._curr_field_name.clear();
			req._curr_field_value.clear();

			req.multipart_state = PARSE::STATE_BOUNDARY;
			break;
		}
		case PARSE::STATE_FILE_DATA_SETUP:
		{
			if (memcmp(ptr, "Content-Type: ", 14) == 0)
			{
				ptr += 14;
				std::string rest = std::string(ptr);
				req._curr_content_type = rest.substr(0, rest.find("\r\n"));
				ptr += req._curr_content_type.length() + 2;
			}
			else
				return (req._Error_msg = "Invalid Content-Type", -1);

			if (memcmp(ptr, "\r\n", 2) == 0)
				ptr += 2;
			else
				return (req._Error_msg = "invalid end of line.", -1);

			std::string full_path = req._fullDir + req._curr_uploaded_file;
			_uploaded_file.open(full_path.c_str(), std::ios::binary);
			if (!_uploaded_file.is_open())
				return (req._Error_msg = "Could not open uploaded file", -1);
			req.multipart_state = PARSE::STATE_FILE_DATA;
			break;
		}
		case PARSE::STATE_FILE_DATA:
		{
			std::vector<char> rest(ptr, end);
			std::string boundary = "\r\n--" + req._boundary;

			std::vector<char>::iterator boundary_pos = std::search(
				rest.begin(), rest.end(),
				boundary.begin(), boundary.end());

			if (boundary_pos != rest.end())
			{
				// Write the data before the boundary to the file
				_uploaded_file.write(&rest[0], boundary_pos - rest.begin());
				_uploaded_file.close();

				// Move the pointer to the end of the boundary
				ptr += (boundary_pos - rest.begin()) + 2; // +2 for \r\n
				req.multipart_state = PARSE::STATE_BOUNDARY;
			}
			else
			{
				// step 1, take last boundary.length() + 10 bytes from the rest
				std::vector<char> last_bytes(rest.end() - boundary.length() - 10, rest.end());
				// step 2, read boundary.length() + 10 bytes from the file and add to the last_bytes
				std::vector<char> next_bytes(boundary.length() + 10);
				_tmp_file.read(&next_bytes[0], boundary.length() + 10);
				last_bytes.insert(last_bytes.end(), next_bytes.begin(), next_bytes.end());
				// step 3, check for boundary in the last_bytes
				std::vector<char>::iterator boundary_pos = std::search(
					last_bytes.begin(), last_bytes.end(),
					boundary.begin(), boundary.end());

				if (boundary_pos != last_bytes.end())
				{
					// Write the data before the boundary to the file
					_uploaded_file.write(&rest[0], boundary_pos - rest.begin());
					_uploaded_file.close();

					// Move the pointer to the end of the boundary
					ptr = end;
					// add the aditional bytes readed till the start of the boundary
					req._curr_tmpfile_pos += (boundary_pos - last_bytes.begin());
					req.multipart_state = PARSE::STATE_BOUNDARY;
				}
				else
				{
					// Write the entire rest of the data to the file
					_uploaded_file.write(&rest[0], rest.size());
					ptr += rest.size();
				}
			}
			break;
		}
		default:
			break;
		}
	}

	req._curr_tmpfile_pos += (ptr - Buffer);
	return (0);
}

int Response::Post(int client_socket, HttpRequestData &req)
{
	std::string Root = req._location_res["root"];
	std::string resolvedPath = Root + req._uri.host; // No index file concat yet.

	// ***CGI***
	std::string ext = (resolvedPath.find_last_of('.') != std::string::npos) ? resolvedPath.substr(resolvedPath.find_last_of('.'))
																			: ""; // [ ".php" || ""]
	if (req._location_res.find("cgi") && !req._location_res[ext.data()].empty())
		return (ServeCGI(client_socket, ext, req));
	// *********

	if (!req._location_res.check("POST"))
	{
		MethodNotAllowed(client_socket, req);
		return (1);
	}

	return (ParseMultiPartFormData(req, client_socket));
}
