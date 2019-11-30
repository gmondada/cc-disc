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

#include "traj.h"

/*
 * Naming conventions:
 * s*   setpoint (target), input
 * n*   value for the next cycle
 * x    position
 * v    velocity
 * a    acceleration
 * *x_r remaining stroke, should always be positive
 * *a_b acceleration needed to brake in the remaining stroke
 *
 * The trajectory generator works in a cycle. On each cycle, traj_step()
 * must be called. This method computes the next position in the trajectory.
 * If the movement is stopped (moving == false), you can set any trajectory
 * parameter, including sx and sdir. If sx or sdir is set, the movement
 * starts automatically.
 * If a movement is already in progress, you can modify any movement
 * parameters including sdir and sx but you have then to call traj_update()
 * to inform the trajectory generator about this changement.
 * Changing any parameter while the movement is in progress without calling
 * traj_update() results in a undefined behavior.
 *
 * Standard movement:
 * By setting sx, you start a standard movement where sx is the target
 * position. The speed will increase up to sv and decrese at the rigth moment
 * in order to reach the target position when speed is zero.
 *
 * Infinite mode:
 * By setting sdir=1 or sdir=-1 you start an infinite movement. The movement
 * starts in the direction given by sdir, accelerates up to sv, then continues
 * at constant speed forever. The value of sx is ignored.
 *
 * When a movement is in progress, you can call traj_brake() to stop the
 * movement. The speed will decrease with the programmed deceleration until
 * it reaches zero.
 * After a brake, sx is automatically set to the the position where the
 * movement stops. This is true also for infinite movements.
 *
 * The generated trajectory has a trapezoidal speed. This satisfies continuity
 * of speed.
 * In order to have continuity of acceleration, the trajectory generator
 * applies a filter on the computed position. In some ways, this filter limits
 * the jerk. This filter is defined by its time jt, also called jerk time.
 * The jerk time also defines the delay the filter introduces. If jt=16, for
 * instance, the movement is 16 cycle longer than without the filter.
 * The moving flag tells when the movement is finished but, due to the
 * filter, the movement ends jt cycle later. The jl_moving flag takes care of
 * this additional delay and tells when the filtered movement is finished.
 * The jerk time is defined by TRAJ_JL_SIZE. It must be a power of two and
 * it's defined at compilation time.
 *
 * Trajectory parameters:
 *  sx = target position (used for finite movements)
 *  sdir = target direction (used for infinite movements)
 *  sv = max speed (must be positive)
 *  sa = acceleration and deceleration (must be positive)
 *
 * Units:
 * position is in increments, speeds is in increments per cycle, acceleration
 * is in increment per square cycle, jerk time is in cycles.
 */

static int _sign(int a)
{
    if (a < 0)
        return -1;
    if (a > 0)
        return 1;
    return 0;
}

static int _pos_sign(traj_pos_t a)
{
    if (a < 0)
        return -1;
    if (a > 0)
        return 1;
    return 0;
}

/**
 * This function computes the next position in the trajectory. It must
 * be called once per cycle.
 */
void traj_step(struct traj *traj)
{
    int         sa = traj->sa;
    int         sv = traj->sv;
    traj_pos_t  sx = traj->sx;
    int         v = traj->v;
    traj_pos_t  x = traj->x;
    int         dir = traj->dir;
    int         na;
    int         a_b;
    int         nv = v;
    traj_pos_t  nx = x;
    int         na_b;
    traj_pos_t  x_r;
    traj_pos_t  nx_r;
    traj_pos_t  brake_dist;
    traj_pos_t  vv = (traj_pos_t)v * v;

step:
    switch (traj->state) {
        case TRAJ_STATE_WAIT:
            if (sx == x && !traj->sdir)
                break;
            traj->moving = true;
            traj->jl_moving = TRAJ_JL_SIZE;
            traj->state = TRAJ_STATE_START;
            goto step;

        case TRAJ_STATE_START:

            // define in which direction we reach the target
            if (traj->sdir) {
                dir = traj->sdir;
            } else {
                dir = _pos_sign(sx - x);
                if (!dir) {
                    /*
                     * We are at target position.
                     * We have to decelerate, invert the speed and reach the
                     * target in the opposite direction.
                     */
                    dir = -_sign(v);
                    if (!dir) {
                        // speed is zero, so we are standstill
                        traj->state = TRAJ_STATE_STANDSTILL;
                        goto step;
                    }
                } else {
                    x_r = (sx - x) * dir; // x_z is never zero here
                    a_b = (int)(vv / (x_r * 2) + 1);
                    if (_sign(v) == dir && a_b >= sa) {
                        /*
                         * Even by breaking now, we go farther than the target.
                         * We have to decelerate, invert the speed and reach
                         * the target in the opposite direction.
                         */
                        dir *= -1;
                    }
                }
            }

            // define next step
            if (v * dir < traj->sv)
                traj->state = TRAJ_STATE_ACC;
            else
                traj->state = TRAJ_STATE_DEC;
            goto step;

        case TRAJ_STATE_ACC:
            // speed is below the target speed or it is in the opposite direction
            nv = v + sa * dir;
            nx = x + (v + nv) / 2;
            nx_r = (sx - nx) * dir;
            if (_sign(nv) == dir && !traj->sdir) {
                if (nx_r <= 0) {
                    traj->state = TRAJ_STATE_STANDSTILL;
                    goto step;
                }
                na_b = (int)(vv / (nx_r * 2) + 1);
                if (na_b > sa) {
                    traj->state = TRAJ_STATE_DEC_TO_ZERO;
                    goto step;
                }
            }
            if ((nv * dir) > sv) {
                traj->state = TRAJ_STATE_CONST_SPEED;
                goto step;
            }
            break;

        case TRAJ_STATE_DEC:
            // speed is in the right direction but above the target speed
            nv = v - sa * dir;
            nx = x + (v + nv) / 2;
            if ((nv * dir) <= sv) {
                traj->state = TRAJ_STATE_CONST_SPEED;
                goto step;
            }
            break;

        case TRAJ_STATE_CONST_SPEED:
            nv = sv * dir;
            nx = x + (v + nv) / 2;

            if (traj->sdir)
                break;

            nx_r = (sx - nx) * dir;
            if (nx_r <= 0) {
                traj->state = TRAJ_STATE_STANDSTILL;
                goto step;
            }

            na_b = (int)(vv / (nx_r * 2) + 1);
            if (na_b > sa) {
                traj->state = TRAJ_STATE_DEC_TO_ZERO;
                goto step;
            }
            break;

        case TRAJ_STATE_DEC_TO_ZERO:
            x_r = (sx - x) * dir;
            if (x_r <= 0) {
                traj->state = TRAJ_STATE_STANDSTILL;
                goto step;
            }
            //na = v * v / x_r / 2;
            na = (int)((vv + x_r) / (x_r * 2));
            if (na <= 0)
                na = 1;
            nv = v - na * dir;
            nx = x + (v + nv) / 2;
            if (_sign(nv) != dir) {
                traj->state = TRAJ_STATE_STANDSTILL;
                goto step;
            }
            break;

        case TRAJ_STATE_STANDSTILL:
            nv = 0;
            nx = sx;
            dir = 0;
            traj->moving = false;
            traj->state = TRAJ_STATE_WAIT;
            break;

        case TRAJ_STATE_BRAKE:
            dir = _sign(v);
            brake_dist = vv / (2 * sa);
            sx = x + brake_dist * dir;
            traj->sx = sx;
            traj->sdir = 0;
            traj->state = TRAJ_STATE_DEC_TO_ZERO;
            goto step;

        default:
            nx = x;
            nv = v;
    }

    traj->x = nx;
    traj->v = nv;
    traj->dir = dir;

    // filter to limit the jerk
    traj_pos_t out = traj->jl_array[traj->jl_index];
    traj->jl_array[traj->jl_index] = nx;
    traj->jl_acc += nx - out;
    traj->jl_index = (traj->jl_index + 1) & TRAJ_JL_SIZE_MASK;
    traj->jl_x = traj->jl_acc >> TRAJ_JL_SIZE_SHIFT;

    if (!traj->moving && traj->jl_moving > 0)
        traj->jl_moving--;
}

/**
 * This method must be called each time you modify the trajectory
 * parameters on the fly, while the movement is in progress.
 */
void traj_update(struct traj *traj)
{
    if (traj->moving)
        traj->state = TRAJ_STATE_START;
}

/**
 * This method forces the trajectory generator to brake, i.e. to
 * decrease its speed until it reaches speed zero.
 * After this call, you do not have to call traj_update().
 */
void traj_brake(struct traj *traj)
{
    switch (traj->state) {
        case TRAJ_STATE_ACC:
        case TRAJ_STATE_DEC:
        case TRAJ_STATE_CONST_SPEED:
        case TRAJ_STATE_DEC_TO_ZERO:
            traj->state = TRAJ_STATE_BRAKE;
            break;
    }
}

/**
 * This method abruptly stops any movement being in-progress and
 * set the current position to the given one.
 * After this call, the trajectory is standstill (speed is zero),
 * and the movement is stopped (moving == false, jl_moving == false).
 * After this call, you do not have to call traj_update().
 */
void traj_jump(struct traj *traj, traj_pos_t x)
{
    traj->sx = x;
    traj->sdir = 0;
    traj->x = x;
    traj->v = 0;
    traj->state = TRAJ_STATE_WAIT;
    traj->moving = false;

    for (int i=0; i<TRAJ_JL_SIZE; i++)
        traj->jl_array[i] = x;
    traj->jl_acc = x * TRAJ_JL_SIZE;
    traj->jl_x = x;
    traj->jl_moving = false;
}
