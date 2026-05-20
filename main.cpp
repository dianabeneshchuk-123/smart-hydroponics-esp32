#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
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
const char* ssid     = "RASPBERRYNET";
const char* password = "VerySecret";

// NTP Time setup (Denmark - CEST UTC+2)
WiFiUDP ntpUDP;
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
// CLIMATE SENSORS CONFIGURATION
// ==========================================
// BME280 (I2C)
Adafruit_BME280 bme; 

// DS18B20 Water Temperature Sensor
#define ONE_WIRE_BUS 5 // Yellow wire connected to GPIO 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire);

unsigned long previousMillis = 0;
const long interval = 2000; // Update all sensors every 2 seconds

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
const int relay1Pin = 25;      // Light
const int buttonPin = 33;      // Pump button
const int relay2Pin = 26;      // Pump

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;
String lightStatus = "OFF";
String pumpStatus = "OFF";

// Function to update the TFT screen
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

  // 3. Water Temperature (DS18B20)
  tft.setCursor(10, 100);
  tft.print("Wat T: ");
  // Highlight water temp in cyan for visual distinction
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  if (waterTemp == -127.00) {
     tft.print("ERR  "); // Show error if sensor is disconnected
  } else {
     tft.print(waterTemp, 1); 
     tft.print(" C ");
  }

  // 4. System Statuses
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 150);
  tft.print("Light: ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  tft.setCursor(10, 180);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("Pump:  ");
  if (pumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(pumpStatus);
}

void setup() {
  Serial.begin(115200); 

  tft.begin();
  tft.setRotation(1); 
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  
  if (!bme.begin(0x76)) {
    Serial.println("Error! BME280 sensor not found!");
  }

  // Initialize Water Sensor
  waterSensor.begin();

  tft.setCursor(10, 30);
  tft.print("Connecting to Wi-Fi...");
  
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

  timeClient.begin();
  
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); 
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);     

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  tft.fillScreen(ILI9341_BLACK); 
  updateDisplay(); 
}

void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();

  // ==========================================
  // PART 1: SENSOR READINGS
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    
    // Read Air Climate
    airTemp = bme.readTemperature();
    airHum = bme.readHumidity();

    // Read Water Temperature
    waterSensor.requestTemperatures(); 
    waterTemp = waterSensor.getTempCByIndex(0);
    
    updateDisplay(); 
  }

  // ==========================================
  // PART 2: LOGIC
  // ==========================================
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    if (lightStatus != "ON ") { 
      digitalWrite(relay1Pin, LOW); 
      delay(300);                   
      fill_solid(leds, NUM_LEDS, CRGB::White); 
      FastLED.show();
      lightStatus = "ON "; 
    }
  } else {
    if (lightStatus != "OFF") {
      fill_solid(leds, NUM_LEDS, CRGB::Black); 
      FastLED.show();
      delay(50); 
      digitalWrite(relay1Pin, HIGH); 
      lightStatus = "OFF";
    }
  }

  if (oldLightStatus != lightStatus) {
    updateDisplay();
  }

  int buttonState = digitalRead(buttonPin); 
  if (buttonState == LOW) {
    pumpStatus = "ON ";
    updateDisplay(); 
    
    digitalWrite(relay2Pin, HIGH);  
    delay(10000);                   
    digitalWrite(relay2Pin, LOW);   
    
    pumpStatus = "OFF";
    updateDisplay(); 
  }

  delay(50); 
}