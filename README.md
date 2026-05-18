Project Phase: Proof of Concept (PoC)
Target Environment: School Canteen Kitchen
Core Microcontroller: ESP32 DevKit V1 
  1. Project Overview & Purpose
This System Design Document (SDD) outlines the technical specifications, physical architecture, and software logic for an automated IoT-enabled hydroponic system. Designed specifically as a Proof of Concept (PoC) for a school canteen kitchen environment, the system aims to automate the intensive cultivation parameters of fresh herbs (such as basil) while minimizing daily manual oversight. By integrating smart sensor arrays, local telemetry feedback, and cloud synchronization, the system establishes an energy-efficient, resilient growth environment tailored for non-technical users. 
  2. Target Audience and User Interaction
2.1 Target Audience
The primary end-users of the system are the school canteen kitchen staff. Since the device operates in an active, fast-paced kitchen environment, it is intentionally designed for operators who do not possess specialized technical knowledge in electronics, microcontrollers, or programming. Consequently, the operational interface must be frictionless, requiring zero manual configuration during standard routines. 
2.2 User Interaction & Automation Level
The system runs on a fully automated loop. Canteen personnel do not need to manually trigger lighting schedules, aeration cycles, or water distribution loops. Instead, human interaction is restricted to passive, observational check-ins: 
•	Daily Monitoring: Once per shift or day, a canteen staff member checks the integrated local TFT display.
•	Visual Telemetry Feedback: The screen delivers at-a-glance status updates regarding environmental conditions and safety thresholds, prominently indicating critical metrics such as reservoir water capacity (e.g., "Water Level: OK" or "REFILL REQUIRED").
By eliminating manual handling, the system seamlessly provides fresh produce to the canteen kitchen without adding administrative or labor overhead to the daily kitchen schedule. 
  3. Hardware Architecture & Pin Mapping
3.1 Hardware Components
The PoC hardware layout consists of modular components chosen for power efficiency, precision, and reliable deployment over continuous operational cycles: 
•	Central Microcontroller: ESP32 (DevKit V1) – processes sensor inputs, handles scheduling loops, drives local visualization, and establishes internet telemetry links.
•	Actuator Interface: 4-Channel Relay Module (5V) – safely isolates and toggles the higher voltage 12V lines for pumps and lighting.
•	Lighting Array: WS2815 Addressable LED Strip (12V) – delivers customizable full-spectrum photosynthetic lighting.
•	Sensor Array: KY-018 Photoresistor (LDR) Module for ambient light evaluation; DHT22 for ambient atmospheric monitoring; and DS18B20 for direct liquid core temperature tracking.
3.2 Pin Configuration Table
To ensure structural replicability and clear resource allocation for future scaling, the physical connections between the ESP32 and peripheral hardware nodes are defined below:

Component	
Component

Pin	ESP32 GPIO
Pin	Connection Type / Purpose
KY-018 LDR
Sensor	S (Signal)	GPIO34 (D34)	Analog Input (Ambient Light Intensity Tracking)
4-Channel Relay	IN1 (Channel 1)	GPIO25 (D25)	Digital Output (12V Main Lighting Line Toggle)
WS2815 LED Strip	DI (Data In)	GPIO32 (D32)	Digital Output (FastLED SPI Data Stream
Control)
WS2815 LED Strip	BI (Backup In)	GND	Tied to Common Ground for Signal Stability
  5. Software Logic, Hardware Requirements, and Safety
4.1 Core Automation & Control Logic
The automation architecture utilizes localized conditional loops executed continuously by the ESP32 core: 
•	Smart Lighting (Grow Light: Relay 1 + Light Sensor): Operates via a hybrid, energy-conserving protocol.
The grow lights engage only if two parameters are met simultaneously: 
1.	Time Window: The local time is within the active photosynthetic daylight window (06:00 to 22:00), synchronized via Network Time Protocol (NTP).
2.	Natural Light Deficit: The LDR sensor reports ambient kitchen lighting levels dropped below a specified dark threshold (e.g., cloudy weather or evening dusk).
If solar illumination becomes sufficient, or the time crosses past 22:00, the system instantly suspends Relay 1 to conserve energy. 
•	Aeration Control (Air Pump: Relay 2): To ensure optimal root oxygenation and combat pathogen development, the air pump operates on a strict, continuous 24/7 cycle (30 minutes ON, 15 minutes OFF), fully decoupled from lighting states.
•	Manual Circulation (Water Pump: Relay 3 + Button): Pressing a physical chassis button forces the water pump to engage for exactly 10 seconds to mix added nutrients or demonstrate circulation. A hardware-timed software debounce routine is embedded to avoid erratic triggers or contact spam.
4.2 Data Collection (Sensors)
•	LDR Photoresistor: Polled every 2-3 seconds. Data streams pass through a software moving-average smoothing filter to prevent grow-light flickering caused by moving shadows or temporary personnel obstructions.
•	DHT22 Air Temp/Humidity: Polled every 10 seconds. Monitors ideal climate brackets (Target: 20–25°C, 40– 60% RH).
•	DS18B20 Water Temp: Waterproof probe polled every 10 seconds. Monitors water conditions (Target: 18– 22°C).
4.3 User Interface (TFT Dashboard)
The integrated display updates its layout every 5 seconds, displaying: Local Time, Wi-Fi connectivity status, atmospheric readings, core water temperatures, visual ambient status icons ("Sun" / "Dark"), and current relay statuses (ON/OFF). It also retains a sticky timestamp log of the last manual fertilization/circulation pump trigger. 
4.4 Cloud Integration (ThingsBoard / MQTT)
•	Network Resilience: Upon local Wi-Fi or router dropout, the ESP32 seamlessly switches into standalone offline execution mode, running all schedules via its internal hardware timers without freezing.
•	Telemetry: Environmental telemetry lines are package-serialized and pushed via MQTT to a centralized ThingsBoard dashboard every 60 seconds. Critical relay state transitions override this timer and post telemetry payloads instantly upon execution.
4.5 Hardware and Electrical Requirements
The main electrical input is delivered by a regulated 12V DC (5A) power brick. An LM2596 step-down buck converter steps down the 12V main bus down to a stable 5.0V to drive the ESP32 logic core, sensor pull-ups, and the relay coils. The relay module opto-isolates the lower-voltage logic traces from the high-draw inductive loads of the pumps, preventing back-EMF spikes from crashing the microcontroller. 
4.6 Construction and Safety (Critical Constraints)
•	Absolute Light Isolation (Algae Prevention): Given the high-sun exposure on school kitchen window sills, the water tank must be 100% opaque (blacked out or foil-shielded). Light penetration into nutrient-rich water leads to rapid algae blooms, choking root systems.
•	Enclosure Waterproofing: The controller, buck converters, and relay arrays are isolated inside a sealed plastic junction enclosure mounted above high-splash zones to shield traces from kitchen steam or window condensation.
•	Cable Management (Drip Loops): All physical wiring and aeration tubes exiting the moist tank boundaries must incorporate structural downward "drip loops" prior to breaching the electronics container, ensuring traveling moisture sheds safely onto the floor rather than wicking into the PCB contacts.
  5. Success Criteria and Testing Strategy
5.1 Scope of Testing and Limitations
Due to development timeline constraints, tracking a full biological vegetative cycle from seed to harvest is excluded from this verification loop. Instead, mature, store-bought basil units are introduced physically to validate real-world spacing, foliage shadow footprints, and root submersion. Testing focuses strictly on evaluating system up-time stability, automated control loops, and user interactions. 
5.2 Technical Success Criteria
•	System Stability: Continuous, crash-free execution across a fixed 72-hour stress-test window without requiring manual resets or encountering stack overflow errors.
•	Logic Loop Validation: Flawless activation of targeted relays when physical sensor signals are artificially brought past threshold parameters.
•	Telemetry Fidelity: Constant data packet delivery to the ThingsBoard server via MQTT without transmission drops or structural data corruption.
5.3 User Acceptance Testing (UAT)
Qualitative post-test reviews will be performed with canteen kitchen operators to rate dashboard ergonomics, checking text legibility, font size adjustments, and clarity of system warning states in a high-stress workspace. 
5.4 Advanced Edge-Case and Physical Testing
•	Fault Tolerance (Offline Mode Test): The school Wi-Fi router link will be cut mid-cycle. The ESP32 must continue internal timekeeping and execute lighting/pump operations autonomously.
•	Power Recovery Test: The 12V mains connection will be disconnected during active pump operation. Upon reconnection, the system must cleanly boot, parse current states, self-heal network handshakes, and safely resume targeted automation states.
•	Leak and Water Routing Test: The water circulation line will run continuously for 10 minutes to verify pressurized joints and confirm condensation sheds perfectly via the drip loops.
•	UI Resilience (Debounce Test): The manual activation button will be subjected to high-frequency mechanical inputs. The software must filter out physical bounce noises and limit the action to one clean 10-second run.
  6. Bill of Materials (BOM)

	 
Item
#	Component Name	Primary Function in Project	Qty
1	ESP32 DevKit V1		1
Item
#	Component Name	Primary Function in Project	Qty
		Central microcontroller for sensor processing and automation logic.	
2	4-Channel Relay Module (5V)	Electronically switches the 12V power lines for lights and pumps.	1
3	WS2815 Addressable LED Strip
(12V)	Provides full-spectrum grow light for the plants.	1m
4	KY-018 Photoresistor Module	Measures ambient light to trigger the smart lighting loop.	1
5	DHT22 Temperature & Humidity
Sensor	Monitors climate parameters inside the kitchen area.	1
6	DS18B20 Waterproof Sensor	Measures water temperature inside the nutrient reservoir.	1
7	TFT Display (SPI)	Local dashboard showing real-time telemetry to canteen staff.	1
8	12V DC Water Pump	Handles nutrient solution circulation (triggered by manual button).	1
9	12V Air Pump + Air Stone	Provides continuous oxygenation to prevent root rot.	1
10	LM2596 Step-Down Buck
Converter	Converts 12V main power down to stable 5V for logic electronics.	1
11	12V Main Power Supply Adapter	Main wall adapter to power the entire system.	1
12	Solderless Breadboard & Jumper
Wires	Used for quick, secure prototyping and shared GND connections.	1
13	Opaque Water Tank & Tubing	Structural reservoir protected from light to prevent algae.	1
  8. Future Scope & Scalability
7.1 Advanced Chemical & Water Automation
•	Automated Nutrient and pH Dosing: Integration of glass pH probes and electrical conductivity (EC) sensors paired with industrial peristaltic pump heads to automate fertilizer adjustments without human calculations.
•	Auto-Refill Infrastructure: Direct plumbing coupling via solid-state solenoid valves and low-draw physical float switches to completely automate fluid replenishment routines.
7.2 Physical and Structural Upscaling
• Multi-Tier Vertical Racks: Transitioning the isolated PoC footprint into a modular, vertically stacked array. Scaling up relay ratings and migrating to heavy-duty industrial 24V power distribution lines allows one ESP32 to safely sync growth tasks across multi-level racks.
7.3 Software and User Experience Enhancements
•	Dedicated Mobile/Web Application: Deployment of a cross-platform mobile UI built over WebSockets or
MQTT, delivering push notifications, remote overrides, and direct chart viewing to the head chef's smartphone.
•	Data-Driven Growth Optimization: Aggregating historical environmental log entries to build light-cycle and watering profiles customized per crop type via simple machine learning algorithms.
