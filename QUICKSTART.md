# Quick Start Guide

## Prerequisites

1. **nRF Connect SDK 3.1.1** installed
2. **Supported Nordic board** (e.g., nrf52840dk_nrf52840)
3. **nRF Command Line Tools** installed
4. **Terminal emulator** (minicom, screen, or nRF Terminal)

## Build and Flash

### Step 1: Navigate to Project Directory

```bash
cd FW-Challenge
```

### Step 2: Build the Application

```bash
# For nRF52840 DK
west build -b nrf52840dk_nrf52840

# For nRF5340 DK (app core)
west build -b nrf5340dk_nrf5340_cpuapp

# For nRF52 DK
west build -b nrf52dk_nrf52832
```

### Step 3: Flash to Device

```bash
west flash
```

### Step 4: View Logs

```bash
# Using west
west attach

# Or using minicom (adjust device path)
minicom -D /dev/ttyACM0 -b 115200
```

## Expected Output

When the application starts, you should see:

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
```

## Connect with Mobile App

### Using nRF Connect for Mobile (Android/iOS)

1. **Open nRF Connect app**
2. **Tap "SCAN"**
3. **Find "Battery Monitor"** in the list
4. **Tap "CONNECT"**
5. **Wait for connection** to establish
6. **Tap "Unknown Service"** (12345678-1234-5678-1234-56789abcdef0)
7. **Explore characteristics:**
   - Battery Voltage (12345678-1234-5678-1234-56789abcdef1)
   - Sample Interval (12345678-1234-5678-1234-56789abcdef2)

### Enable Notifications

1. **Tap the download icon** (↓) next to Battery Voltage characteristic
2. **Select "Notify"**
3. **Observe voltage updates** appearing every second

### Change Sample Interval

1. **Tap the upload icon** (↑) next to Sample Interval characteristic
2. **Select "UINT32"** format
3. **Enter new value** (e.g., 2000 for 2 seconds)
4. **Tap "SEND"**
5. **Observe that updates now come every 2 seconds**

## Troubleshooting

### Build Errors

**Problem**: `west: command not found`
- **Solution**: Ensure nRF Connect SDK environment is activated:
  ```bash
  source ~/ncs/zephyr/zephyr-env.sh
  ```

**Problem**: `No such file or directory: 'CMakeLists.txt'`
- **Solution**: Ensure you're in the project root directory

**Problem**: Board not supported
- **Solution**: Check `sample.yaml` for supported boards, or try:
  ```bash
  west boards | grep nrf
  ```

### Flash Errors

**Problem**: `No connected boards`
- **Solution**: Check USB connection, install nRF Command Line Tools

**Problem**: Permission denied on `/dev/ttyACM0`
- **Solution**: Add user to dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  # Log out and back in
  ```

### Runtime Issues

**Problem**: No advertising visible
- **Solution**: Check logs for errors, verify Bluetooth is enabled

**Problem**: Cannot connect
- **Solution**: 
  - Reset board
  - Clear bonds in mobile app
  - Check logs for connection errors

**Problem**: Characteristics not visible
- **Solution**: Refresh services in mobile app (pull down on service list)

## Advanced Usage

### Change Device Name

Edit `prj.conf`:
```
CONFIG_BT_DEVICE_NAME="My Custom Name"
```

### Adjust Log Level

Edit `prj.conf`:
```
CONFIG_LOG_DEFAULT_LEVEL=4  # 4=DEBUG, 3=INFO, 2=WARN, 1=ERR
```

### Modify Sample Interval Range

Edit `src/battery_service.c`, function `write_interval()`:
```c
/* Validate interval (min 100ms, max 60s) */
if (new_interval < 100 || new_interval > 60000) {
```

### Use Real ADC Measurement

Replace simulation in `src/main.c`:
```c
static uint16_t simulate_battery_voltage(void)
{
    // TODO: Replace with actual ADC reading
    // See IMPLEMENTATION.md for example code
}
```

## Testing Checklist

- [ ] Build completes without errors
- [ ] Flash succeeds
- [ ] Device advertises as "Battery Monitor"
- [ ] Can connect from mobile app
- [ ] Custom service appears with correct UUID
- [ ] Both characteristics are visible
- [ ] Can read voltage (returns value ~3000-4200 mV)
- [ ] Can read interval (returns 1000 by default)
- [ ] Can write interval (try 2000 ms)
- [ ] Notifications work for voltage
- [ ] Voltage changes over time (simulation)
- [ ] Changing interval affects update rate
- [ ] Invalid interval rejected (try 50 ms)
- [ ] Can disconnect and reconnect

## Next Steps

1. **Implement real battery measurement** - Replace simulation with ADC
2. **Add security** - Enable bonding and authentication
3. **Optimize power** - Use low-power modes
4. **Add more services** - Device Information Service, etc.
5. **Production hardening** - Error handling, recovery, etc.

## Resources

- [nRF Connect SDK Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Bluetooth Low Energy Basics](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/ug_ble.html)
- [GATT Services](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/connectivity/bluetooth/api/gatt.html)
- [nRF Connect for Mobile](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile)

## Support

For issues or questions:
1. Check logs for error messages
2. Review IMPLEMENTATION.md for technical details
3. Consult nRF Connect SDK documentation
4. Check Nordic DevZone forums
