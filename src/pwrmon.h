/*
 * Copyright (C) 2026  Lukas Gehreke
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

#include "ina3221_params.h"

#define MIOT_PWRMON_CHANNEL_MASK 0x3F

/** Available power monitor channels */
enum miot_pwrmon_channel {
    MIOT_PWRMON_CHANNEL_LDO = 0x01,            /**< LDO power supply */
    MIOT_PWRMON_CHANNEL_RFM95W = 0x02,         /**< RFM95W 868MHz LoRa Transceiver */
    MIOT_PWRMON_CHANNEL_CC1101 = 0x04,         /**< CC1101 433MHz Transceiver */
    MIOT_PWRMON_CHANNEL_EEPROM_SENSORS = 0x08, /**< EEPROM storage and sensors */
    MIOT_PWRMON_CHANNEL_NRF24L01 = 0x10,       /**< nRF 2.4GHz Transceiver */
    MIOT_PWRMON_CHANNEL_AT86 = 0x20,           /**< AT86 IEEE 802.15.4 Transceiver */
};

/** Power monitor operation mode */
enum miot_pwrmon_op_mode {
    MIOT_PWRMON_OP_MODE_CURRENT, /**< Only measure current */
    MIOT_PWRMON_OP_MODE_VOLTAGE, /**< Only measure voltage */
    MIOT_PWRMON_OP_MODE_BOTH,    /**< Measure current and voltage*/
};

/** Configuration for the power monitor */
struct pwrmon_cfg {
    /** Bitmap of enabled channels enabled for measurement */
    uint8_t channels;
    /** Operation mode */
    enum miot_pwrmon_op_mode op_mode;
    /** Shunt voltage conversion time */
    ina3221_conv_time_shunt_adc_t sadc;
    /** Bus voltage conversion time */
    ina3221_conv_time_bus_adc_t badc;
    /** Number of samples for averaging */
    ina3221_num_samples_t num_samples;
};

/**
 * Initialize the power monitor.
 *
 * @return 0 on success. Otherwise a negative error code. 
 */
int miot_pwrmon_init(void);

/**
 * Start the power monitor measurement.
 *
 * @param cfg The power monitor configuration.
 * @return 0 on success. Otherwise a negative error code. 
 */
int miot_pwrmon_start_meas(const struct pwrmon_cfg *cfg);
