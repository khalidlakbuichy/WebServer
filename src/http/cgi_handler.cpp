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


// Utility: trim whitespace from both ends of a string
static void trim(std::string &str) {
    const char *whitespace = " \t\n\r";
    str.erase(0, str.find_first_not_of(whitespace));
    str.erase(str.find_last_not_of(whitespace) + 1);
}

// =================== Non-Blocking CGI Handler Function with Timeout ===================
void handleCGI(RequestCgi &request, ResponseCgi &response) {
    std::string interpreter = request.getInterpreter();
    std::string scriptFile  = request.getScriptName();

    // Set up the CGI environment variables.
    std::map<std::string, std::string> env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SCRIPT_FILENAME"]   = scriptFile;
    env["SERVER_PROTOCOL"]   = "HTTP/1.1";
    env["REQUEST_METHOD"]    = request.getRequestMethod();
    env["SCRIPT_NAME"]       = request.getScriptName();
    env["PATH_INFO"]         = request.getPathInfo();
    env["HTTP_COOKIE"]       = request.getCookies();
    env["REDIRECT_STATUS"]   = "200"; // For php-cgi compatibility

    if (request.getRequestMethod() == "POST") {
        env["CONTENT_LENGTH"] = request.getContentLength();
        env["CONTENT_TYPE"]   = request.getContentType();
    } else if (request.getRequestMethod() == "GET" && !request.getQueryString().empty()) {
        env["QUERY_STRING"] = request.getQueryString();
    }

    // Create pipes for communication with the CGI process.
    int stdin_pipe[2];
    int stdout_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        response.setStatus(500);
        return;
    }

    // Set the parent's ends of the pipes to non-blocking.
    fcntl(stdin_pipe[1], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK, FD_CLOEXEC);

    // Fork the process.
    pid_t pid = fork();
    if (pid == -1) {
        response.setStatus(500);
        return;
    }
    else if (pid == 0) {  // Child process
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        // Duplicate the pipe ends.
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        // Build an environment array for execve.
        std::vector<char*> envp;
        for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            envp.push_back(strdup(entry.c_str()));
        }
        envp.push_back(NULL);

        // Prepare arguments and execute the CGI interpreter.
        char *argv[] = { const_cast<char*>(interpreter.c_str()),
                         const_cast<char*>(scriptFile.c_str()),
                         NULL };
        execve(interpreter.c_str(), argv, &envp[0]);
        _exit(EXIT_FAILURE); // In case execve fails.
    }
    else {  // Parent process
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        std::string request_body = request.getBody();
        size_t body_written = 0;
        std::string cgi_output;  // Will accumulate the full CGI output (headers + body)

        // Set up poll file descriptors:
        pollfd fds[2];
        fds[0].fd = stdin_pipe[1]; // Write end for CGI's STDIN
        fds[0].events = POLLOUT;
        fds[1].fd = stdout_pipe[0]; // Read end for CGI's STDOUT
        fds[1].events = POLLIN;

        bool stdin_closed = false;
        bool stdout_closed = false;

        // Set overall timeout (in milliseconds) for the CGI response.
        const int overall_timeout_ms = 5000; // 5 seconds
        struct timeval start, now;
        gettimeofday(&start, NULL);

        // Main poll loop.
        while (!stdin_closed || !stdout_closed) {
            gettimeofday(&now, NULL);
            long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000;
            if (elapsed_ms >= overall_timeout_ms) {
                // Overall timeout reached: kill CGI process and return 504.
                kill(pid, SIGKILL);
                waitpid(pid, NULL, 0);
                response.setStatus(504);  // Gateway Timeout
                return;
            }

            int poll_timeout = overall_timeout_ms - elapsed_ms;
            int ret = poll(fds, 2, poll_timeout);
            if (ret < 0) {
                response.setStatus(500);
                return;
            }

            // Write request body to CGI if needed.
            if (!stdin_closed && (fds[0].revents & POLLOUT)) {
                if (!request_body.empty()) {
                    ssize_t n = write(stdin_pipe[1],
                                      request_body.c_str() + body_written,
                                      request_body.size() - body_written);
                    if (n > 0) {
                        body_written += n;
                    }
                    else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        response.setStatus(500);
                        return;
                    }
                }
                if (body_written >= request_body.size() || request_body.empty()) {
                    close(stdin_pipe[1]);
                    stdin_closed = true;
                    fds[0].fd = -1;
                }
            }

            // Read CGI output.
            if (!stdout_closed && (fds[1].revents & (POLLIN | POLLHUP))) {
                char buffer[4096];
                ssize_t n = read(stdout_pipe[0], buffer, sizeof(buffer));
                if (n > 0) {
                    cgi_output.append(buffer, n);
                }
                else if (n == 0) {
                    // End-of-file: close the pipe.
                    close(stdout_pipe[0]);
                    stdout_closed = true;
                    fds[1].fd = -1;
                }
                else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    response.setStatus(500);
                    return;
                }
            }
        } // end poll loop

        // Wait for CGI process termination.
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            response.setStatus(502);
            return;
        }

        // Create a temporary file to store only the CGI body.
        char tmpFileName[] = "/tmp/cgi_response_XXXXXX";
        int tmp_fd = mkstemp(tmpFileName);
        if (tmp_fd == -1) {
            response.setStatus(500);
            return;
        }
        close(tmp_fd);

        // Parse the CGI output to separate headers from the body.
        size_t header_end = cgi_output.find("\r\n\r\n");
        std::string body_content;
        if (header_end == std::string::npos) {
            body_content = cgi_output;
            response.setHeader("Content-Type", "text/plain");
        } else {
            std::string headers = cgi_output.substr(0, header_end);
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
            // Body begins after the "\r\n\r\n" delimiter.
            body_content = cgi_output.substr(header_end + 4);
        }

        // Overwrite the temporary file so that it contains only the body.
        std::ofstream bodyFile(tmpFileName, std::ios::binary | std::ios::trunc);
        if (!bodyFile) {
            response.setStatus(500);
            return;
        }
        bodyFile.write(body_content.c_str(), body_content.size());
        bodyFile.close();

        // Associate the temporary file with the response.
        response.setBodyFile(tmpFileName);
    }
}
