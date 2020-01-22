#include "tcp_client.h"

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

int	init_connection(SOCKET remote_socket)
{
	while (1)
	{
		fd_set read_socks;
		FD_ZERO(&read_socks);
		FD_SET(remote_socket, &read_socks);
	#if !defined(_WIN32)
        FD_SET(0, &read_socks);
	#endif
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		if (select(remote_socket + 1, &read_socks, 0, 0, &timeout) < 0)
			return (print_error("select() failed. (%d)\n"));
		if (FD_ISSET(remote_socket, &read_socks))
		{
			char buf[4096];
			int bytes_received = recv(remote_socket, buf, 4096, 0);
			if (bytes_received < 1)
			{
				printf("Connection closed by peer.\n");
				break ;
			}
			printf("Received (%d bytes): %.*s", bytes_received, bytes_received, buf);
		}
	#if defined(_WIN32)
        if(_kbhit())
	#else
		if (FD_ISSET(0, &read_socks))
	#endif
		{
			char read_consl[4096];
			if (!fgets(read_consl, 4096, stdin))
				break ;
			printf("Preparing to send data...\n");
			int bytes_sent = send(remote_socket, read_consl, strlen(read_consl), 0);
			printf("Sent %d bytes.\n", bytes_sent);
		}
	}
	printf("Closing socket...\n");
    CLOSESOCKET(remote_socket);
#if defined(_WIN32)
    WSACleanup();
#endif
	return (0);
}

int	main(int argc, char **argv)
{
	if (argc < 3)
		return (print_usage());
#if defined(_WIN32)
	WSADATA d;
	if (WSAStartup(MAKEWORD(2,2), &d))
	{
		fprintf(stderr, "Failed to initialize.\n");
		return (1);
	}
#endif
	struct addrinfo hints;
	struct addrinfo *peer_addr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(argv[1], argv[2], &hints, &peer_addr))
		return (print_error("getaddrinfo() failed. (%d)\n"));
	char addr_name[100];
	char service_name[100];
	getnameinfo(peer_addr->ai_addr, peer_addr->ai_addrlen, addr_name, sizeof(addr_name), service_name, sizeof(service_name), NI_NUMERICHOST);
	printf("Host found: %s : %s\n", addr_name, service_name);
	printf("Creating socket...\n");
	SOCKET socket_peer;
	socket_peer = socket(peer_addr->ai_family, peer_addr->ai_socktype, peer_addr->ai_protocol);
	if (!ISVALIDSOCKET(socket_peer))
		return (print_error("socket() failed. (%d)\n"));
	if (connect(socket_peer, peer_addr->ai_addr, peer_addr->ai_addrlen))
		return (print_error("connect() failed. (%d)\n"));
	freeaddrinfo(peer_addr);
	printf("Successfully connected.\n");
	printf("To send data, enter text followed by enter.\n");
	if (!init_connection(socket_peer))
		printf("Connection closed.\n");
	return (0);
}