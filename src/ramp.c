/*
 *  ramp.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include "ramp.h"
#include <math.h>


void ramp_init(struct ramp *me)
{
    ramp_set_spd(me, RAMP_SPD);
    ramp_set_acc(me, RAMP_ACC);
}

void ramp_set_spd(struct ramp *me, float spd)
{
    me->traj.sv = (int)round(spd * (float)RAMP_POS_SCALE * RAMP_CYCLE_TIME);
    traj_update(&me->traj);
}

void ramp_set_acc(struct ramp *me, float acc)
{
    me->traj.sa = (int)round(acc * (float)RAMP_POS_SCALE * RAMP_CYCLE_TIME * RAMP_CYCLE_TIME);
    traj_update(&me->traj);
}

void ramp_start(struct ramp *me)
{
    me->traj.sdir = 1;
}

// return the electric angle
float ramp_cycle(struct ramp *me)
{
    traj_step(&me->traj);
    float u = (float)(me->traj.jl_x & (RAMP_POS_SCALE - 1)) / (float)RAMP_POS_SCALE;
    return u * 2.0f * (float)M_PI;
}
