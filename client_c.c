#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int	main()
{
    int network_sock;
    network_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);
    server.sin_addr.s_addr = INADDR_ANY;
    int status = connect(network_sock, (struct sockaddr *) &server, sizeof(server));
    if (status == -1)
    {
        printf("something went wrong\n");
    }
    char server_resp[256];
    recv(network_sock, &server_resp, sizeof(server_resp), 0);
    printf("%s\n", server_resp);
    close(network_sock);
	return (0);
}
