/*
 *  core.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <sys/stat.h>
#include <stddef.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "core.h"
#include "gmutil.h"


/*** globals ***/

atomic_int tick;

static struct cli_io _io;

register char *stack_ptr asm ("sp");
static char *heap_end;


/*** functions ***/

static void _cpu_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    core_print_cpu_info();
}

static void _mem_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    core_print_mem_info();
}

static void _memfill_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    core_fill_heap_and_stack();
}

static void _memdump_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    core_dump_heap_and_stack();
}

static void _reset_cmd(const struct cmd_def *cmd, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    core_system_reset();
}

void core_init(void)
{
    /*
     * 4-bit inturrupt priority is configured to hold:
     *   2 bits for pre-emption priority
     *   2 bits for subpriority
     */
    NVIC_SetPriorityGrouping(5);
    // equivalent to NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    SysTick_Config(SystemCoreClock / 1000);
}

void core_set_stdio(const struct cli_io *io)
{
    _io = *io;
}

void core_print_cpu_info(void)
{
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    printf("sysclk=%d hclk=%d pclk1=%d pclk2=%d coreclk=%d\n",
        (int)RCC_ClocksStatus.SYSCLK_Frequency, (int)RCC_ClocksStatus.HCLK_Frequency,
        (int)RCC_ClocksStatus.PCLK1_Frequency, (int)RCC_ClocksStatus.PCLK2_Frequency,
        (int)SystemCoreClock);
    printf("interruptPriorityGrouping=%d\n", (int)(SCB->AIRCR >> 8) & 0x07); // NVIC_GetPriorityGrouping()
}

void core_print_mem_info(void)
{
    extern char _sdata;
    extern char _edata;
    extern char _sbss;
    extern char _ebss;
    extern char _estack;
    extern char end asm ("end"); /* Defined by the linker.  */
    printf("sdata    = %p\n", &_sdata);
    printf("edata    = %p\n", &_edata);
    printf("sbss     = %p\n", &_sbss);
    printf("ebss     = %p\n", &_ebss);
    printf("end      = %p\n", &end);
    printf("heap_end = %p\n", heap_end);
    printf("estack   = %p\n", &_estack);
    printf("sp       = %p\n", stack_ptr);
}

void core_fill_heap_and_stack(void)
{
    intptr_t beg = (intptr_t)heap_end;
    intptr_t end = (intptr_t)stack_ptr;
    beg = (beg + 3) & ~3;
    end = end & ~3;
    for (intptr_t i = beg; i < end; i+=4) {
        *(int *)i = 0x12345678;
    }
}

void core_dump_heap_and_stack(void)
{
    extern char _estack;
    intptr_t beg = (intptr_t)heap_end;
    intptr_t end = (intptr_t)&_estack;
    beg = (beg + 3) & ~3;
    int col = 0;
    for (intptr_t i = beg; i < end; i+=4) {
        if (col == 0)
            printf("%08x: ", i);
        else
            printf(" ");
        printf("%08x", *(int *)i);
        if (col == 3)
            printf("\n");
        col = (col + 1) & 3;
    }
}

void core_system_reset(void)
{
    NVIC_SystemReset();
    while (1) {
        // spin forever (should not reach this line)
    }
}

int core_interrupt_level(void)
{
    return SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
}

void SysTick_Handler(void)
{
    tick++;
}

/*
 * Dummy function to avoid compiler error
 */
void _init()
{

}

void _fini()
{

}

caddr_t _sbrk_r(struct _reent *r, int incr)
{
    extern char   end asm ("end"); /* Defined by the linker.  */
    char *        prev_heap_end;

    if (heap_end == NULL)
        heap_end = & end;

    prev_heap_end = heap_end;

    if (heap_end + incr > stack_ptr) {
        //errno = ENOMEM;
        return (caddr_t) -1;
    }

    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

/**
 * Called by the assert() macro.
 */
void __assert_func(const char *file, int line, const char *func, const char *assertion)
{
    // TODO
}


/*** stdlib ***/

int _close(int file) {
    return 0;
}

int _fstat(int file, struct stat *st) {
    return 0;
}

int _isatty(int file) {
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _open(const char *name, int flags, int mode) {
    return -1;
}

int _read(int file, char *ptr, int len) {
    if (file != 0 || len == 0)
        return 0;

    int c = _io.read_char();
    if (c < 0)
        return 0;

    *ptr = c;
    return 1;
}

int _write(int file, char *ptr, int len) {
    _io.write_buf(ptr, len);
    return len;
}


/*** module ***/

static struct reg_def _regs[] = {
    {
        .type = REG_TYPE_I32,
        .name = "tick",
        .value = &tick,
        .set = reg_fake_setter,
    },
};

static const struct cmd_def _cmds[] = {
    {
        .name = "cpu",
        .help = "show cpu info",
        .exec = _cpu_cmd,
    }, {
        .name = "mem",
        .help = "show memory info",
        .exec = _mem_cmd,
    }, {
        .name = "memfill",
        .help = "fill stack + heap",
        .exec = _memfill_cmd,
    }, {
        .name = "memdump",
        .help = "dump stack + heap",
        .exec = _memdump_cmd,
    }, {
        .name = "reset",
        .help = "CPU reset",
        .exec = _reset_cmd,
    }
};

const struct mod core_mod = {
    .name = "core",
    .description = "core features",
    .init_level = 0,
    .init = core_init,
    .reg_list = _regs,
    .reg_count = GMU_ARRAY_LEN(_regs),
    .cmd_list = _cmds,
    .cmd_count = GMU_ARRAY_LEN(_cmds),
};
