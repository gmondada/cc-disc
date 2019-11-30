/*
 *  ramp.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _RAMP_H_
#define _RAMP_H_

#include "traj.h"


#define RAMP_CYCLE_TIME  0.0001f   // seconds per cycle
#define RAMP_ACC         50.0f     // max acceleration in electric tours per second
#define RAMP_SPD         50.0f     // max speed in electric tours per second
#define RAMP_POS_SHIFT   23
#define RAMP_POS_SCALE   (1 << RAMP_POS_SHIFT) // increments per electric tours


struct ramp {
    struct traj traj;
};


void ramp_init(struct ramp *me);
void ramp_set_spd(struct ramp *me, float spd);
void ramp_set_acc(struct ramp *me, float acc);
void ramp_start(struct ramp *me);
float ramp_cycle(struct ramp *me);


#endif
