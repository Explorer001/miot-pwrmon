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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "pwrmon.h"
#include "ztimer.h"

static void _usage(void)
{
    printf("version\t- Print the version string\n"
           "init\t\t- Initialize the power monitor\n"
           "measure\t- Start a measurement\n"
           "stop\t\t- Stop the current measurement\n");
}

static void _measure_usage(void)
{
    printf("measure <chan_map> <op_mode> <sadc> <badc> <samples>\n"
           "chan_map: Hexadecimal map of channels enabled for measurement\n"
           "op_mode : Operation mode string\n"
           "            CURRENT - Only measure current\n"
           "            VOLTAGE - Only measure voltage\n"
           "            BOTH    - Measure current and voltage\n"
           "sadc    : Shunt voltage conversion time\n"
           "            Refer to badc for possible values\n"
           "badc    : Bus voltage conversion time\n"
           "            0 - 140uS\n"
           "            1 - 204uS\n"
           "            2 - 332uS\n"
           "            3 - 588uS\n"
           "            4 - 1100uS\n"
           "            5 - 2116uS\n"
           "            6 - 4156uS\n"
           "            7 - 8244uS\n"
           "samples : Number of samples per conversion for averaging\n"
           "            0 - 1\n"
           "            1 - 4\n"
           "            2 - 16\n"
           "            3 - 64\n"
           "            4 - 128\n"
           "            5 - 256\n"
           "            6 - 512\n"
           "            7 - 1024\n");
}

static int _pwrmon_init(void)
{
    if (miot_pwrmon_initialized()) {
        printf("Power monitor is already initialized\n");
        return 1;
    }

    int ret = miot_pwrmon_init();
    if (ret != 0) {
        printf("Failed to initialize power monitor: %d\n", ret);
        return 1;
    }

    return 0;
}

static const char *op_mode_lut[] = {
    "CURRENT",
    "VOLTAGE",
    "BOTH",
};

/**
 * Map string representation of op mode to enum value.
 *
 * @param str The string representation.
 * @param op_mode [out] Gets set to the corresponding enum value.
 * @return 0 on success, -1 otherwise;
 */
static int _pwrmon_map_op_mode(char *str, enum miot_pwrmon_op_mode *op_mode)
{
    for (size_t i = 0; i < ARRAY_SIZE(op_mode_lut); i++) {
        if (strcmp(str, op_mode_lut[i]) == 0) {
            *op_mode = i;
            return 0;
        }
    }

    return -1;
}

static ina3221_conv_time_shunt_adc_t sadc_lut[] = {
    INA3221_CONV_TIME_SADC_140US,  INA3221_CONV_TIME_SADC_204US,  INA3221_CONV_TIME_SADC_332US,
    INA3221_CONV_TIME_SADC_588US,  INA3221_CONV_TIME_SADC_1100US, INA3221_CONV_TIME_SADC_2116US,
    INA3221_CONV_TIME_SADC_4156US, INA3221_CONV_TIME_SADC_8244US,
};

/**
 * Map the given sadc value to enum value.
 *
 * @param val The given sadc value.
 * @param sadc [out] Get set to the corresponding enum value.
 * @return 0 on success, -1 otherwise.
 */
static int _pwrmon_map_sadc(unsigned int val, ina3221_conv_time_shunt_adc_t *sadc)
{
    if (val >= ARRAY_SIZE(sadc_lut)) {
        return -1;
    }

    *sadc = sadc_lut[val];

    return 0;
}

static ina3221_conv_time_bus_adc_t badc_lut[] = {
    INA3221_CONV_TIME_BADC_140US,  INA3221_CONV_TIME_BADC_204US,  INA3221_CONV_TIME_BADC_332US,
    INA3221_CONV_TIME_BADC_588US,  INA3221_CONV_TIME_BADC_1100US, INA3221_CONV_TIME_BADC_2116US,
    INA3221_CONV_TIME_BADC_4156US, INA3221_CONV_TIME_BADC_8244US,
};

/**
 * Map the given badc value to enum value.
 *
 * @param val The given badc value.
 * @param badc [out] Get set to the corresponding enum value.
 * @return 0 on success, -1 otherwise.
 */
static int _pwrmon_map_badc(unsigned int val, ina3221_conv_time_bus_adc_t *badc)
{
    if (val >= ARRAY_SIZE(badc_lut)) {
        return -1;
    }

    *badc = badc_lut[val];

    return 0;
}

static ina3221_num_samples_t samples_lut[] = {
    INA3221_NUM_SAMPLES_1,   INA3221_NUM_SAMPLES_4,    INA3221_NUM_SAMPLES_16,
    INA3221_NUM_SAMPLES_64,  INA3221_NUM_SAMPLES_128,  INA3221_NUM_SAMPLES_256,
    INA3221_NUM_SAMPLES_512, INA3221_NUM_SAMPLES_1024,
};

/**
 * Map the given sample value to enum value.
 *
 * @param val The given sample value.
 * @param samples [out] Get set to the corresponding enum value.
 * @return 0 on success, -1 otherwise.
 */
static int _pwrmon_map_samples(unsigned int val, ina3221_num_samples_t *samples)
{
    if (val >= ARRAY_SIZE(samples_lut)) {
        return -1;
    }

    *samples = samples_lut[val];

    return 0;
}

static ztimer_now_t meas_start_ts = 0;

static void _pwrmon_cb(uint8_t channels, int32_t *current_ua, int16_t *bus_mv)
{
    ztimer_now_t ts = ztimer_now(ZTIMER_MSEC);

    for (uint8_t i = 0; i < 6; i++) {
        if ((channels & (1 << i)) == 0)
            continue;

        printf("%u;%lu;", i, ts - meas_start_ts);
        if (bus_mv) {
            printf("%" PRId16 ";", bus_mv[i]);
        }
        else {
            printf(";");
        }

        if (current_ua) {
            printf("%" PRId32 "", current_ua[i]);
        }
        printf("\r\n");
    }
}

static uint16_t sadc_bads_lut[] = {
    140, 204, 332, 588, 1100, 2116, 4156, 8244,
};

static uint16_t nsamples_lut[] = {
    1, 4, 16, 64, 128, 256, 512, 1024,
};

static int _pwrmon_measure(int argc, char **argv)
{
    if (!miot_pwrmon_initialized()) {
        printf("Power monitor is not initialized\n");
        return 1;
    }

    if (argc != 7) {
        _measure_usage();
        return 1;
    }

    struct pwrmon_cfg cfg;

    uint32_t chan_map = (uint32_t)strtoul(argv[2], NULL, 16);
    if ((chan_map & ~MIOT_PWRMON_CHANNEL_MASK) != 0) {
        printf("Invalid channel mask: %s\n", argv[2]);
        return 1;
    }

    cfg.channels = (uint8_t)chan_map;

    int ret = _pwrmon_map_op_mode(argv[3], &cfg.op_mode);
    if (ret != 0) {
        printf("Invalid op mode: %s\n", argv[3]);
        return 1;
    }

    unsigned int sadc = strtoul(argv[4], NULL, 10);
    ret = _pwrmon_map_sadc(sadc, &cfg.sadc);
    if (ret != 0) {
        printf("Invalid sadc value: %d\n", sadc);
        return 1;
    }

    unsigned int badc = strtoul(argv[5], NULL, 10);
    ret = _pwrmon_map_badc(badc, &cfg.badc);
    if (ret != 0) {
        printf("Invalid badc value: %d\n", badc);
        return 1;
    }

    unsigned int samples = strtoul(argv[6], NULL, 10);
    ret = _pwrmon_map_samples(samples, &cfg.num_samples);
    if (ret != 0) {
        printf("Invalid samples value: %d\n", samples);
        return 1;
    }

    meas_start_ts = ztimer_now(ZTIMER_MSEC);

    ret = miot_pwrmon_start_meas(&cfg, _pwrmon_cb);
    if (ret != 0) {
        printf("Failed to start measurement: %d\n", ret);
        return 1;
    }

    printf("\r\n");
    printf("# Channels: 0x%02lX\r\n", chan_map);
    printf("# Op mode : %s\r\n", argv[3]);
    printf("# Sadc    : %u us\r\n", sadc_bads_lut[sadc]);
    printf("# Badc    : %u us\r\n", sadc_bads_lut[badc]);
    printf("# Samples : %u\r\n", nsamples_lut[samples]);
    printf("# Version : %s\r\n", MIOT_PWRMON_VERSION_STRING);
    printf("channels;timestamp;voltage_mv;current_ua\r\n");

    return 0;
}

static int _pwrmon_stop(void)
{
    if (!miot_pwrmon_initialized()) {
        printf("Power monitor is not initialized\n");
        return 1;
    }

    miot_pwrmon_stop_meas();

    return 0;
}

static int _pwrmon_cmd(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "version") == 0) {
        printf("%s\r\n", MIOT_PWRMON_VERSION_STRING);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "init") == 0) {
        return _pwrmon_init();
    }
    if (argc >= 2 && strcmp(argv[1], "measure") == 0) {
        return _pwrmon_measure(argc, argv);
    }

    if (argc == 2 && strcmp(argv[1], "stop") == 0) {
        return _pwrmon_stop();
    }

    _usage();
    return 1;
}

SHELL_COMMAND(pwrmon, "MIOT-Lab Power Monitor", _pwrmon_cmd);
