#include "gpio_app.h"

APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM2,2);                   // Create the instance "PWM2" using TIMER2.

static volatile bool ready_flag = true;            // A flag indicating PWM status.

void cooling_user_config(uint32_t temperature) {
    NRF_LOG_INFO("received custom data: %d", temperature);
    uint32_t value = temperature;
    if(ready_flag) {
        ready_flag = false;
        app_pwm_channel_duty_set(&PWM2, 0, 100);
        NRF_LOG_INFO("SET PWM %d", value);
        nrf_gpio_pin_set(HBRIGE_NSLEEP_PIN);
    }
    
    // 15-? wait for temperature sensor
    // TODO : find relation PWM & temperature
}

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
    NRF_LOG_INFO("PWM ready: %d", pwm_id);
}

void pwm_app_init()
{
    ret_code_t err_code;

    /* 2-channel PWM, 200Hz, output on HBRIGE pins. */
    // set the duty cycle to 1000
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(1000L, HBRIGE_COOL_PIN, HBRIGE_WARM_PIN);
    app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_1CH(1000L, FAN_PIN);

    /* Switch the polarity of the first channel. */
    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
    pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;

    /* Switch the polarity of the first channel. */
    pwm2_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM1, &pwm1_cfg, pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_pwm_init(&PWM2, &pwm2_cfg, pwm_ready_callback);
    APP_ERROR_CHECK(err_code);

    app_pwm_enable(&PWM1);
    app_pwm_enable(&PWM2);

    // init duty at 0
    app_pwm_channel_duty_set(&PWM1, 0, 0);
    app_pwm_channel_duty_set(&PWM2, 0, 0);
}