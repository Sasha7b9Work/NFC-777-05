// 2024/08/23 09:03:34 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Hardware/HAL/HAL.h"
#include "system.h"


void HAL_IT_PB2::Init()
{
    // enable the key clock
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_CFGCMP);

    // configure button pin as input
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_2);

    // enable and set key EXTI interrupt to the lowest priority
    nvic_irq_enable(EXTI2_3_IRQn, 2U);

    // connect key EXTI line to key GPIO pin
    syscfg_exti_line_config(EXTI_SOURCE_GPIOB, EXTI_SOURCE_PIN12);

    // configure key EXTI line
    exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_2);
}


void HAL_IT_PB2::DeInit()
{
    exti_deinit();
}
