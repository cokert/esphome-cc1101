// Based on https://github.com/dbuezas/esphome-cc1101
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace cc1101 {

static int cc1101_module_count = 0;

class CC1101 : public Component {
 protected:
  int sck_, miso_, mosi_, csn_, gdo0_;
  float bandwidth_, freq_;
  int module_number_;
  remote_transmitter::RemoteTransmitterComponent *transmitter_;

 public:
  CC1101(int sck, int miso, int mosi, int csn, int gdo0,
         float bandwidth, float freq,
         remote_transmitter::RemoteTransmitterComponent *transmitter)
      : sck_(sck), miso_(miso), mosi_(mosi), csn_(csn), gdo0_(gdo0),
        bandwidth_(bandwidth), freq_(freq),
        module_number_(cc1101_module_count++),
        transmitter_(transmitter) {}

  void setup() override {
    pinMode(gdo0_, INPUT);
    ELECHOUSE_cc1101.addSpiPin(sck_, miso_, mosi_, csn_, module_number_);
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setRxBW(bandwidth_);
    ELECHOUSE_cc1101.setMHZ(freq_);
    ELECHOUSE_cc1101.SetRx();
  }

  void beginTransmission() {
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.SetTx();
    pinMode(gdo0_, OUTPUT);
#ifdef USE_ESP32
    transmitter_->setup();
#endif
#ifdef USE_ESP8266
    noInterrupts();
#endif
  }

  void endTransmission() {
    digitalWrite(gdo0_, 0);
    pinMode(gdo0_, INPUT);
#ifdef USE_ESP8266
    interrupts();
#endif
    ELECHOUSE_cc1101.setModul(module_number_);
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.SetRx();  // yes, twice — CC1101 quirk
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

}  // namespace cc1101
}  // namespace esphome
