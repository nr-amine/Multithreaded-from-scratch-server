/* Amine Nouar 12409392, Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */


#include "list/list.h"
#include "user.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include "encryption/aes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT_FREESCORD 4321
#define PACKET_SIZE 512

/*AES Initialization*/
/*The AES implementation used is the one found in https://github.com/kokke/tiny-AES-c */

static const uint8_t aes_key[16] = {
    78, 101, 118, 101, 114, 71, 111, 110, 
    110, 97, 71, 105, 118, 101, 85, 112
};
/*--C'est de ASCII :)---*/
static const uint8_t aes_iv[16] = {
    70, 114, 101, 101, 115, 99, 111, 114, 
    100, 83, 101, 114, 118, 101, 114, 33
};
/* These need to be shared by both the client and the server 
   I tried to use RSA like my python project https://github.com/nr-amine/secure_cli_chat
   but because of the long integer arithmetic required I couldn't use Tiny RSA and using something
   like libsodium or openSSL would limit portability */

/********************/

int pipetb[2];
struct list *users;
pthread_mutex_t lock;
/** Gérer toutes les communications avec le client renseigné dans
 * user, qui doit être l'adresse d'une struct user */
void *handle_client(void *user);
/** Créer et configurer une socket d'écoute sur le port donné en argument
 * retourne le descripteur de cette socket, ou -1 en cas d'erreur */
int create_listening_sock(uint16_t port);

void *repeat_func(void *arg);

int send_encrypted(int sock, const char *msg);

int main(int argc, char *argv[]) {
  pthread_mutex_init(&lock, NULL);
  char *bienvenue = "Bienvenue sur Freescord !\r\nIci, il n'est pas nécessaire de fournir une piece d'identité!....\r\n\r\n";
  if (pipe(pipetb) == -1) {
    perror("pipe");
    return EXIT_FAILURE;
  }
  int sock = create_listening_sock(PORT_FREESCORD);
  if (sock == -1) {
    fprintf(stderr, "Failed to create listening socket\n");
    return EXIT_FAILURE;
  }
  users = list_create();
  pthread_t rept;
  if (pthread_create(&rept, NULL, repeat_func, NULL) != 0) {
    perror("pthread_create");
    close(sock);
    return EXIT_FAILURE;
  }
  pthread_t thr_id;
  for (;;) {
    struct user *usr = user_accept(sock);
    if (usr == NULL) {
      continue;
    }
    usr->nickname[0] = '\0'; /* In case another user does /list before the new one sets a nickname */
    pthread_mutex_lock(&lock);
    list_add(users, usr);
    pthread_mutex_unlock(&lock);
    if (pthread_create(&thr_id, NULL, handle_client, usr) != 0) {
      perror("pthread_create");
      close(usr->sock);
      user_free(usr);
      continue;
    }
    send(usr->sock, bienvenue, strlen(bienvenue), 0);
    pthread_detach(thr_id);
  }
}

void *handle_client(void *user) {
  struct user *usr = (struct user *)user;
  char buf[PACKET_SIZE];
  for (;;) {
    ssize_t s = recv(usr->sock, buf, PACKET_SIZE - 1, 0);
    if (s < 0) {
      perror("recv");
      break;
    }
    if (s == 0) {
      break;
    }
    buf[s] = '\0';

    if (usr->nickname[0] != '\0') {

      uint8_t received_line[PACKET_SIZE];
      unsigned bin_len = chex_decode(received_line, PACKET_SIZE, buf, s);
      received_line[bin_len] = '\0';
      crlf_to_lf((char *)received_line);
      strcpy(buf, (char *)received_line);
      
      struct AES_ctx ctx;
      AES_init_ctx_iv(&ctx, aes_key, aes_iv);
      AES_CTR_xcrypt_buffer(&ctx, received_line, bin_len);
      
      received_line[bin_len] = '\0';
      strcpy(buf, (char *)received_line);
    }

    char *response;
    char *nick = starts_with(buf, "nickname ");
    if (usr->nickname[0] == '\0') {
      if (nick == NULL) {
        response = "3 \r\n";
        send(usr->sock, response, strlen(response), 0);
        continue;
      }
    }
    if (nick != NULL) {
      size_t len = strlen(nick);

      while (len > 0 && (nick[len - 1] == '\n' || nick[len - 1] == '\r')) {
        nick[len - 1] = '\0';
        len--;
      }

      if (strlen(nick) > 16) {
        response = "2 \r\n";
        if (send(usr->sock, response, strlen(response), 0) < 0)
          break;
        continue;
      }
      int invalid_char = 0;
      for (size_t i = 0; i < strlen(nick); i++) {
        if (nick[i] == ':') {
          invalid_char = 1;
          break;
        }
      }
      if (invalid_char) {
        response = "2 \r\n";
        if (send(usr->sock, response, strlen(response), 0) < 0)
          break;
        continue;
      }

      int exists = 0;
      pthread_mutex_lock(&lock);
      for (struct node *curr = users->first; curr != NULL; curr = curr->next) {
        struct user *temp_user = (struct user *)curr->elt;
        if (strcmp(temp_user->nickname, nick) == 0) {
          exists = 1;
          break;
        }
      }

      response = exists ? "1 \r\n" : "0 \r\n";
      if (!exists) {
        strcpy(usr->nickname, nick);
      }
      pthread_mutex_unlock(&lock);
      if (send(usr->sock, response, strlen(response), 0) < 0)
        break;

      continue;
    }
    char *dm = starts_with(buf, "/msg ");

	/* Parse the message to split the target and the content */
    if (dm != NULL) {
      char *space = NULL;
      for (int i = 0; dm[i] != '\0'; i++) {
        if (dm[i] == ' ') {
          space = &dm[i];
          break;
        }
      }
      if (space == NULL) {
        continue;
      }
      *space = '\0';
      char *target_nick = dm;
      char *message = space + 1;

      struct user *target_user = NULL;
      pthread_mutex_lock(&lock);
      for (struct node *curr = users->first; curr != NULL; curr = curr->next) {
        struct user *temp_user = (struct user *)curr->elt;
        if (strcmp(temp_user->nickname, target_nick) == 0) {
          target_user = temp_user;
          break;
        }
      }
      pthread_mutex_unlock(&lock);

      if (target_user != NULL) {
        char dm_message[PACKET_SIZE + 23];
        sprintf(dm_message, "(DM) %s: %s", usr->nickname, message);
        if (send_encrypted(target_user->sock, dm_message) < 0) {
          perror("send");
        }
      }
      continue;
    }
    char *list = starts_with(buf, "/list");
    if (list != NULL) {
      char *hd = "Connected users:\n";
      send_encrypted(usr->sock, hd);

      pthread_mutex_lock(&lock);
      for (struct node *curr = users->first; curr != NULL; curr = curr->next) {
        struct user *temp_user = (struct user *)curr->elt;

        if (temp_user->nickname[0] != '\0') {
          char line[64];
          sprintf(line, "- %s\n", temp_user->nickname);
          send_encrypted(usr->sock, line);
        }
      }
      pthread_mutex_unlock(&lock);
      continue;
    }

    char *is_command = starts_with(buf, "/");
    if (is_command != NULL) {
      char *err_msg = "Unknown command\r\n";
      send_encrypted(usr->sock, err_msg);
      continue;
    }

    char message[PACKET_SIZE + 18];
    sprintf(message, "%s: %s", usr->nickname, buf);
    write(pipetb[1], message, strlen(message));
  }
  close(usr->sock);
  pthread_mutex_lock(&lock);
  list_remove_element(users, usr);
  pthread_mutex_unlock(&lock);
  user_free(usr);
  return NULL;
}

int create_listening_sock(uint16_t port) {
  char s_port[6];
  sprintf(s_port, "%u", port);

  struct addrinfo init = {
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_flags = AI_PASSIVE
  };
  struct addrinfo *addr;
  if (getaddrinfo(NULL, s_port, &init, &addr) != 0) {
    perror("getaddrinfo");
    return -1;
  }
int sock = -1;
    for (struct addrinfo *tmp = addr; tmp != NULL; tmp = tmp->ai_next) {
        sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if (sock == -1) continue;

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(sock, tmp->ai_addr, tmp->ai_addrlen) == 0)
            break;  

        close(sock);
        sock = -1;
    }

    freeaddrinfo(addr);

    if (sock == -1) {
        fprintf(stderr, "Failed to bind\n");
        return -1;
    }

    if (listen(sock, SOMAXCONN) == -1) {
        perror("listen");
        close(sock);
        return -1;
    }

    return sock;
}

void *repeat_func(void *arg) {
  for (;;) {
    char buf[PACKET_SIZE];
    ssize_t s = read(pipetb[0], buf, PACKET_SIZE - 1);
    if (s < 0) {
      perror("read");
      continue;
    }
    buf[s] = '\0';
    
    pthread_mutex_lock(&lock);
    for (struct node *curr = users->first; curr != NULL;) {
      struct node *nxt = curr->next;
      struct user *tmp = (struct user *)curr->elt;
      if (tmp == NULL || tmp->nickname[0] == '\0') {
        curr = nxt;
        continue;
      }
      int len = strlen(tmp->nickname);
      int trv = 1;
      for (int i = 0; i < len; i++) {
        if (buf[i] == '\0' || buf[i] != tmp->nickname[i]) {
          trv = 0;
          break;
        }
      }
      if (trv && buf[len] == ':') {
        curr = nxt;
        continue;
      }

      if (send_encrypted(tmp->sock, buf) < 0) {
        perror("send");
        close(tmp->sock);
        user_free(tmp);
      }
      curr = nxt;
    }
    pthread_mutex_unlock(&lock);
  }
}

int send_encrypted(int sock, const char *msg) {
  size_t msg_len = strlen(msg);
  char temp_msg[PACKET_SIZE];
  strncpy(temp_msg, msg, PACKET_SIZE - 3);
  temp_msg[PACKET_SIZE - 3] = '\0';
  // making sure its CRLF
  size_t len = strlen(temp_msg);
  if (len == 0 || temp_msg[len-1] != '\n') {
      lf_to_crlf(temp_msg);
  } else if (len >= 2 && temp_msg[len-2] != '\r') {
      temp_msg[len-1] = '\r';
      temp_msg[len]   = '\n';
      temp_msg[len+1] = '\0';
  }
  msg_len = strlen(temp_msg);
  
  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, aes_key, aes_iv);
  AES_CTR_xcrypt_buffer(&ctx, (uint8_t *)temp_msg, msg_len);
  
  char hexed_line[PACKET_SIZE * 2];
  unsigned hex_len = chex_encode((uint8_t *)temp_msg, msg_len, hexed_line, sizeof(hexed_line) - 2);
  hexed_line[hex_len] = '\n';
  hexed_line[hex_len + 1] = '\0';
  
  return send(sock, hexed_line, strlen(hexed_line), 0);
}
