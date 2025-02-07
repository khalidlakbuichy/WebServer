#include <unistd.h>
#include <sys/wait.h>
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
#include "RequestCgi.hpp"
#include "ResponseCgi.hpp"

static void trim(std::string &str) {
    const char* whitespace = " \t\n\r";
    str.erase(0, str.find_first_not_of(whitespace));
    str.erase(str.find_last_not_of(whitespace) + 1);
}

// =================== CGI Handler Function ===================

void handleCGI(RequestCgi &request, ResponseCgi &response) { //TODO [ HttpRequestData &req ]
    std::string interpreter = request.getInterpreter();
    std::string scriptFile = request.getScriptName();

    // Set up the CGI environment variables.
    std::map<std::string, std::string> env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SCRIPT_FILENAME"] = scriptFile;
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REQUEST_METHOD"] = request.getRequestMethod();
    env["SCRIPT_NAME"] = request.getScriptName(); //_uri.host
    env["PATH_INFO"] = request.getPathInfo();
    env["REDIRECT_STATUS"] = "200"; // for php-cgi

    if (request.getRequestMethod() == "POST") {
        env["CONTENT_LENGTH"] = request.getContentLength();
        env["CONTENT_TYPE"] = request.getContentType();
    } else if (request.getRequestMethod() == "GET" && !request.getQueryString().empty()) {
        env["QUERY_STRING"] = request.getQueryString(); // _uri.query
    }

    // Create pipes for CGI process I/O.
    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        response.setStatus(500);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        response.setStatus(500);
        return;
    } else if (pid == 0) { // Child process.
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        // Redirect child's STDIN and STDOUT.
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        // Prepare environment array for execve.
        std::vector<char*> env_ptrs;
        for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            env_ptrs.push_back(strdup(entry.c_str()));
        }
        env_ptrs.push_back(NULL);

        // Prepare arguments: argv[0] is the interpreter, argv[1] is the script.
        char* argv[] = { const_cast<char*>(interpreter.c_str()),
                         const_cast<char*>(scriptFile.c_str()),
                         NULL };
        execve(interpreter.c_str(), argv, &env_ptrs[0]);
        _exit(EXIT_FAILURE);
    } else { // Parent process.
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // If method is POST, write the request body to the child's STDIN.
        if (request.getRequestMethod() == "POST") { // TODO : _tmp_file_name
            const std::string &body = request.getBody();
            if (!body.empty()) {
                write(stdin_pipe[1], body.c_str(), body.size());
            }
        }
        close(stdin_pipe[1]);

        // Create a unique temporary file to store the CGI output.
        char tmpFileName[] = "/tmp/cgi_response_XXXXXX";
        int tmp_fd = mkstemp(tmpFileName);
        if (tmp_fd == -1) {
            response.setStatus(500);
            return;
        }
        close(tmp_fd);

        // Write CGI output from stdout_pipe into the temporary file.
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

        // Wait for child process to finish.
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            response.setStatus(502);
            return;
        }

        // Open the temporary file and read its content.
        std::ifstream inFile(tmpFileName, std::ios::binary);
        if (!inFile) {
            response.setStatus(500);
            return;
        }
        std::string cgi_output((std::istreambuf_iterator<char>(inFile)),
                                std::istreambuf_iterator<char>());
        inFile.close();

        // Look for the header/body delimiter ("\r\n\r\n").
        size_t header_end = cgi_output.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            // No headers found: assume entire output is body.
            response.setBodyFile(tmpFileName);
            response.setHeader("Content-Type", "text/plain");
            return;
        }

        // Extract headers and body.
        std::string headers = cgi_output.substr(0, header_end);
        std::string response_body = cgi_output.substr(header_end + 4);

        // Parse headers line by line.
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

        // Instead of loading the entire body into memory, record the temporary file path.
        response.setBodyFile(tmpFileName);
    }
}