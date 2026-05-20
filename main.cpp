#include <Arduino.h>
#include <FastLED.h>
#include <DHT.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ==========================================
// НАЛАШТУВАННЯ СТРІЧКИ
// ==========================================
#define LED_PIN     32
#define NUM_LEDS     60
#define LED_TYPE    WS2813
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// НАЛАШТУВАННЯ DHT22 (КЛІМАТ)
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 2000; 

// ==========================================
// НАЛАШТУВАННЯ TFT ЕКРАНА (SPI)
// ==========================================
#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
// Піни SCK(18) та MOSI(23) використовуються апаратно за замовчуванням

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// ПІНИ СИСТЕМИ
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Світло
const int buttonPin = 33;      // Кнопка
const int relay2Pin = 26;      // Помпа

int threshold = 2000; 

// Змінні для зберігання попередніх станів
float t = 0.0, h = 0.0;
String lightStatus = "OFF";
String pumpStatus = "OFF";

// Функція оновлення екрана
void updateDisplay() {
  // Встановлюємо розмір шрифту (2 - середній, добре читається)
  tft.setTextSize(2);
  
  // Колір тексту білий, фон чорний (щоб старі цифри затиралися без блимання екрана)
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // Виводимо температуру та вологість
  tft.setCursor(10, 20);
  tft.print("Temp: ");
  if (isnan(t)) tft.print("--  "); else { tft.print(t, 1); tft.print(" C "); }
  
  tft.setCursor(10, 50);
  tft.print("Hum:  ");
  if (isnan(h)) tft.print("--  "); else { tft.print(h, 1); tft.print(" % "); }

  // Виводимо статус світла (зробимо його кольоровим)
  tft.setCursor(10, 100);
  tft.print("Light: ");
  if (lightStatus == "ON ") {
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  } else {
    tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  }
  tft.print(lightStatus);

  // Виводимо статус помпи (зробимо її синьою)
  tft.setCursor(10, 130);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Повертаємо білий
  tft.print("Pump:  ");
  if (pumpStatus == "ON ") {
    tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  } else {
    tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  }
  tft.print(pumpStatus);
}

void setup() {
  Serial.begin(115200); 

  // Ініціалізація екрана
  tft.begin();
  tft.setRotation(1); // Повертаємо екран горизонтально (можна 0, 1, 2, 3)
  tft.fillScreen(ILI9341_BLACK); // Заливаємо все чорним на старті
  
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(20, 50);
  tft.println("System");
  tft.setCursor(20, 90);
  tft.println("Booting...");
  
  dht.begin();

  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); 
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);     

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  delay(1500);
  tft.fillScreen(ILI9341_BLACK); // Чистимо екран після завантаження
  updateDisplay(); // Малюємо початкові дані
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Клімат
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    h = dht.readHumidity();
    t = dht.readTemperature();
    updateDisplay();
  }

  // 2. Світло
  int lightValue = analogRead(lightSensorPin);
  String oldLightStatus = lightStatus;

  if (lightValue > threshold) {
    digitalWrite(relay1Pin, LOW); 
    delay(300); 
    fill_solid(leds, NUM_LEDS, CRGB::White); 
    FastLED.show();
    lightStatus = "ON "; 
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black); 
    FastLED.show();
    delay(50); 
    digitalWrite(relay1Pin, HIGH);  
    lightStatus = "OFF";
  }

  if (oldLightStatus != lightStatus) {
    updateDisplay();
  }

  // 3. Помпа
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

  delay(100); 
}