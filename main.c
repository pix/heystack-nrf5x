/**
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief Blinky Sample Application main file.
 *
 * This file contains the source code for a sample server application using the LED Button service.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "main.h"

// Create space for MAX_KEYS public keys
static const char public_key[MAX_KEYS+1][28] = {
    [0] = "OFFLINEFINDINGPUBLICKEYHERE!",
    [MAX_KEYS] = "ENDOFKEYSENDOFKEYSENDOFKEYS!",
};

int last_filled_index = -1;
int current_index = 0;

// Define timer ID variable
APP_TIMER_DEF(m_key_change_timer_id);

// Timer interval definition (example: 1000 ms)
#define TIMER_INTERVAL COMPAT_APP_TIMER_TICKS(KEY_ROTATION_INTERVAL * 1000)  // Timer interval in ticks (assuming 1 second interval)

#if defined(RANDOM_ROTATE_KEYS) && RANDOM_ROTATE_KEYS == 1
#include "nrf_drv_rng.h"
#include "nrf_rng.h"

int randmod(int mod) {
    if (mod <= 0) {
        return -1;  // Invalid modulus.
    }

    uint8_t buffer[2];  // Buffer to hold 2 random bytes (16 bits).
    uint16_t x;
    const uint16_t R_MAX = (UINT16_MAX / mod) * mod;

    uint8_t bytes_available = 0;
    uint32_t err_code;

    // Wait until there are enough random bytes available (at least 2 bytes).
    do {
        err_code = sd_rand_application_bytes_available_get(&bytes_available);
        APP_ERROR_CHECK(err_code);
    } while (bytes_available < sizeof(buffer));

    do {
        // Get 2 random bytes and combine them into a 16-bit number.
        err_code = sd_rand_application_vector_get(buffer, sizeof(buffer));
        APP_ERROR_CHECK(err_code);

        // Combine the two bytes into a 16-bit integer.
        x = (buffer[0] << 8) | buffer[1];
    } while (x >= R_MAX);  // Discard if the number is out of the acceptable range.

    return x % mod;  // Return the modulo result.
}

#endif

#ifdef HAS_RADIO_PA
// Credits: https://forum.mysensors.org/topic/10198/nrf51-52-pa-not-support
static void pa_lna_assist(uint32_t gpio_pa_pin, uint32_t gpio_lna_pin)
{
    ret_code_t err_code;

    static const uint32_t gpio_toggle_ch = 0;
    static const uint32_t ppi_set_ch = 0;
    static const uint32_t ppi_clr_ch = 1;

    // Configure SoftDevice PA/LNA assist
    ble_opt_t opt;
    memset(&opt, 0, sizeof(ble_opt_t));
    // Common PA/LNA config
    opt.common_opt.pa_lna.gpiote_ch_id  = gpio_toggle_ch;        // GPIOTE channel
    opt.common_opt.pa_lna.ppi_ch_id_clr = ppi_clr_ch;            // PPI channel for pin clearing
    opt.common_opt.pa_lna.ppi_ch_id_set = ppi_set_ch;            // PPI channel for pin setting
    // PA config
    opt.common_opt.pa_lna.pa_cfg.active_high = 1;                // Set the pin to be active high
    opt.common_opt.pa_lna.pa_cfg.enable      = 1;                // Enable toggling
    opt.common_opt.pa_lna.pa_cfg.gpio_pin    = gpio_pa_pin;      // The GPIO pin to toggle

    // LNA config
    opt.common_opt.pa_lna.lna_cfg.active_high  = 1;              // Set the pin to be active high
    opt.common_opt.pa_lna.lna_cfg.enable       = 1;              // Enable toggling
    opt.common_opt.pa_lna.lna_cfg.gpio_pin     = gpio_lna_pin;   // The GPIO pin to toggle

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_PA_LNA, &opt);
    APP_ERROR_CHECK(err_code);
    COMPAT_NRF_LOG_INFO("PA/LNA assist enabled on pins: PA=%d, LNA=%d", gpio_pa_pin, gpio_lna_pin);
}
#endif

void set_and_advertise_next_key(void *p_context)
{
    #if defined(RANDOM_ROTATE_KEYS) && RANDOM_ROTATE_KEYS == 1
        // Update key index for next advertisement...Back to zero if out of range
        current_index =  randmod(last_filled_index + 1);
    #else
        // rotate to next key in the list modulo the last filled index
        current_index = (current_index + 1) % (last_filled_index + 1);
    #endif

    #if defined(BATTERY_LEVEL) && BATTERY_LEVEL == 1
        update_battery_level();
    #endif
    
    // Set key to be advertised
    ble_set_advertisement_key(public_key[current_index]);
    COMPAT_NRF_LOG_INFO("Rotating key: %d", current_index);
}

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    #if NRF_SDK_VERSION < 15
        // Specify the timer operation queue size, e.g., 10
        APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
    #else  // For SDK 12 and below
        int err_code = app_timer_init();
        APP_ERROR_CHECK(err_code);
    #endif
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */

void ble_stack_init(void)
{
    ret_code_t err_code;

    #if NRF_SDK_VERSION >= 15
        err_code = nrf_sdh_enable_request();
        APP_ERROR_CHECK(err_code);

        // Configure the BLE stack using the default settings.
        uint32_t ram_start = 0;
        err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        APP_ERROR_CHECK(err_code);

        // Enable BLE stack.
        err_code = nrf_sdh_ble_enable(&ram_start);
        APP_ERROR_CHECK(err_code);

    #else  // SDK 12 and earlier
        #define CENTRAL_LINK_COUNT 0
        #define PERIPHERAL_LINK_COUNT 1
        #define BLE_UUID_VS_COUNT_MIN 1

        nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

        // Initialize the SoftDevice handler module.
        SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

        // Fetch default configuration for BLE enable parameters.
        ble_enable_params_t ble_enable_params;
        err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,    // central link count
                                                        PERIPHERAL_LINK_COUNT, // peripheral link count
                                                        &ble_enable_params);
        APP_ERROR_CHECK(err_code);

        // Set custom UUID count (if needed).
        ble_enable_params.common_enable_params.vs_uuid_count = BLE_UUID_VS_COUNT_MIN;

        // Check the RAM settings against the used number of links.
        CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

        // Enable BLE stack.
        err_code = softdevice_enable(&ble_enable_params);
        APP_ERROR_CHECK(err_code);

    #endif
}


static void log_init(void)
{
#if defined(HAS_DEBUG) && HAS_DEBUG == 1

    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

#if NRF_SDK_VERSION >= 15
    NRF_LOG_DEFAULT_BACKENDS_INIT();
#else
#endif
#endif
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    #if NRF_SDK_VERSION >= 15
        ret_code_t err_code;
        err_code = nrf_pwr_mgmt_init();
        APP_ERROR_CHECK(err_code);
    #else
    #endif
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        #if NRF_SDK_VERSION >= 15
        nrf_pwr_mgmt_run();
        #else
        APP_ERROR_CHECK(sd_app_evt_wait());
        #endif
    }
}

// Function to configure the timer
static void timer_config(void)
{
    uint32_t err_code;

    // Create the timer. It will trigger the 'set_and_advertise_next_key' function on each timeout.
    err_code = app_timer_create(&m_key_change_timer_id, APP_TIMER_MODE_REPEATED, set_and_advertise_next_key);
    APP_ERROR_CHECK(err_code);

    // Start the timer with the specified interval.
    err_code = app_timer_start(m_key_change_timer_id, TIMER_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    log_init();

    // Find the last filled index
    for (int i = MAX_KEYS - 2; i >= 0; i--)
    {
        if (strlen(public_key[i]) > 0)
        {
            last_filled_index = i;
            break;
        }
    }
    COMPAT_NRF_LOG_INFO("last_filled_index: %d", last_filled_index);

    // Initialize the timer module.
    timers_init();

    // Configure the timer for key rotation if there are multiple keys
    if (last_filled_index > 0)
    {
        timer_config();
    }
    
    // Initialize the power management module.
    power_management_init();

    // Initialize the BLE stack.
    ble_stack_init();

    // Initialize advertising.
    ble_advertising_init();

#ifdef HAS_RADIO_PA
    // Configure the PA/LNA
    pa_lna_assist(GPIO_PA_PIN, GPIO_LNA_PIN);
#endif


    COMPAT_NRF_LOG_INFO("Starting advertising");

    // Set the first key to be advertised
    set_and_advertise_next_key(NULL);

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


/**
 * @}
 */
