#ifndef _PLAT_TOKEN_CAPTURE_H_
#define _PLAT_TOKEN_CAPTURE_H_

#include <token_capture.h>

int8_t platform_capture_token(void *data, uint8_t type, sos_pid_t pid);
void platform_destroy_token(void *data, uint8_t type);

enum {
	CYCLOPS_IMAGE	= 1,
	CYCLOPS_MATRIX	= 2,
};

#endif

