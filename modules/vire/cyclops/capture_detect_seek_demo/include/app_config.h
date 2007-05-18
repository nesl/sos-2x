#ifndef _APP_CONFIG_INCL_H_
#define _APP_CONFIG_INCL_H_

#include <camera_settings.h>
#include <image.h>

#define ROWS			_CYCLOPS_RESOLUTION_H
#define COLS			_CYCLOPS_RESOLUTION_W
#define COEFFICIENT		.25
#define SKIP			4
#define DETECT_THRESH	50
#define RANGE			5

#define NONE			0x00
#define OBJ_DETECT		0x01

typedef struct objectInfo
{
  wsize_t objectSize;
  wpos_t objectPosition;
  uint8_t actionFlag;
} objectInfo;

#endif
