/*
 * mac.h
 *
 *  Created on: 30 Sep 2016
 *      Author: sid
 */

#ifndef FANET_STACK_FMAC_H_
#define FANET_STACK_FMAC_H_

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "../misc/boundingbox.h"
#include "sx1272.h"

/*
 * Hard coded tx time assumption:
 * -SR7
 * -BW250
 * -CR8 (worst case)
 *
 * payload (byte) -> airtime (ms) -> airtime per byte payload (ms)
 * 0		9.43		0
 * 1		13.44		4.1
 * 2-5		17.54		4-1.63
 * 10		25.73		1.63
 * 64		87.17		1.2
 * 201		246.91		1.18
 * (number according to LoRa calculator)
 *
 * -> tx time assumption:
 * 15ms + 2*payload(bytes)
 * MAC_TX_MINHEADERTIME_MS + (blength * MAC_TX_TIMEPERBYTE_MS)
 */

/*
 * Timing defines
 * ONLY change if you know what you are doing. Can destroy the hole nearby network!
 */

#define MAC_SLOT_MS							20

#define MAC_TX_MINPREAMBLEHEADERTIME_MS		15
#define MAC_TX_TIMEPERBYTE_MS				2
#define MAC_TX_ACKTIMEOUT					1000
#define MAC_TX_RETRANSMISSION_TIME			1000
#define MAC_TX_RETRANSMISSION_RETRYS		3
#define MAC_TX_BACKOFF_EXP_MIN				7
#define MAC_TX_BACKOFF_EXP_MAX				12

#define MAC_FORWARD_MAX_RSSI_DBM			-90		//todo test
#define MAC_FORWARD_MIN_DB_BOOST			20
#define MAC_FORWARD_DELAY_MIN				100
#define MAC_FORWARD_DELAY_MAX				300

#define NEIGHBOR_MAX_TIMEOUT_MS				250000		//4min + 10sek

#define MAC_SYNCWORD						0xF1

#define MAC_RESET_TIMEOUT					70000

/*
 * Number defines
 */
#define MAC_NEIGHBOR_SIZE					64
#define MAC_MAXNEIGHBORS_4_TRACKING_2HOP	5
#define MAC_CODING48_THRESHOLD				8

#define MAC_FIFO_SIZE						8
#define MAC_FRAME_LENGTH					254

#define MAC_FLASH_PAGESIZE			2048
#define MAC_ADDR_PAGE				((((uint16_t)(READ_REG(*((uint32_t *)FLASHSIZE_BASE)))) * 1024)/MAC_FLASH_PAGESIZE - 1)
#define MAC_ADDR_BASE				(FLASH_BASE + MAC_ADDR_PAGE*MAC_FLASH_PAGESIZE)
#define MAC_ADDR_MAGIC				0x1337000000000000ULL
#define MAC_ADDR_MAGIC_MASK			0xFFFF000000000000ULL

/* Debug */
#define MAC_debug_mode				0
#if !defined(DEBUG) && !defined(DEBUG_SEMIHOSTING) && MAC_debug_mode > 0
	#undef MAC_debug_mode
	#define MAC_debug_mode 0
#endif


//#include "main.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"

#include "lib/LinkedList.h"
#include "lib/TimerObject.h"

#include "frame.h"

/* note: zero copy stack might be faster and more memory efficient, but who cares @ 9kBaud and 64Ks of ram... */

class NeighborNode
{
private:
	unsigned long last_seen;
public:
	const MacAddr addr;
	bool hasTracking;

	NeighborNode(MacAddr addr, bool tracking = false) : addr(addr), hasTracking(tracking) { last_seen = HAL_GetTick(); }
	void seen(void) { last_seen = HAL_GetTick(); }
	bool isAround(void) { return last_seen + NEIGHBOR_MAX_TIMEOUT_MS > HAL_GetTick(); }
};

class Fapp
{
public:
	Fapp() { }
	virtual ~Fapp() { }

	/* device -> air */
	virtual bool is_broadcast_ready(int num_neighbors) = 0;
	virtual void broadcast_successful(int type) = 0;
	virtual Frame *get_frame() = 0;

	/* air -> device */
	virtual void handle_acked(bool ack, MacAddr &addr) = 0;
	virtual void handle_frame(Frame *frm) = 0;

	virtual const Coordinate2D &getPos_deg(void) = 0;
};

class MacFifo
{
private:
	LinkedList<Frame*> fifo;
public:
	/* not usable in async mode */
	Frame* get_nexttx();
	Frame* frame_in_list(Frame *frm);
	Frame* front();

	/* usable in async mode */
	bool remove_delete_acked_frame(MacAddr dest);
	bool remove_delete(Frame *frm);
	int add(Frame *frm);
	int size() { return fifo.size(); }
};

class FanetMac
{
public:
	typedef struct
	{
		const char* name;
		sx_region_t mac;
		BoundingBox bb;
	} region_t;

	int8_t antGain = 3;

	//BoundingBox(float topLat, float btmLat, float rightLon, float leftLon)
	const region_t zones[7] =
	{
			{
					.name = "US920",
					.mac = {.channel = CH_920_800, .dBm = 15, .bw = BW_500},
					.bb = BoundingBox(deg2rad(90), deg2rad(-90), deg2rad(-30), deg2rad(-169))
			},
			{
					.name = "AU920",
					.mac = {.channel = CH_920_800, .dBm = 15, .bw = BW_500},
					.bb = BoundingBox(deg2rad(-10), deg2rad(-48), deg2rad(179), deg2rad(110))
			},
			{
					.name = "IN866",
					.mac = {.channel = CH_866_200, .dBm = 14, .bw = BW_250},
					.bb = BoundingBox(deg2rad(40), deg2rad(5), deg2rad(89), deg2rad(69))
			},
			{
					.name = "KR923",
					.mac = {.channel = CH_923_200, .dBm = 15, .bw = BW_125},
					.bb = BoundingBox(deg2rad(39), deg2rad(34), deg2rad(130), deg2rad(124))
			},
			{
					.name = "AS920",
					.mac = {.channel = CH_923_200, .dBm = 15, .bw = BW_125},
					.bb = BoundingBox(deg2rad(47), deg2rad(21), deg2rad(146), deg2rad(89))
			},
			{
					.name = "IL918",
					.mac = {.channel = CH_918_500, .dBm = 15, .bw = BW_125},
					.bb = BoundingBox(deg2rad(34), deg2rad(29), deg2rad(36), deg2rad(34))
			},
			{		//default
					.name = "EU868",
					.mac = {.channel = CH_868_200, .dBm = 14, .bw = BW_250},
					.bb = BoundingBox(deg2rad(90), deg2rad(-90), deg2rad(180), deg2rad(-180))
			}
	};

private:
	TimerObject myTimer;
	MacFifo tx_fifo;
	MacFifo rx_fifo;
	LinkedList<NeighborNode *> neighbors;
	Fapp *myApp = NULL;
	MacAddr _myAddr;

	unsigned long csma_next_tx = 0;
	int csma_backoff_exp = MAC_TX_BACKOFF_EXP_MIN;

	/* used for interrupt handler */
	uint8_t rx_frame[MAC_FRAME_LENGTH];
	int num_received = 0;
	uint32_t lastRx = 0;

	bool freqApproved = false;
	int16_t freqIdx = -1;
	uint32_t freqNoFixTill = 0;

	static void frameRxWrapper(int length);
	void frameReceived(int length);

	void ack(Frame* frm);

	bool approveFreq(void);

	static void stateWrapper();
	void handleTx();
	void handleRx();

	bool isNeighbor(MacAddr addr);

public:
	bool doForward = true;

	FanetMac() : myTimer(MAC_SLOT_MS, stateWrapper), myAddr(_myAddr) { }
	~FanetMac() { }

	bool begin(Fapp &app);
	void handle() { myTimer.Update(); }
	bool setPower(bool pwr)
	{
		/* forward rx mark into future */
		if(pwr)
			lastRx = HAL_GetTick() + MAC_RESET_TIMEOUT;
		return sx1272_setArmed(pwr);
	}

	bool txQueueDepleted(void) { return (tx_fifo.size() == 0); }
	bool txQueueHasFreeSlots(void){ return (tx_fifo.size() < MAC_FIFO_SIZE); }
	int transmit(Frame *frm) { return tx_fifo.add(frm); }

	uint16_t numNeighbors(void) { return neighbors.size(); }
	uint16_t numTrackingNeighbors(void);

	/* Addr */
	const MacAddr &myAddr;
	bool setAddr(MacAddr addr);
	bool eraseAddr(void);
	MacAddr readAddr();

	/* zone */
	const char *getZoneName(void);
	uint32_t getChannel(void);
	int16_t getZone(void) { return freqIdx; }
	void setZone(uint16_t idx);
};

extern FanetMac fmac;

#endif /* FANET_STACK_FMAC_H_ */
