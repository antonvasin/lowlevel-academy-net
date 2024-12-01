#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

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

int main() {
  int listen_fd, conn_fd, nfds, freeSlot;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  fd_set read_fds, write_fds;
  int slot;
  struct timeval timeout;

  init_clients();

  // Create listening socket
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
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

  // Main loop
  while (1) {
    // Clear FD set
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    // Add listening socket to the read set
    FD_SET(listen_fd, &read_fds);
    nfds = listen_fd + 1;

    // Add active connections to the read set while maintaining nfds
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (g_client_states[i].fd != -1) {
        FD_SET(g_client_states[i].fd, &read_fds);
        if (g_client_states[i].fd >= nfds) nfds = g_client_states[i].fd + 1;
      }
    }

    // Wait for 5s max for select to unblock
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (select(nfds, &read_fds, &write_fds, NULL, &timeout) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    // Check for the new connection and add it to global state
    if (FD_ISSET(listen_fd, &read_fds)) {
      // Skip to next connection if can't accept current one
      if ((conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len)) == -1) {
        perror("accept");
        continue;
      }

      printf("New connection from %s:%d\n",
          inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      if ((slot = find_free_slot()) == -1) {
        printf("Server is full, dropping connection\n");
        close(conn_fd);
      } else {
        g_client_states[slot].fd = conn_fd;
        g_client_states[slot].state = STATE_CONNECTED;
      }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (g_client_states[i].fd != -1 &&
          FD_ISSET(g_client_states[i].fd, &read_fds)) {
        ssize_t bytes_read = read(g_client_states[i].fd,
                                  &g_client_states[i].buffer,
                                  sizeof((g_client_states[i].buffer)));

        if (bytes_read <= 0) {
          close(g_client_states[i].fd);
          g_client_states[i].fd = -1;
          g_client_states[i].state = STATE_DISCONNECTED;
          // Clear buffer to avoid reading data from previous request
          memset(&g_client_states[i].buffer, '\0', BUF_SIZE);
          printf("Client disconnected on error\n");
        } else {
          printf("Received data from client: %s", g_client_states[i].buffer);
          write(g_client_states[i].fd, g_client_states[i].buffer, sizeof(g_client_states[i].buffer));
          close(g_client_states[i].fd);
          g_client_states[i].fd = -1;
          g_client_states[i].state = STATE_DISCONNECTED;
          memset(&g_client_states[i].buffer, '\0', BUF_SIZE);
          printf("Responded to client\n");
        }
      }
    }
  }

  return 0;
}
