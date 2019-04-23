#ifdef USE_ONEWIRE_TEMP

/*************************************************************************************
Print address of DS18B20 sensor to serial
/*************************************************************************************/
void detectDallasAddr(void) 
{
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  DEBUGln(F("Searching for 1-Wire-Devices..."));
  oneWireBus.begin(ONE_WIRE_BUS);
  while(oneWireBus.search(addr)) 
  {
    g1WireDeviceFound = true;
    DEBUG(F("1-Wire-Device found with address: "));
    for( i = 0; i < 8; i++) 
    {
      DEBUG(F("0x"));
      if (addr[i] < 16) 
      {
        DEBUG(F("0"));
      }
      
      Serial.print(addr[i], HEX);
      if (i < 7) 
      {
        DEBUG(F(", "));
      }
    }
    if (OneWire::crc8( addr, 7) != addr[7]) 
    {
      DEBUGln(F("ERROR: CRC is not valid!"));
      return;
    }
  }
  DEBUGln();
  oneWireBus.reset_search();

  if (!g1WireDeviceFound)
    DEBUGln(F("There was no 1-wire device found."));
  
  return;
}

/*************************************************************************************
Read and globally save DS18B20 value
/*************************************************************************************/
void update1WireTemp()
{
  if (g1WireDeviceFound)
  {
    do {
      DS18B20.requestTemperatures(); 
      gTempFl = DS18B20.getTempCByIndex(0);
      //Serial.print("Temperature: ");
      //Serial.println(gTempFl);
    } while (gTempFl == 85.0 || gTempFl == (-127.0));
      
    //gTempStr = String(gTempFl, 2); 
    //sendMQTTmsg(mqttTopicTemp, String(gTempFl, 2), false);
  }
}

#endif
