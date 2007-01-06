/**
 * @file mod_pid.h
 * @brief PID definition for application modules
 * @author Ram Kumar {ram@ee.ucla.edu}
 *
 */

#ifndef _MOD_PID_H
#define _MOD_PID_H

// A central place to register all the application specific module IDs
// The module IDs should be assigned relative to APP_MOD_MIN_PID
// For example
// #define MAG_SENSOR_PID (APP_MOD_MIN_PID + 2)


enum {
  //! 128 Default Application Id. NOT GUARANTEED TO BE UNIQUE !!
  DFLT_APP_ID0  =   (APP_MOD_MIN_PID + 0),

  //! 129 Default Application Id. NOT GUARANTEED TO BE UNIQUE !!
  DFLT_APP_ID1  =   (APP_MOD_MIN_PID + 1),

  //! 130 Default Application Id. NOT GUARANTEED TO BE UNIQUE !!
  DFLT_APP_ID2  =   (APP_MOD_MIN_PID + 2),

  //! 131 Default Application Id. NOT GUARANTEED TO BE UNIQUE !!
  DFLT_APP_ID3  =   (APP_MOD_MIN_PID + 3),

  //! 132 Magnetic Sensor Driver
  MAG_SENSOR_PID  = (APP_MOD_MIN_PID + 4),

  //! 133 Neighbourhood Discovery
  NBHOOD_PID     =  (APP_MOD_MIN_PID + 5),

  //! 134 Tiny Diffusion Protocol
  TD_PROTO_PID  =   (APP_MOD_MIN_PID + 6),

  //! 135 Tiny Diffusion Engine
  TD_ENGINE_PID =   (APP_MOD_MIN_PID + 7),

  //! 136 Module Daemon PC Front-end PID
  MOD_D_PC_PID  =   (APP_MOD_MIN_PID + 8),

  //! 137 Function registry test server module
  MOD_FN_S_PID  =   (APP_MOD_MIN_PID + 9),

  //! 138 Function registry test client module
  MOD_FN_C_PID  =   (APP_MOD_MIN_PID + 10),

  //! 139 Aggregation tree module
  MOD_AGG_TREE_PID  =   (APP_MOD_MIN_PID + 11),

  //! 140 Flooding routing module
  MOD_FLOODING_PID  =   (APP_MOD_MIN_PID + 12),

  //! 141 Tree routing module
  TREE_ROUTING_PID  =  (APP_MOD_MIN_PID + 13),

  //! 142 Surge Application module
  SURGE_MOD_PID   =  (APP_MOD_MIN_PID + 14),

  //! 143 Beef Application module
  BEEF_MOD_PID   =  (APP_MOD_MIN_PID + 15),

  //! 144 Light Potentiometer
  LITEPOT_PID    =  (APP_MOD_MIN_PID + 16),

  //! 145 Photo Sensor
  PHOTOTEMP_SENSOR_PID = (APP_MOD_MIN_PID + 17),

  //! 146 sounder
  SOUNDER_PID    =  (APP_MOD_MIN_PID + 18),

  //! 147 sounder
  // Ram - Two sounders ??
  ACK_MOD_PID    =  (APP_MOD_MIN_PID + 19),

  //! 148 AODV1 Module
  AODV_PID       =  (APP_MOD_MIN_PID + 20),

  //! 149 AODV2 Module
  AODV2_PID      =  (APP_MOD_MIN_PID + 21),

  //! 150 GPSR routing module
  GPSR_MOD_PID   =   (APP_MOD_MIN_PID + 22),

  //! 151 Routing client module
  CLIENT_MOD_PID =   (APP_MOD_MIN_PID + 23),

  //! 152 - I2C Packet Module
  I2CPACKET_PID  =  (APP_MOD_MIN_PID + 24),

  //! 153 - Viz Packet for visualization
  VIZ_PID_0  =  (APP_MOD_MIN_PID + 25),

  //! 154 - Viz Packet for visualization
  VIZ_PID_1  =  (APP_MOD_MIN_PID + 26),

  //! 155 - Temperate Sensor
  ACCEL_SENSOR_PID = (APP_MOD_MIN_PID + 27),

  //! 156 - Camera Driver Module
  CAMERA_PID = (APP_MOD_MIN_PID + 28),

  //! 157 - Blink Communication Module
  BLINK_PID = (APP_MOD_MIN_PID + 29),

  //! 158 - MICA WEATHER BOARD SERIAL SWITCH
  SERIAL_SWITCH_PID = (APP_MOD_MIN_PID + 30),

  //! 159 - TPSN - Time Sync. Module
  TPSN_TIMESYNC_PID = (APP_MOD_MIN_PID + 31),

  //! 160 - RATS - Rate Adaptive Time Synch.
  RATS_TIMESYNC_PID = (APP_MOD_MIN_PID + 32),

  //! 161 - Crypto - Symmetric Crypto Library
  CRYPTO_SYMMETRIC_PID = (APP_MOD_MIN_PID + 33),

  //! 162 - RFSN - Reputation Framework (Security)
  RFSN_PID = (APP_MOD_MIN_PID + 34),

  //! 163 - Outlier Detector (RFSN)
  OUTLIER_PID = (APP_MOD_MIN_PID + 35),

  //! 164 
  PHOTO_SENSOR_PID = (APP_MOD_MIN_PID + 36),
//! PLEASE add the name to modules/mod_pid.c
};

#ifdef PC_PLATFORM
extern char mod_pid_name[][256];
#endif
#endif
