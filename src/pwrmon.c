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

#include <errno.h>

#include "ina3221_params.h"

#include "pwrmon.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "event.h"
#include "ztimer.h"
#include "thread.h"

#define INA3221_NUMOF           2

#define CHANNELS_MON0_MASK      0x07
#define CHANNELS_MON0_SHIFT     0U
#define CHANNELS_MON1_MASK      0x38
#define CHANNELS_MON1_SHIFT     3U

#define SADC_LUT_DIVISOR        0x08
#define BADC_LUT_DIVISOR        0x40
#define NUM_SAMPLES_LUT_DIVISOR 0x200

/** Power monitor contexts */
static struct pwrmon {
    /** INA ADC device descriptor */
    ina3221_t dev;
    /** Timer for measurement */
    ztimer_t timer;
    /** Event for timer interrupt escape */
    event_t event;
    /** Channel shift for mapping to miot_pwrmon_channel  */
    uint8_t channel_shift;
    /** Measurement interval in microseconds */
    uint32_t interval_us;
} miot_pwrmon[INA3221_NUMOF];

/** Power monitor thread components */
kernel_pid_t pwrmon_pid = KERNEL_PID_UNDEF;
static char pwrmon_stack[2048];

/** Event queue used by the power monitor thread */
static event_queue_t pwrmon_queue;

/** Lookup table for sadc/badc conversion time in microseconds */
static uint32_t sadc_badc_lut_usec[] = {
    140, 204, 332, 588, 1100, 2116, 4156, 8244,
};

/** Lookup table for number of samples */
static uint32_t num_samples_lut[] = {
    1, 4, 16, 64, 128, 256, 512, 1024,
};

static void *pwrmon_thread(void *arg)
{
    event_queue_init(&pwrmon_queue);

    event_t *event;
    while ((event = event_wait(&pwrmon_queue))) {
        DEBUG("Got event");
    }

    return NULL;
}

static void _pwrmon_timer_cb(void *arg)
{
    struct pwrmon *pwrmon = arg;

    event_post(&pwrmon_queue, &pwrmon->event);
}

int miot_pwrmon_init(void)
{
    int ret = ina3221_init(&miot_pwrmon[0].dev, &ina3221_params[0]);
    if (ret != 0) {
        DEBUG("Failed to initialize ina3221 0\n");
        return ret;
    }

    ret = ina3221_init(&miot_pwrmon[1].dev, &ina3221_params[1]);
    if (ret != 0) {
        DEBUG("Failed to initialize ina3221 0\n");
        return ret;
    }

    miot_pwrmon[0].channel_shift = CHANNELS_MON0_SHIFT;
    miot_pwrmon[0].timer.arg = &miot_pwrmon[0];
    miot_pwrmon[0].timer.callback = _pwrmon_timer_cb;

    miot_pwrmon[1].channel_shift = CHANNELS_MON1_SHIFT;
    miot_pwrmon[1].timer.arg = &miot_pwrmon[1];
    miot_pwrmon[1].timer.callback = _pwrmon_timer_cb;

    pwrmon_pid =
        thread_create(pwrmon_stack, sizeof(pwrmon_stack), 7, 0, pwrmon_thread, NULL, "pwrmon");

    return 0;
}

/**
 * Check if the sadc value is valid.
 *
 * @param sadc The sadc value.
 * @return @c true if valid, @c false otherwise. 
 */
static bool _sadc_valid(ina3221_conv_time_shunt_adc_t sadc)
{
    switch (sadc) {
    case INA3221_CONV_TIME_SADC_140US:
    case INA3221_CONV_TIME_SADC_204US:
    case INA3221_CONV_TIME_SADC_332US:
    case INA3221_CONV_TIME_SADC_588US:
    case INA3221_CONV_TIME_SADC_1100US:
    case INA3221_CONV_TIME_SADC_2116US:
    case INA3221_CONV_TIME_SADC_4156US:
    case INA3221_CONV_TIME_SADC_8244US:
        return true;
    default:
        return false;
    }
}

/**
 * Check if the badc value is valid.
 *
 * @param badc The badc value.
 * @return @c true if valid, @c false otherwise. 
 */
static bool _badc_valid(ina3221_conv_time_bus_adc_t badc)
{
    switch (badc) {
    case INA3221_CONV_TIME_BADC_140US:
    case INA3221_CONV_TIME_BADC_204US:
    case INA3221_CONV_TIME_BADC_332US:
    case INA3221_CONV_TIME_BADC_588US:
    case INA3221_CONV_TIME_BADC_1100US:
    case INA3221_CONV_TIME_BADC_2116US:
    case INA3221_CONV_TIME_BADC_4156US:
    case INA3221_CONV_TIME_BADC_8244US:
        return true;
    default:
        return false;
    }
}

/**
 * Check if the number of samples is valid.
 *
 * @param sadc The snumber of samples.
 * @return @c true if valid, @c false otherwise. 
 */
static bool _num_samples_valid(ina3221_num_samples_t num_samples)
{
    switch (num_samples) {
    case INA3221_NUM_SAMPLES_1:
    case INA3221_NUM_SAMPLES_4:
    case INA3221_NUM_SAMPLES_16:
    case INA3221_NUM_SAMPLES_64:
    case INA3221_NUM_SAMPLES_128:
    case INA3221_NUM_SAMPLES_256:
    case INA3221_NUM_SAMPLES_512:
    case INA3221_NUM_SAMPLES_1024:
        return true;
    default:
        return false;
    }
}

/**
 * Build a configuration for the ina3221.
 *
 * @param dst Pointer to the configuration to write.
 * @param channels Channels to enable.
 * @param cfg The provided config. 
 */
static void _build_cfg(uint16_t *dst, uint8_t channels, const struct pwrmon_cfg *cfg)
{
    ina3221_mode_t mode;

    switch (cfg->op_mode) {
    case MIOT_PWRMON_OP_MODE_CURRENT:
        mode = INA3221_MODE_CONTINUOUS_SHUNT_ONLY;
        break;
    case MIOT_PWRMON_OP_MODE_VOLTAGE:
        mode = INA3221_MODE_CONTINUOUS_BUS_ONLY;
        break;
    case MIOT_PWRMON_OP_MODE_BOTH:
        mode = INA3221_MODE_CONTINUOUS_SHUNT_BUS;
        break;
    default:
        mode = INA3221_MODE_POWER_DOWN;
        break;
    }

    *dst = 0;
    ina3221_config_set_enabled_channels(dst, channels);
    ina3221_config_set_mode(dst, mode);
    ina3221_config_set_conv_time_shunt(dst, cfg->sadc);
    ina3221_config_set_conv_time_bus(dst, cfg->badc);
    ina3221_config_set_num_samples(dst, cfg->num_samples);
}

/**
 * Get the update interval from a configuration value.
 *
 * @param cfg The configuration.
 * @return The update interval in microseconds.
 */
static uint32_t _get_measurement_interval(uint16_t cfg)
{
    uint32_t interval = 0;

    ina3221_mode_t mode = ina3221_config_get_mode(cfg);
    if (mode == INA3221_MODE_POWER_DOWN || mode == INA3221_MODE_POWER_DOWN_) {
        return interval;
    }

    ina3221_conv_time_shunt_adc_t sadc = ina3221_config_get_conv_time_shunt(cfg);
    ina3221_conv_time_bus_adc_t badc = ina3221_config_get_conv_time_bus(cfg);

    if (mode == INA3221_MODE_TRIGGER_SHUNT_ONLY || mode == INA3221_MODE_CONTINUOUS_SHUNT_ONLY) {
        interval = sadc_badc_lut_usec[sadc / SADC_LUT_DIVISOR];
    }
    else if (mode == SADC_LUT_DIVISOR || mode == INA3221_MODE_CONTINUOUS_BUS_ONLY) {
        interval = sadc_badc_lut_usec[badc / BADC_LUT_DIVISOR];
    }
    else {
        interval = sadc_badc_lut_usec[sadc / SADC_LUT_DIVISOR] +
                   sadc_badc_lut_usec[badc / BADC_LUT_DIVISOR];
    }

    ina3221_num_samples_t samples = ina3221_config_get_num_samples(cfg);

    ina3221_ch_t channels = ina3221_config_get_enabled_channels(cfg);

    uint32_t num_channels = ((channels & (INA3221_CH1)) ? 1 : 0) +
                            ((channels & (INA3221_CH2)) ? 1 : 0) +
                            ((channels & (INA3221_CH3)) ? 1 : 0);

    return num_channels * interval * num_samples_lut[samples / NUM_SAMPLES_LUT_DIVISOR];
}

static int _configure(struct pwrmon *mon, uint16_t cfg)
{
    uint32_t interval_us = _get_measurement_interval(cfg);
    if (interval_us == 0) {
        DEBUG("Interval is 0. No channels enabled?");
        return -EINVAL;
    }

    mon->interval_us = interval_us;

    int ret = ina3221_set_config(&mon->dev, cfg);
    if (ret != 0) {
        DEBUG("Failed to set config: %d\n", ret);
        return ret;
    }

    (void)ztimer_set(ZTIMER_USEC, &mon->timer, interval_us);

    return 0;
}

/**
 * Resets the power monitors. 
 */
static void _reset_monitors(void)
{
    (void)ina3221_reset(&miot_pwrmon[0].dev);
    (void)ina3221_reset(&miot_pwrmon[1].dev);
}

int miot_pwrmon_start_meas(const struct pwrmon_cfg *cfg)
{
    if ((cfg->channels & MIOT_PWRMON_CHANNEL_MASK) != 0) {
        DEBUG("Invalid channel mask: 0x%02X\n", cfg->channels);
        return -EINVAL;
    }

    if (cfg->channels == 0) {
        DEBUG("No channels enabled\n");
        return 0;
    }

    if (cfg->op_mode > MIOT_PWRMON_OP_MODE_BOTH) {
        DEBUG("Invalid op mode: %u\n", cfg->op_mode);
        return -EINVAL;
    }

    if (!_sadc_valid(cfg->sadc)) {
        DEBUG("Invalid shunt conversion time: %u\n", cfg->sadc);
        return -EINVAL;
    }

    if (!_badc_valid(cfg->badc)) {
        DEBUG("Invalid bus conversion time: %u\n", cfg->badc);
        return -EINVAL;
    }

    if (!_num_samples_valid(cfg->num_samples)) {
        DEBUG("Invalid number of samples: %u\n", cfg->num_samples);
        return -EINVAL;
    }

    uint8_t channels0 = cfg->channels & CHANNELS_MON0_MASK;
    uint8_t channels1 = (cfg->channels & CHANNELS_MON1_MASK) >> CHANNELS_MON1_SHIFT;

    uint16_t cfg_val;
    int ret;

    if (channels0 != 0) {
        _build_cfg(&cfg_val, channels0, cfg);
        ret = _configure(&miot_pwrmon[0], cfg_val);
        if (ret != 0) {
            DEBUG("Failed to set configuration for monitor 0\n");
            _reset_monitors();
            return ret;
        }
    }
    if (channels1 != 0) {
        _build_cfg(&cfg_val, channels1, cfg);
        ret = _configure(&miot_pwrmon[1], cfg_val);
        if (ret != 0) {
            DEBUG("Failed to set configuration for monitor 1\n");
            _reset_monitors();
            return ret;
        }
    }

    return 0;
}
