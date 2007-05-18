#include <plat_token_capture.h>
#include <string.h>
#include <vire_malloc.h>

void *get_token_data(token_type_t *t) {
	if (t != NULL) return t->data;

	return NULL;
}

token_length_t get_token_length(token_type_t *t) {
	if (t != NULL) return t->length;

	return 0;
}

void release_token(token_type_t *t) {
	if (t != NULL) t->locked = 0;
}

void *capture_token_data(token_type_t *t, sos_pid_t pid) {
	uint8_t new_owner = (pid > KER_MOD_MAX_PID) ? ELEMENT : ENGINE;
	void *data_copy;

	if (t == NULL) return NULL;

	if (t->locked == 0) {
		// The token is free to be transferred to anyone.
		// The new owner captures and locks it.
		t->owner = new_owner;
		t->locked = 1;
		return t->data;
	}

	// The token data is locked. 
	// Need to make a copy of the data and return the pointer to new data
	// with appropriate ownership information.
	if (t->length > 0) {
		data_copy = vire_malloc(t->length, pid);
		if (data_copy == NULL) return NULL;
		memcpy(data_copy, t->data, t->length);
	} else {
		return NULL;
	}
	if (t->type == NORMAL) {
		return data_copy;
	} else {
		if (platform_capture_token(data_copy, t->type, pid) < 0) {
			vire_free(data_copy, t->length);
			return NULL;
		}
		return data_copy;
	}

	return NULL;
}

static token_type_t *create_token_internal(void *data, token_length_t length, uint8_t type, sos_pid_t pid) {
	token_type_t *t = (token_type_t *)token_malloc(pid);

	if (t == NULL) return NULL;
	t->data = data;
	t->length = length;
	if (pid > KER_MOD_MAX_PID) {
		t->owner = ELEMENT;
	} else {
		t->owner = ENGINE;
	}
	t->type = type;
	t->locked = 1;

	return t;
}

token_type_t *create_token(void *data, token_length_t length, sos_pid_t pid) {
	return create_token_internal(data, length, NORMAL, pid);
}

void set_token_type(token_type_t *t, uint8_t type) {
	if (t == NULL) return;
	t->type = type;
}

void destroy_token_data(void *data, uint8_t type, uint16_t length) {
	if (data == NULL) return;
	platform_destroy_token(data, type);
	vire_free(data, length);
}

void destroy_token(token_type_t *t) {
	if (t == NULL) return;
	if (t->locked == 0) {
		destroy_token_data(t->data, t->type, t->length);
	}
	token_free(t);
}

