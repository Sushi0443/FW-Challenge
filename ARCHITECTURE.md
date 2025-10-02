# BLE Service Architecture Diagram

## Service Hierarchy

```
┌─────────────────────────────────────────────────────────────────────┐
│                         BLE Battery Monitor                          │
│                     Device Name: "Battery Monitor"                   │
└─────────────────────────────────────────────────────────────────────┘
                                  │
                                  │
                    ┌─────────────▼──────────────┐
                    │    Advertising Packet      │
                    │  (Custom UUID included)    │
                    │  12345678-1234-5678-1234-  │
                    │      56789abcdef0          │
                    └────────────────────────────┘
                                  │
                                  │
        ┌─────────────────────────▼──────────────────────────┐
        │         Battery Monitor GATT Service               │
        │  UUID: 12345678-1234-5678-1234-56789abcdef0       │
        └─────────────────────────────────────────────────────┘
                    │                        │
         ┌──────────▼──────────┐  ┌─────────▼──────────┐
         │  Battery Voltage    │  │  Sample Interval   │
         │   Characteristic    │  │   Characteristic   │
         │                     │  │                    │
         │  UUID: ...def1      │  │  UUID: ...def2     │
         │                     │  │                    │
         │  Properties:        │  │  Properties:       │
         │  • Read             │  │  • Read            │
         │  • Notify           │  │  • Write           │
         │                     │  │  • Notify          │
         │  Type: uint16_t     │  │                    │
         │  Unit: mV           │  │  Type: uint32_t    │
         │  Range: 0-65535     │  │  Unit: ms          │
         │                     │  │  Range: 100-60000  │
         │  Default: 3300      │  │  Default: 1000     │
         └─────────┬───────────┘  └─────────┬──────────┘
                   │                        │
         ┌─────────▼──────────┐  ┌─────────▼──────────┐
         │  CCC Descriptor    │  │  CCC Descriptor    │
         │  (for Notify)      │  │  (for Notify)      │
         └────────────────────┘  └────────────────────┘
```

## Data Flow Diagram

```
┌─────────────┐              ┌──────────────┐              ┌─────────────┐
│   BLE       │              │  FW-Challenge│              │  Battery    │
│   Central   │              │  Application │              │  Monitor    │
│  (Mobile)   │              │   (nRF)      │              │  Thread     │
└──────┬──────┘              └──────┬───────┘              └──────┬──────┘
       │                             │                             │
       │ 1. Scan                     │                             │
       ├────────────────────────────>│                             │
       │                             │                             │
       │ 2. Advertising Packet       │                             │
       │<────────────────────────────┤                             │
       │                             │                             │
       │ 3. Connect Request          │                             │
       ├────────────────────────────>│                             │
       │                             │                             │
       │ 4. Connected                │                             │
       │<────────────────────────────┤                             │
       │                             │                             │
       │ 5. Service Discovery        │                             │
       ├────────────────────────────>│                             │
       │                             │                             │
       │ 6. Service/Char List        │                             │
       │<────────────────────────────┤                             │
       │                             │                             │
       │ 7. Enable Notifications     │                             │
       │    (Write CCC)              │                             │
       ├────────────────────────────>│                             │
       │                             │                             │
       │                             │    8. Wake up (periodic)    │
       │                             │<────────────────────────────┤
       │                             │                             │
       │                             │    9. Measure Battery       │
       │                             │<────────────────────────────┤
       │                             │                             │
       │                             │   10. Update Characteristic │
       │                             │<────────────────────────────┤
       │                             │                             │
       │ 11. Notification (Voltage)  │                             │
       │<────────────────────────────┤                             │
       │                             │                             │
       │ 12. Write New Interval      │                             │
       │     (e.g., 2000 ms)         │                             │
       ├────────────────────────────>│                             │
       │                             │                             │
       │                             │   13. Update Interval       │
       │                             ├────────────────────────────>│
       │                             │                             │
       │                             │    14. Sleep 2000ms         │
       │                             │<────────────────────────────┤
       │                             │                             │
       │ 15. Notification (Voltage)  │                             │
       │     (2 sec later)           │                             │
       │<────────────────────────────┤                             │
       │                             │                             │
```

## Memory Map

```
┌──────────────────────────────────────────────────────────┐
│                  Flash Memory Layout                      │
├──────────────────────────────────────────────────────────┤
│  Application Code                                         │
│  • main.c compiled                                        │
│  • battery_service.c compiled                             │
│  • Zephyr RTOS kernel                                     │
│  • Bluetooth stack                                        │
│                                                           │
│  ~50-100 KB (typical for this application)                │
└──────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────┐
│                   RAM Memory Layout                       │
├──────────────────────────────────────────────────────────┤
│  Bluetooth Stack                      ~8-10 KB            │
│  ├─ HCI buffers                                           │
│  ├─ GATT database                                         │
│  └─ Connection state                                      │
├──────────────────────────────────────────────────────────┤
│  Application Data                     ~50 bytes           │
│  ├─ battery_voltage_mv (2 bytes)                          │
│  └─ sample_interval_ms (4 bytes)                          │
├──────────────────────────────────────────────────────────┤
│  Thread Stacks                        ~2 KB               │
│  ├─ Main thread (default)                                 │
│  ├─ Battery monitor thread (1024 B)                       │
│  └─ BT RX/TX threads                                      │
├──────────────────────────────────────────────────────────┤
│  Kernel & Heap                        ~2-4 KB             │
└──────────────────────────────────────────────────────────┘
   Total RAM: ~12-16 KB (typical)
```

## State Machine

```
┌─────────────┐
│   INITIAL   │
│   (Reset)   │
└──────┬──────┘
       │
       │ bt_enable()
       ▼
┌─────────────┐
│ BT_ENABLED  │
└──────┬──────┘
       │
       │ service_init()
       ▼
┌─────────────┐
│ INITIALIZED │
└──────┬──────┘
       │
       │ bt_le_adv_start()
       ▼
┌─────────────┐
│ ADVERTISING │◄─────────┐
└──────┬──────┘          │
       │                 │
       │ Central         │ Disconnect
       │ Connects        │
       ▼                 │
┌─────────────┐          │
│  CONNECTED  ├──────────┘
└──────┬──────┘
       │
       │ Client enables
       │ notifications
       ▼
┌─────────────┐
│  NOTIFYING  │
│  (Active)   │
└─────────────┘
```

## Thread Interaction

```
┌────────────────────────────────────────────────────────────┐
│                      Main Thread                            │
│  • Initializes Bluetooth                                    │
│  • Starts advertising                                       │
│  • Then becomes idle (waits for events)                     │
└────────────────────────────────────────────────────────────┘
                           │
                           │ creates
                           ▼
┌────────────────────────────────────────────────────────────┐
│                 Battery Monitor Thread                      │
│  • Priority: 7 (cooperative)                                │
│  • Stack: 1024 bytes                                        │
│  • Loop:                                                    │
│    1. Sleep for interval                                    │
│    2. Measure battery                                       │
│    3. Update characteristic                                 │
│    4. Notify if enabled                                     │
│    5. Repeat                                                │
└────────────────────────────────────────────────────────────┘
                           │
                           │ calls
                           ▼
┌────────────────────────────────────────────────────────────┐
│              Bluetooth Stack Threads                        │
│  • BT RX thread (handles incoming packets)                  │
│  • BT TX thread (handles outgoing packets)                  │
│  • System work queue (deferred work)                        │
└────────────────────────────────────────────────────────────┘
```

## UUID Encoding

```
Custom 128-bit UUID Format:
┌────────────────────────────────────────────────────────────┐
│ 12345678 - 1234 - 5678 - 1234 - 56789abcdefX              │
│    │         │      │      │         │                     │
│    │         │      │      │         └─ Variable byte     │
│    │         │      │      └─────────── Fixed             │
│    │         │      └────────────────── Fixed             │
│    │         └───────────────────────── Fixed             │
│    └─────────────────────────────────── Fixed             │
└────────────────────────────────────────────────────────────┘

Service UUID:  ...def0  (X = 0)
Voltage UUID:  ...def1  (X = 1)
Interval UUID: ...def2  (X = 2)

This makes the UUIDs easy to identify and remember.
```

## Notification Flow

```
Battery Monitor Thread                BLE Stack
       │                                  │
       │ 1. New voltage measured          │
       │                                  │
       │ 2. battery_service_update_       │
       │    voltage(voltage_mv)           │
       ├─────────────────────────────────>│
       │                                  │
       │                        3. Check if│
       │                        notifications│
       │                        enabled?   │
       │                                  │
       │                        4. Build  │
       │                        notification│
       │                        packet     │
       │                                  │
       │                        5. Send to│
       │                        connected  │
       │                        central    │
       │                                  │
       │ 6. Return (success/fail)         │
       │<─────────────────────────────────┤
       │                                  │
```
