#include "screens.h"

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void initDisplay() {
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.display();
}

void drawStartupScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ESP32 Osiloskop");
  display.println("Baslatiliyor...");
  display.display();
  delay(800);
}

void drawErrorScreen(String message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("HATA:");
  display.println(message);
  display.display();
}

static void drawGrid() {
  for (int x = 0; x < SCREEN_WIDTH; x += 16) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      if (y % 4 == 0) display.drawPixel(x, y, SH110X_WHITE);
    }
  }
  for (int y = 0; y < SCREEN_HEIGHT; y += 16) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      if (x % 4 == 0) display.drawPixel(x, y, SH110X_WHITE);
    }
  }
}

// Ham ADC (0-4095) degerini gerilime cevirir
static inline float rawToVoltage(uint16_t raw) {
  return ((float)raw / ADC_MAX_RAW) * ADC_VREF;
}

// Gerilimi, secili volt/div ve merkez gerilime gore ekran Y pikseline cevirir
static int voltageToPixelY(float voltage, float voltsPerDiv, float centerVoltage) {
  float fullScale = voltsPerDiv * NUM_V_DIVISIONS; // ekranin tamamina denk gelen gerilim araligi
  float minV = centerVoltage - fullScale / 2.0f;
  float maxV = centerVoltage + fullScale / 2.0f;
  int y = (int)((maxV - voltage) / (maxV - minV) * (SCREEN_HEIGHT - 1));
  // Ekran disina cikan degerleri sinira sabitle (clipping)
  if (y < 0) y = 0;
  if (y > SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 1;
  return y;
}

void drawScopeFrame(uint16_t *samples, int triggerLevel,
                     uint32_t sampleRateHz, float freqHz,
                     float voltsPerDiv, float centerVoltage) {
  display.clearDisplay();
  drawGrid();

  // Dalga formu + Vpp icin min/max takibi
  float minV = 99.0f, maxV = -99.0f;
  int prevX = -1, prevY = -1;
  for (int x = 0; x < PLOT_WIDTH; x++) {
    float v = rawToVoltage(samples[x]);
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
    int y = voltageToPixelY(v, voltsPerDiv, centerVoltage);
    if (prevX >= 0) {
      display.drawLine(prevX, prevY, x, y, SH110X_WHITE);
    }
    prevX = x;
    prevY = y;
  }
  float vpp = maxV - minV;

  // Trigger cizgisi (kesikli) - trigger de voltaja cevrilip ayni olcekle cizilir
  float trigVoltage = rawToVoltage(triggerLevel);
  int trigY = voltageToPixelY(trigVoltage, voltsPerDiv, centerVoltage);
  for (int x = 0; x < SCREEN_WIDTH; x += 3) {
    display.drawPixel(x, trigY, SH110X_WHITE);
  }

  // Ust bilgi satirlari - okunabilirlik icin arkalarina siyah dolgu
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_BLACK);
  display.setCursor(0, 0);
  char line1[32];
  snprintf(line1, sizeof(line1), "%luHz f=%.0fHz",
           (unsigned long)sampleRateHz, freqHz);
  display.print(line1);

  display.setCursor(0, 8);
  char line2[32];
  snprintf(line2, sizeof(line2), "%.2fV/div Vpp=%.2fV", voltsPerDiv, vpp);
  display.print(line2);

  display.display();
}