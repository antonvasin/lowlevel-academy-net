#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
  memcpy(employee->data, addstr, sizeof(employee->data));

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

  return STATUS_SUCCESS;
}

int send_list_employees(int fd) {
  char buf[BUF_SIZE] = {0};
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = htonl(MSG_EMPLOYEE_LIST_REQ);
  hdr->len = htons(0);

  printf("Sending MSG_EMPLOYEE_LIST_REQ\n");
  write(fd, hdr, sizeof(proto_hdr_t));

  read(fd, buf, sizeof(dbproto_hdr_t));

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (hdr->type == MSG_ERROR) {
    printf("Error listing the employees\n");
    close(fd);
    return STATUS_ERROR;
  }

  if (hdr->type == MSG_EMPLOYEE_LIST_RESP) {
    printf("Received MSG_EMPLOYEE_LIST_RESP\n");
    dbproto_employee_list_resp *employee = (dbproto_hello_req *)&hdr[1];

    int i = 0;
    for (; i < hdr->len; i++) {
      read(fd, employee, sizeof(dbproto_employee_list_resp));
      employee->hours = ntohl(employee->hours);
      printf("\tEmployee %d: %s, %s - %dhrs\n", i, employee->name, employee->address, employee->hours);
    }
  }

  return EXIT_SUCCESS;
}

void print_usage(char * argv[]) {
  printf("Usage: %s -h <host> -p <port>\n", argv[0]);
  printf("\t-h - (required) host to connect to\n");
  printf("\t-p - (required) port to use\n");
  printf("\t-a - add an employee\n");
  printf("\t-l - get list of employees\n");
  return;
}

int main(int argc, char *argv[]) {
  char *host = NULL;
  char *addstring = NULL;
  bool listarg = false;
  int port = -1;
  int c;

  while ((c = getopt(argc, argv, "h:p:a:l")) != -1) {
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
      case 'l':
        listarg = true;
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

  if (listarg && send_list_employees(fd) != STATUS_SUCCESS) {
    printf("Error getting list of employees\n");
  }

  close(fd);

  return 0;
}
