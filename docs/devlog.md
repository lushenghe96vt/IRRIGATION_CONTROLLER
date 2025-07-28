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

07/17/2025
- completed IRRIGATION_SENSOR to successfully post to server
- ran into non static ip problem
- could fix issue manually on bootup every time or could switch to a different way to connect to server
- maybe run own wifi server on board?

07/18/2025
- reconfigured main board to act as AP instead of using home wifi network, also fixed IP changing issue as esp32 uses default IP 192.168.4.2
- changed moisture_level to dryness_level so that it makes more sense
- need to get rid of magic numbers and find a way to make a web dashboard, do not want to mess with cloud stuff

07/21/2025
- cleaned up main.c in controller
- adjusted sensors array to live inside a new struct sensorContext so that it can stay in main and can be passed as argument into http handler

07/22/2025
- powered sensor nodes with batteries and successfully transmitted data
- started development of webserver, having troubles with uploading data ever x amount of seconds from control board to webserver

07/27/2025
- changed start_http_server to start both servers, one for HTTP_POST (send moisture data) and one for HTTP_GET (recieve moisture data and send to webserver)
- works but method is very crude, polls the /data server every second for json, and also not scaleable since the html is hardcoded, could change to websockets?
- web server up and running
- need to get both boards running on battery then figure out valve circuit next
- found an issue for when I start the AP after the sensors, the server does not recieve any data
- also need to test both sensors at the same time to ensure they work independantly
