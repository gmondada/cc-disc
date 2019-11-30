/*
 *  uart.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <stdio.h>
#include "stm32f4xx.h"
#include "mod.h"
#include "gmutil.h"


static void _init()
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_USART3);

    GPIO_InitTypeDef pgio_def = { 0 };
    pgio_def.GPIO_Mode = GPIO_Mode_AF;
    pgio_def.GPIO_OType = GPIO_OType_PP;
    pgio_def.GPIO_Speed = GPIO_Speed_100MHz;
    pgio_def.GPIO_PuPd = GPIO_PuPd_NOPULL;

    pgio_def.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOC, &pgio_def);

    USART_InitTypeDef uart_def = { 0 };
    USART_StructInit(&uart_def);
    uart_def.USART_BaudRate = 115200;

    USART_Init(USART3, &uart_def);
    // USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
    // USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);
}

static void _stat(const struct cmd_def *def, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    printf("SR=0x%08x\n", (int)USART3->SR);
}

static void _send_char(const struct cmd_def *def, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    const char *arg = mod_arg_iterator_next(arg_it);
    if (!arg) {
        printf("missing arg\n");
        return;
    }
    char c = arg[0];
    USART_SendData(USART3, c);
}

static void _recv_char(const struct cmd_def *def, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    char c = USART_ReceiveData(USART3);
    printf("recevied: 0x%02x\n", (int)c & 0xff);
}


/*** module definition ***/

static const struct cmd_def _cmds[] = {
    {
        .name = "ustat",
        .help = "ustat          show uart state",
        .exec = _stat,
    }, {
        .name = "usend",
        .help = "usend <char>   send a character",
        .exec = _send_char,
    }, {
        .name = "urecv",
        .help = "urecv <char>   receive a character",
        .exec = _recv_char,
    }
};

const struct mod uart_mod = {
    .name = "uart",
    .description = "manage the uart",
    .init = _init,
    .cmd_list = _cmds,
    .cmd_count = GMU_ARRAY_LEN(_cmds),
};
