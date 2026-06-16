#ifndef HBRIDGE_H
#define HBRIDGE_H

#include <Arduino.h>

class Modbus;

class HBridge {
public:
  HBridge(uint8_t lIsPin, uint8_t lEnPin, uint8_t lPwmPin, uint8_t rIsPin,
          uint8_t rEnPin, uint8_t rPwmPin);

  void begin();
  void poll();

  void setLeftPwm(uint16_t duty);
  void setRightPwm(uint16_t duty);

  void setLeftFreq(uint16_t freqHz);
  void setRightFreq(uint16_t freqHz);

  void enableLeft(bool enable);
  void enableRight(bool enable);

private:
  friend class Modbus;

  static constexpr uint32_t kDefaultPwmFrequencyHz = 1000; // 1 kHz
  static constexpr uint32_t kCurrentSensePollIntervalMs = 100;
  static constexpr float kCurrentSenseMilliVoltToMilliAmpFactor = 7.0f;
  static constexpr adc_attenuation_t kCurrentSenseAttenuation = ADC_11db;
  static constexpr uint8_t kLeftChannel = 0;
  static constexpr uint8_t kRightChannel = 1;
  static constexpr uint8_t kDutyInputBits = 16;

  uint8_t getPwmResolutionBits(uint32_t freqHz) const;
  uint16_t dutyToResolution(uint16_t duty16, uint8_t resolutionBits) const;

  uint8_t _lIsPin;
  uint8_t _lEnPin;
  uint8_t _lPwmPin;

  uint8_t _rIsPin;
  uint8_t _rEnPin;
  uint8_t _rPwmPin;

  uint32_t _leftFreqHz;
  uint32_t _rightFreqHz;
  uint8_t _leftResolutionBits;
  uint8_t _rightResolutionBits;

  uint16_t _leftDuty16;
  uint16_t _rightDuty16;
  uint16_t _leftMilliAmps;
  uint16_t _rightMilliAmps;

  uint32_t _lastCurrentSensePollMs;

  bool _leftEnabled;
  bool _rightEnabled;
  bool _initialized;
};

#endif
