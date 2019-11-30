/*
 *  acm.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _ACM_H_
#define _ACM_H_

#include <stdbool.h>
#include <string.h>
#include "gmutil.h"
#include "mod.h"


/*** globals ***/

extern struct cli_io acm_io;
extern const struct mod acm_mod;


/*** functions ***/

void acm_init(void);
bool acm_cmd(struct mod_arg_iterator *arg_it);
void acm_add_esc_handler(void (* handler)(void));
void acm_remove_esc_handler(void (* handler)(void));
int  acm_getc(void);
void acm_write(const void *buf, int len);
void acm_putc(char c);


#endif
