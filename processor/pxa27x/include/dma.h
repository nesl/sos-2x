#ifndef _DMA_H
#define _DMA_H

typedef enum
  {
    DMA_8ByteBurst = 1,
    DMA_16ByteBurst,
    DMA_32ByteBurst
  } DMAMaxBurstSize_t;

typedef enum
  {
    DMA_NonPeripheralWidth = 0 ,
    DMA_1ByteWidth,
    DMA_2ByteWidth,
    DMA_4ByteWidth
  } DMATransferWidth_t;

typedef enum
  {
    DMA_Priority1 = 1 ,
    DMA_Priority2 = 2,
    DMA_Priority3 = 4,
    DMA_Priority4 = 8
  } DMAPriority_t;

typedef enum
  {
    DMAID_DREQ0 = 0,
    DMAID_DREQ1,
    DMAID_I2S_RX,
    DMAID_I2S_TX,
    DMAID_BTUART_RX,
    DMAID_BTUART_TX,
    DMAID_FFUART_RX,
    DMAID_FFUART_TX,
    DMAID_AC97_MIC,
    DMAID_AC97_MODEMRX,
    DMAID_AC97_MODEMTX,
    DMAID_AC97_AUDIORX,
    DMAID_AC97_AUDIOTX,
    DMAID_SSP1_RX,
    DMAID_SSP1_TX,
    DMAID_SSP2_RX,
    DMAID_SSP2_TX,
    DMAID_ICP_RX,
    DMAID_ICP_TX,
    DMAID_STUART_RX,
    DMAID_STUART_TX,
    DMAID_MMC_RX,
    DMAID_MMC_TX,
    DMAID_USB_END0 = 24,
    DMAID_USB_ENDA,
    DMAID_USB_ENDB,
    DMAID_USB_ENDC,
    DMAID_USB_ENDD,
    DMAID_USB_ENDE,
    DMAID_USB_ENDF,
    DMAID_USB_ENDG,
    DMAID_USB_ENDH,
    DMAID_USB_ENDI,
    DMAID_USB_ENDJ,
    DMAID_USB_ENDK,
    DMAID_USB_ENDL,
    DMAID_USB_ENDM,
    DMAID_USB_ENDN,
    DMAID_USB_ENDP,
    DMAID_USB_ENDQ,
    DMAID_USB_ENDR,
    DMAID_USB_ENDS,
    DMAID_USB_ENDT,
    DMAID_USB_ENDU,
    DMAID_USB_ENDV,
    DMAID_USB_ENDW,
    DMAID_USB_ENDX,
    DMAID_MSL_RX1,
    DMAID_MSL_TX1,
    DMAID_MSL_RX2,
    DMAID_MSL_TX2,
    DMAID_MSL_RX3,
    DMAID_MSL_TX3,
    DMAID_MSL_RX4,
    DMAID_MSL_TX4,
    DMAID_MSL_RX5,
    DMAID_MSL_TX5,
    DMAID_MSL_RX6,
    DMAID_MSL_TX6,
    DMAID_MSL_RX7,
    DMAID_MSL_TX7,
    DMAID_USIM_RX,
    DMAID_USIM_TX,
    DMAID_MEMSTICK_RX,
    DMAID_MEMSTICK_TX,
    DMAID_SSP3_RX,
    DMAID_SSP3_TX,
    DMAID_CIF_RX0,
    DMAID_CIF_RX1,
    DMAID_DREQ2
  } DMAPeripheralID_t;


typedef struct {
    uint32_t DDADR;
    uint32_t DSADR;
    uint32_t DTADR;
    uint32_t DCMD;
} DMADescriptor_t;


#define MAX_DESC_TRANSFER  8184		// max is 8K-1, CIF requires a multiple of 8	//8192

// ----------------------------------------------
#define DescArray_NBR_DESC         20//8
#define DescArray_BYTE_ALLIGNMENT  16
#define DescArray_BUFFER_SIZE      (DescArray_NBR_DESC*sizeof(DMADescriptor_t) + DescArray_BYTE_ALLIGNMENT)

typedef struct DescArray
{
    uint8_t data[DescArray_BUFFER_SIZE];   // The data is alligned from data[baseIndex]
} DescArray;

void DMA_setSourceAddr(DMADescriptor_t* descPtr, uint32_t val);
void DMA_setTargetAddr(DMADescriptor_t* descPtr, uint32_t val);
void DMA_enableSourceAddrIncrement(DMADescriptor_t* descPtr, bool enable);
void DMA_enableTargetAddrIncrement(DMADescriptor_t* descPtr, bool enable);
void DMA_enableSourceFlowControl(DMADescriptor_t* descPtr, bool enable);
void DMA_enableTargetFlowControl(DMADescriptor_t* descPtr, bool enable);
void DMA_setMaxBurstSize(DMADescriptor_t* descPtr, DMAMaxBurstSize_t size);
void DMA_setTransferLength(DMADescriptor_t* descPtr, uint16_t length);
void DMA_setTransferWidth(DMADescriptor_t* descPtr, DMATransferWidth_t width);

void PXA27XHPLDMA_setByteAlignment(uint32_t channel, bool enable);
void PXA27XHPLDMA_mapChannel(uint32_t channel,uint16_t peripheralID);
void PXA27XHPLDMA_unmapChannel(uint32_t channel);
void PXA27XHPLDMA_setDCSR(uint32_t channel,uint32_t val);
uint32_t PXA27XHPLDMA_getDCSR(uint32_t channel);
void PXA27XHPLDMA_setDCMD(uint32_t channel, uint32_t val);
uint32_t PXA27XHPLDMA_getDCMD(uint32_t channel);
void PXA27XHPLDMA_setDDADR(uint32_t channel ,uint32_t val);
uint32_t PXA27XHPLDMA_getDDADR(uint32_t channel);
void PXA27XHPLDMA_setDSADR(uint32_t channel, uint32_t val);
uint32_t PXA27XHPLDMA_getDSADR(uint32_t channel);
void PXA27XHPLDMA_setDTADR(uint32_t channel, uint32_t val);
uint32_t PXA27XHPLDMA_getDTADR(uint32_t channel);
uint32_t PXA27XHPLDMA_getDPCSR();
void PXA27XHPLDMA_setDPCSR(uint32_t val);
void DMA_run();

uint32_t DescArray_getBaseIndex(DescArray *DAPtr);
DMADescriptor_t* DescArray_get(DescArray *DAPtr, uint8_t descIndex);


#endif //_DMA_H
