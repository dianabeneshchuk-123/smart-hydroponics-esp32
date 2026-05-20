#include <Arduino.h>
#include <FastLED.h>

// ==========================================
// НАЛАШТУВАННЯ СТРІЧКИ (З вашого робочого коду)
// ==========================================
#define LED_PIN     32       // Зелений дріт
#define NUM_LEDS     60    // Кількість діодів (можете змінити під ваш шматок)
#define LED_TYPE    WS2813   // Протокол
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// ==========================================
// ПІНИ СИСТЕМИ
// ==========================================
// Світло
const int lightSensorPin = 34; 
const int relay1Pin = 25;      // Реле 1 (Світло) - у вашому коді було relayPin

// Помпа та Кнопка (Додано)
const int buttonPin = 33;      // Пін для Кнопки ручного поливу
const int relay2Pin = 26;      // Пін для Реле 2 (Водяна помпа)

// НОВИЙ ПОРІГ: Якщо значення перевалить за 2000 (стане темно) - вмикаємо!
int threshold = 2000; 

void setup() {
  Serial.begin(115200); 

  // --- Налаштування світла ---
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, HIGH); // На старті стрічка вимкнена

  // --- Налаштування помпи та кнопки ---
  pinMode(buttonPin, INPUT_PULLUP); // Внутрішній резистор для кнопки
  pinMode(relay2Pin, OUTPUT);
  digitalWrite(relay2Pin, LOW);     // На старті помпа вимкнена

  // Ініціалізація FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); // Яскравість (0-255)

  Serial.println("Система готова! Чекаю на темряву та натискання кнопки...");
}

void loop() {
  // ==========================================
  // ЧАСТИНА 1: ЛОГІКА ДАТЧИКА СВІТЛА ТА СТРІЧКИ
  // ==========================================
  int lightValue = analogRead(lightSensorPin);

  Serial.print("Рівень світла: ");
  Serial.print(lightValue);

  // Якщо значення перевалить за 2000 (стане темно)
  if (lightValue > threshold) {
    digitalWrite(relay1Pin, LOW); // Вмикаємо реле (12V)
    delay(50); 

    fill_solid(leds, NUM_LEDS, CRGB::White); // Кажемо світити білим
    FastLED.show();

    Serial.println("  --> ТЕМНО! Реле 1 УВІМКНЕНО. Світло ГОРИТЬ!");
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black); // Вимикаємо діоди
    FastLED.show();

    digitalWrite(relay1Pin, HIGH);  // Вимикаємо реле

    Serial.println("  --> СВІТЛО! Реле 1 ВИМКНЕНО.");
  }

  // ==========================================
  // ЧАСТИНА 2: ЛОГІКА КНОПКИ ТА ПОМПИ
  // ==========================================
  int buttonState = digitalRead(buttonPin); // Читаємо стан кнопки
  
  // Якщо кнопку натиснуто (сигнал LOW через INPUT_PULLUP)
  if (buttonState == LOW) {
    Serial.println("  *** Кнопку натиснуто! Запускаю помпу на 10 секунд... ***");
    
    digitalWrite(relay2Pin, HIGH);  // Вмикаємо друге реле (водяну помпу)
    
    delay(10000);                   // Чекаємо рівно 10 секунд (10 000 мілісекунд)
    
    digitalWrite(relay2Pin, LOW);   // Вимикаємо помпу
    
    Serial.println("  *** Цикл перемішування завершено. Реле помпи вимкнено. ***");
  }

  delay(500); 
}