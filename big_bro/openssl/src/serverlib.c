#include "../includes/config.h"

extern network_io neuronka;
extern int code;

int connect_as_client(); // to clientlib

// IMPLEMENT
// code as a mutex

//load_CA();

int feed_to_network(int i)
{
	#define NO 0
	#define OK 1

	int read_b = 0;
	char result[2] = "";
    char *mess = "";

    if (i == 21)
        mess = "rec";
    else
        mess = "enc";
    int bytes = write(neuronka.into, mess, strlen(mess));
    printf("fetching response from client...\n");
    read_b = read(neuronka.out, result, 1);
    printf("Bytes read: %d\n", read_b);
	printf("Client said: %s\n", result);
    if (!read_b || result[0] == '0')
	    return (NO);
    if (result[0] == '0')
	    return (NO);
    return (OK);
}

inline void clear_face(void)
{
	system(DELFACE);
	system(DELMAP);
}

static void is_request(char *buf, size_t len)
{
	if (len < sizeof(struct request))
	{
		code = ABORT; // mutex here
		return;
	}
	parse_packet(buf, len);
	if (code == NEW_DATA)
	{
		// int fd = open(FACE_PATH, O_RDONLY);
		// if (fd < 0)
		// {
		// 	perror("Error opening a file\n");
		// 	exit(1);
		// }
		if (!feed_to_network(42))
		{
			code = DATA_ERROR;
			clear_face(); // clears picture and depth map
		}
	}
}

static void wait_for_data(int sock, SSL_CTX *ctx)
{
	while(1)
	{
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);
        SSL *ssl;

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);
// ask for client certificate - to implement
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        }
        else
		{
			char buff[1024] = {};
			len = SSL_read(ssl, buff, sizeof(buff));
			is_request(buff, len);
			if (!(code & NEW_DATA) && !(code & DATA_ERROR))
				goto clean_up;
			const char replyOk[] = "ok";
			const char replyNope[] = "nope";

			while (len > 0 && code != NEW_DATA) // data valid, but neironka could not understand data
			{
				SSL_write(ssl, replyNope, strlen(replyNope));
				len = SSL_read(ssl, buff, sizeof(buff));
				if (len > 0)
					is_request(buff, len);;
			}
            if (code == NEW_DATA)
				SSL_write(ssl, replyOk, strlen(replyOk));
			clean_up:
			{
				SSL_shutdown(ssl);
    			SSL_free(ssl);
    			close(client);
				if (code == NEW_DATA) break;
				//free_meta_data();
			}
		}
    }
}

int start_server(SSL_CTX *ctx)
{
	int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
		perror("Unable to create socket");
		exit(EXIT_FAILURE);
    }
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Unable to bind");
		exit(EXIT_FAILURE);
    }
    if (listen(s, 1) < 0) {
		perror("Unable to listen");
		exit(EXIT_FAILURE);
    }
	wait_for_data(s, ctx);
    return s;
}

// load CA 
inline static void configure_server_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, CLIENT_CRT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, CLIENT_KEY, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }
}

SSL_CTX *create_server_context(void)
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }
	configure_server_context(ctx);

    return ctx;
}

void cleanup_openssl(void)
{
    EVP_cleanup();
}