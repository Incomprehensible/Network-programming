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

#define MAX_FD 150

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
# ifdef SO_NOSIGPIPE
# define CEPH_USE_SO_NOSIGPIPE
# else
# error "Cannot block SIGPIPE!"
# endif
#endif

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
    char *slave_addr;
    int Master, _new, last_index = 0;
    int opt = 1;
    if ((Master = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0)
    {
        std::cerr << "ERROR: socket() failed\n";
        exit(EXIT_FAILURE);
    }
    if (setsockopt(Master, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        std::cerr << "ERROR: setsockopt() failed\n";
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = 0;

    if (bind(Master, (struct sockaddr *)&SockAddr, sizeof(SockAddr)) < 0)
    {
        std::cerr << "ERROR binding socket\n";
        exit(1);
    }
    set_nonblock(Master);
    if (listen(Master, SOMAXCONN) < 0)
    {
        std::cerr << "ERROR: listen() failed\n";
        exit(1);
    }
    std::cout << "Listening..." << std::endl;

    int KQueue = kqueue();

    struct kevent KEvent[MAX_FD];
    bzero(KEvent, sizeof(*KEvent) * MAX_FD);
    EV_SET(KEvent, Master, EVFILT_READ, EV_ADD, 0, 0, 0);
    kevent(KQueue, KEvent, 1, NULL, 0, NULL);

    while (true)
    {
        bzero(KEvent, sizeof(*KEvent) * MAX_FD);
        _new = kevent(KQueue, NULL, 0, KEvent, MAX_FD, NULL);
        if (_new <= 0) continue;
        for (int i = 0; i < _new; ++i)
        {
            if (KEvent[i].filter == EVFILT_READ)
            {
                std::cout << "Event detected!..." << std::endl;
                if (KEvent[i].ident == Master)
                {
                    std::cout << "Event on MASTER..." << std::endl;
                    struct sockaddr_in SlaveAddr;
                    SlaveAddr.sin_family = AF_INET;
                    socklen_t addr_size = sizeof(struct sockaddr_in);
                    int Slave = accept(Master, (struct sockaddr *)&SlaveAddr, &addr_size);
                    set_nonblock(Slave);
                    slave_addr = inet_ntoa(SlaveAddr.sin_addr);
                    Slaves.push_back(std::make_pair(Slave, slave_addr));
                    bzero(&KEvent[i], sizeof(KEvent));
                    EV_SET(&KEvent[i], Slave, EVFILT_READ, EV_ADD, 0, 0, 0);
                    kevent(KQueue, &KEvent[i], 1, NULL, 0, NULL);
                    std::cout << "New client added..." << std::endl;

                    for (auto slave: Slaves)
                    {
                        if (slave.first != Slave)
                        {
                            //construct message properly here
                            std::cout << "Informing clients about client entering chat..." << std::endl;
                            send(slave.first, slave_addr, strlen(slave_addr), SO_NOSIGPIPE);
                            send(slave.first, " entered the chat!\n", 19, SO_NOSIGPIPE);
                        }
                    }
                }
                else
                {
                    std::cout << "Event on client side..." << std::endl;
                    static char Buff[1024];
                    memset(Buff, 0, sizeof(Buff));
                    int bytes = recv(KEvent[i].ident, Buff, 1024, MSG_NOSIGNAL);
                    int triggered_fd = KEvent[i].ident;
                    my_slaves::const_iterator i = std::find_if(Slaves.begin(), Slaves.end(), get_addr(triggered_fd));
                    if (i != Slaves.end())
                        slave_addr = i->second;
                    std::cout << "Event on " << slave_addr << std::endl;
                    if (bytes <= 0 && errno != EAGAIN)
                    {
                        std::cout << "Client closed connection..." << std::endl;
                        for (auto Slave: Slaves)
                        {
                            if (Slave.first != triggered_fd)
                            {
                                std::cout << "Informing clients about client leaving chat..." << std::endl;
                                send(Slave.first, slave_addr, strlen(slave_addr), SO_NOSIGPIPE);
                                send(Slave.first, " left the chat!\n", 16, SO_NOSIGPIPE);
                            }
                        }
                        Slaves.erase(i);
                        shutdown(triggered_fd, SHUT_RDWR);
                        close(triggered_fd);
                        // inform all other slaves about this slave disconnecting
                        //  for(auto [First, Second] : a)
                        //  {
                        //     std::cout << First
                        //     << "-->"
                        //     << Second
                        //     << "\n";
                        // }
                    }
                    else
                    {
                        std::cout << "New message from client " << slave_addr << "..." << std::endl;
                        std::cout << "New message: " << Buff << std::endl;
                        for (auto Slave: Slaves)
                        {
                            if (Slave.first != triggered_fd)
                            {
                                std::cout << "Informing clients of this message..." << std::endl;
                                send(Slave.first, slave_addr, strlen(slave_addr), SO_NOSIGPIPE);
                                send(Slave.first, "| ", 2, SO_NOSIGPIPE);
                                send(Slave.first, Buff, bytes, SO_NOSIGPIPE);
                            }
                        }
                        std::cout << "Coming back..." << std::endl;
                    }
                }
            }
        }
    }
    return (0);
}