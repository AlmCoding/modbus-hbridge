#ifndef MODBUS_H
#define MODBUS_H

#include "HBridge.h"
#include <Arduino.h>

class Modbus {
public:
  Modbus(uint8_t rxPin, uint8_t txPin, uint8_t rePin, HBridge &hbridge);

  void begin(uint8_t slaveAddr = kDefaultSlaveAddress,
             uint32_t baudRate = kDefaultBaudRate);
  void update(); // Call regularly in main loop to process incoming data

  uint8_t getSlaveAddress() const { return _slaveAddress; }
  uint32_t getBaudRate() const { return _baudRate; }

private:
  // Modbus function codes
  enum FunctionCode : uint8_t {
    ReadHoldingRegisters = 0x03,
    WriteSingleRegister = 0x06,
    WriteMultipleRegisters = 0x10,
  };

  // Register addresses for extensible command support
  enum RegisterAddress : uint16_t {
    REG_SLAVE_ADDRESS = 0x00,
    REG_BAUD_RATE = 0x01,
    REG_LEFT_PWM = 0x02,
    REG_RIGHT_PWM = 0x03,
    REG_LEFT_FREQ = 0x04,
    REG_RIGHT_FREQ = 0x05,
    REG_ENABLE = 0x06, // Bit 0: left enable, Bit 1: right enable
    REG_MAX,           // Sentinel value for bounds checking
  };

  static constexpr uint8_t kDefaultSlaveAddress = 0x01;
  static constexpr uint32_t kDefaultBaudRate = 9600;
  static constexpr uint16_t kCrcPoly = 0xA001;
  static constexpr size_t kBufferSize = 256 + 1; // Max Modbus RTU frame size
  static constexpr uint32_t kRxTimeoutMs = 4;    // Inter-frame timeout

  uint8_t _rxPin;
  uint8_t _txPin;
  uint8_t _rePin; // RS485 driver enable pin

  HBridge &_hbridge;

  uint8_t _slaveAddress;
  uint32_t _baudRate;
  bool _baudRateReconfigPending;

  uint8_t _rxBuffer[kBufferSize];
  size_t _rxIndex;
  unsigned long _lastRxTime;

  // Modbus utilities
  uint16_t calculateCRC(const uint8_t *data, size_t len);
  bool validateCRC(const uint8_t *data, size_t len);

  // Command processing
  void processRxData();
  void handleModbusCommand(const uint8_t *request, size_t requestLen);

  // Modbus function handlers
  void handleFunctionCode03(const uint8_t *request,
                            size_t requestLen); // Read Holding Registers
  void handleFunctionCode06(const uint8_t *request,
                            size_t requestLen); // Write Single Register
  void handleFunctionCode16(const uint8_t *request,
                            size_t requestLen); // Write Multiple Registers

  // RS485 communication
  void sendData(const uint8_t *data, size_t len);
  void enableTransmit();
  void disableTransmit();

  // Register access (extensible for future commands)
  uint16_t readRegister(uint16_t address);
  void writeRegister(uint16_t address, uint16_t value);

  // Persistent settings storage
  void loadPersistedSettings(uint8_t defaultSlaveAddress,
                             uint32_t defaultBaudRate);
  void saveSlaveAddress();
  void saveBaudRate();
};

#endif
