/********************************************************************************************\
  Connects ESP to WiFi and stores IP globally
  If connections fails 10 times, it reboots and retries.
/********************************************************************************************/
void WiFiStart()
{  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to: "));
  Serial.println(ssid);
  
  gIP = "unknown";
  unsigned int conCntr, rebootCntr = 0;

  WiFi.mode(WIFI_STA);
  //WiFi.config(ip, gw, subnet, dns); // Uncomment not to use DHCP
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    if (conCntr > 12)
    {
      conCntr = 0;
      rebootCntr++;
    }

    // Reboot, if there is no network accessable, it's probably an ESP issue
    if (rebootCntr > 10)
    {
      ESP.restart();
    }
    
    delay(500);
    Serial.print(F("."));
    
    conCntr++;
  }
  
  Serial.println();
  Serial.println(F("WiFi connected!"));
  Serial.println();

  //Init random generator
  randomSeed(micros());
  
  // Print the IP and MAC address
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("MAC address: "));
  Serial.println(WiFi.macAddress());
  Serial.println();
  
  
  //Update global Strings
  gIP = WiFi.localIP().toString();
  gMacAdr = (String)WiFi.macAddress();
}
