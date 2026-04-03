#include <Arduino.h>
#include <HBridge.h>
#include <Modbus.h>

// ESP32-C3 Pin Configuration
// H-Bridge Pins
const uint8_t R_IS_PIN = 0;  // Right Current Sense
const uint8_t R_EN_PIN = 1;  // Right Enable
const uint8_t R_PWM_PIN = 2; // Right PWM
const uint8_t L_IS_PIN = 3;  // Left Current Sense
const uint8_t L_EN_PIN = 4;  // Left Enable
const uint8_t L_PWM_PIN = 5; // Left PWM

// RS485 Modbus Pins
const uint8_t MODBUS_RX_PIN = 20; // UART1 RX
const uint8_t MODBUS_TX_PIN = 21; // UART1 TX
const uint8_t MODBUS_RE_PIN = 2;  // RS485 driver enable

// Create H-Bridge and Modbus instances
HBridge hbridge(R_IS_PIN, R_EN_PIN, R_PWM_PIN, L_IS_PIN, L_EN_PIN, L_PWM_PIN);
Modbus modbus(MODBUS_RX_PIN, MODBUS_TX_PIN, MODBUS_RE_PIN, hbridge);

void setup() {
  // Initialize H-Bridge
  hbridge.enableRight(true);
  hbridge.enableLeft(true);
  hbridge.setRightPwm(uint16_t(0)); // Start with 0 PWM
  hbridge.setLeftPwm(uint16_t(0));  // Start with 0 PWM

  // Initialize Modbus RTU
  modbus.begin();
}

void loop() {
  // Process Modbus commands
  modbus.update();
}
