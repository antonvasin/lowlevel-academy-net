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
  MSG_EMPLOYEE_DEL_RESP
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

#endif
