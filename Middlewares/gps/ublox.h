/*
 * ublox.h
 *
 *  Created on: Dec 24, 2017
 *      Author: parallels
 */

#ifndef GPS_UBLOX_H_
#define GPS_UBLOX_H_

#include "gps.h"

#define GPS_I2C_ADDRESS ((uint8_t)0b1000010) //0x42
#define GPS_I2C_ADDRESS_SHIFT (GPS_I2C_ADDRESS << 1)

#define UBX_SYNC_BYTE_1 0xB5
#define UBX_SYNC_BYTE_2 0x62

// UBX Protocol Class/Message ID's
#define MSG_CLASS_ACK 0X05
    #define MSG_ID_ACK_ACK 0x01
    #define MSG_ID_ACK_NAK 0x00
#define MSG_CLASS_AID 0x0B
    #define MSG_ID_AID_ALM 0x30
    #define MSG_ID_AID_ALPSRV 0X32
    #define MSG_ID_AID_ALP 0x50
    #define MSG_ID_AID_AOP 0x33
    #define MSG_ID_AID_DATA 0x10
    #define MSG_ID_AID_EPH 0x31
    #define MSG_ID_AID_HUI 0x02
    #define MSG_ID_AID_INI 0x01
    #define MSG_ID_AID_REQ 0x00
#define MSG_CLASS_CFG 0x06
    #define MSG_ID_CFG_ANT 0X13
    #define MSG_ID_CFG_CNFGR 0x09
    #define MSG_ID_CFG_DAT 0x06
    #define MSG_ID_CFG_EKF 0x12
    #define MSG_ID_CFG_ESFGWT 0x29
    #define MSG_ID_CFG_FXN 0x0E
    #define MSG_ID_CFG_ITFM 0x39
    #define MSG_ID_CFG_MSG 0x01
    #define MSG_ID_CFG_NAV5 0x24
    #define MSG_ID_CFG_NAVX5 0x23
    #define MSG_ID_CFG_NMEA 0x17
    #define MSG_ID_CFG_NVS 0x22
    #define MSG_ID_CFG_PM2 0x3B
    #define MSG_ID_CFG_PM 0x32
    #define MSG_ID_CFG_PRT 0x00
    #define MSG_ID_CFG_RATE 0x08
    #define MSG_ID_CFG_RINV 0x34
    #define MSG_ID_CFG_RST 0x04
    #define MSG_ID_CFG_RXM 0x11
    #define MSG_ID_CFG_SBAS 0x16
    #define MSG_ID_CFG_TMODE2 0x3D
    #define MSG_ID_CFG_TMODE 0x1D
    #define MSG_ID_CFG_TP5 0x31
    #define MSG_ID_CFG_TP 0x07
    #define MSG_ID_CFG_USB 0x1B
	#define MSG_ID_CFG_INF 0x02
	#define MSG_ID_CFG_GNSS 0x3E
	#define MSG_ID_CFG_GEOFENCE 0x69
	#define MSG_ID_CFG_CFG 0x09
#define MSG_CLASS_ESF 0x10
    #define MSG_ID_ESF_MEAS 0x02
    #define MSG_ID_ESF_STATUS 0x10
#define MSG_CLASS_INF 0x04
    #define MSG_ID_INF_DEBUG 0x04
    #define MSG_ID_INF_ERROR 0x00
    #define MSG_ID_INF_NOTICE 0x02
    #define MSG_ID_INF_TEST 0x03
    #define MSG_ID_INF_WARNING 0x01
#define MSG_CLASS_MON 0x0A
    #define MSG_ID_MON_HW2 0x0B
    #define MSG_ID_MON_HW 0x09
    #define MSG_ID_MON_IO 0x02
    #define MSG_ID_MON_MSGPP 0x06
    #define MSG_ID_MON_RXBUF 0x07
    #define MSG_ID_MON_RXR 0x21
    #define MSG_ID_MON_TXBUF 0X08
    #define MSG_ID_MON_VER 0x04
#define MSG_CLASS_NAV 0x01
    #define MSG_ID_NAV_AOPSTATUS 0x60
    #define MSG_ID_NAV_CLOCK 0x22
    #define MSG_ID_NAV_DGPS 0x31
    #define MSG_ID_NAV_DOP 0x04
    #define MSG_ID_NAV_EKFSTATUS 0x40
    #define MSG_ID_NAV_POSECEF 0x01
    #define MSG_ID_NAV_POSLLH 0x02
    #define MSG_ID_NAV_SBAS 0x32
    #define MSG_ID_NAV_SOL 0x06
    #define MSG_ID_NAV_STATUS 0x03
    #define MSG_ID_NAV_SVINFO 0x30
    #define MSG_ID_NAV_TIMEGPS 0x20
    #define MSG_ID_NAV_TIMEUTC 0x21
    #define MSG_ID_NAV_VELECEF 0x11
    #define MSG_ID_NAV_VELNED 0x12
	#define MSG_ID_NAV_PVT 0x07
	#define MSG_ID_NAV_GEOFENCE 0x39
#define MSG_CLASS_RXM 0x02
    #define MSG_ID_RXM_ALM 0x30
    #define MSG_ID_RXM_EPH 0x31
    #define MSG_ID_RXM_PMREQ 0x41
    #define MSG_ID_RXM_RAW 0x10
    #define MSG_ID_RXM_SFRB 0x11
    #define MSG_ID_RXM_SVSI 0x20
#define MSG_CLASS_TIM 0x0D
    #define MSG_ID_TIM_SVIN 0x04
    #define MSG_ID_TIM_TM2 0x03
    #define MSG_ID_TIM_TP 0x01
    #define MSG_ID_TIM_VRFY 0x06

typedef struct
{
	uint8_t sync_char_1;
	uint8_t sync_char_2;
	uint8_t pkt_class;
	uint8_t id;
	uint16_t length; // Little endian
	uint8_t* data;
	uint8_t checksum_a;
	uint8_t checksum_b;
} ubx_packet_t;

typedef struct
{
	uint8_t protocolID;
	uint8_t reserved1[3];
	bitfield8_t infMsgMask[6];
} ubx_cfg_inf_data_t;

typedef struct
{
	uint32_t iTOW;
	uint8_t gpsFix;
	bitfield8_t flags;
	bitfield8_t fixStat;
	bitfield8_t flags2;
	uint32_t ttff;
	uint32_t msss;
} ubx_nav_status_data_t;

typedef struct
{
	bitfield16_t navBbrMask;
	uint8_t resetMode;
	uint8_t reserved1;
} ubx_cfg_rst_data_t;

typedef struct
{
	uint8_t version;
	uint8_t reserved1[3];
	uint32_t duration;
	bitfield32_t flags;
	bitfield32_t wakeupSources;
} ubx_rxm_pmreq_data_t;

typedef struct
{
	bitfield16_t mask;
	uint8_t dynModel;
	uint8_t fixMode;
	int32_t fixedAlt;
	uint32_t fixedAltVar;
	int8_t minElev;
	uint8_t drLimit;
	uint16_t pDop;
	uint16_t tDop;
	uint16_t pAcc;
	uint16_t tAcc;
	uint8_t staticHoldThresh;
	uint8_t dgnssTimeout;
	uint8_t cnoTheshNumSVs;
	uint8_t cnoThresh;
	uint8_t reserved1[2];
	uint16_t staticHoldMaxDist;
	uint8_t utcStandard;
	uint8_t reserved2[5];
} ubx_cfg_nav5_data_t;

typedef struct
{
	uint8_t version;
	uint8_t reserved1;
	uint8_t maxStartupStateDur;
	uint8_t reserved2;
	bitfield32_t flags;
	uint32_t updatePeriod;
	uint32_t searchPeriod;
	uint32_t gridOffset;
	uint16_t onTime;
	uint16_t minAcqTime;
	uint8_t reserved3[20];
	uint32_t extintInactivityMs;
} ubx_cfg_pm2_data_t;

typedef struct
{
	uint8_t msgVer;
	uint8_t numTrkChHw;
	uint8_t numTrkChUse;
	uint8_t numConfigBlocks;
} ubx_cfg_gnss_data_t;

typedef struct
{
	uint8_t gnssId;
	uint8_t resTrkCh;
	uint8_t maxTrkCh;
	uint8_t reserved1;
	bitfield32_t flags;
} ubx_cfg_gnss_data_block_t;

typedef struct
{
	uint8_t reserved1;
	uint8_t lpMode;
} ubx_cfg_rxm_data_t;

typedef struct
{
	bitfield32_t clearMask;
	bitfield32_t saveMask;
	bitfield32_t loadMask;
	//bitfield8_t deviceMask;
} ubx_cfg_cfg_data_t;

typedef struct
{
	bitfield16_t flags;
	bitfield16_t pins;
} ubx_cfg_ant_data_t;

typedef struct
{
	uint8_t msgClass;
	uint8_t msgID;
	uint8_t rate;
} ubx_cfg_msg_data_t;

typedef struct
{
	uint8_t portID;
	uint8_t reserved1;
	bitfield16_t txReady;
	bitfield32_t mode;
	uint8_t reserved2[4];
	bitfield16_t inProtoMask;
	bitfield16_t outProtoMask;
	bitfield16_t flags;
	uint8_t reserved3[2];
} ubx_cfg_prt_ddc_data_t;

typedef struct
{
	uint8_t portID;
	uint8_t reserved1;
	bitfield16_t txReady;
	bitfield32_t mode;
	uint32_t baudRate;
	bitfield16_t inProtoMask;
	bitfield16_t outProtoMask;
	bitfield16_t flags;
	uint8_t reserved2[2];
} ubx_cfg_prt_uart_data_t;

typedef struct
{
	bitfield32_t pinSel;
	bitfield32_t pinBank;
	bitfield32_t pinDir;
	bitfield32_t pinVal;
	uint16_t noisePerMS;
	uint16_t agcCnt;
	uint8_t aStatus;
	uint8_t aPower;
	bitfield8_t flags;
	uint8_t reserved1;
	bitfield32_t usedMask;
	uint8_t VP[17];
	uint8_t jamInd;
	uint8_t reserved2[2];
	bitfield32_t pinIrq;
	bitfield32_t pullH;
	bitfield32_t pullL;
} ubx_mon_hw_data_t;

typedef struct
{
	uint8_t version;
	uint8_t numFences;
	uint8_t confLvl;
	uint8_t reserved1[1];
	uint8_t pioEnabled;
	uint8_t pinPolarity;
	uint8_t pin;
	uint8_t reserved2[1];
	gps_geofence_t fences[4]; // Note that there will be 0-4 of these!
} ubx_cfg_geofence_data_t;

typedef struct
{
	uint32_t iTOW;
	uint8_t version;
	uint8_t status;
	uint8_t numFences;
	uint8_t combState;
	struct
	{
		uint8_t state;
		uint8_t reserved1[1];
	} geofence_state[4];
} ubx_nav_geofence_data_t;

#endif /* GPS_UBLOX_H_ */
