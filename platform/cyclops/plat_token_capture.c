#include <plat_token_capture.h>
#include <string.h>
#include <image.h>
#include <matrix.h>
#include <led.h>

int8_t platform_capture_token(void *data, uint8_t type, sos_pid_t pid) {
	switch(type) {
		case CYCLOPS_IMAGE:
		{
			CYCLOPS_Image *img = (CYCLOPS_Image *)data;
			if (img == NULL) return SOS_OK;
			uint16_t size = imageSize(img);
			if (size == 0) return SOS_OK;
			uint8_t *src_ptr = ker_get_handle_ptr(img->imageDataHandle), *dest_ptr;
			if (src_ptr == NULL) return -EINVAL;

			img->imageDataHandle = ker_get_handle(size, pid);
			dest_ptr = ker_get_handle_ptr(img->imageDataHandle);
			if (dest_ptr == NULL) return -ENOMEM;

			memcpy(dest_ptr, src_ptr, size);

			return SOS_OK;
		}
		case CYCLOPS_MATRIX:
		{
			CYCLOPS_Matrix *m = (CYCLOPS_Matrix *)data;
			if (m == NULL) return SOS_OK;
			uint16_t size = m->rows * m->cols;
			if (size == 0) return SOS_OK;
			uint8_t *src_ptr = ker_get_handle_ptr(m->data.hdl8), *dest_ptr;
			if (src_ptr == NULL) return -EINVAL;
			
			m->data.hdl8 = ker_get_handle(size, pid);
			dest_ptr = ker_get_handle_ptr(m->data.hdl8);
			if (dest_ptr == NULL) return -ENOMEM;

			memcpy(dest_ptr, src_ptr, size);

			return SOS_OK;
		}
		default:
			return -EINVAL;
	}

	return SOS_OK;
}

void platform_destroy_token(void *data, uint8_t type) {
	switch(type) {
		case CYCLOPS_IMAGE:
		{
			CYCLOPS_Image *img = (CYCLOPS_Image *)data;
			if (img == NULL) break;
			ker_free_handle(img->imageDataHandle);
			break;
		}
		case CYCLOPS_MATRIX:
		{
			CYCLOPS_Matrix *m = (CYCLOPS_Matrix *)data;
			if (m == NULL) break;
			ker_free_handle(m->data.hdl8);
			break;
		}
		default:
			break;
	}
}

