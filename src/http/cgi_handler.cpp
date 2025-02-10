#include "../../includes/http/cgi_handler.hpp"

// Req CGI
RequestCgi::RequestCgi() : _request_method(""), _script_name(""), _query_string(""),
						   _content_length(""), _content_type(""), _body(""), _cookies(""), _path_info(""), _interpreter("") {}
RequestCgi::RequestCgi(const std::string &request_method,
					   const std::string &script_name,
					   const std::string &query_string,
					   const std::string &content_length,
					   const std::string &content_type,
					   const std::string &body,
                       const std::string &cookies,
					   const std::string &path_info,
					   const std::string &interpreter)
	: _request_method(request_method), _script_name(script_name),
	  _query_string(query_string), _content_length(content_length),
	  _content_type(content_type), _body(body), _cookies(cookies),
      _path_info(path_info), _interpreter(interpreter) {}
RequestCgi::~RequestCgi() {}
// Setters:
void RequestCgi::setRequestMethod(const std::string &request_method) { _request_method = request_method; }
void RequestCgi::setScriptName(const std::string &script_name) { _script_name = script_name; }
void RequestCgi::setQueryString(const std::string &query_string) { _query_string = query_string; }
void RequestCgi::setContentLength(const std::string &content_length) { _content_length = content_length; }
void RequestCgi::setContentType(const std::string &content_type) { _content_type = content_type; }
void RequestCgi::setBody(const std::string &body) { _body = body; }
void RequestCgi::setCookies(const std::string &cookies) { _cookies = cookies; }
void RequestCgi::setPathInfo(const std::string &path_info) { _path_info = path_info; }
void RequestCgi::setInterpreter(const std::string &interpreter) { _interpreter = interpreter; }
// Getters:
const std::string &RequestCgi::getRequestMethod() const { return _request_method; }
const std::string &RequestCgi::getScriptName() const { return _script_name; }
const std::string &RequestCgi::getQueryString() const { return _query_string; }
const std::string &RequestCgi::getContentLength() const { return _content_length; }
const std::string &RequestCgi::getContentType() const { return _content_type; }
const std::string &RequestCgi::getBody() const { return _body; }
const std::string &RequestCgi::getCookies() const { return _cookies; }
const std::string &RequestCgi::getPathInfo() const { return _path_info; }
const std::string &RequestCgi::getInterpreter() const { return _interpreter; }


// Resp CGI
ResponseCgi::ResponseCgi()
    : _status(200), _body(""), _bodyFile("") {}
ResponseCgi::~ResponseCgi() {}
void ResponseCgi::setStatus(int status) {
    _status = status;
}
int ResponseCgi::getStatus() const {
    return _status;
}
void ResponseCgi::setHeader(const std::string &key, const std::string &value) {
    _headers[key] = value;
}
const std::map<std::string, std::string>& ResponseCgi::getHeaders() const {
    return _headers;
}
void ResponseCgi::setBody(const std::string &body) {
    _body = body;
}
const std::string& ResponseCgi::getBody() const {
    return _body;
}
void ResponseCgi::setBodyFile(const std::string &filePath) {
    _bodyFile = filePath;
}
const std::string& ResponseCgi::getBodyFile() const {
    return _bodyFile;
}


static void trim(std::string &str) {
    const char* whitespace = " \t\n\r";
    str.erase(0, str.find_first_not_of(whitespace));
    str.erase(str.find_last_not_of(whitespace) + 1);
}

// =================== CGI Handler Function ===================

void handleCGI(RequestCgi &request, ResponseCgi &response) {
    std::string interpreter = request.getInterpreter();
    std::string scriptFile = request.getScriptName();

    // Set up the CGI environment variables
    std::map<std::string, std::string> env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SCRIPT_FILENAME"] = scriptFile;
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REQUEST_METHOD"] = request.getRequestMethod();
    env["SCRIPT_NAME"] = request.getScriptName();
    env["PATH_INFO"] = request.getPathInfo();
    env["HTTP_COOKIE"] = request.getCookies();
    env["REDIRECT_STATUS"] = "200"; // for php-cgi

    if (request.getRequestMethod() == "POST") {
        env["CONTENT_LENGTH"] = request.getContentLength();
        env["CONTENT_TYPE"] = request.getContentType();
    } else if (request.getRequestMethod() == "GET" && !request.getQueryString().empty()) {
        env["QUERY_STRING"] = request.getQueryString();
    }

    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        response.setStatus(500);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        response.setStatus(500);
        return;
    } else if (pid == 0) {
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        std::vector<char *> env_ptrs;
        for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            env_ptrs.push_back(strdup(entry.c_str()));
        }
        env_ptrs.push_back(NULL);

        char *argv[] = {const_cast<char *>(interpreter.c_str()), const_cast<char *>(scriptFile.c_str()), NULL};
        execve(interpreter.c_str(), argv, &env_ptrs[0]);
        _exit(EXIT_FAILURE);
    } else {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        if (request.getRequestMethod() == "POST") {
            const std::string &body = request.getBody();
            if (!body.empty()) {
                write(stdin_pipe[1], body.c_str(), body.size());
            }
        }
        close(stdin_pipe[1]);

        char tmpFileName[] = "/tmp/cgi_response_XXXXXX";
        int tmp_fd = mkstemp(tmpFileName);
        if (tmp_fd == -1) {
            response.setStatus(500);
            return;
        }
        close(tmp_fd);

        std::ofstream outFile(tmpFileName, std::ios::binary);
        if (!outFile) {
            response.setStatus(500);
            return;
        }

        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
            outFile.write(buffer, bytes_read);
        }
        outFile.close();
        close(stdout_pipe[0]);

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            response.setStatus(502);
            return;
        }

        std::ifstream inFile(tmpFileName, std::ios::binary);
        if (!inFile) {
            response.setStatus(500);
            return;
        }
        std::string cgi_output((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        size_t header_end = cgi_output.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            response.setBodyFile(tmpFileName);
            response.setHeader("Content-Type", "text/plain");
            return;
        }

        std::string headers = cgi_output.substr(0, header_end);
        std::string response_body = cgi_output.substr(header_end + 4);

        std::istringstream header_stream(headers);
        std::string header_line;
        while (std::getline(header_stream, header_line)) {
            size_t colon_pos = header_line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = header_line.substr(0, colon_pos);
                std::string value = header_line.substr(colon_pos + 1);
                trim(key);
                trim(value);
                if (key == "Status") {
                    size_t space_pos = value.find(' ');
                    if (space_pos != std::string::npos) {
                        response.setStatus(std::atoi(value.substr(0, space_pos).c_str()));
                    }
                } else {
                    response.setHeader(key, value);
                }
            }
        }

        response.setBodyFile(tmpFileName);
    }
}
