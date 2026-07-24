/*
 * ESP32 + SSD1306 OLED Mini Osiloskop
 * -----------------------------------
 * Dosya yapisi:
 *   osiloskop.ino  -> ornekleme (ADC ISR), trigger mantigi
 *   screens.h/.cpp -> tum ekran cizim islevleri
 *   buttons.h/.cpp -> Btn struct + debounce (Arduino IDE'nin otomatik
 *                      prototip olusturucusu struct'lari .ino icinde
 *                      bazen yanlis isliyor, bu yuzden ayri dosyada)
 *
 * Donanim:
 *   - ADC girisi : GPIO34 (ADC1_CH6, sadece giris - input only pin)
 *   - OLED       : I2C, SDA=GPIO21, SCL=GPIO22, 128x64, adres 0x3C
 *   - Butonlar   : (opsiyonel) time/div, trigger seviyesi, volt/div ayari
 *       BTN_TIME_UP   = GPIO32
 *       BTN_TIME_DOWN = GPIO33
 *       BTN_TRIG_UP   = GPIO25
 *       BTN_TRIG_DOWN = GPIO26
 *       BTN_VOLT_UP   = GPIO27
 *       BTN_VOLT_DOWN = GPIO14
 *
 * ONEMLI: ESP32 ADC girisi 0-3.3V ile sinirlidir. Bu araliktan buyuk
 * sinyaller icin mutlaka bir voltaj bolucu / attenuator devresi kullanin.
 * Dogrudan yuksek voltaj / AC sebeke baglamayin.
 *
 * ADC1 kanallari kullanildi cunku WiFi acikken ADC2 guvenilmez calisir.
 */

#include "screens.h"
#include "buttons.h"
#include <driver/adc.h>

// ---------------- Pin / kanal tanimlari ----------------
#define ADC_PIN        34
#define ADC_CHANNEL    ADC1_CHANNEL_6   // GPIO34

#define BTN_TIME_UP    32
#define BTN_TIME_DOWN  33

#define BTN_TRIG_UP    25
#define BTN_TRIG_DOWN  26

#define BTN_VOLT_UP    27
#define BTN_VOLT_DOWN  14

#define BUFFER_SIZE    256   // dairesel ornek tamponu

// ---------------- Ornekleme (ISR) ----------------
volatile uint16_t sampleBuffer[BUFFER_SIZE];
volatile uint32_t writeIndex = 0;
hw_timer_t *samplingTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

const uint32_t sampleRates[] = {1000, 2000, 5000, 10000, 20000, 50000};
const uint8_t  numRates = sizeof(sampleRates) / sizeof(sampleRates[0]);
volatile uint8_t rateIndex = 3; // baslangic: 10 kHz

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  int raw = adc1_get_raw(ADC_CHANNEL);
  sampleBuffer[writeIndex] = (uint16_t)raw;
  writeIndex = (writeIndex + 1) % BUFFER_SIZE;
  portEXIT_CRITICAL_ISR(&timerMux);
}

// NOT: ESP32 Arduino core 3.x ile timer API degisti.
// timerBegin artik dogrudan frekans (Hz) aliyor, alarm ayari timerAlarm() ile tek adimda yapiliyor.
void setupTimer(uint32_t rateHz) {
  if (samplingTimer) {
    timerEnd(samplingTimer);
  }
  samplingTimer = timerBegin(1000000);           // 1 MHz taban saat -> 1 tick = 1us
  timerAttachInterrupt(samplingTimer, &onTimer);  // core 3.x'te ISR arg. yok
  uint32_t periodUs = 1000000UL / rateHz;
  timerAlarm(samplingTimer, periodUs, true, 0);   // (timer, alarm_value, autoreload, reload_count)
}

// ---------------- Trigger seviyesi ----------------
volatile int triggerLevel = 2048; // 12-bit ADC orta nokta (0-4095)
const int triggerStep = 100;

// ---------------- Volt/div ayari ----------------
// NOT: Bu yazilimsal bir yakinlastirma - ESP32 girisi hala sabit 0-3.3V ile
// sinirli. Gercekten daha kucuk sinyalleri (ornegin 10mV/div) hassas olcmek
// icin harici bir on-yukselteç (PGA) / kazanc devresi gerekir.
const float voltDivOptions[] = {0.1f, 0.2f, 0.5f, 1.0f}; // V/div (4 dikey bolme ile carpiliyor)
const uint8_t numVoltDivs = sizeof(voltDivOptions) / sizeof(voltDivOptions[0]);
volatile uint8_t voltDivIndex = 3; // baslangic: 1.0V/div -> 4V tam skala (3.3V'a kirpilir)
const float centerVoltage = ADC_VREF / 2.0f; // ekran dikey ortasi = 1.65V (ADC orta nokta)

// ---------------- Buton tanimlari (Btn struct + buttonPressed -> buttons.h/.cpp) ----------------
Btn btns[6] = {
  {BTN_TIME_UP, HIGH, 0},
  {BTN_TIME_DOWN, HIGH, 0},
  {BTN_TRIG_UP, HIGH, 0},
  {BTN_TRIG_DOWN, HIGH, 0},
  {BTN_VOLT_UP, HIGH, 0},
  {BTN_VOLT_DOWN, HIGH, 0}
};

// ---------------- Frekans tahmini ----------------
float estimateFrequency(uint16_t *buf, int len, uint32_t sampleRateHz) {
  int crossings = 0;
  int firstCross = -1, lastCross = -1;
  for (int i = 1; i < len; i++) {
    if (buf[i - 1] < triggerLevel && buf[i] >= triggerLevel) {
      if (firstCross == -1) firstCross = i;
      lastCross = i;
      crossings++;
    }
  }
  if (crossings < 2) return 0.0f;
  float avgPeriodSamples = (float)(lastCross - firstCross) / (crossings - 1);
  if (avgPeriodSamples <= 0) return 0.0f;
  return (float)sampleRateHz / avgPeriodSamples;
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(BTN_TIME_UP, INPUT_PULLUP);
  pinMode(BTN_TIME_DOWN, INPUT_PULLUP);
  pinMode(BTN_TRIG_UP, INPUT_PULLUP);
  pinMode(BTN_TRIG_DOWN, INPUT_PULLUP);
  pinMode(BTN_VOLT_UP, INPUT_PULLUP);
  pinMode(BTN_VOLT_DOWN, INPUT_PULLUP);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_11); // ~0-3.3V arali

  initDisplay();
  drawStartupScreen();

  setupTimer(sampleRates[rateIndex]);
}

// ---------------- Loop ----------------
void loop() {
  // --- Buton kontrolleri ---
  if (buttonPressed(btns[0])) { // time up -> daha yuksek sample rate
    if (rateIndex < numRates - 1) rateIndex++;
    setupTimer(sampleRates[rateIndex]);
  }
  if (buttonPressed(btns[1])) { // time down
    if (rateIndex > 0) rateIndex--;
    setupTimer(sampleRates[rateIndex]);
  }
  if (buttonPressed(btns[2])) { // trigger up
    triggerLevel = min(4095, triggerLevel + triggerStep);
  }
  if (buttonPressed(btns[3])) { // trigger down
    triggerLevel = max(0, triggerLevel - triggerStep);
  }
  if (buttonPressed(btns[4])) { // volt/div - daha genis olcek (uzaklas)
    if (voltDivIndex < numVoltDivs - 1) voltDivIndex++;
  }
  if (buttonPressed(btns[5])) { // volt/div - daha dar olcek (yakinlas)
    if (voltDivIndex > 0) voltDivIndex--;
  }

  // --- Tampondan guvenli kopya al ---
  uint16_t snapshot[BUFFER_SIZE];
  portENTER_CRITICAL(&timerMux);
  uint32_t wIdx = writeIndex;
  memcpy((void *)snapshot, (const void *)sampleBuffer, sizeof(sampleBuffer));
  portEXIT_CRITICAL(&timerMux);

  // Dairesel tamponu dogrusal hale getir (en eski -> en yeni)
  uint16_t linear[BUFFER_SIZE];
  for (int i = 0; i < BUFFER_SIZE; i++) {
    linear[i] = snapshot[(wIdx + i) % BUFFER_SIZE];
  }

  // --- Trigger noktasi ara (rising edge), yoksa free-run ---
  int startIdx = 0;
  bool found = false;
  int searchLimit = BUFFER_SIZE - PLOT_WIDTH;
  for (int i = 1; i < searchLimit; i++) {
    if (linear[i - 1] < triggerLevel && linear[i] >= triggerLevel) {
      startIdx = i;
      found = true;
      break;
    }
  }
  if (!found) {
    startIdx = BUFFER_SIZE - PLOT_WIDTH; // en son PLOT_WIDTH ornek
  }

  float freq = estimateFrequency(linear, BUFFER_SIZE, sampleRates[rateIndex]);

  // --- Ciz (tum cizim islemi screens.cpp icinde) ---
  drawScopeFrame(&linear[startIdx], triggerLevel, sampleRates[rateIndex], freq,
                 voltDivOptions[voltDivIndex], centerVoltage);
}