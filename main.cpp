#include <Arduino.h>
#include <FastLED.h>
#include <DHT.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// ==========================================
// WI-FI NETWORK CONFIGURATION
// ==========================================
const char* ssid     = "RASPBERRYNET";     // Put school Wi-Fi name here
const char* password = "VerySecret"; // Put school Wi-Fi password here

// NTP (Time Server) Settings
WiFiUDP ntpUDP;
// Time offset: 7200 seconds = UTC+2 (Summer time in Ukraine/Denmark)
// Change to 3600 for Winter time (UTC+1) if needed
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200); 

// ==========================================
// LED STRIP CONFIGURATION
// ==========================================
#define LED_PIN     32
#define NUM_LEDS     60
#define LED_TYPE    WS2813
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// DHT22 CONFIGURATION (CLIMATE)
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 2000; // Update sensor data every 2 seconds

// ==========================================
// TFT DISPLAY CONFIGURATION (SPI)
// ==========================================
#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// SYSTEM PINS
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Relay 1: Grow Light (Strip)
const int buttonPin = 33;      // Manual pump button
const int relay2Pin = 26;      // Relay 2: Water Pump

int threshold = 2000; 
float t = 0.0, h = 0.0;
String lightStatus = "OFF";
String pumpStatus = "OFF";

// Function to refresh all data on the TFT screen without screen flickering
void updateDisplay() {
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // 1. Display Current Time (Format 14:05)
  tft.setCursor(10, 10);
  tft.print("Time: ");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getHours());
  tft.print(":");
  int mins = timeClient.getMinutes();
  if(mins < 10) tft.print("0"); // Add leading zero for minutes
  tft.print(mins);
  tft.print("   "); // Clear potential pixel artifacts
  
  // 2. Display Climate Data
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 45);
  tft.print("Temp: ");
  if (isnan(t)) tft.print("--  "); else { tft.print(t, 1); tft.print(" C "); }
  
  tft.setCursor(10, 75);
  tft.print("Hum:  ");
  if (isnan(h)) tft.print("--  "); else { tft.print(h, 1); tft.print(" % "); }

  // 3. Display Light Status (Changes color dynamically)
  tft.setCursor(10, 120);
  tft.print("Light: ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  // 4. Display Pump Status
  tft.setCursor(10, 150);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("Pump:  ");
  if (pumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(pumpStatus);
}

void setup() {
  Serial.begin(115200); 

  // Initialize TFT Display
  tft.begin();
  tft.setRotation(1); // Set landscape mode
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  
  tft.setCursor(10, 30);
  tft.print("Connecting to Wi-Fi...");
  
  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  int connectionTimeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    connectionTimeout++;
    if(connectionTimeout > 20) { // Wrap text on screen if it takes too long
      tft.setCursor(10, 60); 
      connectionTimeout = 0; 
    }
  }
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 30);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Wi-Fi Connected!");
  delay(1000);

  // Start the NTP time client
  timeClient.begin();
  
  // Initialize DHT sensor
  dht.begin();
  
  // Configure light pins
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); // Light is OFF by default
  
  // Configure pump and button pins
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);  // Pump is OFF by default   

  // Initialize FastLED for addressable strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  tft.fillScreen(ILI9341_BLACK); 
  updateDisplay(); // Draw the initial interface layout
}

void loop() {
  // Always update time client from the internet
  timeClient.update();

  unsigned long currentMillis = millis();

  // ==========================================
  // PART 1: CLIMATE READING & SCREEN REFRESH
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    h = dht.readHumidity();
    t = dht.readTemperature();
    updateDisplay(); // Refresh screen data every 2 seconds
  }

  // ==========================================
  // PART 2: LIGHT SENSOR & STRIP LOGIC
  // ==========================================
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  // Presentation Mode: Activates strictly based on light sensor value
  if (lightValue > threshold) {
    if (lightStatus != "ON ") { // Prevents code from spamming FastLED commands
      digitalWrite(relay1Pin, LOW); // Turn ON 12V relay
      delay(300);                   // Essential startup pause for LED chips
      fill_solid(leds, NUM_LEDS, CRGB::White); 
      FastLED.show();
      lightStatus = "ON "; 
    }
  } else {
    if (lightStatus != "OFF") {
      fill_solid(leds, NUM_LEDS, CRGB::Black); 
      FastLED.show();
      delay(50); 
      digitalWrite(relay1Pin, HIGH); // Turn OFF 12V relay 
      lightStatus = "OFF";
    }
  }

  // If light status changed, update display immediately for instant visual feedback
  if (oldLightStatus != lightStatus) {
    updateDisplay();
  }

  // ==========================================
  // PART 3: MANUAL WATERING PUMP LOGIC
  // ==========================================
  int buttonState = digitalRead(buttonPin); 
  if (buttonState == LOW) {
    pumpStatus = "ON ";
    updateDisplay(); 
    
    digitalWrite(relay2Pin, HIGH);  // Turn ON pump relay
    delay(10000);                   // Keep it running for exactly 10 seconds
    digitalWrite(relay2Pin, LOW);   // Turn OFF pump relay
    
    pumpStatus = "OFF";
    updateDisplay(); 
  }

  delay(50); // Small stability delay
}