
#include <sos.h>
#include <malloc.h>
#include <sos_cam.h>

enum {
	CAM_NUM_BINS  =  4,
};

typedef struct _cam_str {
	sos_cam_t key;
	void *value;
	struct _cam_str *next;
} cam_t;

static cam_t *cam_bin[CAM_NUM_BINS] = {NULL};

static inline uint8_t hash_bin(sos_cam_t key)
{
	return key % CAM_NUM_BINS;
}

static inline cam_t *key_to_bin(sos_cam_t key)
{
	return cam_bin[hash_bin(key)];
}

static cam_t *cam_lookup(sos_cam_t key) 
{
	cam_t *cam = key_to_bin(key);
	
	while(cam != NULL) {
		if(cam->key == key) {
			return cam;
		}
		cam = cam->next;
	}
	return NULL;
}

int8_t ker_cam_add(sos_cam_t key, void *value)
{
	cam_t *cam;	
	
	if(cam_lookup(key) != NULL) {
		return -EEXIST;
	}

	cam = ker_malloc( sizeof(cam_t), KER_CAM_PID );

	if( cam == NULL ) { return -ENOMEM; }
	
	cam->key = key;
	cam->value = value;

	cam->next = key_to_bin( key );
	cam_bin[ hash_bin( key ) ] = cam;
	return SOS_OK;	
}

void* ker_cam_reassign(sos_cam_t key, void *new_value)
{
	cam_t *cam = cam_lookup( key );

	if( cam != NULL ) {
		void *ret = cam->value;
		cam->value = new_value;
		return ret;
	}
	return NULL;
}

void* ker_cam_remove(sos_cam_t key)
{
	cam_t *cam = key_to_bin(key);
	cam_t *prev;
	cam_t *curr;
	void *ret;

	prev = cam;
	curr = cam;
	while(curr != NULL) {
		if( curr->key == key ) {
			if( curr == cam ) {
				//! remove head
				cam_bin[ hash_bin( key ) ] = cam->next;
			} else {
				prev->next = curr->next;
			}
			ret = curr->value;
			ker_free(curr);
			return ret;
		}
		prev = curr;
		curr = curr->next;
	}
	return NULL;
}

void* ker_cam_lookup(sos_cam_t key)
{
	cam_t *cam = cam_lookup( key );

	if(cam != NULL) return cam->value;
	return NULL;
}

