/*
remix from HarryFun's great Morphing Digital Clock idea https://github.com/hwiguna/HariFun_166_Morphing_Clock
follow the great tutorial there and eventually use this code as alternative

provided 'AS IS', use at your own risk
 * mirel.t.lazar@gmail.com

Further Modified by:
 * Brian Leschke
 * 9 September 2019
 * 
 * 
Additions:
 - Weather Alerts (not currently working)
 - Fire/EMS Alerts
 - WMATA Metro Arrival Times
 - Network Device Status (UP/DN)
 - Additional date-based events
 */

 

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>

#define double_buffer
#include <PxMatrix.h>

#define USE_ICONS
#define USE_FIREWORKS
#define USE_WEATHER_ANI
#define SHOW_SOME_PRIDE
#define SHOW_SOME_LOVE
//#define SHOW_WX_ALERT
#define SHOW_FIREEMS_ALERT
#define SHOW_WMATA
#define SHOW_NETWORK_STATUS


//#include <Adafruit_GFX.h>    // Core graphics library
//#include <Fonts/FreeMono9pt7b.h>

//=== FIRE-EMS INFORMATION ===
char SERVER_NAME[]    = "x.x.x.x"; // Address of the webserver without "http". Can also be an IP address.
int SERVER_PORT       = PORT-NUM-HERE;       // webserver port

char Str[11];
int prevNum           = 0; //Number of previous emails before check
int num               = 0; //Number of emails after check

//=== WIFI MANAGER ===
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
char wifiManagerAPName[] = "MorphClk";
char wifiManagerAPPassword[] = "MorphClk";

//=== OPEN-WEATHER-MAP ===
//open weather map api key https://openweathermap.org/
String apiKey   = "KEY-HERE"; //e.g a hex string like "abcdef0123456789abcdef0123456789"
//the city you want the weather for 
String location = "LOCATION-HERE"; //e.g. "5391811" = San Diego, CA

//=== NETWORK CHECK ===
//change device names on line 1484
const char* device1 = "www.google.com";   //IP address or web address without Http://
const char* device2 = "x.x.x.x";
const char* device3 = "x.x.x.x";
const char* device4 = "x.x.x.x";
const char* device5 = "x.x.x.x";
const char* device6 = "x.x.x.x";
const char* device7 = "x.x.x.x";

int pingResult1;                      //Do not change.
int pingResult2;                      //Do not change.
int pingResult3;                      //Do not change.
int pingResult4;                      //Do not change.
int pingResult5;                      //Do not change.
int pingResult6;                      //Do not change.
int pingResult7;                      //Do not change.

//=== WMATA INFORMATION ===
#define      WMATAServer       "api.wmata.com"      // name address for WMATA (using DNS)
const String myKey           = "KEY-HERE";           // See: https://developer.wmata.com/ (change here with your Primary/Secondary API KEY)
const String stationCode     = "STATION-CODE-HERE";      // Metro station code . Ex Greenbelt = E10

const int wmata_buffer_size = 300;                  // Do not change. Length of json buffer
const int buffer=300;                               // Do not change.

char* metroConds[]={                                // Do not change.
   "\"Car\":",
   "\"Destination\":",
   "\"DestinationCode\":",
   "\"DestinationName\":",
   "\"Group\":",
   "\"Line\":",
   "\"LocationCode\":",
   "\"LocationName\":",
   "\"Min\":",
};

int num_elements        = 9;  // number of conditions you are retrieving, count of elements in conds
unsigned long WMillis   = 0;  // temporary millis() register

//=== NWS WEATHER.GOV API INFOMRATION===

#define NWSServer            "api.weather.gov"  //name address for weather.gov (using dns)
const String nwsKey        = "KEY-HERE";
const String countyCode    = "COUNTY-CODE-HERE"; //Your county Code (ex. NCC055) https://alerts.weather.gov/

const int nws_buffer_size = 300;                  // Do not change. Length of json buffer
const int nws_Buffer=300;                         // Do not change.

char* nwsConds[]={                                // Do not change.
   "\"status\": ",
   "\"messageType\": ",
   "\"urgency\": ",
   "\"event\": ",
   "\"headline\": ",
};
int num_wx_elements      = 5;  // number of conditions you are retrieving, count of elements in conds
unsigned long WXMillis   = 0;  // temporary millis() register


//== DOUBLE-RESET DETECTOR ==
#include <DoubleResetDetector.h>
#define DRD_TIMEOUT 10 // Second-reset must happen within 10 seconds of first reset to be considered a double-reset
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

//== SAVING CONFIG ==
#include "FS.h"
#include <ArduinoJson.h>
bool shouldSaveConfig = false; // flag for saving data

//callback notifying us of the need to save config
void saveConfigCallback () 
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

#ifdef ESP8266
#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2
#endif

// Pins for LED MATRIX
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

//=== SEGMENTS ===
int cin = 25; //color intensity
#include "Digit.h"
Digit digit0(&display, 0, 63 - 1 - 9*1, 8, display.color565(0, 0, 255));
Digit digit1(&display, 0, 63 - 1 - 9*2, 8, display.color565(0, 0, 255));
Digit digit2(&display, 0, 63 - 4 - 9*3, 8, display.color565(0, 0, 255));
Digit digit3(&display, 0, 63 - 4 - 9*4, 8, display.color565(0, 0, 255));
Digit digit4(&display, 0, 63 - 7 - 9*5, 8, display.color565(0, 0, 255));
Digit digit5(&display, 0, 63 - 7 - 9*6, 8, display.color565(0, 0, 255));

#ifdef ESP8266
// ISR for display refresh
void display_updater ()
{
  //display.displayTestPattern(70);
  display.display (70);
}
#endif

void getWeather ();

void configModeCallback (WiFiManager *myWiFiManager) 
{
  Serial.println ("Entered config mode");
  Serial.println (WiFi.softAPIP());

  // You could indicate on your screen or by an LED you are in config mode here

  // We don't want the next time the boar resets to be considered a double reset
  // so we remove the flag
  drd.stop ();
}

char timezone[5] = "-4";
char military[3] = "Y";     // 24 hour mode? Y/N
char u_metric[3] = "N";     // use metric for units? Y/N
char date_fmt[7] = "M.D.Y"; // date format: D.M.Y or M.D.Y or M.D or D.M or D/M/Y.. looking for trouble
bool loadConfig () 
{
  File configFile = SPIFFS.open ("/config.json", "r");
  if (!configFile) 
  {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size ();
  if (size > 1024) 
  {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes (buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success ()) 
  {
    Serial.println("Failed to parse config file");
    return false;
  }

  strcpy (timezone, json["timezone"]);
  strcpy (military, json["military"]);
  //avoid reboot loop on systems where this is not set
  if (json.get<const char*>("metric"))
    strcpy (u_metric, json["metric"]);
  else
  {
    Serial.println ("metric units not set, using default: Y");
  }
  if (json.get<const char*>("date-format"))
    strcpy (date_fmt, json["date-format"]);
  else
  {
    Serial.println ("date format not set, using default: D.M.Y");
  }
  
  return true;
}

bool saveConfig () 
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["timezone"] = timezone;
  json["military"] = military;
  json["metric"] = u_metric;
  json["date-format"] = date_fmt;

  File configFile = SPIFFS.open ("/config.json", "w");
  if (!configFile)
  {
    Serial.println ("Failed to open config file for writing");
    return false;
  }

  Serial.println ("Saving configuration to file:");
  Serial.print ("timezone=");
  Serial.println (timezone);
  Serial.print ("military=");
  Serial.println (military);
  Serial.print ("metric=");
  Serial.println (u_metric);
  Serial.print ("date-format=");
  Serial.println (date_fmt);

  json.printTo (configFile);
  return true;
}

#include "TinyFont.h"
const byte row0 = 2+0*10;
const byte row1 = 2+1*10;
const byte row2 = 2+2*10;
void wifi_setup ()
{
  //-- Config --
  if (!SPIFFS.begin ()) 
  {
    Serial.println ("Failed to mount FS");
    return;
  }
  loadConfig ();

  //-- Display --
  display.fillScreen (display.color565 (0, 0, 0));
  display.setTextColor (display.color565 (0, 0, 255));

  //-- WiFiManager --
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback (saveConfigCallback);
  WiFiManagerParameter timeZoneParameter ("timeZone", "Time Zone", timezone, 5); 
  wifiManager.addParameter (&timeZoneParameter);
  WiFiManagerParameter militaryParameter ("military", "24Hr (Y/N)", military, 3); 
  wifiManager.addParameter (&militaryParameter);
  WiFiManagerParameter metricParameter ("metric", "Metric Units (Y/N)", u_metric, 3); 
  wifiManager.addParameter (&metricParameter);
  WiFiManagerParameter dmydateParameter ("date_fmt", "Date Format (D.M.Y)", date_fmt, 6); 
  wifiManager.addParameter (&dmydateParameter);

  //-- Double-Reset --
  if (drd.detectDoubleReset ()) 
  {
    Serial.println ("Double Reset Detected");
    TFDrawText (&display, String("     ONLINE     "), 0, 13, display.color565(0, 0, 255));

    display.setCursor (0, row0);
    display.print ("AP:");
    display.print (wifiManagerAPName);

    display.setCursor (0, row1);
    display.print ("Pw:");
    display.print (wifiManagerAPPassword);

    display.setCursor (0, row2);
    display.print ("192.168.4.1");

    wifiManager.startConfigPortal (wifiManagerAPName, wifiManagerAPPassword);

    display.fillScreen (display.color565(0, 0, 0));
  } 
  else 
  {
    Serial.println ("No Double Reset Detected");

    //display.setCursor (2, row1);
    //display.print ("connecting");
    TFDrawText (&display, String("   CONNECTING   "), 0, 13, display.color565(0, 0, 255));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name wifiManagerAPName
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect (wifiManagerAPName);
  }
  
  Serial.print ("timezone=");
  Serial.println (timezone);
  Serial.print ("military=");
  Serial.println (military);
  Serial.print ("metric=");
  Serial.println (u_metric);
  Serial.print ("date-format=");
  Serial.println (date_fmt);
  //timezone
  strcpy (timezone, timeZoneParameter.getValue ());
  //military time
  strcpy (military, militaryParameter.getValue ());
  //metric units
  strcpy (u_metric, metricParameter.getValue ());
  //date format
  strcpy (date_fmt, dmydateParameter.getValue ());
  //display.fillScreen (0);
  //display.setCursor (2, row1);
  TFDrawText (&display, String("     ONLINE     "), 0, 13, display.color565(0, 0, 255));
  Serial.print ("WiFi connected, IP address: ");
  Serial.println (WiFi.localIP ());\
  display.setCursor (0, row2);
  display.print ((WiFi.localIP ()));
  /*delay(5);
  display.fillScreen (0);
  TFDrawText (&display, String("     Welcome      "), 0, 0, display.color565(0, 150, 255));
  TFDrawText (&display, String("        to        "), 0, 8, display.color565(0, 130, 255));
  TFDrawText (&display, String("    MorphClock    "), 0, 16, display.color565(0, 110, 255));
  delay(5);
  display.fillScreen (0);
  */
  //
  //start NTP
  NTP.begin ("pool.ntp.org", String(timezone).toInt(), false);
  NTP.setInterval (10);//force rapid sync in 10sec

  if (shouldSaveConfig) 
  {
    saveConfig ();
  }
  drd.stop ();
  
  //delay (1500);
  getWeather ();
}

byte hh;
byte mm;
byte ss;
byte ntpsync = 1;
//
void setup()
{	
	Serial.begin (115200);
  //display setup
  display.begin (16);
#ifdef ESP8266
  display_ticker.attach (0.002, display_updater);
#endif
  //
  wifi_setup ();
  //
	NTP.onNTPSyncEvent ([](NTPSyncEvent_t ntpEvent) 
	{
		if (ntpEvent) 
		{
			Serial.print ("Time Sync error: ");
			if (ntpEvent == noResponse)
				Serial.println ("NTP server not reachable");
			else if (ntpEvent == invalidAddress)
				Serial.println ("Invalid NTP server address");
		}
		else 
		{
			Serial.print ("Got NTP time: ");
			Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
      ntpsync = 1;
		}
	});
  //prep screen for clock display
  display.fillScreen (0);
  int cc_gry = display.color565 (128, 128, 128);
  //reset digits color
  digit0.SetColor (cc_gry);
  digit1.SetColor (cc_gry);
  digit2.SetColor (cc_gry);
  digit3.SetColor (cc_gry);
  digit4.SetColor (cc_gry);
  digit5.SetColor (cc_gry);
  digit1.DrawColon (cc_gry);
  digit3.DrawColon (cc_gry);
  //
  Serial.print ("display color range [");
  Serial.print (display.color565 (0, 0, 0));
  Serial.print (" .. ");
  Serial.print (display.color565 (255, 255, 255));
  Serial.println ("]");
  //
}

char server[]   = "api.openweathermap.org";
WiFiClient client;
int tempMin = -10000;
int tempMax = -10000;
int tempM = -10000;
int presM = -10000;
int humiM = -10000;
int condM = -1;  //-1 - undefined, 0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
String condS = "";
void getWeather ()
{
  if (!apiKey.length ())
  {
    Serial.println ("w:missing API KEY for weather data, skipping"); 
    return;
  }
  Serial.print ("i:connecting to weather server.. "); 
  // if you get a connection, report back via serial: 
  if (client.connect (server, 80))
  { 
    Serial.println ("connected."); 
    // Make a HTTP request: 
    client.print ("GET /data/2.5/weather?"); 
    client.print ("id="+location); 
    client.print ("&appid="+apiKey); 
    client.print ("&cnt=1"); 
    (*u_metric=='Y')?client.println ("&units=metric"):client.println ("&units=imperial");
    client.println ("Host: api.openweathermap.org"); 
    client.println ("Connection: close");
    client.println (); 
  } 
  else 
  { 
    Serial.println ("w:unable to connect");
    return;
  } 
  delay (1000);
  String sval = "";
  int bT, bT2;
  //do your best
  String line = client.readStringUntil ('\n');
  if (!line.length ())
    Serial.println ("w:unable to retrieve weather data");
  else
  {
    Serial.print ("weather:"); 
    Serial.println (line); 
    //weather conditions - "main":"Clear",
    bT = line.indexOf ("\"main\":\"");
    if (bT > 0)
    {
      bT2 = line.indexOf ("\",\"", bT + 8);
      sval = line.substring (bT + 8, bT2);
      Serial.print ("cond ");
      Serial.println (sval);
      //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
      condM = 0;
      if (sval.equals("Clear"))
        condM = 1;
      else if (sval.equals("Clouds"))
        condM = 2;
      else if (sval.equals("Overcast"))
        condM = 3;
      else if (sval.equals("Rain"))
        condM = 4;
      else if (sval.equals("Drizzle"))
        condM = 4;
      else if (sval.equals("Thunderstorm"))
        condM = 5;
      else if (sval.equals("Snow"))
        condM = 6;
      //
      condS = sval;
      Serial.print ("condM ");
      Serial.println (condM);
    }
    //tempM
    bT = line.indexOf ("\"temp\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 7);
      sval = line.substring (bT + 7, bT2);
      Serial.print ("temp: ");
      Serial.println (sval);
      tempM = sval.toInt ();
    }
    else
      Serial.println ("temp NOT found!");
    //tempMin
    bT = line.indexOf ("\"temp_min\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("temp min: ");
      Serial.println (sval);
      tempMin = sval.toInt ();
    }
    else
      Serial.println ("temp_min NOT found!");
    //tempMax
    bT = line.indexOf ("\"temp_max\":");
    if (bT > 0)
    {
      bT2 = line.indexOf ("},", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("temp max: ");
      Serial.println (sval);
      tempMax = sval.toInt ();
    }
    else
      Serial.println ("temp_max NOT found!");
    //pressM
    bT = line.indexOf ("\"pressure\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("press ");
      Serial.println (sval);
      presM = sval.toInt();
    }
    else
      Serial.println ("pressure NOT found!");
    //humiM
    bT = line.indexOf ("\"humidity\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      Serial.print ("humi ");
      Serial.println (sval);
      humiM = sval.toInt();
    }
    else
      Serial.println ("humidity NOT found!");
  }//connected
}

#ifdef USE_ICONS
#include "TinyIcons.h"
//icons 10x5: 10 cols, 5 rows
int moony_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x4a49, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18c3,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int moony1_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x18c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4a49,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};

int moony2_ico [50] = {
  //3 nuances: 0x18c3 < 0x3186 < 0x4a49
  0x0000, 0x3186, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3186,
  0x0000, 0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x4a49, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
  0x0000, 0x0000, 0x4a49, 0x3186, 0x3186, 0x3186, 0x3186, 0x18c3, 0x0000, 0x0000,
};
#endif

int sunny_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffe0,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int sunny1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffff,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};

int sunny2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000, 0x0000,
  0xffff, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0xffe0,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0x0000, 0x0000,
};
#endif

int cloudy_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};

#ifdef USE_WEATHER_ANI
int cloudy1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffe0, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffff, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffff, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};

int cloudy2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000,
  0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0x0000, 0x0000, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0xffe0, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618,
  0x0000, 0x0000, 0xffe0, 0xffe0, 0xffe0, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618,
};
#endif

int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};

#ifdef USE_WEATHER_ANI
int ovrcst1_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};

int ovrcst2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};
#endif

int thndr_ico [50] = {
  0x041f, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0x041f,
  0xc618, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618,
  0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618,
  0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x041f,
  0xc618, 0x041f, 0xc618, 0xc618, 0xc618, 0xc618, 0x041f, 0xc618, 0x041f, 0xc618,
};

int rain_ico [50] = {
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
};

#ifdef USE_WEATHER_ANI
int rain1_ico [50] = {
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
};

int rain2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
};

int rain3_ico [50] = {
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
};

int rain4_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f,
  0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x041f, 0x0000,
  0x041f, 0x0000, 0x041f, 0x0000, 0x0000, 0x0000, 0x041f, 0x0000, 0x0000, 0x041f,
};
#endif

int snow_ico [50] = {
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
};

#ifdef USE_WEATHER_ANI
int snow1_ico [50] = {
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
};

int snow2_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
};

int snow3_ico [50] = {
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
};

int snow4_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618,
  0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0xc618, 0x0000,
  0xc618, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0xc618,
};
#endif

#ifdef USE_WEATHER_ANI
int *suny_ani[] = {sunny_ico, sunny1_ico, sunny2_ico, sunny1_ico, sunny2_ico};
int *clod_ani[] = {cloudy_ico, cloudy1_ico, cloudy2_ico, cloudy1_ico, cloudy2_ico};
int *ovct_ani[] = {ovrcst_ico, ovrcst1_ico, ovrcst2_ico, ovrcst1_ico, ovrcst2_ico};
int *rain_ani[] = {rain_ico, rain1_ico, rain2_ico, rain3_ico, rain4_ico};
int *thun_ani[] = {thndr_ico, rain1_ico, rain2_ico, rain3_ico, rain4_ico};
int *snow_ani[] = {snow_ico, snow1_ico, snow2_ico, snow3_ico, snow4_ico};
int *mony_ani[] = {moony_ico, moony1_ico, moony2_ico, moony1_ico, moony2_ico};
#else
int *suny_ani[] = {sunny_ico, sunny_ico, sunny_ico, sunny_ico, sunny_ico};
int *clod_ani[] = {cloudy_ico, cloudy_ico, cloudy_ico, cloudy_ico, cloudy_ico};
int *ovct_ani[] = {ovrcst_ico, ovrcst_ico, ovrcst_ico, ovrcst_ico, ovrcst_ico};
int *rain_ani[] = {rain_ico, rain_ico, rain_ico, rain_ico, rain_ico};
int *thun_ani[] = {thndr_ico, rain_ico, rain_ico, rain_ico, rain_ico};
int *snow_ani[] = {snow_ico, snow_ico, snow_ico, snow_ico, snow_ico};
int *mony_ani[] = {moony_ico, moony_ico, moony_ico, moony_ico, moony_ico};
#endif
/*
 * 
int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0xffff, 0xc618, 0x0000, 0x0000, 0x0000,
  0x0000, 0xc618, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
};

int ovrcst_ico [50] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xc618, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0xc618, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xc618, 0x0000,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};

int cloudy_ico [50] = {
  0x0000, 0x0000, 0x0000, 0xffe0, 0x0000, 0x0000, 0xffe0, 0xc618, 0x0000, 0x0000,
  0x0000, 0xffe0, 0xc618, 0xc618, 0x0000, 0xc618, 0xc618, 0xc618, 0xffe0, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xffe0, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000,
  0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xffe0,
  0x0000, 0x0000, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x0000, 0x0000,
};
 */
#endif

int xo = 1, yo = 26;
char use_ani = 0;
char daytime = 1;
void draw_weather_conditions ()
{
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  Serial.print ("weather conditions ");
  Serial.println (condM);
  //cleanup previous cond
  xo = 3*TF_COLS; yo = 1;
#ifdef USE_ICONS
if (condM == 0 && daytime)
  {
    Serial.print ("!weather condition icon unknown, show: ");
    Serial.println (condS);
    int cc_dgr = display.color565 (30, 30, 30);
    //draw the first 5 letters from the unknown weather condition
    String lstr = condS.substring (0, (condS.length () > 5?5:condS.length ()));
    lstr.toUpperCase ();
    TFDrawText (&display, lstr, xo, yo, cc_dgr);
  }
  else
  {
    TFDrawText (&display, String("     "), xo, yo, 0);
  }
  //
  xo = 4*TF_COLS; yo = 1;
  switch (condM)
  {
    case 0://unk
      break;
    case 1://sunny
      if (!daytime)
        DrawIcon (&display, moony_ico, xo, yo, 10, 5);
      else
        DrawIcon (&display, sunny_ico, xo, yo, 10, 5);
      //DrawIcon (&display, cloudy_ico, xo, yo, 10, 5);
      //DrawIcon (&display, ovrcst_ico, xo, yo, 10, 5);
      //DrawIcon (&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 2://cloudy
      DrawIcon (&display, cloudy_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 3://overcast
      DrawIcon (&display, ovrcst_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 4://rainy
      DrawIcon (&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 5://thunders
      DrawIcon (&display, thndr_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 6://snow
      DrawIcon (&display, snow_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
  }
#else
  xo = 3*TF_COLS; yo = 1;
  Serial.print ("!weather condition icon unknown, show: ");
  Serial.println (condS);
  int cc_dgr = display.color565 (30, 30, 30);
  //draw the first 5 letters from the unknown weather condition
  String lstr = condS.substring (0, (condS.length () > 5?5:condS.length ()));
  lstr.toUpperCase ();
  TFDrawText (&display, lstr, xo, yo, cc_dgr);
#endif
}

void draw_weather ()
{
  int cc_wht = display.color565 (cin, cin, cin);
  int cc_red = display.color565 (cin, 0, 0);
  int cc_grn = display.color565 (0, cin, 0);
  int cc_blu = display.color565 (0, 0, cin);
  //int cc_ylw = display.color565 (cin, cin, 0);
  //int cc_gry = display.color565 (128, 128, 128);
  int cc_dgr = display.color565 (30, 30, 30);
  Serial.println ("showing the weather");
  xo = 0; yo = 1;
  TFDrawText (&display, String("                "), xo, yo, cc_dgr);
  if (tempM == -10000 || humiM == -10000 || presM == -10000)
  {
    //TFDrawText (&display, String("NO WEATHER DATA"), xo, yo, cc_dgr);
    Serial.println ("!no weather data available");
  }
  else
  {
    //weather below the clock
    //-temperature
    int lcc = cc_red;
    if (*u_metric == 'Y')
    {
      //C
      if (tempM < 26)
        lcc = cc_grn;
      if (tempM < 18)
        lcc = cc_blu;
      if (tempM < 6)
        lcc = cc_wht;
    }
    else
    {
      //F
      if (tempM < 79)
        lcc = cc_grn;
      if (tempM < 64)
        lcc = cc_blu;
      if (tempM < 43)
        lcc = cc_wht;
    }
    //
    String lstr = String (tempM) + String((*u_metric=='Y')?"C":"F");
    Serial.print ("temperature: ");
    Serial.println (lstr);
    TFDrawText (&display, lstr, xo, yo, lcc);
    //weather conditions
    //-humidity
    lcc = cc_red;
    if (humiM < 65)
      lcc = cc_grn;
    if (humiM < 35)
      lcc = cc_blu;
    if (humiM < 15)
      lcc = cc_wht;
    lstr = String (humiM) + "%";
    xo = 8*TF_COLS;
    TFDrawText (&display, lstr, xo, yo, lcc);
    //-pressure
    lstr = String (presM);
    xo = 12*TF_COLS;
    TFDrawText (&display, lstr, xo, yo, cc_blu);
    //draw temp min/max
    if (tempMin > -10000)
    {
      xo = 0*TF_COLS; yo = 26;
      TFDrawText (&display, "   ", xo, yo, 0);
      lstr = String (tempMin);// + String((*u_metric=='Y')?"C":"F");
      //blue if negative
      int ct = cc_dgr;
      if (tempMin < 0)
      {
        ct = cc_blu;
        lstr = String (-tempMin);// + String((*u_metric=='Y')?"C":"F");
      }
      Serial.print ("temp min: ");
      Serial.println (lstr);
      TFDrawText (&display, lstr, xo, yo, ct);
    }
    if (tempMax > -10000)
    {
      TFDrawText (&display, "   ", 13*TF_COLS, yo, 0);
      //move the text to the right or left as needed
      xo = 14*TF_COLS; yo = 26;
      if (tempMax < 10)
        xo = 15*TF_COLS;
      if (tempMax > 99)
        xo = 13*TF_COLS;
      lstr = String (tempMax);// + String((*u_metric=='Y')?"C":"F");
      //blue if negative
      int ct = cc_dgr;
      if (tempMax < 0)
      {
        ct = cc_blu;
        lstr = String (-tempMax);// + String((*u_metric=='Y')?"C":"F");
      }
      Serial.print ("temp max: ");
      Serial.println (lstr);
      TFDrawText (&display, lstr, xo, yo, ct);
    }
    //weather conditions
    draw_weather_conditions ();
  }
}

void draw_love ()
{
  Serial.println ("Showing some love");
  use_ani = 0;
  //love*you,boo. live laugh love
  int cc = random (255, 65535);

  yo = 1;
  xo  = 0; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'h', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, 'i', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'Y', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'U', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ',', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'B', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);

  yo=26;
  xo  = 0; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'I', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'A', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'U', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'G', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'H', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
}
//

void draw_pride ()
{
  Serial.println ("Showing some pride month pride!");
  use_ani = 0;
  //happy pride! love * wins
  int cc = random (255, 65535);

  yo = 1;
  xo  = 0; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'H', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'A', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'P', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'P', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'Y', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'P', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'R', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'I', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'D', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, '!', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);

  yo=26;
  xo  = 0; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'h', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, 'i', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'W', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'I', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'N', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'S', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
}
//  

void draw_OutDay ()
{
  Serial.println ("Showing some pride for National Coming Out Day!");
  use_ani = 0;
  //natl coming out day, love * you!
  int cc = random (255, 65535);

  yo = 1;
  xo  = 0; TFDrawChar (&display, 'N', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'A', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'T', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'C', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'M', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'I', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'N', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'G', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'U', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'T', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);

  yo=26;
  xo  = 0; TFDrawChar (&display, 'D', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'A', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'Y', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ',', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'L', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'V', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'E', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'h', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, 'i', xo, yo, display.color565 (255, 0, 0));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'Y', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'O', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, 'U', xo, yo, cc); cc = random (255, 65535);
  xo += 4; TFDrawChar (&display, '!', xo, yo, cc); cc = random (255, 65535);
}
//  

void check_NWS ()
{
  Serial.print("NWS: Connecting to ");
  Serial.println(NWSServer);

  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  const int nws_httpPort = 443;
  
  if (!client.connect(NWSServer, nws_httpPort)) {
    Serial.println("NWS: connection failed");
    return;
  }
  
  String cmd = "GET /alerts/active/zone/";  cmd += countyCode;      // build request_string cmd
  //cmd += "?api_key=";  cmd += nwsKey;  //
  //cmd += " HTTP/1.1\r\nHost: api.weather.gov\r\n\r\n";            
  delay(500);
  client.print(cmd);                                            
  delay(500);
  unsigned int i = 0;                                           // timeout counter
  char json[nws_buffer_size]="{";                                   // first character for json-string is begin-bracket 
  int n = 1;                                                    // character counter for json
  
  
  for (int j=0;j<num_wx_elements;j++){                             // do the loop for every element/condition
    boolean quote = false; int nn = false;                      // if quote=fals means no quotes so comma means break
    while (!client.find(nwsConds[j]))                         // If nws condition data is not available, try again.
    {
      Serial.println("No weather alert data available");
      use_ani = 0;
      yo = 1;
      xo = 0;  TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'O', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'W', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'X', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      return;
    }                            
  
    String Str1= nwsConds[j];                                     // Str1 gets the name of element/condition
  
    for (int l=0; l<(Str1.length());l++)                        // for as many character one by one
        {json[n] = Str1[l];                                     // fill the json string with the name
         n++;}                                                  // character count +1
    while (i<5000) {                                            // timer/counter
      if(client.available()) {                                  // if character found in receive-buffer
        char c = client.read();                                 // read that character
           Serial.print(c);                                     // 
           
// ************************ construction of json string converting comma's inside quotes to dots ******************** 
               if ((c=='"') && (quote==false))                  // there is a " and quote=false, so start of new element
                  {quote = true;nn=n;}                          // make quote=true and notice place in string
               if ((c==',')&&(quote==true)) {c='.';}            // if there is a comma inside quotes, comma becomes a dot.
               if ((c=='"') && (quote=true)&&(nn!=n))           // if there is a " and quote=true and on different position
                  {quote = false;}                              // quote=false meaning end of element between ""
               if((c==',')&&(quote==false)) break;              // if comma delimiter outside "" then end of this element
 // ****************************** end of construction ******************************************************
          json[n]=c;                                            // fill json string with this character
          n++;                                                  // character count + 1
          i=0;                                                  // timer/counter + 1
        }
        i++;                                                    // add 1 to timer/counter
      }                    // end while i<5000
     if (j==num_elements-1)                                     // if last element
        {json[n]='}';}                                          // add end bracket of json string
     else                                                       // else
        {json[n]=',';}                                          // add comma as element delimiter
     n++;                                                       // next place in json string
  }
  Serial.println(json);                                         // debugging json string 
  parseJSON_nws(json);                                              // extract the conditions
  WXMillis=millis();
}

void parseJSON_nws(char json[300])
{
  display.fillScreen (0);
  StaticJsonBuffer<buffer> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
 
 if (!root.success())
{
  Serial.println("?fparseObject() failed");
  //return;
}

 
 String wxstatus              = root["status"];
 String MessageType           = root["messageType"];
 String Urgency               = root["urgency"];
 String Event                 = root["event"];
 String Headline              = root["headline"];
 
 wxstatus.toUpperCase();
 MessageType.toUpperCase();
 Urgency.toUpperCase();
 Event.toUpperCase();
 Headline.toUpperCase();
 
 Serial.println(wxstatus);
 Serial.println(MessageType);
  Serial.println("..");
 Serial.println(Urgency);
 Serial.println(Event);
 Serial.println(Headline);
 
 if (wxstatus == "ACTUAL")
 {
  if (Event == "TORNADO WARNING")
  {
    Serial.println ("Tornado Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("TORNADO  WARNING"), xo, yo, display.color565(255, 0, 0));
    yo=26;
    xo  = 0; TFDrawText (&display, String("SEEK SHELTER NOW"), xo, yo, display.color565(255, 0, 0)); 
  }
  else if (Event == "TORNADO WATCH")
  {
    Serial.println ("Tornado Watch");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String(" TORNADO  WATCH "), xo, yo, display.color565(255, 165, 0));    
  }
  else if (Event == "HURRICANE WARNING")
  {
    Serial.println ("Hurricane Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String(" HURRICANE  WRN "), xo, yo, display.color565(255, 165, 0));  
  }
  else if (Event == "HURRICANE WATCH")
  {
    Serial.println ("Hurricane Watch");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String(" HURRICANE  WAT "), xo, yo, display.color565(255, 165, 0)); 
  }
  else if (Event == "SEVERE THUNDERSTORM WARNING")
  {
    Serial.println ("Severe Thunderstorm Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("SEV THUNDER WRN "), xo, yo, display.color565(255, 165, 0));
  }
  else if (Event == "SEVERE THUNDERSTORM WATCH")
  {
    Serial.println ("Severe Thunderstorm Watch");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("SEV THUNDER WAT "), xo, yo, display.color565(255, 165, 0));
  }
  else if (Event == "BLIZZARD WARNING")
  {
    Serial.println ("Blizzard Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("BLIZZARD WARNING"), xo, yo, display.color565(255, 165, 0)); 
  }
  else if (Event == "WINTER STORM WARNING")
  {
    Serial.println ("Winter Storm Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("WINTER STORM WRN"), xo, yo, display.color565(255, 165, 0)); 
  }
  else if (Event == "WINTER STORM WATCH")
  {
    Serial.println ("Winter Storm Watch");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("WINTER STORM WAT"), xo, yo, display.color565(255, 165, 0)); 
  }
  else if (Event == "FLOOD WARNING")
  {
    Serial.println ("Flood Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String(" FLOOD  WARNING "), xo, yo, display.color565(255, 165, 0)); 
  }
  else if (Event == "FLASH FLOOD WARNING")
  {
    Serial.println ("Flash Flood Warning");
    use_ani = 0;
    yo = 1;
    xo  = 0; TFDrawText (&display, String("FLASH FLOOD WRN "), xo, yo, display.color565(255, 165, 0)); 
  }
  else
  {
    Serial.println (" No Notable Severe Weather Alerts");
  }
 }
 delay (8000);
 draw_weather ();
 draw_date ();
}
//

void draw_FireEMSAlert ()
{
  Serial.println ("Fire EMS Alert check");
  WiFiClient client;
  if (client.connect(SERVER_NAME, SERVER_PORT)) {
    Serial.println("Fire/EMS email check: connected");
    // Make a HTTP request:
    client.println("GET /GetGmail.php");  // Apache server pathway.
    client.println();
    delay(2000);
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("Fire/EMS email check: connection failed");  //cannot connect to server
  }

  // if there's data ready to be read:
  if (client.available()) {  
    int i = 0;   
    //put the data in the array:
    do {
      Str[i] = client.read();
      i++;
      delay(1);
    } while (client.available());
     
    // Pop on the null terminator:
    Str[i] = '\0';
    //convert server's repsonse to a int so we can evaluate it
    num = atoi(Str); 
     
    Serial.print("Server's response: ");
    Serial.println(num);
    Serial.print("Previous response: ");
    Serial.println(prevNum);
    if (prevNum < 0)
    { //the first time around, set the previous count to the current count
      prevNum = num; 
      Serial.println("First email count stored.");
    }
    if (prevNum > num)
    { // handle if count goes down for some reason
      prevNum = num; 
    }
  }
  else
    {
    Serial.println("No response from server."); //cannot connect to server.
    }
    Serial.println("Disconnecting."); //disconnecting from server to reconnect
    client.stop();
    
    // ---------------- FIRE\EMS: ALERT FOR FIRE\EMS CALL ----------------   
    
    if(num > prevNum) {
    Serial.println("FIRE/EMS ALERT!");  //alert for new email
    prevNum = num;  //number of old emails =  number of new emails
    
    use_ani = 0;
    yo = 1;
    //xo  = 0; TFDrawText (&display, String("FIRE/EMS ALERT  "), xo, yo, display.color565(255, 255, 0));
    xo  = 0; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'F', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'I', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, '/', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'M', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'S', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(255, 255, 0));
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 255, 0));
    }
    else  //if email value is lower/equal to previous, no alert.
    {
    Serial.println("No New Alert Emails");
    }
}

//
void check_Network ()
{
  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device1);
  pingResult1 = Ping.ping(device1, 1);
  
  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device2);
  pingResult2 = Ping.ping(device2, 1);
  
  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device3);
  pingResult3 = Ping.ping(device3, 1);
  
  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device4);
  pingResult4 = Ping.ping(device4, 1);
  
  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device5);
  pingResult5 = Ping.ping(device5, 1);

  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device6);
  pingResult6 = Ping.ping(device6, 1);

  Serial.println("");
  Serial.println("Pinging ");
  Serial.print(device7);
  pingResult6 = Ping.ping(device7, 1);


  display.fillScreen (0);
  
  if (pingResult1 >= 0) {
    Serial.print("1. ONLINE");
    use_ani = 0;
    yo = 1;
    xo = 0; TFDrawChar  (&display, 'I', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("1. OFFLINE");
    use_ani = 0;
    yo = 1;
    xo = 0; TFDrawChar  (&display, 'I', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }

  if (pingResult2 >= 0) {
    Serial.print("2. ONLINE");
    yo = 7;
    xo = 0; TFDrawChar  (&display, 'F', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("2. OFFLINE");
    use_ani = 0;
    yo = 7;
    xo = 0; TFDrawChar  (&display, 'F', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }
  
  if (pingResult3 >= 0) {
    Serial.print("3. ONLINE");
    yo = 13;
    xo = 0; TFDrawChar  (&display, '.', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'S', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("3. OFFLINE");
    yo = 13;
    xo = 0; TFDrawChar  (&display, '.', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'S', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }

  if (pingResult4 >= 0) {
    Serial.print("4. ONLINE");
    yo = 19;
    xo = 0; TFDrawChar  (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'B', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'S', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'V', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("4. OFFLINE");
    yo = 19;
    xo = 0; TFDrawChar  (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'B', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'S', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'V', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }

  if (pingResult5 >= 0) {
    Serial.print("5. ONLINE");
    yo = 25;
    xo = 0; TFDrawChar  (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'X', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("5. OFFLINE");
    yo = 25;
    xo = 0; TFDrawChar  (&display, 'W', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'X', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }

  if (pingResult6 >= 0) {
    Serial.print("6. ONLINE");
    yo = 1;
    xo = 32; TFDrawChar  (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'M', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("6. OFFLINE");
    yo = 1;
    xo = 32; TFDrawChar  (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'M', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }

  if (pingResult7 >= 0) {
    Serial.print("7. ONLINE");
    yo = 7;
    xo = 32; TFDrawChar  (&display, 'P', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'G', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'U', xo, yo, display.color565(0, cin, 0)); 
    xo += 4; TFDrawChar (&display, 'P', xo, yo, display.color565(0, cin, 0)); 
  } else {
    Serial.print("7. OFFLINE");
    yo = 7;
    xo = 32; TFDrawChar  (&display, 'P', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'G', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, 'E', xo, yo, display.color565(cin, cin, cin)); 
    xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
    xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, 0, 0)); 
  }
  delay (6000);
  display.fillScreen (0);
  ntpsync = 1;
  draw_weather ();
  draw_date ();
}

//
void check_WMATA ()
{ 
  Serial.print("Metro: Connecting to ");
  Serial.println(WMATAServer);

  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  const int httpPort = 80;
  
  if (!client.connect(WMATAServer, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  String cmd = "GET /StationPrediction.svc/json/GetPrediction/";  cmd += stationCode;      // build request_string cmd
  cmd += "?api_key=";  cmd += myKey;  //
  cmd += " HTTP/1.1\r\nHost: api.wmata.com\r\n\r\n";            
  delay(500);
  client.print(cmd);                                            
  delay(500);
  unsigned int i = 0;                                           // timeout counter
  char json[wmata_buffer_size]="{";                                   // first character for json-string is begin-bracket 
  int n = 1;                                                    // character counter for json
  
  
  for (int j=0;j<num_elements;j++){                             // do the loop for every element/condition
    boolean quote = false; int nn = false;                      // if quote=fals means no quotes so comma means break
    while (!client.find(metroConds[j]))                         // If metro condition data is not available, try again.
    {
      Serial.println("No data available");
      use_ani = 0;
      yo = 1;
      xo = 0;  TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'W', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'M', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, '-', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'N', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'O', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'T', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, 'A', xo, yo, display.color565(cin, cin, 0)); 
      xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, cin, 0)); 
      return;
    }                            
  
    String Str1= metroConds[j];                                     // Str1 gets the name of element/condition
  
    for (int l=0; l<(Str1.length());l++)                        // for as many character one by one
        {json[n] = Str1[l];                                     // fill the json string with the name
         n++;}                                                  // character count +1
    while (i<5000) {                                            // timer/counter
      if(client.available()) {                                  // if character found in receive-buffer
        char c = client.read();                                 // read that character
           Serial.print(c);                                     // 
           
// ************************ construction of json string converting comma's inside quotes to dots ******************** 
               if ((c=='"') && (quote==false))                  // there is a " and quote=false, so start of new element
                  {quote = true;nn=n;}                          // make quote=true and notice place in string
               if ((c==',')&&(quote==true)) {c='.';}            // if there is a comma inside quotes, comma becomes a dot.
               if ((c=='"') && (quote=true)&&(nn!=n))           // if there is a " and quote=true and on different position
                  {quote = false;}                              // quote=false meaning end of element between ""
               if((c==',')&&(quote==false)) break;              // if comma delimiter outside "" then end of this element
 // ****************************** end of construction ******************************************************
          json[n]=c;                                            // fill json string with this character
          n++;                                                  // character count + 1
          i=0;                                                  // timer/counter + 1
        }
        i++;                                                    // add 1 to timer/counter
      }                    // end while i<5000
     if (j==num_elements-1)                                     // if last element
        {json[n]='}';}                                          // add end bracket of json string
     else                                                       // else
        {json[n]=',';}                                          // add comma as element delimiter
     n++;                                                       // next place in json string
  }
  Serial.println(json);                                         // debugging json string 
  parseJSON_metro(json);                                              // extract the conditions
  WMillis=millis();
}

void parseJSON_metro(char json[300])
{
  display.fillScreen (0);
  StaticJsonBuffer<buffer> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(json);
 
 if (!root.success())
{
  Serial.println("?fparseObject() failed");
  //return;
}

 
 //int Car                    = root["Car"];
 String Car                   = root["Car"];
 String Destination           = root["Destination"];
//const char* Destination     = root["Destination"];
 const char* DestinationCode  = root["DestinationCode"];
//const char* DestinationName = root["DestinationName"];
 String DestinationName       = root["DestinationName"];
 int Group                    = root["Group"];
 String Line                  = root["Line"];
 const char* LocationCode     = root["LocationCode"];
 //const char* LocationName   = root["LocationName"];
 String LocationName          = root["LocationName"];
 //const char* CMin           = root["Min"];
 String CMin                  = root["Min"];
 float Min                    = root["Min"];

 Car.toUpperCase();
 LocationName.toUpperCase();
 Destination.toUpperCase();
 Line.toUpperCase();
 
 Serial.println(Car);
 Serial.println(Destination);
  Serial.println("..");
 Serial.println(DestinationCode);
 Serial.println(DestinationName);
 Serial.println(Group);
 Serial.println(Line);
 Serial.println(LocationCode);
 Serial.println(CMin);
 Serial.println(Min);
 
 if (Line == "RD")
 {
  Serial.println("RED LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(cin, 0, 0));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo =  0; TFDrawChar (&display, 'R', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, 'D', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else if (Line == "OR")
 {
  Serial.println("ORANGE LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(255, 165, 0));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'O', xo, yo, display.color565(255, 165, 0));
  xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(255, 165, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(255, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
  
 }
 else if (Line == "YL")
 {
  Serial.println("YELLOW LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(cin, cin, 0));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'Y', xo, yo, display.color565(cin, cin, 0));
  xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(cin, cin, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else if (Line == "GR")
 {
  Serial.println("GREEN LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(0, cin, 0));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'G', xo, yo, display.color565(0, cin, 0));
  xo += 4; TFDrawChar (&display, 'R', xo, yo, display.color565(0, cin, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else if (Line == "BL")
 {
  Serial.println("BLUE LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(0, 0, 255));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'B', xo, yo, display.color565(0, 0, cin));
  xo += 4; TFDrawChar (&display, 'L', xo, yo, display.color565(0, 0, cin)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else if (Line == "SV")
 {
  Serial.println("SILVER LINE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display,"O", xo, yo, display.color565(cin, cin, cin));
  xo += 6; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'S', xo, yo, display.color565(cin, cin, cin));
  xo += 4; TFDrawChar (&display, 'V', xo, yo, display.color565(cin, cin, cin)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    

 }
 else if (Line == "--")
 {
  Serial.println("LINE/TRAIN OUT OF SERVICE");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  '-', xo, yo, display.color565(cin, cin, cin));
  xo += 4; TFDrawChar (&display, '-', xo, yo, display.color565(cin, cin, cin)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else if (Line == "No")
 {
  Serial.println("No Passenger");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawChar (&display,  'N', xo, yo, display.color565(cin, cin, cin));
  xo += 4; TFDrawChar (&display, 'O', xo, yo, display.color565(cin, cin, cin)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 else
 {
  Serial.println("LINE UNKNOWN");
  use_ani = 0;
  yo = 1;
  xo = 0; TFDrawText (&display, String(LocationName), xo, yo, display.color565(cin, cin, 0));
  yo = 7;
  xo = 0; TFDrawText (&display, "LN   CAR   MIN", xo, yo, display.color565(cin, 0, 0));
  yo = 13;
  xo = 0; TFDrawText (&display, String(Line), xo, yo, display.color565(cin, cin, cin));
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  if (Car == "8")
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(0, cin, 0)); 
  }
  else
  {
    xo += 4; TFDrawText (&display, String(Car), xo, yo, display.color565(cin, cin, 0));
  }
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawChar (&display, ' ', xo, yo, display.color565(cin, 0, 0)); 
  xo += 4; TFDrawText (&display, String(CMin), xo, yo, display.color565(cin, cin, 0));
  
  yo = 19;
  xo = 0; TFDrawText (&display, "DEST", xo, yo, display.color565(cin, 0, 0));
  yo = 25;
  xo = 0; TFDrawText (&display, String(Destination), xo, yo, display.color565(cin, cin, 0));
    
 }
 delay (8000);
 display.fillScreen (0);
 ntpsync = 1;
 draw_weather ();
 draw_date ();

}

//
void draw_date ()
{
  int cc_grn = display.color565 (0, cin, 0);
  Serial.println ("showing the date");
  //for (int i = 0 ; i < 12; i++)
    //TFDrawChar (&display, '0' + i%10, xo + i * 5, yo, display.color565 (0, 255, 0));
  //date below the clock
  long tnow = now();
  String lstr = "";
  for (int i = 0; i < 5; i += 2)
  {
    switch (date_fmt[i])
    {
      case 'D':
        lstr += (day(tnow) < 10 ? "0" + String(day(tnow)) : String(day(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'M':
        lstr += (month(tnow) < 10 ? "0" + String(month(tnow)) : String(month(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'Y':
        lstr += String(year(tnow));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
    }
  }
  //
  if (lstr.length())
  {
    //
    xo = 3*TF_COLS; yo = 26;
    TFDrawText (&display, lstr, xo, yo, cc_grn);
  }
}

void draw_animations (int stp)
{
#ifdef USE_ICONS
  //weather icon animation
  int xo = 4*TF_COLS; 
  int yo = 1;
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  if (use_ani)
  {
    int *af = NULL;
    //weather/night icon
    if (!daytime)
      af = mony_ani[stp%5];
    else
    {
      switch (condM)
      {
        case 1:
            af = suny_ani[stp%5];
          break;
        case 2:
            af = clod_ani[stp%5];
          break;
        case 3:
            af = ovct_ani[stp%5];
          break;
        case 4:
            af = rain_ani[stp%5];
          break;
        case 5:
            af = thun_ani[stp%5];
          break;
        case 6:
            af = snow_ani[stp%5];
          break;
      }
    }
    //draw animation
    if (af)
      DrawIcon (&display, af, xo, yo, 10, 5);
  }
#endif
}

#ifdef USE_FIREWORKS
//fireworks
// adapted to Arduino pxMatrix
// from https://r3dux.org/2010/10/how-to-create-a-simple-fireworks-effect-in-opengl-and-sdl/
// Define our initial screen width, height, and colour depth
int SCREEN_WIDTH  = 64;
int SCREEN_HEIGHT = 32;

const int FIREWORKS = 6;           // Number of fireworks
const int FIREWORK_PARTICLES = 8;  // Number of particles per firework

class Firework
{
  public:
    float x[FIREWORK_PARTICLES];
    float y[FIREWORK_PARTICLES];
    char lx[FIREWORK_PARTICLES], ly[FIREWORK_PARTICLES];
    float xSpeed[FIREWORK_PARTICLES];
    float ySpeed[FIREWORK_PARTICLES];

    char red;
    char blue;
    char green;
    char alpha;

    int framesUntilLaunch;

    char particleSize;
    boolean hasExploded;

    Firework(); // Constructor declaration
    void initialise();
    void move();
    void explode();
};

const float GRAVITY = 0.05f;
const float baselineSpeed = -1.0f;
const float maxSpeed = -2.0f;

// Constructor implementation
Firework::Firework()
{
  initialise();
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
  {
    lx[loop] = 0;
    ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
  }
}

void Firework::initialise()
{
    // Pick an initial x location and  random x/y speeds
    float xLoc = (rand() % SCREEN_WIDTH);
    float xSpeedVal = baselineSpeed + (rand() % (int)maxSpeed);
    float ySpeedVal = baselineSpeed + (rand() % (int)maxSpeed);

    // Set initial x/y location and speeds
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        x[loop] = xLoc;
        y[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
        xSpeed[loop] = xSpeedVal;
        ySpeed[loop] = ySpeedVal;
        //don't reset these otherwise particles won't be removed
        //lx[loop] = 0;
        //ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
    }

    // Assign a random colour and full alpha (i.e. particle is completely opaque)
    red   = (rand() % 255);/// (float)RAND_MAX);
    green = (rand() % 255); /// (float)RAND_MAX);
    blue  = (rand() % 255); /// (float)RAND_MAX);
    alpha = 50;//max particle frames

    // Firework will launch after a random amount of frames between 0 and 400
    framesUntilLaunch = ((int)rand() % (SCREEN_HEIGHT/2));

    // Size of the particle (as thrown to glPointSize) - range is 1.0f to 4.0f
    particleSize = 1.0f + ((float)rand() / (float)RAND_MAX) * 3.0f;

    // Flag to keep trackof whether the firework has exploded or not
    hasExploded = false;

    //cout << "Initialised a firework." << endl;
}

void Firework::move()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Once the firework is ready to launch start moving the particles
        if (framesUntilLaunch <= 0)
        {
            //draw black on last known position
            //display.drawPixel (x[loop], y[loop], cc_blk);
            lx[loop] = x[loop];
            ly[loop] = y[loop];
            //
            x[loop] += xSpeed[loop];

            y[loop] += ySpeed[loop];

            ySpeed[loop] += GRAVITY;
        }
    }
    framesUntilLaunch--;

    // Once a fireworks speed turns positive (i.e. at top of arc) - blow it up!
    if (ySpeed[0] > 0.0f)
    {
        for (int loop2 = 0; loop2 < FIREWORK_PARTICLES; loop2++)
        {
            // Set a random x and y speed beteen -4 and + 4
            xSpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
            ySpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
        }

        //cout << "Boom!" << endl;
        hasExploded = true;
    }
}

void Firework::explode()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Dampen the horizontal speed by 1% per frame
        xSpeed[loop] *= 0.99;

        //draw black on last known position
        //display.drawPixel (x[loop], y[loop], cc_blk);
        lx[loop] = x[loop];
        ly[loop] = y[loop];
        //
        // Move the particle
        x[loop] += xSpeed[loop];
        y[loop] += ySpeed[loop];

        // Apply gravity to the particle's speed
        ySpeed[loop] += GRAVITY;
    }

    // Fade out the particles (alpha is stored per firework, not per particle)
    if (alpha > 0)
    {
        alpha -= 1;
    }
    else // Once the alpha hits zero reset the firework
    {
        initialise();
    }
}

// Create our array of fireworks
Firework fw[FIREWORKS];

void fireworks_loop (int frm)
{
  int cc_frw;
  //display.fillScreen (0);
  // Draw fireworks
  //cout << "Firework count is: " << Firework::fireworkCount << endl;
  for (int loop = 0; loop < FIREWORKS; loop++)
  {
      for (int particleLoop = 0; particleLoop < FIREWORK_PARTICLES; particleLoop++)
      {
  
          // Set colour to yellow on way up, then whatever colour firework should be when exploded
          if (fw[loop].hasExploded == false)
          {
              //glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
              cc_frw = display.color565 (255, 255, 0);
          }
          else
          {
              //glColor4f(fw[loop].red, fw[loop].green, fw[loop].blue, fw[loop].alpha);
              //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
              cc_frw = display.color565 (fw[loop].red, fw[loop].green, fw[loop].blue);
          }

          // Draw the point
          //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
          display.drawPixel (fw[loop].x[particleLoop], fw[loop].y[particleLoop], cc_frw);
          display.drawPixel (fw[loop].lx[particleLoop], fw[loop].ly[particleLoop], 0);
      }
      // Move the firework appropriately depending on its explosion state
      if (fw[loop].hasExploded == false)
      {
          fw[loop].move();
      }
      else
      {
          fw[loop].explode();
      }
      //
      //delay (10);
  }
}
//-
#endif //define USE_FIREWORKS

byte prevhh = 0;
byte prevmm = 0;
byte prevss = 0;
long tnow;
void loop()
{
	static int i = 0;
	static int last = 0;
  static int cm;
  //time changes every miliseconds, we only want to draw when digits actually change
  tnow = now ();
  //
  hh = hour (tnow);   //NTP.getHour ();
  mm = minute (tnow); //NTP.getMinute ();
  ss = second (tnow); //NTP.getSecond ();
  //animations?
  cm = millis ();
  //

//#define SHOW_WMATA
#ifdef SHOW_WMATA
    if (ss == 50)
       check_WMATA ();
    else
      //Serial.println ("WMATA: It is not currently on the 45 second mark.");
#endif

//#define SHOW_WX_ALERT
#ifdef SHOW_WX_ALERT
      if (ss == 15)
        check_NWS ();
      else
        //Serial.println("Weather Alerts: It is not currently on the 15 second mark.");
#endif

#ifdef SHOW_NETWORK_STATUS
    if (ss == 37)
       check_Network ();
    else
      //Serial.println ("CHECK NETWORK: It is not currently on the 45 second mark.");
#endif

#ifdef USE_FIREWORKS
  //fireworks on 1st of Jan 00:00, for 55 seconds
  if (1 && (month (tnow) == 1 && day (tnow) == 1 && hh == 0 && mm == 0) || (month (tnow) == 7 && day (tnow) == 4 && hh == 0 && mm == 0))
  {
    if (ss > 0 && ss < 30)
    {
      if ((cm - last) > 50)
      {
        //Serial.println(millis() - last);
        last = cm;
        i++;
        fireworks_loop (i);
      }
      ntpsync = 1;
      return;
    }
    Serial.println("Not time for Fireworks");
  }
#endif //define USE_FIREWORKS

  //weather animations
  if ((cm - last) > 150)
  {
    //Serial.println(millis() - last);
    last = cm;
    i++;
    //
    draw_animations (i);
    //
  }
  //
  if (ntpsync)
  {
    ntpsync = 0;
    //
    prevss = ss;
    prevmm = mm;
    prevhh = hh;
    //brightness control: dimmed during the night(25), bright during the day(150)
    if (hh >= 20 && cin == 150)
    {
      cin = 25;
      Serial.println ("night mode brightness");
      daytime = 0;
    }
    if (hh < 8 && cin == 150)
    {
      cin = 25;
      Serial.println ("night mode brightness");
      daytime = 0;
    }
    //during the day, bright
    if (hh >= 8 && hh < 20 && cin == 25)
    {
      cin = 150;
      Serial.println ("day mode brightness");
      daytime = 1;
    }
    //we had a sync so draw without morphing
    int cc_gry = display.color565 (128, 128, 128);
    int cc_dgr = display.color565 (30, 30, 30);
    //dark blue is little visible on a dimmed screen
    //int cc_blu = display.color565 (0, 0, cin);
    int cc_col = cc_gry;
    //
    if (cin == 25)
      cc_col = cc_dgr;
    //reset digits color
    digit0.SetColor (cc_col);
    digit1.SetColor (cc_col);
    digit2.SetColor (cc_col);
    digit3.SetColor (cc_col);
    digit4.SetColor (cc_col);
    digit5.SetColor (cc_col);
    //clear screen
    display.fillScreen (0);
    //date and weather
    draw_weather ();
    draw_date ();
    //
    digit1.DrawColon (cc_col);
    digit3.DrawColon (cc_col);
    //military time?
    if (hh > 12 && military[0] == 'N')
      hh -= 12;
    //
    digit0.Draw (ss % 10);
    digit1.Draw (ss / 10);
    digit2.Draw (mm % 10);
    digit3.Draw (mm / 10);
    digit4.Draw (hh % 10);
    digit5.Draw (hh / 10);
  }
  else
  {
    //seconds
    if (ss != prevss) 
    {
      int s0 = ss % 10;
      int s1 = ss / 10;
      if (s0 != digit0.Value ()) digit0.Morph (s0);
      if (s1 != digit1.Value ()) digit1.Morph (s1);
      //ntpClient.PrintTime();
      prevss = ss;
      //refresh weather every 5mins at 30sec in the minute
      if (ss == 30 && ((mm % 5) == 0))
        getWeather ();
    }
    //minutes
    if (mm != prevmm)
    {
      int m0 = mm % 10;
      int m1 = mm / 10;
      if (m0 != digit2.Value ()) digit2.Morph (m0);
      if (m1 != digit3.Value ()) digit3.Morph (m1);
      prevmm = mm; // draw weather every minute on the mark
      //
      draw_weather ();
      draw_date ();

//#define SHOW_SOME_LOVE
#ifdef SHOW_SOME_LOVE
        if (mm == 0)
          draw_love ();
        else
          Serial.println("No Love, It's not on the hour mark.");
#endif

//#define SHOW_SOME_PRIDE
#ifdef SHOW_SOME_PRIDE
      if ((month (tnow) == 6))
        if (mm == 0)
          draw_pride ();
        else
          Serial.println("Pride Month: Not on the hour mark.");
      else if ((month (tnow) == 10 && day (tnow) == 11))
        if (mm == 0)
          draw_OutDay ();
        else
          Serial.println("National Coming Out Day: Not on the hour mark.");
      else
        Serial.println("No current Pride events");
#endif

//#define SHOW_FIREEMS_ALERT
#ifdef SHOW_FIREEMS_ALERT
      if (ss == 0)
        draw_FireEMSAlert ();
      else
      Serial.println("");
#endif
 
        //draw_weather ();
    }
    //hours
    if (hh != prevhh) 
    {
      prevhh = hh;
      //
      //draw_date ();
      //brightness control: dimmed during the night(25), bright during the day(150)
      if (hh == 20 || hh == 8)
      {
        ntpsync = 1;
        //bri change is taken care of due to the sync
      }
      //military time?
      if (hh > 12 && military[0] == 'N')
        hh -= 12;
      //
      int h0 = hh % 10;
      int h1 = hh / 10;
      if (h0 != digit4.Value ()) digit4.Morph (h0);
      if (h1 != digit5.Value ()) digit5.Morph (h1);
    }//hh changed
    
  }
  //set NTP sync interval as needed
  if (NTP.getInterval() < 3600 && year(now()) > 1970)
  {
    //reset the sync interval if we're already in sync
    NTP.setInterval (3600 * 24);//re-sync once a day
  }
  //
	//delay (0);
}
