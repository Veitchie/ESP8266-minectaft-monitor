/*
 * A nodeMCU (ESP8266) based minecraft server monitor
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <LinkedList.h>
#include <Servo.h>

const char* ssid = "name";
const char* password = "password";

unsigned long lastTime = 0;       //Last time function was called - Leave this alone
unsigned long timerDelay = 300000;//Default function delay - (currently 5 minutes)
unsigned long maxDelay = 900000;  //Time wait when checking the server when it's offline (ms) - (currently 15 minutes)
unsigned long minDelay = 300000;  //Time to wait if the server is online (ms) - (currently 5 minutes)

const int maxPlayerReqToGrabSample = 20;  //Servers with high player counts do not return samples, this seems to work
bool firstLog = true;                     //Skips the timer whenever this is true

String minecraftIP = "eu.mineplex.com";                         //Minecraft server IP - This is just a random public server
String address = "https://mcapi.us/server/status?ip=[address]"; //Minecraft API address

//Server info
//===========================================================================================

String pingStatus;      //Status of request returned by the API
bool online = false;    //Whether the requested server is online
int numPlayers = 0;     //Number of players currently on the server
int maxPlayers = 0;     //Maximum players for that server
int prevNumPlayers = 0; //Previous number of players on the server
LinkedList<String> players = LinkedList<String>();  //Usernames of players on the server
LinkedList<String> playerIDs = LinkedList<String>();//Player IDs of players on the server

//===========================================================================================
//LED and servo settings
//===========================================================================================

int leds[6] = {D1,D2,D3,D5,D6,D7};  //Pins used for LEDs - Change to whatever you need
int numLEDs = 6;                    //Number of pins in the above array - Change as needed
int servoPin = D4;                  //Pin used for the servo
Servo servo;                        //Servo object
float servoScale = 1.1;             //Servo position may not be accurate, use this to scale the position

//===========================================================================================

void connectToWifi(){
  //Connects to the wifi using the name and password at the beginning of this code
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  //LED and servo setup
  //[Sets all pins out outputs and to LOW]
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  for (int i = 0; i < numLEDs; i++){
    pinMode(leds[i],OUTPUT);
    digitalWrite(leds[i],LOW);
  }
  digitalWrite(leds[0],HIGH);

  servo.attach(servoPin);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //Wifi setup
  //[Create the request URL and connect to the wifi]
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  address.replace("[address]", minecraftIP);
  Serial.begin(9600); 

  connectToWifi();
  
  Serial.println("Using API available at: https://mcapi.us/");
  Serial.println("Sending request to " + address);
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

void updateServerInfo(){
  //If enough time has passed, attempt to retrieve server info
  
  if (millis() - lastTime > timerDelay or firstLog){
    firstLog = false;
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      
      HTTPClient http;  //Declare an object of class HTTPClient
      http.useHTTP10(true);
      WiFiClientSecure wifiClient;
      wifiClient.setInsecure();
      http.useHTTP10(true);
      http.begin(wifiClient, address);
       
      int httpCode = http.GET();
      if (httpCode > 0){
              
        DynamicJsonDocument doc(2048 * 8);
        deserializeJson(doc, http.getStream());

        pingStatus = doc["status"].as<String>();

        if (pingStatus.equals("success")){      //If the payload contains active server data
          online = doc["online"].as<bool>();
          
          if (online){
            timerDelay = minDelay;
            numPlayers = doc["players"]["now"].as<int>();
            maxPlayers = doc["players"]["max"].as<int>();
            
            if (prevNumPlayers != numPlayers){
              players.clear();
              playerIDs.clear();
              
              if (numPlayers != 0 and maxPlayers <= maxPlayerReqToGrabSample){
                Serial.println("Sampling players.");
                
                for (int i = 0; i < numPlayers; i++){
                  players.add(doc["players"]["sample"][i]["name"].as<String>());
                  playerIDs.add(doc["players"]["sample"][i]["id"].as<String>());
                }
              }
            }
          }else{
            timerDelay = maxDelay;
          }
          
        }else if (pingStatus.equals("error")){  //If the server is likely offline, so no data
          Serial.println("Server is offline.");
          online = false;
          numPlayers = 0;
          maxPlayers = 0;
          players.clear();
          playerIDs.clear();
          timerDelay = maxDelay;
        }else{                                  //If the payload was empty, likely too large to process into string
          Serial.println("No data.");
          Serial.println(http.errorToString(httpCode).c_str());
          online = false;
          numPlayers = 0;
          maxPlayers = 0;
          players.clear();
          playerIDs.clear();
          timerDelay = maxDelay;
        }
        
        lastTime = millis();
   
      }else{
        Serial.println("An error occurred sending the request");
        Serial.println(http.errorToString(httpCode).c_str());

        pingStatus = "HTTP error";
        online = false;
        numPlayers = 0;
        maxPlayers = 0;
        players.clear();
        playerIDs.clear();
        //String players[];
      }
   
      http.end();
   
    }
  }
}

void loop() {

  //Check for serial commands
  if(Serial.available()){
      String input = Serial.readStringUntil('\n');
      if(input.equals("ping")){
        firstLog = true;
      }
      else if(input.equals("info")){
        Serial.println("Status:\t" + String(online));
        Serial.println("Current players:" + String(numPlayers));
        Serial.println("Max players:\t" + String(maxPlayers));
      }
      else if(input.indexOf("server:") > -1){
        String newIP = input.substring(7);
        Serial.println("Changing server IP: " + newIP);
        address.replace(minecraftIP, newIP);
        Serial.println("Sending request to " + address);
        minecraftIP = newIP;
        firstLog = true;
      }
  }

  //Check wifi connetion
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Connection lost.");
    digitalWrite(leds[3],LOW);
    connectToWifi();
  }

  //Pretty self explanatory
  updateServerInfo();

  //LED and servo control - Change to display whatever information you want
  //======================================================================================
  if(online){
    digitalWrite(leds[1],HIGH);
    digitalWrite(leds[2],HIGH);
  }else{
    digitalWrite(leds[1],LOW);
    digitalWrite(leds[2],LOW);
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(leds[3],HIGH);
  }else{
    digitalWrite(leds[3],LOW);
  }
  if (numPlayers > 0){
    digitalWrite(leds[4],HIGH);
    digitalWrite(leds[5],HIGH);

    if (maxPlayers > 0){
      servo.write(180 - int((float(numPlayers) / float(maxPlayers)) * 180) * servoScale);
    }
  }else{
    digitalWrite(leds[4],LOW);
    digitalWrite(leds[5],LOW);
    servo.write(180);
  }
  //======================================================================================
  
}
