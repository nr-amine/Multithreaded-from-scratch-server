#include "buffer/buffer.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include "encryption/aes.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT_FREESCORD 4321
#define PACKET_SIZE 512

/*AES Initialization*/

static const uint8_t aes_key[16] = {
    78, 101, 118, 101, 114, 71, 111, 110, 
    110, 97, 71, 105, 118, 101, 85, 112
};

static const uint8_t aes_iv[16] = {
    70, 114, 101, 101, 115, 99, 111, 114, 
    100, 83, 101, 114, 118, 101, 114, 33
};
/* These need to be shared by both the client and the server 
   I tried to use RSA like my python project https://github.com/nr-amine/secure_cli_chat
   but because of the long integer arithmetic required I couldn't use Tiny RSA and using something
   like libsodium or openSSL would limit portability */

/********************/

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

  /* Nickname init */
  for(;;) {
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
  /********************/

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
      if (lf_to_crlf(line) == NULL) continue;
      size_t msg_len = strlen(line);
      

      struct AES_ctx ctx;
      AES_init_ctx_iv(&ctx, aes_key, aes_iv);
      AES_CTR_xcrypt_buffer(&ctx, (uint8_t *)line, msg_len);

      /*Turning the encrypted message into a hex string*/
      char hexed_line[PACKET_SIZE*2];
      unsigned hex_len = chex_encode((uint8_t *)line, msg_len, hexed_line, PACKET_SIZE - 2);
      hexed_line[hex_len] = '\n'; /* The whole point of the hex encoding is the ability to add
                                     this newline */
      hexed_line[hex_len + 1] = '\0';
      
      sent = send(sock, hexed_line, strlen(hexed_line), 0);
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
      size_t in_len = strlen(line);
      if (in_len > 0 && line[in_len - 1] == '\n') {
          in_len--;
      }
      
      uint8_t received_line[PACKET_SIZE];
      unsigned bin_len = chex_decode(received_line, PACKET_SIZE, line, in_len);
      
      struct AES_ctx ctx;
      AES_init_ctx_iv(&ctx, aes_key, aes_iv);
      AES_CTR_xcrypt_buffer(&ctx, received_line, bin_len);
      
      received_line[bin_len] = '\0';
      crlf_to_lf((char *)received_line);
      printf("%s", (char *)received_line);
    }
    while (buff_ready(buf)) {
      if (buff_fgets(buf, line, PACKET_SIZE) == NULL)
        break;
      size_t in_len = strlen(line);
      if (in_len > 0 && line[in_len - 1] == '\n') {
          in_len--;
      }
      
      uint8_t received_line[PACKET_SIZE];
      unsigned bin_len = chex_decode(received_line, PACKET_SIZE, line, in_len);
      
      struct AES_ctx ctx;
      AES_init_ctx_iv(&ctx, aes_key, aes_iv);
      AES_CTR_xcrypt_buffer(&ctx, received_line, bin_len);
      
      received_line[bin_len] = '\0';
      crlf_to_lf((char *)received_line);
      printf("%s", (char *)received_line);
    }
    stat = buff_ready(buf) ? 0 : -1;
  }
  close(sock);
  buff_free(buf);
  free(line);
  return 0;
}

int connect_serveur_tcp(char *adresse, uint16_t port) {
    char s_port[6];
    snprintf(s_port, sizeof(s_port), "%u", port);

    struct addrinfo init = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };

    struct addrinfo *addr;
    if (getaddrinfo(adresse, s_port, &init, &addr) != 0) {
        fprintf(stderr, "getaddrinfo failed for %s\n", adresse);
        return -1;
    }

    int sock = -1;
    for (struct addrinfo *tmp = addr; tmp != NULL; tmp = tmp->ai_next) {
        sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if (sock == -1) continue;

        if (connect(sock, tmp->ai_addr, tmp->ai_addrlen) == 0)
            break; 

        close(sock);
        sock = -1;
    }

    freeaddrinfo(addr);

    if (sock == -1)
        fprintf(stderr, "Could not connect to %s\n", adresse);

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
  if (strcmp(response, "0 \r\n") == 0) {
    return 0;
  } else if (strcmp(response, "1 \r\n") == 0) {
    fprintf(stderr, "Name already taken. Please choose another nickname.\n");
    return -1;
  } else if (strcmp(response, "3 \r\n") == 0) {

    fprintf(stderr, "Command must start with nickname.\n");

    return -1;

  } else if (strcmp(response, "2 \r\n") == 0) {
    fprintf(stderr, "Invalid nickname. Please choose a nickname with at most "
                    "16 characters.\n");
    return -1;
  } else {
    fprintf(stderr, "Unknown error: '%s'\n", response);
    return -1;
  }
}
