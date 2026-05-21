#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>                 // Kræves til I2C (BME280)
#include <Adafruit_BME280.h>      // BME280 sensorbibliotek
#include <SPI.h>                  // Kræves til SPI (TFT-skærm)
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <OneWire.h>              // Kræves til DS18B20
#include <DallasTemperature.h>    // Kræves til DS18B20
#include <time.h>                 // Tilføjet til håndtering af dato

// ==========================================
// WI-FI KONFIGURATION
// ==========================================
const char* ssid     = "RASPBERRYNET";     
const char* password = "VerySecret"; 

// NTP Tidsindstilling specifikt for Danmark
WiFiUDP ntpUDP;
time_t timeOffset = 7200; // UTC+2 (Dansk sommertid)
NTPClient timeClient(ntpUDP, "pool.ntp.org", timeOffset); 

// ==========================================
// KONFIGURATION AF LED-BÅND
// ==========================================
#define LED_PIN     32
#define NUM_LEDS     60
#define LED_TYPE    WS2813
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// KONFIGURATION AF SENSORER
// ==========================================
Adafruit_BME280 bme; // BME280 (Luftklima)

#define ONE_WIRE_BUS 5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire); // DS18B20 (Vandtemperatur)

// HC-SR04 (Vandstand)
const int trigPin = 27; 
const int echoPin = 14; 

unsigned long previousMillis = 0;
const long interval = 2000; // Opdater sensorer hvert 2. sekund
int lastSecond = -1;        // Til sporing af sekundændringer

// ==========================================
// KONFIGURATION AF TFT-SKÆRM (SPI)
// ==========================================
#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// SYSTEMPINS (RELÆER OG KNAP)
// ==========================================
const int lightSensorPin = 34; // Lyssensor
const int relay1Pin = 25;      // Relæ 1: Vækstlys
const int relay2Pin = 26;      // Relæ 2: Vandpumpe
const int buttonPin = 33;      // Knap til manuel opfyldning

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;

int waterDistance = 0; 
int waterPercent = 0; 

String lightStatus = "OFF";
String waterPumpStatus = "OFF";
String airPumpStatus = "ON ";  // Manuel 230V pumpe i 11L tank

// ==========================================
// FUNKTION TIL AT TEGNE DASHBOARDET
// ==========================================
void updateDisplay() {
  tft.setTextSize(2);
  
  // --- 1. HEADER: DATO OG TID ---
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  
  char dateBuffer[15];
  sprintf(dateBuffer, "%02d/%02d/%04d", ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900);
  
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(dateBuffer);
  tft.print("  ");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getFormattedTime()); // Giver automatisk TT:MM:SS
  
  // --- 2. BLOK: KLIMA (Venstre kolonne) ---
  tft.setCursor(10, 50); 
  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK); 
  tft.print("AIR:");
  
  tft.setCursor(10, 75); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("T: "); tft.print(airTemp, 1); tft.print("C  ");
  
  tft.setCursor(10, 100); 
  tft.print("H: "); tft.print(airHum, 1); tft.print("%  ");

  // --- 3. BLOK: VAND (Højre kolonne) ---
  tft.setCursor(160, 50); 
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK); 
  tft.print("WATER:");
  
  tft.setCursor(160, 75); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("T: "); 
  if (waterTemp == -127.00) tft.print("ERR "); 
  else { tft.print(waterTemp, 1); tft.print("C  "); }

  tft.setCursor(160, 100); 
  tft.print("L: "); 
  if (waterPercent <= 10) tft.setTextColor(ILI9341_RED, ILI9341_BLACK); 
  else tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(waterPercent); tft.print("%   ");

  // Tankens status (med farvet baggrund til alarm)
  tft.setCursor(160, 125);
  if (waterPercent <= 10) {
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED); // Hvid tekst på rød baggrund
    tft.print(" LOW WATER ");
  } else {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK); // Grøn tekst på sort baggrund
    tft.print("[LEVEL OK]");
  }

  // --- 4. BLOK: SYSTEMSTATUS ---
  tft.setCursor(10, 160); 
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK); 
  tft.print("SYSTEM STATUS:");

  tft.setCursor(10, 190); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("Light : ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  tft.setCursor(170, 190); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("A-Pump: ");
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK); 
  tft.print(airPumpStatus);

  tft.setCursor(10, 215); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("W-Pump: ");
  if (waterPumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(waterPumpStatus);
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
  
  // Tegner statisk dashboard-grafik KUN ÉN GANG for at forhindre flimren
  tft.fillScreen(ILI9341_BLACK);
  tft.drawFastHLine(0, 35, 320, ILI9341_DARKGREY);   // Linje under uret
  tft.drawFastHLine(0, 150, 320, ILI9341_DARKGREY);  // Linje over status
  tft.drawFastVLine(150, 35, 115, ILI9341_DARKGREY); // Lodret linje mellem luft og vand
  
  timeClient.begin();
  
  digitalWrite(relay1Pin, HIGH); 
  pinMode(relay1Pin, OUTPUT);
  
  digitalWrite(relay2Pin, HIGH); 
  pinMode(relay2Pin, OUTPUT);

  pinMode(buttonPin, INPUT_PULLUP); 

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  updateDisplay(); 
}

void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();
  bool displayNeedsUpdate = false; // Flag til smart opdatering af skærmen

  // 1. KONTROL AF UR (Hvert sekund)
  if (timeClient.getSeconds() != lastSecond) {
    lastSecond = timeClient.getSeconds();
    displayNeedsUpdate = true; // Sekundet er skiftet - skærmen skal opdateres
  }

  // 2. KONTROL AF SENSORER (Hvert 2. sekund)
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
    
    waterPercent = map(waterDistance, 10, 5, 0, 100);
    if (waterPercent > 100) waterPercent = 100;
    if (waterPercent < 0) waterPercent = 0;
    
    displayNeedsUpdate = true; // Nye data er læst - skærmen skal opdateres
  }

  // 3. SYSTEMLOGIK
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    if (lightStatus != "ON ") { 
      digitalWrite(relay1Pin, LOW);            
      delay(150);                              
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
  if (oldLightStatus != lightStatus) displayNeedsUpdate = true;

  int buttonState = digitalRead(buttonPin); 
  String oldPumpStatus = waterPumpStatus;

  if (buttonState == LOW) {
    digitalWrite(relay2Pin, LOW);   
    waterPumpStatus = "ON ";
  } else {
    if (waterDistance >= 10) {      
      digitalWrite(relay2Pin, LOW); 
      waterPumpStatus = "ON ";
    } 
    else if (waterDistance <= 5) {  
      digitalWrite(relay2Pin, HIGH); 
      waterPumpStatus = "OFF";
    }
  }
  if (oldPumpStatus != waterPumpStatus) displayNeedsUpdate = true;

  // 4. OPDATERING AF SKÆRM (Kun hvis noget er ændret)
  if (displayNeedsUpdate) {
    updateDisplay();
  }

  delay(50); 
}