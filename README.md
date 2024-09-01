# HeyStack-NRF5X - OpenHaystack Compatible Low Power Firmware

This repository contains an alternative OpenHaystack firmware. It is based on the SoftDevice from Nordic Semiconductor. This approach could potentially extend battery life, with some estimates suggesting up to three years on a CR2032 battery! (See [this comment](https://github.com/seemoo-lab/openhaystack/issues/57#issuecomment-841642356)).

It's based on [acalatrava's](https://raw.githubusercontent.com/acalatrava/openhaystack-firmware/main/README.md) firmware with fixes
and support for newer nRF5x devices and SDKs.

## Supported Devices

- **nRF52810**: Tested on an original Tile Tag.
- **nRF51822**: Tested on an aliexpress tag.
- **nRF52832**: Tested with the YJ-17024 board (see link below).

Other nRF devices might be supported, but untested.

These aliexpress tags should work with the nRF52810 firmware:

- [Holyiot NRF52810](https://s.click.aliexpress.com/e/_DdDyDp9)

These aliexpress tags works with the nRF51822 firmware:

- [1: NRF51822](https://s.click.aliexpress.com/e/_De2JHyL)
- [2: NRF51822](https://s.click.aliexpress.com/e/_DdkWkyJ)
- [3: NRF51822](https://s.click.aliexpress.com/e/_DBp4icn)

This AliExpress tag works with the nRF52832 firmware:

- [HolyIOT YJ-17024-NRF52832 Amplified Module](https://s.click.aliexpress.com/e/_DlpmE0n): [Manufacturer's documentation](http://www.holyiot.com/eacp_view.asp?id=299).
- [HolyIOT YJ-17095-NRF52832](https://s.click.aliexpress.com/e/_DCkw8LV)

These are affiliate links, so if you buy something using them, I get a small commission, and you help me to keep working on this project.

## Setup Instructions

Unzip the relevant Nordic SDK and a compiler and place it in the `nrf-sdk` folder:

```bash
gcc-arm-none-eabi-6-2017-q2-update/ # Migth work with newer versions
nRF5_SDK_12.3.0_d7731ad/
nRF5_SDK_15.3.0_59ac345/
```

### Compile the Firmware

```
make all # Compile all the supported devices and place them in the release folder
```

### Flash the Firmware

The device can be flashed using a STLink V2 programmer. The programmer should be connected to the SWD pins on the device. The following command can be used to flash the firmware:

```bash
cd nrf51822/armgcc
make clean
make stflash
```

To compile the firmware for the nRF52832 with the YJ-17024 board configuration, use the following command:

```bash
cd nrf52832/armgcc
make clean
make stflash BOARD=yj17024

You can directly flash the firmware with the keys using the following command:

```bash
# nrf52810 support 100 (and more) keys
# nrf51822 support 50 keys
cd nrf51822/armgcc
make clean
make stflash-patched MAX_KEYS=50 ADV_KEYS_FILE=./50_NRF_keyfile
```
### Flashing with Raspberry Pi

If you're using a Raspberry Pi for flashing instead of a STLink V2 programmer, you can change the OpenOCD configuration file. Toggle between the configuration for the STLink V2 and Raspberry Pi by modifying the OpenOCD script.

Locate the configuration line in your `openocd.cfg` file:

```bash
source [find interface/stlink.cfg]
```

To use a Raspberry Pi for flashing, comment out the STLink line and uncomment the Raspberry Pi configuration line:

```bash
# source [find interface/stlink.cfg]
source [find interface/raspberrypi2-native.cfg]
```

This change allows you to use the Raspberry Pi GPIO pins for flashing your device instead of the STLink programmer.

### Makefile Variables Summary

This section describes key Makefile variables you can adjust to customize the firmware:

- **HAS_DEBUG**: Controls debug logging; set to `1` to enable or `0` to disable (default).
- **MAX_KEYS**: Defines the maximum number of keys supported;
- **KEY_ROTATION_INTERVAL**: Sets the key rotation interval in seconds (default is 3600 * 3 seconds);
- **ADVERTISING_INTERVAL**: Adjusts Bluetooth advertising interval; `0` (default) uses the standard interval (1000ms);
- **BOARD**: Specifies the custom board configuration; defaults to `custom_board` (see `custom_board.h`), but can be overridden with your board's configuration. For example, set `BOARD=yj17024` for the nRF52832 device.
- **ADV_KEYS_FILE**: Specifies the file containing the keys to be flashed to the device.
- **GNU_INSTALL_ROOT**: Path to the GNU toolchain; eg: ../../nrf-sdk/gcc-arm-none-eabi-6-2017-q2-update/bin/

### Debugging with strtt

The firmware supports using strtt for displaying debug logs. To enable this feature, compile the firmware with `HAS_DEBUG=1`:

```bash
cd nrf51822/armgcc
make clean
make stflash-patched MAX_KEYS=50 HAS_DEBUG=1 ADV_KEYS_FILE=./50_NRF_keyfile
```

This will activate debug logging, which can be viewed using `strtt`.
