#include <Arduino.h>
#include <unity.h>

// ==========================================
// TESTLOGIK (simulerer din kode)
// ==========================================

// Pumpelogik: modtager knapstatus (true = trykket ned) og vandstand
bool shouldPumpRun(bool buttonPressed, int waterPercent) {
  bool safetyAllowed = true; 
  if (waterPercent >= 60) {
    safetyAllowed = false; // Blokering ved 60% og derover for at undgå overløb
  }
  
  if (buttonPressed && safetyAllowed) {
    return true;  // Pumpe TÆNDT
  } else {
    return false; // Pumpe SLUKKET
  }
}

// Lyslogik: modtager sensorværdi og tærskelværdi
bool shouldLightBeOn(int lightValue, int threshold) {
  return lightValue > threshold;
}

// ==========================================
// UNIT-TESTS
// ==========================================

// Test 1: Pumpen kører, hvis knappen er trykket ind, og vandstanden er lav (Normal drift)
void test_pump_runs_when_button_pressed_and_safe(void) {
  bool isPressed = true;
  int waterLevel = 45; // 45% - sikkert niveau
  TEST_ASSERT_TRUE(shouldPumpRun(isPressed, waterLevel));
}

// Test 2: OVERLØBSBESKYTTELSE (Den vigtigste test!)
void test_pump_stops_when_water_level_high(void) {
  bool isPressed = true; // Selv hvis brugeren holder knappen nede!
  int waterLevel = 60;   // Vandstanden har nået den kritiske grænse på 60%
  TEST_ASSERT_FALSE(shouldPumpRun(isPressed, waterLevel)); // Skal returnere false for at beskytte systemet
}

// Test 3: Pumpen stopper automatisk, når knappen slippes
void test_pump_off_when_button_released(void) {
  bool isPressed = false;
  int waterLevel = 30;
  TEST_ASSERT_FALSE(shouldPumpRun(isPressed, waterLevel));
}

// Test 4: Lyset tænder, når det er mørkt (sensorværdi > 2000)
void test_light_turns_on_when_dark(void) {
  int sensorValue = 2500;
  int limit = 2000;
  TEST_ASSERT_TRUE(shouldLightBeOn(sensorValue, limit));
}

// Test 5: Lyset slukker, når der er nok lys (sensorværdi <= 2000)
void test_light_turns_off_when_bright(void) {
  int sensorValue = 1500;
  int limit = 2000;
  TEST_ASSERT_FALSE(shouldLightBeOn(sensorValue, limit));
}

// ==========================================
// KØRSEL AF TESTS
// ==========================================
void setup() {
  delay(2000); // Forsinkelse, så den serielle port kan nå at stabilisere sig
  UNITY_BEGIN(); // Start af Unity test-framework

  RUN_TEST(test_pump_runs_when_button_pressed_and_safe);
  RUN_TEST(test_pump_stops_when_water_level_high);
  RUN_TEST(test_pump_off_when_button_released);
  RUN_TEST(test_light_turns_on_when_dark);
  RUN_TEST(test_light_turns_off_when_bright);

  UNITY_END(); // Afslutning af tests og udskrivning af resultater
}

void loop() {
  // I unit-tests forbliver loop-funktionen normalt tom
}
