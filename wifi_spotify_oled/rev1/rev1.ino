#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "screens.h"

const char* ssid     = "SUPERONLINE_WiFi_75A7";
const char* password = "5455443877.m";

WebServer server(80);

String currentArtist   = "";
String currentTitle    = "";
int    currentPosition = 0;
int    currentDuration = 0;
bool   currentPlaying  = false;
bool   hasTrack        = false;

unsigned long lastRefresh = 0;
const unsigned long REFRESH_INTERVAL_MS = 40; 

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
  currentPosition = doc["position"] | 0;
  currentDuration = doc["duration"] | 0;
  currentPlaying  = doc["playing"]  | true;
  hasTrack = true;

  drawTrackScreen(currentArtist, currentTitle, currentPosition, currentDuration, currentPlaying);
  server.send(200, "text/plain", "OK");
}

// Hex string gelen 576 karakterlik resmi 288 byte binary'ye dönüştürür
void handleArt() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  String hexData = server.arg("plain");
  // 288 byte = 576 hex karakteri
  if (hexData.length() != 576) {
    Serial.printf("Art Read Error! Expected length 576, got: %d\n", hexData.length());
    server.send(400, "text/plain", "Invalid Hex Data Length");
    return;
  }

  // Hex metnini Byte dizisine çevir
  for (int i = 0; i < 288; i++) {
    char highNibble = hexData.charAt(i * 2);
    char lowNibble  = hexData.charAt(i * 2 + 1);

    auto hexToNibble = [](char c) -> uint8_t {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return 0;
    };

    albumArtBuffer[i] = (hexToNibble(highNibble) << 4) | hexToNibble(lowNibble);
  }

  hasAlbumArt = true;
  server.send(200, "text/plain", "Art Updated");
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
  server.on("/art", HTTP_POST, handleArt);
  server.begin();
}

void loop() {
  server.handleClient();

  if (hasTrack) {
    unsigned long now = millis();
    if (now - lastRefresh >= REFRESH_INTERVAL_MS) {
      lastRefresh = now;
      drawTrackScreen(currentArtist, currentTitle, currentPosition, currentDuration, currentPlaying);
    }
  }
}