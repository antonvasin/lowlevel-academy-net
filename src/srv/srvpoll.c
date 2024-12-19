#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>

#include "common.h"
#include "srvpoll.h"


void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees, clientstate_t *client) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (client->state == STATE_HELLO) {
    if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
      printf("Didn't get MSG_HELLO_REQ in HELLO state...\n");
      // TODO: send error msg
    }

    printf("Receiving MSG_HELLO_REQ\n");
    dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
    hello->proto = ntohs(hello->proto);
    if (hello->proto != PROTO_VER) {
      printf("Version mismatch.\n");
      hdr->type = htonl(MSG_ERROR);
      hdr->len = htons(0);
      write(client->fd, hdr, sizeof(dbproto_hdr_t));
      return;
    }

    printf("Sending MSG_HELLO_RESP\n");
    hdr->type = htonl(MSG_HELLO_RESP);
    hdr->len = htons(1);
    dbproto_hello_resp *hello_resp = (dbproto_hello_resp *)&hdr[1];
    hello_resp->proto = htons(PROTO_VER);
    write(client->fd, hdr, sizeof(dbproto_hdr_t));

    client->state = STATE_MSG;
    printf("Client upgraded to STATE_MSG\n");
  }

  if (client->state == STATE_MSG) {
    printf("Can't handle MSG yet...\n");
  }
}

void init_clients(clientstate_t *states) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    states[i].fd = -1; // free slot
    states[i].state = STATE_NEW;
    memset(&states[i].buffer, '\0', BUF_SIZE);
  }
}

int find_free_slot(clientstate_t *states) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (states[i].fd == -1) {
      return i;
    }
  }
  return -1; // no free slots
}

int find_slot_by_fd(int fd, clientstate_t *states) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (states[i].fd == fd) {
      return i;
    }
  }
  return -1;
}

