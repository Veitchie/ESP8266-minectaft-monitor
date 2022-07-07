# ESP8266-minectaft-monitor
## _NodeMCU (ESP8266) minecraft server monitor_

![alt text](https://github.com/Veitchie/ESP8266-minectaft-monitor/blob/main/images/photo%20-%20front.jpg?raw=true)

A servo motor and 6 LEDs controlled with a NodMCU (ESP8266) microcontroller
to display minecraft server info.

The servo moves the arm to indicate the current number of players (current / max)
while the LEDs indicate power, wifi connection, server status, and player count.

The API used can get:
- Server status
- Current players
- Max players
- A sample of current player usernames and IDs

## Hardware requirements
- NodeMCU (ESP8266) microcontroller
- HK-5330 Servo motor
- 6x 3mm LEDs

## Arduino library requirements
- ESP8266WiFi
- ESP8266HTTPClient
- WiFiClientSecure
- ArduinoJson
- LinkedList
- Servo

## Photos
![alt text](https://github.com/Veitchie/ESP8266-minectaft-monitor/blob/main/images/render%20-%20top.jpg?raw=true)
![alt text](https://github.com/Veitchie/ESP8266-minectaft-monitor/blob/main/images/render%20-%20front.jpg?raw=true)
