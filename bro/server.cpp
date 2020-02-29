#include "server.h"
int fd_to[2] = {0}, fd_from[2] = {0};

void	start_subprogram_init(void) __attribute__ ((constructor));

struct get_addr {
    get_addr(int it_fd): fd(it_fd) {}
    bool operator () (std::pair<int, char *> const &slave)
    {
        return (slave.first == fd);
    }
    int fd;
};

int	print_usage(void)
{
	fprintf(stderr, "Usage: tcp_client hostname port\n");
	return (1);
}

int	print_error(const char *err)
{
	fprintf(stderr, err, GETSOCKETERRNO());
	return (1);
}

void	start_subprogram_init(void)
{
	pid_t child;
	int status;

    std::cout << "configuration before main() started..." << std::endl;
	pipe(fd_to);
	pipe(fd_from);

	child = fork();
	if (child < 0)
	{
		std::cerr << "Internal error in init: fork() failed" << std::endl;
		exit(1);
	}
	else if (child == 0)
	{
		char fd_a[20];
		char fd_b[20];
        snprintf(fd_a, 20, "%d", fd_to[0]);
		snprintf(fd_b, 20, "%d", fd_from[1]);
		close(fd_to[1]);
		close(fd_from[0]);
		char *python[] = {"daemon.py", fd_a, fd_b, NULL};
		execvp("./daemon.py", python);
        std::cerr << "Error forking daemon...\n" << std::flush;
        exit(1);
	}
    std::cout << "fd_to[1] " << fd_to[1] << std::endl;
    std::cout << "fd_from[0] " << fd_from[0] << std::endl;
	close(fd_to[0]);
	close(fd_from[1]);
	printf("forked a daemon...\n");
}

bool    start_subprogram(char *mess)
{
    int read_b = 0;
	char result[2] = "";

    std::cout << "trying to communicate with daemon...\n";
     std::cout << "fd_to[1] " << fd_to[1] << std::endl;
     std::cout << "fd_from[0] " << fd_from[0] << std::endl;
    std::cout << mess << " bytes to write: " << strlen(mess) << std::endl;
    int bytes = write(fd_to[1], mess, strlen(mess));
    std::cout << "wrote " << bytes << std::endl;
    std::cout << "fetching response from daemon...\n";
    read_b = read(fd_from[0], result, 1);
	//while((read_b = read(fd_from[0], result, 1)) != 0) {}
	std::cout << "daemon said: " << result << std::endl;
	return (true);
}

int	init_connection(SOCKET sock_listen)
{
	fd_set socks;
	my_slaves Slaves;
    char *slave_addr = NULL;
	FD_ZERO(&socks);
	FD_SET(sock_listen, &socks);
	SOCKET max_sock = sock_listen;
	printf("Waiting for connections...\n");
	while (1)
	{
		fd_set reads;
		reads = socks;
		if (select(max_sock + 1, &reads, 0, 0, 0) < 0)
			return (print_error("select() failed. (%d)\n"));
		SOCKET i;
		for (int i = 1; i <= max_sock; ++i)
		{
			if (FD_ISSET(i, &reads))
			{
				my_slaves::const_iterator it = std::find_if(Slaves.begin(), Slaves.end(), get_addr(i));
                if (it != Slaves.end())
                    slave_addr = it->second;
				if (i == sock_listen)
				{
					struct sockaddr_storage client_addr;
					socklen_t client_len = sizeof(client_addr);
					SOCKET sock_client = accept(sock_listen, (struct sockaddr *) &client_addr, &client_len);
					if (!ISVALIDSOCKET(sock_client))
						return (print_error("accept() failed. (%d)\n"));
					FD_SET(sock_client, &socks);
					max_sock = (sock_client > max_sock) ? sock_client : max_sock;
					char address_buff[100];
					getnameinfo((struct sockaddr *) &client_addr, client_len, address_buff, sizeof(address_buff), 0, 0, NI_NUMERICHOST);
					printf("New connection from %s\n", address_buff);
                    Slaves.push_back(std::make_pair(sock_client, address_buff));
				}
				else
				{
					printf("starting subprogram...\n");
					start_subprogram("42");
                    std::cout << "clearing socks" << std::endl;
					FD_CLR(i, &socks);
				}
			}
		}
	}
	CLOSESOCKET(sock_listen);
	return (0);
}

int	main(int argc, char **argv)
{
	printf("Configuring local address...\n");
	struct addrinfo hints;
	struct addrinfo *bindaddr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	struct sockaddr_in my_addr;
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //start_subprogram_init();
	getaddrinfo("192.168.20.11", "12345", &hints, &bindaddr);
	SOCKET sock_listen;
	sock_listen = socket(bindaddr->ai_family, bindaddr->ai_socktype, bindaddr->ai_protocol);
	if (!ISVALIDSOCKET(sock_listen))
		return (print_error("socket() failed. (%d)\n"));
	int opt = 1;
	if ((setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) < 0)
		std::cerr << "Internal error: setsockopt() failed\n";
	printf("Socket is UP\n\n");
    if (bind(sock_listen, bindaddr->ai_addr, bindaddr->ai_addrlen))
		return (print_error("bind() failed. (%d)\n"));
	freeaddrinfo(bindaddr);
	if (listen(sock_listen, 11) < 0)
		return (print_error("listen() failed. (%d)\n"));
	while (1)
	{
		printf("Listening...\n\n");
		if (!init_connection(sock_listen))
			printf("Connection finished\n");
	}
	CLOSESOCKET(sock_listen);
	return (0);
}