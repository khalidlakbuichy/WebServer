#include "../../includes/config/Config_Loader.hpp"


ConfigLoader::ConfigLoader() {}

ConfigLoader::~ConfigLoader() 
{
    server.clear();
    error.clear();
    location.clear();
}



std::string ConfigLoader::operator[](const char *str)
{
    if(server.find(str))
        return(server[str]);
    return(error[str]);
}


t_map findd(string path , vector<t_map> &loc)
{

    vector<t_map>::iterator it = loc.begin();

    while(it != loc.end())
    {
        
        if(it->find("uri" , path.data()))
            return *it;
        it++;
    }
    
    std::cout << "+++++++++++" << path << std::endl;
    std::cout << "-----------" << path.erase(path.find_last_of('/') , path.size() - path.find_last_of('/')) << std::endl;
    
    if(path.empty())
    {
        std::cout << "(())" << std::endl;
        path = "/";

    }

    return(findd(path , loc));
}

t_map ConfigLoader::operator()(const char *path)
{
    t_map loc = findd(string(path) , location);
    return(loc);
}



int ConfigLoader::empty()
{
    return((error.empty() || location.empty() || server.empty()));
}






























///  functions  helper

void  throwConfigError(bool expr , const char *str)
{
    if(expr) throw std::runtime_error(string("webserve : " + string(str)));
}


int strtrim(std::string &str , const char *sharset)
{
    size_t fpos = str.find_first_not_of(sharset);
    size_t lpos = str.find_last_not_of(sharset);
    if(fpos == str.npos )
        fpos = 0;

    str = str.substr(fpos , lpos - fpos + 1);
    return 1;
}
