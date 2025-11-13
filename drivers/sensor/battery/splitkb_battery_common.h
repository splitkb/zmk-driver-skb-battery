/*
 * Copyright (c) 2026 Splitkb.com
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

uint8_t skb_lipo_mv_to_pct(int16_t bat_mv);
uint8_t coincell_mv_to_pct(int16_t bat_mv);
