/*
 *  stepper.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <stdbool.h>
#include "mod.h"


extern const struct mod stepper_mod;


void stepper_pwm(int port, float value);


#endif
