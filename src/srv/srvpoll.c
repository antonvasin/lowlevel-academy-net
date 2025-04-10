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

void send_error(int fd, dbproto_hdr_t *hdr) {
  hdr->type = htonl(MSG_ERROR);
  hdr->len = htons(0);
  write(fd, hdr, sizeof(dbproto_hdr_t));
}

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employeesptr, clientstate_t *client, int dbfd) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (client->state == STATE_HELLO) {
    if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
      printf("Didn't get MSG_HELLO_REQ in HELLO state...\n");
      send_error(client->fd, hdr);
    }

    dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
    hello->proto = ntohs(hello->proto);
    if (hello->proto != PROTO_VER) {
      printf("Version mismatch.\n");
      send_error(client->fd, hdr);
      return;
    }

    hdr->type = htonl(MSG_HELLO_RESP);
    hdr->len = htons(1);
    dbproto_hello_resp *hello_resp = (dbproto_hello_resp *)&hdr[1];
    hello_resp->proto = htons(PROTO_VER);
    write(client->fd, hdr, sizeof(dbproto_hdr_t));

    client->state = STATE_MSG;
    printf("Client upgraded to STATE_MSG\n");
  }

  if (client->state == STATE_MSG) {
    if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
      dbproto_add_employee_req *employee = (dbproto_add_employee_req *)&hdr[1];

      if (add_employee(dbhdr, employeesptr, (char *)employee->data) != STATUS_SUCCESS) {
        printf("Failed to add an employee\n");
        send_error(client->fd, hdr);
        return;
      } else {
        hdr->type = htonl(MSG_EMPLOYEE_ADD_RESP);
        hdr->len = htons(1);
        dbproto_add_employee_resp *employee = (dbproto_add_employee_resp *)&hdr[1];
        employee->id = htons(dbhdr->count);
        write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_add_employee_resp));
        output_file(dbfd, dbhdr, *employeesptr);
        printf("Handled MSG_EMPLOYEE_ADD_REQ\n");
      }
    }

    if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
      hdr->type = htonl(MSG_EMPLOYEE_LIST_RESP);
      hdr->len = htons(dbhdr->count);

      // Send header first
      write(client->fd, hdr, sizeof(dbproto_hdr_t));

      dbproto_employee_list_resp *employee = (dbproto_hello_req *)&hdr[1];
      struct employee_t *employees = *employeesptr;

      // Send empolyees one by one
      for (int i = 0; i < dbhdr->count; i++) {
        strncpy(&employee->name, employees[i].name, sizeof(employee->name));
        strncpy(&employee->address, employees[i].address, sizeof(employee->address));
        employee->hours = htonl(employees[i].hours);
        printf("Sending employee %s\n", &employee->name);
        write(client->fd, employee, sizeof(dbproto_employee_list_resp));
      }

      printf("Handled MSG_EMPLOYEE_LIST_REQ\n");
    }
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

