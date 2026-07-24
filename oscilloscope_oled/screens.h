#ifndef SCREENS_H
#define SCREENS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_ADDR      0x3C
#define PLOT_WIDTH     128   // dalga formu icin ekrana cizilecek nokta sayisi

#define ADC_VREF        3.3f  // ESP32 ADC referans gerilimi (yaklasik, kalibrasyonsuz)
#define ADC_MAX_RAW     4095  // 12-bit ADC max deger
#define NUM_V_DIVISIONS 4     // dikey grid bolme sayisi (64px / 16px = 4)

extern Adafruit_SH1106G display;

// Ekran / display yasam dongusu
void initDisplay();
void drawStartupScreen();
void drawErrorScreen(String message);

// Osiloskop cizimi
// samples       : ADC degerleri (0-4095 arasi), en az PLOT_WIDTH eleman
// triggerLevel  : 0-4095 arasi trigger seviyesi (yatay cizgi olarak gosterilir)
// sampleRateHz  : o anki ornekleme hizi (bilgi satirinda gosterilir)
// freqHz        : tahmini sinyal frekansi (bilgi satirinda gosterilir)
// voltsPerDiv   : dikey olcek (V/div) - ekrandaki yakinlastirma seviyesi
// centerVoltage : ekranin dikey ortasina denk gelen gerilim (V)
void drawScopeFrame(uint16_t *samples, int triggerLevel,
                     uint32_t sampleRateHz, float freqHz,
                     float voltsPerDiv, float centerVoltage);

#endif