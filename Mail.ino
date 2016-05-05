
char server[] = "mail.smtpcorp.com";

#define HEADER_WELCOME "ESP8266 System powered on"
#define HEADER_ALARM "ESP8266 ALARM!"
#define HEADER_LIVE "ESP8266 System is working ok"
#define HEADER_LOW_BAT "ESP8266 System has low battery"

#define MAIL_FROM "sensorsiot@gmail.com"
#define MAIL_TO "andreas.spiess@arumba.com"


// my Chip IDs to give it a cleartype name
#define WA1 14117293
#define WA2 12612352


bool sendEmail(byte MailType)
{
  byte thisByte = 0;
  byte respCode;

#ifdef DEBUG_NO_SEND
  if (MailType == MAIL_WELCOME)
    Serial.println(F(HEADER_WELCOME));
  if (MailType == MAIL_ALARM)
    Serial.println(F(HEADER_ALARM));
  if (MailType == MAIL_LIVE_SIGNAL)
    Serial.println(F(HEADER_LIVE));
  if (MailType == MAIL_LOW_BAT)
    Serial.println(F(HEADER_LOW_BAT));

  return true;
#endif

  if (client.connect(server, 2525) == 1) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return false;
  }
  if (!eRcv()) return false;

  Serial.println(F("Sending EHLO"));
  client.println("EHLO www.example.com");
  if (!eRcv()) return false;
  Serial.println(F("Sending auth login"));
  client.println("auth login");
  if (!eRcv()) return false;
  Serial.println(F("Sending User"));
  // Change to your base64, ASCII encoded user
  // client.println(MAIL_USER);
  client.println(SMTP2goUSER); //<---------User in base64
  if (!eRcv()) return false;
  Serial.println(F("Sending Password"));
  // change to your base64, ASCII encoded password
  //  client.println(MAIL_PASS);
  client.println(SMTP2goPW);//<---------Password in base64
  if (!eRcv()) return false;
  Serial.println(F("Sending From"));
  // change to your email address (sender)
  client.print(F("MAIL From: "));
  client.println(F(MAIL_FROM));
  if (!eRcv()) return false;
  // change to recipient address
  Serial.println(F("Sending To"));
  client.print(F("RCPT To: "));
  client.println(F(MAIL_TO));
  if (!eRcv()) return false;
  Serial.println(F("Sending DATA"));
  client.println(F("DATA"));
  if (!eRcv()) return false;
  Serial.println(F("Sending email"));
  // change to recipient address
  client.print(F("To:  "));
  client.println(F(MAIL_TO));
  // change to your address
  client.print(F("From: "));
  client.println(F(MAIL_FROM));
  client.print(F("Subject: "));
  if (MailType == MAIL_WELCOME)
    client.println(F(HEADER_WELCOME));
  if (MailType == MAIL_ALARM)
    client.println(F(HEADER_ALARM));
  if (MailType == MAIL_LIVE_SIGNAL)
    client.println(F(HEADER_LIVE));
  if (MailType == MAIL_LOW_BAT)
    client.println(F(HEADER_LOW_BAT));

  client.println(F("This is from my ESP8266\n"));
  client.print(F("Power is: "));
  client.print(float(ESP.getVcc() * VCC_ADJ));
  client.println(F("mV"));
  int32 ChipID = ESP.getChipId();
  if (ChipID == WA1)
    client.println("Device: Water-Alarm 1");
  else if (ChipID == WA2)
    client.println("Device: Water-Alarm 2");
  else
  { client.print(F("Device Chip ID: "));
    client.println(ChipID);
  }


  client.println(F("."));
  if (!eRcv()) return false;
  Serial.println(F("Sending QUIT"));
  client.println(F("QUIT"));
  if (!eRcv()) return false;
  client.stop();
  Serial.println(F("disconnected"));
  return true;
}

byte eRcv()
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;

  while (!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return false;
    }
  }

  respCode = client.peek();
  while (client.available())
  {
    thisByte = client.read();
    Serial.write(thisByte);
  }

  if (respCode >= '4')
  {
    //  efail();
    return false;
  }
  return true;
}
