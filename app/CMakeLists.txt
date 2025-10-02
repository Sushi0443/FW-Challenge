#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fw_challenge)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/adc_sampler.c
  src/buttons.c
  src/status_led.c
  src/app_events.c
  src/ble.c
  src/watchdog.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# zephyr_dts_bindings_path(dts/bindings)
# zephyr_add_dt_binding_path(dts)
# NORDIC SDK APP END
