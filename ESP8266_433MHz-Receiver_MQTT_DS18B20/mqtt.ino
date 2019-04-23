/*************************************************************************************
Send a MQTT message to former connected MQTT broker
/*************************************************************************************/
boolean sendMQTTmsg(const char* pTopic, String pValue, boolean pRetained)
{
  if (mqttClient.connected())
  {
    DEBUG(F("Sending via MQTT... '"));
    //DEBUG(F("MQTT-Message: '"));
    DEBUG(pTopic);
    DEBUG(F("': '"));
    DEBUG(pValue);
    
    char msg[MAX_TOPIC_LENGTH];
    pValue.toCharArray(msg, sizeof(msg));
    
    boolean sendStatus = mqttClient.publish(pTopic, msg, pRetained);
    if (sendStatus)
    {
      DEBUGln("' ...done");
      //DEBUGln("' OK");
    } else {
      DEBUGln("' ...failed");
      //DEBUGln("' ERROR");
    }      
    //DEBUGln();
  
    return sendStatus;
  } else {
    DEBUGln(F("WARNING: Trying to send MQTT-data, but broker is currently not connected! (MQTT-Status: "));
    DEBUG(getMqttStatusCode());
    DEBUGln(F(")")); 
  }
  
  return false;
}


/*************************************************************************************
MQTT reconnect function, when connection to broker is lost 
with assignment to topic
/*************************************************************************************/
boolean mqttReconnect() 
{
  Serial.print(F("Attempting MQTT connection... (MQTT-Status: "));
  Serial.print(getMqttStatusCode());
  Serial.println(F(")"));
    
  // Create a random client ID
  String clientId = "esp8266-";
  clientId += String(random(0xffff), HEX);

  char topicStatus[MAX_TOPIC_LENGTH];
  sprintf(topicStatus, "%s/%s", mqttBaseTopic, "status"); //Concat topic
    
  // Attempt to connect
  //if (mqttClient.connect(clientId.c_str()))
  //boolean connect (clientID, willTopic, willQoS, willRetain, willMessage)
  if (mqttClient.connect(clientId.c_str(), topicStatus, 0, true, "offline"))  //Last will
  {
    DEBUG(F("MQTT broker connected as "));
    DEBUG(clientId);
    DEBUG(F(" (Status Code: "));
    DEBUG(getMqttStatusCode());
    DEBUGln(")");

    delay(100);
    
    mqttConnectFailCounter = 0;
 
    // Once connected, publish an announcement...
    //mqttClient.publish("zuhause/haus/osten/test", "hello world");
    sendMQTTmsg(topicStatus, F("online"), true);

    char topicVer[MAX_TOPIC_LENGTH];
    sprintf(topicVer, "%s/%s", mqttBaseTopic, "version"); //Concat topic
    sendMQTTmsg(topicVer, (String)FW_VERSION, false);

    char topicIP[MAX_TOPIC_LENGTH];
    sprintf(topicIP, "%s/%s", mqttBaseTopic, "ip"); //Concat topic
    sendMQTTmsg(topicIP, gIP, false);
    
    //Send MAC-Address
    char topicMAC[MAX_TOPIC_LENGTH];
    sprintf(topicMAC, "%s/%s", mqttBaseTopic, "mac"); //Concat topic
    sendMQTTmsg(topicMAC, gMacAdr, false);

    //Measure current voltage and send value
    gESPVCC = ESP.getVcc()/1000.0;
    char topicVoltage[MAX_TOPIC_LENGTH];
    sprintf(topicVoltage, "%s/%s", mqttBaseTopic, "voltage"); //Concat topic
    sendMQTTmsg(topicVoltage, String(gESPVCC, 3), false);
    
    DEBUGln();
    DEBUGln(F("Subscribing to MQTT topics... "));

    //Learn new sensor and store into config
    char topicLearnSensor[MAX_TOPIC_LENGTH];    
    sprintf(topicLearnSensor, "%s/%s", mqttBaseTopic, "learn-sensor/#"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicLearnSensor);
    DEBUGln("'");
    mqttClient.subscribe(topicLearnSensor);
    mqttClient.loop();

    //Delete existing sensor and remove from config
    char topicDeleteSensor[MAX_TOPIC_LENGTH];    
    sprintf(topicDeleteSensor, "%s/%s", mqttBaseTopic, "delete-sensor"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicDeleteSensor);
    DEBUGln("'");
    mqttClient.subscribe(topicDeleteSensor);
    mqttClient.loop();
    
    //Reboot ESP
    char topicReboot[MAX_TOPIC_LENGTH];    
    sprintf(topicReboot, "%s/%s", mqttBaseTopic, "command/reboot"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicReboot);
    DEBUGln("'");
    mqttClient.subscribe(topicReboot);
    mqttClient.loop();

    //LED ON/OFF
    char topicLED[MAX_TOPIC_LENGTH];    
    sprintf(topicLED, "%s/%s", mqttBaseTopic, "command/led"); //Concat topic
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(topicLED);
    DEBUGln("'");
    mqttClient.subscribe(topicLED);
    mqttClient.loop();

    //Subscribe to Date
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(mqttSubTopicDate);
    DEBUGln("'");
    mqttClient.subscribe(mqttSubTopicDate);
    mqttClient.loop();

    //Subscribe to Time
    DEBUG(F(" >Subscribe to:  '"));
    DEBUG(mqttSubTopicTime);
    DEBUGln("'");
    mqttClient.subscribe(mqttSubTopicTime);
    mqttClient.loop();

    DEBUGln();
    Serial.print(F("...done. MQTT is up and running now! (MQTT-Status: "));
    Serial.print(getMqttStatusCode());
    Serial.println(F(")"));    
    DEBUGln();
    
  } else {
    //Did not connect, count max 10, then reboot
    mqttConnectFailCounter++;

    if (mqttConnectFailCounter > 10)
    {
      Serial.println(F("ERROR: MQTT connect failed too often!"));
      Serial.print(F("Return Code: "));
      Serial.println(getMqttStatusCode());
      Serial.println(F("Rebooting..."));
      ESP.restart();
    }
  }
  
  return mqttClient.connected();
}



/*************************************************************************************
Returns current MQTT status as readable String for easier debugging in case of 
conncetion errors or broken connections during usage
/*************************************************************************************/
String getMqttStatusCode()
{
  /*
  * mqttClient.state()
  * -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
  * -3 : MQTT_CONNECTION_LOST - the network connection was broken
  * -2 : MQTT_CONNECT_FAILED - the network connection failed
  * -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
  *  0 : MQTT_CONNECTED - the client is connected
  *  1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
  *  2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
  *  3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
  *  4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
  *  5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
  */  

  int mqttCode = mqttClient.state();
  char retCode[42];

  switch (mqttCode)
  {
    case (0):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTED");      
      return String(retCode);
      break;
    case (-4):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTION_TIMEOUT");
      return String(retCode);
      break;
    case (-3):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECTION_LOST");
      return String(retCode);
      break;
    case (-2):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_FAILED");
      return String(retCode);
      break;
    case (-1):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_DISCONNECTED");
      return String(retCode);
      break;
    case (1):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_PROTOCOL");
      return String(retCode);
      break;
    case (2):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_CLIENT_ID");
      return String(retCode);
      break;
    case (3):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_UNAVAILABLE");
      return String(retCode);
      break;
    case (4):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_BAD_CREDENTIALS");
      return String(retCode);
      break;
    case (5):
      sprintf(retCode, "%d: %s", mqttCode, "MQTT_CONNECT_UNAUTHORIZED");
      return String(retCode);
      break;      
  }

  // If nothing fits, return the integer as String
  return String(mqttCode);
}

/*************************************************************************************
MQTT callback function, when receiving a MQTT message
--> Handle incoming MQTT events
/*************************************************************************************/
void mqttCallback(char* topic, byte* payloadByte, unsigned int len)
{
    //No data, no interest
    if (len == 0) return;

    // Prepare Payload into char* "message"
    char* payload = (char *) payloadByte;
    char message[len + 1];
    strlcpy(message, (char *) payloadByte, len + 1);

    String topicStr = String(topic);

    //Date
    if (topicStr.startsWith(mqttSubTopicDate))
    {
      curDate = String(message);
    }

    //Time
    if (topicStr.startsWith(mqttSubTopicTime))
    {
      curTime = String(message);
    }

    //Reboot
    char checkTopic[MAX_TOPIC_LENGTH];
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/reboot");
    if (topicStr.startsWith(checkTopic))
    {
      //Reboot detected
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: ESP-Reboot via MQTT received with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' as value triggers reboot of device)"));
            
      if (value == 1)
      {
        DEBUG(F("Restarting ESP8266..."));
        delay(500);
        ESP.restart();
      }      
    }

    // LED-ON/OFF
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "command/led");
    if (topicStr.startsWith(checkTopic))
    {
      unsigned char value = relayParsePayload(payload);
      //DEBUG("Parsed value (0=off,1=on,2=toggle): ");
      DEBUG(timestamp());
      DEBUG(F("MQTT: LED command via MQTT received with value: "));
      DEBUG(value);
      DEBUGln(F(" ('1' turns LED on, '0' turns LED off, '2' toggles LED)"));
            
      if (value == 1)
      {
        DEBUGln(F("Switching LED to ON"));
        digitalWrite(LED_BUILTIN, LOW); // LED on
      } else if (value == 0)
      {
        DEBUGln(F("Switching LED to OFF"));
        digitalWrite(LED_BUILTIN, HIGH); // LED off
      } else if (value == 2)
      {
        DEBUG(F("Toggle LED: "));
        int curStat = digitalRead(LED_BUILTIN);

        if (curStat == HIGH)
        {
          DEBUGln(F("Switching LED to ON"));
          digitalWrite(LED_BUILTIN, LOW); // LED on          
        } else {
          DEBUGln(F("Switching LED to OFF"));
          digitalWrite(LED_BUILTIN, HIGH); // LED off
        }
      }
    }

    //Learning a new sensor
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "learn-sensor"); //Concat topic    
    if (topicStr.startsWith(checkTopic))
    {      
      String messageStr = String(message);
      
      DEBUG(timestamp());
      DEBUG(F("MQTT: There is a new sensor to learn with data provided: '"));
      DEBUG(messageStr);
      DEBUGln(F("'"));

      //Provide data to store it in the array of known sensor objects
      if (learnNewSensor(messageStr))
      {
        DEBUGln(F("OK: Sensor successfully added to config."));
      } else {
        DEBUGln(F("ERROR: Sensor could not be added to config."));
      }
      DEBUGln();
    }

    //Deleting an existing sensor
    memset(checkTopic, 0, sizeof(checkTopic));
    sprintf(checkTopic, "%s/%s", mqttBaseTopic, "delete-sensor"); //Concat topic    
    if (topicStr.startsWith(checkTopic))
    {      
      String messageStr = String(message);
      
      DEBUG(timestamp());
      DEBUG(F("MQTT: Sensor with following data is going to be deleted from the config: '"));
      DEBUG(messageStr);
      DEBUGln(F("'"));

      //Provide data to store it in the array of known sensor objects
      if (deleteExistingSensor(messageStr))
      {
        DEBUGln(F("OK: Sensor successfully removed from config."));
      } else {
        DEBUGln(F("WARNING: Sensor could not be found and therefore not be removed from config."));
      }
      DEBUGln();
    }
}

    
//Parses Relais status:
//  Payload could be "OFF", "ON", "TOGGLE"
//  or its number equivalents: 0, 1 or 2
unsigned char relayParsePayload(const char* payload) 
{
    // Payload could be "OFF", "ON", "TOGGLE"
    // or its number equivalents: 0, 1 or 2

    if (payload[0] == '0') return 0;
    if (payload[0] == '1') return 1;
    if (payload[0] == '2') return 2;

    unsigned int lenPl = strlen(payload);
    if (lenPl>6) 
      lenPl=6;
    
    //Only focus on relevant payload
    char pyld[7];
    strncpy(pyld, payload, lenPl);
    pyld[6] = '\0';

    // trim payload
    char * p = ltrim((char *)pyld);

    // to lower
    unsigned int l = strlen(p);
    //if (l>6) l=6;
    for (unsigned char i=0; i<l; i++) 
    {
        p[i] = tolower(p[i]);
    }

    String compare = (String)p;
    unsigned int value = 0xFF;
    
    if (compare.startsWith("off")) { 
      value = 0;
    } else if (compare.startsWith("on")) { 
      value = 1;
    } else if (compare.startsWith("toggle")) { 
      value = 2;
    }

    return value;
}



/********************************************************************************************\
  Left trim (remove leading spaces)
/********************************************************************************************/
char* ltrim(char* s) 
{
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}
