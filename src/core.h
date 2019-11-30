/*
 *  core.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _CORE_H_
#define _CORE_H_

#include <stdint.h>
#include <stdatomic.h>
#include "mod.h"
#include "cli.h"


/*** globals ***/

extern const struct mod core_mod;
extern atomic_int tick;


/*** prototypes ***/

void core_init(void);
void core_set_stdio(const struct cli_io *io);
void core_print_cpu_info(void);
void core_print_mem_info(void);
void core_fill_heap_and_stack(void);
void core_dump_heap_and_stack(void);
void core_system_reset(void);
int core_interrupt_level(void);


/*** inline functions ***/

static inline int32_t core_get_tick(void)
{
    return tick;
}


#endif
