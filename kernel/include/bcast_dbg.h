/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    header file for bcast debugging
 * @author   Naim Busek
 */


#ifndef _BCAST_DBG_H
#define _BCAST_DBG_H

#include <sos_info.h>
#include <malloc.h>
#include <message.h>

/* id must be a unique char string that will be appended to var instantation
 * to prevent namespace conflicts name */
#ifdef BCAST_DEBUG
#define BCAST_DBG_MSG_LEN 4
#define BCAST_DBG(name, flg0, flg1, flg2, flg3, pid) { \
	uint8_t *bcast_msg##name = ker_malloc(BCAST_DBG_MSG_LEN, pid); \
	if(bcast_msg##name) { \
		bcast_msg##name[0] = (flg0); \
		bcast_msg##name[1] = (flg1); \
		bcast_msg##name[2] = (flg2); \
		bcast_msg##name[3] = (flg3); \
		post_net(pid, pid, 69, BCAST_DBG_MSG_LEN, bcast_msg##name, SOS_MSG_RELEASE, BCAST_ADDRESS); \
	} else { \
		ker_free(bcast_msg##name); \
	} \
}
#else
#define BCAST_DBG(name, flg0, flg1, flg2, flg3, pid)
#endif

#endif

