
#include <Arduino.h>
extern "C" {
#include "sic/power/battery.h"
#include "sic/sic.h"
}

#ifndef BAT_ADC_PIN
#define BAT_ADC_PIN 10
#endif
#ifndef BAT_DIV_K
#define BAT_DIV_K 2.0f
#endif

static int percent_from_voltage(float v){
  // crude mapping 3.3V=0%, 4.2V=100%
  if (v <= 3.30f) return 0;
  if (v >= 4.20f) return 100;
  return (int)((v - 3.30f) * (100.0f / (4.20f - 3.30f)));
}

int sic_battery_read(sic_battery_t* out){
  if (!out) return -1;
  analogReadResolution(12);
  float mv = (float)analogRead(BAT_ADC_PIN) * (3300.0f/4095.0f) * BAT_DIV_K;
  out->voltage_v = mv / 1000.0f;
  out->percent = percent_from_voltage(out->voltage_v);
  return 0;
}
