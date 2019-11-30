/*
 *  stepper.c
 *
 *  Copyright (c) 2019 Gabriele Mondada.
 *  This software is distributed under the terms of the MIT license.
 *  See https://opensource.org/licenses/MIT
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "stm32f4xx.h"
#include "gmutil.h"
#include "stepper.h"
#include "cli.h"
#include "core.h"
#include "ramp.h"


/*
 * Base clock: 168000000 Hz
 * Timer 1 + 8 clock: 168000000 Hz
 * Other Timer clock: 84000000 Hz
 * For a classic triangular waveform, the counter goes from 0 to RC_RANGE and
 * back to 0 with a frequency of:
 *   RC_PWM_FREQ = RC_PWM_COUNTER_FREQ / (2 * RC_RANGE)
 * We want a PWM clock of a multiple of 25 Hz.
 *   RC_PWM_FREQ = 20000 Hz
 *   RC_RANGE = 1050
 *   RC_PWM_COUNTER_FREQ = 42000000 Hz
 */
// #define RC_PWM_FREQ               20000 // Hz
#define RC_PWM_COUNTER_FREQ       42000000 // Hz
#define RC_RANGE                  (1050 * 2)


static int c;
static struct ramp ramp;
static float spd = RAMP_SPD;


static void _gpio_init(void)
{
    GPIO_InitTypeDef pgio_def = { 0 };
    pgio_def.GPIO_Mode = GPIO_Mode_AF;
    pgio_def.GPIO_OType = GPIO_OType_PP;
    pgio_def.GPIO_Speed = GPIO_Speed_100MHz;
    pgio_def.GPIO_PuPd = GPIO_PuPd_NOPULL;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    // setup GPIOs TIM1 (PE9, PE11, PE13, PE14)

    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9,  GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_TIM1);

    pgio_def.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_Init(GPIOE, &pgio_def);

    // setup GPIOs TIM2 (PA15, PB3, PB10, PB11)

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3,  GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_TIM2);

    pgio_def.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &pgio_def);
    pgio_def.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &pgio_def);

    // setup GPIOs TIM3 (PA6, PA7, PB0, PB1)

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6,  GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7,  GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource0,  GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource1,  GPIO_AF_TIM3);

    pgio_def.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &pgio_def);
    pgio_def.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_Init(GPIOB, &pgio_def);

    // setup GPIOs TIM4 (PD12, PD13, PD14, PD15)

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_TIM4);

    pgio_def.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOD, &pgio_def);

    // setup GPIOs TIM5 (PA0, PA1, PA2, PA3)

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource0,  GPIO_AF_TIM5);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource1,  GPIO_AF_TIM5);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2,  GPIO_AF_TIM5);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3,  GPIO_AF_TIM5);

    pgio_def.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &pgio_def);

    // setup GPIOs TIM8 (PC6, PC7, PC8, PC9)

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM8);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM8);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_TIM8);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_TIM8);

    pgio_def.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOC, &pgio_def);
}

static void _irq_init(TIM_TypeDef *tim)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    if (tim == TIM1) {
        NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
        NVIC_Init(&NVIC_InitStructure);
    }
    if (tim == TIM8) {
        NVIC_InitStructure.NVIC_IRQChannel = TIM8_UP_TIM13_IRQn;
        NVIC_Init(&NVIC_InitStructure);
    }

    TIM_ITConfig(tim, TIM_IT_Update, ENABLE);
}

static void _tim_init(TIM_TypeDef *tim)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    RCC_ClocksTypeDef RCC_ClocksFreq;
    int base_clock = 0;
    int tim_index = -1;

    // enable and configure clock
    RCC_GetClocksFreq(&RCC_ClocksFreq);
    if (tim == TIM1) {
        tim_index = 1;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK2_Frequency;
    }
    if (tim == TIM2) {
        tim_index = 2;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK1_Frequency;
    }
    if (tim == TIM3) {
        tim_index = 3;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK1_Frequency;
    }
    if (tim == TIM4) {
        tim_index = 4;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK1_Frequency;
    }
    if (tim == TIM5) {
        tim_index = 5;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK1_Frequency;
    }
    if (tim == TIM8) {
        tim_index = 8;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
        base_clock = (int)RCC_ClocksFreq.PCLK2_Frequency;
    }
    TIM_TimeBaseStructure.TIM_Period = RC_RANGE; // TIMx->ARR register
    TIM_TimeBaseStructure.TIM_Prescaler = (base_clock / RC_PWM_COUNTER_FREQ) - 1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned2; // TIMx->CR1 register
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0; // TIMx->RCR
    TIM_TimeBaseInit(tim, &TIM_TimeBaseStructure);
    printf("timer %d: base_clock=%d pwm_freq=%g range=%d\n",
            tim_index, base_clock,
            (double)base_clock / (TIM_TimeBaseStructure.TIM_Prescaler + 1) / 2 / RC_RANGE,
            RC_RANGE);

    // set output mode
    TIM_OCInitTypeDef TIM_OCInitStructure = {
        .TIM_OCMode = TIM_OCMode_PWM1,
        .TIM_OutputState = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_OCPolarity = TIM_OCPolarity_High,
        .TIM_OCNPolarity = TIM_OCNPolarity_High,
        .TIM_OCIdleState = TIM_OCIdleState_Reset,
    };
    TIM_OC1Init(tim, &TIM_OCInitStructure);
    TIM_OC2Init(tim, &TIM_OCInitStructure);
    TIM_OC3Init(tim, &TIM_OCInitStructure);
    TIM_OC4Init(tim, &TIM_OCInitStructure);

    // enable outputs (do TIMx->BDTR |= TIM_BDTR_MOE)
    TIM_CtrlPWMOutputs(tim, ENABLE);

    // set preload mode (working with shadow registers, see CCMR1)
    TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);

    // set counter starting value
    tim->CNT = 0;
}

static void _tim_start(TIM_TypeDef *tim)
{
    TIM_Cmd(tim, ENABLE);
}

static volatile uint32_t *_tim_reg(int port)
{
    TIM_TypeDef *tims[] = {TIM1, TIM2, TIM3, TIM4, TIM5, TIM8};
    TIM_TypeDef *tim = tims[port / 4];
    return &tim->CCR1 + (port % 4);
}

static void _esc_handler(void)
{
    // rc_enable(0);
}

static inline float shape(float x)
{
    float y = sinf(x);
    if (y > 0)
        return 1;
    if (y < 0)
        return -1;
    return 0;
}

static void _init(void)
{
    ramp_init(&ramp);

    cli_add_esc_handler(_esc_handler);

    _gpio_init();

    // setup timers
    _tim_init(TIM1);
    _tim_init(TIM2);
    _tim_init(TIM3);
    _tim_init(TIM4);
    _tim_init(TIM5);
    _tim_init(TIM8);

    // setup interrupt
    _irq_init(TIM1);

    // start all
    _tim_start(TIM1);
    _tim_start(TIM2);
    _tim_start(TIM3);
    _tim_start(TIM4);
    _tim_start(TIM5);
    _tim_start(TIM8);
}

static void _loop(void)
{
}

// run at 10kHz
static void _cycle(void)
{
    static bool started = false;
    if (!started) {
        started = true;
        ramp_start(&ramp);
    }

    float alpha = ramp_cycle(&ramp);
    float a = sinf(alpha);
    float b = cosf(alpha);
    stepper_pwm(0, a);
    stepper_pwm(1, -a);
    stepper_pwm(2, b);
    stepper_pwm(3, -b);
}

#if 0
static void _cycle(void)
{
    static int t = 0;

    t++;

    float speed = 50.0f; // electric tours per second
    float n = (float)t * (0.0001f * speed); // in tours
    float a = sinf(n * 2.0f * (float)M_PI);
    float b = sinf((n + 0.25f) * 2.0f * (float)M_PI);
    stepper_pwm(0, a);
    stepper_pwm(1, -a);
    stepper_pwm(2, b);
    stepper_pwm(3, -b);
}
#endif

void _stat_cmd(const struct cmd_def *def, struct cmd_ctx ctx, struct mod_arg_iterator *arg_it)
{
    int t1a = TIM1->CNT;
    int t2  = TIM2->CNT;
    int t8  = TIM8->CNT;
    int t1b = TIM1->CNT;
    printf("tim1=%d tim2=%d tim8=%d tim1=%d\n", t1a, t2, t8, t1b);
    printf("tim1.ccr=%d,%d,%d,%d\n", (int)TIM1->CCR1, (int)TIM1->CCR2, (int)TIM1->CCR3, (int)TIM1->CCR4);
    printf("tim8.ccr=%d,%d,%d,%d\n", (int)TIM8->CCR1, (int)TIM8->CCR2, (int)TIM8->CCR3, (int)TIM8->CCR4);
    printf("c=%d\n", c);
}

void _spd_reg_set(const struct reg_def *def, struct reg_ctx ctx, const void *val)
{
    memcpy(def->value, val, sizeof(float));
    float spd = gmu_get_as_f32(val);
    ramp_set_spd(&ramp, spd);
}

void stepper_pwm(int port, float value)
{
    volatile uint32_t *reg = _tim_reg(port);

    int pwm = (int)roundf((0.5f + 0.5f * value) * (RC_RANGE - 1));
    *reg = pwm;
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    c++;
    if ((c % 8) == 0)
        _cycle();
}

static struct reg_def _regs[] = {
    {
        .type = REG_TYPE_F32,
        .value = &spd,
        .name = "stspd",
        .help = "stapper speed in electric tours per seconds",
        .set = _spd_reg_set,
    }
};

static const struct cmd_def _cmds[] = {
    {
        .name = "ststat",
        .help = "stapper state",
        .exec = _stat_cmd,
    }
};

const struct mod stepper_mod = {
    .name = "st",
    .description = "stepper control",
    .init_level = 12,
    .init = _init,
    .loop = _loop,
    .reg_list = _regs,
    .reg_count = GMU_ARRAY_LEN(_regs),
    .cmd_list = _cmds,
    .cmd_count = GMU_ARRAY_LEN(_cmds),
};
