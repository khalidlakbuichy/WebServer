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

void  Parse_Config::throwConfigError(bool expr , const char *str)
{
    if(expr) throw std::runtime_error(string("webserve : " + string(str)));
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



ConfigLoader Parse_Config::operator()(const char *_str)
{
    vector<ConfigLoader>::iterator it = blocks.begin();
    string str = _str;
    
    try
    {
    getKeyValue(str , ':');
    }
    catch(const std::exception& e) 
    {
        std::cout << e.what() << std::endl;
    }

    while(it != blocks.end())
    {
        if(it->server.find("host", key.data()) && it->server.find("port", value.data()))
            return(*it);
        it++;
    }
    
    return it[0];
}

Parse_Config::~Parse_Config()
{
    func_ptr.clear(); key.clear();
    value.clear(); filename.clear();
    blocks.clear();
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
        throwConfigError(!CheckCharacter(str[i]) , "webserv : Invalid String");
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
        throwConfigError(!flag && !count(&VALUES[0], &VALUES[size], word), "invalid value1" );
        data.push_back(word);
    }
    _errno = (flag == 1 && i != 1) + (flag == 3  && i != 2); 
    throwConfigError(_errno , "invalid value2" );
}


void Parse_Config::CheckBlockErrors()
{
    throwConfigError((key.size() != 3 || key[0] < '3' || key[0] > '5') , "must btw 300 to 599");
    func_ptr.push_back(isdigit); CheckString(key);

    throwConfigError(data.error.find(key) , "dupblicate key error");
    CheckArrayOfValue(data.error[key] , 1);
}


void Parse_Config::CheckBlockLocation()
{
    flag = sizeof(KEYOFLOCATION) / sizeof(KEYOFLOCATION[0]);
    throwConfigError(!count(KEYOFLOCATION , &KEYOFLOCATION[flag] , key) ,  "location this key is not exist");

    flag = (key[0] == 'c') * 2 + !(key[0] == 'm');

    throwConfigError(key[0] != 'c' && loc.find(key) , "dupblicate key location");
    CheckArrayOfValue(loc[key] , flag);

    _errno = !loc.find("uri");
    _errno += key[0] == 'a' && (value != "on" && value != "off");
    throwConfigError(_errno, "ERROR IN LOCATION");
}


void Parse_Config::CheckInfoServer()
{
    flag  = sizeof(KEYOFSERVER) / sizeof(KEYOFSERVER[0]);
    throwConfigError(!count(KEYOFSERVER , &KEYOFSERVER[flag] , key) ,  "server this key is not exist");

    checks = "ps";
    flag = (checks.find(key[0]) != checks.npos ) ? 2 : 1 ;

    throwConfigError(data.server.find(key) , " dupblicate key server");
    CheckArrayOfValue(data.server[key] , flag);

    if(key[0] == 'b')
    {
        value = data.server[key.data()];
        size_t pos = value.find_first_not_of("0123456789");
        throwConfigError(pos != value.size() - 1 || value[pos] != 'k'  , "Error in body size");
    }
}


void Parse_Config::getKeyValue(string &line , char set)
{
    size_t c;
    c = line.find(set);
    throwConfigError(c == line.npos , "Error line");

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
        throwConfigError(flag < 1024 || flag > 65535 , "invalid range");

        flag =  getaddrinfo(host, it[i].data(), NULL, &res);
        throwConfigError(flag , gai_strerror(flag));
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
        loc.insert("methods" , "GET");
        loc.insert("root" , "www");
        loc.insert("index" , "index.html");
        loc.insert("upload" , "uploads");
        loc.insert("autoindex" , "off");
        data.location.push_back(loc);
    }

    data.error.insert("404" , "www/errors/400.html");
    data.server.insert("host" , "127.0.0.1");
    data.server.insert("port" , "8080");
    data.server.insert("body_size" , "4096K");
    data.server.insert("server_name" , "localhost");

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



void Parse_Config::ParseFile(const char *filename)
{
    std::ifstream file(filename , ios::out);
    string line;
    short _flag;
    
    _flag = 0;
    _fill();
    for(int i = 0; std::getline(file , line) && strtrim(line , " \t") ; i++)
    {
        if(line.empty()) continue;
        if(count(NAMESOFBLOCKS, NAMESOFBLOCKS + 3, line))
        {
            _flag = (int)line.size();
            (this->*funcPush[_flag])();
            continue;
        }
        throwConfigError(!_flag  , "webserve : Error");
        
        getKeyValue(line , '=');
        (this->*funcCheck[_flag])();
        key.clear(); value.clear();
        line.clear();func_ptr.clear();
    }
    (this->*funcPush[8])();
}
