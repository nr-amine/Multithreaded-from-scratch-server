#ifndef USER_H
#define USER_H
#include <sys/socket.h>
#include <netinet/in.h>

struct user {
	struct sockaddr_storage address;
	socklen_t addr_len;
	int sock;
	char nickname[17];
};

struct user *user_accept(int sl);
void user_free(struct user *user);

#endif /* ifndef USER_H */
