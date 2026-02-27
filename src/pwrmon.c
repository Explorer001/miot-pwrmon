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

#include "ina3221_params.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "event.h"
#include "ztimer.h"
#include "thread.h"

#define INA3221_NUMOF 2

/** Power monitor contexts */
static struct pwrmon {
    /** INA ADC device descriptor */
    ina3221_t dev;
    /** Timer for measurement */
    ztimer_t timer;
    /** Event for timer interrupt escape */
    event_t event;
} miot_pwrmon[INA3221_NUMOF];

/** Power monitor thread components */
kernel_pid_t pwrmon_pid = KERNEL_PID_UNDEF;
static char pwrmon_stack[2048];

/** Event queue used by the power monitor thread */
static event_queue_t pwrmon_queue;

static void *pwrmon_thread(void *arg)
{
    event_queue_init(&pwrmon_queue);

    event_t *event;
    while ((event = event_wait(&pwrmon_queue))) {
        DEBUG("Got event");
    }

    return NULL;
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

    pwrmon_pid =
        thread_create(pwrmon_stack, sizeof(pwrmon_stack), 7, 0, pwrmon_thread, NULL, "pwrmon");

    return 0;
}
