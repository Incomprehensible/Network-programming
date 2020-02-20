#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <set>
#include <tuple>
#include <vector>

typedef std::vector<std::pair<int, char *>> my_slaves;

struct get_addr {
    get_addr(int it_fd): fd(it_fd) {}
    bool operator () (std::pair<int, char *> const &slave)
    {
        return (slave.first == fd);
    }
    int fd;
};

int set_nonblock(int fd)
{
    int flags;

#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK | SO_NOSIGPIPE));
#else
    flags = 1;
    return (ioctl(fd, FIOBIO, &flags));
#endif
}


int main()
{
    my_slaves Slaves;
    int Master = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = 0;

    bind(Master, (struct sockaddr *)&SockAddr, sizeof(SockAddr));
    set_nonblock(Master);
    listen(Master, SOMAXCONN);

    int KQueue = kqueue();

    struct kevent KEvent;
    bzero(&KEvent, sizeof(KEvent));
    EV_SET(&KEvent, Master, EVFILT_READ, EV_ADD, 0, 0, 0);
    kevent(KQueue, &KEvent, 1, NULL, 0, NULL);

    while (true)
    {
        bzero(&KEvent, sizeof(KEvent));
        kevent(KQueue, NULL, 0, &KEvent, 1, NULL);
        if (KEvent.filter == EVFILT_READ)
        {
            if (KEvent.ident == Master)
            {
                struct sockaddr_in SlaveAddr;
                SlaveAddr.sin_family = AF_INET;
                int Slave = accept(Master, (struct sockaddr *)&SlaveAddr, sizeof(SlaveAddr));
                set_nonblock(Slave);
                Slaves.emplace_back(Slave, inet_ntoa(SlaveAddr.sin_addr));
                bzero(&KEvent, sizeof(KEvent));
                EV_SET(&KEvent, Slave, EVFILT_READ | EVFILT_WRITE, EV_ADD, 0, 0, 0);
                kevent(KQueue, &KEvent, 1, NULL, 0, NULL);
            }
            else
            {
                static char Buff[1024];
                int bytes = recv(KEvent.ident, Buff, 1024, SO_NOSIGPIPE);
                char *slave_addr;
                my_slaves::iterator i = std::find_if(Slaves.begin(), Slaves.end(), get_addr(KEvent.ident));
                if (i != Slaves.end())
                    slave_addr = i->second;
                if (bytes <= 0)
                {
                    close(KEvent.ident);
                    // inform all other slaves about this slave disconnecting
                    //  for(auto [First, Second] : a)
                    //  {
                    //     std::cout << First
                    //     << "-->"
                    //     << Second
                    //     << "\n";
                    // }

                    // for(auto p : a) { // a is a std::vector<std::pair<int, int>>
                    //     for(auto value : {p.first, p.second}) {
                    //         std::cout << value << "\n";
                    //     }
                    // }
                }
                else
                {
                    for (auto Slave: Slaves)
                    {
                        if (Slave.first != KEvent.ident)
                        {
                            //construct message properly here
                            send(Slave.first, slave_addr, bytes, SO_NOSIGPIPE);
                            send(Slave.first, "| ", bytes, SO_NOSIGPIPE);
                            send(Slave.first, Buff, bytes, SO_NOSIGPIPE);
                        }
                    }
                }
            }
        }
    }
}