#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>                 // Required for I2C (BME280)
#include <Adafruit_BME280.h>      // BME280 sensor library
#include <SPI.h>                  // Required for SPI (TFT Display)
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>              // Required for DS18B20
#include <DallasTemperature.h>    // Required for DS18B20

// ==========================================
// WI-FI CONFIGURATION
// ==========================================
const char* ssid     = "RASPBERRYNET";     
const char* password = "VerySecret"; 

// NTP Time setup specifically for Denmark
WiFiUDP ntpUDP;
time_t timeOffset = 7200; // UTC+2 (Denmark Summer Time)
NTPClient timeClient(ntpUDP, "pool.ntp.org", timeOffset); 

// ==========================================
// LED STRIP CONFIGURATION
// ==========================================
#define LED_PIN     32
#define NUM_LEDS     60
#define LED_TYPE    WS2813
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// SENSORS CONFIGURATION
// ==========================================
Adafruit_BME280 bme; 

#define ONE_WIRE_BUS 5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);

// HC-SR04 (Water Level)
const int trigPin = 27; 
const int echoPin = 14; 

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
// SYSTEM PINS (RELAYS & BUTTON)
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Relay 1: Grow Light
const int relay2Pin = 26;      // Relay 2: Water Pump
const int buttonPin = 33;      // Manual top-up button

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;

// CALIBRATION VARIABLES (11-Liter Tank)
int waterDistance = 0; 
int waterPercent = 0; 

String lightStatus = "OFF";
String waterPumpStatus = "OFF";
String airPumpStatus = "ON ";  // Manual 230V Pump in 11L tank

// Function to refresh the information on the TFT screen
void updateDisplay() {
  tft.setTextSize(2);
  
  // 1. Time
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Time: ");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getHours());
  tft.print(":");
  int mins = timeClient.getMinutes();
  if(mins < 10) tft.print("0"); 
  tft.print(mins);
  tft.print("   "); 
  
  // 2. Air Climate
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 40);
  tft.print("Air T: "); tft.print(airTemp, 1); tft.print(" C "); 
  tft.setCursor(10, 65);
  tft.print("Air H: "); tft.print(airHum, 1); tft.print(" % ");

  // 3. Water Temperature
  tft.setCursor(10, 95);
  tft.print("Wat T: ");
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  if (waterTemp == -127.00) tft.print("ERR  "); 
  else { tft.print(waterTemp, 1); tft.print(" C "); }

  // 4. Water Level (%)
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 120);
  tft.print("Level: ");
  
  if (waterPercent <= 10) tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(waterPercent); tft.print("%    ");

  // Critical Low Water Warning
  tft.setCursor(10, 138);
  if (waterPercent <= 10) {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print("! LOW WATER !");
  } else {
    tft.print("             "); // Clear warning line if OK
  }

  // 5. System Statuses
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  
  // Light
  tft.setCursor(10, 160);
  tft.print("Light:  ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  // Water Pump
  tft.setCursor(10, 185);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("W-Pump: ");
  if (waterPumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(waterPumpStatus);

  // Air Pump (Always ON status visual)
  tft.setCursor(10, 210);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("A-Pump: ");
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.print(airPumpStatus);
}

void setup() {
  Serial.begin(115200); 

  tft.begin();
  tft.setRotation(1); 
  tft.fillScreen(ILI9341_BLACK); 
  
  bme.begin(0x76);
  waterSensor.begin();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 30);
  tft.print("Connecting Wi-Fi...");
  
  WiFi.begin(ssid, password);
  int connectionTimeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    connectionTimeout++;
    if(connectionTimeout > 20) break; 
  }
  tft.fillScreen(ILI9341_BLACK);
  
  timeClient.begin();
  
  // Initialize Relays (ACTIVE LOW LOGIC)
  digitalWrite(relay1Pin, HIGH); // Grow Light OFF
  pinMode(relay1Pin, OUTPUT);
  
  digitalWrite(relay2Pin, HIGH); // Water Pump OFF 
  pinMode(relay2Pin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP); 

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  updateDisplay(); 
}

void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();

  // ==========================================
  // PART 1: SENSOR READINGS (Every 2 seconds)
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    
    airTemp = bme.readTemperature();
    airHum = bme.readHumidity();

    waterSensor.requestTemperatures(); 
    waterTemp = waterSensor.getTempCByIndex(0);

    // Ultrasonic pulse
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH);
    waterDistance = duration * 0.034 / 2; 
    
    // Convert cm to percentage based on calibration:
    // 10 cm or more = 0% (Critical Low)
    // 5 cm or less = 100% (Tank Full)
    waterPercent = map(waterDistance, 10, 5, 0, 100);
    if (waterPercent > 100) waterPercent = 100;
    if (waterPercent < 0) waterPercent = 0;
    
    updateDisplay(); 
  }

  // ==========================================
  // PART 2: SYSTEM LOGIC
  // ==========================================
  
  // 1. Grow Light Logic
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    if (lightStatus != "ON ") { 
      digitalWrite(relay1Pin, LOW);            // Turn ON Relay 1
      delay(150);                              // Wait for strip power stabilization
      fill_solid(leds, NUM_LEDS, CRGB::White); 
      FastLED.show();
      lightStatus = "ON "; 
    }
  } else {
    if (lightStatus != "OFF") {
      fill_solid(leds, NUM_LEDS, CRGB::Black); 
      FastLED.show();
      delay(50);                               
      digitalWrite(relay1Pin, HIGH);           // Turn OFF Relay 1
      lightStatus = "OFF";
    }
  }
  if (oldLightStatus != lightStatus) updateDisplay();

  // 2 & 3. Water Pump Logic (Auto-Top-Off + Manual Button)
  int buttonState = digitalRead(buttonPin); 
  String oldPumpStatus = waterPumpStatus;

  if (buttonState == LOW) {
    // MANUAL OVERRIDE: Pump runs as long as you hold the physical button
    digitalWrite(relay2Pin, LOW);   // Turn ON pump
    waterPumpStatus = "ON ";
  } else {
    // AUTOMATIC SENSOR-BASED MODE (Hysteresis Corridor)
    if (waterDistance >= 10) {      
      // Distance is 10cm or more (Water depth is <= 4cm -> 0%)
      digitalWrite(relay2Pin, LOW); // Turn ON pump to start filling
      waterPumpStatus = "ON ";
    } 
    else if (waterDistance <= 5) {  
      // Distance is 5cm or less (Water depth is >= 9cm -> 100%)
      digitalWrite(relay2Pin, HIGH); // Turn OFF pump, container is full
      waterPumpStatus = "OFF";
    }
    // If distance is between 5cm and 10cm, it safely maintains its current state
  }
  
  if (oldPumpStatus != waterPumpStatus) updateDisplay();

  delay(50); 
}