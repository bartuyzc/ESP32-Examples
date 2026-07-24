#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "screens.h"

const char* ssid     = "SUPERONLINE_WiFi_75A7";
const char* password = "5455443877.m";

WebServer server(80);

// Su anki sarki bilgisini global olarak saklıyoruz ki kayan yazi
// animasyonu yeni bir HTTP istegi gelmeden de ilerleyebilsin
String currentArtist   = "";
String currentTitle    = "";
int    currentPosition = 0;
int    currentDuration = 0;
bool   currentPlaying  = false;
bool   hasTrack        = false;

unsigned long lastRefresh = 0;
const unsigned long REFRESH_INTERVAL_MS = 40; // ~25 fps, kayan yazi icin yeterli

void handleUpdate() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    server.send(400, "text/plain", "Bad JSON");
    return;
  }

  currentArtist   = doc["artist"].as<String>();
  currentTitle    = doc["title"].as<String>();
  currentPosition = doc["position"] | 0;     // saniye
  currentDuration = doc["duration"] | 0;     // saniye
  currentPlaying  = doc["playing"]  | true;
  hasTrack = true;

  drawTrackScreen(currentArtist, currentTitle, currentPosition, currentDuration, currentPlaying);
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  initDisplay();
  drawWaitingScreen();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
}

void loop() {
  server.handleClient();

  // Yeni veri gelmese bile ekrani duzenli araliklarla yeniden ciziyoruz,
  // boylece uzun sarki/sanatci isimleri kesintisiz kaymaya devam ediyor
  if (hasTrack) {
    unsigned long now = millis();
    if (now - lastRefresh >= REFRESH_INTERVAL_MS) {
      lastRefresh = now;
      drawTrackScreen(currentArtist, currentTitle, currentPosition, currentDuration, currentPlaying);
    }
  }
}