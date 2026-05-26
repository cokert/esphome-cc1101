// Self-contained CC1101 driver for ESPHome.
// No external libraries — uses ESPHome's SPI component directly.
//
// Ported from SmartRC-CC1101-Driver-Lib v3.0.2 by Little Satan (SmartRC)
// https://github.com/lsatan/SmartRC-CC1101-Driver-Lib
//
// Key source references:
//   SmartRC_CC1101::Reset()            → reset()
//   SmartRC_CC1101::RegConfigSettings() → setup() register block
//   SmartRC_CC1101::setCCMode(false)   → setup() IOCFG/PKTCTRL/MDMCFG writes
//   SmartRC_CC1101::setModulation(OOK) → setup() MDMCFG2/FREND0 + PA table
//   SmartRC_CC1101::setMHZ()           → set_mhz()
//   SmartRC_CC1101::Calibrate()        → calibrate()
//   SmartRC_CC1101::setRxBW()          → set_rx_bw()
//   SmartRC_CC1101::SetTx/SetRx()      → beginTransmission() / endTransmission()
//
// Configured for ASK/OOK async serial mode (remote_transmitter / remote_receiver).
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include <math.h>

namespace esphome {
namespace cc1101_smartrc {

// ---------------------------------------------------------------------------
// CC1101 register addresses
// ---------------------------------------------------------------------------
static const uint8_t R_IOCFG2    = 0x00;
static const uint8_t R_IOCFG0    = 0x02;
static const uint8_t R_PKTCTRL1  = 0x07;
static const uint8_t R_PKTCTRL0  = 0x08;
static const uint8_t R_ADDR      = 0x09;
static const uint8_t R_CHANNR    = 0x0A;
static const uint8_t R_FSCTRL1   = 0x0B;
static const uint8_t R_FSCTRL0   = 0x0C;
static const uint8_t R_FREQ2     = 0x0D;
static const uint8_t R_FREQ1     = 0x0E;
static const uint8_t R_FREQ0     = 0x0F;
static const uint8_t R_MDMCFG4   = 0x10;
static const uint8_t R_MDMCFG3   = 0x11;
static const uint8_t R_MDMCFG2   = 0x12;
static const uint8_t R_MDMCFG1   = 0x13;
static const uint8_t R_MDMCFG0   = 0x14;
static const uint8_t R_DEVIATN   = 0x15;
static const uint8_t R_MCSM0     = 0x18;
static const uint8_t R_FOCCFG    = 0x19;
static const uint8_t R_BSCFG     = 0x1A;
static const uint8_t R_AGCCTRL2  = 0x1B;
static const uint8_t R_AGCCTRL1  = 0x1C;
static const uint8_t R_AGCCTRL0  = 0x1D;
static const uint8_t R_FREND1    = 0x21;
static const uint8_t R_FREND0    = 0x22;
static const uint8_t R_FSCAL3    = 0x23;
static const uint8_t R_FSCAL2    = 0x24;
static const uint8_t R_FSCAL1    = 0x25;
static const uint8_t R_FSCAL0    = 0x26;
static const uint8_t R_FSTEST    = 0x29;
static const uint8_t R_TEST2     = 0x2C;
static const uint8_t R_TEST1     = 0x2D;
static const uint8_t R_TEST0     = 0x2E;
static const uint8_t R_PKTLEN    = 0x06;
static const uint8_t R_PATABLE   = 0x3E;

// Strobes
static const uint8_t S_SRES      = 0x30;
static const uint8_t S_SRX       = 0x34;
static const uint8_t S_STX       = 0x35;
static const uint8_t S_SIDLE     = 0x36;

static const uint8_t SPI_READ_BURST  = 0xC0;
static const uint8_t SPI_WRITE_BURST = 0x40;
static const float   F_OSC           = 26000000.0f;

// ---------------------------------------------------------------------------

class CC1101 : public Component,
               public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                     spi::CLOCK_POLARITY_LOW,
                                     spi::CLOCK_PHASE_LEADING,
                                     spi::DATA_RATE_4MHZ> {
 protected:
  int gdo0_;
  float bandwidth_khz_, freq_mhz_;

  // --- SPI primitives ---

  void write_reg(uint8_t addr, uint8_t value) {
    this->enable();
    this->transfer_byte(addr);
    this->transfer_byte(value);
    this->disable();
  }

  uint8_t read_status(uint8_t addr) {
    this->enable();
    this->transfer_byte(addr | SPI_READ_BURST);
    uint8_t val = this->transfer_byte(0x00);
    this->disable();
    return val;
  }

  void write_burst(uint8_t addr, const uint8_t *buf, uint8_t len) {
    this->enable();
    this->transfer_byte(addr | SPI_WRITE_BURST);
    for (uint8_t i = 0; i < len; i++) this->transfer_byte(buf[i]);
    this->disable();
  }

  void strobe(uint8_t cmd) {
    this->enable();
    this->transfer_byte(cmd);
    this->disable();
  }

  // --- CC1101 init helpers ---

  void reset() {
    // Per CC1101 datasheet section 11.3: CS toggle to trigger POR, then SRES
    this->enable();
    delayMicroseconds(10);
    this->disable();
    delayMicroseconds(40);
    this->enable();
    delay(1);  // wait for chip ready (replaces MISO polling)
    this->transfer_byte(S_SRES);
    this->disable();
    delay(1);
  }

  // Frequency calibration for 300-348 MHz band (our 304 MHz is here).
  // Ported from SmartRC_CC1101::Calibrate().
  void calibrate() {
    if (freq_mhz_ >= 300.0f && freq_mhz_ <= 348.0f) {
      uint8_t cal = (uint8_t)(24 + (freq_mhz_ - 300.0f) * (28 - 24) / (348.0f - 300.0f));
      write_reg(R_FSCTRL0, cal);
      if (freq_mhz_ < 322.88f) {
        write_reg(R_TEST0, 0x0B);
      } else {
        write_reg(R_TEST0, 0x09);
        uint8_t s = read_status(R_FSCAL2);
        if (s < 32) write_reg(R_FSCAL2, s + 32);
      }
    }
  }

 public:
  CC1101(int gdo0, float bandwidth_khz, float freq_mhz)
      : gdo0_(gdo0), bandwidth_khz_(bandwidth_khz), freq_mhz_(freq_mhz) {}

  void setup() override {
    this->spi_setup();
    pinMode(gdo0_, INPUT);

    reset();

    // ASK/OOK async serial mode — register values from SmartRC RegConfigSettings()
    // + setCCMode(false) + setModulation(OOK)
    write_reg(R_FSCTRL1,  0x06);
    write_reg(R_IOCFG2,   0x0D);  // GDO2: async serial data output
    write_reg(R_IOCFG0,   0x0D);  // GDO0: async serial data output
    write_reg(R_PKTCTRL0, 0x32);  // async serial mode, no whitening
    write_reg(R_MDMCFG3,  0x93);
    write_reg(R_MDMCFG4,  0x07);
    write_reg(R_MDMCFG2,  0x30);  // OOK, no sync word
    write_reg(R_FREND0,   0x11);  // OOK PA setting
    write_reg(R_MDMCFG1,  0x02);
    write_reg(R_MDMCFG0,  0xF8);
    write_reg(R_CHANNR,   0x00);
    write_reg(R_DEVIATN,  0x47);
    write_reg(R_FREND1,   0x56);
    write_reg(R_MCSM0,    0x18);
    write_reg(R_FOCCFG,   0x16);
    write_reg(R_BSCFG,    0x1C);
    write_reg(R_AGCCTRL2, 0xC7);
    write_reg(R_AGCCTRL1, 0x00);
    write_reg(R_AGCCTRL0, 0xB2);
    write_reg(R_FSCAL3,   0xE9);
    write_reg(R_FSCAL2,   0x2A);
    write_reg(R_FSCAL1,   0x00);
    write_reg(R_FSCAL0,   0x1F);
    write_reg(R_FSTEST,   0x59);
    write_reg(R_TEST2,    0x81);
    write_reg(R_TEST1,    0x35);
    write_reg(R_TEST0,    0x09);
    write_reg(R_PKTCTRL1, 0x04);
    write_reg(R_ADDR,     0x00);
    write_reg(R_PKTLEN,   0x00);

    // PA table: OOK at 300-348 MHz → ~12 dBm (PA_TABLE_315[7] = 0xC2)
    // OOK uses index 0 for off, index 1 for on
    static const uint8_t pa[8] = {0x00, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    write_burst(R_PATABLE, pa, 8);

    set_rx_bw(bandwidth_khz_);
    set_mhz(freq_mhz_);
    set_rx();
  }

  void set_mhz(float mhz) {
    freq_mhz_ = mhz;
    uint32_t w = (uint32_t)(mhz * 65536.0f / (F_OSC / 1000000.0f));
    write_reg(R_FREQ2, (w >> 16) & 0xFF);
    write_reg(R_FREQ1, (w >> 8)  & 0xFF);
    write_reg(R_FREQ0,  w        & 0xFF);
    calibrate();
  }

  void set_rx_bw(float bw_khz) {
    // Ported from SmartRC_CC1101::setRxBW()
    float ratio = F_OSC / (8.0f * bw_khz * 1000.0f);
    int e = (int)(logf(ratio / 4.0f) / logf(2.0f));
    if (e < 0) e = 0; if (e > 3) e = 3;
    int m = (int)roundf(ratio / powf(2.0f, (float)e) - 4.0f);
    if (m < 0) m = 0; if (m > 3) m = 3;
    uint8_t cur = read_status(R_MDMCFG4) & 0x0F;
    write_reg(R_MDMCFG4, ((uint8_t)e << 6) | ((uint8_t)m << 4) | cur);
  }

  void set_rx() {
    strobe(S_SIDLE);
    strobe(S_SRX);
  }

  void set_tx() {
    strobe(S_SIDLE);
    strobe(S_STX);
  }

  // Called from remote_transmitter on_transmit
  void beginTransmission() {
    set_tx();
    pinMode(gdo0_, OUTPUT);
  }

  // Called from remote_transmitter on_complete
  void endTransmission() {
    digitalWrite(gdo0_, LOW);
    pinMode(gdo0_, INPUT);
    set_rx();
    set_rx();  // twice — CC1101 quirk, see dev-diary.md
  }
};

}  // namespace cc1101_smartrc
}  // namespace esphome
