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
#include <OneWire.h>              
#include <DallasTemperature.h>    
#include <time.h>                 
#include <PubSubClient.h>         

// ==========================================
// WI-FI & THINGSBOARD
// ==========================================
const char* ssid     = "RASPBERRYNET";     
const char* password = "VerySecret"; 

#define TOKEN "XZE8jljP8NUnN5TOFlRZ"         
const char* mqtt_server = "thingsboard.cloud";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMqttReconnectAttempt = 0;  

WiFiUDP ntpUDP;
time_t timeOffset = 7200; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", timeOffset); 

// ==========================================
// LED & SENSORER
// ==========================================
#define LED_PIN      32
#define NUM_LEDS     60
#define LED_TYPE     WS2813
#define COLOR_ORDER  GRB
CRGB leds[NUM_LEDS];

Adafruit_BME280 bme; 

#define ONE_WIRE_BUS 5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature waterSensor(&oneWire); 

const int trigPin = 27; 
const int echoPin = 14; 

unsigned long previousMillis = 0;
const long interval = 2000; 
int lastSecond = -1;        

// ==========================================
// TFT SPI SKÆRM
// ==========================================
#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// PIN-KONFIGURATION
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      
const int relay2Pin = 19;      
const int buttonPin = 33;      

int threshold = 2000; 
float airTemp = 0.0, airHum = 0.0, waterTemp = 0.0;
int waterDistance = 0; 
int waterPercent = 0; 

String lightStatus = "OFF";
String waterPumpStatus = "OFF";
String airPumpStatus = "ON ";  

// ==========================================
// MQTT FORBINDELSE
// ==========================================
boolean reconnectMQTT() {
  if (client.connect("ESP32_Hydroponics_Main", TOKEN, NULL)) {
    return true;
  }
  return false;
}

// ==========================================
// SKÆRMOPDATERING
// ==========================================
void updateDisplay() {
  tft.setTextSize(2);
  
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  char dateBuffer[15];
  sprintf(dateBuffer, "%02d/%02d/%04d", ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900);
  
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print(dateBuffer);
  tft.print("  ");
  
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getFormattedTime()); 
  
  tft.setCursor(10, 50);
  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.print("AIR:");
  
  tft.setCursor(10, 75);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("T: ");
  tft.print(airTemp, 1);
  tft.print("C  ");
  
  tft.setCursor(10, 100);
  tft.print("H: ");
  tft.print(airHum, 1);
  tft.print("%  ");

  tft.setCursor(160, 50);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.print("WATER:");
  
  tft.setCursor(160, 75);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("T: ");
  if (waterTemp == -127.00) {
    tft.print("ERR ");
  } else {
    tft.print(waterTemp, 1);
    tft.print("C  ");
  }
  
  tft.setCursor(160, 100);
  tft.print("L: ");
  if (waterPercent <= 10) {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  } else {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  }
  tft.print(waterPercent);
  tft.print("%   ");

  tft.setCursor(160, 125);
  if (waterPercent <= 10) {
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.print(" LOW WATER ");
  } else {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print("[LEVEL OK]");
  }

  tft.setCursor(10, 160);
  tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
  tft.print("SYSTEM STATUS:");

  tft.setCursor(10, 190);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Light : ");
  if (lightStatus == "ON ") {
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  } else {
    tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  }
  tft.print(lightStatus);

  tft.setCursor(160, 190);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Air Pumpe:");
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(airPumpStatus);

  tft.setCursor(10, 215);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("W-Pump: ");
  if (waterPumpStatus == "ON ") {
    tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  } else {
    tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  }
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
  
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(1024);

  tft.fillScreen(ILI9341_BLACK);
  tft.drawFastHLine(0, 35, 320, ILI9341_DARKGREY);
  tft.drawFastHLine(0, 150, 320, ILI9341_DARKGREY);
  tft.drawFastVLine(150, 35, 115, ILI9341_DARKGREY); 
  
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
  unsigned long currentMillis = millis();
  
  if (!client.connected()) {
    if (currentMillis - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = currentMillis;
      if (reconnectMQTT()) {
        lastMqttReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }

  timeClient.update();
  bool displayNeedsUpdate = false; 
  
  if (timeClient.getSeconds() != lastSecond) {
    lastSecond = timeClient.getSeconds();
    displayNeedsUpdate = true;
  }

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
    
    long duration = pulseIn(echoPin, HIGH, 30000);
    waterDistance = duration * 0.034 / 2; 
    
    waterPercent = map(waterDistance, 10, 5, 0, 100);
    if (waterPercent > 100) waterPercent = 100;
    if (waterPercent < 0) waterPercent = 0;
    
    displayNeedsUpdate = true; 

    if (client.connected()) {
      String payload = "{";
      payload += "\"temperature\":" + String(airTemp) + ",";
      payload += "\"humidity\":" + String(airHum) + ",";
      payload += "\"water_temp\":" + String(waterTemp) + ",";
      payload += "\"water_level\":" + String(waterPercent) + ","; 
      payload += "\"waterPumpStatus\":\"" + waterPumpStatus + "\","; 
      payload += "\"lightStatus\":\"" + lightStatus + "\"";          
      payload += "}";                                                
      client.publish("v1/devices/me/telemetry", payload.c_str());
    }
  }

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
  
  if (oldLightStatus != lightStatus) {
    displayNeedsUpdate = true;
  }

  int buttonState = digitalRead(buttonPin); 
  String oldPumpStatus = waterPumpStatus;

  // ==================================================
  // AUTOMATISK OVERLØBSBESKYTTELSE (60%)
  // ==================================================
  bool safetyAllowed = true; 
  if (waterPercent >= 60) {
    safetyAllowed = false; // Hvis vandstanden er over 60%, blokeres pumpen helt!
  }

  if (buttonState == LOW && safetyAllowed) {
    digitalWrite(relay2Pin, LOW);
    waterPumpStatus = "ON ";
  } 
  else {
    digitalWrite(relay2Pin, HIGH);
    waterPumpStatus = "OFF";
  }
  
  // ==================================================
  // SOFTWARE-GENSTART AF SKÆRM VED PUMPESKIFT
  // ==================================================
  if (oldPumpStatus != waterPumpStatus) {
    displayNeedsUpdate = true;
    
    delay(30); // Optimal reduceret forsinkelse
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    
    tft.drawFastHLine(0, 35, 320, ILI9341_DARKGREY);   
    tft.drawFastHLine(0, 150, 320, ILI9341_DARKGREY);  
    tft.drawFastVLine(150, 35, 115, ILI9341_DARKGREY);
  }

  if (displayNeedsUpdate) {
    updateDisplay();
  }
  
  delay(50); 
}