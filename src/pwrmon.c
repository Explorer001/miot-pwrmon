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

#define INA3221_NUMOF 2

struct pwrmon {
    ina3221_t dev;
} miot_pwrmon[INA3221_NUMOF];

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

    return 0;
}
