#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return -1;
  }
  return 0;
}
