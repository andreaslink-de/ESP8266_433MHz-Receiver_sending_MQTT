unsigned long gLastReceivedData = 0;

/*************************************************************************************
Read on 433MHz band and check, if there is some data available (received via 
Interrupt) and send it via MQTT
/*************************************************************************************/
/*
 Code-Berechnung, wenn Sabotage bekannt ist mit Bsp.:
   0 : '4194567' sabotage
  +3 : '4194570' open
  +4 : '4194574' close
  +58: '4194632' button hold
  +58: '4194690' button press
*/
void handleNewReadDataOn433MhzBand()
{
  if (receiver433MHz.available()) 
  {
    unsigned long receivedData = receiver433MHz.getReceivedValue();
    
    //Nothing useful detected, so leave the show
    if (receivedData <= 0)
      return;
      
    //Prevent double fired aka detected events
    if (gLastReceivedData == receivedData)
    {
      //No new data, so leaving the show
      return;
    } else {
      gLastReceivedData = receivedData;
    }

    SensorStatus sensStat = SENSOR_UNKNOWN;
    String sensorTopic;
    boolean sensorFound = false;

    /*
    Serial.print("Info: There is a buffer of ");
    Serial.print(MAX_AMOUNT_SENSORS);
    Serial.println(" sensors to analyse:");
    */
    
    //Step through array of sensors and check each value, if this is one of it
    for (int i=0; i<MAX_AMOUNT_SENSORS; i++)
    {
      sensStat = SENSOR_UNKNOWN;
      sensorTopic = "";

      if (sensors433MHz[i].isDefined())
      {    
        if (sensors433MHz[i].validateSensorID(receivedData, &sensorTopic, &sensStat))
        {
          /*
          DEBUGln(timestamp());
          Serial.print("  ID: ");
          Serial.println(receivedData);
          Serial.print("  Sensor Status: ");
          Serial.println(sensStat);
          Serial.print("  Topic: ");
          Serial.println(sensorTopic);      
          Serial.println();
          */

          //Sensor found, leaving the analysis
          sensorFound = true;
          break;
        }
      }
    }

    if (sensorFound)
    {
      DEBUGln();
      DEBUG(timestamp());
      Serial.print  (F(" Free Mem: "));
      Serial.print  (ESP.getFreeHeap());
      Serial.println(F(" Byte"));
      Serial.println("Yeah! Known sensor detected in config:");
      Serial.print("  Topic: ");
      Serial.println(sensorTopic);      
      Serial.print("  Sensor Status: ");
      Serial.print(sensStat);
      Serial.print(" -> ");
            

      //Convert Topic String to char-array
      char topicToSend[MAX_TOPIC_LENGTH];
      sensorTopic.toCharArray(topicToSend, sizeof(topicToSend));
      
      //Send data via MQTT
      switch (sensStat)
      {
        case SENSOR_OPEN:              
              Serial.println("SENSOR_OPEN");
              sendMQTTmsg(topicToSend, "1", true);
          break;
        case SENSOR_CLOSED:
              Serial.println("SENSOR_CLOSED");
              sendMQTTmsg(topicToSend, "0", true);
          break;
        case SENSOR_BTNPRESSED:
              Serial.println("SENSOR_BTNPRESSED");
              sendMQTTmsg(topicToSend, "1", false);
          break;
        case SENSOR_BTNHOLD:
              Serial.println("SENSOR_BTNHOLD");
              sendMQTTmsg(topicToSend, "1", false);
          break;
        case SENSOR_SABOTAGE:
              Serial.println("SENSOR_SABOTAGE");
              sendMQTTmsg(topicToSend, "1", true);
          break;
      }
      
      //Serial.println();
      ledDoubleFlash();
    } else {
      //No sensor was found in the setup, so just provide the detected code as bare code to the mqtt bus
      Serial.println();
      DEBUG(timestamp());
      Serial.print  (F(" Free Mem: "));
      Serial.print  (ESP.getFreeHeap());
      Serial.println(F(" Byte"));
      Serial.print("Unknown code received: ");
      Serial.println(receivedData);
      
      char topicToSend[MAX_TOPIC_LENGTH];
      //memset(topicToSend, 0, sizeof(topicToSend));
      sprintf(topicToSend, "%s/%s", mqttBaseTopic, "unknown-data"); //Concat topic
      sendMQTTmsg(topicToSend, String(receivedData), false);

      ledFlash();
    }
  }

  //Delete received value from queue to prepare for next one
  receiver433MHz.resetAvailable();
}


/*************************************************************************************
Learning new sensor data, if provided in the correct format: <BtnPushID>;<MQTT-Topic>
If data is correct, add it to the internal array of sensors, as soon as an action of
the sensor is recognized, provide the corresponding MQTT topic and value to the bus.
/*************************************************************************************/
boolean learnNewSensor(String pMQTTmessage)
{
  //sensors433MHz[0].initSensor(12674904, "DEVELOPMENT/zuhause/test-zimmer");

  //Prepare data
  String mqttMsg = pMQTTmessage;
  mqttMsg.trim();

  if (mqttMsg == "")
    return false;

  //Split string into PushBtnID-string and topic-string
  int seperatorPos = mqttMsg.indexOf(';');  //Finds location of first ";"
  String pushBtnIDStr = mqttMsg.substring(0, seperatorPos); //Grabs first data string up to seperator
  String topicStr = mqttMsg.substring(seperatorPos+1); //Grabs rest data string from seperator to end
    
  unsigned long pushBtnID = strtoul(pushBtnIDStr.c_str(), NULL, 10);

  if (pushBtnID <= 0)
    return false;

  //Debug:
  Serial.print("Going to add new sensor with button ID '");
  Serial.print(pushBtnID);
  Serial.print("' and MQTT-topic '");
  Serial.print(topicStr);
  Serial.println("' to internal setup array...");

  //First run, to ensure, the sensor is not already setup
  for (int i=0; i<MAX_AMOUNT_SENSORS; i++)
  {
    if (sensors433MHz[i].isDefined())
    {
      //Ensure, sensor is not already setup with a potentially different topic
      if (sensors433MHz[i].getButtonHoldID() == pushBtnID)
      {
        //Get full topic to report
        String currentTopic = sensors433MHz[i].getMQTTtopic();
        Serial.print("...aborted as sensor is already set up in slot ");
        Serial.print(i);
        Serial.print(" with current topic: ");
        Serial.println(currentTopic);
        return false;        
      }
    }
  }

  //If all fine so far step through all inactive array of sensors entries and find a free slot for the the given one
  for (int i=0; i<MAX_AMOUNT_SENSORS; i++)
  {
    if (!sensors433MHz[i].isDefined())
    {
      //Free slot is found, init it with new values
      sensors433MHz[i].initSensor(pushBtnID, topicStr);
      
      Serial.print("...done and successfully added new sensor to slot ");
      Serial.println(i);
      return true;
    } 
  }

  Serial.print("...done and FAILED to add the new sensor to an empty slot. Probably all ");
  Serial.print(MAX_AMOUNT_SENSORS);
  Serial.println(" slots are already used?");
  return false;
}


/*************************************************************************************
Deleting an existing sensor from the current config , if provided in the correct 
format: <BtnPushID>
If data is correct and ID is found in the config aka internal array of sensors, it
will be removed from the config and further on treated as an unknown device.
/*************************************************************************************/
boolean deleteExistingSensor(String pMQTTmessage)
{
  //Prepare data
  String mqttMsg = pMQTTmessage;
  mqttMsg.trim();

  if (mqttMsg == "")
    return false;

  //unsigned long pushBtnID = atol(mqttMsg.c_str());
  unsigned long pushBtnID = strtoul(mqttMsg.c_str(), NULL, 10);

  if (pushBtnID <= 0)
    return false;
    
  //Step through all active array of sensors entries and identify the given one
  for (int i=0; i<MAX_AMOUNT_SENSORS; i++)
  {
    if (sensors433MHz[i].isDefined())
    {
      if (sensors433MHz[i].deleteSensor(pushBtnID))
      {
        Serial.print("Deleted Sensor '");
        Serial.print(pushBtnID);
        Serial.println("' from setup.");
        return true;
      }
    }
  }
  
  return false;
}
