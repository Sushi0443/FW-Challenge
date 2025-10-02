#!/bin/bash
# Verification script for BLE Battery Monitor application

echo "=== BLE Battery Monitor - Configuration Verification ==="
echo ""

# Check if required files exist
echo "Checking project structure..."
files=(
    "CMakeLists.txt"
    "prj.conf"
    "Kconfig"
    "sample.yaml"
    "src/main.c"
    "src/battery_service.c"
    "src/battery_service.h"
)

all_present=true
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file (missing)"
        all_present=false
    fi
done

echo ""
echo "Checking service configuration..."

# Check for 128-bit UUID definitions
if grep -q "BT_UUID_128_ENCODE" src/battery_service.c; then
    echo "✓ Custom 128-bit UUIDs defined"
else
    echo "✗ Custom 128-bit UUIDs not found"
    all_present=false
fi

# Check for characteristics
voltage_char=$(grep -c "BT_UUID_BATTERY_VOLTAGE" src/battery_service.c)
interval_char=$(grep -c "BT_UUID_SAMPLE_INTERVAL" src/battery_service.c)

if [ "$voltage_char" -gt 0 ]; then
    echo "✓ Battery Voltage characteristic defined"
else
    echo "✗ Battery Voltage characteristic not found"
    all_present=false
fi

if [ "$interval_char" -gt 0 ]; then
    echo "✓ Sample Interval characteristic defined"
else
    echo "✗ Sample Interval characteristic not found"
    all_present=false
fi

# Check for GATT service definition
if grep -q "BT_GATT_SERVICE_DEFINE" src/battery_service.c; then
    echo "✓ GATT service properly defined"
else
    echo "✗ GATT service definition not found"
    all_present=false
fi

# Check for advertising setup
if grep -q "bt_le_adv_start" src/main.c; then
    echo "✓ BLE advertising configured"
else
    echo "✗ BLE advertising not configured"
    all_present=false
fi

# Check for notification support
if grep -q "BT_GATT_CHRC_NOTIFY" src/battery_service.c; then
    echo "✓ Notification support enabled"
else
    echo "✗ Notification support not found"
    all_present=false
fi

echo ""
echo "Checking Bluetooth configuration..."

# Check prj.conf
if grep -q "CONFIG_BT=y" prj.conf; then
    echo "✓ Bluetooth enabled"
else
    echo "✗ Bluetooth not enabled"
    all_present=false
fi

if grep -q "CONFIG_BT_PERIPHERAL=y" prj.conf; then
    echo "✓ Peripheral role enabled"
else
    echo "✗ Peripheral role not enabled"
    all_present=false
fi

echo ""
if [ "$all_present" = true ]; then
    echo "=== Verification PASSED ==="
    echo "All required components are present and properly configured."
    exit 0
else
    echo "=== Verification FAILED ==="
    echo "Some components are missing or not properly configured."
    exit 1
fi
