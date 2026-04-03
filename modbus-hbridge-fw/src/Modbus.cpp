#include "Modbus.h"
#include <HardwareSerial.h>
#include <Preferences.h>

namespace {
constexpr const char *kPrefsNamespace = "modbus";
constexpr const char *kPrefsKeySlaveAddress = "slave_addr";
constexpr const char *kPrefsKeyBaudRate = "baud_rate";
} // namespace

// Using Serial1 for UART1 on ESP32
extern HardwareSerial Serial1;

Modbus::Modbus(uint8_t rxPin, uint8_t txPin, uint8_t rePin, HBridge &hbridge)
    : _rxPin(rxPin), _txPin(txPin), _rePin(rePin), _hbridge(hbridge),
      _slaveAddress(kDefaultSlaveAddress), _baudRate(kDefaultBaudRate),
      _baudRateReconfigPending(false), _rxIndex(0), _lastRxTime(0) {}

void Modbus::begin(uint8_t slaveAddr, uint32_t baudRate) {
  loadPersistedSettings(slaveAddr, baudRate);

  // Initialize RS485 control pin
  pinMode(_rePin, OUTPUT);
  disableTransmit();

  // Initialize UART1 with selected pins
  Serial1.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
}

void Modbus::loadPersistedSettings(uint8_t defaultSlaveAddress,
                                   uint32_t defaultBaudRate) {
  _slaveAddress = defaultSlaveAddress;
  _baudRate = defaultBaudRate;

  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return;
  }

  _slaveAddress = prefs.getUChar(kPrefsKeySlaveAddress, _slaveAddress);
  _baudRate = prefs.getUInt(kPrefsKeyBaudRate, _baudRate);
  prefs.end();
}

void Modbus::saveSlaveAddress() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }

  prefs.putUChar(kPrefsKeySlaveAddress, _slaveAddress);
  prefs.end();
}

void Modbus::saveBaudRate() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }

  prefs.putUInt(kPrefsKeyBaudRate, _baudRate);
  prefs.end();
}

void Modbus::update() {
  // Process incoming serial data
  while (Serial1.available()) {
    uint8_t byte = Serial1.read();
    _rxBuffer[_rxIndex++] = byte;
    _lastRxTime = millis();

    if (_rxIndex >= kBufferSize) {
      _rxIndex = 0; // Buffer overflow, reset
    }
  }

  // Process complete frame after timeout
  if (_rxIndex > 0 && (millis() - _lastRxTime) > kRxTimeoutMs) {
    processRxData();
    _rxIndex = 0;
  }
}

void Modbus::processRxData() {
  if (_rxIndex < 4) { // Minimum frame: address(1) + func(1) + crc(2)
    return;
  }

  // Check if frame is for us
  if (_rxBuffer[0] != _slaveAddress) {
    return;
  }

  // Validate CRC
  if (!validateCRC(_rxBuffer, _rxIndex)) {
    return;
  }

  // Process the command
  handleModbusCommand(_rxBuffer, _rxIndex);
}

void Modbus::handleModbusCommand(const uint8_t *request, size_t requestLen) {
  uint8_t functionCode = request[1];

  switch (functionCode) {
  case ReadHoldingRegisters:
    handleFunctionCode03(request, requestLen);
    break;
  case WriteSingleRegister:
    handleFunctionCode06(request, requestLen);
    break;
  case WriteMultipleRegisters:
    handleFunctionCode16(request, requestLen);
    break;
  default:
    // Unknown function code - could send exception response
    break;
  }
}

void Modbus::handleFunctionCode03(const uint8_t *request, size_t requestLen) {
  // FC03: Read Holding Registers
  // Request: Address(1) + FC(1) + StartAddr(2) + Quantity(2) + CRC(2)
  // Response: Address(1) + FC(1) + ByteCount(1) + Data(2*N) + CRC(2)

  if (requestLen != 8) {
    return;
  }

  uint16_t startAddr = (request[2] << 8) | request[3];
  uint16_t quantity = (request[4] << 8) | request[5];

  if (quantity == 0 || quantity > REG_MAX) {
    return; // Invalid quantity
  }

  uint8_t response[REG_MAX * 2 + 5]; // Max response size
  response[0] = request[0];          // Slave address
  response[1] = request[1];          // Function code
  response[2] = quantity * 2;        // Byte count

  size_t responseLen = 3;
  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t value = readRegister(startAddr + i);
    response[responseLen++] = (value >> 8) & 0xFF;
    response[responseLen++] = value & 0xFF;
  }

  // Add CRC
  uint16_t crc = calculateCRC(response, responseLen);
  response[responseLen++] = crc & 0xFF;
  response[responseLen++] = (crc >> 8) & 0xFF;

  sendData(response, responseLen);
}

void Modbus::handleFunctionCode06(const uint8_t *request, size_t requestLen) {
  // FC06: Write Single Register
  // Request: Address(1) + FC(1) + RegAddr(2) + RegValue(2) + CRC(2)
  // Response: Echo of request

  if (requestLen != 8) {
    return;
  }

  uint16_t regAddr = (request[2] << 8) | request[3];
  uint16_t regValue = (request[4] << 8) | request[5];

  writeRegister(regAddr, regValue);

  // Request frame was already CRC-validated; FC06 response is an exact echo.
  sendData(request, 8);
}

void Modbus::handleFunctionCode16(const uint8_t *request, size_t requestLen) {
  // FC16: Write Multiple Registers
  // Request: Address(1) + FC(1) + StartAddr(2) + Quantity(2) +
  // ByteCount(1) + Data(2*N) + CRC(2)
  // Response: Address(1) + FC(1) + StartAddr(2) + Quantity(2) + CRC(2)

  if (requestLen < 9) {
    return;
  }

  uint16_t startAddr = (request[2] << 8) | request[3];
  uint16_t quantity = (request[4] << 8) | request[5];
  uint8_t byteCount = request[6];

  if (byteCount != quantity * 2 || requestLen < (9 + byteCount)) {
    return; // Invalid data
  }

  // Write registers
  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t regValue = (request[7 + i * 2] << 8) | request[8 + i * 2];
    writeRegister(startAddr + i, regValue);
  }

  // Send response
  uint8_t response[8];
  response[0] = request[0]; // Slave address
  response[1] = request[1]; // Function code
  response[2] = request[2]; // Start address high
  response[3] = request[3]; // Start address low
  response[4] = request[4]; // Quantity high
  response[5] = request[5]; // Quantity low

  uint16_t crc = calculateCRC(response, 6);
  response[6] = crc & 0xFF;
  response[7] = (crc >> 8) & 0xFF;

  sendData(response, 8);
}

uint16_t Modbus::readRegister(uint16_t address) {
  switch (address) {
  case REG_SLAVE_ADDRESS:
    return _slaveAddress;
  case REG_BAUD_RATE:
    // Return baud rate in units (e.g., 9600 / 100 = 96)
    return _baudRate / 100;
  case REG_LEFT_PWM:
    return _hbridge._leftDuty16;
  case REG_RIGHT_PWM:
    return _hbridge._rightDuty16;
  case REG_LEFT_FREQ:
    return _hbridge._leftFreqHz;
  case REG_RIGHT_FREQ:
    return _hbridge._rightFreqHz;
  case REG_ENABLE: {
    // Bit 0: left enable, Bit 1: right enable
    uint16_t enableStatus = _hbridge._leftEnabled ? 0x01 : 0x00;
    enableStatus |= _hbridge._rightEnabled ? 0x02 : 0x00;
    return enableStatus;
  }
  default:
    return 0;
  }
}

void Modbus::writeRegister(uint16_t address, uint16_t value) {
  switch (address) {
  case REG_SLAVE_ADDRESS:
    if (value <= 0xFF) {
      _slaveAddress = value;
      saveSlaveAddress();
    }
    break;
  case REG_BAUD_RATE:
    // Value is in units of 100 (e.g., 96 = 9600)
    if (value >= 12 && value <= 9216) { // Support 1200 to 921600 baud
      _baudRate = value * 100;
      saveBaudRate();
      _baudRateReconfigPending = true;
    }
    break;
  case REG_LEFT_PWM:
    _hbridge.setLeftPwm(value);
    break;
  case REG_RIGHT_PWM:
    _hbridge.setRightPwm(value);
    break;
  case REG_LEFT_FREQ:
    _hbridge.setLeftFreq(value);
    break;
  case REG_RIGHT_FREQ:
    _hbridge.setRightFreq(value);
    break;
  case REG_ENABLE:
    // Bit 0: left enable, Bit 1: right enable
    _hbridge.enableLeft((value & 0x01) != 0);
    _hbridge.enableRight((value & 0x02) != 0);
    break;
  default:
    // Future extension: ignore unknown registers
    break;
  }
}

uint16_t Modbus::calculateCRC(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ kCrcPoly;
      } else {
        crc = crc >> 1;
      }
    }
  }

  return crc;
}

bool Modbus::validateCRC(const uint8_t *data, size_t len) {
  if (len < 3) {
    return false;
  }

  uint16_t receivedCrc = (data[len - 1] << 8) | data[len - 2];
  uint16_t calculatedCrc = calculateCRC(data, len - 2);

  return receivedCrc == calculatedCrc;
}

void Modbus::sendData(const uint8_t *data, size_t len) {
  enableTransmit();
  // delayMicroseconds(100);

  Serial1.write(data, len);
  Serial1.flush();

  // delayMicroseconds(100);
  disableTransmit();

  if (_baudRateReconfigPending) {
    Serial1.begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
    _baudRateReconfigPending = false;
  }
}

void Modbus::enableTransmit() { digitalWrite(_rePin, HIGH); }

void Modbus::disableTransmit() { digitalWrite(_rePin, LOW); }
