/*------------------------------------------------------------------------------------
 * ESP8266 - MQTT 433MHz Receiver with optional 1wire Temp Sensor (DS18B20)
 * 
 * Author: Dipl.-Inform. (FH) Andreas Link
 * Date: 18.04.2019, Germany
 * 
 * Usage:
 *   Boots, connects to WiFi, connects to MQTT server, subscribes to learn-topic and
 *   learns all new IDs which are provided. Then listens to any new 433MHz data 
 *   and provides it plain via MQTT, except it was learned before, then device is 
 *   handled as announced.
 *   If connected, temperature is measured as well and value provided to predefined 
 *   MQTT topic.
 *   
 * TODO:
 *   - Adding a simple webserver to also read values and potentially reboot device.
 *   - Load WiFi settings from external file (not stored on github :-))
 *
 ------------------------------------------------------------------------------------*/

#define DEBUG_OUTPUT      // Uncomment to enable serial IO debug messages
#define USE_ONEWIRE_TEMP  // Comment to deactivate Dallas DS18X20 temperature integration

/* *********************************************** <<< DEBUG >>> ****************************************************/
////////////////////////
// Debug Serial Magic //
////////////////////////
#ifdef DEBUG_OUTPUT
  #define DEBUG(input)   { Serial.print(input); }
  #define DEBUGln(input) { Serial.println(input); }
#else
  #define DEBUG(input) ;
  #define DEBUGln(input) ;
#endif

/* *********************************************** >>> DEBUG <<< ****************************************************/


#define FW_VERSION "1.0"

#define MQTT_MAX_PACKET_SIZE 396  //Remind: If you mess with this value and go too low, the code throws weird exceptions
#define ONE_WIRE_BUS  12          // 1Wire-Bus for DS18B20: pin 12
#define RECEIVER433MHZ_PIN  14    // 433MhZ-Receiver on interrupt 0 => that is pin #GPIO14 for ESP8266

#define MAX_AMOUNT_SENSORS  100   // Max amount of sensor to be installed, size can variy in relation to amount of memory being available

#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LinkTech_SensorMQTTlib.h>

//Read power via ADC
ADC_MODE(ADC_VCC);

/* *********************************************** <<< Config >>> ****************************************************/

// Set WIFI SSID and password
#define MY_WIFI_SSID "YOUR WIFI HERE"
#define MY_WIFI_PASSWORD "YOUR PASSWORD HERE"

//WiFi and network setup
const char* ssid = MY_WIFI_SSID;         
const char* password = MY_WIFI_PASSWORD; 


IPAddress ip = IPAddress(192,168,0,228);
IPAddress subnet = IPAddress(255,255,255,0);
IPAddress gw = IPAddress(192,168,0,254);
IPAddress dns = IPAddress(192,168,0,254);


// ----------------------------------------- MQTT ------------------------------------------ //

//MQTT-Topic Max-Length
#define MAX_TOPIC_LENGTH 100

//MQTT-Setup
const char* mqttServer = "192.168.0.253";

const char* mqttBaseTopic      = "zuhause/haus/433MHzReceiver";
const char* mqttTempTopic      = "zuhause/haus/abstellraum/raum/temperatur";

const char* mqttSubTopicDate   = "system/date";
const char* mqttSubTopicTime   = "system/time";


// ----------------------------------------------------------------------------------------- //

//Vars:
float gESPVCC = 0.0;
String gIP = "unknown";

String curDate = "";
String curTime = "";

String gMacAdr = "00:00:00:00:00:00";
unsigned long lastReconnectAttempt = 0;
uint8_t mqttConnectFailCounter = 0;


//Prepare announced sensors container
SensorMQTT sensors433MHz[MAX_AMOUNT_SENSORS];

//Define an ESP-WiFi-Client for MQTT-connection
WiFiClient espClient;
PubSubClient mqttClient(espClient);

//Defining 433MHz-Receiver-Object
RCSwitch receiver433MHz = RCSwitch();

#ifdef USE_ONEWIRE_TEMP
  #define TEMP_12_BIT 0x7F // 12 bit
  
  //Init 1Wire-Bus and PIN
  OneWire oneWireBus(ONE_WIRE_BUS);
  DallasTemperature DS18B20(&oneWireBus);
  boolean g1WireDeviceFound = false;
  
  float gTempFl = 0.0;
  unsigned long lastTempSent = 0;
#endif

/* *********************************************** Setup ****************************************************/
void setup()  
{
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println(F("-------------------------------------------------"));
  Serial.print  (F(" ESP8266-12F 433MHz-Receiver v"));
  Serial.println(FW_VERSION);
  Serial.print  (F("  ESP8266 Chip-ID: "));
  Serial.print  (ESP.getChipId());
  Serial.print  (F("  MAC Address: "));
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.print  (F("  Sketch size (kB): "));
  Serial.print  (ESP.getSketchSize()/1000.0);
  Serial.print  (F(" ("));
  Serial.print  (ESP.getSketchSize());
  Serial.println(F(" Byte)"));
  Serial.print  (F("  Free space (kB): "));
  Serial.print  (ESP.getFreeSketchSpace()/1000.0);
  Serial.print  (F(" ("));
  Serial.print  (ESP.getFreeSketchSpace());
  Serial.println(F(" Byte)"));
  Serial.print  (F("  Free Mem/Heap (kB): "));
  Serial.print  (ESP.getFreeHeap()/1000.0);
  Serial.print  (F(" ("));
  Serial.print  (ESP.getFreeHeap());
  Serial.println(F(" Byte)"));
  Serial.println();
  Serial.println(F("        Dipl.-Inform. (FH) Andreas Link"));  
  Serial.println(F("            Release Build 04.2019"));
  Serial.println(F("             www.andreaslink.de"));
  Serial.println(F("-------------------------------------------------"));
  Serial.println();

  // Prepare Debug-LED GPIO2 (TX1) for testing/measuring (IF NO SERIAL2 IS USED!)
  pinMode(LED_BUILTIN, OUTPUT); // Initialize digital pin LED_BUILTIN as an output.
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off (HIGH = off, LOW = on)
  //pinMode(2, OUTPUT); // LED vorbereiten
  //digitalWrite(2, 1); // Turn LED off
  
  ledDoubleFlash();

  // Read current voltage
  gESPVCC = ESP.getVcc()/1000.0;
  Serial.print(F(" Spannung: "));
  Serial.print(gESPVCC);
  Serial.println(F(" V"));


#ifdef USE_ONEWIRE_TEMP
  //pinMode(ONE_WIRE_BUS, INPUT_PULLUP); // Input and turn on a 20K pullup internally
  DEBUGln();
  DEBUGln(F("Analyzing 1-wire bus:"));
  detectDallasAddr();
  
  //Read first initial value of DS18B20 if found
  if (g1WireDeviceFound)
  {
    DEBUG(F("Init and read temp..."));
    DS18B20.begin();
    DS18B20.setResolution(TEMP_12_BIT); // Set precision to 12-Bit
    update1WireTemp();
    DEBUGln(F("done"));
  }
#endif  
  
  // Initial Wifi mode setup and connection to WiFi
  WiFiStart();

  //Init MQTT broker details for MQTT broker and setup callback
  Serial.print(F("Init/Prepare MQTT-server: "));
  Serial.print(mqttServer);
  Serial.println(F(":1883"));
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(mqttCallback);

  //Prepare 433MHz-Receiver
  DEBUG(F("Init/Prepare 433MHz reveiver module on pin '"));
  DEBUG(RECEIVER433MHZ_PIN);
  DEBUG(F("'... "));
  receiver433MHz.enableReceive(RECEIVER433MHZ_PIN);  // Receiver on interrupt 0 => that is pin #GPIO14 for ESP8266
  DEBUGln(F("done."));

  //DEMO-Testcase:
  //sensors433MHz[0].initSensor(12674904, "DEVELOPMENT/zuhause/test-zimmer");
  
  Serial.println(F("-- Initial setup is done --"));
  Serial.println();

}


/* *********************************************** Loop ****************************************************/
void loop() 
{
  unsigned long now = millis();
  
  //WiFi is still up:
  if (WiFi.status() != WL_CONNECTED)
  {
    DEBUG(timestamp());
    Serial.println(F("ERROR: WiFi lost! Reconnecting..."));
    WiFiStart();
  }

  // Handle MQTT messages
  if (!mqttClient.connected())
  {
    if (now < lastReconnectAttempt)
      lastReconnectAttempt = 0;
    
    if (now - lastReconnectAttempt > 5000) 
    {
      lastReconnectAttempt = now;
      
      // Attempt to reconnect
      if (mqttReconnect()) 
      {
        lastReconnectAttempt = 0;
        mqttClient.loop();
      }
      ledFlash();
      delay(200);
      ledFlash();
    }
  } else {
    // MQTT-Client connected
    mqttClient.loop();

    //Receive 433MHz data, if available and if something is received send it via MQTT
    handleNewReadDataOn433MhzBand();    
      

    #ifdef USE_ONEWIRE_TEMP
    if (now < lastTempSent)
      lastTempSent = 0;
    
    if ((now - lastTempSent > 60000) && g1WireDeviceFound)
    {
      lastTempSent = now;

      Serial.println();
      Serial.println(F("Reading temp from DS18B20 sensor..."));
      update1WireTemp();
      Serial.print(F("  Measured Temperature: "));
      Serial.print(String(gTempFl, 2));
      Serial.print(F(" "));
      //Serial.print((char)247);
      Serial.println(F("C"));
      
      //Send value via MQTT
      sendMQTTmsg(mqttTempTopic, String(gTempFl, 1), false);
    }
    #endif
  }

}
