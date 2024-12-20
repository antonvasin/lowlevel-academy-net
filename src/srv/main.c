#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>

#include "file.h"
#include "parse.h"
#include "common.h"
#include "srvpoll.h"


clientstate_t g_client_states[MAX_CLIENTS];

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>\n", argv[0]);
  printf("\t-f - (required) path to database file\n");
  printf("\t-p - (required) port to use\n");
  printf("\t-n - create database file\n");
  printf("\t-a - [name,address,hours] add record to database\n");
  printf("\t-l - list records\n");
  printf("\t-u - [name,hours] update employee's hours\n");
  printf("\t-r - remove record by employee name\n");
  return;
}

void poll_loop(unsigned short port, struct dbheader_t *dbhdr, struct employee_t *employees, int dbfd) {
  int listen_fd, conn_fd, freeSlot;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  struct pollfd fds[MAX_CLIENTS + 1];
  int nfds = 1;

  init_clients(g_client_states);

  // Create listening socket
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsocketopt");
    exit(EXIT_FAILURE);
  }

  // Server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind");
    close(listen_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(listen_fd, 10) == -1) {
    perror("listen");
    close(listen_fd);
    // return -1;
  }

  printf("Listening on port %d...\n", port);

  memset(fds, 0, sizeof(fds));
  fds[0].fd = listen_fd;
  fds[0].events = POLLIN;
  nfds = 1;

  // Main loop
  while (1) {
    int fd_i = 1;

    // Add active connections to the read set while maintaining nfds
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (g_client_states[i].fd != -1) {
        fds[fd_i].fd = g_client_states[i].fd;
        fds[fd_i].events = POLLIN;
        fd_i++;
      }
    }

    int n_events = poll(fds, fd_i, -1); // -1 means no timeout
    if (n_events == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }

    // Check for new connections
    if (fds[0].revents & POLLIN) {

      if ((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) {
        perror("accept");
        continue;
      }

      printf("New connection from %s:%d\n",
          inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      if ((freeSlot = find_free_slot(g_client_states)) == -1) {
        printf("No free connection slots, sorry.\n");
        close(conn_fd);
      } else {
        g_client_states[freeSlot].fd = conn_fd;
        // FIXME: it workd with HELLO but shouldn't it be CONNECTED first and
        // then hello?
        g_client_states[freeSlot].state = STATE_HELLO;
        printf("Slot %d has client %d\n", freeSlot, conn_fd);
        nfds++;
      }

      n_events--;
    }

    // Check for new events in tracked connections until we out of connections
    // or events to process
    for (int i = 0; i <= nfds && n_events > 0; i++) {
      // We got new event at fds[i]
      if (fds[i].revents & POLLIN) {
        n_events--;

        int fd = fds[i].fd;
        int slot = find_slot_by_fd(fd, g_client_states);
        ssize_t bytes_read = read(fd, &g_client_states[slot].buffer, sizeof(g_client_states[slot]));

        // Handle read error
        if (bytes_read <= 0) {
          // perror("read");
          close(fd);

          if (slot != -1) {
            g_client_states[slot].fd = -1;
            g_client_states[slot].state = STATE_DISCONNECTED;
            memset(&g_client_states[slot].buffer, '\0', BUF_SIZE);
            nfds--;
            printf("Client disconnected\n");
          }
          // Handle request
        } else {
          handle_client_fsm(dbhdr, &employees, &g_client_states[slot], dbfd);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  char *filepath = NULL;
  bool newfile = false;
  int port = -1;
  int c;
  int dbfd = -1;
  struct dbheader_t *dbhdr = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:p:")) != -1) {
    switch (c) {
      case 'n':
        newfile = true;
        break;
      case 'f':
        filepath = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case '?':
        print_usage(argv);
        break;
      default:
        return -1;
    }
  }

  if (filepath == NULL) {
    printf("Filepath is a required argument.\n");
    print_usage(argv);
    return 0;
  }

  if (port == -1) {
    printf("Port is a required argument.\n");
    print_usage(argv);
    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create database file\n");
      return -1;
    }

    if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to create db header\n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);

    if (dbfd == -1) {
      printf("Unable to open database file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to validate db file\n");
      return -1;
    }

  }

  if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
    printf("Failed to read employees\n");
    return 0;
  }

  poll_loop(port, dbhdr, employees, dbfd);

  return 0;
}
