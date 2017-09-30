/*
  ESP 8266-based talking clock
  created 06 Sep 2017
  by Dirk Spiller

  This sketch gets the Time via NTP and speaks it out loud using DFPlayer MP3 module and several voice files as mp3 (e.g. in German language).
  It utilizes an IR distance sensor TCRT5000 
  (eg. https://www.amazon.de/Ecloud-TCRT5000-Reflective-Barrier-Leichtathletik-Lichtschranke/dp/B06WGQJ2ZW/ref=sr_1_4?s=ce-de&ie=UTF8&qid=1505044834&sr=1-4&keywords=TCRT5000+modul)
  to recognize if the user holds his hand close to the sensor and then speaks the time again.

  It uses:
  - Standard ESP8266 WiFi Libs for Arduino IDE: https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
  - TimeLib: https://github.com/PaulStoffregen/Time
  - Timezone Library for correct handling of DST: https://github.com/JChristensen/Timezone
  - The DFPlayer Mini Lib: https://github.com/DFRobot/DFRobotDFPlayerMini
  - The 4x7-segment display TM1637Display lib: https://github.com/avishorp/TM1637

  The mp3 handling is inspired by the mp3 talking clock from LAGSILVA: http://www.instructables.com/id/Talking-Clock-With-Arduino/
  Instead of setting the initial Time manually, it uses NTP as a Sync Provider as in the TimeNTP_ESP8266WiFi example from TimeLib // https://github.com/PaulStoffregen/Time/tree/master/examples/TimeNTP_ESP8266WiFi
  Instead of saying the time every ten or so minutes, it says the time, when you hold your hand close to the IR sensor

   -------- MP3 Voice output via DFPlayer module -------
   On microSD card, all sound files have to be put in a folder called "mp3"
   The following files are expected:
   1.mp3 - 24-mp3  : "zero hours", "one hours", "two hours" to "23 hours"; In German it is required to say "Uhr" meaning "hours" between hours and minutes of a timestamp!
   25.mp3 - 84.mp3 : "zero" to "fifty-nine" for the minutes
   85.mp3          : "to"
   86.mp3          : "past"
   87.mp3          : "quarter"
   88.mp3          : "half"
   85.mp3 - 88.mp3 are meant for colloquial time telling (future expansion, not yet implemented!)

   Decent mp3 files can easily be created online using the following strategy:

   1. Create a text file containing all required words with breaks, like in German:
      0 Uhr, 1 Uhr, 2 Uhr, 3 Uhr, 4 Uhr, 5 Uhr, 6 Uhr, 7 Uhr, 8 Uhr, 9 Uhr, 10 Uhr, 11 Uhr, 12 Uhr, 13 Uhr, 14 Uhr, 15 Uhr, 16 Uhr, 17 Uhr, 18 Uhr, 19 Uhr, 20 Uhr, 21 Uhr, 22 Uhr, 23 Uhr, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, Nach, Vor, Viertel, Halb,
   2. Choose an online Text-To-Speech Service like https://www.naturalreaders.com/online/
   3. Paste the text file contents to the input box
   4. Choose language and voice
   5. Use audacity for recording the output directly as explained under http://manual.audacityteam.org/man/tutorial_recording_computer_playback_on_windows.html
   6. Label recording into separate tracks as described at http://manual.audacityteam.org/man/silence_finder_and_sound_finder.html
   7. Export separate sound files from labels as described here: http://manual.audacityteam.org/man/splitting_a_recording_into_separate_tracks.html
   8. Now click "ok" 88 times ;-) -> the resulting files are cut and named correctly by default.

  Possible Expansions: 
  - Add a TM1637-based 4 Digit module to also display the time (eg https://www.amazon.de/TM1637-Digit-7-Segment-Display-Modul/dp/B0117C1332) >Already contained in source code!
  - Add a DS3231 RTC module to overcome powerloss and network failures (eg. https://www.amazon.de/DaoRier-Precision-Modul-Speichermodul-Raspberry-Mikrocontroller/dp/B06W2PLFJY/ref=sr_1_5?s=ce-de&ie=UTF8&qid=1505029576&sr=1-5&keywords=rtc+module+ds3231)

  This code is in the public domain.
*/

#define TM1637installed
//#define DEBUG       // Remove Comment for Serial interface debug prints
//#define DSTTEST     // Remove comment to test if setting DST to STD works correctly
//#define MP3BUSYTEST // Remove comment to test if PIN_BUSY is working correctly

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>       // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <WiFiUdp.h>           // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <DFPlayer_Mini_Mp3.h> // https://github.com/DFRobot/DFRobotDFPlayerMini
#include <TimeLib.h>           // https://github.com/PaulStoffregen/Time
#include <Timezone.h>          // https://github.com/JChristensen/Timezone
#ifdef TM1637installed
  #include <TM1637Display.h>     // https://github.com/avishorp/TM1637
#endif

#define PIN_BUSY D0 // Connect to DFPlayer Module Pin 8 for busy detection (doesn't work too good)
#define PIN_NEAR D5 // Connect to IR Module Near detection pin.

#ifdef TM1637installed
  #define CLK D3      // Connect to CLK of Display Module
  #define DIO D4      // Connect to DIO of Display Module

  TM1637Display display(CLK, DIO);
#endif

SoftwareSerial mp3Serial(D7, D6); // RX, TX for DFPlayer connection

bool showColon = true;

char ssid[] = "******************";  //  your network SSID (name)
char pass[] = "******************";  // your network password

// NTP Servers:
// IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov
// IPAddress timeServer(129, 6, 15, 28);   // time.nist.gov NTP server

// Don't hardwire the IP address or we won't get the benefits of the pool.
// Lookup the IP address for the host name instead */
IPAddress timeServer;                    // time.nist.gov NTP server address will be retrieved using WiFi.byHost() command
const char* ntpServerName = "time.nist.gov";

//Timezone
//Central European Time (Frankfurt, Paris)
TimeChangeRule myDST = { "CEST", Last, Sun, Mar, 2, 120 };     // Central European Summer Time
TimeChangeRule mySTD = { "CET ", Last, Sun, Oct, 3, 60 };      // Central European Standard Time
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;                                           // pointer to the time change rule, use to get the TZ abbrev
time_t local;                                                  // local time for display / voice output
time_t prevDisplay = 0;                                        // when the digital clock was displayed

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

void setup()
{

#ifdef TM1637installed
  display.setBrightness (0x0a);     //(0x0f) is the max brightness;
  display.showNumberDecEx(8888, showColon?0x40:0x00, false, 4, 0); // show that power is on
#endif
  
#ifdef DEBUG
  Serial.begin(115200);
  delay(250);
  Serial.println("ESP Talking CLock");
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }

#ifdef DEBUG
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
#endif
  Udp.begin(localPort);
#ifdef DEBUG
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("Waiting for sync");
#endif
  setSyncProvider(getNtpTime);
  while (timeStatus() == timeNotSet) {
     // wait until the time is set by the sync provider
#ifdef TM1637installed
      display.showNumberDecEx(8888, showColon?0x40:0x00, false, 4, 0);
      showColon = !showColon;
      delay(1000);
#endif
  }
  local = myTZ.toLocal(now(), &tcr);
#ifdef TM1637installed
  display.showNumberDecEx(hour(local) * 100 + minute(local), showColon?0x40:0x00, false, 4, 0);
#endif
  pinMode(PIN_BUSY, INPUT_PULLUP);
  pinMode(PIN_NEAR, INPUT_PULLUP);
  mp3Serial.begin (9600);
#ifdef DEBUG
  Serial.println("Setting up mp3 player");
#endif
  mp3_set_serial (mp3Serial);
  // Delay is required before accessing player. From my experience it's ~1 sec
  delay(1000);
  mp3_set_volume (22);
#ifdef DEBUG
  digitalClockDisplay();
#endif
  delay(1000);
  sayTime();
}

void loop()
{
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
#ifdef TM1637installed
      showColon = !showColon; // blink colon on display
      display.showNumberDecEx(hour(local) * 100 + minute(local), showColon?0x40:0x00, false, 4, 0);
#endif
      prevDisplay = now();
      local = myTZ.toLocal(now(), &tcr);
#ifdef DEBUG
      digitalClockDisplay();
#endif
    }
    if (digitalRead(PIN_NEAR) != 1) {
#ifdef DEBUG
      Serial.print("**** NEAR= ");
      Serial.println(digitalRead(PIN_NEAR));
#endif
      sayTime();
    }
  }
}

/* ------- Time display on Serial port for debugging ------------*/
void digitalClockDisplay() {
  // digital clock display of the time

  Serial.print(hour(local));
  printDigits(minute(local));
  printDigits(second(local));
  Serial.print(" ");
  Serial.print(day(local));
  Serial.print(".");
  Serial.print(month(local));
  Serial.print(".");
  Serial.print(year(local));
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void sayTime() {
  unsigned long mp3start = millis();
#ifdef DEBUG
  Serial.println("*********** mp3_play:");
  digitalClockDisplay();
  Serial.print("hour file:");
  Serial.print((hour(local) + 1) % 24);
  Serial.println(".mp3");
  Serial.print("minute file:");
  Serial.print(minute(local) + 25);
  Serial.println(".mp3");
#endif
  mp3_play((hour(local) + 1) % 24); // "1.mp3" is for 0h, "2.mp3" for 1h, etc.
  /*
      Uncomment "#define MP3BUSYTEST" at the top of this code to test, if mp3 busy detection works correclty in your setup.
      If this is functioning correctly, you will see PIN_BUSY changing to 1, when mp3 output has finished.
  */
#ifdef MP3BUSYTEST
  Serial.println("PIN_BUSY=");
  while (millis() <  mp3start + 2000) {
    Serial.println(digitalRead(PIN_BUSY));
    delay(50);
  }
#endif
  // If MP3BUSYTEST above is successful, replace the upper delay by the following for a more natural speech output due to shorter breaks between hours and minutes:
  delay(100); // Minimal delay to allow PIN_BUSY to become 0.
  while (digitalRead(PIN_BUSY) == 0) {         // Wait for end of play
  }
  //delay(1000);
  mp3_play(minute(local) + 25); // "25.mp3" is "0", "26.mp" is "1" etc.
  delay(2000);
  mp3_stop();
}


/* -------- NTP code ----------
   based on https://github.com/PaulStoffregen/Time/tree/master/examples/TimeNTP_ESP8266WiFi
   added debug code to test if DST change works correctly
*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
#ifdef DEBUG
  Serial.println("Transmit NTP Request");
#endif
  WiFi.hostByName(ntpServerName, timeServer);
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
#ifdef DEBUG
      Serial.println("Receive NTP Response");
#endif
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
#ifdef DSTTEST
      return 1509238740UL; // Unix Timestamp for 29 October 2017, 2:59 CEST, short before setting back from DST to STD (CEST to CET)
#else
      return secsSince1900 - 2208988800UL;
#endif
    }
  }
#ifdef DEBUG
  Serial.println("No NTP Response :-(");
#endif
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

