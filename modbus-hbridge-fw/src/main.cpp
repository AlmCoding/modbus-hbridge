#include "common.h"
#include <Arduino.h>
#include <HBridge.h>
#include <Modbus.h>

#define DEBUG_ENABLE_MAIN 1
#if ((DEBUG_ENABLE_MAIN == 1) && (ENABLE_DEBUG_OUTPUT == 1))
#define DEBUG_INFO(f, ...) debugPrint("[INF][Main]", f, ##__VA_ARGS__)
#define DEBUG_ERROR(f, ...) debugPrint("[ERR][Main]", f, ##__VA_ARGS__)
#else
#define DEBUG_INFO(...)
#define DEBUG_ERROR(...)
#endif

/*
// ESP32-C3 Pin Configuration (previous mapping)
// H-Bridge Pins
const uint8_t L_IS_PIN = 3;  // Left Current Sense
const uint8_t L_EN_PIN = 4;  // Left Enable
const uint8_t L_PWM_PIN = 5; // Left PWM
const uint8_t R_IS_PIN = 0;  // Right Current Sense
const uint8_t R_EN_PIN = 1;  // Right Enable
const uint8_t R_PWM_PIN = 2; // Right PWM

// RS485 Modbus Pins
const uint8_t MODBUS_RX_PIN = 20; // UART1 RX
const uint8_t MODBUS_TX_PIN = 21; // UART1 TX
const uint8_t MODBUS_RE_PIN = 2;  // RS485 driver enable
*/

// ESP32-WROOM Dev Board Pin Configuration
// H-Bridge Pins
const uint8_t L_IS_PIN = 35;  // Left Current Sense (input only)
const uint8_t L_EN_PIN = 25;  // Left Enable
const uint8_t L_PWM_PIN = 19; // Left PWM
const uint8_t R_IS_PIN = 34;  // Right Current Sense (input only)
const uint8_t R_EN_PIN = 26;  // Right Enable
const uint8_t R_PWM_PIN = 18; // Right PWM

// RS485 Modbus Pins
const uint8_t MODBUS_RX_PIN = 16; // UART2 RX
const uint8_t MODBUS_TX_PIN = 17; // UART2 TX
const uint8_t MODBUS_RE_PIN = 21; // RS485 driver enable

// Create H-Bridge and Modbus instances
HBridge hbridge(L_IS_PIN, L_EN_PIN, L_PWM_PIN, R_IS_PIN, R_EN_PIN, R_PWM_PIN);
Modbus modbus(MODBUS_RX_PIN, MODBUS_TX_PIN, MODBUS_RE_PIN, hbridge);

void setup() {
  setCpuFrequencyMhz(240);

#if (ENABLE_DEBUG_OUTPUT == 1)
  Serial.begin(921600);
#endif

  DEBUG_INFO(DIVIDER);
  DEBUG_INFO("Setup Modbus H-Bridge [...]");
  DEBUG_INFO("Firmware version: %s", FIRMWARE_VERSION);
  DEBUG_INFO("Free heap: %u bytes", ESP.getFreeHeap());
  DEBUG_INFO("CPU frequency: %d MHz", getCpuFrequencyMhz());
  sleep(0.25);

  // Initialize H-Bridge
  hbridge.enableLeft(true);
  hbridge.enableRight(true);
  hbridge.setRightPwm(0); // Start with 0 PWM
  hbridge.setLeftPwm(0);  // Start with 0 PWM

  // Initialize Modbus RTU
  modbus.begin();

  DEBUG_INFO("Setup Modbus H-Bridge [OK]");
  DEBUG_INFO(DIVIDER);
}

void loop() {
  // Process Modbus commands
  modbus.update();
}
