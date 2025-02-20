#include "../../includes/config/Parse_Config.hpp"

string checks = "";

map<int , void (Parse_Config::*)()> funcCheck;
map<int , void (Parse_Config::*)()> funcPush;



void Parse_Config::test() {}

Parse_Config::Parse_Config() :check_host(true) {}

vector<addrinfo *>Parse_Config::getHosts()
{
    return save_addr;
}

bool Parse_Config::check()
{
    return(check_host);
}


int Parse_Config::ft_freeaddrinof()
{
    for(unsigned int i = 0; i < save_addr.size(); i++)
        freeaddrinfo(save_addr[i]);

    return 1;
}

void  Parse_Config::throwConfigError(bool expr , const char *str)
{
    stringstream ss;
    string err("webserve (line ");
    ss << limit;
    err += ss.str();
    err += " ) : ";
    err += string(str);

    if(expr) throw std::runtime_error(err);
}

void _fill()
{
    funcCheck[8] = &Parse_Config::CheckInfoServer;
    funcCheck[17] = &Parse_Config::CheckBlockLocation;
    funcCheck[15] = &Parse_Config::CheckBlockErrors;

    funcPush[8] = &Parse_Config::push_back_data;
    funcPush[17] = &Parse_Config::push_back_location;
    funcPush[15] = &Parse_Config::test;
}

ConfigLoader Parse_Config::default_Server()
{
    ConfigLoader def;
    t_map def_loc;

    def_loc.insert("uri" , "/");
    def_loc.insert("root" , "www");
    def_loc.insert("index" , "index.html");
    def_loc.insert("upload" , "uploads");
    def_loc.insert("autoindex" , "off");

    def.location.push_back(def_loc);

    def.server.insert("host" , "127.0.0.3");
    def.server.insert("port" , "8080");
    def.error.insert("404" , "www/errors/404.html");
    def._body_size = 20048;
    
    return def;
}

ConfigLoader Parse_Config::operator()(const char *_str)
{
    vector<ConfigLoader> it = blocks;
    string str = _str;

    try {getKeyValue(str , ':');}
    catch(const std::exception& e) {}

    for(unsigned long i = 0; i < blocks.size() ; i++)
    {
        _errno = (it[i].server.find("host", key.data()) || it[i].server.find("server_name", key.data())) && it[i].server.find("port", value.data());
        _errno += it[i].server.find("server_name", _str);
        if(_errno)
            return(it[i]);
    }
    it[0] = default_Server();
    return it[0];
}

Parse_Config::~Parse_Config()
{
    func_ptr.clear(); key.clear();
    value.clear(); blocks.clear();
};

int Parse_Config::CheckCharacter(char c)
{
    for(unsigned long i = 0;i < func_ptr.size();i++)
        if(func_ptr[i](c)) return 1;
    return (checks.find(c) != checks.npos);
}

void Parse_Config::CheckString(string &str)
{
    for(unsigned long i = 0; i < str.size() ; i++)
        throwConfigError(!CheckCharacter(str[i]) , "Invalid value for the key. It must be in the range of 300 to 599.");
}

void Parse_Config::CheckArrayOfValue( vector<string> &data , int flag)
{
    stringstream  obj(value);
    string word;
    size_t i;
    short size;

    i = 0;
    size = sizeof(VALUES) / sizeof(VALUES[0]);
    for(i = 0; obj >> word; i++)
    {
        throwConfigError(!flag && !count(&VALUES[0], &VALUES[size], word), "The value is invalid. Please provide a valid value and retry." );
        data.push_back(word);
    }
    _errno = (flag == 1 && i != 1) + (flag == 3  && i != 2); 
    throwConfigError(_errno , "The value is invalid. Please provide a valid value and retry." );
}

void Parse_Config::CheckBlockErrors()
{
    throwConfigError((key.size() != 3 || key[0] < '3' || key[0] > '5') , "This key must btw 300 to 599");
    func_ptr.push_back(isdigit); CheckString(key);

    throwConfigError(data.error.find(key) , "Duplicate key found. Each key must be unique.");
    CheckArrayOfValue(data.error[key] , 1);
}

void Parse_Config::CheckBlockLocation()
{
    flag = sizeof(KEYOFLOCATION) / sizeof(KEYOFLOCATION[0]);
    throwConfigError(!count(KEYOFLOCATION , &KEYOFLOCATION[flag] , key) ,  "This key cannot be used in this configuration");

    flag = !(key[0] == 'm');
    
    _errno = key[0] != 'c' && loc.find(key);
    throwConfigError(_errno , "Duplicate key found. Each key must be unique.");
    CheckArrayOfValue(loc[key] , flag);

    _errno = !loc.find("uri");
    _errno += key[0] == 'a' && (value != "on" && value != "off");
    _errno += key[0] == 'c' && (value.find(':') == value.npos || value.find(':') == value.size() - 1);
    throwConfigError(_errno, "The [server.location] configuration is incorrect. Please ensure all parameters are valid.");
}

void Parse_Config::CheckInfoServer()
{
    flag  = sizeof(KEYOFSERVER) / sizeof(KEYOFSERVER[0]);
    throwConfigError(!count(KEYOFSERVER , &KEYOFSERVER[flag] , key) ,  "This key cannot be used in this configuration");

    checks = "ps";
    flag = (checks.find(key[0]) != checks.npos ) ? 2 : 1 ;

    throwConfigError(data.server.find(key) , "Duplicate key found. Each key must be unique.");
    CheckArrayOfValue(data.server[key] , flag);

    if(key[0] == 'b')
    {
        size_t pos;
        pos = value.find_first_not_of("0123456789");
        _errno = pos != (value.size() - 1) || (value[pos] != 'K' && value[pos] != 'M');
        _errno += ((data._body_size = strtol(value.c_str() , NULL , 10)) > INT_MAX) ;
        throwConfigError(_errno , "The value must be a number followed by a unit ('M' -> MB, 'K' -> KB).");
        data._body_size *= (value[pos] == 'M' ? 1024 : 1) * 1024;
    }
}


void Parse_Config::getKeyValue(string &line , char set)
{
    size_t c;
    c = line.find(set);
    throwConfigError(c == line.npos , " Line must contain both a key and a value separated by '='.");

    key = line.substr(0 , c);
    value = line.substr(c + 1, line.npos);
    strtrim(key , " \t"); strtrim(value , " \t");
}


void Parse_Config::ft_getaddrinfo()
{
    vector<string> it;
    const char *host;
    addrinfo *res;

    host = data.server["host"].data();
    it = data.server[string("port")];
    for(unsigned long i = 0; i < it.size() ; i++)
    {
        res = NULL;
        flag = strtol(it[i].c_str() , NULL , 10);
        throwConfigError((flag < 1024 || flag > 65535) && ft_freeaddrinof() , "the port is numeric and within the valid range (1-65535)");

        flag =  getaddrinfo(host, it[i].data(), NULL, &res);
        throwConfigError(flag && ft_freeaddrinof() , gai_strerror(flag));
        save_addr.push_back(res);
    }
}

void Parse_Config::push_back_data()
{
    push_back_location();
    if(data.error.empty() && data.server.empty() && data.location.empty())
        return;

    if(data.location.empty())
    {
        loc.insert("uri" , "/");
        loc.insert("root" , "www");
        loc.insert("index" , "index.html");
        loc.insert("upload" , "uploads");
        loc.insert("autoindex" , "off");
        data.location.push_back(loc);
    }

    data.server.insert("host" , "127.0.0.1");
    data.server.insert("port" , "8080");
    data.error.insert("404" , "www/errors/400.html");
    if(!data.server.find("body_size"))
        data._body_size = 20048;

    ft_getaddrinfo();

    blocks.push_back(data);
    data.server.clear(); data.error.clear(); data.location.clear();
}

void Parse_Config::push_back_location()
{
    if(loc.empty())
        return;
    {
        loc.insert("root" , "www");
        loc.insert("index" , "index.html");
        loc.insert("upload" , "uploads");
        loc.insert("autoindex" , "off");
    }

    data.location.push_back(loc);
    loc.clear(); loc.clear();
}



void Parse_Config::ParseFile(const char *fl)
{
    string line;
    short _flag;
    string _filename;

    _filename = (!fl) ? "config/default.ini" : fl;
    throwConfigError(_filename.find(".ini") != _filename.size() - 4 , "The file must have the '.ini' extension. Please check the file name.");

    std::ifstream file(_filename.c_str() , ios::out);
    throwConfigError(!file.is_open() , strerror(errno));
    
    _flag = 0;
    _fill();
    for(int i = 0; std::getline(file , line) && strtrim(line , " \t") ; i++)
    {
        limit++;
        if(line.empty()) continue;
        if(count(NAMESOFBLOCKS, NAMESOFBLOCKS + 3, line))
        {
            throwConfigError(!_flag && line.size() != 8, " Missing 'server' section. The configuration must start with the 'server' section.");
            _flag = (int)line.size();
            (this->*funcPush[_flag])();
            continue;
        }
        throwConfigError(!_flag, " Section 'server' does not exist. Please define the 'server' section in the configuration.");
        getKeyValue(line , '=');
        (this->*funcCheck[_flag])();
        key.clear(); value.clear();
        line.clear();func_ptr.clear();
    }
    data.server.insert("port" , "8080");
    (this->*funcPush[8])();
}
