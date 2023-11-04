/*
 * Purpose:  this is a simple barometric display using the neopixel ring.  It is designed
 * to work with the weatherflow tempest json data feed
 * 
 * written by:  John Rogers
 * john at wizworks dot net
 * 
 * permission is granted for commercial and non-commercial use as long as this 
 * attribution is left intact.
 * 
 * 
 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

bool firstRun = true;
String devHostname = "WX-PYRAMID";

// Replace with the URL to the JSON data (HTTPS)
const char* jsonUrl = "https://your.tempest.weather.apifeed.json"; // Updated URL

// Replace with your NeoPixel configuration
#define NEOPIXEL_PIN 4  // Pin connected to the NeoPixel ring  (GPIO4)
#define NUM_PIXELS 16    // Number of NeoPixels in the ring
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 120000;  //  minutes in milliseconds

void setup() {
  Serial.begin(115200);
  delay(10);
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'

  // Set the initial color (e.g., red)
  strip.fill(strip.Color(2, 0, 4));   // purple while waiting for wifi connection
  strip.show();

  WiFiManager wifiManager;
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting WiFi ");
  WiFi.hostname(devHostname.c_str());

  if (!wifiManager.autoConnect("Weather Pyramid")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.print("MyIP: ");
  Serial.println(WiFi.localIP());
}
  
void loop() {
  unsigned long currentMillis = millis();

  // Check if it's the first run or 5 minutes have passed
  if (firstRun || (currentMillis - lastUpdateTime >= updateInterval)) {
    lastUpdateTime = currentMillis;
    changeNeoPixelColor();

    // Set the firstRun flag to false after the initial run
    if (firstRun) {
      firstRun = false;
    }
  }
}

void changeNeoPixelColor() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure(); // Ignore SSL certificate verification (not recommended in a production environment)

    http.begin(client, jsonUrl);

    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      http.end();

      // Find the position of "pressure_trend" in the JSON payload
      int trendPos = payload.indexOf("\"pressure_trend\":\"");
      if (trendPos != -1) {
        trendPos += 18; // Move the position to the start of the trend value
        int trendEnd = payload.indexOf("\"", trendPos); // Find the end of the trend value
        if (trendEnd != -1) {
          String pressureTrend = payload.substring(trendPos, trendEnd);
          Serial.print("Pressure Trend: ");
          Serial.println(pressureTrend);

          // Now you can use the pressureTrend value in your decision logic
          if (pressureTrend == "falling") {
            strip.fill(strip.Color(0, 0, 160)); // Blue
          } else if (pressureTrend == "rising") {
            strip.fill(strip.Color(160, 160, 0)); // Yellow
          } else if (pressureTrend == "steady") {
            strip.fill(strip.Color(0, 160, 0)); // Green
          } else {
            strip.fill(strip.Color(190, 0, 0)); // Default color for an unknown trend (Red)
          }

          strip.show();
        }
      } else {
        Serial.println("Failed to find 'pressure_trend' in JSON");
      }
    } else {
      Serial.println("Failed to connect to JSON server");
    }
  } else {
    Serial.println("WiFi not connected");
  }
}
