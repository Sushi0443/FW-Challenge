# BLE Battery Monitor - Implementation Details

## Architecture

This BLE application follows the Nordic Connectivity SDK (NCS) 3.1.1 architecture and best practices.

### Components

1. **Main Application (`src/main.c`)**
   - Initializes Bluetooth subsystem
   - Manages BLE advertising
   - Handles connection/disconnection events
   - Runs battery monitoring thread

2. **Battery Service (`src/battery_service.c/h`)**
   - Implements custom 128-bit GATT service
   - Manages two characteristics (voltage and interval)
   - Handles read/write/notify operations
   - Provides API for updating characteristic values

### BLE Service Architecture

```
Battery Monitor Service (12345678-1234-5678-1234-56789abcdef0)
├── Battery Voltage Characteristic (12345678-1234-5678-1234-56789abcdef1)
│   ├── Properties: Read, Notify
│   ├── Value: uint16_t (2 bytes)
│   └── CCC Descriptor (for notifications)
└── Sample Interval Characteristic (12345678-1234-5678-1234-56789abcdef2)
    ├── Properties: Read, Write, Notify
    ├── Value: uint32_t (4 bytes)
    └── CCC Descriptor (for notifications)
```

## UUID Design

The custom 128-bit UUIDs follow a consistent pattern:

- **Base UUID**: `12345678-1234-5678-1234-56789abcdef0`
- **Voltage UUID**: Base + 1 → `...def1`
- **Interval UUID**: Base + 2 → `...def2`

This makes the UUIDs easy to remember and manage.

## Data Flow

1. **Initialization**
   - Bluetooth subsystem initialized
   - Custom service registered with GATT database
   - Advertising started with custom UUID

2. **Connection**
   - Client discovers service and characteristics
   - Client enables notifications (optional)

3. **Battery Monitoring Loop**
   - Thread wakes up at configured interval
   - Simulates battery voltage reading
   - Updates characteristic value
   - Sends notification to connected clients

4. **Interval Update**
   - Client writes new interval value
   - Service validates range (100-60000ms)
   - Updates internal state
   - Battery thread uses new interval

## Configuration Options

### Bluetooth Configuration (`prj.conf`)

- **CONFIG_BT=y**: Enable Bluetooth
- **CONFIG_BT_PERIPHERAL=y**: Enable peripheral role
- **CONFIG_BT_GATT_DYNAMIC_DB=y**: Enable dynamic GATT database
- **CONFIG_BT_SETTINGS=y**: Enable BT settings for bonding

### Build Configuration (`CMakeLists.txt`)

- Links main application and service implementation
- Uses Zephyr build system

### Logging

- Module-based logging for main and service
- Configurable log levels via Kconfig
- Default: INFO level

## Thread Model

The application uses Zephyr's threading:

- **Main Thread**: Initializes system and starts advertising
- **Battery Monitor Thread**: Periodic battery voltage updates
  - Priority: 7 (cooperative scheduling)
  - Stack size: 1024 bytes
  - Sleeps between measurements

## Memory Layout

### GATT Database

- Service declaration
- 2 characteristics with CCC descriptors
- Total: ~7 GATT attributes

### RAM Usage

- Battery voltage: 2 bytes
- Sample interval: 4 bytes
- Thread stack: 1024 bytes
- Bluetooth stack: ~8-10KB (depending on configuration)

## Testing Procedure

### Unit Testing (Manual)

1. **Advertisement Test**
   - Scan for device
   - Verify "Battery Monitor" appears
   - Verify custom UUID in advertising data

2. **Service Discovery Test**
   - Connect to device
   - Discover services
   - Verify custom service UUID
   - Verify two characteristics present

3. **Read Test**
   - Read voltage characteristic
   - Read interval characteristic
   - Verify correct data types and values

4. **Write Test**
   - Write valid interval (e.g., 2000ms)
   - Verify write succeeds
   - Write invalid interval (e.g., 50ms)
   - Verify write fails with error

5. **Notification Test**
   - Enable notifications on voltage
   - Verify periodic updates received
   - Change interval
   - Verify update frequency changes

## Production Considerations

### Real Battery Measurement

Replace simulation in `src/main.c` with actual ADC reading:

```c
#include <zephyr/drivers/adc.h>

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

static uint16_t read_battery_voltage(void)
{
    int16_t sample_buffer;
    struct adc_sequence sequence = {
        .buffer = &sample_buffer,
        .buffer_size = sizeof(sample_buffer),
    };
    
    adc_read(adc_dev, &sequence);
    
    // Convert ADC value to millivolts
    return (uint16_t)(sample_buffer * 3600 / 4096);
}
```

### Power Optimization

- Use low-power advertising intervals
- Enable connection parameter update
- Use SYSTEM_OFF between measurements for ultra-low power
- Optimize advertising data size

### Security

- Enable bonding with CONFIG_BT_BONDABLE=y
- Add authentication requirements to characteristics
- Enable privacy features

### Error Handling

- Add error recovery for Bluetooth stack errors
- Implement watchdog for thread monitoring
- Add diagnostics logging

## Compliance

- Bluetooth Core Specification 5.x compliant
- Uses standard GATT procedures
- Compatible with all BLE central devices
- No proprietary extensions

## Known Limitations

1. Simulated battery readings (not production-ready)
2. Single connection support
3. No persistent storage of interval value
4. No security/encryption implemented
5. Basic error handling

## Future Enhancements

- [ ] Add real ADC battery measurement
- [ ] Support multiple simultaneous connections
- [ ] Persistent storage for configuration
- [ ] Battery level service (standard)
- [ ] Device Information Service
- [ ] Over-the-air (OTA) firmware updates
- [ ] Low battery alerts
- [ ] Calibration support
