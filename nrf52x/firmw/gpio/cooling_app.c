#include <string.h>
#include "gpio_app.h"
#include "SEGGER_RTT.h"

APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.
static volatile bool ready_flag = true;            // A flag indicating PWM status.

void cooling_user_config(uint32_t temperature) {
    SEGGER_RTT_printf(0, "\nreceived custom data: %d\n", temperature);
    uint32_t value = temperature;
    if(ready_flag) {
        ready_flag = false;
        app_pwm_channel_duty_set(&PWM1, 0, value);
        SEGGER_RTT_printf(0, "\nset pwn %d%\n", 100-value);
    }
    
    // 15-? wait for temperature sensor
    // TODO : find relation PWM & temperature
}


void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
    SEGGER_RTT_printf(0, "\npwn ready\n");
}

void pwm_init()
{
    ret_code_t err_code;

    /* 2-channel PWM, 200Hz, output on DK LED pins. */
    // set the duty cycle to 1000
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(1000L, LED_PIN_2);

    /* Switch the polarity of the first channel. */
    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1, &pwm1_cfg, pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    app_pwm_enable(&PWM1);

    // init
    app_pwm_channel_duty_set(&PWM1, 0, 100);
}