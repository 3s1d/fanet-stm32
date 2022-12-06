/*
 * mac.cpp
 *
 *  Created on: 30 Sep 2016
 *      Author: sid
 */

#include <stdlib.h>
#include <math.h>

#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "spi.h"

#include "common.h"

#include "lib/random.h"
#include "sx1272.h"
#include "fmac.h"


/* get next frame which can be sent out */
//todo: this is potentially dangerous, as frm may be deleted in another place.
Frame* MacFifo::get_nexttx()
{
	int next;
	uint32_t prim = __get_PRIMASK();
	__disable_irq();
	for (next = 0; next < fifo.size(); next++)
		if (fifo.get(next)->next_tx < HAL_GetTick())
			break;
	Frame *frm;
	if (next == fifo.size())
		frm = NULL;
	else
		frm = fifo.get(next);
	if (!prim)
		__enable_irq();

	return frm;
}

Frame* MacFifo::frame_in_list(Frame *frm)
{
	uint32_t prim = __get_PRIMASK();
	__disable_irq();

	for (int i = 0; i < fifo.size(); i++)
	{
		Frame *frm_list = fifo.get(i);
		if (*frm_list == *frm)
		{
			if (!prim)
				__enable_irq();
			return frm_list;
		}
	}

	if (!prim)
		__enable_irq();

	return NULL;
}

Frame* MacFifo::front()
{
	uint32_t prim = __get_PRIMASK();
	__disable_irq();
	Frame *frm = fifo.shift();
	if (!prim)
		__enable_irq();

	return frm;
}

/* add frame to fifo */
int MacFifo::add(Frame *frm)
{
	uint32_t prim = __get_PRIMASK();
	__disable_irq();

	/* buffer full */
	/* note: ACKs will always fit */
	if (fifo.size() >= MAC_FIFO_SIZE && frm->type != FRM_TYPE_ACK)
	{
		if (!prim)
			__enable_irq();
		return -1;
	}

	/* only one ack_requested from us to a specific address at a time is allowed in the queue */
	//in order not to screw with the awaiting of ACK
	if (frm->ack_requested)
	{
		for (int i = 0; i < fifo.size(); i++)
		{
			//note: this never succeeds for received packets -> tx condition only
			Frame *ffrm = fifo.get(i);
			if (ffrm->ack_requested && ffrm->src == fmac.myAddr && ffrm->dest == frm->dest)
			{
				if (!prim)
					__enable_irq();
				return -2;
			}
		}
	}

	if (frm->type == FRM_TYPE_ACK)
		/* add to front */
		fifo.unshift(frm);
	else
		/* add to tail */
		fifo.add(frm);

	if (!prim)
		__enable_irq();
	return 0;
}

/* remove frame from linked list and delete it */
bool MacFifo::remove_delete(Frame *frm)
{
	bool found = false;

	uint32_t prim = __get_PRIMASK();
	__disable_irq();
	for (int i = 0; i < fifo.size() && !found; i++)
		if (frm == fifo.get(i))
		{
			delete fifo.remove(i);
			found = true;
		}
	if (!prim)
		__enable_irq();

	return found;
}

/* remove any pending frame that waits on an ACK from a host */
bool MacFifo::remove_delete_acked_frame(MacAddr dest)
{
	bool found = false;
	uint32_t prim = __get_PRIMASK();
	__disable_irq();

	for (int i = 0; i < fifo.size(); i++)
	{
		Frame* frm = fifo.get(i);
		if (frm->ack_requested && frm->dest == dest)
		{
			delete fifo.remove(i);
			found = true;
		}
	}
	if (!prim)
		__enable_irq();
	return found;
}

/* this is executed in a non-linear fashion */
void FanetMac::frameReceived(int length)
{
	/* quickly read registers */
	num_received = sx1272_getFrame(rx_frame, sizeof(rx_frame));
	int rssi = sx1272_getRssi();

#if MAC_debug_mode > 0
	printf("### Mac Rx:%d @ %d ", num_received, rssi);

	for(int i=0; i<num_received; i++)
	{
		printf("02X", rx_frame[i]);
		if(i<num_received-1)
			printf(":");
	}
	printf("\n");
#endif

	/* build frame from stream */
	Frame *frm = new Frame(num_received, rx_frame);
	frm->rssi = rssi;

	/* add to fifo */
	if (rx_fifo.add(frm) < 0)
		delete frm;
}

/* wrapper to fit callback into c++ */
void FanetMac::frameRxWrapper(int length)
{
	fmac.frameReceived(length);
}

bool FanetMac::begin(Fapp &app)
{
	myApp = &app;

	/* configure phy radio */
	if (sx1272_init(HAL_SPI_get()) == false)
		return false;
	sx1272_setBandwidth(BW_250);
	sx1272_setSpreadingFactor(SF_7);
	sx1272_setCodingRate(CR_5);
	sx1272_setExplicitHeader(true);
	sx1272_setSyncWord(MAC_SYNCWORD);
	sx1272_setPayloadCrc(true);
	sx1272_setLnaGain(LNAGAIN_G1_MAX, true);
	sx1272_setIrqReceiver(frameRxWrapper);

	/* region specific. default is EU */
	sx_region_t region = zones[NELEM(zones)-1].mac;
	region.dBm -= antGain;
	sx1272_setRegion(region);

	/* enter sleep mode */
	sx1272_setArmed(false);

	/* address */
	_myAddr = readAddr();

	/* start state machine */
	myTimer.Start();

	/* start random machine */
	randomSeed(HAL_GetUIDw0() + HAL_GetUIDw1() + HAL_GetUIDw2());

	return true;
}

/* wrapper to fit callback into c++ */
void FanetMac::stateWrapper()
{
	fmac.handleRx();
	fmac.handleTx();

	/* radio chip might be broken */
	//note: postpone (10x) next reset in case nobody is around
	if(HAL_GetTick() > fmac.lastRx + MAC_RESET_TIMEOUT and sx1272_reset(true))
		fmac.lastRx = HAL_GetTick() + 10*MAC_RESET_TIMEOUT;
}

bool FanetMac::isNeighbor(MacAddr addr)
{
	for (int i = 0; i < neighbors.size(); i++)
		if (neighbors.get(i)->addr == addr)
			return true;

	return false;
}

/*
 * Generates ACK frame
 */
void FanetMac::ack(Frame* frm)
{
#if MAC_debug_mode > 0
	printf("### generating ACK\n");
#endif

	/* generate reply */
	Frame *ack = new Frame(myAddr);
	ack->type = FRM_TYPE_ACK;
	ack->dest = frm->src;

	/* only do a 2 hop ACK in case it was requested and we received it via a two hop link (= forward bit is not set anymore) */
	if (frm->ack_requested == FRM_ACK_TWOHOP && !frm->forward)
		ack->forward = true;

	/* add to front of fifo */
	//note: this will not fail by define
	if (tx_fifo.add(ack) != 0)
		delete ack;
}

/*
 * Processes stuff from rx_fifo
 */
void FanetMac::handleRx()
{
	/* nothing to do */
	if (rx_fifo.size() == 0)
	{
		/* clean neighbors list */
		for (int i = 0; i < neighbors.size(); i++)
		{
			if (neighbors.get(i)->isAround() == false)
				delete neighbors.remove(i);
		}

		return;
	}

	Frame *frm = rx_fifo.front();
	if(frm == nullptr)
		return;

	/* store rx timestamp */
	lastRx = HAL_GetTick();

	/* build up neighbors list */
	bool neighbor_known = false;
	for (int i = 0; i < neighbors.size(); i++)
	{
		if (neighbors.get(i)->addr == frm->src)
		{
			/* update presents */
			neighbors.get(i)->seen();
			if(frm->type == FRM_TYPE_TRACKING || frm->type == FRM_TYPE_GROUNDTRACKING)
				neighbors.get(i)->hasTracking = true;
			neighbor_known = true;
			break;
		}
	}

	/* neighbor unknown until now, add to list */
	if (neighbor_known == false)
	{
		/* too many neighbors, delete oldest member */
		if (neighbors.size() > MAC_NEIGHBOR_SIZE)
			delete neighbors.shift();

		neighbors.add(new NeighborNode(frm->src, frm->type == FRM_TYPE_TRACKING || frm->type == FRM_TYPE_GROUNDTRACKING));
	}

	/* is the frame a forwarded one and is it still in the tx queue? */
	Frame *frm_list = tx_fifo.frame_in_list(frm);
	if (frm_list != NULL)
	{
		/* frame already in tx queue */

		if (frm->rssi > frm_list->rssi + MAC_FORWARD_MIN_DB_BOOST)
		{
			/* somebody broadcasted it already towards our direction */
#if MAC_debug_mode > 0
			printf("### rx frame better than org. dropping both.\n");
#endif
			/* received frame is at least 20dB better than the original -> no need to rebroadcast */
			tx_fifo.remove_delete(frm_list);
		}
		else
		{
#if MAC_debug_mode >= 2
			printf("### adjusting tx time");
#endif
			/* adjusting new departure time */
			frm_list->next_tx = HAL_GetTick() + random(MAC_FORWARD_DELAY_MIN, MAC_FORWARD_DELAY_MAX);
		}
	}
	else
	{
		if ((frm->dest == MacAddr() || frm->dest == myAddr) && frm->src != myAddr)
		{
			/* a relevant frame */
			if (frm->type == FRM_TYPE_ACK)
			{
				if (tx_fifo.remove_delete_acked_frame(frm->src) && myApp != NULL)
					myApp->handle_acked(true, frm->src);
			}
			else
			{
				/* generate ACK */
				if (frm->ack_requested)
					ack(frm);

				/* forward frame */
				if (myApp != NULL)
					myApp->handle_frame(frm);
			}
		}

		/* Forward frame */
		if (doForward && frm->forward && tx_fifo.size() < MAC_FIFO_SIZE - 3 && frm->rssi <= MAC_FORWARD_MAX_RSSI_DBM
				&& (frm->dest == MacAddr() || isNeighbor(frm->dest)) && sx1272_get_airlimit() < 0.5f)
		{
#if MAC_debug_mode >= 2
			printf("### adding new forward frame\n");
#endif
			/* prevent from re-forwarding */
			frm->forward = false;

			/* generate new tx time */
			frm->next_tx = HAL_GetTick() + random(MAC_FORWARD_DELAY_MIN, MAC_FORWARD_DELAY_MAX);
			frm->num_tx = !!frm->ack_requested;

			/* add to list */
			tx_fifo.add(frm);
			return;
		}
	}

	/* discard frame */
	delete frm;
}

/*
 * get a from from tx_fifo (or the app layer) and transmit it
 */
void FanetMac::handleTx()
{
	/* still in backoff or chip turned off*/
	if (HAL_GetTick() < csma_next_tx  || !sx1272_isArmed())
		return;

	/* ensure we are at the correct freq */
	if(freqApproved == false and approveFreq() == false)
		return;

	/* find next send-able packet */
	/* this breaks the layering. however, this approach is much more efficient as the app layer now has a much higher priority */
	Frame* frm;
	bool app_tx = false;
	if (myApp->is_broadcast_ready(neighbors.size()))
	{
		/* the app wants to broadcast the glider state */
		frm = myApp->get_frame();
		if (frm == NULL)
			return;

		if (neighbors.size() <= MAC_MAXNEIGHBORS_4_TRACKING_2HOP)
			frm->forward = true;
		else
			frm->forward = false;

		app_tx = true;
	}
	else if(sx1272_get_airlimit() < 0.9f)
	{
#if MAC_debug_mode >= 2
		static int queue_length = 0;
		int current_qlen = tx_fifo.size();
		if(current_qlen != queue_length)
		{
			printf("### tx queue: %d\n", current_qlen);
			queue_length = current_qlen;
		}
#endif

		/* get a frame from the fifo */
		frm = tx_fifo.get_nexttx();
		if (frm == nullptr)
			return;

		/* frame w/o a received ack and no more re-transmissions left */
		if (frm->ack_requested && frm->num_tx <= 0)
		{
#if MAC_debug_mode > 0
			printf("### Frame, 0x%02X NACK!\n", frm->type);
#endif
			if (myApp != nullptr && frm->src == myAddr)
				myApp->handle_acked(false, frm->dest);
			tx_fifo.remove_delete(frm);
			return;
		}

		/* unicast frame w/o forwarding and it is not a direct neighbor */
		if (frm->forward == false && frm->dest != MacAddr() && isNeighbor(frm->dest) == false)
			frm->forward = true;

		app_tx = false;
	}
	else
	{
		return;
	}

	/* serialize frame */
	uint8_t* buffer;
	int blength = frm->serialize(buffer);
	if (blength < 0)
	{
#if MAC_debug_mode > 0
		printf("### Problem serialization type 0x%02X. removing.\n", frm->type);
#endif
		/* problem while assembling the frame */
		if (app_tx)
			delete frm;
		else
			tx_fifo.remove_delete(frm);
		return;
	}

#if MAC_debug_mode > 0
	printf("### Sending, 0x%02X... ", frm->type);
#endif

#if MAC_debug_mode >= 4
	/* print hole packet */
	printf(" ");
	for(int i=0; i<blength; i++)
	{
		printf("%02X", buffer[i]);
		if(i<blength-1)
			printf(":");
	}
	printf(" ");
#endif

	/* channel free and transmit? */
	//note: for only a few nodes around, increase the coding rate to ensure a more robust transmission
	int tx_ret = sx1272_sendFrame(buffer, blength, neighbors.size() < MAC_CODING48_THRESHOLD ? CR_8 : CR_5);
	delete[] buffer;

	if (tx_ret == TX_OK)
	{
#if MAC_debug_mode > 0
		printf("done.\n");
#endif

		if (app_tx)
		{
			/* app tx */
			myApp->broadcast_successful(frm->type);
			delete frm;
		}
		else
		{
			/* fifo tx */

			/* transmission successful */
			if (!frm->ack_requested || frm->src != myAddr)
			{
				/* remove frame from FIFO */
				tx_fifo.remove_delete(frm);
			}
			else
			{
				/* update next transmission */
				if (--frm->num_tx > 0)
					frm->next_tx = HAL_GetTick() + (MAC_TX_RETRANSMISSION_TIME * (MAC_TX_RETRANSMISSION_RETRYS - frm->num_tx));
				else
					frm->next_tx = HAL_GetTick() + MAC_TX_ACKTIMEOUT;
			}
		}

		/* ready for a new transmission in */
		csma_backoff_exp = MAC_TX_BACKOFF_EXP_MIN;
		csma_next_tx = HAL_GetTick() + MAC_TX_MINPREAMBLEHEADERTIME_MS + (blength * MAC_TX_TIMEPERBYTE_MS);
	}
	else if (tx_ret == TX_RX_ONGOING || tx_ret == TX_TX_ONGOING)
	{
#if MAC_debug_mode > 0
		if(tx_ret == TX_RX_ONGOING)
			printf("rx, abort.\n");
		else
			printf("tx not done yet, abort.\n");
#endif

		if (app_tx)
			delete frm;

		/* channel busy, increment backoff exp */
		if (csma_backoff_exp < MAC_TX_BACKOFF_EXP_MAX)
			csma_backoff_exp++;

		/* next tx try */
		csma_next_tx = HAL_GetTick() + random(1 << (MAC_TX_BACKOFF_EXP_MIN - 1), 1 << csma_backoff_exp);

#if MAC_debug_mode > 1
		printf("### backoff %lums\n", csma_next_tx - HAL_GetTick());
#endif
	}
	else
	{
		/* ignoring TX_TX_ONGOING */
#if MAC_debug_mode > 2
		printf("## WAT: %d\n", tx_ret);
#endif

		if (app_tx)
			delete frm;
	}
}

uint16_t FanetMac::numTrackingNeighbors(void)
{
	uint16_t num = 0;
	for (uint16_t i = 0; i < neighbors.size(); i++)
		if (neighbors.get(i)->hasTracking)
			num++;

	return num;
}

MacAddr FanetMac::readAddr(void)
{
	uint64_t addr_container = *(__IO uint64_t*)MAC_ADDR_BASE;

	/* identify container */
	if((addr_container & MAC_ADDR_MAGIC_MASK) != MAC_ADDR_MAGIC)
	{
#if MAC_debug_mode > 0
		printf("## No Addr set!\n");
#endif
		return MacAddr();
	}

	return MacAddr((addr_container>>16) & 0xFF, addr_container & 0xFFFF);
}

bool FanetMac::setAddr(MacAddr addr)
{
	/* test for clean storage */
	if(*(__IO uint64_t*)MAC_ADDR_BASE != UINT64_MAX)
		return false;

	/* build config */
	uint64_t addr_container = MAC_ADDR_MAGIC | (addr.manufacturer&0xFF)<<16 | (addr.id&0xFFFF);

	HAL_FLASH_Unlock();
	HAL_StatusTypeDef flash_ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, MAC_ADDR_BASE, addr_container);
	HAL_FLASH_Lock();

	if(flash_ret == HAL_OK)
		_myAddr = addr;

	return (flash_ret == HAL_OK);
}

bool FanetMac::eraseAddr(void)
{
	/* determine page */
	FLASH_EraseInitTypeDef eraseInit = {0};
	eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	eraseInit.Banks = FLASH_BANK_1;
	eraseInit.Page = MAC_ADDR_PAGE;
	eraseInit.NbPages = 1;

	uint32_t sectorError = 0;
	HAL_FLASH_Unlock();
	HAL_StatusTypeDef ret = HAL_FLASHEx_Erase(&eraseInit, &sectorError);
	HAL_FLASH_Lock();

	return (ret == HAL_OK && sectorError == UINT32_MAX);
}

bool FanetMac::approveFreq(void)
{
	if(freqApproved)
		return freqApproved;

	if(std::isnan(myApp->getPos_deg().latitude) or myApp->getPos_deg() == Coordinate2D())
	{
		/* no gnss fix -> no freq range for sure */
		freqNoFixTill = HAL_GetTick();
		return false;
	}
	else if(HAL_GetTick() - freqNoFixTill < 5000)
	{
		/* fix not old enough */
		return false;
	}

	/* fix for >5sec */
	const Coordinate2D pos_rad = Coordinate2D(deg2rad(myApp->getPos_deg().latitude), deg2rad(myApp->getPos_deg().longitude));
	for(uint16_t i=0; i<NELEM(zones); i++)
	{
		if(zones[i].bb.isInside(pos_rad))
		{
			bool reArm = false;
			if(sx1272_isArmed())
				reArm = sx1272_setArmed(false);

			sx_region_t region = zones[i].mac;
			region.dBm -= antGain;
			freqApproved = sx1272_setRegion(region);
			if(freqApproved)
				freqIdx = i;

			if(reArm)
				sx1272_setArmed(true);
			break;
		}
	}

	return freqApproved;
}

void FanetMac::setZone(uint16_t idx)
{
	if(idx >= NELEM(zones))
		return;

	/* manually adjust zone */
	bool reArm = false;
	if(sx1272_isArmed())
		reArm = sx1272_setArmed(false);

	sx_region_t region = zones[idx].mac;
	region.dBm -= antGain;
	freqApproved = sx1272_setRegion(region);
	if(freqApproved)
		freqIdx = idx;

	if(reArm)
		sx1272_setArmed(true);
}

const char *FanetMac::getZoneName(void)
{
	if(freqIdx < 0 or freqIdx >= (int16_t) NELEM(zones))
		return nullptr;
	return zones[freqIdx].name;
}

uint32_t FanetMac::getChannel(void)
{
	if(freqIdx < 0 or freqIdx >= (int16_t) NELEM(zones))
		return 0;
	return zones[freqIdx].mac.channel;
}

FanetMac fmac = FanetMac();
