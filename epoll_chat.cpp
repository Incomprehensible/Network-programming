// ANALYZE AND REALIZE WTF
// written by Григорий Васильев 2 years ago

#include <set>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define MAX_EVENTS 32

int set_nonblock(int socketfd)
{
    int flags;
#if defined(O_NONBLOCK)
    flags = fcntl(socketfd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "fcntl failed (F_GETFL)\n";
        return 0;
    }

    flags |= O_NONBLOCK;
    int s = fcntl(socketfd, F_SETFL, flags);
    if (s == -1)
    {
        std::cerr << "fcntl failed (F_SETFL)\n";
        return 0;
    }

    return 1;

#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

int writeip(sockaddr_in &client, char* buffer)
{
    int a = (client.sin_addr.s_addr&0xFF);
    int b = (client.sin_addr.s_addr&0xFF00)>>8;
    int c = (client.sin_addr.s_addr&0xFF0000)>>16;
    int d = (client.sin_addr.s_addr&0xFF000000)>>24;
    int n = std::sprintf(buffer, "[%d.%d.%d.%d]: ", a, b, c, d);
    return n;
}

int main() {

    std::set<int> sockets;

    int server_fd, client_fd, nfds, epollfd, opt = 1;

    ssize_t res, rsize;

    char recv_buffer[1024] = {0};
    char send_buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0)
    {
        std::cerr << "socket failed\n";
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "setsockopt\n";
        exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5533);
    socklen_t addrlen = sizeof(address);
    if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "bind failed\n";
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "listen\n";
        exit(EXIT_FAILURE);
    }

    epollfd = epoll_create1(0);

    if (epollfd == -1) {
        std::cerr << "epoll_create1\n";
        exit(EXIT_FAILURE);
    }

    epoll_event event{}, events[MAX_EVENTS];

    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "epoll_ctl: listen_sock\n";
        exit(EXIT_FAILURE);
    }

    while(recv_buffer[0] != '#' && recv_buffer[1] != '0'){
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            std::cerr << "epoll_wait\n";
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {

                client_fd = accept(server_fd, (sockaddr *)&address, &addrlen);

                if (client_fd == -1) {
                    std::cerr << "accept\n";
                    exit(EXIT_FAILURE);
                }

                set_nonblock(client_fd);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;

                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    std::cerr << "epoll_ctl: client_fd\n";
                    exit(EXIT_FAILURE);
                }


                sockets.insert(client_fd);
                res = getpeername(client_fd, (sockaddr *)&address, &addrlen);
                if(res != -1) {
                    res = writeip(address, send_buffer);
                    std::strcat(send_buffer, "Connected!\n");
                    for (auto socket: sockets) {
                        //if(socket != events[i].data.fd) {
                            send(socket, send_buffer, static_cast<size_t>(res + 11), MSG_NOSIGNAL);
                        //}
                    }
                }

            } else {
                rsize = recv(events[i].data.fd, recv_buffer, 1024, MSG_NOSIGNAL);

                if(rsize == 0 && errno != EAGAIN) {
                    sockets.erase(events[i].data.fd);
                    res = getpeername(events[i].data.fd, (sockaddr *)&address, &addrlen);
                    if(res != -1) {
                        res = writeip(address, send_buffer);
                        std::strcat(send_buffer, "Disconnected!\n");
                        for (auto socket: sockets) {
                            //if(socket != events[i].data.fd) {
                                send(socket, send_buffer, static_cast<size_t>(res + 14), MSG_NOSIGNAL);
                            //}
                        }
                    }
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);

                } else if(rsize > 0) {
                    res = getpeername(events[i].data.fd, (sockaddr *)&address, &addrlen);
                    if(res != -1) {
                        rsize += writeip(address, send_buffer);
                        std::strcat(send_buffer, recv_buffer);
                        for (auto socket: sockets) {
                            //if(socket != events[i].data.fd) {
                            send(socket, send_buffer, static_cast<size_t>(rsize), MSG_NOSIGNAL);
                            //}
                        }
                    }
                }

            }
        }
    }
    return 0;
}