#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define STATUS_ERROR   -1
#define STATUS_SUCCESS 0
#define PROTO_VER 100

typedef enum {
  MSG_HELLO_REQ,
  MSG_HELLO_RESP,
  MSG_EMPLOYEE_LIST_REQ,
  MSG_EMPLOYEE_LIST_RESP,
  MSG_EMPLOYEE_ADD_REQ,
  MSG_EMPLOYEE_ADD_RESP,
  MSG_EMPLOYEE_DEL_REQ,
  MSG_EMPLOYEE_DEL_RESP,
  MSG_ERROR
} dbproto_type_e;

typedef struct {
  dbproto_type_e type;
  uint16_t len;
} dbproto_hdr_t;

typedef struct {
  uint16_t  proto;
} dbproto_hello_req;

typedef struct {
  uint16_t  proto;
} dbproto_hello_resp;

typedef struct {
  uint8_t data[1024];
} dbproto_add_employee_req;

typedef struct {
  uint16_t id;
} dbproto_add_employee_resp;

typedef struct {
  uint8_t name[256];
  uint8_t address[256];
  uint32_t hours;
} dbproto_employee_list_resp;

#endif
