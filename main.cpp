#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>                 // Необхідно для роботи I2C (BME280)
#include <Adafruit_BME280.h>      // Бібліотека для датчика BME280
#include <SPI.h>                  // Необхідно для роботи SPI (TFT екран)
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// ==========================================
// НАЛАШТУВАННЯ WI-FI
// ==========================================
const char* ssid     = "RASPBERRYNET";     // Назва шкільного Wi-Fi
const char* password = "VerySecret"; // Пароль від Wi-Fi

// Налаштування часу NTP
WiFiUDP ntpUDP;
time_t timeOffset = 7200; // UTC+2 (Літній час в Україні/Данії)
NTPClient timeClient(ntpUDP, "pool.ntp.org", timeOffset); 

// ==========================================
// НАЛАШТУВАННЯ АДРЕСНОЇ СТРІЧКИ
// ==========================================
#define LED_PIN     32
#define NUM_LEDS     60
#define LED_TYPE    WS2813
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// НАЛАШТУВАННЯ ДАТЧИКА BME280 (I2C)
// ==========================================
// Піни SDA(21) та SCL(22) використовуються платою автоматично
Adafruit_BME280 bme; 

unsigned long previousMillis = 0;
const long interval = 2000; // Опитування датчика кожні 2 секунди

// ==========================================
// НАЛАШТУВАННЯ TFT ЕКРАНА (SPI)
// ==========================================
#define TFT_CS   15
#define TFT_DC   16
#define TFT_RST  17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==========================================
// ПІНИ СИСТЕМИ (РЕЛЕ ТА КНОПКА)
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Реле 1: Світло
const int buttonPin = 33;      // Кнопка помпи
const int relay2Pin = 26;      // Реле 2: Водяна помпа

int threshold = 2000; 
float t = 0.0, h = 0.0;
String lightStatus = "OFF";
String pumpStatus = "OFF";

// Функція оновлення інформації на екрані
void updateDisplay() {
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // 1. Вивід часу
  tft.setCursor(10, 10);
  tft.print("Time: ");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.print(timeClient.getHours());
  tft.print(":");
  int mins = timeClient.getMinutes();
  if(mins < 10) tft.print("0"); 
  tft.print(mins);
  tft.print("   "); 
  
  // 2. Вивід клімату з BME280
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(10, 45);
  tft.print("Temp: ");
  tft.print(t, 1); 
  tft.print(" C "); 
  
  tft.setCursor(10, 75);
  tft.print("Hum:  ");
  tft.print(h, 1); 
  tft.print(" % ");

  // 3. Статус світла
  tft.setCursor(10, 120);
  tft.print("Light: ");
  if (lightStatus == "ON ") tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(lightStatus);

  // 4. Статус помпи
  tft.setCursor(10, 150);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
  tft.print("Pump:  ");
  if (pumpStatus == "ON ") tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  else tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.print(pumpStatus);
}

void setup() {
  Serial.begin(115200); 

  // Ініціалізація екрана
  tft.begin();
  tft.setRotation(1); 
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  
  // Ініціалізація датчика BME280
  // 0x76 - це стандартна адреса більшості китайських модулів BME280.
  // Якщо датчик не знайдено, спробуйте змінити адресу на 0x77
  if (!bme.begin(0x76)) {
    Serial.println("Помилка! Не вдалося знайти датчик BME280!");
    tft.setTextColor(ILI9341_RED);
    tft.print("BME280 Sensor Error!");
    while (1) delay(10); // Зупиняємо систему, якщо датчик підключено неправильно
  }

  tft.setCursor(10, 30);
  tft.print("Connecting to Wi-Fi...");
  
  // Підключення до Wi-Fi
  WiFi.begin(ssid, password);
  int connectionTimeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft.print(".");
    connectionTimeout++;
    if(connectionTimeout > 20) { 
      tft.setCursor(10, 60); 
      connectionTimeout = 0; 
    }
  }
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 30);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Wi-Fi Connected!");
  delay(1000);

  // Запуск клієнта часу
  timeClient.begin();
  
  // Налаштування реле та кнопки
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); 
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);     

  // Налаштування FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  tft.fillScreen(ILI9341_BLACK); 
  updateDisplay(); 
}

void loop() {
  timeClient.update();
  unsigned long currentMillis = millis();

  // ==========================================
  // ЧАСТИНА 1: ЗЧИТУВАННЯ КЛІМАТУ З BME280
  // ==========================================
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 
    
    // Зчитуємо нові дані з BME280
    t = bme.readTemperature();
    h = bme.readHumidity();
    
    updateDisplay(); 
  }

  // ==========================================
  // ЧАСТИНА 2: ЛОГІКА ДАТЧИКА СВІТЛА
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

  // ==========================================
  // ЧАСТИНА 3: РУЧНИЙ ПОЛИВ (КНОПКА)
  // ==========================================
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