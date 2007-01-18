/**
 * @brief    node id
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  1.0
 * This is the interface for modules, kernel can
 * just use node_address
 * 9/13/2005 mods made by Nicholas Kottenstette (nkottens@nd.edu)
 *   changed ker_loc to return copy of node_loc, changed node_loc.{x,y,z}
 *   to be int16_t, added gps_loc data, ker_gps interface, and
 *   ker_loc_r2 interface.
 */
#include <sos_info.h>
#include <sos.h>

node_loc_t node_loc = {
#ifdef NODE_LOC_UNIT
    .unit = NODE_LOC_UNIT,
#else
    .unit = UNIT_FEET,
#endif

#ifdef NODE_X
    .x = NODE_X,
#else
    .x = 0,
#endif

#ifdef NODE_Y
    .y = NODE_Y,
#else
    .y = 0,
#endif

#ifdef NODE_Z
    .z = NODE_Z,
#else
    .z = 0,
#endif
};

#ifndef NODE_GPS_X_DIR
#define NODE_GPS_X_DIR WEST
#endif
#ifndef NODE_GPS_X_DEG
#define NODE_GPS_X_DEG 0
#endif
#ifndef NODE_GPS_X_MIN
#define NODE_GPS_X_MIN 0
#endif
#ifndef NODE_GPS_X_SEC
#define NODE_GPS_X_SEC 0
#endif
#ifndef NODE_GPS_Y_DIR
#define NODE_GPS_Y_DIR NORTH
#endif
#ifndef NODE_GPS_Y_DEG
#define NODE_GPS_Y_DEG 0
#endif
#ifndef NODE_GPS_Y_MIN
#define NODE_GPS_Y_MIN 0
#endif
#ifndef NODE_GPS_Y_SEC
#define NODE_GPS_Y_SEC 0
#endif
#ifndef NODE_GPS_Z_UNIT
#define NODE_GPS_Z_UNIT UNIT_FEET
#endif
#ifndef NODE_GPS_Z
#define NODE_GPS_Z 0
#endif
gps_t gps_loc = {
    .x    = { NODE_GPS_X_DIR, NODE_GPS_X_DEG, NODE_GPS_X_MIN, NODE_GPS_X_SEC },
    .y     = { NODE_GPS_Y_DIR, NODE_GPS_Y_DEG, NODE_GPS_Y_MIN, NODE_GPS_Y_SEC },
    .unit  = NODE_GPS_Z_UNIT,
    .z     = NODE_GPS_Z,
};

#ifdef NODE_ADDR
uint16_t node_address = NODE_ADDR;
#else
uint16_t node_address = 1;
#endif



#ifdef NODE_GROUP_ID
uint8_t node_group_id = NODE_GROUP_ID;
#else
uint8_t node_group_id = 1;
#endif


#ifdef HW_TYPE
uint8_t hw_type = HW_TYPE;
#else
uint8_t hw_type = UNKNOWEN;
#endif

int8_t id_init()
{
   return SOS_OK;
}


//! The following function is used by the 802.15.4 MAC to set the node address after boot-up
void ker_set_id(uint16_t alloc_address)
{
    node_address = alloc_address;
}

uint16_t ker_id()
{
    return node_address;
}

node_loc_t ker_loc()
{
    return node_loc;
}

gps_t ker_gps()
{
    return gps_loc;
}

uint32_t ker_loc_r2(node_loc_t *loc1, node_loc_t *loc2)
{
    int32_t dx=0, dy=0, dz=0;
    uint32_t dx2=0, dy2=0, dz2=0;
#define SQUARED(x) ((x)*(x))
#define DXYZ_MAX ((int16_t)1<<14)
#define DXYZ_MIN -DXYZ_MAX

#ifndef MAX
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#endif

    if (loc1->unit != loc2->unit)
        return 0xffffffff;
    /* Limit the largest dimension to +/- 16384 to prevent overflow */
    dx = loc1->x - loc2->x;
    dy = loc1->y - loc2->y;
    dz = loc1->z - loc2->z;
    dx = MAX(dx,DXYZ_MIN);
    dy = MAX(dy,DXYZ_MIN);
    dz = MAX(dz,DXYZ_MIN);
    dx = MIN(dx,DXYZ_MAX);
    dy = MIN(dy,DXYZ_MAX);
    dz = MIN(dz,DXYZ_MAX);
    dx2 = SQUARED(dx);
    dy2 = SQUARED(dy);
    dz2 = SQUARED(dz);
    return dx2 + dy2 + dz2;
}

uint16_t ker_uart_id()
{
    return (uint16_t)UART_ADDRESS;
}

uint8_t ker_i2c_id()
{
    return (uint8_t)I2C_ADDRESS;
}

uint8_t ker_hw_type()
{
    return hw_type;
}

uint8_t ker_get_group(void) 				{ return node_group_id; }

void ker_set_group(uint8_t new_group_id) 	{ node_group_id = new_group_id; }

