
#include "../../includes/server/server.hpp"
using namespace std;

Server::Server()
{
    flag = 0;
    epfd = epoll_create(1);
}



void set_nonblocking(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL, 0); 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


void Server::CreatServer(vector<addrinfo *> hosts)
{
    int option = 1;
    int fd;
    for (unsigned long i = 0; i < hosts.size(); i++)
    {
        fd = socket(hosts[i]->ai_family, hosts[i]->ai_socktype, hosts[i]->ai_protocol);
        set_nonblocking(fd);
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); 

        if(bind(fd, hosts[i]->ai_addr, hosts[i]->ai_addrlen) == -1 && close(fd))
            continue;
        if(listen(fd, 1024) == -1)
            Config.throwConfigError(1 , "error in listen") ;

        sockfds.push_back(fd);
        ADD_Events(fd, EPOLLIN ,EPOLL_CTL_ADD);
    }
}


bool Server::find(int fd)
{
    vector<int>::iterator it = std::find(sockfds.begin(), sockfds.end(), fd);
    return (it != sockfds.end());
}

Server::~Server()
{
    close(epfd);
};

void Server::ADD_Events(int _fd, EPOLL_EVENTS ev, int op)
{

    epoll_event event;

    event.events = ev;
    event.data.fd = _fd;
    epoll_ctl(epfd, op, _fd, &event);
}

void Server::ChangeMonitor(int fd)
{
    ADD_Events(fd, EPOLLIN, EPOLL_CTL_MOD);
    delete serv[fd];
    serv[fd] = new my_class(fd);
}


void Server::block_request(int fd)
{
    std::cout << "\n\n-------------------------- block request --------------------------\n\n" << std::endl;

    char buffer[4096];

    ssize_t len = recv(fd, buffer, sizeof(buffer), 0);

            if (len <= 0)
            {
                std::cout << "Connection closed" << std::endl;
                close(fd);
                return;
            }

            if (static_cast<size_t>(len) < sizeof(buffer))
                buffer[len] = '\0';

            int reqParser_res = serv[fd]->req.Parse(string(buffer, len));

            if (reqParser_res == 1)
            {
                serv[fd]->resData = serv[fd]->req.getResult();
                if (serv[fd]->resData._client_requesting_continue) // Expect: 100-continue
                {
                    Response res;
                    res.WithHttpVersion("HTTP/1.1")
                        .WithStatus(100)
                        .Generate()
                        .Send(fd);
                    serv[fd]->resData._client_requesting_continue = 0;
                    return; 
                }
                else
                    ADD_Events(fd, EPOLLOUT, EPOLL_CTL_MOD); // Respond to the request
            }
            else if (reqParser_res == 0)    // Continue
            {
                // ======>> wa9ila khass tkoun continue HNA
            }
            else if (reqParser_res == -1)   // Bad Request
            {
                Response::BadRequest(fd, serv[fd]->resData);
                close(fd);
            }
            else if (reqParser_res == -2)   // Internal Server Error
            {
                Response::InternalServerError(fd, serv[fd]->resData);
                close(fd);
            }
            else if (reqParser_res == -3)   // Unsupported Feature
            {
                Response::NotImplemented(fd, serv[fd]->resData);
                close(fd);
            }
            else if (reqParser_res == -4)   // Payload too large
            {
                Response::Http413(fd);
                close(fd);
            }
}



void Server::block_respond(int fd)
{
    std::cout << "\n\n++++++++++++++++++++++++ block Response ++++++++++++++++++++++++\n\n" << std::endl;

    Method::Type method = serv[fd]->resData._method;
            switch (method)
            {
            case Method::GET:
            {
                if (serv[fd]->res.Serve(fd, serv[fd]->resData))
                    ChangeMonitor(fd);
                break;
            }
            case Method::POST:
            {
                if (serv[fd]->res.Post(fd, serv[fd]->resData))
                    ChangeMonitor(fd);
                else
                {
                    std::cout << "Post failed" << std::endl;
                    exit(1);
                }
                break;
            }
            case Method::DELETE:
            {
                if (serv[fd]->res.Delete(fd, serv[fd]->resData))
                    ChangeMonitor(fd);
                break;
            }
            default:
                break;
            }
}


void Server::ft_accept(int fd)
{
    std::cout << "\n\n============================ block connection ============================\n\n" << std::endl;
    fd = accept(fd, NULL, NULL);
    set_nonblocking(fd);
    serv[fd] = new my_class(fd);
    ADD_Events(fd , EPOLLIN,EPOLL_CTL_ADD);
}




void Server::ForEachEvents(epoll_event *events, int n_events)
{
    int fd;

    for (int i = 0; i < n_events; i++)
    {
        fd = events[i].data.fd;

        if (find(fd))
            ft_accept(fd);
        else if (events[i].events & EPOLLIN)
            block_request(fd);
        else if (events[i].events & EPOLLOUT)
          block_respond(fd);
    }
}



void Server::CreatMultiplexing()
{
    epoll_event events[1024];
    std::map<int, my_class *>::iterator it;
    int n_events;
    it = serv.begin();

    while (true)
    {
        n_events = epoll_wait(epfd, events, 1024, 0);
        ForEachEvents(events, n_events);

        it = (it != serv.end() && it->second->check()) ? it++ : serv.begin();
    }
}
