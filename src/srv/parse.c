#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int i = 0;
  for (; i < dbhdr->count; i++) {
    printf("Employee %d:\n", i);
    printf("\tname - %s\n", employees[i].name);
    printf("\taddress - %s\n", employees[i].address);
    printf("\thours - %d\n", employees[i].hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
  char *name = strtok(addstring, ",");
  char *address = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");
  printf("%s %s %s\n", name, address, hours);
  strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
  strncpy(employees[dbhdr->count-1].address, address, sizeof(employees[dbhdr->count-1].address));
  employees[dbhdr->count-1].hours = atoi(hours);
  return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *removestring) {
  int i = 0;
  for (; i < dbhdr->count; i++) {
    if (strcmp(employees[i].name, removestring) == 0) break;
  }

  if (i == dbhdr->count) {
    printf("Can't find employee with name \"%s\"\n", removestring);
    return STATUS_ERROR;
  }

  dbhdr->count--;
  for (int j = i; j < dbhdr->count; j++) {
    employees[j] = employees[j+1];
  }

  struct employee_t *empl_tmp = realloc(employees, dbhdr->count);
  if (empl_tmp == NULL) {
    printf("realloc failed\n");
    return STATUS_ERROR;
  }

  employees = empl_tmp;

  return STATUS_SUCCESS;
}

int update_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *updatestring) {
  char *name = strtok(updatestring, ",");
  char *hours = strtok(NULL, ",");
  int i = 0;
  for (; i < dbhdr->count; i++) {
    if (strcmp(employees[i].name, name) == 0) break;
  }

  if (i == dbhdr->count) {
    return STATUS_ERROR;
  }

  employees[i].hours = atoi(hours);

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Bad descriptor from the user\n");
    return STATUS_ERROR;
  }

  int count = dbhdr->count;

  struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == (void*)-1) {
    printf("malloc failed\n");
    return STATUS_ERROR;
  }

  read(fd, employees, count * sizeof(struct employee_t));
  int i = 0;
  for (; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
  if (fd < 0) {
    printf("Bad descriptor from the user");
    return STATUS_ERROR;
  }

  int realcount = dbhdr->count;

  dbhdr->magic = htonl(dbhdr->magic);
  dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t)) * realcount);
  dbhdr->count = htons(dbhdr->count);
  dbhdr->version = htons(dbhdr->version);

  lseek(fd, 0, SEEK_SET);
  write(fd, dbhdr, sizeof(struct dbheader_t));

  int i = 0;
  for (; i < realcount; i++) {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, &employees[i], sizeof(struct employee_t));
  }

  ftruncate(fd, sizeof(struct dbheader_t) + realcount * sizeof(struct employee_t));

  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Bad descriptor\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));

  if (header == NULL) {
    printf("malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);

  if (header->version != 1) {
    printf("Wrong header version\n");
    free(header);
    return STATUS_ERROR;
  }

  if (header->magic != HEADER_MAGIC) {
    printf("Wrong header magic\n");
    free(header);
    return STATUS_ERROR;
  }

  struct stat dbstat = {0};
  fstat(fd, &dbstat);
  if (header->filesize != dbstat.st_size) {
    printf("Corrupt file\n");
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;

  return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));

  if (header == NULL) {
    printf("malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_SUCCESS;
}


