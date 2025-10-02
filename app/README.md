FW Challenge Application

This is a Zephyr application that:
- Samples battery voltage via ADC at a configurable interval (CONFIG_APP_SAMPLE_INTERVAL_MS) which is configurable either by DT or Kconfig. As soon as a sample is taken, Power manager puts the adc module to sleep. Unfortunately, I do not have hardware that supports PM and was not able to fully test its functionality. I have place code snippets for it under config gaurds either way. When this firmware gets flashed on a capable hardware, it should run smoothly.
- Advertises a custom BLE service with two characteristics (Voltage and Sample Interval). Includes CCC and CPF.
- Sends push notifications of Voltage to a client when it subscribes.
- User button disables adc sampling to conserve power and also stops pushing notifications. Includes a software debounce.
- Persists a sample counter using Zephyr Settings + NVS. Gets logged and stored everytime a sample is taken. To avo
- Led blinking is done on delayed work items. idle state blinks less frequently. Sample is indicated by quick double blink and error state is indicated by rapid blinking.
- As soon as any module(Button, adc, ble) reports an error, an event is registered and the callback is delegated to a work item. all work items are suspended until reset if an error is registered.
- Watchdog is fed every 8/2 seconds
- Custom device tree overlays for custom boards are provided. This application was developed and tested on nrf52dk instead of native-sim. Overlays for 
- Kconfig with project specific options were added and handled cleverly in the code as it takes precedence over DT.

How to build (example)
- Select your board, then build normally in nRF Connect for VS Code or with west.
- use board specific .yaml and .overlay files

How to run
- After the hardware boots, you should see ADC sampling at the sample rate by looking at the LED.
- When you connect to a central over bluetooth, you should be able to see Unknown services with user defined UUID's  

Key Kconfig options
- CONFIG_APP_SAMPLE_INTERVAL_MS: ADC sampling interval in ms (default 1000)
- CONFIG_APP_ENABLE_BLE: Start with BLE enabled (default y)
- CONFIG_APP_LED_ACTIVE_LOW: Polarity for status LED (default y)
- APP_VOLTAGE_THRESHOLD_MV : Voltage threshold below which a warning log is sent
- APP_BLE_DEVICE_NAME : The name of the device that appears to central
- APP_BUTTON_DEBOUNCE_DELAY_MS : debouce period can be calibratable
- APP_LOG_LEVEL : Log level accross the application
There are other options to enable watchdog, watchdog timeout, enable PM, enable settings for persistant storage

Notes
- BLE functionality is in src/ble.c with public API in include/ble.h.
- Main application logic lives in src/main.c; ADC sampling in src/adc_sampler.c.
- LED patterns are in src/app_events.c and src/status_led.c.
- User button and debouncing is done is src/buttons.c
- Error handling is done is app_events.c and watchdog is implemented in watchdog.c
- AI was used to generate minimal code like function headers and syntaxes.

