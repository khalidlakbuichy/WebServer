#include "ResponseCgi.hpp"

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