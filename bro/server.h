#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <algorithm>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

typedef std::vector<std::pair<int, char *> > my_slaves;

#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif