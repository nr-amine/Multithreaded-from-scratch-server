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
	user->addr_len = sizeof(user->address);
	int clt_sock = accept(sl, (struct sockaddr *) &user->address, &user->addr_len);
	if (clt_sock < 0) {
		perror("accept");
		free(user);
		return NULL;
	}
	user->sock = clt_sock;
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
