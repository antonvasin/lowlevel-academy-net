#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int send_hello(int fd) {
  char buf[BUF_SIZE] = {0};

  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = MSG_HELLO_REQ;
  hdr->len = 1;

  // buf: [type][len][proto]
  //      ↑          ↑
  //      hdr        hello
  dbproto_hello_req *hello = (dbproto_hello_req*)&hdr[1];
  hello->proto = PROTO_VER;

  hdr->type = htonl(hdr->type);
  hdr->len = htons(hdr->len);
  hello->proto = htons(hello->proto);

  // send hello message
  write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

  // wait for the response
  read(fd, buf, sizeof(buf));

  // parse the response
  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (hdr->type == MSG_ERROR) {
    printf("Server responded with error\n");
    close(fd);
    return STATUS_ERROR;
  }

  printf("Connected to server using protocol v1.\n");
  return STATUS_SUCCESS;
}

int send_employee(int fd, char *addstr) {
  char buf[BUF_SIZE] = {0};
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = MSG_EMPLOYEE_ADD_REQ;
  hdr->len = 1;

  dbproto_add_employee_req *employee = (dbproto_add_employee_req *)&hdr[1];
  strncpy(&employee->data, addstr, sizeof(employee->data));

  hdr->type = htonl(hdr->type);
  hdr->len = htons(hdr->len);

  // [       type         ][len][          addstr             ]
  // [MSG_EMPLOYEE_ADD_REQ][ 1 ][John Smith,123 Parkway Dr.,50]
  write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_add_employee_req));

  read(fd, buf, sizeof(buf));

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (hdr->type == MSG_ERROR) {
    printf("Error adding an employee \"%s\"\n", addstr);
    close(fd);
    return STATUS_ERROR;
  }

  printf("Succesfully added an employee \"%s\"\n", addstr);
  return STATUS_SUCCESS;
}

void print_usage(char * argv[]) {
  printf("Usage: %s -h <host> -p <port>\n", argv[0]);
  printf("\t-h - (required) host to connect to\n");
  printf("\t-p - (required) port to use\n");
  printf("\t-a - add an employee\n");
  return;
}

int main(int argc, char *argv[]) {
  char *host = NULL;
  char *addstring = NULL;
  int port = -1;
  int c;

  while ((c = getopt(argc, argv, "h:p:a:")) != -1) {
    switch (c) {
      case 'h':
        host = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'a':
        addstring = optarg;
        break;
      case '?':
        printf("Unknown option -%c\n", c);
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

  if (send_hello(fd) != STATUS_SUCCESS) {
    return -1;
  }

  if (addstring) {
    send_employee(fd, addstring);
  }

  close(fd);

  return 0;
}
