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
#include <OneWire.h>            // Required for DS18B20
#include <DallasTemperature.h>  // Required for DS18B20

// ==========================================
// WI-FI CONFIGURATION
// ==========================================
const char* ssid     = "RASPBERRYNET";     // Put school Wi-Fi name here
const char* password = "VerySecret"; // Put school Wi-Fi password here

// NTP Time setup specifically for Denmark
WiFiUDP ntpUDP;
// Time offset: 7200 seconds = UTC+2 (Summer time / CEST in Denmark)
// Note: Change to 3600 for Winter time (UTC+1 / CET)
time_t timeOffset = 7200; 
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
// CLIMATE & WATER SENSORS CONFIGURATION
// ==========================================
// BME280 (I2C) - Uses GPIO21 (SDA) and GPIO22 (SCL) automatically
Adafruit_BME280 bme; 

// DS18B20 Water Temperature Sensor (One-Wire)
#define ONE_WIRE_BUS 5 // Yellow wire connected to GPIO 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);

// HC-SR04 Ultrasonic Distance Sensor
const int trigPin = 27; // Ping sender
const int echoPin = 14; // Ping listener

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
const int buttonPin = 33;      // Pump button
const int relay2Pin = 26;      // Relay 2: Water Pump

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;
int waterDistance = 0; // Stores distance in cm
String lightStatus = "OFF";
String pumpStatus = "OFF";

// Function to refresh the information on the TFT screen
void updateDisplay() {
  tft.setTextSize(2);
  
  // 1. Time (Denmark)
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Time: ");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getHours());
  tft.print(":");
  int mins = timeClient.getMinutes();
  if(mins < 10) tft.print("0"); 
  tft.print(mins);
  tft.print("   "); // Clear artifacts
  
  // 2. Room Climate (BME280)
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 40);
  tft.print("Air T: ");
  tft.print(airTemp, 1); 
  tft.print(" C "); 
  
  tft.setCursor(10, 70);
  tft.print("Air H: ");
  tft.print(airHum, 1); 
  tft.print(" % ");

  // 3. Water Stats (Temp & Level)
  tft.setCursor(10, 100);
  tft.print("Wat T: ");
  // Highlight water temperature in cyan
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  if (waterTemp == -127.00) {
     tft.print("ERR  "); 
  } else {
     tft.print(waterTemp, 1); 
     tft.print(" C ");
  }

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 130);
  tft.print("Level: ");
  // Highlight water level in green
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(waterDistance);
  tft.print(" cm  "); // Clear artifacts

  // 4. System Statuses
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 170);
  tft.print("Light: ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  tft.setCursor(10, 200);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("Pump:  ");
  if (pumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(pumpStatus);
}

void setup() {
  Serial.begin(115200); 

  // Initialize TFT screen
  tft.begin();
  tft.setRotation(1); 
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  
  // Initialize Climate Sensor (BME280)
  // Try 0x77 if 0x76 address fails
  bme.begin(0x76);

  // Initialize Water Sensor (DS18B20)
  waterSensor.begin();

  // Configure Water Level Sensor (HC-SR04)
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  tft.setCursor(10, 30);
  tft.print("Connecting to Wi-Fi...");
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  int connectionTimeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
    connectionTimeout++;
    if(connectionTimeout > 20) { tft.setCursor(10, 60); connectionTimeout = 0; }
  }
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 30);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Wi-Fi Connected!");
  delay(1000);

  // Start NTP time client
  timeClient.begin();
  
  // Configure relays and button
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); // Default OFF
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);  // Default OFF   

  // Configure FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  tft.fillScreen(ILI9341_BLACK); 
  updateDisplay(); 
}

void loop() {
  // Update time from internet
  timeClient.update();
  unsigned long currentMillis = millis();

  // ==========================================
  // PART 1: SENSOR READINGS
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    
    // Read BME280 (Air)
    airTemp = bme.readTemperature();
    airHum = bme.readHumidity();

    // Read DS18B20 (Water Temp)
    waterSensor.requestTemperatures(); 
    waterTemp = waterSensor.getTempCByIndex(0);

    // Read HC-SR04 (Water Level)
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);   // Send ping
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Read response pulse
    long duration = pulseIn(echoPin, HIGH);
    // Calculate distance based on speed of sound (343 m/s) and divide by 2
    waterDistance = duration * 0.034 / 2; 
    
    updateDisplay(); // Update screen every 2 seconds
  }

  // ==========================================
  // PART 2: SYSTEM LOGIC
  // ==========================================
  // Grow Light Logic
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    if (lightStatus != "ON ") { 
      digitalWrite(relay1Pin, LOW); // Turn on grow light
      delay(300);                   // Small delay for LED startup
      fill_solid(leds, NUM_LEDS, CRGB::White); 
      FastLED.show();
      lightStatus = "ON "; 
    }
  } else {
    if (lightStatus != "OFF") {
      fill_solid(leds, NUM_LEDS, CRGB::Black); 
      FastLED.show();
      delay(50); 
      digitalWrite(relay1Pin, HIGH); // Turn off grow light
      lightStatus = "OFF";
    }
  }

  // Instant display update for light feedback
  if (oldLightStatus != lightStatus) {
    updateDisplay();
  }

  // Manual Water Pump Logic (Button)
  int buttonState = digitalRead(buttonPin); 
  if (buttonState == LOW) {
    pumpStatus = "ON ";
    updateDisplay(); // Show status change instantly
    
    digitalWrite(relay2Pin, HIGH);  // Turn pump ON
    delay(10000);                   // Wait 10 seconds
    digitalWrite(relay2Pin, LOW);   // Turn pump OFF
    
    pumpStatus = "OFF";
    updateDisplay(); 
  }

  delay(50); // Small stability delay
}