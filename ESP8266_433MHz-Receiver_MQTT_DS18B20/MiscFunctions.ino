/*************************************************************************************
Blue On-Board LED short flash
/*************************************************************************************/
void ledFlash()
{
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(130);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
}


/*************************************************************************************
Blue On-Board LED double flash
/*************************************************************************************/
void ledDoubleFlash()
{
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(70);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
    delay(50);
    digitalWrite(LED_BUILTIN, LOW); // LED on
    delay(70);
    digitalWrite(LED_BUILTIN, HIGH); // LED off
}

/*************************************************************************************
Return current date/time timestamp if received from bus
/*************************************************************************************/
String timestamp()
{
  if (curDate == "")
    return "";
    
  return curDate + " " + curTime + ": ";
}
