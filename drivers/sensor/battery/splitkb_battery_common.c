/*
 * Copyright (c) 2026 Splitkb.com
 *
 * SPDX-License-Identifier: MIT
 */

#include "splitkb_battery_common.h"

uint8_t skb_lipo_mv_to_pct(int16_t bat_mv) {
    if (bat_mv >= 4200) return 100;
    if (bat_mv <= 3300) return 0;

    int16_t voltages[] = {3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
    uint8_t percents[] = {0,    10,   20,   40,   60,   70,   80,   90,   100};
    int n = sizeof(voltages) / sizeof(voltages[0]);

    for (int i = 0; i < n - 1; i++) {
        if (bat_mv >= voltages[i] && bat_mv < voltages[i + 1]) {
            int16_t v0 = voltages[i];
            int16_t v1 = voltages[i + 1];
            uint8_t p0 = percents[i];
            uint8_t p1 = percents[i + 1];

            return p0 + (uint8_t)((p1 - p0) * (bat_mv - v0) / (v1 - v0));
        }
    }

    return 0;
}

uint8_t coincell_mv_to_pct(int16_t bat_mv) {
    if (bat_mv >= 3000) return 100;
    if (bat_mv >= 2980) return 90; 
    if (bat_mv >= 2960) return 80; 
    if (bat_mv >= 2940) return 70; 
    if (bat_mv >= 2920) return 60; 
    if (bat_mv >= 2900) return 50; 
    if (bat_mv >= 2880) return 40; 
    if (bat_mv >= 2830) return 30; 
    if (bat_mv >= 2730) return 20; 
    if (bat_mv >= 2600) return 10; 

    return 0;
}