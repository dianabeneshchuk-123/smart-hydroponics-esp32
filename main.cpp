#include <Arduino.h>
#include <FastLED.h>
#include <DHT.h> // Підключаємо бібліотеку для датчика клімату

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
#define DHTPIN 4          // Пін, до якого підключено DATA датчика
#define DHTTYPE DHT22     // Вказуємо тип датчика (DHT22 або DHT11)
DHT dht(DHTPIN, DHTTYPE); // Створюємо об'єкт датчика

unsigned long previousMillis = 0; // Змінна для зберігання часу останнього опитування
const long interval = 2000;       // Інтервал опитування: 2000 мілісекунд (2 секунди)

// ==========================================
// ПІНИ СИСТЕМИ
// ==========================================
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Світло
const int buttonPin = 33;      // Кнопка
const int relay2Pin = 26;      // Помпа

int threshold = 2000; 

void setup() {
  Serial.begin(115200); 

  // Ініціалізація датчика клімату
  dht.begin();

  // Налаштування світла
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); 

  // Налаштування помпи та кнопки
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);     

  // Ініціалізація FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); 

  Serial.println("Система готова! Датчик клімату запущено.");
}

void loop() {
  // ==========================================
  // ЧАСТИНА 1: ЧИТАННЯ КЛІМАТУ КОЖНІ 2 СЕКУНДИ
  // ==========================================
  unsigned long currentMillis = millis(); // Дивимось на "секундомір"

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Запам'ятовуємо час цього опитування

    // Зчитуємо вологість та температуру
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Перевіряємо, чи не було помилки при зчитуванні
    if (isnan(h) || isnan(t)) {
      Serial.println("Помилка зчитування з датчика DHT!");
    } else {
      Serial.print("Температура: ");
      Serial.print(t);
      Serial.print(" °C  |  Вологість: ");
      Serial.print(h);
      Serial.println(" %");
    }
  }

  // ==========================================
  // ЧАСТИНА 2: ЛОГІКА ДАТЧИКА СВІТЛА ТА СТРІЧКИ
  // ==========================================
  int lightValue = analogRead(lightSensorPin);

  if (lightValue > threshold) {
    digitalWrite(relay1Pin, LOW); 
    delay(300); // Пауза для ініціалізації чипів стрічки
    fill_solid(leds, NUM_LEDS, CRGB::White); 
    FastLED.show();
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black); 
    FastLED.show();
    delay(50); 
    digitalWrite(relay1Pin, HIGH);  
  }

  // ==========================================
  // ЧАСТИНА 3: ЛОГІКА КНОПКИ ТА ПОМПИ
  // ==========================================
  int buttonState = digitalRead(buttonPin); 
  
  if (buttonState == LOW) {
    Serial.println(" *** Запускаю помпу на 10 секунд... ***");
    digitalWrite(relay2Pin, HIGH);  
    delay(10000);                   
    digitalWrite(relay2Pin, LOW);   
    Serial.println(" *** Цикл перемішування завершено. ***");
  }

  delay(100); // Невеличка затримка для стабільності
}