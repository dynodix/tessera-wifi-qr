#pragma once
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::wifi_qr {

// The plugin: an ESPHome component and a tessera::Plugin. It registers its tile types in setup().
class WifiQr : public Component, public tessera::Plugin {
 public:
  void set_ssid(text_sensor::TextSensor *ssid) { ssid_ = ssid; }
  void set_password(text_sensor::TextSensor *password) { password_ = password; }
  const std::string &ssid() const { return ssid_->state; }
  const std::string &password() const { return password_->state; }

  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 private:
  text_sensor::TextSensor *ssid_{};
  text_sensor::TextSensor *password_{};
};

}  // namespace esphome::wifi_qr
