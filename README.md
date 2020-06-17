# SMPS Driver for ATTinys

A [switched-mode power supply](https://en.wikipedia.org/wiki/Switched-mode_power_supply) driver for ATTinyx5 MCUs

## Features

- Adjustable output voltage range
- Feedback
- UART info
- 31kHz PWM (for 8MHz MCUs)

## Build and Flash

Get the sources
```sh
git clone https://github.com/VipeOut23/smps_driver
cd smps_driver
# if you want to use uart info also run
git submodule update --init src/software_uart
```

**NOTE: You might want to create your own device file based on** ```attiny85.conf```

Configuring the build

``` sh
meson setup build --cross-file=attinyXX.conf \
    -Duart=enabled \
    -Duart_tx_pin=PB2 \
    -Dflash_port=/dev/ttyACM0 \
    -Dflash_programmer=stk500v1 \
    -Dflash_baud=19200
```

Optionally reconfigure with

``` sh
meson configure build ...
```

Compile rom.hex with
```sh
cd build
ninja rom.hex
```

Upload rom.hex with
```sh
ninja flash
```


## Example Setup

![smps attiny85 symbolic](smps_attiny85_symbolic.png)
