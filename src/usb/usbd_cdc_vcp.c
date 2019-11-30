/**
 * This file has been stolen from STM sample code:
 *
 * @file    usbd_cdc_vcp.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    22-July-2011
 *
 * It has been customized for abc application.
 */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
#pragma     data_alignment = 4
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

#include "usbd_cdc_vcp.h"
#include "stm32f4xx.h"
#include "usbd_cdc_core.h"


/*** literals ***/

#define RX_FIFO_SIZE 128
#define RX_FIFO_MASK (RX_FIFO_SIZE - 1)

#if RX_FIFO_MASK & RX_FIFO_SIZE
#error RX_FIFO_SIZE must be a power of two
#endif


/*** prototypes ***/

static uint16_t VCP_Init(void);
static uint16_t VCP_DeInit(void);
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataTx(void);
static uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len);


/*** external variables ***/

extern uint8_t USB_Rx_Buffer[];

/* These are external variables imported from CDC core to be used for IN
   transfer management. */
extern uint8_t  APP_Rx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Rx_Buffer. */
extern uint32_t APP_Rx_ptr_out;


/*** variables ***/

static LINE_CODING linecoding = {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
};

static void *_cdc_AsyncDataOut_dev;
static _Atomic int USB_Rx_Buffer_available;
static int USB_Rx_Buffer_consumed;

static uint8_t rx_fifo[RX_FIFO_SIZE];
static uint32_t rx_fifo_in;
static uint32_t rx_fifo_out;

CDC_IF_Prop_TypeDef VCP_fops = {
  VCP_Init,
  VCP_DeInit,
  VCP_Ctrl,
  VCP_DataTx
  // VCP_DataRx is now called directly by _cdc_AsyncDataOut()
};


/*** functions ***/

/**
  * This function is a variant of usbd_cdc_DataOut(). We reimplemented
  * it because usbd_cdc_DataOut() does not give any way to make a back pressure
  * on incoming data. If the host sent too much data, we resulted in a input
  * buffer overflow.
  * This implementation fix this problem. It calls VCP_DataRx() and wait a
  * callback to _cdc_AsyncDataOutAck().
  */
static uint8_t _cdc_AsyncDataOut(void *pdev, uint8_t epnum)
{
  uint16_t USB_Rx_Cnt;

  _cdc_AsyncDataOut_dev = pdev;

  /* Get the received data buffer and update the counter */
  USB_Rx_Cnt = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;

  VCP_DataRx(USB_Rx_Buffer, USB_Rx_Cnt);

  /* Next USB traffic will be NAKed till _cdc_AsyncDataOutAck() will be called */

  return USBD_OK;
}

static void _cdc_AsyncDataOutAck(void)
{
    /* Prepare Out endpoint to receive next packet */
    DCD_EP_PrepareRx(_cdc_AsyncDataOut_dev,
                     CDC_OUT_EP,
                     (uint8_t*)(USB_Rx_Buffer),
                     CDC_DATA_OUT_PACKET_SIZE);
}

void VCP_global_init(void)
{
    /*
     * In order to handle data coming from the PC in a way that prevents buffer
     * overflow, we overwrite the default implementation of usbd_cdc_DataOut().
     */
    USBD_CDC_cb.DataOut = _cdc_AsyncDataOut;
}

/**
  * @brief  VCP_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_Init(void)
{
    return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_DeInit(void)
{
    return USBD_OK;
}

/**
 * @brief  VCP_Ctrl
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{
    switch (Cmd) {
    case SEND_ENCAPSULATED_COMMAND:
        /* Not  needed for this driver */
        break;

    case GET_ENCAPSULATED_RESPONSE:
        /* Not  needed for this driver */
        break;

    case SET_COMM_FEATURE:
        /* Not  needed for this driver */
        break;

    case GET_COMM_FEATURE:
        /* Not  needed for this driver */
        break;

    case CLEAR_COMM_FEATURE:
        /* Not  needed for this driver */
        break;

    case SET_LINE_CODING:
        /* Not  needed for this driver */
        break;

    case GET_LINE_CODING:
        Buf[0] = (uint8_t) (linecoding.bitrate);
        Buf[1] = (uint8_t) (linecoding.bitrate >> 8);
        Buf[2] = (uint8_t) (linecoding.bitrate >> 16);
        Buf[3] = (uint8_t) (linecoding.bitrate >> 24);
        Buf[4] = linecoding.format;
        Buf[5] = linecoding.paritytype;
        Buf[6] = linecoding.datatype;
        break;

    case SET_CONTROL_LINE_STATE:
        /* Not  needed for this driver */
        break;

    case SEND_BREAK:
        /* Not  needed for this driver */
        break;

    default:
        break;
    }

    return USBD_OK;
}

void VCP_put_char(uint8_t buf)
{
    VCP_send_buffer(&buf, 1);
}

void VCP_send_str(uint8_t* buf)
{
    uint32_t i = 0;
    while (*(buf + i)) {
        i++;
    }
    VCP_send_buffer(buf, i);
}

void VCP_send_buffer(uint8_t* buf, int len)
{
    uint32_t i = 0;
    int in = APP_Rx_ptr_in;

    while (i < len) {
        APP_Rx_Buffer[in] = *(buf + i);
        in++;
        i++;
        if (in == APP_RX_DATA_SIZE)
            in = 0;
    }

    *(volatile int *)&APP_Rx_ptr_in = in;
}

int VCP_nonblocking_send_size(void)
{
    int in = APP_Rx_ptr_in;
    int out = APP_Rx_ptr_out;
    int busy = in - out;
    if (busy < 0)
        busy += APP_RX_DATA_SIZE;
    return APP_RX_DATA_SIZE - busy - 1;
}

/**
 * @brief  VCP_DataTx
 *         CDC received data to be send over USB IN endpoint are managed in
 *         this function.
 * @param  Buf: Buffer of data to be sent
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
static uint16_t VCP_DataTx(void)
{
    return USBD_OK;
}

/**
 * @brief  VCP_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will block any OUT packet reception on USB endpoint
 *         until exiting this function. If you exit this function before transfer
 *         is complete on CDC interface (ie. using DMA controller) it will result
 *         in receiving more data while previous ones are still not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */

static uint16_t VCP_DataRx(uint8_t* buf, uint32_t len)
{
    USB_Rx_Buffer_available = len;
    return USBD_OK;
}

/**
 * Copy data from the USB Rx Buffer into the rx fifo.
 */
void VCP_rx_pump(void)
{
    for (;;) {
        if (USB_Rx_Buffer_consumed >= USB_Rx_Buffer_available) {
            if (USB_Rx_Buffer_consumed) {
                /*
                 * We just used all data in the USB_Rx_Buffer. So, we can
                 * acknowledge the reception which allows the host to send us
                 * more data.
                 */
                USB_Rx_Buffer_consumed = 0;
                USB_Rx_Buffer_available = 0;
                _cdc_AsyncDataOutAck();
            }
            return;
        }
        if (rx_fifo_in == rx_fifo_out + RX_FIFO_SIZE)
            return;
        rx_fifo[rx_fifo_in & RX_FIFO_MASK] = USB_Rx_Buffer[USB_Rx_Buffer_consumed];
        rx_fifo_in++;
        USB_Rx_Buffer_consumed++;
    }
}

/**
 * Get the next character received from the PC. Return 1 if a character is
 * present, 0 if not.
 */
int VCP_get_char(uint8_t *buf)
{
    VCP_rx_pump();

    if (rx_fifo_in == rx_fifo_out)
        return 0;

    *buf = rx_fifo[rx_fifo_out & RX_FIFO_MASK];
    rx_fifo_out++;
    return 1;
}

#if 0
int VCP_get_string(uint8_t *buf) {
    if (APP_tx_ptr_head == APP_tx_ptr_tail)
        return 0;

    while (!APP_Tx_Buffer[APP_tx_ptr_tail]
            || APP_Tx_Buffer[APP_tx_ptr_tail] == '\n'
            || APP_Tx_Buffer[APP_tx_ptr_tail] == '\r') {
        APP_tx_ptr_tail++;
        if (APP_tx_ptr_tail == APP_TX_BUF_SIZE)
            APP_tx_ptr_tail = 0;
        if (APP_tx_ptr_head == APP_tx_ptr_tail)
            return 0;
    }

    int i = 0;
    do {
        *(buf + i) = APP_Tx_Buffer[i + APP_tx_ptr_tail];
        i++;

        if ((APP_tx_ptr_tail + i) == APP_TX_BUF_SIZE)
            i = -APP_tx_ptr_tail;
        if (APP_tx_ptr_head == (APP_tx_ptr_tail + i))
            return 0;

    } while (APP_Tx_Buffer[APP_tx_ptr_tail + i]
            && APP_Tx_Buffer[APP_tx_ptr_tail + i] != '\n'
            && APP_Tx_Buffer[APP_tx_ptr_tail + i] != '\r');

    *(buf + i) = 0;
    APP_tx_ptr_tail += i;
    if (APP_tx_ptr_tail >= APP_TX_BUF_SIZE)
        APP_tx_ptr_tail -= APP_TX_BUF_SIZE;
    return i;
}
#endif
