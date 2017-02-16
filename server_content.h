#include <ArduinoJson.h>
/*
void jsr_tabs(AsyncResponseStream *response, int tab_count) {
  for(var i=0; i<tab_count; i++) response->print("\t");
}

void jsr_property_str(AsyncResponseStream *response, char[] pname, char[] pvalue) {
  response->printf("\"%s\":\"%s\"",pname,pvalue);
}
*/

void server_setup() {
  // serve the SPIFFSEditor
  server.addHandler(new SPIFFSEditor( htaccess_username, htaccess_password ));

  // serve static pages from www root
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.htm");
 
  server.on("/adc", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String( analogRead(A0) ));
  });

  server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "ok");
  });
  
  server.on("/system/time", HTTP_GET, [](AsyncWebServerRequest *request){
    time_t now = millis() / 1000; // time( NULL );
    request->send(200, "text/plain", ctime(&now));
    // Serial.println(ctime(&now));
  });
  
  server.on("/servo/start", HTTP_GET, [](AsyncWebServerRequest *request){
    // make sure the servo system is running
    servos_state_change = SERVO_STATE_AUTO;
    request->send(200, "text/plain", "ok" );
  });
  
  server.on("/servo/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    // make sure the servo system is running
    servos_state_change = SERVO_STATE_OFF;
    request->send(200, "text/plain", "ok" );
  });
  
  server.on("/servo/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // response->addHeader("Server","ESP Async Web Server");
    // wifi scan report
    response->print("{\"servo\":{\"status\":{\n");
    response->printf("\t\"state\":%d,\n",servos_state);
    // servo state list
    response->print("\t\"servos\":[\n");
    for(int i=0; i<SERVO_COUNT; i++) {
      response->print("\t\t{");
      response->printf("\"state\":%d,",servo_state[i]);
      response->printf("\"position\":%d,",servo_current[i]);
      response->printf("\"target\":%d,",servo_target[i]);
      response->printf("\"speed\":%d,",servo_speed[i]);
      response->printf("\"timeout\":%d",servo_timeout[i]);
      if((i+1) < SERVO_COUNT) {
        response->print("},\n");
      } else {
        response->print("}\n");
      }
    }
    response->print("\t]\n");
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
  });

  /*
  server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    switch(scan_state) {
      case SCAN_STATE_OFF:
        // schedule the network scan
        scan_state_change = SCAN_STATE_START;
        //if(servos_state==0) servos_state = 1;
        request->send(200, "text/plain", "START" );
        break;
      case SCAN_STATE_START:
        request->send(200, "text/plain", "STARTING" );
        break;
      case SCAN_STATE_ACTIVE:
      case SCAN_STATE_IDLE:
        int n = scan_count;
        // compile the response
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        // response->addHeader("Server","ESP Async Web Server");
        // wifi scan report
        response->print("{\"wifi\":{\"scan\":{\n");
        // count of list entries
        response->print("\"count\":");
        response->print(n);
        response->print("\n");
        // scan list
        response->print("\t\"list\":[\n");
        for (int i = 0; i < n; ++i) {
          // scan list entry object
          response->print("\t\t{ ");
          // response->print("\"index\":");
          // response->print(i);
          response->print("\"ssid\":\"");
          response->print(scan_ssid[i]);
          response->print("\", \"bssid\":\"");
          response->print(scan_bssid[i]);
          response->print("\", \"encrypt\":\"");
          switch(WiFi.encryptionType(i)) {
            case ENC_TYPE_NONE:  response->print("none"); break;
            case ENC_TYPE_WEP:   response->print("wep"); break;
            case ENC_TYPE_TKIP:  response->print("tkip"); break;
            case ENC_TYPE_CCMP:  response->print("ccmp"); break;
            case ENC_TYPE_AUTO:  response->print("auto"); break;
          }
          response->print("\", \"channel\":");
          response->print(scan_channel[i]);
          response->print(", \"rssi\":");
          response->print(scan_rssi[i]);
          // response->print(" }\n");
          if((i+1) < n) {
            response->print(" },\n");
          } else {
            response->print(" }\n");
          }
        }
        // end list
        response->print("\t]\n");
        // end block with an OK code
        response->print("}},\"ok\":true}");
        // send the response
        request->send(response);
        break;
    }
  });
  */

  server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // response->addHeader("Server","ESP Async Web Server");
    // wifi scan report
    response->print("{\"wifi\":{\"status\":{\n");
    response->printf("\t\"host_name\":\"%s\",\n",local_hostname.c_str());
    // ap ip address
    IPAddress ip = WiFi.softAPIP();
    response->print("\t\"ap_ip\":\"");
    response->print(ip);
    response->print("\",\n");
    // mac addresses
    byte mac[6];
    WiFi.softAPmacAddress(mac);
    response->printf("\t\"ap_mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\n",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // client ip address
    ip = WiFi.localIP();
    response->print("\t\"local_ip\":\"");
    response->print(ip);
    response->print("\",\n");
    // and mac address
    WiFi.macAddress(mac);
    response->printf("\t\"local_mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\n",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // send a list of access points we are currently ignoring
    response->print("\t\"skip\":{");
    boolean first = true;
    for(int i=0; i<SKIP_LIST_COUNT; i++) {
      if(skip_counter[i]>0) {
        byte * mac = &skip_bssid[i*6];
        response->printf("%s\n\t\t\"%02X:%02X:%02X:%02X:%02X:%02X\":%d", first?"":",", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], skip_counter[i]);
        first = false;
      }
    }
    response->print("\n\t}\n");
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
  });

  /*
  server.on("/wifi/powersave", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    // response->addHeader("Server","ESP Async Web Server");
    // wifi scan report
    response->print("{\"wifi\":{\"powersave\":{\n");
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
    if(wifi_state == 2) {
      // switch to station-only mode
      WiFi.mode(WIFI_STA);
      // enable auto modem sleep?
      WiFi.setSleepMode(WIFI_MODEM_SLEEP);
      // we now have the ability to sleep
      wifi_state == 3;
    }
  });
  */
  
  server.on("/system/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"system\":{\"status\":{\n");
    response->print("\t\"host_name\":\"");
    response->print(local_hostname);
    response->print("\",\n");
    // device details
    response->print("\t\"processor\":\"esp8266\",\n");
    response->printf("\t\"chip_id\":\"%x\",\n",ESP.getChipId());
    response->printf("\t\"free_heap\":%u,\n",ESP.getFreeHeap());
    // flash device info
    response->print("\t\"flash\":{\n");
    response->printf("\t\t\"chip_id\":\"%x\",\n",ESP.getFlashChipId());
    response->printf("\t\t\"chip_size\":%u,\n",ESP.getFlashChipSize());
    response->printf("\t\t\"chip_speed\":%u\n",ESP.getFlashChipSpeed());
    response->print("\t},\n");
    // get the filesystem info
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    response->print("\t\"spiffs\":{\n");
    response->printf("\t\t\"total\":%d,\n",fs_info.totalBytes);
    response->printf("\t\t\"used\":%d,\n",fs_info.usedBytes);      
    response->printf("\t\t\"block_size\":%d,\n",fs_info.blockSize);
    response->printf("\t\t\"page_size\":%d,\n",fs_info.pageSize);      
    response->printf("\t\t\"max_open_files\":%d,\n",fs_info.maxOpenFiles);
    response->printf("\t\t\"max_path_length\":%d\n",fs_info.maxPathLength);      
    response->print("\t},\n");
    // servo info
    /*
    response->print("\t\"servo\":{\n");
    // response->printf("\t\t\"state\":\"%x\",\n",servo_state);
    response->printf("\t\t\"1\":%u,\n",servo_state[0]);
    response->printf("\t\t\"2\":%u\n",servo_state[1]);
    response->print("\t},\n");
    */
    // analog read values
    response->printf("\t\"adc\":%d,\n",analogRead(A0));
    response->printf("\t\"vcc\":%d\n",ESP.getVcc());
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
  });

  // battery 3.17v (probably 3v after regulator) = 2173
  // battery 4.2v (probably 3.3v fter regulator) = 3138

  // return all the relevant status for the UI in one go.
  server.on("/ui/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"ui\":{\"status\":{\n");
    // wifi stuff
    response->printf("\t\"wifi\":{\n",wifi_state);
    response->printf("\t\t\"state\":\"%s\",\n",wifi_state_name(wifi_state));
    response->printf("\t\t\"hostname\":\"%s\",\n",local_hostname.c_str());
    response->printf("\t\t\"connected_ssid\":\"%s\",\n",apconnect_ssid.c_str());
    // ap ip address
    IPAddress ip = WiFi.softAPIP();
    response->print("\t\t\"ap_ip\":\"");
    response->print(ip);
    response->print("\",\n");
    // mac addresses
    byte mac[6];
    WiFi.softAPmacAddress(mac);
    response->printf("\t\t\"ap_mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\n",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // client ip address
    ip = WiFi.localIP();
    response->print("\t\t\"local_ip\":\"");
    response->print(ip);
    response->print("\",\n");
    // and mac address
    WiFi.macAddress(mac);
    response->printf("\t\t\"local_mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\"\n",  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    response->print("\t},\n");
    // scan state
    response->print("\t\"scan\":{\n");
    response->printf("\t\t\"state\":\"%s\"\n", scan_state_name(scan_state) );
    response->print("\t},\n");
    // FX settings
    response->print("\t\"fx\":{\n");
    response->printf("\t\t\"servo_start\":\"%s\"\n", fx_servo_start.c_str() );
    response->print("\t},\n");
    // servo state list
    response->printf("\t\"servos_state\":\"%s\",\n", servo_state_string(servos_state).c_str() );
    response->print("\t\"servos\":[\n");
    for(int i=0; i<SERVO_COUNT; i++) {
      response->print("\t\t{");
      response->printf("\"state\":%d,",servo_state[i]);
      response->printf("\"position\":%d,",servo_current[i]);
      response->printf("\"target\":%d,",servo_target[i]);
      response->printf("\"speed\":%d,",servo_speed[i]);
      response->printf("\"timeout\":%d",servo_timeout[i]);
      if((i+1) < SERVO_COUNT) {
        response->print("},\n");
      } else {
        response->print("}\n");
      }
    }
    response->print("\t],\n");
    //
    response->print("\t\"log\":{\n");
    // response->printf("\t\t\"state\":%d,\n",logging_state);
    response->printf("\t\t\"state\":\"%s\",\n",logging_state_name(logging_state));
    response->printf("\t\t\"url\":\"/log.txt\"\n");
    response->print("\t},\n");
    // get the filesystem info
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    response->print("\t\"storage\":{\n");
    response->printf("\t\t\"total\":%d,\n",fs_info.totalBytes);
    response->printf("\t\t\"used\":%d,\n",fs_info.usedBytes);      
    response->printf("\t\t\"block_size\":%d,\n",fs_info.blockSize);
    response->printf("\t\t\"page_size\":%d,\n",fs_info.pageSize);      
    response->printf("\t\t\"max_open_files\":%d,\n",fs_info.maxOpenFiles);
    response->printf("\t\t\"max_path_length\":%d\n",fs_info.maxPathLength);      
    response->print("\t},\n");
    // vcc
    response->printf("\t\"vcc\":%d\n",ESP.getVcc());
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
  });

  server.on("/system/config", HTTP_GET, [](AsyncWebServerRequest *request){
    // compile the response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"system\":{\"config\":{\n");
    // host name
    response->print("\t\"host_name\":\"");
    response->print(local_hostname);
    response->print("\",\n");
    // wifi name
    response->print("\t\"wifi_name\":\"");
    response->print(ap_ssid);
    response->print("\",\n");
    // wifi password exists
    response->print("\t\"wifi_password\":");
    response->print(ap_password =="" ? "false" : "true");
    response->print(",\n");
    // wifi tx power
    response->print("\t\"wifi_txpower\":");
    response->print(ap_txpower);
    response->print(",\n");
    // htaccess name
    response->print("\t\"htaccess_username\":\"");
    response->print(htaccess_username);
    response->print("\",\n");
    // htaccess password exists
    response->print("\t\"htaccess_password\":");
    response->print(htaccess_password == "" ? "false" : "true");
    response->print(",\n");
    // servo config list
    response->print("\t\"servo\":{\n");
    for(int i=0; i<SERVO_COUNT; i++) {
      response->printf("\t\t\"%d\":{",i+1);
      response->printf("\"pin\":%d,",servo_config[i].pin);
      response->printf("\"min_micros\":%d,",servo_config[i].min_micros);
      response->printf("\"max_micros\":%d,",servo_config[i].max_micros);
      response->printf("\"zero_micros\":%d,",servo_config[i].zero_micros);
      response->print("\"unit_scaling\":");
      response->print(servo_config[i].unit_scaling);
      if((i+1) < SERVO_COUNT) {
        response->print("},\n");
      } else {
        response->print("}\n");
      }
    }
    response->print("\t}\n");
    // end block with an OK code
    response->print("}},\"ok\":true}");
    // send the response
    request->send(response);
  });

  

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
}
