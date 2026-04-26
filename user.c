/* Amine Nouar 12409392, Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */


#include "user.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

/** accepter une connection TCP depuis la socket d'écoute sl et retourner un
 * pointeur vers un struct user, dynamiquement alloué et convenablement
 * initialisé */
struct user *user_accept(int sl)
{
	struct user *user = malloc(sizeof(struct user));
	if (user == NULL) {
		perror("malloc");
		return NULL;
	}
	struct sockaddr_in addr_clt;
	socklen_t slt = sizeof(addr_clt);
	int clt_sock = accept(sl, (struct sockaddr *) &addr_clt, &slt);
	if (clt_sock < 0) {
		perror("accept");
		free(user);
		return NULL;
	}
	user->sock = clt_sock;
	user->address = malloc(slt);
	memcpy(user->address, &addr_clt, slt);
	user->addr_len = slt;
	return user;
}

/** libérer toute la mémoire associée à user */
void user_free(struct user *user)
{
	if (user == NULL) {
		return;
	}
	free(user->address);
	free(user);
}
