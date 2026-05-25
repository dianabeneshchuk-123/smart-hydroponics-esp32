#include <Arduino.h>
#include <unity.h>

// ==========================================
// LOGIKKEN FOR PUMPEN (Kopieret hertil for test)
// ==========================================
bool calculatePumpState(bool isPhysicalButtonPressed, bool isRpcPumpON, int currentWaterDistance, bool currentPumpState) {
  // 1 PRIORITET: Fysisk knap
  if (isPhysicalButtonPressed) return true; 
  
  // 2 PRIORITET: Knap fra ThingsBoard (RPC)
  if (isRpcPumpON) {
    if (currentWaterDistance <= 5) return false; // Sikkerhedsstop
    return true;
  }
  
  // 3 PRIORITET: Baggrundsautomatik
  if (currentWaterDistance >= 10) return true;  
  if (currentWaterDistance <= 5) return false; 

  return currentPumpState; 
}

// ==========================================
// VORES TESTER
// ==========================================

void test_physical_button_priority(void) {
    // Hvis den fysiske knap er trykket ind (true), uanset vandstanden (selv 1 cm) -> pumpen skal TÆNDE
    TEST_ASSERT_TRUE(calculatePumpState(true, false, 1, false));
}

void test_rpc_button_normal(void) {
    // Virtuel knap (RPC) er aktiveret, vandstanden er lav (15 cm) -> pumpen skal TÆNDE
    TEST_ASSERT_TRUE(calculatePumpState(false, true, 15, false));
}

void test_rpc_safety_cutoff(void) {
    // SIKKERHEDSSTOP: Virtuel knap er aktiveret, men vandstanden er kritisk høj (4 cm) -> pumpen skal være SLUKKET
    TEST_ASSERT_FALSE(calculatePumpState(false, true, 4, true));
}

void test_auto_refill_start(void) {
    // Ingen knapper er trykket, vandstanden faldt til 12 cm -> automatikken skal TÆNDE pumpen
    TEST_ASSERT_TRUE(calculatePumpState(false, false, 12, false));
}

void test_auto_refill_stop(void) {
    // Ingen knapper er trykket, vandstanden steg til 3 cm -> automatikken skal SLUKKE pumpen
    TEST_ASSERT_FALSE(calculatePumpState(false, false, 3, true));
}

void test_idle_state(void) {
    // Vandstanden er normal (8 cm), ingen knapper er trykket. 
    // Hvis pumpen var slukket (false), skal den forblive slukket (false)
    TEST_ASSERT_FALSE(calculatePumpState(false, false, 8, false));
    
    // Hvis pumpen på en eller anden måde kørte (true), skal den fortsætte med at køre (true)
    TEST_ASSERT_TRUE(calculatePumpState(false, false, 8, true));
}

// ==========================================
// START AF UNITY
// ==========================================

void setup() {
    // Venter 2 sekunder, så Serial Monitor kan nå at forbinde
    delay(2000); 
    UNITY_BEGIN();
    
    RUN_TEST(test_physical_button_priority);
    RUN_TEST(test_rpc_button_normal);
    RUN_TEST(test_rpc_safety_cutoff);
    RUN_TEST(test_auto_refill_start);
    RUN_TEST(test_auto_refill_stop);
    RUN_TEST(test_idle_state);
    
    UNITY_END();
}

void loop() {
    // Tom, da testene kun køres én gang i setup()
}
