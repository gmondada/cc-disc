/*
 *  main.h
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include "stm32f4xx.h"
#include "gmutil.h"
#include "app.h"
#include "acm.h"
#include "cli.h"
#include "led.h"
#include "stepper.h"
#include "trace.h"
#include "uart.h"


/*** globals ***/

const struct mod *mods[] = {
    &acm_mod,
    &core_mod,
    &led_mod,
    &stepper_mod,
    &trace_mod,
    &uart_mod,
};

const struct app app = {
    .name = "cc-disc",
    .version = "1.0.0",
    .module_list = mods,
    .module_count = GMU_ARRAY_LEN(mods),
};


/*** functions ***/

int main(int argc, char *argv[])
{
    core_set_stdio(&acm_io);
    cli_set_io(&acm_io);
    app_init(&app);
    trace_set_main_reg_lookup(app_reg_lookup);
    app_start();
    app_run();
}
