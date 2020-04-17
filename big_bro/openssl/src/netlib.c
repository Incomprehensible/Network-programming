#include "../includes/config.h"

#define ERROR 0
#define OK 1
extern USR_DATA User;
extern int code;

int decompress(char *payload);

int payload_handler(char *payload, size_t payload_len, size_t bufflen)
{
	if (bufflen <= 0 || !payload)
		return (ERROR);
	// here decompress payload
	// bufflen = decompress();
	const int size = (bufflen < payload_len)? bufflen : payload_len;
	int fd = open(FACE_PATH, O_WRONLY | O_CREAT, 0777);
	if (fd < 0)
	{
		perror("Error creating a new photo\n");
		exit(1);
	}
	long written = write(fd, payload, payload_len);
	if (written <= 0)
	{
		perror("Error saving payload\n");
		close(fd);
		return (ERROR);
	}
	printf("Created new photo! total size: %ld, with name %s\n", written, FACE_PATH);
	close(fd);
	code = NEW_DATA;
	return (OK);
}

int meta_handler(char *body, size_t len)
{
	if (len < META_BODY)
		return (ERROR);
	PAYLOAD *request = (struct payload_request_s *)body;
	User.SID = request->SID;
	if (!payload_handler(body + HEADER_LEN + META_BODY, request->SIZE, len - META_BODY))
		return (ERROR);
	return (OK);
}

int bare_handler(char *body, size_t len)
{
	if (len < BARE_BODY)
		return (ERROR);
	BARE *request = (struct bare_request_s *)body;
	if (!memcmp((void *)request->MSG, "0", 1))
		User.status = 0;
	else if (!memcmp((void *)request->MSG, "1", 1))
		User.status = 1;
	else return (ERROR);
	if (User.SID != request->SID)
		return (ERROR);
	code = NEW_MSG;
	return (OK);
}

int parse_header(char *body, size_t len)
{
	int i = -1;
	if (len < HEADER_LEN)
		return (i);
	REQ *request = (struct request *)body;
	if (!request->ID)
		return (i);
	 if (memcmp((void *)request->ID, "BAR", HEADER_LEN))
	 	i = 0;
	else if (memcmp((void *)request->ID, "PAY", HEADER_LEN)) 
		i = 1;
	return (i);
}

void parse_packet(char *request, size_t len) // to netlib
{
	code = 0;
	static int (*ptr[2]) (char *, size_t);

	if (!ptr[0])
	{
		ptr[0] = bare_handler;
		ptr[1] = meta_handler;
	}
	const int i = parse_header(request, len);
	if (i < 0)
	{
		code = ABORT; // mutex here
		return;
	}
	if (!(ptr[i](request, len - HEADER_LEN)))
	{
		code = ABORT;
		return;
	}
	// this code DOES:
	// sets code flag; writes metadata to structure or file; writes payload to file;
}