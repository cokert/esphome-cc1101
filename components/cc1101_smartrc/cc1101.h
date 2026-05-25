// Based on https://github.com/dbuezas/esphome-cc1101
// Renamed to cc1101_smartrc to avoid collision with the native ESPHome cc1101
// component added in ESPHome 2026.x.
//
// TX/RX mode switching is handled externally via on_transmit/on_complete hooks
// on remote_transmitter, so this component no longer needs a transmitter reference.
#pragma once

#include "esphome/core/component.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace cc1101_smartrc {

static int cc1101_module_count = 0;

class CC1101 : public Component {
 protected:
  int sck_, miso_, mosi_, csn_, gdo0_;
  float bandwidth_, freq_;
  int module_number_;

 public:
  CC1101(int sck, int miso, int mosi, int csn, int gdo0, float bandwidth, float freq)
      : sck_(sck), miso_(miso), mosi_(mosi), csn_(csn), gdo0_(gdo0),
        bandwidth_(bandwidth), freq_(freq),
        module_number_(cc1101_module_count++) {}

  void setup() override {
    pinMode(gdo0_, INPUT);
    ELECHOUSE_cc1101.addSpiPin(sck_, miso_, mosi_, csn_, module_number_);
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(bandwidth_);
    ELECHOUSE_cc1101.setMHZ(freq_);
    ELECHOUSE_cc1101.SetRx();
  }

  // Call from remote_transmitter on_transmit hook
  void beginTransmission() {
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.SetTx();
    pinMode(gdo0_, OUTPUT);
  }

  // Call from remote_transmitter on_complete hook
  void endTransmission() {
    digitalWrite(gdo0_, 0);
    pinMode(gdo0_, INPUT);
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.SetRx();  // yes, twice — CC1101 quirk documented in dev-diary.md
  }

  void setBW(float bandwidth) {
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.setRxBW(bandwidth);
  }

  void setFreq(float freq) {
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.setMHZ(freq);
  }
};

}  // namespace cc1101_smartrc
}  // namespace esphome
