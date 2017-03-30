#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <string>
#include <math.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
// #include <DNSServer.h>

// global events queue
AsyncEventSource events("/events");

// default local hostname (used if no config file is found)
static String local_hostname = String("esp");
// service access point config (used if no config file is found)
static String ap_ssid = String("ESP");
static String ap_password = String("");
static  float ap_txpower = 10.0; // 50% power 

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

// server content setup
#include "server_content.h"


void serial_setup(){
  // initialize serial now
  Serial.begin(115200);
  // if we're not developing, we don't need the debug output
  Serial.setDebugOutput(false);
}


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
  // load configuration for each servo
  for(int i=0; i<SERVO_COUNT; i++) load_servo_config(i+1, servo_config[i]);
  // load initial servo state
  load_fx_config(); // fx config
  servos_state_change = string_servo_state( fx_servo_start );
  // everything is ready for the fx system to do poses
  fx_pose(String("startup")); // earliest chance to apply a pose
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

  // everything is supergreen (unless the ready state overrides the color)
  leds[0] = CRGB::Green; 
  fx_pose(String("ready"));
}

void loop(){
  // Handle WiFi/OTA events first, just in case
  wifi_loop();
  // do light/servo effects
  fx_loop();
  // status broadcast state
  switch(status_state) {
    case 1: // emit status now
      status_state = 0;
      int status_vcc = ESP.getVcc();
      int status_rssi =  WiFi.RSSI();
      char cb[64];
      sprintf(cb,"{\"status\":{\"vcc\":%d,\"rssi\":%d,\"color\":\"#%02X%02X%02X\" }}",
        status_vcc,
        status_rssi,
        leds[0][0], leds[0][1], leds[0][2]
      );
      events.send(cb,"status");
      // add to the entropy pool
      random16_add_entropy(status_rssi);
      random16_add_entropy(status_vcc);
      break;
  }
  // if we can sleep for a moment, do so
  delay(10);
}



