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
// КОНФІГУРАЦІЯ
// ==========================================
const char* ssid     = "RASPBERRYNET";     
const char* password = "VerySecret"; 
#define TOKEN "XZE8jljP8NUnN5TOFlRZ"         
const char* mqtt_server = "thingsboard.cloud";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMqttReconnectAttempt = 0;  

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200); 

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
const int lightSensorPin = 34; 
const int relay1Pin = 25;      
const int relay2Pin = 26;      
const int buttonPin = 33;      

#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

int threshold = 2000; 
float airTemp, airHum, waterTemp;
int waterDistance, waterPercent;
String lightStatus = "OFF", waterPumpStatus = "OFF", airPumpStatus = "ON "; 
bool rpcPumpON = false; 
unsigned long rpcPumpTimer = 0;
unsigned long previousMillis = 0;
int lastSecond = -1;

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  if (message.indexOf("\"method\":\"setValue\"") != -1) {
    if (message.indexOf("\"params\":true") != -1) {
      rpcPumpON = true; 
      rpcPumpTimer = millis(); 
    } else {
      rpcPumpON = false;
    }
  }
}

boolean reconnectMQTT() {
  if (client.connect("ESP32_Hydroponics", TOKEN, NULL)) {
    client.subscribe("v1/devices/me/rpc/request/+");
    return true;
  }
  return false;
}

void updateDisplay() {
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getFormattedTime()); 
  
  tft.setCursor(10, 50); tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Water: "); tft.print(waterPercent); tft.print("% ");
  
  tft.setCursor(10, 190); tft.print("Light: "); tft.print(lightStatus);
  tft.setCursor(10, 215); tft.print("W-Pump: "); tft.print(waterPumpStatus);
}

void setup() {
  Serial.begin(115200);
  tft.begin(); tft.setRotation(1); tft.fillScreen(ILI9341_BLACK);
  bme.begin(0x76);
  waterSensor.begin();
  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);
  pinMode(relay1Pin, OUTPUT); digitalWrite(relay1Pin, HIGH);
  pinMode(relay2Pin, OUTPUT); digitalWrite(relay2Pin, HIGH);
  pinMode(buttonPin, INPUT_PULLUP);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  timeClient.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (!client.connected() && (currentMillis - lastMqttReconnectAttempt > 5000)) {
    lastMqttReconnectAttempt = currentMillis;
    reconnectMQTT();
  }
  client.loop();
  timeClient.update();

  if (currentMillis - previousMillis >= 2000) {
    previousMillis = currentMillis;
    waterSensor.requestTemperatures();
    waterTemp = waterSensor.getTempCByIndex(0);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10); digitalWrite(trigPin, LOW);
    waterDistance = pulseIn(echoPin, HIGH) * 0.034 / 2;
    waterPercent = map(waterDistance, 10, 5, 0, 100);
    waterPercent = constrain(waterPercent, 0, 100);
    updateDisplay();
  }

  // Логіка світла
  if (analogRead(lightSensorPin) > threshold) {
    digitalWrite(relay1Pin, LOW); fill_solid(leds, NUM_LEDS, CRGB::White); FastLED.show(); lightStatus = "ON ";
  } else {
    digitalWrite(relay1Pin, HIGH); fill_solid(leds, NUM_LEDS, CRGB::Black); FastLED.show(); lightStatus = "OFF";
  }

  // --- ФІНАЛЬНА ЛОГІКА ПОМПИ (З БЛОКУВАННЯМ > 60%) ---
  int buttonState = digitalRead(buttonPin);
  bool safetyAllowed = (waterPercent < 60);

  if (rpcPumpON && (currentMillis - rpcPumpTimer >= 5000)) {
    rpcPumpON = false;
  }

  if (safetyAllowed && (buttonState == LOW || rpcPumpON)) {
    digitalWrite(relay2Pin, LOW);
    waterPumpStatus = "ON ";
  } else {
    digitalWrite(relay2Pin, HIGH);
    waterPumpStatus = "OFF";
    if (!safetyAllowed) rpcPumpON = false;
  }

  delay(50);
}