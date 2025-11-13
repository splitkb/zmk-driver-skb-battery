# Splitkb.com ZMK battery driver

Currently, ZMK is hardcoded to use a Lithium-Ion voltage-to-percentage curve. This module adds support for common 3V coincell batteries and a better Lipo implementation for our Halcyon boards. 

This can then be used in place of the `compatible = "zmk,battery-voltage-divider"` as `compatible = "splitkb,coincell-voltage-divider"` or `compatible = "splitkb,lipo-voltage-divider"`. Other options from the voltage-divider still apply here.

Furthermore this adds a driver for enabling a charging LED based on the output of the `STAT` pin of a charging IC, where the `STAT` pin would blink when no battery is connected. This can be added to your board as shown here:
```
/ {
    charging_led_controller {
        compatible = "splitkb,charging-ic-led";
        status = "okay";
        stat-gpios = <&gpio1 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
        led-gpios = <&gpio0 31 GPIO_ACTIVE_HIGH>;
    };
};
```
