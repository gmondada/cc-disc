/*
 * S-Curve Trajectory Generator.
 *
 * Copyright (c) 2011-2019 Gabriele Mondada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TRAJ_H_
#define _TRAJ_H_

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


/*** literals ***/

#define TRAJ_STATE_WAIT         0
#define TRAJ_STATE_START        1
#define TRAJ_STATE_ACC          2
#define TRAJ_STATE_DEC          3
#define TRAJ_STATE_CONST_SPEED  4
#define TRAJ_STATE_DEC_TO_ZERO  5
#define TRAJ_STATE_STANDSTILL   6
#define TRAJ_STATE_BRAKE        7

#define TRAJ_JL_SIZE_SHIFT      4
#define TRAJ_JL_SIZE            (1 << TRAJ_JL_SIZE_SHIFT)
#define TRAJ_JL_SIZE_MASK       ((1 << TRAJ_JL_SIZE_SHIFT) - 1)

#define TRAJ_64BIT


/*** types ***/

#ifdef TRAJ_64BIT
typedef int64_t traj_pos_t;
#else
typedef int traj_pos_t;
#endif

struct traj {
    // inputs (public)
    int        sa;
    int        sv;
    traj_pos_t sx;
    int        sdir; // infinite mode direction

    // used internally by traj_step() (private)
    int dir; // direction in which we plan to reach the target (this is not always the start dir)
    int state;

    // output values (public)
    traj_pos_t x;   // signed
    int        v;   // signed

    // output status (public)
    bool moving;

    // jerk limiter (private)
    traj_pos_t jl_array[TRAJ_JL_SIZE];
    traj_pos_t jl_acc;
    int        jl_index;

    // output values after jerk limiter (public)
    traj_pos_t jl_x;

    // output status taking care of the jerk limiter (public)
    int jl_moving;   // zero means the movement is finished
};


/*** prototypes ***/

void traj_step(struct traj *traj);
void traj_update(struct traj *traj);
void traj_brake(struct traj *traj);
void traj_jump(struct traj *traj, traj_pos_t x);


#endif
