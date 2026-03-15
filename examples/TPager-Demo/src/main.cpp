#include <Arduino.h>
extern "C" {
  #include "sic/sic.h"
  #include "sic/sic_board.h"
}

// T-Pager has no battery ADC on reference hardware.
// sic_battery_read() returns SIC_ENOENT in that case — no special handling needed.

static void sys_info() {
  sic_sysinfo_t si{};
  if (sic_sysinfo(&si) == 0) {
    char mac[18];
    snprintf(mac, sizeof mac, "%02X:%02X:%02X:%02X:%02X:%02X",
      si.mac[0], si.mac[1], si.mac[2], si.mac[3], si.mac[4], si.mac[5]);
    Serial.printf("[SYS] model=%s rev=%u CPU=%uMHz flash=%uB psram=%uB mac=%s\n",
      si.chip_model, si.chip_rev, si.cpu_mhz, si.flash_bytes, si.psram_bytes, mac);
  }
}

static void battery_info() {
  sic_battery_t bat{};
  int rc = sic_battery_read(&bat);
  if (rc == SIC_ENOENT) {
    Serial.println("[BAT] no ADC (MCU-only or not wired)");
  } else if (rc == 0) {
    Serial.printf("[BAT] %.3f V  %d%%\n", bat.voltage_v, bat.percent);
  } else {
    Serial.printf("[BAT] read error %d\n", rc);
  }
}

static void i2c_scan() {
#if defined(I2C_SDA_PIN) && defined(I2C_SCL_PIN)
  Serial.printf("[I2C] scanning SDA=%d SCL=%d...\n", I2C_SDA_PIN, I2C_SCL_PIN);
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("  0x%02X\n", a); found++; }
  }
  if (!found) Serial.println("[I2C] none");
#else
  Serial.println("[I2C] I2C_SDA_PIN/I2C_SCL_PIN not defined — skipping scan");
#endif
}

void setup() {
  delay(1500);
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 2000) delay(10);

#if defined(I2C_SDA_PIN) && defined(I2C_SCL_PIN)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#endif

  sic_begin_opts_t opts{ .init_buses = 1, .lazy_drivers = 0 };
  int rc = sic_begin(&SIC_BOARD_TPAGER, &opts);
  Serial.printf("[SIC] sic_begin rc=%d\n", rc);

  sys_info();
  battery_info();

  Serial.println("[KEYS] 'i'=i2c scan  'b'=battery  's'=sys info");
}

void loop() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    switch (c) {
      case 'i': i2c_scan();    break;
      case 'b': battery_info(); break;
      case 's': sys_info();    break;
      default: break;
    }
  }
  sic_delay_ms(10);
}
