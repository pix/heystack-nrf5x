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

### Available make targets

- `nrf51822/armgcc`: `nrf51822_xxac` `nrf51822_xxac-dcdc`
- `nrf52810/armgcc`: `nrf52810_xxaa` `nrf52810_xxaa-dcdc`
- `nrf52832/armgcc`: `nrf52832_xxaa` `nrf52832_xxaa-dcdc` `nrf52832_yj17024`

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
make stflash-nrf51822_xxac-patched ADV_KEYS_FILE=./50_NRF_keyfile
```

```bash
```

To compile the firmware for the nRF52832 with the YJ-17024 board configuration, use the following command:

```bash
cd nrf52832/armgcc
make clean
make stflash-nrf52832_yj17024-patched ADV_KEYS_FILE=./50_NRF_keyfile
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
- **HAS_BATTERY**: Enables battery level reporting; set to `1` to enable or `0` to disable (default);
- **HAS_DCDC**: Enables DCDC mode; set to `1` to enable or `0` to for automatic selection (default);
- **KEY_ROTATION_INTERVAL**: Sets the key rotation interval in seconds (default is 3600 * 3 seconds);
- **ADVERTISING_INTERVAL**: Adjusts Bluetooth advertising interval; `0` (default) uses the standard interval (1000ms, down to 20ms);
- **BOARD**: Specifies the custom board configuration; defaults to `custom_board` (see `custom_board.h`), but can be overridden with your board's configuration. For example, set `BOARD=yj17024` for the nRF52832 device.
- **ADV_KEYS_FILE**: Specifies the file containing the keys to be flashed to the device.
- **GNU_INSTALL_ROOT**: Path to the GNU toolchain; eg: ../../nrf-sdk/gcc-arm-none-eabi-6-2017-q2-update/bin/

### Debugging with strtt

The firmware supports using strtt for displaying debug logs. To enable this feature, compile the firmware with `HAS_DEBUG=1`:

```bash
cd nrf51822/armgcc
make clean
make stflash-nrf51822_xxac-patched MAX_KEYS=500 HAS_DEBUG=1 ADV_KEYS_FILE=./50_NRF_keyfile
```

This will activate debug logging, which can be viewed using `strtt`.

### Using Black Magic Probe

The firmware can also be flashed using a Black Magic Probe. The programmer should be connected to the SWD pins on the device. The following command can be used to flash the firmware:

```bash
cd nrf52832/armgcc
make clean
make bmpflash-nrf52832_yj17024-patched ADV_KEYS_FILE=./50_NRF_keyfileZ
```

### Using RTT monitor

You can use the RTT monitor to see the debug logs. The following command can be used to monitor the logs:

```bash
make bmpflash-monitor
  BMP /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v1.10.0-1151-g3fe0bc5a-XXXXXXXX-if00 (monitor)
  minicom -c on -D /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v1.10.0-1151-g3fe0bc5a-XXXXXXXX-if02
Target voltage: 3.35V
....
```

In another terminal, you can monitor the logs:

```bash
minicom -c on -D /dev/serial/by-id/usb-Black_Magic_Debug_Black_Magic_Probe__ST-Link_v2__v1.10.0-1151-g3fe0bc5a-XXXXXXXX-if02
<info> app: last_filled_index: 249
<info> app: Starting advertising
<info> app: ble_set_mac_address: D3:7F:6F:DA:64:78
<info> app: ble_set_max_tx_power: 8 dB failed
<info> app: ble_set_max_tx_power: 7 dBm failed
<info> app: ble_set_max_tx_power: 6 dBm failed
<info> app: ble_set_max_tx_power: 5 dBm failed
<info> app: ble_set_max_tx_power: 4 dBm
<info> app: Rotating key: 59
<info> app: last_filled_index: 249
[0.000] <info> app: Starting advertising
[0.000] <info> app: ble_set_mac_address: XX:XX:XX:XX:XX:XX
```
