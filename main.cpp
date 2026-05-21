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
// Time offset: 7200 seconds = UTC+2 (Summer time / CEST in Denmark)
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
// SENSORS CONFIGURATION
// ==========================================
// BME280 (Air Climate)
Adafruit_BME280 bme; 

// DS18B20 (Water Temperature)
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
const int buttonPin = 33;      // Water Pump manual button

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;
int waterDistance = 0; 
String lightStatus = "OFF";
String waterPumpStatus = "OFF";
String airPumpStatus = "ON ";  // Assuming Air Pump is plugged straight into 230V

// Water Pump Timers
unsigned long previousPumpMillis = 0;
const unsigned long SIX_HOURS_MS = 21600000UL; // 6 hours in milliseconds
const unsigned long PUMP_RUN_TIME = 10000UL;   // 10 seconds

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
  tft.print("   "); 
  
  // 2. Air Climate
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 40);
  tft.print("Air T: "); tft.print(airTemp, 1); tft.print(" C "); 
  tft.setCursor(10, 65);
  tft.print("Air H: "); tft.print(airHum, 1); tft.print(" % ");

  // 3. Water Stats
  tft.setCursor(10, 95);
  tft.print("Wat T: ");
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  if (waterTemp == -127.00) tft.print("ERR  "); 
  else { tft.print(waterTemp, 1); tft.print(" C "); }

  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 120);
  tft.print("Level: ");
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(waterDistance); tft.print(" cm  ");

  // 4. System Statuses
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  
  // Light
  tft.setCursor(10, 155);
  tft.print("Light:  ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  // Water Pump
  tft.setCursor(10, 180);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("W-Pump: ");
  if (waterPumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(waterPumpStatus);

  // Air Pump (Always ON status)
  tft.setCursor(10, 205);
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
  // Set HIGH state BEFORE declaring pinMode to prevent startup glitch
  digitalWrite(relay1Pin, HIGH); // Grow Light OFF
  pinMode(relay1Pin, OUTPUT);
  
  digitalWrite(relay2Pin, HIGH); // Water Pump OFF 
  pinMode(relay2Pin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP); 

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  updateDisplay(); 
}

// Helper function to run the water pump for 10 seconds
void runWaterPump() {
  waterPumpStatus = "ON ";
  updateDisplay(); 
  
  digitalWrite(relay2Pin, LOW);   // Turn ON pump (Active LOW)
  delay(PUMP_RUN_TIME);           // Wait 10 seconds      
  digitalWrite(relay2Pin, HIGH);  // Turn OFF pump
  
  waterPumpStatus = "OFF";
  updateDisplay(); 
  
  // Reset the timer so it doesn't trigger immediately after a manual press
  previousPumpMillis = millis(); 
}

void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();

  // ==========================================
  // PART 1: SENSOR READINGS
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    
    airTemp = bme.readTemperature();
    airHum = bme.readHumidity();

    waterSensor.requestTemperatures(); 
    waterTemp = waterSensor.getTempCByIndex(0);

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH);
    waterDistance = duration * 0.034 / 2; 
    
    updateDisplay(); 
  }

  // ==========================================
  // PART 2: SYSTEM LOGIC
  // ==========================================
  
  // 1. Grow Light Logic (FIXED: Added delays for Relay switching)
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    if (lightStatus != "ON ") { 
      digitalWrite(relay1Pin, LOW);            // 1. Turn ON Relay 1 (provide 12V to Strip)
      delay(150);                              // 2. WAIT for strip to power up
      fill_solid(leds, NUM_LEDS, CRGB::White); // 3. Send color data
      FastLED.show();
      lightStatus = "ON "; 
    }
  } else {
    if (lightStatus != "OFF") {
      fill_solid(leds, NUM_LEDS, CRGB::Black); // 1. Turn off LEDs gracefully
      FastLED.show();
      delay(50);                               // 2. Wait a moment for data to process
      digitalWrite(relay1Pin, HIGH);           // 3. Cut 12V power from relay
      lightStatus = "OFF";
    }
  }
  if (oldLightStatus != lightStatus) updateDisplay();

  // 2. Water Pump Auto-Circulation (Every 6 Hours)
  if (currentMillis - previousPumpMillis >= SIX_HOURS_MS) {
    runWaterPump();
  }

  // 3. Manual Water Pump Logic (Button)
  int buttonState = digitalRead(buttonPin); 
  if (buttonState == LOW) {
    runWaterPump();
  }

  delay(50); 
}