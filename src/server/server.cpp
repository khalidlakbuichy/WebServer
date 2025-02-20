
#include "../../includes/server/server.hpp"
using namespace std;

Server::Server()
{
    flag = 0;
    epfd = epoll_create(1);
}

void Server::CreatServer(vector<addrinfo *> hosts)
{
    int option = 1;
    int err;
    int fd;
    for (unsigned long i = 0; i < hosts.size(); i++)
    {
        fd = socket(hosts[i]->ai_family, hosts[i]->ai_socktype, hosts[i]->ai_protocol);
        Config.throwConfigError(fd < 0, strerror(errno));

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        err = bind(fd, hosts[i]->ai_addr, hosts[i]->ai_addrlen);
        Config.throwConfigError(err < 0, strerror(errno));

        err = listen(fd, 1024);
        Config.throwConfigError(err < 0, strerror(errno));

        sockfds.push_back(fd);
        ADD_Events(fd, EPOLLIN, EPOLL_CTL_ADD);


        {
        struct  sockaddr_in *ipv4;
        char local_ip[16];
    
        ipv4 = (struct sockaddr_in *)hosts[i]->ai_addr;
        inet_ntop(AF_INET,&(ipv4->sin_addr) , local_ip, sizeof(local_ip));
        std::cout << "Server is listening on " <<  local_ip << ":" <<   ntohs(ipv4->sin_port)  << "..." << std::endl;
        }

        freeaddrinfo(hosts[i]);
    }
}

bool Server::find(int fd)
{
    vector<int>::iterator it = std::find(sockfds.begin(), sockfds.end(), fd);
    return (it != sockfds.end());
}

Server::~Server()
{
    std::map<int, my_class *>::iterator it = serv.begin();

    while(it != serv.end())
    {
        delete it->second;
        it++;
    }
    close(epfd);
};

void Server::ADD_Events(int _fd, EPOLL_EVENTS ev, int op)
{

    epoll_event event;

    event.events = ev | EPOLLERR | EPOLLHUP;
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

    char buffer[16384];

    ssize_t len = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);
    buffer[len] = 0;


    if (len <= 0)
    {
        close(fd);
        return;
    }
    if (static_cast<size_t>(len) < sizeof(buffer))
        buffer[len] = '\0';

    int reqParser_res = serv[fd]->req.Parse(string(buffer, len));
    serv[fd]->resData = serv[fd]->req.getResult();

    if (serv[fd]->resData._client_requesting_continue) // Expect: 100-continue
    {
        const char *continue_response = "HTTP/1.1 100 Continue\r\n\r\n";
        send(fd, continue_response, strlen(continue_response), MSG_NOSIGNAL | MSG_DONTWAIT);
        serv[fd]->resData._client_requesting_continue = 0;
    }

    if (reqParser_res == 1)
    {
        ADD_Events(fd, EPOLLOUT, EPOLL_CTL_MOD);
    }
    else if (reqParser_res == 0) // Continue
    {
        // ======>> wa9ila khass tkoun continue HNA
    }
    else if (reqParser_res == -1) // Bad Request
    {
        Response::BadRequest(fd, serv[fd]->resData);
        close(fd);
    }
    else if (reqParser_res == -2) // Internal Server Error
    {
        Response::InternalServerError(fd, serv[fd]->resData);
        close(fd);
    }
    else if (reqParser_res == -3) // Unsupported Feature
    {
        Response::NotImplemented(fd, serv[fd]->resData);
        close(fd);
    }
    else if (reqParser_res == -4) // Payload too large
    {
        Response::Http413(fd);
        close(fd);
    }
}

void Server::block_respond(int fd)
{

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
        int res = serv[fd]->res.Post(fd, serv[fd]->resData);
        if (res == 1)
            ChangeMonitor(fd);
        else if (res == -1)
        {
            remove(serv[fd]->resData._tmp_file_name.c_str());
            Response::InternalServerError(fd, serv[fd]->resData);
            close(fd);
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
    fd = accept(fd, NULL, NULL);
    serv[fd] = new my_class(fd);
    ADD_Events(fd, EPOLLIN, EPOLL_CTL_ADD);
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
        else
            close(fd);
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
