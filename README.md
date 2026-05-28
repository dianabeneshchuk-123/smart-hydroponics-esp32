# SYSTEM DESIGN DOCUMENT (SDD)
## Automated Hydroponic IoT System

**Project Phase:** Proof of Concept (PoC) & Advanced Prototyping  
**Target Deployment Environment:** Educational Institution / School Canteen Kitchen  
**Core Microcontroller Unit:** Single ESP32 DevKit V1  
**Document Version:** 3.0 (Final Architecture, Exhaustive Pinout, and Quality Assurance)

---

## 1. PROJECT OVERVIEW & EXECUTIVE SUMMARY

### 1.1 Introduction and Problem Statement
The modern culinary environment, particularly within educational institutions, is increasingly focused on sustainability, zero-mile supply chains, and the integration of fresh, organic ingredients into daily menus. However, maintaining indoor herb gardens (such as basil and lettuce) in a fast-paced canteen kitchen introduces significant operational challenges. Kitchen staff are occupied with food preparation and lack the time or specialized botanical knowledge required to monitor water pH, manage strict photosynthetic lighting schedules, or ensure optimal root oxygenation. 

This System Design Document (SDD) outlines the comprehensive technical architecture for an automated, IoT-enabled hydroponic growing system. The system is engineered to completely remove the cognitive load from the kitchen staff by automating the entire environmental control loop. 

### 1.2 Core Objectives
1. **Total Automation:** To design a system that independently manages lighting cycles, water circulation, and root aeration without requiring daily human intervention.
2. **Robust Safety:** To implement hardware and software safeguards, specifically strict overflow protection mechanisms, to prevent any risk of water damage in the kitchen.
3. **Intuitive Telemetry:** To provide real-time, highly readable data visualization through both a localized TFT display and a remote cloud dashboard (ThingsBoard).
4. **Frictionless User Experience:** To ensure that non-technical users (canteen workers) can interact with and understand the system state at a glance, eliminating confusing technical jargon.

### 1.3 Scope and Limitations
The current Proof of Concept (PoC) focuses on the environmental monitoring and electrical actuation of lighting and fluid pumps. It successfully tracks ambient temperature, humidity, water temperature, and water level. Advanced chemical automation, such as automated pH balancing and Electrical Conductivity (EC) nutrient dosing, are considered out-of-scope for this phase and are reserved for future iterations. 

---

## 2. SYSTEM ARCHITECTURE & TOPOLOGY

### 2.1 High-Level Architecture
The system architecture follows a centralized edge-computing model. A single ESP32 DevKit V1 microcontroller serves as the centralized processing unit for all data collection, logical decision-making, and network communication. The architecture is divided into three distinct layers:
1. **The Perception Layer (Sensors):** Responsible for gathering physical environmental data (light intensity, spatial distance to water, atmospheric conditions).
2. **The Processing Layer (ESP32):** Responsible for signal conversion, moving-average filtering, safety state-machine execution, and UI rendering.
3. **The Action/Cloud Layer (Relays & MQTT):** Responsible for toggling high-voltage circuits and transmitting serialized JSON telemetry to the ThingsBoard cloud infrastructure.

### 2.2 Network Topology and Protocol
The ESP32 connects to the local facility network ("RASPBERRYNET") via its onboard 2.4GHz Wi-Fi transceiver. Time synchronization is maintained via Network Time Protocol (NTP) connecting to `pool.ntp.org` to ensure lighting schedules remain accurate regardless of power cycles. Data transmission to the cloud utilizes the MQTT (Message Queuing Telemetry Transport) protocol over port 1883. MQTT was selected for its lightweight packet structure, low bandwidth consumption, and high reliability in environments with potentially unstable Wi-Fi connections.

---

## 3. EXHAUSTIVE HARDWARE SPECIFICATIONS & PIN MAPPING

### 3.1 Component Rationale
* **ESP32 DevKit V1:** Chosen for its dual-core 240MHz processor, built-in Wi-Fi, and extensive GPIO availability. It provides ample computational headroom for handling local TFT SPI rendering simultaneously with Wi-Fi telemetry.
* **BME280 Sensor:** Selected over cheaper alternatives (like DHT11) for its high precision in measuring ambient temperature and humidity, communicating efficiently via the I2C bus.
* **DS18B20 Sensor:** A sealed, waterproof digital temperature probe utilizing the 1-Wire protocol, perfect for permanent submersion in the nutrient reservoir.
* **HC-SR04 Ultrasonic Sensor:** Used to calculate the distance from the top of the reservoir to the water surface. This data is mathematically mapped into a percentage (0-100%) to drive the overflow safety logic.
* **4-Channel Relay Module (5V logic):** Provides opto-isolated switching. Essential for separating the ESP32's sensitive 3.3V logic circuits from the inductive loads of the 12V water pump and air pump.

### 3.2 Complete System Pinout and Wiring Matrix
To ensure absolute replicability of this physical prototype, the following tables document every single electrical connection within the system.

#### Power Distribution Subsystem
| Component | Physical Pin | Destination / Source | Signal Type / Purpose |
| :--- | :--- | :--- | :--- |
| **Wall Adapter** | 12V Positive | Breadboard 12V Main Rail | Primary System Power |
| **Wall Adapter** | 12V Negative | Breadboard Common GND Rail | Primary System Ground |
| **LM2596 Buck** | IN+ | Breadboard 12V Main Rail | Receives 12V |
| **LM2596 Buck** | IN- | Breadboard Common GND Rail | Ground reference |
| **LM2596 Buck** | OUT+ | Breadboard 5V Logic Rail | Outputs stable 5.0V |
| **LM2596 Buck** | OUT- | Breadboard Common GND Rail | Ground reference |
| **ESP32 MCU** | VIN (5V In) | Breadboard 5V Logic Rail | Powers the microcontroller |
| **ESP32 MCU** | GND (Pin 14) | Breadboard Common GND Rail | Main logic ground |
| **ESP32 MCU** | 3V3 (Pin 1) | Breadboard 3.3V Sensor Rail | Outputs 3.3V for sensors |
| **ESP32 MCU** | GND (Pin 38) | Breadboard Common GND Rail | Secondary logic ground |

#### Sensor Array Wiring
| Component | Physical Pin | Destination ESP32 Pin / Rail | Signal Type / Purpose |
| :--- | :--- | :--- | :--- |
| **BME280 (Air)** | VCC | Breadboard 3.3V Sensor Rail | Sensor Power |
| **BME280 (Air)** | GND | Breadboard Common GND Rail | Sensor Ground |
| **BME280 (Air)** | SCL | ESP32 GPIO22 | I2C Clock Line |
| **BME280 (Air)** | SDA | ESP32 GPIO21 | I2C Data Line |
| **DS18B20 (Water)** | VCC (Red) | Breadboard 3.3V Sensor Rail | Sensor Power |
| **DS18B20 (Water)** | GND (Black) | Breadboard Common GND Rail | Sensor Ground |
| **DS18B20 (Water)** | DATA (Yellow) | ESP32 GPIO5 | 1-Wire Data (Requires 4.7k Pull-up to 3.3V) |
| **HC-SR04 (Sonar)** | VCC | Breadboard 5V Logic Rail | Sensor Power (Requires 5V) |
| **HC-SR04 (Sonar)** | GND | Breadboard Common GND Rail | Sensor Ground |
| **HC-SR04 (Sonar)** | TRIG | ESP32 GPIO27 | Digital Output (Sends ultrasonic pulse) |
| **HC-SR04 (Sonar)** | ECHO | ESP32 GPIO14 | Digital Input (Receives echo timing) |
| **LDR (Photoresistor)**| VCC | Breadboard 3.3V Sensor Rail | Power for voltage divider |
| **LDR (Photoresistor)**| GND | Breadboard Common GND Rail | Ground reference |
| **LDR (Photoresistor)**| SIGNAL | ESP32 GPIO34 | Analog Input (Reads light intensity 0-4095) |

#### Actuators, Displays & UI Wiring
| Component | Physical Pin | Destination ESP32 Pin / Rail | Signal Type / Purpose |
| :--- | :--- | :--- | :--- |
| **Relay Module** | VCC | Breadboard 5V Logic Rail | Powers relay electromagnets |
| **Relay Module** | GND | Breadboard Common GND Rail | Relay Ground |
| **Relay Module** | IN1 | ESP32 GPIO25 | Digital Out (Controls WS2813 Light Power) |
| **Relay Module** | IN2 | ESP32 GPIO19 | Digital Out (Controls Water Pump) |
| **TFT Display** | VCC | Breadboard 3.3V Sensor Rail | Screen Power |
| **TFT Display** | GND | Breadboard Common GND Rail | Screen Ground |
| **TFT Display** | CS | ESP32 GPIO15 | SPI Chip Select |
| **TFT Display** | RESET | ESP32 GPIO17 | SPI Reset |
| **TFT Display** | DC / RS | ESP32 GPIO16 | SPI Data/Command Selector |
| **TFT Display** | SDI / MOSI| ESP32 GPIO23 | SPI Master Out Slave In (Data) |
| **TFT Display** | SCK / CLK | ESP32 GPIO18 | SPI Clock |
| **TFT Display** | LED / BL | Breadboard 3.3V Sensor Rail | TFT Backlight Power |
| **WS2813 LED Strip** | 12V | Breadboard 12V Main Rail | Direct high-power connection |
| **WS2813 LED Strip** | GND | Breadboard Common GND Rail | Strip Ground |
| **WS2813 LED Strip** | DI (Data) | ESP32 GPIO32 | FastLED Data Signal |
| **WS2813 LED Strip** | BI (Backup)| Breadboard Common GND Rail | Grounded for signal stability |
| **Push Button** | Terminal 1 | ESP32 GPIO33 | Digital Input (Internal Pull-up enabled) |
| **Push Button** | Terminal 2 | Breadboard Common GND Rail | Pulls GPIO33 LOW when pressed |

---

## 4. SOFTWARE ENGINEERING & CONTROL LOGIC

### 4.1 Execution Loop and Timing Architecture
The software is written in C++ using the Arduino core framework within PlatformIO. To ensure non-blocking execution (preventing the system from freezing while waiting for a sensor to reply), the code utilizes the `millis()` function for state-machine timing rather than blocking `delay()` functions. The main loop cycles at high speed, handling Wi-Fi reconnection routines, MQTT client looping, time updates, and sensor polling at strictly defined intervals.

### 4.2 Automated Lighting Logic (Smart Photosynthesis)
The lighting subsystem operates on an intelligent threshold algorithm. The ESP32 continuously polls the LDR (Photoresistor) on GPIO34. 
* If the analog value exceeds the predefined threshold (e.g., `> 2000`, indicating the kitchen has become dark), the system asserts Relay 1 to the `LOW` state (engaging the relay), and simultaneously sends a FastLED command to illuminate the WS2813 strip to pure white (`CRGB::White`). 
* When ambient light returns (sensor value `< 2000`), the system writes `CRGB::Black` to the LEDs and disengages the relay to conserve municipal electricity.

### 4.3 Hardware-Level Overflow Protection (The 60% Rule)
The most critical software mechanism in the system is the overflow protection state machine. The HC-SR04 ultrasonic sensor continuously maps the distance to the water surface to a percentage. 
* **Safe State (0% - 59%):** The `safetyAllowed` boolean flag remains `true`. The user can press the physical button to engage the water pump.
* **Critical State (60% and above):** If the water level reaches 60%, the `safetyAllowed` flag is strictly forced to `false`. This triggers a hard software interlock. Even if the user physically holds down the hardware button on GPIO33, the system will actively drive the pump relay `HIGH` (OFF). This absolute override ensures the kitchen cannot be flooded under any circumstances.

### 4.4 Aeration Logic
Root health in hydroponics requires highly oxygenated water to prevent anaerobic bacteria growth. Therefore, the "Air Pumpe" is decoupled from any complex logic conditions and remains in a constant `ON` state, ensuring maximum dissolved oxygen in the nutrient solution at all times.

---

## 5. USER INTERFACE & EXPERIENCE (UI/UX)

### 5.1 Localized TFT Dashboard
The local interface is driven by an SPI-based ILI9341 TFT display. The screen is divided into a strict grid system to maximize readability from a distance.
* **Top Header:** Displays the current synchronized date and time (via NTP).
* **Left Column (Air Metrics):** Displays ambient Temperature (°C) and Humidity (%).
* **Right Column (Water Metrics):** Displays fluid Temperature (°C) and fluid Level (%). The text dynamically changes color (e.g., turning RED and displaying "LOW WATER" if the level drops below 10%).
* **Bottom Panel (System Status):** Displays the real-time ON/OFF state of the Light, Water Pump, and Air Pump. 

### 5.2 Cloud Telemetry (ThingsBoard)
Simultaneously, the ESP32 formats a JSON string containing all variables (e.g., `{"temperature": 22.5, "water_level": 45, "lightStatus": "ON "}`). This is published to the `v1/devices/me/telemetry` MQTT topic. The ThingsBoard cloud receives this data and populates a secure, remote dashboard accessible by school administration or IT staff on any web browser.

---

## 6. QUALITY ASSURANCE & TESTING PROTOCOLS

### 6.1 Unit Testing (Unittests)
To mathematically prove the reliability of the system's core logic, we decoupled the decision-making functions from the hardware inputs and executed a suite of automated Unit Tests using the **Unity Framework** within the PlatformIO environment. 

A total of 5 automated tests were successfully executed and passed (`[PASSED]`):
1. **Normal Pump Operation Test:** Validated that the pump activates when the button is pressed and the water level is simulated at a safe 45%.
2. **Overflow Protection Test:** *Critical Safety Test.* Validated that if the water level reaches 60%, the function strictly overrides an active button press simulation.
3. **Pump Release Test:** Confirmed the pump logic instantly disengages when the button state becomes inactive.
4. **Low-Light Activation Test:** Verified that simulated dark environments (sensor value 2500) correctly trigger a lighting state.
5. **High-Light Deactivation Test:** Verified that simulated bright environments (sensor value 1500) correctly turn off the lighting state.

### 6.2 User Acceptance Testing (Brugertest / UAT)
To evaluate the practical UI/UX, we conducted live User Acceptance Testing with a primary stakeholder: a non-technical worker from the school canteen.

**Methodology & Results:**
The canteen worker was presented with the physical prototype and the remote ThingsBoard dashboard. The user successfully and rapidly identified temperature readings and water levels. However, a critical usability flaw was discovered. The screen originally displayed the aeration system status under the acronym **"A-Pump"**. The canteen worker expressed confusion, logically assuming "A-Pump" stood for "Auto-Pumpe" (an automated water pump), completely misunderstanding its function as the root oxygenator. 

**Corrective Action:**
Relying on this invaluable user feedback, we modified the C++ source code and replaced the ambiguous acronym with the fully explicit text **"Air Pumpe"**. This iteration entirely resolved the confusion, proving that the system is now intuitively readable.

---

## 7. DEPLOYMENT, SAFETY & ENVIRONMENTAL CONSTRAINTS

Deploying bare electronics into a commercial kitchen setting requires strict physical safeguards:
* **Algae Prevention Strategy:** Because school kitchen windowsills receive heavy UV exposure, the primary water reservoir is constructed from 100% opaque materials. Preventing light penetration into the nutrient-rich water is mandatory to stop algae blooms from choking the internal water pump mechanics and plant root systems.
* **Electrical Waterproofing & Drip Loops:** All microcontrollers, relays, and power converters are housed in an elevated, splash-proof junction box. Crucially, all wiring and tubing exiting the water reservoir feature physical "drip loops" (a U-shaped bend in the wire). If condensation forms on a wire, gravity forces the water to drip off the bottom of the loop onto the floor, physically preventing moisture from wicking upward into the sensitive ESP32 circuitry.
* **Logic Isolation:** The use of the opto-isolated relay module ensures that the high-voltage flyback spikes generated when the mechanical water pump turns off do not travel back through the grounding planes and crash the ESP32.

---

## 8. BILL OF MATERIALS (BOM) & FUTURE SCALABILITY

### 8.1 Bill of Materials
| Item | Component Description | Primary System Function | Qty |
| :--- | :--- | :--- | :--- |
| 1 | ESP32 DevKit V1 | Main microcontroller, Wi-Fi processing, logic. | 1 |
| 2 | 4-Channel Relay (5V) | Opto-isolated high-voltage switching. | 1 |
| 3 | WS2813 LED Strip (12V) | Photosynthetic light generation. | 1m |
| 4 | BME280 Sensor Module | Atmospheric temperature and humidity tracking. | 1 |
| 5 | DS18B20 Probe | Submerged nutrient water temperature tracking. | 1 |
| 6 | HC-SR04 Ultrasonic | Water level distance tracking for overflow safety. | 1 |
| 7 | KY-018 Photoresistor | Ambient light tracking. | 1 |
| 8 | ILI9341 TFT Display | Localized UI dashboard rendering. | 1 |
| 9 | 12V DC Water Pump | Nutrient circulation. | 1 |
| 10 | 12V DC Air Pump | Continuous root oxygenation. | 1 |
| 11 | LM2596 Buck Converter | Steps main 12V supply down to clean 5V logic. | 1 |
| 12 | 12V/5A Power Supply | Primary system electrical source. | 1 |

### 8.2 Future Scope & Scalability
1. **Chemical Automation:** The integration of industrial glass pH probes and Electrical Conductivity (EC) sensors, paired with precise peristaltic dosing pumps, will allow the system to automatically adjust fertilizer ratios and acid/base balances based on real-time plant consumption.
2. **Physical Upscaling:** Transitioning from a single desktop reservoir to multi-tier vertical farming racks. Scaling relay ratings allows one ESP32 to safely orchestrate an entire wall of hydroponic growth within the school infrastructure. 
3. **Machine Learning:** Aggregating the massive amounts of telemetry data stored on ThingsBoard to train localized machine learning models capable of predicting the optimal lighting and watering schedules based on historical yield data for specific crop types.
