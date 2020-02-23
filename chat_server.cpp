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

typedef std::vector<std::pair<int, char *> > my_slaves;

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

    if (bind(Master, (struct sockaddr *)&SockAddr, sizeof(SockAddr)) < 0)
    {
        std::cerr << "ERROR binding socket" << std::endl;
        exit(1);
    }
    set_nonblock(Master);
    listen(Master, SOMAXCONN);
    std::cout << "Listening..." << std::endl;

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
            std::cout << "Event detected!..." << std::endl;
            if (KEvent.ident == Master)
            {
                std::cout << "Event on MASTER..." << std::endl;
                struct sockaddr_in SlaveAddr;
                //bzero(&SlaveAddr, sizeof(SlaveAddr));
                SlaveAddr.sin_family = AF_INET;
                socklen_t addr_size = sizeof(struct sockaddr_in);
                int Slave = accept(Master, (struct sockaddr *)&SlaveAddr, &addr_size);
                set_nonblock(Slave);
                Slaves.push_back(std::make_pair(Slave, inet_ntoa(SlaveAddr.sin_addr)));
                bzero(&KEvent, sizeof(KEvent));
                EV_SET(&KEvent, Slave, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
                kevent(KQueue, &KEvent, 1, NULL, 0, NULL);
                std::cout << "New client added..." << std::endl;
            }
            else
            {
                std::cout << "Event on client side..." << std::endl;
                static char Buff[1024];
                int bytes = recv(KEvent.ident, Buff, 1024, SO_NOSIGPIPE);
                char *slave_addr;
                my_slaves::const_iterator i = std::find_if(Slaves.begin(), Slaves.end(), get_addr(KEvent.ident));
                if (i != Slaves.end())
                    slave_addr = i->second;
                std::cout << "Event on " << slave_addr << std::endl;
                if (bytes <= 0)
                {
                    std::cout << "Client closed connection..." << std::endl;
                    for (auto Slave: Slaves)
                    {
                        if (Slave.first != KEvent.ident)
                        {
                            //construct message properly here
                            std::cout << "Informing clients about client leaving chat..." << std::endl;
                            send(Slave.first, slave_addr, bytes, SO_NOSIGPIPE);
                            send(Slave.first, " left the chat!\n ", bytes, SO_NOSIGPIPE);
                        }
                    }
                    Slaves.erase(i);
                    shutdown(KEvent.ident, SHUT_RDWR);
                    close(KEvent.ident);
                    // inform all other slaves about this slave disconnecting
                    //  for(auto [First, Second] : a)
                    //  {
                    //     std::cout << First
                    //     << "-->"
                    //     << Second
                    //     << "\n";
                    // }
                    for(auto p : Slaves) { // a is a std::vector<std::pair<int, int>>
                            std::cout << p.first << "\n";
                        }
                }
                else
                {
                    std::cout << "New message from client " << slave_addr << "..." << std::endl;
                    std::cout << "New message: " << Buff << std::endl;
                    for (auto Slave: Slaves)
                    {
                        if (Slave.first != KEvent.ident)
                        {
                            std::cout << "Informing clients of this message..." << std::endl;
                            //construct message properly here
                            send(Slave.first, slave_addr, bytes, SO_NOSIGPIPE);
                            send(Slave.first, "| ", bytes, SO_NOSIGPIPE);
                            send(Slave.first, Buff, bytes, SO_NOSIGPIPE);
                        }
                    }
                    std::cout << "Coming back..." << std::endl;
                }
            }
        }
    }
}