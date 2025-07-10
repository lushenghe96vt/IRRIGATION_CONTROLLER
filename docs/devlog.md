06/20/2025
- made git repo
- set up project structure
- set up platformIO with native board to simulate while board come in
- set up roadmap

06/24/2025
- Finalized parts list
- Finalized road map

06/30/2025
- Found pin layout
- Blinked LED
- Powered relay
- Relay doesn't switch, requires 5V supply
- Ordered 3.3V relay along with 12V battery for valve

07/02/2025
- Started working on moisture sensors
- Configured moisture sensor raw readings

07/03/2025
- implemented sensor class structure
- began on http transmission

07/04/2025
- development in wifi transmission
- could not get anything to connect to home wifi

07/07/2025
- resolved connection error, but json parsing is wrong
- server sends undefined errors when json payload is correctly formatted

07/08/2025
- completely reorganized codebase to feature two different sub repos for main controller and sensors
- nothing worked so reverted to previous state

07/10/2025
- successfully set up server on main board to recieve posts from home wifi
- began sensor node posts directly to main board
- set up new repo for sensor node code for simplicity, IRRIGATION_SENSOR
- renamed repo from WATERINGSYSTEM to IRRIGATION_CONTROLLER 