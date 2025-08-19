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
- also need to test both sensors at the same time to ensure they work independantly'

08/11/2025
- ordered parts for valve control
- changed circuit for valve
- added boost converters and ditched 12V battery design
- still need to recieve parts and test if battery charging works
- then need to actually turn on valve
- then right control logic for valve
- also need to refine http webserver, make data transfer more secure and add 2 way communication with controls, look into websockets

08/13/2025
- built valve circuit
- started solar panel integration

08/14/2025
- updated valve control logic
- included freertos functions and queue to recieve and send data
- updated http_server.c to implement freertos and to prepare it for websockets
- found bug in sensor code, it only attempts to connect to AP wifi once on bootup and never again, must fix as well as refine the sensor code to match the controller code

08/16/2025
- updated all sensor node logic main.c and wifi initialization
- updated dryness threshoolds in controller main and main control loop Hz
- encountered a weird bug where sensor does not work on battery anymore, even reverting back to the old sensor code, the battery does not work anymore does not connect to AP
- need to figure out a new way to power boards, with usb c battery now, nevermind fixed it used 3v3 port instead of VIN so the board powers now. Also the driver circuit was messed up and now switched to 3v3 isntead of using VIN to power relay. Now entire configuration works
- now ready for websockets
- need to re order wires?
- need to reconfigure output to relay from 3V3 to a gpio output pin in order to power board with battery in future

08/18/2025
- configured websockets, everything works
- changed relay power to D5 from 3v3
- implemented button on webpage for manual override
- changed dryness to take average
- added a display for whether the valve is open or closed and made webpage look prettier
