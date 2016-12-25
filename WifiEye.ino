#define IP_FORWARD  1 // ...probably doesn't work.

#include <ESP8266WiFi.h>
//#include <ESP8266WiFiGeneric.h>
// #include "ESP8266WiFi.h"
// #include "ESP8266WiFiGeneric.h"
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <string>
#include <math.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <ArduinoJson.h>
// #include <DNSServer.h>

// global events queue
AsyncEventSource events("/events");

// default local hostname (used if no config file is found)
static String local_hostname = String("esp");
// service access point config (used if no config file is found)
static String ap_ssid = String("ESP");
static String ap_password = String("");
// http authentication config (used if no config file is found)
static String htaccess_username = String("admin");
static String htaccess_password = String("esp");


// include all the filesystem functions
#include "filesystem.h"

// include all the "special effects" (Neopixel and Servo) functions
#include "fx.h"
// include our wifi setup and functions
#include "wifi.h"

// include default (static) content
#include "default_content.h"

// include http server dynamic content
#include "http_server.h"

// set up websockets server
#include "ws_server.h"



void serial_setup(){
  // initialize serial now
  Serial.begin(115200);
  // if we're not developing, we don't need the debug output
  Serial.setDebugOutput(false);
}

// server content setup
#include "server_content.h"

// configure the ESP8266 to internal voltage mode
ADC_MODE(ADC_VCC);


// wifi scan timer
Ticker rescan_timer;
void rescan_tick() {
  if(scan_state == SCAN_STATE_IDLE) scan_state = SCAN_STATE_START;
}


// status update timer
int status_state = 0;
Ticker status_timer;
void status_tick() {
  if(status_state == 0) status_state = 1;
}


void setup(){
  // initialize the neopixel
  fastled_setup();
  // I'm blue (abadee abadow...)
  leds[0] = CRGB::Blue; FastLED.show();
  // then the serial port
  serial_setup();
  // now the filesystem
  SPIFFS.begin();
  // load config files
  load_hostname(); // local hostname
  load_ap_config(); // access point configuration
  load_htaccess_config(); // web server authorization

  // start wifi and ota
  wifi_setup();
  ota_setup();

  // network services
  MDNS.addService("http","tcp",80);
  server_setup();
  websockets_setup();
  server.begin();
  
  // start the DNS redirector
  // setupDNS();
  
  // schedule rescan check every 10s
  rescan_timer.attach(10.0, rescan_tick);
  // and status updates every 5s
  status_timer.attach(5.0, status_tick);

  // everything is supergreen
  leds[0] = CRGB::Green; FastLED.show();
}


void loop(){
  // Handle OTA events first, just in case
  //ArduinoOTA.handle();
  
  wifi_loop();
  fx_loop();
  
  // status broadcast state
  switch(status_state) {
    case 1: // emit status now
      status_state = 0;
      char cb[64];
      sprintf(cb,"{\"status\":{\"vcc\":%d,\"rssi\":%d}}",
        ESP.getVcc(),
        WiFi.RSSI()
      );
      events.send(cb,"status");
      break;
  }

  /*
  // manage the wifi system
  switch(wifi_state_change) {
    case WIFI_STATE_RECONNECT:
      switch(wifi_state) {
        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_IDLE:
          // I'm not blue
          leds[0] = CRGB::Orange; FastLED.show();
          // disconnect, then fall through
          WiFi.disconnect(false);
          delay(1000);
        case WIFI_STATE_OFF:
        case WIFI_STATE_STOP:
        case WIFI_STATE_SEARCH:
        default:
          // reconnect now
          wifi_reconnect();
          // clear the state change
          wifi_state_change = WIFI_STATE_OFF;
          break;
      }
      break;
    case WIFI_STATE_SEARCH: 
      switch(wifi_state) {
        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_IDLE:
          // I'm not blue
          leds[0] = CRGB::Orange; FastLED.show();
          // disconnect
          WiFi.disconnect(false);
          delay(1000);
          // fall through
        case WIFI_STATE_OFF:
        case WIFI_STATE_STOP:
        case WIFI_STATE_SEARCH:
          // begin the search
          wifi_state = WIFI_STATE_SEARCH;
          wifi_state_change = WIFI_STATE_OFF;
          // make sure the scan is running
          switch(scan_state) {
            case SCAN_STATE_OFF:
            case SCAN_STATE_STOP:
              scan_state_change = SCAN_STATE_START;
              break;
          }
          break;
      }
      break;
    case WIFI_STATE_STOP:
      WiFi.disconnect(false);
      delay(1000);
      wifi_state = WIFI_STATE_OFF;
      // clear the state change
      wifi_state_change = WIFI_STATE_OFF;
  }
  
  // manage the scan system
  switch(scan_state_change) {
    case SCAN_STATE_START: 
      switch(scan_state) {
        case SCAN_STATE_OFF:
          scan_state = SCAN_STATE_START;
          events.send("{\"scan\":{\"state\":\"started\"}}", "scan");
          scan_state_change = SCAN_STATE_OFF;
          break;
      }
      break;
    case SCAN_STATE_STOP: 
      switch(scan_state) {
        case SCAN_STATE_ACTIVE:
        case SCAN_STATE_IDLE:
          scan_state = SCAN_STATE_STOP;
          events.send("{\"scan\":{\"state\":\"stopped\"}}", "scan");
          scan_state_change = SCAN_STATE_OFF;
          break;
      }
      break;
  }
*/

/*
  // manage the servo system
  switch(servos_state_change) {
    case SERVO_STATE_START: 
      switch(servos_state) {
        case SERVO_STATE_OFF: servos_start(); servos_state_change = SERVO_STATE_OFF; break;
      }
      break;
    case SERVO_STATE_STOP: 
      switch(servos_state) {
        case SERVO_STATE_ACTIVE: servos_stop(); servos_state_change = SERVO_STATE_OFF; break;
      }
      break;
  }
*/
/*
  // manage the logging system
  switch(logging_state_change) {
    case LOGGING_STATE_START: 
      switch(logging_state) {
        case LOGGING_STATE_OFF: logging_start(); logging_state_change = LOGGING_STATE_OFF; break;
      }
      break;
    case LOGGING_STATE_STOP: 
      switch(logging_state) {
        case LOGGING_STATE_ACTIVE: logging_stop(); logging_state_change = LOGGING_STATE_OFF; break;
      }
      break;
  }


  // WiFi scanning
  switch(scan_state) {
    case SCAN_STATE_OFF: // not scanning
      break;
    case SCAN_STATE_START: // begin async scan for all networks
      scan_start_event();
      // leds[0] = CRGB::Yellow; FastLED.show();
      WiFi.scanNetworks(true, true);
      scan_state = SCAN_STATE_ACTIVE;
      // open the eye during the scan
      //servo_move(0,90,24);
      //servo_move(1,90,24);
      break;
    case SCAN_STATE_ACTIVE: // async scanning, check for results
      r = WiFi.scanComplete();
      switch(r) {
        case WIFI_SCAN_RUNNING: 
          break;
        case WIFI_SCAN_FAILED: 
          // scan error?
          scan_state = SCAN_STATE_STOP;
          leds[0] = CRGB::Red; FastLED.show();
          break;
        default:
          // scan complete, count returned
          scan_count = r;
          leds[0] = CRGB::Green; FastLED.show();
          // scan is complete
          async_wifi_scan_complete();
          // do report lines after combining with files.
          scan_report_index = 0;
          scan_state = SCAN_STATE_REPORT;
      }
      break;
    case SCAN_STATE_REPORT: // scan complete, sending reports
      // more to do?
      if(scan_report_index < scan_count) {
        // report on the next entry
        scan_report_event(scan_report_index++);
      } else {
        // finish, then go idle
        scan_complete_event();
        scan_state = SCAN_STATE_IDLE;
      }
      break;
    case SCAN_STATE_IDLE: // scan complete, waiting for timer
      break;
    case SCAN_STATE_STOP: // scan shutdown
      scan_state = SCAN_STATE_OFF;
      
  }
  */

  // LED effects
  switch(scan_state) {
    case SCAN_STATE_ACTIVE: // async scanning
      // still busy
      fastled_noise();
      leds[0] = leds[0] | (CRGB)CRGB(64,128,16);  LEDS.show();
      break;
    case SCAN_STATE_OFF: // not scanning
    case SCAN_STATE_IDLE: // scan complete
      // still busy
      fastled_noise();
      leds[0] = leds[0] | (CRGB)CRGB::Green;  LEDS.show();
      break;
  }
  
  // if we can sleep for a moment, do so
  /* if(wifi_state == 3) {
    wifi_set_sleep_type(MODEM_SLEEP_T)
  } */
  delay(10);
}



