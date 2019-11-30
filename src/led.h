/*
 *  led.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _LED_H_
#define _LED_H_

#include "mod.h"


extern const struct mod led_mod;


void led_init(void);
void led_loop(void);
void led_set(int led, bool state);
void led_toggle(int led);
void led_code(int code);


#endif
