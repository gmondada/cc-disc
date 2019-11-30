/*
 *  led.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <stdbool.h>
#include "stm32f4xx.h"
#include "gmutil.h"
#include "led.h"
#include "core.h"


static bool _states[1];


void led_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; // PB15
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    for (int i=0; i<GMU_ARRAY_LEN(_states); i++)
        led_set(i, false);
}

void led_loop(void)
{
    static int t;
    const int led_freq = 500;

    if (gmu_sub_s32(tick, t) > led_freq) {
        t = tick;
        led_toggle(0);
    }
}

void led_set(int led, bool state)
{
    switch (led) {
        case 0:
            if (state)
                GPIO_SetBits(GPIOB, GPIO_Pin_15);
            else
                GPIO_ResetBits(GPIOB, GPIO_Pin_15);
            _states[0] = state;
            break;
    }
}

void led_toggle(int led)
{
    if (led < 0 || led >= GMU_ARRAY_LEN(_states))
        return;
    led_set(led, !_states[led]);
}

void led_code(int code)
{
    // TODO
}

const struct mod led_mod = {
    .name = "led",
    .init_level = 2,
    .init = led_init,
    .loop = led_loop,
};
