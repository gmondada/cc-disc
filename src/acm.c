/*
 *  acm.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
//#include <sys/stat.h>
#include "stm32f4xx.h"
#include "acm.h"
#include "gmutil.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"
#include "cli.h"


/*** globals ***/

__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

struct cli_io acm_io = {
    .read_char = acm_getc,
    .write_char = acm_putc,
    .write_buf = acm_write,
};


/*** functions ***/

void acm_init(void)
{
    VCP_global_init();

    USBD_Init(&USB_OTG_dev,
                USB_OTG_FS_CORE_ID,
                &USR_desc,
                &USBD_CDC_cb,
                &USR_cb);
}

void acm_write(const void *buf, int len)
{
    uint8_t *ibuf = (uint8_t *)buf;
    uint8_t obuf[2 * len];
    int o = 0;

    for (int i=0; i<len; i++) {
        uint8_t c = ibuf[i];
        if (c == 10)
            obuf[o++] = 13;
        obuf[o++] = c;
    }

    int done = 0;
    for (;;) {
        int todo = o - done;
        if (todo <= 0)
            break;
        int max = VCP_nonblocking_send_size();
        if (todo > max)
            todo = max;
        if (todo > 0) {
            VCP_send_buffer(obuf + done, todo);
            done += todo;
        }
    }
}

void acm_putc(char c)
{
    acm_write(&c, 1);
}

int acm_getc(void)
{
    static int skip_c = -1;
    uint8_t c;

    for (;;) {
        if (!VCP_get_char(&c))
            return -1;

        if (c == 27) {
            cli_fire_esc_handlers();
            skip_c = -1;
            continue;
        }

        if ((int)c == skip_c) {
            skip_c = -1;
            continue;
        }

        if (c == '\r') {
            skip_c = '\n';
            return '\n';
        }

        if (c == '\n') {
            skip_c = '\r';
            return '\n';
        }

        skip_c = -1;
        return c;
    }
}


/*** module ***/

const struct mod acm_mod = {
    .name = "acm",
    .description = "serial line over usb",
    .init_level = 3,
    .init = acm_init,
};
