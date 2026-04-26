/* Amine Nouar 12409392, Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */


#include "buffer/buffer.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT_FREESCORD 4321
#define PACKET_SIZE 512
/** se connecter au serveur TCP d'adresse donnée en argument sous forme de
 * chaîne de caractère et au port donné en argument
 * retourne le descripteur de fichier de la socket obtenue ou -1 en cas
 * d'erreur. */
int connect_serveur_tcp(char *adresse, uint16_t port);

/** Verifier si un nickname est valide */
int nickname_check(int sock, char *nickname);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <adresse ipv4 du serveur>\n", argv[0]);
    return EXIT_FAILURE;
  }
  int sock = connect_serveur_tcp(argv[1], PORT_FREESCORD);
  if (sock == -1) {
    fprintf(stderr, "Failed to connect to server\n");
    return EXIT_FAILURE;
  }

  /*    Welcome message    */
  char *bienvenue = malloc(PACKET_SIZE);
  ssize_t b_size = recv(sock, bienvenue, PACKET_SIZE - 1, 0);
  if (b_size > 0) {
    bienvenue[b_size] = '\0';
    printf("%s", bienvenue);
  }
  free(bienvenue);
  /************************/

  printf("Type \"nickname <your_nickname>\" to set your nickname (max 16 "
         "characters)\n");
  while (1) {
    char nickname[PACKET_SIZE];
    if (fgets(nickname, PACKET_SIZE, stdin) == NULL) {
      fprintf(stderr, "Error reading nickname\n");
      close(sock);
      return EXIT_FAILURE;
    }
    nickname[strlen(nickname) - 1] = '\0';
    if (nickname_check(sock, nickname) == 0) {
      fprintf(stderr, "Nickname set to %s\n", nickname + 9);
      break;
    }
  }
  struct buffer *buf = buff_create(sock, PACKET_SIZE - 1);
  char *line = malloc(PACKET_SIZE);
  if (buf == NULL) {
    perror("buff_create");
    close(sock);
    return EXIT_FAILURE;
  }
  ssize_t sent;
  struct pollfd fiches[2] = {{.fd = 0, .events = POLLIN},
                             {.fd = sock, .events = POLLIN}};
  int stat = -1;
  while (poll(fiches, 2, stat) != -1) {
    if (fiches[0].revents & POLLIN) {
      ssize_t sz = read(0, line, PACKET_SIZE - 1);
      if (sz <= 0)
        break;
      line[sz] = '\0';

      sent = send(sock, line, strlen(line), 0);
      if (sent < 0) {
        perror("send");
        break;
      }
    }
    if (fiches[1].revents & POLLIN) {
      if (buff_fgets(buf, line, PACKET_SIZE) == NULL) {
        fprintf(stderr, "Server disconnected\n");
        break;
      }
      printf("%s", line);
    }
    while (buff_ready(buf)) {
      if (buff_fgets(buf, line, PACKET_SIZE) == NULL)
        break;
      printf("%s", line);
    }
    stat = buff_ready(buf) ? 0 : -1;
  }
  close(sock);
  buff_free(buf);
  free(line);
  return 0;
}

int connect_serveur_tcp(char *adresse, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    return -1;
  }
  struct sockaddr_in sa = {.sin_family = AF_INET, .sin_port = htons(port)};
  if (inet_pton(AF_INET, adresse, &sa.sin_addr) != 1) {
    fprintf(stderr, "adresse ipv4 non valable\n");
    close(sock);
    return -1;
  }
  if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
    perror("connect");
    close(sock);
    return -1;
  }

  return sock;
}

int nickname_check(int sock, char *nickname) {
  if (send(sock, nickname, strlen(nickname), 0) < 0) {
    perror("send");
    return -1;
  }
  char response[PACKET_SIZE];
  ssize_t recv_size = recv(sock, response, PACKET_SIZE - 1, 0);
  if (recv_size < 0) {
    perror("recv");
    return -1;
  }
  response[recv_size] = '\0';
  if (strcmp(response, "0 \r\n") == 0 || strcmp(response, "0") == 0) {
    return 0;
  } else if (strcmp(response, "1 \r\n") == 0 || strcmp(response, "1") == 0) {
    fprintf(stderr, "Name already taken. Please choose another nickname.\n");
    return -1;
  } else if (strcmp(response, "3 \r\n") == 0 || strcmp(response, "3") == 0) {

    fprintf(stderr, "Command must start with nickname.\n");

    return -1;

  } else if (strcmp(response, "2 \r\n") == 0 || strcmp(response, "2") == 0) {
    fprintf(stderr, "Invalid nickname. Please choose a nickname with at most "
                    "16 characters.\n");
    return -1;
  } else {
    fprintf(stderr, "Unknown error: '%s'\n", response);
    return -1;
  }
}
