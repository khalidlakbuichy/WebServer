#ifndef RESPONSECGI_HPP
#define RESPONSECGI_HPP

#include <map>
#include <string>

class ResponseCgi {
public:
    ResponseCgi();
    ~ResponseCgi();

    void setStatus(int status);
    int getStatus() const;

    void setHeader(const std::string &key, const std::string &value);
    const std::map<std::string, std::string>& getHeaders() const;

    void setBody(const std::string &body);
    const std::string& getBody() const;

    // New method: store the file path where the CGI body is saved.
    void setBodyFile(const std::string &filePath);
    const std::string& getBodyFile() const;
    
private:
    int _status;
    std::map<std::string, std::string> _headers;
    std::string _body;
    std::string _bodyFile;
};

#endif