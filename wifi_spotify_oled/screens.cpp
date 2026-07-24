#include "screens.h"

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);

// ---- Kayan yazi (marquee) alanlari ----
// Baslik alani: ikonun sagindan (x=20) sag cerceveye kadar
static const int TITLE_X = 20, TITLE_Y = 19, TITLE_W = 104, TITLE_H = 10;
// Sanatci alani: sol cerceveden (x=6) sag cerceveye kadar
static const int ARTIST_X = 6, ARTIST_Y = 32, ARTIST_W = 118, ARTIST_H = 10;

static const int SCROLL_STEP_PX   = 1;    // her adimda kac piksel kayacak
static const unsigned long SCROLL_STEP_MS = 30;   // adimlar arasi minimum sure
static const unsigned long SCROLL_PAUSE_MS = 800; // basta/sonda bekleme suresi
static const int SCROLL_GAP_PX = 16; // iki kopya arasindaki bosluk

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

// Basit bir 8'lik nota ikonu: daire (nota kafasi) + govde + bayrak
static void drawNoteIcon(int x, int y) {
  display.fillCircle(x, y + 6, 3, SH110X_WHITE);
  display.drawLine(x + 3, y, x + 3, y + 6, SH110X_WHITE);
  display.drawLine(x + 3, y, x + 7, y + 2, SH110X_WHITE);
  display.drawLine(x + 3, y + 1, x + 7, y + 3, SH110X_WHITE);
}

// Duraklatildiginda gosterilecek iki cubuk ikonu
static void drawPauseIcon(int x, int y) {
  display.fillRect(x, y, 2, 8, SH110X_WHITE);
  display.fillRect(x + 4, y, 2, 8, SH110X_WHITE);
}

// Verilen metni kucuk bir canvas'a cizip, sigmiyorsa kaydirarak
// ana ekrana basar. Metin/durum "state" icinde saklanir; ayni state
// ile tekrar tekrar cagrildikca kayma animasyonu ilerler.
static void drawScrollingLine(GFXcanvas1 &canvas, const String &text, ScrollState &st,
                               int destX, int destY) {
  int w = canvas.width();
  int h = canvas.height();

  // Sarki/sanatci degisince kaydirmayi bastan baslat
  if (text != st.lastText) {
    st.lastText = text;
    st.offset = 0;
    st.lastStepTime = millis();
    st.pauseUntil = millis() + SCROLL_PAUSE_MS;
  }

  canvas.fillScreen(SH110X_BLACK);
  canvas.setTextWrap(false); // canvas kenarinda otomatik satir atlamasin
  canvas.setTextSize(1);
  canvas.setTextColor(SH110X_WHITE);

  int16_t x1, y1;
  uint16_t textW, textH;
  canvas.getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);

  if (textW <= (uint16_t)w) {
    // Alana sigiyor, kaydirmaya gerek yok
    canvas.setCursor(0, 1);
    canvas.print(text);
  } else {
    unsigned long now = millis();
    if (now >= st.pauseUntil && now - st.lastStepTime >= SCROLL_STEP_MS) {
      st.lastStepTime = now;
      st.offset += SCROLL_STEP_PX;
      if (st.offset > (int)textW + SCROLL_GAP_PX) {
        st.offset = 0;
        st.pauseUntil = now + SCROLL_PAUSE_MS; // basa donunce kisa bekleme
      }
    }
    // Ayni metnin iki kopyasini yan yana ciziyoruz ki kayarken
    // bosluk olmadan surekli akiyormus gibi gorunsun
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
  display.setCursor(20, 28);
  display.println("Waiting for Connection...");
  display.display();
}

void drawTrackScreen(String artist, String title, int positionSec, int durationSec, bool isPlaying) {
  display.clearDisplay();

  // Dis cerceve
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);

  // Baslik: "Now Playing" / "Paused"
  display.setTextSize(1);
  display.setCursor(6, 5);
  display.println(isPlaying ? "Now Playing" : "Paused");
  display.drawLine(4, 15, 124, 15, SH110X_WHITE);

  // Ikon (nota / duraklatma)
  if (isPlaying) drawNoteIcon(6, 20);
  else drawPauseIcon(8, 20);

  // Sarki adi (uzunsa kayar)
  drawScrollingLine(titleCanvas, title, titleScroll, TITLE_X, TITLE_Y);

  // Sanatci (uzunsa kayar)
  drawScrollingLine(artistCanvas, artist, artistScroll, ARTIST_X, ARTIST_Y);

  // Progress bar
  int barX = 6, barY = 48, barW = 100, barH = 6;
  display.drawRect(barX, barY, barW, barH, SH110X_WHITE);
  if (durationSec > 0) {
    int fillW = (int)((float)positionSec / durationSec * (barW - 2));
    if (fillW < 0) fillW = 0;
    if (fillW > barW - 2) fillW = barW - 2;
    display.fillRect(barX + 1, barY + 1, fillW, barH - 2, SH110X_WHITE);
  }

  // Sure metni (bar altinda, sag alt kose)
  display.setCursor(6, 56);
  display.print(formatTime(positionSec));
  display.print(" / ");
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
