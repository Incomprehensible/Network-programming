#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int	main()
{
    int server_sock;
    char server_resp[] = "You sent awesome message\n";
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);
    server.sin_addr.s_addr = INADDR_ANY;
    bind(server_sock, (struct sockaddr *) &server, sizeof(server));
    listen(server_sock, 5);
    int client_sock;
    client_sock = accept(server_sock, NULL, NULL);
    send(client_sock, server_resp, sizeof(server_resp), 0);
    printf("%s\n", server_resp);
    close(server_sock);
    return (0);
}