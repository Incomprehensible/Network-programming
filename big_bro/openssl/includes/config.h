#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netinet/in.h>

# ifndef CONFIG_H
# define CONFIG_H

#define HOST "localhost"
#define MASTER "84.201.164.158"
#define PORT "12345"
#define ROOT_CA "/home/incomprehensible/BigBro/Recognizing/malina_side/src/openssl/CA/myCA.pem"
#define CLIENT_CRT "/home/incomprehensible/BigBro/Recognizing/malina_side/src/openssl/SERT_client/client.crt"
#define CLIENT_KEY "/home/incomprehensible/BigBro/Recognizing/malina_side/src/openssl/SERT_client/client.key"
#define MSS 1460

SSL_CTX *create_server_context(void);
int start_server(SSL_CTX *);
void cleanup_openssl(void);
void parse_packet(char *request, size_t len);

#define NEW_DATA 0x01
#define NEW_MSG 0x03
#define DATA_ERROR 0x02
#define DESTROY 0x4
#define ABORT 0x5
#define FORCE 0x666

//#define META_PATH 
#define HOME "/home/incomprehensible/BigBro/Recognizing/malina_side/src/openssl"
#define FACE "src/somedata"
#define FACE_PATH HOME "/" FACE
#define HEADER_LEN 4
#define MSG_LEN 2
#define BARE_BODY 6
#define META_BODY 8

typedef struct	network_io_s
{
	int into;
	int out;
}				network_io;

typedef struct	request
{
	char ID[4];
	char reserved[8];
}				REQ;

typedef struct	bare_request_s
{
	char	ID[4];
	int32_t	SID;
	char	MSG[2];
}				BARE;

// aign both structures so you can use a common pointer to each
typedef struct	payload_request_s
{
	char	ID[4];
	int32_t	SID;
	int32_t	SIZE;
	// rest is a payload
}				PAYLOAD;

typedef struct	user_data_s
{
	int32_t SID;
	int32_t status;
}				USR_DATA;

/* Errors */

#define NEGOT_ERR "Could not negotiate TLS protocol version\n"
#define PROTOCOL_ERR "Could not set TLS protocol\n"
#define CA_ERR "Could not load trusted CA chain\n"
#define CIPHER_ERR "Could not remove unwanted ciphers from list\n"
#define CONN_ERR "Could not connect to remote host\n"
#define HAND_ERR "Error performing handshake\n"
#define SSL_ERR "Error fetching SSL connection object\n"
#define BIO_ERR "Error creating BIO/SSL connection object\n"
#define HOST_ERR "Error setting hostname for connection\n"
#define CERT_ERR "Error getting remote host's certificate\n"
#define TRUST_ERR "Error: not a trusted server. Aborting connection\n"
#define STDOUT_ERR "Error setting stdout\n"
#define DELFACE "rm -f " FACE "/somefile1"
#define DELMAP "rm -f " FACE "/somefile2"

# endif
