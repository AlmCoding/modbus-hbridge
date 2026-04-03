#include "HBridge.h"

HBridge::HBridge(uint8_t lIsPin, uint8_t lEnPin, uint8_t lPwmPin,
                 uint8_t rIsPin, uint8_t rEnPin, uint8_t rPwmPin)
    : _lIsPin(lIsPin), _lEnPin(lEnPin), _lPwmPin(lPwmPin), //
      _rIsPin(rIsPin), _rEnPin(rEnPin), _rPwmPin(rPwmPin),
      _leftFreqHz(kDefaultPwmFrequencyHz), _rightFreqHz(kDefaultPwmFrequencyHz),
      _leftDuty16(0), _rightDuty16(0) {
  pinMode(_lIsPin, INPUT);
  pinMode(_rIsPin, INPUT);

  pinMode(_lEnPin, OUTPUT);
  pinMode(_rEnPin, OUTPUT);
  digitalWrite(_lEnPin, HIGH);
  digitalWrite(_rEnPin, HIGH);

  ledcSetup(kLeftChannel, _leftFreqHz, kPwmResolutionBits);
  ledcSetup(kRightChannel, _rightFreqHz, kPwmResolutionBits);
  ledcAttachPin(_lPwmPin, kLeftChannel);
  ledcAttachPin(_rPwmPin, kRightChannel);

  ledcWrite(kLeftChannel, _leftDuty16);
  ledcWrite(kRightChannel, _rightDuty16);
}

void HBridge::setLeftPwm(uint16_t duty) {
  _leftDuty16 = duty;
  ledcWrite(kLeftChannel, _leftDuty16);
}

void HBridge::setRightPwm(uint16_t duty) {
  _rightDuty16 = duty;
  ledcWrite(kRightChannel, _rightDuty16);
}

void HBridge::setLeftFreq(uint16_t freqHz) {
  if (freqHz == 0U) {
    return;
  }

  _leftFreqHz = freqHz;
  ledcSetup(kLeftChannel, _leftFreqHz, kPwmResolutionBits);
  ledcWrite(kLeftChannel, _leftDuty16);
}

void HBridge::setRightFreq(uint16_t freqHz) {
  if (freqHz == 0U) {
    return;
  }

  _rightFreqHz = freqHz;
  ledcSetup(kRightChannel, _rightFreqHz, kPwmResolutionBits);
  ledcWrite(kRightChannel, _rightDuty16);
}

void HBridge::enableLeft(bool enable) {
  _leftEnabled = enable;
  digitalWrite(_lEnPin, enable ? HIGH : LOW);
}

void HBridge::enableRight(bool enable) {
  _rightEnabled = enable;
  digitalWrite(_rEnPin, enable ? HIGH : LOW);
}
