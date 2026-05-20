#include <Arduino.h>
#include <FastLED.h>

// --- Налаштування стрічки ---
#define LED_PIN     32       // Зелений дріт
#define NUM_LEDS     60    // Кількість діодів (можете змінити під ваш шматок)
#define LED_TYPE    WS2813   // Протокол
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// --- Піни ---
const int lightSensorPin = 34; 
const int relayPin = 25;

// НОВИЙ ПОРІГ: Якщо значення перевалить за 2000 (стане темно) - вмикаємо!
int threshold = 2000; 

void setup() {
  Serial.begin(115200); 

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // На старті стрічка вимкнена

  // Ініціалізація FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(150); // Яскравість (0-255)

  Serial.println("Система готова! Чекаю на темряву...");
}

void loop() {
  int lightValue = analogRead(lightSensorPin);

  Serial.print("Рівень світла: ");
  Serial.print(lightValue);

  // УВАГА: ТУТ ЗМІНЕНО ЗНАК НА ">" (БІЛЬШЕ)
  if (lightValue > threshold) {
    digitalWrite(relayPin, LOW); // Вмикаємо реле (12V)
    delay(50); 

    fill_solid(leds, NUM_LEDS, CRGB::White); // Кажемо світити білим
    FastLED.show();

    Serial.println("  --> ТЕМНО! Реле УВІМКНЕНО. Світло ГОРИТЬ!");
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black); // Вимикаємо діоди
    FastLED.show();

    digitalWrite(relayPin, HIGH);  // Вимикаємо реле

    Serial.println("  --> СВІТЛО! Реле ВИМКНЕНО.");
  }

  delay(500); 
}