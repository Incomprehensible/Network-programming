#include "../includes/config.h"

#if defined DEBUG
void err(const char *err_msg)
{
	fprintf(stderr, "ssl connection state: %s\n", SSL_state_string_long(ssl));
	fprintf(stderr, "Error: %s\n", ERR_reason_error_string(ERR_get_error()));
	fprintf(stderr, "Error: %s\n", ERR_error_string(ERR_get_error(), NULL));
	ERR_print_errors_fp(stderr);
	perror(err_msg);
	exit(1);
}
#else
void err(const char *err_msg)
{
	perror(err_msg);
	exit(1);
}
#endif

void init_openssl_library(void)
{
	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	//		SSL_library_init();
	//		ERR_load_BIO_strings();
	//		OpenSSL_add_all_algorithms();
}

#if defined DEBUG
int get_mss(void)
{
	int sock;
	struct ifreq ifr;
	const char *interface = "enp4s0";
	#define DEFAULT 1452
	//#define PPPoE_overhead 8

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return DEFAULT;
	memset(&ifr, 0, sizeof(struct ifreq));
	memcpy(ifr.ifr_ifrn.ifrn_name, interface, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFMTU, &ifr) < 0)
	{
		perror("Error getting MTU value, setting default MSS");
		return DEFAULT;
	}
	close(sock);
#if defined PPPoE_overhead
	return (ifr.ifr_ifru.ifru_mtu - 40 - PPPoE_overhead);
#else
	return (ifr.ifr_ifru.ifru_mtu - 40);
#endif
}
#endif

SSL_CTX *init_CTX(void)
{
	SSL_CTX *ctx;
														// negotiate best mutually supported TLS protocol version
	const SSL_METHOD *method = SSLv23_method();
	if (!method) err(NEGOT_ERR);
														// create SSL/TLS object with negotiated method
	ctx = SSL_CTX_new(method);
	if (!ctx) err(PROTOCOL_ERR);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	//SSL_CTX_set_verify_depth(ctx, 4);
	const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
	SSL_CTX_set_options(ctx, flags);
														// load trusted CA
	long res = SSL_CTX_load_verify_locations(ctx, ROOT_CA, NULL);
	if (res != 1) err(CA_ERR);
	return (ctx);
}

void configure_ssl(SSL *ssl)
{
	long res;
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
														// ALERT! remove ciphers client/server doesn't support.
														// it'll reduce the size of binary.
	const char* const CIPHERS = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4!DSS";
	res = SSL_set_cipher_list(ssl, CIPHERS);
	if (res != 1) err(CIPHER_ERR);
	//res = SSL_set_tlsext_host_name(ssl, HOST);
	//if (res != 1) err(SIPHER_ERR);
}

void connect_to_server(SSL *ssl, BIO *obj)
{
	int len = 0;
	int num = 1000;
#if defined DEBUG
	const int MSS_test = get_mss();
	printf("MSS: %d\n", MSS_test);
#endif
	do
	{
		char buff[MSS] = {};
		char msg[] = "BAR12341";
		SSL_write(ssl, msg, strlen(msg));
		len = BIO_read(obj, buff, sizeof(buff));
		printf("Msg from server: %s\n", buff);
		//BIO_write(obj, msg, strlen(msg));
		//SSL_write(ssl, msg, strlen(msg));
		--num;
		/* code */
	} while (num > 0 || BIO_should_retry(obj));
}

int main()
{
	SSL_CTX *ctx = NULL;
	SSL *ssl = NULL;
	BIO *obj = NULL, *out = NULL;
	long res;

	init_openssl_library();
	init_CTX();
	ctx = init_CTX();
														// create SSL connection object & BIO chain
	obj = BIO_new_ssl_connect(ctx);
	if (!obj) err(BIO_ERR);
	res = BIO_set_conn_hostname(obj, HOST ":" PORT);
	if (res != 1) err(HOST_ERR);
	BIO_get_ssl(obj, &ssl);								// fetch connection object
	if (!ssl) err(SSL_ERR);
	configure_ssl(ssl);
	out = BIO_new_fp(stdout, BIO_NOCLOSE);
	if (!out) err(STDOUT_ERR);
	res = BIO_do_connect(obj);
	
	if (res != 1) err(CONN_ERR);
	res = BIO_do_handshake(obj);
	if (res != 1) err(HAND_ERR);
	
	X509 *cert = SSL_get_peer_certificate(ssl);			// make sure server provided certificate
	cert ? X509_free(cert) : err(CERT_ERR);
	
	res = SSL_get_verify_result(ssl);					// verify result of cert verification
	if (res != X509_V_OK) err(TRUST_ERR);
														// we could verify hostname here
	//BIO_puts(obj, "Hello server!");
	//BIO_puts(out, "\n");
	connect_to_server(ssl, obj);						// main code

	if (out)
		BIO_free(out);
	if (obj)
		BIO_free_all(obj);
	if (ctx)
		SSL_CTX_free(ctx);
	// 		THREAD_setup() need to use with threads
	return 0;
}