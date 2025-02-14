#ifndef PARSE_CONFIG_HPP
#define PARSE_CONFIG_HPP

#include <climits>
#include "Config_Loader.hpp"

using namespace std;

const string  KEYOFLOCATION[] = {"uri" , "upload"  , "redirect" , "cgi" , "root" , "methods" , "index" , "autoindex" , "methods_cgi"};

const string  NAMESOFBLOCKS[] = {"[server]" , "[server.errors]" , "[server.location]"};

const string  KEYOFSERVER[] = {"host"  , "body_size", "port" , "server_name"};

const string  VALUES[] = {"GET" , "POST" , "DELETE"};




class Parse_Config
{

private:
    vector<int (*)(int)> func_ptr;
    vector<ConfigLoader >  blocks;
    vector<addrinfo *> save_addr;
    vector<int> socks;
    ConfigLoader data;
    t_map loc;
    string key;
    string value;
    bool check_host;
    bool _errno;
    int limit;
    int flag;
public:
    Parse_Config();
    ~Parse_Config();

public:
    ConfigLoader operator()(const char *);

public:
    vector<addrinfo *>getHosts();

public:
    void CheckArrayOfValue(vector<string> &data ,int flag);
    void getKeyValue(string &line , char set);
    ConfigLoader default_Server();
    void ParseFile(const char *_file);
    void CheckString(string &str);
    int CheckCharacter(char c);
    void ft_getaddrinfo();
    void  throwConfigError(bool expr , const char *str);
    bool check();

public:
    void CheckBlockLocation();
    void CheckBlockErrors();
    void CheckInfoServer();

public:
    void push_back_location();
    void push_back_data();
    void test();
};

extern Parse_Config Config;

#endif