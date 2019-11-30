
#ifndef __USBD_CDC_VCP_H
#define __USBD_CDC_VCP_H

#include "stm32f4xx_conf.h"

#include "usbd_cdc_core.h"
#include "usbd_conf.h"
#include <stdint.h>

/* Exported typef ------------------------------------------------------------*/
/* The following structures groups all needed parameters to be configured for the
   ComPort. These parameters can modified on the fly by the host through CDC class
   command class requests. */
typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
} LINE_CODING;

/* Exported constants --------------------------------------------------------*/
/* The following define is used to route the USART IRQ handler to be used.
   The IRQ handler function is implemented in the usbd_cdc_vcp.c file. */
#ifdef USE_STM322xG_EVAL
 #define EVAL_COM_IRQHandler            USART3_IRQHandler
#elif defined(USE_STM3210C_EVAL)
 #define EVAL_COM_IRQHandler            USART2_IRQHandler
#endif /* USE_STM322xG_EVAL */

void VCP_global_init(void);
void VCP_put_char(uint8_t buf);
void VCP_send_str(uint8_t* buf);
int VCP_get_char(uint8_t *buf);
int VCP_get_string(uint8_t *buf);
void VCP_send_buffer(uint8_t* buf, int len);
int VCP_nonblocking_send_size(void);

#define DEFAULT_CONFIG                  0
#define OTHER_CONFIG                    1

#endif
