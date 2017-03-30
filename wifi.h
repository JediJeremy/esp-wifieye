
// initial connection settings
static String  apconnect_ssid = String("kamatarj");
static String  apconnect_password = String("shamballa");
static boolean apconnect_has_ota = true;

// 
static uint8_t apconnect_bssid[6];
static boolean apconnect_has_bssid = false;

/* 
 * connection skip list - access points we skip connecting to for a while 
 * (usually because they denied an earlier connection request) 
 */
#define SKIP_LIST_COUNT 4
static uint8_t skip_bssid[SKIP_LIST_COUNT * 6];
static uint8_t skip_counter[SKIP_LIST_COUNT];

void skip_list_add(uint8_t * bssid, uint8_t set_counter = 50) {
  // go through the skip list and check if the entry is already there
  int lowest = -1; int lowest_count = 256;
  for(int i=0; i<SKIP_LIST_COUNT; i++) {
    uint8_t c = skip_counter[i];
    if(c > 0) {
      if( memcmp( &skip_bssid[i * 6], bssid, 6) == 0 ) {
        // found it. reset the counter
        skip_counter[i] = set_counter;
        return;
      } else {
        // is this the lowest count?
        if(c < lowest_count) {
          // nearly empty slot
          lowest = i; lowest_count = c;
        }
      }
    } else {
      // empty slot
      lowest = i; lowest_count = 0;
    }
  }
  // wasn't there. use the least necessary slot to store the index
  memcpy( &skip_bssid[lowest * 6], bssid, 6 );
  // and set the counter
  skip_counter[lowest] = set_counter;
}

boolean skip_list_contains(uint8_t * bssid) {
  // go through the skip list and check if the entry is already there
  for(int i=0; i<SKIP_LIST_COUNT; i++) {
    uint8_t c = skip_counter[i];
    if(c > 0) {
      if( memcmp( &skip_bssid[i * 6], bssid, 6) == 0 ) {
        // found it.
        return true;
      }
    }
  }
  // wasn't there.
  return false;
}

void skip_list_decrement() {
  for(int i=0; i<SKIP_LIST_COUNT; i++) {
    if(skip_counter[i] > 0) skip_counter[i]--;
  }
}

// wifi state
#define WIFI_STATE_OFF       0
#define WIFI_STATE_START     1
#define WIFI_STATE_IDLE      2
#define WIFI_STATE_SERVICE   3
#define WIFI_STATE_CONNECTED 4
#define WIFI_STATE_STOP      5
#define WIFI_STATE_RECONNECT 6
#define WIFI_STATE_SEARCH    7

const char * wifi_state_name(int scan_state) {
  switch(scan_state) {
    case WIFI_STATE_OFF: return "stopped";
    case WIFI_STATE_START: return "started";
    case WIFI_STATE_CONNECTED: return "started";
    case WIFI_STATE_IDLE: return "started";
    case WIFI_STATE_STOP: return "stopped";
  }
  return "";
}

int wifi_state = WIFI_STATE_OFF;
int wifi_state_change = WIFI_STATE_OFF;

// over-the-air programming state
#define OTA_STATE_OFF      0
#define OTA_STATE_START    1
#define OTA_STATE_ACTIVE   2
#define OTA_STATE_STOP     4

int ota_state = OTA_STATE_OFF;
int ota_state_change = OTA_STATE_OFF;

// scan state
#define SCAN_STATE_OFF      0
#define SCAN_STATE_START    1
#define SCAN_STATE_ACTIVE   2
#define SCAN_STATE_REPORT   6
#define SCAN_STATE_IDLE     3
#define SCAN_STATE_STOP     4

const char * scan_state_name(int scan_state) {
  switch(scan_state) {
    case SCAN_STATE_OFF: return "stopped";
    case SCAN_STATE_START: return "started";
    case SCAN_STATE_ACTIVE: return "started";
    case SCAN_STATE_IDLE: return "started";
    case SCAN_STATE_STOP: return "stopped";
  }
  return "";
}

int scan_state = SCAN_STATE_OFF;
int scan_state_change = SCAN_STATE_OFF;
int scan_count = 0;
unsigned long scan_time = 0;

int search_index = 0;
uint8_t search_bssid[6];
int search_priority;
int search_rssi;
boolean search_ota;

// scan logging state
#define LOGGING_STATE_OFF     0
#define LOGGING_STATE_START   1
#define LOGGING_STATE_ACTIVE  2
#define LOGGING_STATE_STOP    3

const char * logging_state_name(int scan_state) {
  switch(scan_state) {
    case LOGGING_STATE_OFF: return "stopped";
    case LOGGING_STATE_START: return "started";
    case LOGGING_STATE_ACTIVE: return "started";
    case LOGGING_STATE_STOP: return "stopped";
  }
  return "";
}

int logging_state = LOGGING_STATE_OFF;
int logging_state_change = LOGGING_STATE_OFF;

void logging_start() {
    logging_state = LOGGING_STATE_ACTIVE;
    events.send("{\"log\":{\"state\":\"started\"}}", "log");
}

void logging_stop() {
    logging_state = LOGGING_STATE_OFF;
    events.send("{\"log\":{\"state\":\"stopped\"}}", "log");
}

void logging_storage() {
  char p[96];
  // get the filesystem info
  FSInfo fs_info;  
  SPIFFS.info(fs_info);
  // sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
  sprintf(p, "{\"log\":{\"storage\":{\"used\":%u,\"total\":%u}}}", fs_info.usedBytes, fs_info.totalBytes);
  events.send(p, "log");
}

void logging_clear_all() {
  // delete the log file
  SPIFFS.remove("/www/log.txt");
  logging_storage();
}

int scan_report_index = 0;
int scan_tracking_index = -1;
int scan_tracking_rssi = -200;
CRGB scan_tracking_color = 0;

int ap_count = 0;
int ap_happy = 0;
int ap_angry = 0;
int ap_track = 0;

// compose and send a scan start message
void scan_start_event() {
  // update pose during the scan
  fx_pose("scan");
  // report the scan to our clients
  scan_time = millis();
  char p[32];
  sprintf(p, "{\"scan\":{\"start\":%u}}", scan_time );
  events.send(p, "scan");
}

// the scan is complete, prepare to report
void async_wifi_scan_complete() {
  char p[192];
  char json_name[128];
  // write to the log file
  File log_file;
  if(logging_state == LOGGING_STATE_ACTIVE) {
    log_file = SPIFFS.open("/www/log.txt", "a");
    if (!log_file) {
      events.send("{\"log\":{\"error\":\"file open failed\"}}", "log");
    } else {
       for (int i = 0; i < scan_count; i++) {
        // enumerate the encryption type
        char * encrypt_str = "none";
        switch(WiFi.encryptionType(i)) {
          // case ENC_TYPE_NONE:  response->print("none"); break;
          case ENC_TYPE_WEP:   encrypt_str = "wep"; break;
          case ENC_TYPE_TKIP:  encrypt_str = "tkip"; break;
          case ENC_TYPE_CCMP:  encrypt_str = "ccmp"; break;
          case ENC_TYPE_AUTO:  encrypt_str = "auto"; break;
        }
        // store it to the file
        log_file.printf("%u,\"", 
          scan_time
        );
        // special encoding for the name
        string_to_csv(log_file, 
          WiFi.SSID(i)
        );
        log_file.printf("\",%s,%s,%d,%d\n", 
          WiFi.BSSIDstr(i).c_str(), 
          encrypt_str,
          WiFi.channel(i), 
          WiFi.RSSI(i)
        );
      }
      // close the file
      log_file.close();
    }
  }
  // clear the search results before the reporting loop
  search_index = -1;
  // clear the old search result
  scan_tracking_index = -1;
  scan_tracking_rssi = -200;
  scan_tracking_color = 0;
  // reset report item index
  scan_report_index = 0;
  // keep some stats
  ap_count = 0;
  ap_happy = 0;
  ap_angry = 0;
  ap_track = 0;
}

void scan_report_event(int index) {
  char json_name[128];
  // sanity check the index
  if(index<0) return;
  if(index>=scan_count) return;
  // encode the ssid name into json, we'll need it eventually
  string_to_json(json_name, 128, WiFi.SSID(index) );
  // enumerate the encryption type
  char * encrypt_str = "none";
  switch(WiFi.encryptionType(index)) {
    // case ENC_TYPE_NONE:  response->print("none"); break;
    case ENC_TYPE_WEP:   encrypt_str = "wep"; break;
    case ENC_TYPE_TKIP:  encrypt_str = "tkip"; break;
    case ENC_TYPE_CCMP:  encrypt_str = "ccmp"; break;
    case ENC_TYPE_AUTO:  encrypt_str = "auto"; break;
  }
  // deference the bssid
  byte * bssid = WiFi.BSSID(index);
  // is this the same entry as the currently connected access point?
  boolean is_ap = memcmp( WiFi.BSSID(), bssid, 6 ) == 0;
  // now look around the filesystem and see if we have config
  boolean file_found = false;
  // try to look up the bssid config
  String filename = String("/wifi/mac/")+ WiFi.BSSIDstr(index);
  const char * fn = filename.c_str();
  if(SPIFFS.exists(fn)) {
    file_found = true;
  } else {
    // // try to look up the ssid config
    filename = String("/wifi/ap/")+ WiFi.SSID(index);
    fn = filename.c_str();
    if(SPIFFS.exists(fn)) {
      file_found = true;
    }   
  }
  // success. look for the extended fields and send them too
  boolean x_ota = false;
  boolean x_auto = false;
  int     x_priority = 0;
  String  x_fx = String("none");
  String  x_color = String("");
  // did we find a file?
  boolean send_extended = false;
  if(file_found) {
    // // open the file
    load_json_then(fn, 1024, [&x_ota,&x_auto,&x_priority,&x_fx,&send_extended,&x_color](JsonObject& root){
      // success. look for the extended fields and send them too
      x_ota = root["ota"];
      x_auto = root["auto"];
      x_priority = root["priority"];
      x_fx = root["fx"].asString();
      x_color = root["color"].asString();
      send_extended = true;
    }, [](){
      // failure.
      events.send("{\"error\":\"could not load config \"}");
    });
  }
  // briefly flash the LED to the AP color as we 'see' it, use current color as fallback
  LEDS.setBrightness(255);
  leds[0] = parse_color_html(x_color.c_str(), leds[0]); LEDS.show();
  // check if this entry is a tracking candidate
  bool track = (x_fx == "track") || (x_fx == "happy") || (x_fx == "angry");
  // [todo:] also check against explicit tracking lists?
  // are we strongest?
  int rssi = WiFi.RSSI(index);
  if(track && (scan_tracking_rssi < rssi)) {
    // then we're the new best option
    scan_tracking_index  =  index;
    scan_tracking_color  =  parse_color_html(x_color.c_str(), CRGB::Green);
    scan_tracking_rssi  =  rssi;
  }
  // broadcast the result
  char p[256];
  if(send_extended) {
    // extended access point scan information
    sprintf(p, "{\"scan\":{\"ssid\":\"%s\", \"bssid\":\"%s\", \"encrypt\":\"%s\", \"channel\":%d, \"rssi\":%d, \"ap\":%s, \"ota\":%s, \"auto\":%s, \"priority\":%d, \"fx\":\"%s\", \"color\":\"%s\" }}\n", 
      json_name, // WiFi.SSID(i).c_str(), 
      WiFi.BSSIDstr(index).c_str(), 
      encrypt_str,
      WiFi.channel(index), 
      rssi,
      is_ap ? "true" : "false",
      x_ota ? "true" : "false",
      x_auto ? "true" : "false",
      x_priority,
      x_fx.c_str(),
      x_color.c_str()
    );
  } else {
    // basic access point scan information
    sprintf(p, "{\"scan\":{\"ssid\":\"%s\", \"bssid\":\"%s\", \"encrypt\":\"%s\", \"channel\":%d, \"rssi\":%d, \"ap\":%s }}\n", 
      json_name, // WiFi.SSID(i).c_str(), 
      WiFi.BSSIDstr(index).c_str(), 
      encrypt_str,
      WiFi.channel(index), 
      rssi,
      is_ap ? "true" : "false"
    );
  }
  events.send(p,"scan");
  // can we autoconnect?
  if(x_auto) {
    // are we a better connection option?
    boolean better = false;
    if(search_index == -1) {
      // first one, we're the best
      better = true;
    } else {
      // are our metrics better?
      if(x_priority > search_priority) {
        // we're better based on priority
        better = true;
      } else if(x_priority == search_priority) {
        // equal priority.
        if( WiFi.RSSI(index) > search_rssi ) {
          // we're better based signal strength
          better = true;
        }
      }
    }
    // make sure we're not excluded by the skip list
    // if(better && skip_list_contains(bssid)) better = false;
    // if we're better, store our metrics in the search result
    if(better) {
      search_index = index;
      search_priority = x_priority;
      search_ota = x_ota;
      search_rssi = WiFi.RSSI(index);
      memcpy(search_bssid, bssid, 6);
    }
  }
  // apply other special effects properties
  // does the special effect require it?
  if(!(x_fx == "ignore")) ap_count++;
  if(x_fx == "happy") ap_happy++;
  if(x_fx == "angry") ap_angry++;
}

// compose and send a scan complete message
void scan_report_complete() {
  // adopt a new pose based on the scan result
  if(ap_angry>0) {
    fx_pose("angry");
  } else if(ap_happy>0) {
    fx_pose("happy");
  } else if(ap_count>0) {
    // check for tracking target
    if(scan_tracking_index != -1) { 
      // transition to the tracking color
      fx_pose("track");
      if(scan_tracking_color) {
        // we assume the pose has started a color transition
        RGBPose * pose = &color_pose[0];
        if(pose->animate) {
          // replace the default color with the AP-specific one
          pose->target = scan_tracking_color;
        } else {
          // change color to the AP
          led_animate(0, scan_tracking_color, 50);      
        }
      }
    } else {
      // start with the generic detection pose
      fx_pose("detect");      
    }
  } else {
    fx_pose("idle");
  }
  
  // update the LED pulsing parameters
  pulse_track = scan_tracking_index==-1 ? 0 : scan_tracking_rssi;
  // pulse_update(scan_tracking_index, scan_tracking_rssi);
  
  
  // let people know what the best pick for ap would be
  char p[128];
  if(search_index >=0) {
    // do an expanded search for the AP password
    String password = wifi_lookup_password(WiFi.SSID(search_index), WiFi.BSSIDstr(search_index), true);
    // if we are searching for an AP, then copy over the details and flag a reconnect
    if(wifi_state == WIFI_STATE_SEARCH) {
      apconnect_ssid = WiFi.SSID(search_index);
      memcpy(apconnect_bssid, search_bssid, 6);
      apconnect_has_bssid = true;
      apconnect_has_ota = search_ota;
      apconnect_password = password;
      wifi_state_change = WIFI_STATE_RECONNECT;
    }
    // let people know what the best pick for ap would be
    byte * m = search_bssid;
    sprintf(p, "{\"autoconnect\":{\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"password\":%s,\"priority\":%d,\"ota\":%s}}", 
      m[0],m[1],m[2],m[3],m[4],m[5], 
      (password=="") ? "false":"true",
      search_priority, 
      search_ota ? "true":"false" 
    );
    events.send(p, "scan");
  }
  // all done
  sprintf(p, "{\"scan\":{\"complete\":%u}}", millis() );
  events.send(p, "scan");
  // if we are logging, send a storage update
  if(logging_state == LOGGING_STATE_ACTIVE) logging_storage();
  // decay entries from the skip list - the AP's we ignore for a while because we couldn't connect. they might let us now?
  skip_list_decrement();
  //  all done
}

void wifi_event(WiFiEvent_t event) {
  static int i=0;
  switch(event) {
    case WIFI_EVENT_ANY:
      break;
    case WIFI_EVENT_MODE_CHANGE:
      break;
    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
      events.send("{\"wifi\":{\"event\":\"ap_probe\"}}","wifi");
      break;
    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
      events.send("{\"wifi\":{\"event\":\"ap_connect\"}}","wifi");
      break;
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
      events.send("{\"wifi\":{\"event\":\"ap_disconnect\"}}","wifi");
      break;
    case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
      events.send("{\"wifi\":{\"event\":\"ap_authmode\"}}","wifi");
      break;
    case WIFI_EVENT_STAMODE_CONNECTED:
      events.send("{\"wifi\":{\"event\":\"station_connect\"}}","wifi");
      break;
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      events.send("{\"wifi\":{\"event\":\"dhcp_timeout\"}}","wifi");
      break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      events.send("{\"wifi\":{\"event\":\"station_disconnect\"}}","wifi");
      break;
    case WIFI_EVENT_STAMODE_GOT_IP:
      // events.send("{\"wifi\":{\"event\":\"got_ip\"}}","wifi");
      char p[64];
      sprintf(p, "{\"wifi\":{\"event\":\"got_ip\",\"ip\":\"%s\"}}", WiFi.localIP().toString().c_str());
      events.send(p, "wifi");
      break;
  }
}

void station_connected(const WiFiEventSoftAPModeStationConnected& event) {
  // events.send("{\"wifi\":{\"event\":\"ap_connected\"}}","wifi");
  char p[128];
  const uint8 * m = event.mac;
  sprintf(p, "{\"wifi\":{\"event\":\"client_connect\",\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"aid\":\"%x\"}}", m[0], m[1], m[2], m[3], m[4], m[5], event.aid);
  events.send(p, "wifi");
}

void station_disconnected(const WiFiEventSoftAPModeStationDisconnected& event) {
  //events.send("{\"wifi\":{\"event\":\"ap_disconnect\"}}","wifi");
  
  char p[128];
  const uint8 * m = event.mac;
  sprintf(p, "{\"wifi\":{\"event\":\"client_disconnect\",\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"aid\":\"%x\"}}", m[0], m[1], m[2], m[3], m[4], m[5], event.aid);
  events.send(p, "wifi");
  
}

void wifi_connected(const WiFiEventStationModeConnected& event) {
  // events.send("{\"wifi\":{\"event\":\"ap_connected\"}}","wifi");
  char p[192];
  char json_name[128];
  string_to_json(json_name, 128, event.ssid ); 
  const uint8 * m = event.bssid;
  sprintf(p, "{\"wifi\":{\"event\":\"ap_connect\",\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"ssid\":\"%s\",\"channel\":%d}}", 
    m[0], m[1], m[2], m[3], m[4], m[5], 
    json_name,
    event.channel
  );
  events.send(p, "wifi");
}

void wifi_gotip(const WiFiEventStationModeGotIP& event) {
  //events.send("{\"wifi\":{\"event\":\"ap_disconnect\"}}","wifi");
  char p[128];
  IPAddress ip = event.ip;
  IPAddress mk = event.mask;
  IPAddress gw = event.gw;
  sprintf(p, "{\"wifi\":{\"event\":\"ap_gotip\",\"ip\":\"%d.%d.%d.%d\",\"mask\":\"%d.%d.%d.%d\",\"gw\":\"%d.%d.%d.%d\"}}", 
    ip[0], ip[1], ip[2], ip[3], 
    mk[0], mk[1], mk[2], mk[3], 
    gw[0], gw[1], gw[2], gw[3]
  );
  events.send(p, "wifi");
}

void wifi_disconnected(const WiFiEventStationModeDisconnected& event) {
  //events.send("{\"wifi\":{\"event\":\"ap_disconnect\"}}","wifi");
  char p[192];
  char json_name[128];
  string_to_json(json_name, 128, event.ssid ); 
  const uint8 * m = event.bssid;
  sprintf(p, "{\"wifi\":{\"event\":\"ap_disconnect\",\"bssid\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"ssid\":\"%s\",\"reason\":%d}}", 
    m[0], m[1], m[2], m[3], m[4], m[5], 
    json_name,
    event.reason
  );
  events.send(p, "wifi");
}

void wifi_reconnect() {
    wifi_state = WIFI_STATE_START;  
    // which kind of connection?
    if(apconnect_has_bssid) {
      // name + bssid connection
      WiFi.begin( apconnect_ssid.c_str(), apconnect_password.c_str(), 0, apconnect_bssid );
    } else {
      // name-only connection
      WiFi.begin( apconnect_ssid.c_str(), apconnect_password.c_str() );
    }
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      wifi_state = WIFI_STATE_CONNECTED;
      // supergreen
      // leds[0] = CRGB::Green; FastLED.show();
      fx_pose("online");
      // optionally enable over-the-air programming updates, if allowed
      if(apconnect_has_ota) { 
        ota_state_change = OTA_STATE_START;
      } else {
        ota_state_change = OTA_STATE_STOP;
      }
    } else {
      events.send("{\"wifi\":{\"error\":\"connect failed\"}}" );
      wifi_state = WIFI_STATE_OFF;
      // fail
      // leds[0] = CRGB::Purple; FastLED.show();
      fx_pose("offline");
      // add the bssid to the skip list for a while
      skip_list_add(apconnect_bssid);
    }
}

// wifi setup

static WiFiEventHandler ap_connect_handler;
static WiFiEventHandler ap_disconnect_handler;
static WiFiEventHandler sta_connect_handler;
static WiFiEventHandler sta_gotip_handler;
static WiFiEventHandler sta_disconnect_handler;

void wifi_setup(){
  // attach event handlers
  //WiFi.onEvent(WiFiEvent,WIFI_EVENT_ANY); 
  ap_connect_handler = WiFi.onSoftAPModeStationConnected(station_connected);
  ap_disconnect_handler = WiFi.onSoftAPModeStationDisconnected(station_disconnected);

  sta_connect_handler = WiFi.onStationModeConnected(wifi_connected);
  sta_gotip_handler = WiFi.onStationModeGotIP(wifi_gotip);
  sta_disconnect_handler = WiFi.onStationModeDisconnected(wifi_disconnected);

  // set hostname
  WiFi.hostname(local_hostname);

  // start local access point
  WiFi.setOutputPower(ap_txpower);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid.c_str(),ap_password.c_str());

  // do we have a hardcoded access point?
  if(apconnect_ssid == "") {
    // no default. start searching.
    wifi_state_change = WIFI_STATE_SEARCH;
  } else {
    // connect to the hardcoded AP
    wifi_state = WIFI_STATE_CONNECTED;
    WiFi.begin(apconnect_ssid.c_str(), apconnect_password.c_str());  
    // wait for a result
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("Failed to connect to access point \"%s\".\n",apconnect_ssid.c_str());
      WiFi.disconnect(false);
      delay(1000);
      // begin a general search
      wifi_state_change = WIFI_STATE_SEARCH;
    } else {
      // we're connected. 
      wifi_state = WIFI_STATE_CONNECTED;
      // also enable the OTA system?
      if(apconnect_has_ota) { ota_state_change = OTA_STATE_START; }
    }
  }
}

int ota_last_progress = -1;

void ota_setup(){
  // echo OTA events to the websocket clients
  ArduinoOTA.onStart([]() { 
    // fx_pose("ota");
    events.send("{\"ota\":{\"state\":\"start\"}}", "ota"); 
  });
  ArduinoOTA.onEnd([]() { 
    // fx_pose("shutdown");
    events.send("{\"ota\":{\"state\":\"end\"}}", "ota"); 
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pc = (float)progress / (float)total * 100.0;
    if(ota_last_progress != pc) {
      ota_last_progress = pc;
      char p[64];
      // sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
      sprintf(p, "{\"ota\":{\"progress\":%u}}", pc);
      events.send(p, "ota");
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) { events.send("{\"ota\":{\"error\":\"Auth Failed\"}}", "ota"); }
    else if(error == OTA_BEGIN_ERROR) { events.send("{\"ota\":{\"error\":\"Begin Failed\"}}", "ota"); }
    else if(error == OTA_CONNECT_ERROR) { events.send("{\"ota\":{\"error\":\"Connect Failed\"}}", "ota"); }
    else if(error == OTA_RECEIVE_ERROR) { events.send("{\"ota\":{\"error\":\"Recieve Failed\"}}", "ota"); }
    else if(error == OTA_END_ERROR) { events.send("{\"ota\":{\"error\":\"End Failed\"}}", "ota"); }
  });
  //ArduinoOTA.setHostname(local_hostname.c_str());
  //ArduinoOTA.begin();
}

/*
DNSServer dnsServer;

void setupDNS() {  
  // Setup the DNS server redirecting all the domains to the apIP 
  IPAddress ip = WiFi.softAPIP();
    
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", ip);
}
*/

void wifi_loop(){
  int r;
  // Handle OTA events first, just in case
  if(ota_state == OTA_STATE_ACTIVE) ArduinoOTA.handle();
  
  // manage the OTA system
  switch(ota_state_change) {
    case OTA_STATE_START:
      switch(ota_state) {
        case OTA_STATE_OFF:
          // start it up
          ArduinoOTA.setHostname(local_hostname.c_str());
          ArduinoOTA.begin();
          // fall through
        case OTA_STATE_STOP:
          // we're now active
          ota_state = OTA_STATE_ACTIVE;
      }
      break;
    case OTA_STATE_STOP:
      switch(ota_state) {
        case OTA_STATE_ACTIVE:
          // disable for now
          ota_state = OTA_STATE_STOP;
      }
      break;
  }
  // processed
  ota_state_change = OTA_STATE_OFF;
  // manage the wifi system
  switch(wifi_state_change) {
    case WIFI_STATE_RECONNECT:
      switch(wifi_state) {
        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_IDLE:
          // I'm not blue
          // leds[0] = CRGB::Orange; FastLED.show();
          fx_pose("offline");
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
          // leds[0] = CRGB::Orange; FastLED.show();
          fx_pose("offline");
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
      fx_pose("offline");
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
      // 
      scan_start_event();
      WiFi.scanNetworks(true, true);
      scan_state = SCAN_STATE_ACTIVE;
      break;
    case SCAN_STATE_ACTIVE: // async scanning, check for results
      r = WiFi.scanComplete();
      switch(r) {
        case WIFI_SCAN_RUNNING: 
          break;
        case WIFI_SCAN_FAILED: 
          // scan error?
          scan_state = SCAN_STATE_STOP;
          // leds[0] = CRGB::Red; FastLED.show();
          break;
        default:
          // scan complete, count returned
          scan_count = r;
          // leds[0] = CRGB::Green; FastLED.show();
          // scan is complete
          async_wifi_scan_complete();
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
        scan_report_complete();
        scan_state = SCAN_STATE_IDLE;
      }
      break;
    case SCAN_STATE_IDLE: // scan complete, waiting for timer
      break;
    case SCAN_STATE_STOP: // scan shutdown
      scan_state = SCAN_STATE_OFF;
  }
  
}

