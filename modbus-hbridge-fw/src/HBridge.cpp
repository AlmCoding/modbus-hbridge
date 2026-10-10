#include "HBridge.h"
#include "common.h"

#define DEBUG_ENABLE_HBRIDGE 1
#if ((DEBUG_ENABLE_HBRIDGE == 1) && (ENABLE_DEBUG_OUTPUT == 1))
#define DEBUG_INFO(f, ...) debugPrint("[INF][HBridge]", f, ##__VA_ARGS__)
#define DEBUG_ERROR(f, ...) debugPrint("[ERR][HBridge]", f, ##__VA_ARGS__)
#else
#define DEBUG_INFO(...)
#define DEBUG_ERROR(...)
#endif

HBridge::HBridge(uint8_t lIsPin, uint8_t lEnPin, uint8_t lPwmPin,
                 uint8_t rIsPin, uint8_t rEnPin, uint8_t rPwmPin)
    : _lIsPin(lIsPin), _lEnPin(lEnPin), _lPwmPin(lPwmPin), //
      _rIsPin(rIsPin), _rEnPin(rEnPin), _rPwmPin(rPwmPin),
      _leftFreqHz(kDefaultPwmFrequencyHz), _rightFreqHz(kDefaultPwmFrequencyHz),
      _leftResolutionBits(getPwmResolutionBits(kDefaultPwmFrequencyHz)),
      _rightResolutionBits(getPwmResolutionBits(kDefaultPwmFrequencyHz)),
  _leftDuty16(0), _rightDuty16(0), _leftMilliAmps(0),
  _rightMilliAmps(0), _lastCurrentSensePollMs(0), _leftEnabled(true),
  _rightEnabled(true), _initialized(false) {}

void HBridge::begin() {
  if (_initialized) {
    return;
  }

  pinMode(_lIsPin, INPUT);
  pinMode(_rIsPin, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(_lIsPin, kCurrentSenseAttenuation);
  analogSetPinAttenuation(_rIsPin, kCurrentSenseAttenuation);

  pinMode(_lEnPin, OUTPUT);
  pinMode(_rEnPin, OUTPUT);
  digitalWrite(_lEnPin, HIGH);
  digitalWrite(_rEnPin, HIGH);

  _leftResolutionBits = getPwmResolutionBits(_leftFreqHz);
  _rightResolutionBits = getPwmResolutionBits(_rightFreqHz);

  ledcSetup(kLeftChannel, _leftFreqHz, _leftResolutionBits);
  ledcSetup(kRightChannel, _rightFreqHz, _rightResolutionBits);
  ledcAttachPin(_lPwmPin, kLeftChannel);
  ledcAttachPin(_rPwmPin, kRightChannel);

  ledcWrite(kLeftChannel, dutyToResolution(_leftDuty16, _leftResolutionBits));
  ledcWrite(kRightChannel,
            dutyToResolution(_rightDuty16, _rightResolutionBits));

  _lastCurrentSensePollMs = millis() - kCurrentSensePollIntervalMs;
  _initialized = true;
}

void HBridge::poll() {
  const uint32_t now = millis();
  if ((now - _lastCurrentSensePollMs) < kCurrentSensePollIntervalMs) {
    return;
  }
  _lastCurrentSensePollMs = now;

  const uint32_t leftMilliVolts = analogReadMilliVolts(_lIsPin);
  const uint32_t rightMilliVolts = analogReadMilliVolts(_rIsPin);
    _leftMilliAmps = static_cast<uint16_t>(
      (leftMilliVolts * kCurrentSenseMilliVoltToMilliAmpFactor) + 0.5f);
    _rightMilliAmps = static_cast<uint16_t>(
      (rightMilliVolts * kCurrentSenseMilliVoltToMilliAmpFactor) + 0.5f);

  DEBUG_INFO("IS_L=%04lu mV (%04lu mA), IS_R=%04lu mV (%04lu mA)",
         leftMilliVolts, _leftMilliAmps, rightMilliVolts, _rightMilliAmps);
}

void HBridge::setLeftPwm(uint16_t duty) {
  _leftDuty16 = duty;
  ledcWrite(kLeftChannel, dutyToResolution(_leftDuty16, _leftResolutionBits));
}

void HBridge::setRightPwm(uint16_t duty) {
  _rightDuty16 = duty;
  ledcWrite(kRightChannel,
            dutyToResolution(_rightDuty16, _rightResolutionBits));
}

void HBridge::setLeftFreq(uint16_t freqHz) {
  if (freqHz == 0U) {
    return;
  }

  _leftFreqHz = freqHz;
  _leftResolutionBits = getPwmResolutionBits(_leftFreqHz);
  ledcSetup(kLeftChannel, _leftFreqHz, _leftResolutionBits);
  ledcWrite(kLeftChannel, dutyToResolution(_leftDuty16, _leftResolutionBits));
}

void HBridge::setRightFreq(uint16_t freqHz) {
  if (freqHz == 0U) {
    return;
  }

  _rightFreqHz = freqHz;
  _rightResolutionBits = getPwmResolutionBits(_rightFreqHz);
  ledcSetup(kRightChannel, _rightFreqHz, _rightResolutionBits);
  ledcWrite(kRightChannel,
            dutyToResolution(_rightDuty16, _rightResolutionBits));
}

void HBridge::enableLeft(bool enable) {
  _leftEnabled = enable;
  digitalWrite(_lEnPin, enable ? HIGH : LOW);
}

void HBridge::enableRight(bool enable) {
  _rightEnabled = enable;
  digitalWrite(_rEnPin, enable ? HIGH : LOW);
}

uint16_t HBridge::getLeftCurrentMilliAmps() const { return _leftMilliAmps; }

uint16_t HBridge::getRightCurrentMilliAmps() const { return _rightMilliAmps; }

uint8_t HBridge::getPwmResolutionBits(uint32_t freqHz) const {
  /*
  if (freqHz <= 1000U) {
    return 16U;
  } else if (freqHz <= 2000U) {
    return 15U;
  } else
   */
  if (freqHz <= 4000U) {
    return 14U;
  } else if (freqHz <= 8000U) {
    return 13U;
  } else if (freqHz <= 16000U) {
    return 12U;
  } else if (freqHz <= 32000U) {
    return 11U;
  }
  return 10U;
}

uint16_t HBridge::dutyToResolution(uint16_t duty16,
                                   uint8_t resolutionBits) const {
  if (resolutionBits >= kDutyInputBits) {
    return duty16;
  }

  return duty16 >> (kDutyInputBits - resolutionBits);
}
