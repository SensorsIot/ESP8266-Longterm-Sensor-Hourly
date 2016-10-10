
#include <ESP8266WiFi.h>
ADC_MODE(ADC_VCC); //vcc read-mode

// Include API-Headers
extern "C" {
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "cont.h"
}

#include <credentials.h>;


// state definietions
#define STATE_COLDSTART 0
#define STATE_SLEEP_WAKE 1
#define STATE_ALARM 2
#define STATE_CONNECT_WIFI 4

#define ALARM_PIN 14                                 // pin to test for alarm condition
#define ALARM_POLARITY 1                            // signal representing alarm
#define SLEEP_TIME 60*1*1000000                    // sleep intervalls in us
#define SLEEP_COUNTS_FOR_LIVE_MAIL 24*2*7           // send live mail every 7 days
#define SLEEP_COUNTS_FOR_BATT_CHECK 2*24            // send low-Batt mail once a day
#define BATT_WARNING_VOLTAGE 2.4                    // Voltage for Low-Bat warning
#define WIFI_CONNECT_TIMEOUT_S 20                   // max time for wifi connect to router, if exceeded restart


// RTC-MEM Adresses
#define RTC_BASE 65
#define RTC_STATE 66
#define RTC_WAKE_COUNT 67
#define RTC_MAIL_TYPE 68

// mail Types
#define MAIL_NO_MAIL 0
#define MAIL_WELCOME 1     // mail at startup
#define MAIL_LIVE_SIGNAL 2
#define MAIL_ALARM 3
#define MAIL_LOW_BAT 4

#define SPARKFUN_BATTERY 1

#define VCC_ADJ 1.096

#define SERIAL_DEBUG
//#define DEBUG_NO_SEND

// global variables
byte buf[10];
byte state;   // state variable
byte event = 0;
uint32_t sleepCount;
uint32_t time1, time2;

// Temporary buffer
uint32_t b = 0;

int i;

WiFiClient client;

void setup() {
  WiFi.forceSleepBegin();  // send wifi to sleep to reduce power consumption
  yield();
  system_rtc_mem_read(RTC_BASE, buf, 2); // read 2 bytes from RTC-MEMORY

  // if serial is not initialized all following calls to serial end dead.
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println(F("Started from reset"));
#endif

  if ((buf[0] != 0x55) || (buf[1] != 0xaa))  // cold start, magic number is nor present
  { state = STATE_COLDSTART;
    buf[0] = 0x55; buf[1] = 0xaa;
    system_rtc_mem_write(RTC_BASE, buf, 2);
  }
  else // reset was due to sleep-wake or external event
  { system_rtc_mem_read(RTC_STATE, buf, 1);
    state = buf[0];
    if (state == STATE_SLEEP_WAKE) // could be a sleep wake or an alarm
    { pinMode(ALARM_PIN, INPUT);
      bool pinState = digitalRead(ALARM_PIN);  // read the alarm pin to find out whether an alarm is teh reset cause
      Serial.printf("GPIO %d read: %d\r\n", ALARM_PIN, pinState);
      if (pinState == ALARM_POLARITY)
      { // this is an alarm!
        state = STATE_ALARM;
      }
    }
  }

  // now the restart cause is clear, handle the different states
  Serial.printf("State: %d\r\n", state);

  switch (state)
  { case STATE_COLDSTART:   // first run after power on - initializes
      sleepCount = 0;
      system_rtc_mem_write(RTC_WAKE_COUNT, &sleepCount, 4); // initialize counter
      buf[0] = MAIL_WELCOME;
      system_rtc_mem_write(RTC_MAIL_TYPE, buf, 1); // set a welcome-mail when wifi is on
      // prepare to activate wifi
      buf[0] = STATE_CONNECT_WIFI;  // one more sleep required to to wake with wifi on
      WiFi.forceSleepWake();
      WiFi.mode(WIFI_STA);
      system_rtc_mem_write(RTC_STATE, buf, 1); // set state for next wakeUp
      ESP.deepSleep(10, WAKE_RFCAL);
      yield();

      break;
    case STATE_SLEEP_WAKE:
      system_rtc_mem_read(RTC_WAKE_COUNT, &sleepCount, 4); // read counter
      sleepCount++;
      if (sleepCount > SLEEP_COUNTS_FOR_LIVE_MAIL)
      { // its time to send mail as live signal
        buf[0] = MAIL_LIVE_SIGNAL;
        system_rtc_mem_write(RTC_MAIL_TYPE, buf, 1); // set mail type to send
        // prepare to activate wifi
        buf[0] = STATE_CONNECT_WIFI;  // one more sleep required to to wake with wifi on
        WiFi.forceSleepWake();
        WiFi.mode(WIFI_STA);
        system_rtc_mem_write(RTC_STATE, buf, 1); // set state for next wakeUp
        ESP.deepSleep(10, WAKE_RFCAL);
        yield();
      }
      // check battery
      if (sleepCount > SLEEP_COUNTS_FOR_BATT_CHECK)
      { if ((float)ESP.getVcc()* VCC_ADJ < BATT_WARNING_VOLTAGE)
        { sleepCount = 0;  // start from beginning so batt-warning is not sent every wakeup
          buf[0] = MAIL_LOW_BAT;
          system_rtc_mem_write(RTC_MAIL_TYPE, buf, 1); // set mail type to send
          system_rtc_mem_write(RTC_WAKE_COUNT, &sleepCount, 4); // write counter
          // prepare to activate wifi
          buf[0] = STATE_CONNECT_WIFI;  // one more sleep required to to wake with wifi on
          WiFi.forceSleepWake();
          WiFi.mode(WIFI_STA);
          system_rtc_mem_write(RTC_STATE, buf, 1); // set state for next wakeUp
          ESP.deepSleep(10, WAKE_RFCAL);
          yield();
        }
      }

      // no special event, go to sleep again
      system_rtc_mem_write(RTC_WAKE_COUNT, &sleepCount, 4); // write counter
      buf[0] = STATE_SLEEP_WAKE;
      system_rtc_mem_write(RTC_STATE, buf, 1);            // set state for next wakeUp
      ESP.deepSleep(SLEEP_TIME, WAKE_RF_DISABLED);
      yield();                                            // pass control back to background processes to prepate sleep
      break;

    case STATE_ALARM:
      buf[0] = MAIL_ALARM;
      system_rtc_mem_write(RTC_MAIL_TYPE, buf, 1); // set mail type to send
      // prepare to activate wifi
      buf[0] = STATE_CONNECT_WIFI;  // one more sleep required to to wake with wifi on
      WiFi.forceSleepWake();
      WiFi.mode(WIFI_STA);
      system_rtc_mem_write(RTC_STATE, buf, 1); // set state for next wakeUp
      ESP.deepSleep(10, WAKE_RFCAL);
      yield();
      break;
    case STATE_CONNECT_WIFI:
      WiFi.forceSleepWake();
      delay(500);
      wifi_set_sleep_type(MODEM_SLEEP_T);
      WiFi.mode(WIFI_STA);
      yield();

      time1 = system_get_time();

      // Connect to WiFi network
      Serial.println();
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(ASP_ssid);
      WiFi.begin(ASP_ssid, ASP_password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        time2 = system_get_time();
        if (((time2 - time1) / 1000000) > WIFI_CONNECT_TIMEOUT_S)  // wifi connection lasts too ling, retry
        { ESP.deepSleep(10, WAKE_RFCAL);
          yield();
        }
      }
      Serial.println("");
      Serial.println("WiFi connected");

      system_rtc_mem_read(RTC_MAIL_TYPE, buf, 1);

      if (buf[0] == MAIL_ALARM) {
        Serial.printf("Post to Sparkfun %d\r\n", buf[0]);
        //sendSparkfun(SPARKFUN_BATTERY);
        sendubidots(SPARKFUN_BATTERY);
      }
      else {
        Serial.printf("Send email %d\r\n", buf[0]);
        sendEmail(buf[0]);
      }
      // now re-initialize
      sleepCount = 0;
      system_rtc_mem_write(RTC_WAKE_COUNT, &sleepCount, 4); // initialize counter
      buf[0] = MAIL_NO_MAIL;
      system_rtc_mem_write(RTC_MAIL_TYPE, buf, 1); // no mail pending
      buf[0] = STATE_SLEEP_WAKE;
      system_rtc_mem_write(RTC_STATE, buf, 1); // set state for next wakeUp
      ESP.deepSleep(SLEEP_TIME, WAKE_RF_DISABLED);
      yield();                                            // pass control back to background processes to prepate sleep

      break;

  }
  delay(1000);
  //

}


void loop()
{
  delay(10);


}
