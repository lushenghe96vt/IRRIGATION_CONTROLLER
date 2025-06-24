Road Map

1. Build Main Control Unit (Valve Controller)
 Hardware Setup
 - Connect ESP32 to relay module
 - Wire 12V solenoid valve to relay
 - Build 11.1V battery system: battery + CN3791 charger + 12V solar panel
 - Step down voltage via buck converter to power ESP32 (5V or 3.3V)
 - Enclose in weatherproof housing
 Software Setup
 - Write valve control logic (open/close timing, scheduling)
 - Implement Wi-Fi communication for HTTP server

2. Build Moisture Sensor Nodes (x2)
 Hardware Setup
 - Build 3.7V battery + TP4056 charger + 5V solar panel
 - Connect ESP32 to: Battery power and Moisture sensor (analog pin)
 - Compact into housing for outdoor placement
 - Confirm moisture sensor voltage range (3.3–5V)
 Software Setup
 - Write logic to:
    - Read analog value from moisture sensor
    - Read battery voltage
 - Implement ESP-NOW transmission to main controller
 - Transmit every X minutes (deep sleep if needed)

3. Integrate Sensor Readings into Main Unit
 - Implement ESP-NOW receiver on main ESP32
 - Decode struct from sensor nodes
 - Log most recent values per sensor
 - Implement decision logic:
    - If any sensor is below threshold, turn valve ON
    - Otherwise, keep valve OFF
    - Allow HTTP override to manually override ESP-NOW logic

4. System Integration and Testing
 - Test each sensor independently
 - Verify main control logic with mock sensor data
 - Power test: ensure solar panels can keep battery charged
 - Leak test + weatherproofing
 - Add failsafe modes (e.g., shut off valve if no data received in 10+ mins)

5. Web Dashboard
 - Display:
    - Valve status (ON/OFF)
    - Moisture readings (sensor 1 & 2)
    - Battery levels (sensor 1 & 2)
    - Event log (valve toggles + moisture at time of action)
 - Add manual override buttons (Valve ON / OFF)
 - Use endpoints:
    - GET /status → JSON with sensor + valve data
    - POST /valve/on and /valve/off → manual control
    - GET /log → event history
 - Store 10–20 log entries in memory (with timestamp + sensor snapshot)
 - Update data periodically with AJAX (setInterval)

