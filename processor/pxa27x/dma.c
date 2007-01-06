#include <hardware.h>
#include "dma.h"

#define atomic


void PXA27XHPLDMA_setByteAlignment(uint32_t channel, bool enable)
{
	if (channel < 32) {
		if (enable) {
			DALGN |= (1 << channel);
		}
		else {
			DALGN &= ~(1 << channel);
		}
	}
	return;
}

void PXA27XHPLDMA_mapChannel(uint32_t channel,uint16_t peripheralID)
{
	if(channel < 32){
		DRCMR(peripheralID) = DRCMR_CHLNUM(channel) | DRCMR_MAPVLD;
	}
	return;
}

void PXA27XHPLDMA_unmapChannel(uint32_t channel)
{
	if(channel < 32){
		DRCMR(channel) = 0;
	}
}

void PXA27XHPLDMA_setDCSR(uint32_t channel,uint32_t val)
{
	if (channel < 32) {
		DCSR(channel) = val;
	}
	return;
}

uint32_t PXA27XHPLDMA_getDCSR(uint32_t channel)
{
	uint32_t val;
	if (channel < 32) {
		val = DCSR(channel);
		return val;
	}
	return 0;
}

void PXA27XHPLDMA_setDCMD(uint32_t channel, uint32_t val)
{
	if (channel < 32) {
		DCMD(channel) = val;
	}
	return;
}

uint32_t PXA27XHPLDMA_getDCMD(uint32_t channel)
{
	uint32_t val;
	if (channel < 32) {
		val = DCMD(channel);
		return val;
	}
	return 0;
}

void PXA27XHPLDMA_setDDADR(uint32_t channel ,uint32_t val)
{
	if (channel < 32) {
		DDADR(channel) = val;
	}
	return;
}

uint32_t PXA27XHPLDMA_getDDADR(uint32_t channel)
{
	uint32_t val;
	if (channel < 32) {
		val = DDADR(channel);
		return val;
	}
	return 0;
}

void PXA27XHPLDMA_setDSADR(uint32_t channel, uint32_t val)
{
	if (channel < 32) {
		DSADR(channel) = val;
	}
	return;
}

uint32_t PXA27XHPLDMA_getDSADR(uint32_t channel)
{
	uint32_t val;
	if (channel < 32) {
		val = DSADR(channel);
		return val;
	}
	return 0;
}

void PXA27XHPLDMA_setDTADR(uint32_t channel, uint32_t val)
{
	if (channel < 32) {
		DTADR(channel) = val;
	}
	return;
}

uint32_t PXA27XHPLDMA_getDTADR(uint32_t channel)
{
	uint32_t val;
	if (channel < 32) {
		val = DTADR(channel);
		return val;
	}
	return 0;
}

#if 0
//we don't expose any of the external DMA pins, so no sense in exposing this.  However, there's also no sense in deleting it...
async command uint8_t PXA27XDMAExtReq.getDREQPend[uint8_t pin]()
{
	uint8_t val;
	if (pin < 3) {
		atomic val = (DRQSR(pin) & 0x1F);
	}
	return val;
}

async command void PXA27XDMAExtReq.clearDREQPend[uint8_t pin]()
{
	if (pin < 3) {
		atomic DRQSR(pin) = DRQSR_CLR;
	}
	return;
}
#endif

uint32_t PXA27XHPLDMA_getDPCSR()
{
	return DPCSR;
}

void PXA27XHPLDMA_setDPCSR(uint32_t val)
{
	DPCSR = val;
	return;
}



void DMA_setSourceAddr(DMADescriptor_t* descPtr, uint32_t val)
{
	atomic{ descPtr->DSADR = val; }
}

void DMA_setTargetAddr(DMADescriptor_t* descPtr, uint32_t val)
{
	atomic{ descPtr->DTADR = val; }
}

void DMA_enableSourceAddrIncrement(DMADescriptor_t* descPtr, bool enable)
{
	atomic{ descPtr->DCMD = (enable == TRUE) ? descPtr->DCMD | DCMD_INCSRCADDR : descPtr->DCMD & ~DCMD_INCSRCADDR; }
}

void DMA_enableTargetAddrIncrement(DMADescriptor_t* descPtr, bool enable)
{
	atomic{ descPtr->DCMD = (enable == TRUE) ? descPtr->DCMD | DCMD_INCTRGADDR : descPtr->DCMD & ~DCMD_INCTRGADDR; }
}

void DMA_enableSourceFlowControl(DMADescriptor_t* descPtr, bool enable)
{
	atomic{descPtr->DCMD = (enable == TRUE) ? descPtr->DCMD | DCMD_FLOWSRC : descPtr->DCMD & ~DCMD_FLOWSRC;}
}

void DMA_enableTargetFlowControl(DMADescriptor_t* descPtr, bool enable)
{
	descPtr->DCMD = (enable == TRUE) ? descPtr->DCMD | DCMD_FLOWTRG : descPtr->DCMD & ~DCMD_FLOWTRG;
}

void DMA_setMaxBurstSize(DMADescriptor_t* descPtr, DMAMaxBurstSize_t size)
{
	if(size >= DMA_8ByteBurst && size <= DMA_32ByteBurst){
		atomic{
		//clear it out since otherwise |'ing doesn't work so well
		descPtr->DCMD &= ~DCMD_MAXSIZE;
		descPtr->DCMD |= DCMD_SIZE(size);
		}
	}
}

void DMA_setTransferLength(DMADescriptor_t* descPtr, uint16_t length)
{
	uint16_t currentLength;
	currentLength = (length < MAX_DESC_TRANSFER) ? length : MAX_DESC_TRANSFER;
	//currentLength = (length < 8192) ? length : 8190;
	atomic{
	descPtr->DCMD &= ~DCMD_MAXLEN;
	descPtr->DCMD |= DCMD_LEN(currentLength);
	}
}

void DMA_setTransferWidth(DMADescriptor_t* descPtr, DMATransferWidth_t width)
{
	atomic{
	//clear it out since otherwise |'ing doesn't work so well
	descPtr->DCMD &= ~DCMD_MAXWIDTH;
	descPtr->DCMD |= DCMD_WIDTH(width);
	}
}

uint32_t DescArray_getBaseIndex(DescArray *DAPtr)
{
    uint32_t addr = (uint32_t) (&DAPtr->data[0]);
    return DescArray_BYTE_ALLIGNMENT - (addr % DescArray_BYTE_ALLIGNMENT);
}

DMADescriptor_t* DescArray_get(DescArray *DAPtr, uint8_t descIndex)
{
    uint32_t baseIndex = DescArray_getBaseIndex(DAPtr);
    return (DMADescriptor_t*)&DAPtr->data[baseIndex + descIndex*sizeof(DMADescriptor_t)];
}

