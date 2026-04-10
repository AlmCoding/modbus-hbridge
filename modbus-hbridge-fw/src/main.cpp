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
const uint8_t MODBUS_RE_PIN = 10; // RS485 driver enable

const uint8_t LED_PIN = 8; // On-board LED pin
const uint32_t LED_TOGGLE_INTERVAL_MS = 250;

// Create H-Bridge and Modbus instances
HBridge hbridge(L_IS_PIN, L_EN_PIN, L_PWM_PIN, R_IS_PIN, R_EN_PIN, R_PWM_PIN);
Modbus modbus(MODBUS_RX_PIN, MODBUS_TX_PIN, MODBUS_RE_PIN, hbridge);

void setup() {
  setCpuFrequencyMhz(160);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

#if (ENABLE_DEBUG_OUTPUT == 1)
  delay(3000);
  Serial.begin(921600);
  while (!Serial) {
    delay(100);
  }
#endif

  DEBUG_INFO(DIVIDER);
  DEBUG_INFO("Setup Modbus H-Bridge [...]");
  DEBUG_INFO("Firmware version: %s", FIRMWARE_VERSION);
  DEBUG_INFO("Free heap: %u bytes", ESP.getFreeHeap());
  DEBUG_INFO("CPU frequency: %d MHz", getCpuFrequencyMhz());

  // Initialize H-Bridge
  hbridge.begin();
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

  // Toggle LED for heartbeat
  static uint32_t lastToggleTime = 0;
  uint32_t currentTime = millis();
  if (currentTime - lastToggleTime >= LED_TOGGLE_INTERVAL_MS) {
    lastToggleTime = currentTime;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}
