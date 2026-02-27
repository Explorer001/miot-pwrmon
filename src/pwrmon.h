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

enum miot_pwrmon_channel {
    MIOT_PWRMON_CHANNEL_LDO = 0x01,            /**< LDO power supply */
    MIOT_PWRMON_CHANNEL_RFM95W = 0x02,         /**< RFM95W 868MHz LoRa Transceiver */
    MIOT_PWRMON_CHANNEL_CC1101 = 0x04,         /**< CC1101 433MHz Transceiver */
    MIOT_PWRMON_CHANNEL_EEPROM_SENSORS = 0x08, /**< EEPROM storage and sensors */
    MIOT_PWRMON_CHANNEL_NRF24L01 = 0x10,       /**< nRF 2.4GHz Transceiver */
    MIOT_PWRMON_CHANNEL_AT86 = 0x20,           /**< AT86 IEEE 802.15.4 Transceiver */
};

int miot_pwrmon_init(void);
