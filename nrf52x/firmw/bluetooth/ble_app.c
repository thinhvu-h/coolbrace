#include "ble_app.h"
#include "gpio_app.h"
#include "battery_app.h"
#include "temperature_app.h"
#include "ble_cus.h"

#include "nrf_log.h"

static uint16_t          m_conn_handle = BLE_CONN_HANDLE_INVALID;         /**< Handle of the current connection. */
static bool              m_hts_meas_ind_conf_pending = true;              /**< Flag to keep track of when an indication confirmation is pending. */
static bool              m_bas_notif_enable = false;                       /**< Flag to keep track of when an notify is enable. */

BLE_CUS_DEF(m_cus);                                                                 /**< Custom Service instance. */
// BLE_LBS_DEF(m_lbs);                                                                 /**< LED Button Service instance. */
BLE_HTS_DEF(m_hts);                                                                 /**< Structure used to identify the health thermometer service. */
BLE_BAS_DEF(m_bas);                                                                 /**< Structure used to identify the battery service. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */
NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                                    /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_PERIPHERAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);

static ble_uuid_t m_adv_uuids[] =                                                   /**< Universally unique service identifiers. */
{
    {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
};

typedef enum
{
	BATT_EVT_TIMEOUT = 0x01,
	TEMPERATURE_EVT_TIMEOUT
} sensor_timeout_evt_t;

static void battery_level_update(void);
static void temperature_update(void);


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
// void notification_timeout_handler(void * p_context)
// {
//     UNUSED_PARAMETER(p_context);
//     ret_code_t err_code;
    
//     // Increment the value of m_custom_value before nortifing it.
//     m_custom_value++;
    
//     err_code = ble_cus_custom_value_update(&m_cus, m_custom_value);
//     APP_ERROR_CHECK(err_code);
// }

/**@brief Handle events from app_timer .
 *
 * @param[in]   p_event_data   	Event data to be scheduled
 * @param[in]   event_size   	Size of event data to be scheduled.
 */
static void sensor_timeout_sched_handler(void *p_event_data, uint16_t event_size)
{
    UNUSED_PARAMETER(event_size);
    uint8_t *p_evt = p_event_data;
    // NRF_LOG_INFO("sensor_timeout_sched_handler: %d", *p_evt);
    switch (*p_evt)
    {
    	case BATT_EVT_TIMEOUT://display on
			battery_level_update();
    		break;
		
		case TEMPERATURE_EVT_TIMEOUT://display refresh
			temperature_update();
    		break;
		
    	default:
			NRF_LOG_DEBUG("unknown event");
    		break;
    }
}

/**@brief Function for performing a battery measurement, and update the Battery Level characteristic in the Battery Service.
 */
static void battery_level_update(void)
{
    ret_code_t err_code;
    uint8_t  battery_level;
    uint16_t vbatt;              // Variable to hold voltage reading
    battery_voltage_get(&vbatt); // Get new battery voltage

    battery_level = battery_level_percent(vbatt);    //Transform the millivolts value into battery level percent.
    if (m_bas_notif_enable) {
        err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != NRF_ERROR_RESOURCES) &&
            (err_code != NRF_ERROR_BUSY) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
        {
            NRF_LOG_INFO("Battery level: %d%s Sending => FAILED", battery_level, "%");
            APP_ERROR_HANDLER(err_code);
        } else {
            NRF_LOG_INFO("Battery level: %d%s Sending => SUCCESSED", battery_level, "%");
        }
    } else {
        NRF_LOG_INFO("Battery level: %d%s", battery_level, "%");
    }
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
void battery_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    uint8_t batt_sched = BATT_EVT_TIMEOUT;
    app_sched_event_put(&batt_sched, sizeof(batt_sched), sensor_timeout_sched_handler);
}

/**@brief Function for simulating and sending one Temperature Measurement.
 */
static void temperature_update(void)
{
    ret_code_t     err_code;
    ble_hts_meas_t temperature_meas;
    
    temperature_measurement(&temperature_meas);
    if (!m_hts_meas_ind_conf_pending)
    {
        err_code = ble_hts_measurement_send(&m_hts, &temperature_meas);
        switch (err_code)
        {
            case NRF_SUCCESS:
                NRF_LOG_INFO("Temperature: %dC Sending => SUCCESSED", temperature_meas.temp_in_celcius.mantissa);
                // Measurement was successfully sent, wait for confirmation.
                m_hts_meas_ind_conf_pending = true;
                break;

            case NRF_ERROR_INVALID_STATE:
                NRF_LOG_INFO("Temperature: %dC Sending => INVALID_STATE", temperature_meas.temp_in_celcius.mantissa);
                // Ignore error.
                break;

            default:
                NRF_LOG_INFO("Temperature: %dC Sending => FAILED", temperature_meas.temp_in_celcius.mantissa);
                APP_ERROR_HANDLER(err_code);
                break;
        }
    } else {
        NRF_LOG_INFO("Temperature: %dC", temperature_meas.temp_in_celcius.mantissa);
    }
}


/**@brief Function for handling the Temperature measurement timer timeout.
 *
 * @details This function will be called each time the temperature measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
void temperature_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    uint8_t temp_sched = TEMPERATURE_EVT_TIMEOUT;
    app_sched_event_put(&temp_sched, sizeof(temp_sched), sensor_timeout_sched_handler);
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;
    bool       is_indication_enabled;

    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_CONN_SEC_SUCCEEDED:
            NRF_LOG_INFO("PM_EVT_CONN_SEC_SUCCEEDED");
            err_code = ble_hts_is_indication_enabled(&m_hts, &is_indication_enabled);
            APP_ERROR_CHECK(err_code);
            if (is_indication_enabled) {
                m_hts_meas_ind_conf_pending = false;
            }
            break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            NRF_LOG_INFO("PM_EVT_PEERS_DELETE_SUCCEEDED");
            advertising_start();
            break;

        default:
            break;
    }
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                           strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_THERMOMETER);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Handler for shutdown preparation.
 *
 * @details During shutdown procedures, this function will be called at a 1 second interval
 *          untill the function returns true. When the function returns true, it means that the
 *          app is ready to reset to DFU mode.
 *
 * @param[in]   event   Power manager event.
 *
 * @retval  True if shutdown is allowed by this power manager handler, otherwise false.
 */
static bool app_shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
    switch (event)
    {
        case NRF_PWR_MGMT_EVT_PREPARE_DFU:
            // NRF_LOG_INFO("Power management wants to reset to DFU mode.");
            // YOUR_JOB: Get ready to reset into DFU mode
            //
            // If you aren't finished with any ongoing tasks, return "false" to
            // signal to the system that reset is impossible at this stage.
            //
            // Here is an example using a variable to delay resetting the device.
            //
            // if (!m_ready_for_reset)
            // {
            //      return false;
            // }
            // else
            //{
            //
            //    // Device ready to enter
            //    uint32_t err_code;
            //    err_code = sd_softdevice_disable();
            //    APP_ERROR_CHECK(err_code);
            //    err_code = app_timer_stop_all();
            //    APP_ERROR_CHECK(err_code);
            //}
            break;

        default:
            // YOUR_JOB: Implement any of the other events available from the power management module:
            //      -NRF_PWR_MGMT_EVT_PREPARE_SYSOFF
            //      -NRF_PWR_MGMT_EVT_PREPARE_WAKEUP
            //      -NRF_PWR_MGMT_EVT_PREPARE_RESET
            return true;
    }

    // NRF_LOG_INFO("Power management allowed to reset to DFU mode.");
    return true;
}

/**@brief Register application shutdown handler with priority 0.
 */
NRF_PWR_MGMT_HANDLER_REGISTER(app_shutdown_handler, 0);

static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void * p_context)
{
    if (state == NRF_SDH_EVT_STATE_DISABLED)
    {
        // Softdevice was disabled before going into reset. Inform bootloader to skip CRC on next boot.
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        //Go to system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }
}

/* nrf_sdh state observer. */
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) =
{
    .handler = buttonless_dfu_sdh_state_observer,
};

static void advertising_config_get(ble_adv_modes_config_t * p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled  = true;
    p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_DURATION;
}

static void disconnect(uint16_t conn_handle, void * p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
    }
    else
    {
        NRF_LOG_INFO("Disconnected connection handle %d", conn_handle);
    }
}

/**@brief Function for handling dfu events from the Buttonless Secure DFU service
 *
 * @param[in]   event   Event from the Buttonless Secure DFU service.
 */
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
        {
            NRF_LOG_INFO("Device is preparing to enter bootloader mode.");

            // Prevent device from advertising on disconnect.
            ble_adv_modes_config_t config;
            advertising_config_get(&config);
            config.ble_adv_on_disconnect_disabled = true;
            ble_advertising_modes_config_set(&m_advertising, &config);

            // Disconnect all other bonded devices that currently are connected.
            // This is required to receive a service changed indication
            // on bootup after a successful (or aborted) Device Firmware Update.
            uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
            NRF_LOG_INFO("Disconnected %d links.", conn_count);
            break;
        }

        case BLE_DFU_EVT_BOOTLOADER_ENTER:
            // YOUR_JOB: Write app-specific unwritten data to FLASH, control finalization of this
            //           by delaying reset by reporting false in app_shutdown_handler
            NRF_LOG_INFO("Device will enter bootloader mode.");
            break;

        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
            NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
            // YOUR_JOB: Take corrective measures to resolve the issue
            //           like calling APP_ERROR_CHECK to reset the device.
            break;

        case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
            NRF_LOG_ERROR("Request to send a response to client failed.");
            // YOUR_JOB: Take corrective measures to resolve the issue
            //           like calling APP_ERROR_CHECK to reset the device.
            APP_ERROR_CHECK(false);
            break;

        default:
            NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
            break;
    }
}

/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    NRF_LOG_INFO("service_error_handler!!!!");
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Queued Write module.
 */
static void qwr_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init_obj = {0};

    // Initialize Queued Write Module.
    qwr_init_obj.error_handler = service_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Battery Service events.
 *
 * @details This function will be called for all Battery Service events which are passed to the
 |          application.
 *
 * @param[in] p_bas  Battery Service structure.
 * @param[in] p_evt  Event received from the Battery Service.
 */
void on_bas_evt(ble_bas_t * p_bas, ble_bas_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_BAS_EVT_NOTIFICATION_ENABLED:
            m_bas_notif_enable = true;
            NRF_LOG_INFO("BLE_BAS_EVT_NOTIFICATION_ENABLED");
            break; // BLE_BAS_EVT_NOTIFICATION_ENABLED

        case BLE_BAS_EVT_NOTIFICATION_DISABLED:
            m_bas_notif_enable = false;
            NRF_LOG_INFO("BLE_BAS_EVT_NOTIFICATION_DISABLED");
            break; // BLE_BAS_EVT_NOTIFICATION_DISABLED

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for initializing the Battery Service.
 */
static void bas_init(void)
{
    ret_code_t     err_code;
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler          = on_bas_evt;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = NULL;
    bas_init_obj.initial_batt_level   = 100;

    bas_init_obj.bl_rd_sec        = SEC_OPEN;
    bas_init_obj.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init_obj.bl_report_rd_sec = SEC_OPEN;

    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Health Thermometer Service events.
 *
 * @details This function will be called for all Health Thermometer Service events which are passed
 *          to the application.
 *
 * @param[in] p_hts  Health Thermometer Service structure.
 * @param[in] p_evt  Event received from the Health Thermometer Service.
 */
static void on_hts_evt(ble_hts_t * p_hts, ble_hts_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_HTS_EVT_INDICATION_ENABLED:
            NRF_LOG_INFO("received BLE_HTS_EVT_INDICATION_ENABLED");
            m_hts_meas_ind_conf_pending = false;
            break;

        case BLE_HTS_EVT_INDICATION_CONFIRMED:
            NRF_LOG_INFO("received BLE_HTS_EVT_INDICATION_CONFIRMED");
            m_hts_meas_ind_conf_pending = false;
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for initializing the Health Thermometer Service.
 */
static void hts_init(void)
{
    ret_code_t     err_code;
    ble_hts_init_t hts_init_obj;

    memset(&hts_init_obj, 0, sizeof(hts_init_obj));

    hts_init_obj.evt_handler                 = on_hts_evt;
    hts_init_obj.p_gatt_queue                = &m_ble_gatt_queue;
    hts_init_obj.error_handler               = service_error_handler;
    hts_init_obj.temp_type_as_characteristic = TEMP_TYPE_AS_CHARACTERISTIC;
    hts_init_obj.temp_type                   = BLE_HTS_TEMP_TYPE_BODY;

    // Here the sec level for the Health Thermometer Service can be changed/increased.
    hts_init_obj.ht_meas_cccd_wr_sec = SEC_JUST_WORKS;
    hts_init_obj.ht_type_rd_sec      = SEC_OPEN;

    err_code = ble_hts_init(&m_hts, &hts_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Custom Service Service events.
 *
 * @details This function will be called for all Custom Service events which are passed to
 *          the application.
 *
 * @param[in]   p_cus_service  Custom Service structure.
 * @param[in]   p_evt          Event received from the Custom Service.
 *
 */
static void on_cus_evt(ble_cus_t     * p_cus_service,
                       ble_cus_evt_t * p_evt)
{
    // ret_code_t err_code;
    
    switch(p_evt->evt_type)
    {
        case BLE_CUS_EVT_NOTIFICATION_ENABLED:
            
            //  err_code = app_timer_start(m_notification_timer_id, NOTIFICATION_INTERVAL, NULL);
            //  APP_ERROR_CHECK(err_code);
             break;

        case BLE_CUS_EVT_NOTIFICATION_DISABLED:

            // err_code = app_timer_stop(m_notification_timer_id);
            // APP_ERROR_CHECK(err_code);
            break;

        case BLE_CUS_EVT_CONNECTED:
            break;

        case BLE_CUS_EVT_DISCONNECTED:
              break;

        default:
              // No implementation needed.
              break;
    }
}

/**@brief Function for initializing the Custom Service.
 */
static void cus_init(void)
{
    ret_code_t         err_code;
    ble_cus_init_t     cus_init_obj;

    memset(&cus_init_obj, 0, sizeof(cus_init_obj));

    cus_init_obj.evt_handler                = on_cus_evt;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init_obj.custom_value_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init_obj.custom_value_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cus_init_obj.custom_value_char_attr_md.write_perm);

    err_code = ble_cus_init(&m_cus, &cus_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the DFU Service.
 */
static void dfu_init(void)
{
    ret_code_t         err_code;
    ble_dfu_buttonless_init_t dfus_init_obj;

    memset(&dfus_init_obj, 0, sizeof(dfus_init_obj));
    
    dfus_init_obj.evt_handler = ble_dfu_evt_handler;

    err_code = ble_dfu_buttonless_init(&dfus_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Device Information Service.
 */
static void dis_init(void)
{
    ret_code_t         err_code;
    ble_dis_sys_id_t   sys_id_obj;
    ble_dis_init_t     dis_init_obj;

    memset(&sys_id_obj, 0, sizeof(sys_id_obj));
    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

    ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, MANUFACTURER_NAME);
    ble_srv_ascii_to_utf8(&dis_init_obj.model_num_str, MODEL_NUM);

    sys_id_obj.manufacturer_id            = MANUFACTURER_ID;
    sys_id_obj.organizationally_unique_id = ORG_UNIQUE_ID;
    dis_init_obj.p_sys_id                 = &sys_id_obj;

    dis_init_obj.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init_obj);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Health Thermometer, Battery, Custom, DFU and Device Information services.
 */
void services_init(void)
{
    qwr_init();
    bas_init();
    hts_init();
    cus_init();
    dfu_init();
    dis_init();    
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            app_led_blink();
            break;

        case BLE_ADV_EVT_IDLE:
            app_led_off();
            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("BLE Connected.");
            m_bas_notif_enable = true;
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            app_led_on();
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("BLE Disconnected, reason %d.", p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle               = BLE_CONN_HANDLE_INVALID;
            m_hts_meas_ind_conf_pending = true;
            m_bas_notif_enable = false;
            app_led_off();
            advertising_start();
            break;

        case BLE_GAP_EVT_ADV_SET_TERMINATED:
            app_led_off();
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // NRF_LOG_INFO("GATT Client Timeout.");
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // NRF_LOG_INFO("GATT Server Timeout.");
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the Peer Manager initialization.
 */
void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
void advertising_start()
{
    uint32_t err_code;
    err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    if(err_code != NRF_SUCCESS) {
        // Ignore error.
    }
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.config.ble_adv_on_disconnect_disabled = true;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}