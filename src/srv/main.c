#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>

#define MAX_CLIENTS 256
#define PORT 5555
#define BUF_SIZE 4096

typedef enum {
  STATE_NEW,
  STATE_CONNECTED,
  STATE_DISCONNECTED,
} state_e;

typedef struct {
  int fd;
  state_e state;
  char buffer[BUF_SIZE];
} clientstate_t;

clientstate_t g_client_states[MAX_CLIENTS];

void init_clients() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    g_client_states[i].fd = -1; // free slot
    g_client_states[i].state = STATE_NEW;
    memset(&g_client_states[i].buffer, '\0', BUF_SIZE);
  }
}

int find_free_slot() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (g_client_states[i].fd == -1) {
      return i;
    }
  }
  return -1; // no free slots
}

int find_slot_by_fd(int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (g_client_states[i].fd == fd) {
      return i;
    }
  }
  return -1;
}

int main() {
  int listen_fd, conn_fd, freeSlot;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  struct pollfd fds[MAX_CLIENTS + 1];
  int nfds = 1;
  int opt = 1;

  init_clients();

  // Create listening socket
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsocketopt");
    exit(EXIT_FAILURE);
  }

  // Server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind");
    close(listen_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(listen_fd, 10) == -1) {
    perror("listen");
    close(listen_fd);
    return -1;
  }

  printf("Listening on port %d...\n", PORT);

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

      if ((freeSlot = find_free_slot()) == -1) {
        printf("No free connection slots, sorry.\n");
        close(conn_fd);
      } else {
        g_client_states[freeSlot].fd = conn_fd;
        g_client_states[freeSlot].state = STATE_CONNECTED;
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
        int slot = find_slot_by_fd(fd);
        ssize_t bytes_read = read(fd, &g_client_states[slot].buffer, sizeof(g_client_states[slot]));

        // Handle read error
        if (bytes_read <= 0) {
          perror("read");
          close(fd);
          if (slot == -1) {
            printf("Trying to close connection that doesn't exist\n");
          } else {
            g_client_states[slot].fd = -1;
            g_client_states[slot].state = STATE_DISCONNECTED;
            memset(&g_client_states[slot].buffer, '\0', BUF_SIZE);
            nfds--;
            printf("Client %d disconnected on a error\n", i);
          }
        // Handle request
        } else {
          printf("Received data from the client: %s\n", g_client_states[slot].buffer);
          // write(g_client_states[i].fd, g_client_states[i].buffer, sizeof(g_client_states[i].buffer));
          // close(g_client_states[i].fd);
          // g_client_states[i].fd = -1;
          // g_client_states[i].state = STATE_DISCONNECTED;
          // memset(&g_client_states[i].buffer, '\0', BUF_SIZE);
          // printf("Responded to client\n");
        }
      }
    }
  }

  return 0;
}
