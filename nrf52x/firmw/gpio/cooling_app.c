#include "gpio_app.h"
#include "nrf_delay.h"


APP_PWM_INSTANCE(PWM1,1);                   // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM2,2);                   // Create the instance "PWM2" using TIMER2.

static volatile bool ready_flag = false;            // A flag indicating PWM status.

static void fan_cooling_disable() {
    while (app_pwm_channel_duty_set(&PWM1, 0, 0) == NRF_ERROR_BUSY);
    nrf_gpio_pin_clear(HBRIGE_NSLEEP_PIN);
    NRF_LOG_INFO("put hbrige to sleep");
}

static void fan_enable(uint8_t level) {
    while (app_pwm_channel_duty_set(&PWM1, 0, level) == NRF_ERROR_BUSY);
    NRF_LOG_INFO("COOL MODE, SET FAN PWM: %d", level);
}

static void cooling_enable(uint8_t level) {
    nrf_gpio_pin_clear(HBRIGE_COOL_PIN);
    while (app_pwm_channel_duty_set(&PWM2, 1, level) == NRF_ERROR_BUSY);
    NRF_LOG_INFO("COOL MODE, SET COOLING PWM: %d", level);
    nrf_gpio_pin_set(HBRIGE_NSLEEP_PIN); 
    NRF_LOG_INFO("set hbrige pin");
}

static void heating_enable(uint8_t level) {
    nrf_gpio_pin_clear(HBRIGE_WARM_PIN);
    while (app_pwm_channel_duty_set(&PWM2, 0, level) == NRF_ERROR_BUSY);
    NRF_LOG_INFO("COOL MODE, SET HEATING PWM: %d", level);
    nrf_gpio_pin_set(HBRIGE_NSLEEP_PIN); 
    NRF_LOG_INFO("set hbrige pin");
}

void cooling_handler(uint32_t cfg_data) {
    NRF_LOG_INFO("received custom data: %d", cfg_data);

    if(cfg_data == 0) {
        fan_cooling_disable();
    } else if (cfg_data == 11) {
        fan_enable(25);
        cooling_enable(25);
    } else if (cfg_data == 12) {
        fan_enable(50);
        cooling_enable(50);
    } else if (cfg_data == 13) {
        fan_enable(75);
        cooling_enable(75);
    } else if (cfg_data == 21) {
        fan_enable(25);
        heating_enable(25);
    } else if (cfg_data == 22) {
        fan_enable(50);
        heating_enable(50);
    } else {
        // do nothing
    }
}

void pwm_app_init()
{
    ret_code_t err_code;

    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(100000L, FAN_PIN);
    
    /* Switch the polarity of the first channel. */
    pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;

    err_code = app_pwm_init(&PWM1, &pwm1_cfg, NULL);
    APP_ERROR_CHECK(err_code);

    app_pwm_enable(&PWM1);    
    while (app_pwm_channel_duty_set(&PWM1, 0, 0) == NRF_ERROR_BUSY);

    /* 2-channel PWM, 200Hz, output on HBRIGE pins. */
    // set the duty cycle to 1000
    app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(100000L, HBRIGE_COOL_PIN, HBRIGE_WARM_PIN);

    /* Switch the polarity of the first channel. */
    pwm2_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
    pwm2_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;

    /* Initialize and enable PWM. */
    err_code = app_pwm_init(&PWM2, &pwm2_cfg, NULL);
    APP_ERROR_CHECK(err_code);

    app_pwm_enable(&PWM2);
    // init duty at 0
    while (app_pwm_channel_duty_set(&PWM2, 0, 0) == NRF_ERROR_BUSY);
    while (app_pwm_channel_duty_set(&PWM2, 1, 0) == NRF_ERROR_BUSY);
}