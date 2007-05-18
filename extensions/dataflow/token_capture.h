#ifndef _TOKEN_CAPTURE_H_
#define _TOKEN_CAPTURE_H_

#include <module.h>

// Token capture API
enum {
	ELEMENT	= 0,
	ENGINE,
};

enum {
	NORMAL	= 0,
};

typedef uint16_t token_length_t;

typedef struct token_type_t {
	uint8_t type : 5;
	uint8_t owner : 2;
	uint8_t locked : 1;
	token_length_t length;
	void *data;
} PACK_STRUCT token_type_t;

token_type_t *create_token(void *data, token_length_t length, sos_pid_t pid);

void set_token_type(token_type_t *t, uint8_t type);

void release_token(token_type_t *t);

void *capture_token_data(token_type_t *t, sos_pid_t pid);

void *get_token_data(token_type_t *t);

token_length_t get_token_length(token_type_t *t);

void destroy_token(token_type_t *t);

void destroy_token_data(void *data, uint8_t type, uint16_t length);

#endif

