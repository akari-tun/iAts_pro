#pragma once

#include "io/io.h"
#include "util/macros.h"
#include "util/time.h"
#include "util/data_state.h"
#include "tracker/telemetry.h"

#define LTM_START1 0x24 //$
#define LTM_START2 0x54 //T
#define LTM_GFRAME 0x47 //G GPS + Baro altitude data ( Lat, Lon, Speed, Alt, Sats, Sat fix)
#define LTM_AFRAME 0x41 //A Attitude data ( Roll,Pitch, Heading )
#define LTM_SFRAME 0x53 //S Sensors/Status data ( VBat, Consumed current, Rssi, Airspeed, Arm status, Failsafe status, Flight mode )
#define LTM_OFRAME 0x4F //O Origin Frame
#define LTM_NFRAME 0x4E //N Navigation Frame (iNav extension) ( GPS mode, Nav mode, Waypoint number, Nav Error, Flags)
#define LTM_XFRAME 0x58 //X GPS eXended data (iNav extension) ( HDOP, hw status, LTM_X_counter, Disarm Reason, (unused))

#define LTM_GFRAME_SIZE 18
#define LTM_AFRAME_SIZE 10
#define LTM_SFRAME_SIZE 11
#define LTM_OFRAME_SIZE 18
#define LTM_NFRAME_SIZE 10
#define LTM_XFRAME_SIZE 11

#define LTM_BUFFER_SIZE 18
#define LTM_MAX_PAYLOAD_SIZE 14

typedef enum
{
    LTM_IDLE,
    LTM_STATE_START1,
    LTM_STATE_START2,
    LTM_STATE_MSGTYPE,
    LTM_STATE_DATA
} ltm_frame_status_e;

#pragma pack(1)
typedef struct ltm_gframe_s
{
    int32_t latitude; //int32 decimal degrees * 10,000,000 (1E7)
    int32_t longitude; //int32 decimal degrees * 10,000,000 (1E7)
    uint8_t ground_speed; //m/s
    int32_t altitude; //(u)int32, cm (m / 100). In the original specification, this was unsigned. In iNav it is signed and should be so interpreted by consumers
    uint8_t sats; //uchar. bits 0-1 : fix ; bits 2-7 : number of satellites
} ltm_gframe_t;

typedef struct ltm_aframe_s
{
    int16_t pitch; //int16, degrees
    int16_t roll; //int16, degrees
    int16_t heading; //int16, degrees. Course over ground
} ltm_aframe_t;

typedef struct ltm_sframe_s
{
    uint16_t vbat; //uint16, mV
    uint16_t battery; //uint16, Battery Consumption	mAh
    uint8_t rssi; //uchar
    uint8_t airspeed; //uchar, m/s
    uint8_t status; //uchar
} ltm_sframe_t;

typedef struct ltm_oframe_s
{
    int32_t latitude; //int32 decimal degrees * 10,000,000 (1E7)
    int32_t longitude; //int32 decimal degrees * 10,000,000 (1E7)
    uint32_t altitude; //uint32, cm (m / 100) [always 0 in iNav]
    uint8_t osd; //uchar (always 1)
    uint8_t fix; //uchar, home fix status (0 == no fix)
} ltm_oframe_t;

typedef struct ltm_nframe_s
{
    uint8_t gps_mode; //uchar
    uint8_t nav_mode; //uchar
    uint8_t nav_action; //uchar (not all used in inav)
    uint8_t waypoint_number; //uchar, target waypoint
    uint8_t nav_error; //uchar
    uint8_t flags; //uchar (to be defined)
} ltm_nframe_t;

typedef struct ltm_xframe_s
{
    uint8_t hdop; //uint16 HDOP * 100
    uint8_t hw_status; //Note that hw status (hardware sensor status) is iNav 1.5 and later. If the value is non-zero, then a sensor has failed. A complementary update has been made to MSP_STATUS
    uint8_t ltm_x_counter; //The LTM_X_counter value is incremented each transmission and rolls over (modulo 256). It is intended to enable consumers to estimate packet loss.
    uint8_t disarm_reason; //uint8
    int8_t unused; //1 byte
} ltm_xframe_t;
 #pragma pack()
 
typedef struct ltm_s
{
    telemetry_t *plane_vals;
    io_t *io;
    bool home_source;

    uint8_t buf[LTM_BUFFER_SIZE * 2];
    uint8_t payload[LTM_MAX_PAYLOAD_SIZE];
    uint8_t buf_pos;
    uint8_t payload_pos;
    uint8_t length;
    uint8_t function;
    uint8_t crc;
    ltm_frame_status_e status;
    ltm_gframe_t *gframe;
    ltm_aframe_t *aframe;
    ltm_sframe_t *sframe;
    ltm_oframe_t *oframe;
    ltm_nframe_t *nframe;
    ltm_xframe_t *xframe;
} ltm_t;

void ltm_init(ltm_t *ltm);
int ltm_update(ltm_t *ltm, void *data);
bool ltm_decode(ltm_t *ltm, uint8_t c);
void ltm_destroy(ltm_t *ltm);
