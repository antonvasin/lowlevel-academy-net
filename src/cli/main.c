#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "common.h"
#define BUF_SIZE 4096

typedef enum {
  PROTO_HELLO,
} proto_type_e;

// Type Length Value (TLV)
typedef struct {
  proto_type_e type;
  unsigned short len;
} proto_hdr_t;

void send_hello(int fd) {
  char buf[BUF_SIZE] = {0};

  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = MSG_HELLO_REQ;
  hdr->len = 1;

  hdr->type = htonl(hdr->type);
  hdr->len = htons(hdr->len);

  write(fd, buf, sizeof(dbproto_hdr_t));

  printf("Server connected. Protocol v1.\n");
}

void print_usage(char * argv[]) {
  printf("Usage: %s -h <host> -p <port>\n", argv[0]);
  printf("\t-h - (required) host to connect to\n");
  printf("\t-p - (required) port to use\n");
  return;
}

int main(int argc, char *argv[]) {
  char *host = NULL;
  int port = -1;
  int c;

  while ((c = getopt(argc, argv, "h:p:")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      default:
        return -1;
    }
  }

  if (host == NULL || port == -1) {
    printf("Host and port are required\n");
    print_usage(argv);
    return 0;
  }

  struct sockaddr_in serverInfo = {0};

  serverInfo.sin_family = AF_INET;
  serverInfo.sin_addr.s_addr = inet_addr(host);
  serverInfo.sin_port = htons(port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return -1;
  }

  if (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
    perror("connect");
    return 0;
  }

  send_hello(fd);

  close(fd);

  return 0;
}
