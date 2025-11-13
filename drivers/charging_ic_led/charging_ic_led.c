/*
 * Copyright (c) 2026 Splitkb.com
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT splitkb_charging_ic_led

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/usb.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_REGISTER(charging_led, CONFIG_ZMK_LOG_LEVEL);

#define BLINK_FILTER_MS 600

struct chg_config {
    struct gpio_dt_spec stat;
    struct gpio_dt_spec led;
};

struct chg_data {
    struct gpio_callback stat_cb;
    struct k_work_delayable filter_work;
    const struct device *dev;
    bool usb_active;
};

static void filter_work_handler(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct chg_data *data = CONTAINER_OF(d_work, struct chg_data, filter_work);
    const struct chg_config *config = data->dev->config;

    if (!data->usb_active) return;

    gpio_pin_set_dt(&config->led, 1);
}

static void stat_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    struct chg_data *data = CONTAINER_OF(cb, struct chg_data, stat_cb);
    const struct chg_config *config = data->dev->config;

    if (!data->usb_active) return;

    int raw_val = gpio_pin_get(config->stat.port, config->stat.pin);

    if (raw_val == 0) {
        k_work_reschedule(&data->filter_work, K_MSEC(BLINK_FILTER_MS));
    } else {
        k_work_cancel_delayable(&data->filter_work);
        gpio_pin_set_dt(&config->led, 0);
    }
}

static void update_power_state(const struct device *dev) {
    struct chg_data *data = dev->data;
    const struct chg_config *config = dev->config;
    
    bool is_powered = zmk_usb_is_powered();

    if (is_powered && !data->usb_active) {
        /* --- ENTERING USB MODE --- */
        data->usb_active = true;
        LOG_INF("USB Connected: Enabling Interrupts");

        // 1. Configure STAT: Input + Pull Up
        gpio_pin_configure(config->stat.port, config->stat.pin, GPIO_INPUT | GPIO_PULL_UP);
        
        // 2. Enable Interrupts
        gpio_pin_interrupt_configure(config->stat.port, config->stat.pin, GPIO_INT_EDGE_BOTH);

        // 3. Manual check
        if (gpio_pin_get(config->stat.port, config->stat.pin) == 0) {
            k_work_reschedule(&data->filter_work, K_MSEC(BLINK_FILTER_MS));
        }

    } else if (!is_powered && data->usb_active) {
        /* --- ENTERING BATTERY MODE --- */
        data->usb_active = false;
        LOG_INF("USB Disconnected: Disabling Interrupts (0uA)");

        // 1. Disable Interrupts
        gpio_pin_interrupt_configure(config->stat.port, config->stat.pin, GPIO_INT_DISABLE);

        // 2. Disable Pull-Up
        gpio_pin_configure(config->stat.port, config->stat.pin, GPIO_DISCONNECTED);

        // 3. Cleanup
        k_work_cancel_delayable(&data->filter_work);
        gpio_pin_set_dt(&config->led, 0);
    }
}

static int usb_listener(const zmk_event_t *eh) {
    const struct device *dev = DEVICE_DT_GET_ANY(splitkb_charging_ic_led);
    if (dev && device_is_ready(dev)) {
        update_power_state(dev);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(charging_led_listener, usb_listener);
ZMK_SUBSCRIPTION(charging_led_listener, zmk_usb_conn_state_changed);

static int chg_init(const struct device *dev) {
    const struct chg_config *config = dev->config;
    struct chg_data *data = dev->data;

    data->dev = dev;

    if (!gpio_is_ready_dt(&config->led) || !gpio_is_ready_dt(&config->stat)) {
        LOG_ERR("GPIOs not ready");
        return -ENODEV;
    }

    // Default State: Battery Mode
    gpio_pin_configure_dt(&config->led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(config->stat.port, config->stat.pin, GPIO_DISCONNECTED);
    
    data->usb_active = false;

    // Setup Timer
    k_work_init_delayable(&data->filter_work, filter_work_handler);

    // Setup Callback
    gpio_init_callback(&data->stat_cb, stat_isr, BIT(config->stat.pin));
    gpio_add_callback(config->stat.port, &data->stat_cb);

    // Check state immediately
    update_power_state(dev);

    return 0;
}

#define DEFINE_CHG_LED(inst)                                        \
    static const struct chg_config config_##inst = {                \
        .stat = GPIO_DT_SPEC_INST_GET(inst, stat_gpios),            \
        .led = GPIO_DT_SPEC_INST_GET(inst, led_gpios),              \
    };                                                              \
    static struct chg_data data_##inst;                             \
    DEVICE_DT_INST_DEFINE(inst, chg_init, NULL,                     \
                          &data_##inst, &config_##inst,             \
                          POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_CHG_LED)