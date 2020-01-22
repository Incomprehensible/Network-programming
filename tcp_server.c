#include "tcp_server.h"

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

int	init_connection(SOCKET sock_listen)
{
	fd_set socks;
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
		for (i = 1; i <= max_sock; ++i)
		{
			if (FD_ISSET(i, &reads))
			{
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
				}
				else
				{
					char data[1024];
					int bytes_received = recv(i, data, sizeof(data), 0);
					if (bytes_received < 1)
					{
                        FD_CLR(i, &socks);
                        CLOSESOCKET(i);
                        continue ;
                    }
					SOCKET j;
					for (j = 0; j <= max_sock; ++j)
					{
						if (FD_ISSET(j, &socks) && j != i && j != sock_listen)
							send(j, data, bytes_received, 0);
					}
				}
			}
		}
	}
	CLOSESOCKET(sock_listen);
#if defined(_WIN32)
    WSACleanup();
#endif
	return (0);
}

int	main(int argc, char **argv)
{
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif
	printf("Configuring local address...\n");
	struct addrinfo hints;
	struct addrinfo *bindaddr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(0, "8080", &hints, &bindaddr);
	SOCKET sock_listen;
	sock_listen = socket(bindaddr->ai_family, bindaddr->ai_socktype, bindaddr->ai_protocol);
	if (!ISVALIDSOCKET(sock_listen))
		return (print_error("socket() failed. (%d)\n"));
	printf("Socket is UP\n\n");
	if (bind(sock_listen, bindaddr->ai_addr, bindaddr->ai_addrlen))
		return (print_error("bind() failed. (%d)\n"));
	freeaddrinfo(bindaddr);
	printf("Listening...\n\n");
	if (listen(sock_listen, 11) < 0)
		return (print_error("listen() failed. (%d)\n"));
	if (!init_connection(sock_listen))
		printf("Connection finished\n");
	return (0);
}