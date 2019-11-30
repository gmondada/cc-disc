/*
 *  trace.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#ifndef _TRACE_H_
#define _TRACE_H_


#include "trace.h"
#include "mod.h"


extern const struct mod trace_mod;


void trace_set_main_reg_lookup(const struct reg_def *(* reg_lookup)(const char *reg_name, struct reg_ctx *ctx_out));
void trace_define_reg_list(struct mod_arg_iterator *arg_it);
void trace_print_reg_name_list(void);
void trace_print_reg_val_list(void);


#endif
