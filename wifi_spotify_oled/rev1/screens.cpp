#include "screens.h"

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);

uint8_t albumArtBuffer[288];
bool hasAlbumArt = false;

// ---- Yeni Layout Koordinatları (Kapak: Sol 48x48 | Bilgiler: Sağ x=60..122) ----
static const int ART_X = 8;
static const int ART_Y = 8;
static const int ART_W = 48;
static const int ART_H = 48;

// Sağ Panel Elemanları
static const int TITLE_X = 60,  TITLE_Y = 8,  TITLE_W = 62, TITLE_H = 10;
static const int ARTIST_X = 60, ARTIST_Y = 22, ARTIST_W = 62, ARTIST_H = 10;

// Progress Bar & Süre Metni
static const int BAR_X = 60, BAR_Y = 37, BAR_W = 62, BAR_H = 5;
static const int TIME_X = 60, TIME_Y = 47;

static const int SCROLL_STEP_PX   = 1;
static const unsigned long SCROLL_STEP_MS = 35;
static const unsigned long SCROLL_PAUSE_MS = 1000;
static const int SCROLL_GAP_PX = 12;

static GFXcanvas1 titleCanvas(TITLE_W, TITLE_H);
static GFXcanvas1 artistCanvas(ARTIST_W, ARTIST_H);

struct ScrollState {
  String lastText = "";
  int offset = 0;
  unsigned long lastStepTime = 0;
  unsigned long pauseUntil = 0;
};

static ScrollState titleScroll;
static ScrollState artistScroll;

static String formatTime(int totalSeconds) {
  if (totalSeconds < 0) totalSeconds = 0;
  int m = totalSeconds / 60;
  int s = totalSeconds % 60;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d:%02d", m, s);
  return String(buf);
}

static void drawScrollingLine(GFXcanvas1 &canvas, const String &text, ScrollState &st,
                               int destX, int destY) {
  int w = canvas.width();
  int h = canvas.height();

  if (text != st.lastText) {
    st.lastText = text;
    st.offset = 0;
    st.lastStepTime = millis();
    st.pauseUntil = millis() + SCROLL_PAUSE_MS;
  }

  canvas.fillScreen(SH110X_BLACK);
  canvas.setTextWrap(false);
  canvas.setTextSize(1);
  canvas.setTextColor(SH110X_WHITE);

  int16_t x1, y1;
  uint16_t textW, textH;
  canvas.getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);

  if (textW <= (uint16_t)w) {
    canvas.setCursor(0, 1);
    canvas.print(text);
  } else {
    unsigned long now = millis();
    if (now >= st.pauseUntil && now - st.lastStepTime >= SCROLL_STEP_MS) {
      st.lastStepTime = now;
      st.offset += SCROLL_STEP_PX;
      if (st.offset > (int)textW + SCROLL_GAP_PX) {
        st.offset = 0;
        st.pauseUntil = now + SCROLL_PAUSE_MS;
      }
    }
    canvas.setCursor(-st.offset, 1);
    canvas.print(text);
    canvas.setCursor(-st.offset + (int)textW + SCROLL_GAP_PX, 1);
    canvas.print(text);
  }

  display.drawBitmap(destX, destY, canvas.getBuffer(), w, h, SH110X_WHITE);
}

void initDisplay() {
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.display();
}

void drawWaitingScreen() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 28);
  display.println("Waiting for Conn...");
  display.display();
}

void drawTrackScreen(String artist, String title, int positionSec, int durationSec, bool isPlaying) {
  display.clearDisplay();

  // Dış çerçeve
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);

  // 1. SOL TARAF: Kapak Resmi (Yoksa varsayılan çerçeve çizer)
  if (hasAlbumArt) {
    display.drawBitmap(ART_X, ART_Y, albumArtBuffer, ART_W, ART_H, SH110X_WHITE);
  } else {
    display.drawRect(ART_X, ART_Y, ART_W, ART_H, SH110X_WHITE);
    display.setCursor(ART_X + 12, ART_Y + 20);
    display.print("NO ART");
  }

  // 2. SAĞ TARAF: Başlık ve Sanatçı (Marquee)
  drawScrollingLine(titleCanvas, title, titleScroll, TITLE_X, TITLE_Y);
  drawScrollingLine(artistCanvas, artist, artistScroll, ARTIST_X, ARTIST_Y);

  // 3. SAĞ TARAF: Progress Bar
  display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, SH110X_WHITE);
  if (durationSec > 0) {
    int fillW = (int)((float)positionSec / durationSec * (BAR_W - 2));
    if (fillW < 0) fillW = 0;
    if (fillW > BAR_W - 2) fillW = BAR_W - 2;
    display.fillRect(BAR_X + 1, BAR_Y + 1, fillW, BAR_H - 2, SH110X_WHITE);
  }

  // 4. SAĞ TARAF: Süre Metni (Örn: "01:20 / 03:45")
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(TIME_X, TIME_Y);
  // Yer dar olduğundan formatı dinamik/kısa tutuyoruz
  display.print(formatTime(positionSec));
  display.print("/");
  display.print(formatTime(durationSec));

  display.display();
}

void drawErrorScreen(String message) {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(6, 5);
  display.println("Hata");
  display.drawLine(4, 15, 124, 15, SH110X_WHITE);
  display.setCursor(6, 25);
  display.println(message.substring(0, 21));
  display.display();
}