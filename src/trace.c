/*
 *  trace.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "gmutil.h"
#include "trace.h"
#include "mod.h"
#include "core.h"


static struct reg _reg_list[8];
static int _reg_count = 0;
static const struct reg_def *(* _reg_lookup)(const char *reg_name, struct reg_ctx *ctx_out);

static int _log_period = 1000;
static bool _log_enabled = false;


static void _rdef_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    trace_define_reg_list(arg_it);
}

static void _rlist_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    trace_print_reg_name_list();
    printf("\n");
}

static void _rval_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    trace_print_reg_val_list();
    printf("\n");
}

static void _rlog_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    mod_arg_iterator_next(arg_it);
    const char *arg = arg_it->name;
    if (arg) {
        _log_period = (int)strtol(arg, NULL, 0);
        _log_enabled = true;
    } else {
        _log_enabled = !_log_enabled;
    }
}

static void _loop(void)
{
    static int t = 0;
    
    bool log = false;

    int now = tick;
    int d = gmu_sub_s32(now, t);
    if (d >= 2 * _log_period) {
        // we are late - reset
        log = true;
        t = now;
    } else if (d >= _log_period) {
        log = true;
        t += _log_period;
    } if (d < 0) {
        // we are early - reset
        log = true;
        t = now;
    }

    if (log) {
        trace_print_reg_val_list();
    }
}

void trace_set_main_reg_lookup(const struct reg_def *(* reg_lookup)(const char *reg_name, struct reg_ctx *ctx_out))
{
    _reg_lookup = reg_lookup;
}

void trace_define_reg_list(struct mod_arg_iterator *arg_it)
{
    _reg_count = 0;
    for (;;) {
        mod_arg_iterator_next(arg_it);
        const char *arg = arg_it->name;
        if (arg == NULL)
            break;
        struct reg_ctx ctx;
        const struct reg_def *def = _reg_lookup(arg, &ctx);
        if (def && _reg_count < GMU_ARRAY_LEN(_reg_list)) {
            struct reg reg = {
                .def = def,
                .ctx = ctx,
            };
            _reg_list[_reg_count++] = reg;
        }
    }
}

void trace_print_reg_name_list(void)
{
    for (int i=0; i<_reg_count; i++) {
        if (i < 0)
            printf(", ");
        printf("%s", _reg_list[i].def->name);
    }
}

void trace_print_reg_val_list(void)
{
    for (int i=0; i<_reg_count; i++) {
        if (i < 0)
            printf(", ");
        reg_print(_reg_list[i].def, _reg_list[i].ctx);
    }
}


/*** module ***/

static const struct cmd_def _cmds[] = {
    {
        .name = "rdef",
        .help = "define registers to trace",
        .exec = _rdef_cmd,
    }, {
        .name = "rlist",
        .help = "list registers to trace",
        .exec = _rlist_cmd,
    }, {
        .name = "rval",
        .help = "show current values of defined registers",
        .exec = _rval_cmd,
    }, {
        .name = "log",
        .help = "log defined registers with given period in ms",
        .exec = _rlog_cmd,
    }
};

const struct mod trace_mod = {
    .name = "trace",
    .description = "manage list of regs to trace",
    .init_level = 3,
    .loop = _loop,
    .cmd_list = _cmds,
    .cmd_count = GMU_ARRAY_LEN(_cmds),
};
