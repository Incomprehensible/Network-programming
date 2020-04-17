#include "../includes/config.h"

int stop = 0;
int code = 0; // add mutex
USR_DATA User;
network_io neuronka;

static void start_subprogram_init(void)
{
	int fd_to[2], fd_from[2];
	pid_t child;
	int status;

	pipe(fd_to);
	pipe(fd_from);

	child = fork();
	if (child < 0)
	{
		perror("Internal error in init: fork() failed\n");
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
		char *python[] = {"src/cnn_network.py", fd_a, fd_b, NULL};
        execlp("python3", "python3", "src/cnn_network.py", fd_a, fd_b, NULL);
        perror("Error forking client...\n");
        exit(1);
	}
	neuronka.into = fd_to[1];
	neuronka.out = fd_from[0];
	close(fd_to[0]);
	close(fd_from[1]);
}

void system_init(void)
{
	User.SID = 0;
	User.status = 0;
	//drivers_configure();
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	start_subprogram_init();
}


int main()
{
	system_init();
	int sock;
    SSL_CTX *ctx;
	ctx = create_server_context();
	sock = start_server(ctx); // blocks until first data is received
	// here spawn a thread
	// for main code
	// main continues to monitor connections
	// add signal handler here
	while (!stop)
	{


	}
	
	close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
	return (0);
}