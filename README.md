# FW-Challenge - BLE Battery Monitor

Solution to the FW-Challenge: A Bluetooth Low Energy application that advertises a custom 128-bit service with battery monitoring capabilities.

## Overview

This application demonstrates a BLE peripheral device built using Nordic Connectivity SDK (NCS) 3.1.1. It features:

- **Custom 128-bit BLE Service**: `12345678-1234-5678-1234-56789abcdef0`
- **Two Characteristics**:
  1. **Battery Voltage** (`12345678-1234-5678-1234-56789abcdef1`)
     - Read and Notify properties
     - Sends measured battery voltage in millivolts (uint16_t)
  2. **Sample Interval** (`12345678-1234-5678-1234-56789abcdef2`)
     - Read, Write, and Notify properties
     - Controls the sampling interval in milliseconds (uint32_t)
     - Valid range: 100ms to 60000ms (1 minute)

## Features

- BLE advertising with custom service UUID
- Configurable sample interval (default: 1000ms)
- Battery voltage simulation (3.0V - 4.2V range)
- Notification support for both characteristics
- Connection state logging
- Thread-based battery monitoring

## Building

This application requires the Nordic Connectivity SDK (NCS) 3.1.1 or compatible version.

### Prerequisites

- nRF Connect SDK 3.1.1
- Supported board (e.g., nrf52840dk_nrf52840, nrf5340dk_nrf5340_cpuapp, nrf52dk_nrf52832)

### Build Commands

```bash
# Using west build tool
west build -b nrf52840dk_nrf52840

# Flash to device
west flash

# View logs
west attach
```

## Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── prj.conf                # Kconfig configuration
├── sample.yaml             # Sample metadata
├── README.md               # This file
└── src/
    ├── main.c              # Main application
    ├── battery_service.c   # BLE service implementation
    └── battery_service.h   # BLE service header
```

## Service Details

### Custom Battery Monitor Service

**UUID**: `12345678-1234-5678-1234-56789abcdef0`

This service provides battery monitoring functionality through two characteristics.

### Characteristics

#### 1. Battery Voltage Characteristic

- **UUID**: `12345678-1234-5678-1234-56789abcdef1`
- **Properties**: Read, Notify
- **Format**: uint16_t (2 bytes, little-endian)
- **Unit**: Millivolts (mV)
- **Description**: Current battery voltage measurement

#### 2. Sample Interval Characteristic

- **UUID**: `12345678-1234-5678-1234-56789abcdef2`
- **Properties**: Read, Write, Notify
- **Format**: uint32_t (4 bytes, little-endian)
- **Unit**: Milliseconds (ms)
- **Range**: 100 - 60000 ms
- **Default**: 1000 ms
- **Description**: Battery measurement sample interval

## Usage

1. **Build and flash** the application to your Nordic board
2. **Power on** the device - it will start advertising automatically
3. **Connect** using a BLE client (nRF Connect, LightBlue, etc.)
4. **Discover** the Battery Monitor Service and characteristics
5. **Enable notifications** on the Battery Voltage characteristic to receive updates
6. **Write** to the Sample Interval characteristic to change the sampling rate
7. **Monitor** the battery voltage values being sent at the configured interval

## Configuration

Key configuration options in `prj.conf`:

- `CONFIG_BT_DEVICE_NAME`: Device name (default: "Battery Monitor")
- `CONFIG_LOG_DEFAULT_LEVEL`: Logging level (3 = INFO)
- `CONFIG_ADC`: Enable ADC for battery measurements

## Testing with nRF Connect Mobile App

1. Open nRF Connect for Mobile
2. Scan for devices and look for "Battery Monitor"
3. Connect to the device
4. Expand the custom service (12345678-1234-5678-1234-56789abcdef0)
5. Enable notifications on Battery Voltage characteristic
6. Observe voltage updates at the configured interval
7. Write a new interval value (e.g., 2000 for 2 seconds) to Sample Interval characteristic

## Example Output

When the application runs, you'll see output like this:

```
*** Booting Zephyr OS build v3.1.1 ***
[00:00:00.000,000] <inf> main: Starting Bluetooth Low Energy Battery Monitor
[00:00:00.000,000] <inf> main: NCS version: 3.1.1
[00:00:00.010,000] <inf> main: Bluetooth initialized
[00:00:00.010,000] <inf> battery_service: Battery Monitor Service initialized
[00:00:00.010,000] <inf> battery_service: Service UUID: 12345678-1234-5678-1234-56789abcdef0
[00:00:00.010,000] <inf> battery_service: Voltage Char UUID: 12345678-1234-5678-1234-56789abcdef1
[00:00:00.010,000] <inf> battery_service: Interval Char UUID: 12345678-1234-5678-1234-56789abcdef2
[00:00:00.020,000] <inf> main: Advertising successfully started
[00:00:00.020,000] <inf> main: Device name: Battery Monitor
[00:00:00.020,000] <inf> battery_service: Battery monitoring thread started
[00:00:01.020,000] <inf> main: Battery voltage: 4200 mV (interval: 1000 ms)
[00:00:02.020,000] <inf> main: Battery voltage: 4190 mV (interval: 1000 ms)
[00:00:03.020,000] <inf> main: Battery voltage: 4180 mV (interval: 1000 ms)

# When a client connects:
[00:00:05.123,000] <inf> main: Connected: 12:34:56:78:9A:BC (random)
[00:00:06.456,000] <inf> battery_service: Battery voltage notifications enabled

# When client writes new interval:
[00:00:10.789,000] <inf> battery_service: Sample interval updated to: 2000 ms
[00:00:12.789,000] <inf> main: Battery voltage: 4160 mV (interval: 2000 ms)
```

## Development Notes

- The application uses a simulated battery voltage that cycles between 3.0V and 4.2V
- For production use, replace the simulation with actual ADC readings
- The battery monitoring runs in a separate thread with priority 7
- Notifications are automatically sent when values are updated

## License

SPDX-License-Identifier: Apache-2.0
