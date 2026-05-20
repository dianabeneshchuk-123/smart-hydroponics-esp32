#include <Arduino.h>

// Define ESP32 pins according to your SDD specification
const int ldrPin = 34;       // Analog pin for the light sensor (LDR)
const int relay1Pin = 25;    // Pin for Relay 1 (Grow Light)
const int buttonPin = 33;    // Pin for the manual watering button
const int relay2Pin = 26;    // Pin for Relay 2 (Water Pump)

void setup() {
  // Start the serial monitor for debugging (baud rate 115200)
  Serial.begin(115200);
  
  // Configure pins for the light sensor and relay
  pinMode(ldrPin, INPUT);
  pinMode(relay1Pin, OUTPUT);
  
  // Configure pins for the pump and button
  // The magic of INPUT_PULLUP saves us from needing an external resistor!
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(relay2Pin, OUTPUT);
  
  // Turn off both relays at system startup (to prevent unexpected activation)
  // Note: some relay modules turn ON with LOW and OFF with HIGH. 
  // If your relay works the opposite way, simply swap LOW and HIGH below.
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);
  
  Serial.println("Hydroponic System is ready! Waiting for sensors and button...");
}

void loop() {
  // ==========================================
  // PART 1: LIGHT SENSOR LOGIC
  // ==========================================
  int lightValue = analogRead(ldrPin); // Read the ambient light level (from 0 to 4095)
  
  // If it is dark (threshold depends on room lighting, let's assume < 2000)
  if (lightValue < 2000) {
    digitalWrite(relay1Pin, HIGH); // Turn ON the grow light
  } else {
    digitalWrite(relay1Pin, LOW);  // Turn OFF the grow light
  }

  // ==========================================
  // PART 2: BUTTON AND PUMP LOGIC (MIXER)
  // ==========================================
  int buttonState = digitalRead(buttonPin); // Read the current state of the button
  
  // Since we used INPUT_PULLUP, a pressed button gives a LOW signal
  if (buttonState == LOW) {
    Serial.println("Button pressed! Starting the pump for 10 seconds...");
    
    digitalWrite(relay2Pin, HIGH);  // Turn ON the second relay (water pump)
    
    delay(10000);                   // Wait exactly 10 seconds (10,000 milliseconds)
    
    digitalWrite(relay2Pin, LOW);   // Turn OFF the water pump
    
    Serial.println("Mixing cycle completed. Relay turned off.");
  }
  
  // Short delay for microcontroller stability
  delay(100);
}