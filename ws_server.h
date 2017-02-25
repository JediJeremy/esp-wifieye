
// HTTP Websockets server
AsyncWebSocket ws("/ws");

// {"servo":{"1":{ "pos":120, "speed":20 }}}
// {"servo":{"2":{ "pos":120, "speed":20 }}}
void servo_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
   // loop through servo ids
   char id[2] = "0";
   for(int i=0; i<SERVO_COUNT; i++) {
     id[0] = '1' + i;
     if(message[id].is<JsonObject&>()) {
       JsonObject& servo_update = message[id];
       float pos = servo_update["pos"];
       float move_v = servo_update["speed"];
       float spd = dps_servo_speed(move_v); // convert from per-second to per-loop
       // move the servo
       servo_move(i,pos,spd);
       // client->printf("{\"servo\":\"moving\",\"ok\":true}");
       // make sure servo system is running (or start it up)
       servos_state_change = SERVO_STATE_ON;
     }
  }
  // specific mode change?
  if( message["mode"].is<const char *>() ) {
    servos_state_change = string_servo_state( message["mode"] );
  }
  /*
  String m_mode = message["mode"];
  if(m_mode == "on") {
      servos_state_change = SERVO_STATE_ON;
   } else if(m_mode == "off") {
      servos_state_change = SERVO_STATE_OFF;
   } else if(m_mode == "auto") {
      servos_state_change = SERVO_STATE_AUTO;
  } */
}

void scan_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
  // mode change?
  String m_mode = message["mode"];
  if(m_mode == "start") {
      scan_state_change = SCAN_STATE_START;    
  } else if(m_mode == "stop") {
      scan_state_change = SCAN_STATE_STOP;
  }
}

void log_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
  // mode change?
  String m_mode = message["mode"];
  if(m_mode == "start") {
      logging_state_change = LOGGING_STATE_START;    
  } else if(m_mode == "stop") {
      logging_state_change = LOGGING_STATE_STOP;
  }
  // erase file?
  String m_clear = message["clear"];
  if(m_clear == "all") {
      logging_clear_all();    
  } 
}

String wifi_message_password(JsonObject& message, String ssid, String bssid, boolean has_bssid) {
  // were we given a password, or are we expected to search for it?    
  if(message.containsKey("password")) {
    // specifically provided
    return message["password"].asString();
  }
  // default 
  return wifi_lookup_password(ssid, bssid, has_bssid);
}

void wifi_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
  String m_mode = message["mode"];
  if(m_mode == "connect") {
    String m_ssid = message["ssid"];
    String m_bssid = message["bssid"];
    uint8_t bssid[6];
    boolean has_bssid = false;
    if(message.containsKey("bssid")) {
      // try to parse that into mac address
      has_bssid = parse_bssid(m_bssid,bssid);
    }
    // were we given a password, or are we expected to search for it?    
    String password = wifi_message_password(message, m_ssid, m_bssid, has_bssid);
    // good so far. store all the connection settings 
    apconnect_ssid = m_ssid;
    apconnect_password = password;
    apconnect_has_bssid = has_bssid;
    memcpy(apconnect_bssid,bssid,6);
    // and initiate a reconnect in the main loop
    wifi_state_change = WIFI_STATE_RECONNECT;
    //
  } else if(m_mode == "save") {
    String m_ssid = message["ssid"];
    String m_bssid = message["bssid"];
    String m_fx = message["fx"];
    String m_color = message["color"];
    int    m_priority = message["priority"];
    boolean m_auto = message["auto"];
    boolean m_ota = message["ota"];
    
    // try to parse bssid into mac address    
    uint8_t bssid[6];
    boolean has_bssid = false;
    if(message.containsKey("bssid")) {
      has_bssid = parse_bssid(m_bssid,bssid);
    }
    // were we given a password, or are we expected to search for it?    
    String password = wifi_message_password(message, m_ssid, m_bssid, has_bssid);
    // good so far
    if(has_bssid) {
      char p[256];
      sprintf(p,"{\n\t\"ssid\":\"%s\",\n\t\"password\":\"%s\",\n\t\"priority\":%d,\n\t\"fx\":\"%s\",\n\t\"color\":\"%s\",\n\t\"auto\":%s,\n\t\"ota\":%s}", 
        m_ssid.c_str(),
        password.c_str(),
        m_priority,
        m_fx.c_str(), 
        m_color.c_str(), 
        m_auto ? "true" : "false", 
        m_ota ? "true" : "false"
      );
      // name + bssid connection
      // client->printf("{\"wifi\":{\"event\":\"save\",\"mac\":\"%s\",\"password\":\"%s\"}}", m_bssid.c_str(), password.c_str() );
      events.send(p);
      String filename = String("/wifi/mac/")+m_bssid;
      File file = SPIFFS.open( filename, "w");
      file.print(p);
      file.close();
    } else {
      // name-only connections are not currently stored - longer names exceed filesystem limits.
      // client->printf("{\"wifi\":{\"event\":\"save\",\"id\":\"%s\",\"password\":\"%s\"}}", m_ssid.c_str(), password.c_str() );
    }
  } else if(m_mode == "disconnect") {
    // and initiate a reconnect in the main loop
    wifi_state_change = WIFI_STATE_STOP;
  } else if(m_mode == "reconnect") {
    // and initiate a reconnect in the main loop
    wifi_state_change = WIFI_STATE_SEARCH;
  } else if(m_mode == "test") {
    events.send("{\"wifi\":{\"event\":\"test\"}}","wifi");
  } else if(m_mode == "hostname") {
    // reply with the contents
    client->printf("{\"local\":{\"hostname\":\"%s\"}}", local_hostname.c_str());
  } else if(m_mode == "htaccess") {
    client->printf("{\"htaccess\":{{\"username\":\"%s\", \"password\":\"%s\"}}", htaccess_username.c_str(), htaccess_password.c_str() );
  } else if(m_mode == "ap") {
    // read the config file
    client->printf("{\"ap\":{{\"ssid\":\"%s\", \"password\":\"%s\"}}", ap_ssid.c_str(), ap_password.c_str() );
  }
}


void config_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
  // access point update
  boolean do_hostname = false;
  boolean do_ap = false;
  boolean do_htaccess = false;
  boolean do_fx = false;
  // hostname config?
  if(message["hostname"].is<const char*>()) {
    local_hostname = message["hostname"].asString();
    do_hostname = true;
  }
  // access point config?
  if(message["ap"].is<JsonObject&>()) {
    JsonObject& ap = message["ap"];
    if(ap["ssid"].is<const char*>()) {
      ap_ssid = ap["ssid"].asString();
      do_ap = true;
    }
    if(ap["password"].is<const char*>()) {
      ap_password = ap["password"].asString();
      do_ap = true;
    }
    if(ap["txpower"].is<double>() || ap["txpower"].is<int>()) {
      ap_txpower = (double)ap["txpower"];
      WiFi.setOutputPower(ap_txpower);
      do_ap = true;
    }
  }
  // htaccess config?
  if(message["htaccess"].is<JsonObject&>()) {
    JsonObject& htaccess = message["htaccess"];
    if(htaccess["username"].is<const char*>()) {
      htaccess_username = htaccess["username"].asString();
      do_htaccess = true;
    }
    if(htaccess["password"].is<const char*>()) {
      htaccess_password = htaccess["password"].asString();
      do_htaccess = true;
    }
  }
  // fx config?
  if(message["fx"].is<JsonObject&>()) {
    JsonObject& fx = message["fx"];
    if(fx["servo_start"].is<const char*>()) {
      fx_servo_start = fx["servo_start"].asString();
      do_fx = true;
    }
    
  }
  // save changed config files
  if(do_hostname) save_hostname(local_hostname);
  if(do_ap) save_ap_config(ap_ssid,ap_password,ap_txpower);
  if(do_fx) save_fx_config();
  if(do_htaccess) save_htaccess_config(htaccess_username, htaccess_password);
  // servo config?
  if(message["servo"].is<JsonObject&>()) {
    JsonObject& servo = message["servo"];
    // check each servo index
    for(int index = 0; index<SERVO_COUNT; index++) {
      String n = String(index+1);
      if(servo[n].is<JsonObject&>()) {
        JsonObject& servon = servo[n];
        servo_config[index].pin = servon["pin"];
        servo_config[index].min_micros = servon["min_micros"];
        servo_config[index].max_micros = servon["max_micros"];
        servo_config[index].zero_micros = servon["zero_micros"];
        servo_config[index].unit_scaling = servon["unit_scaling"];
        save_servo_config(index+1, servo_config[index]);
      }
    }
  }
}

void fx_message(AsyncWebSocket * server, AsyncWebSocketClient * client, JsonObject& message) {
  if(message["pose"].is<const char *>()) {
    fx_pose(message["pose"],true);
    servos_state_change = SERVO_STATE_ON;
  }
}

void accept_json_message(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  // try to parse the data
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((char *)data, len);
  // Test if parse succeeded
  if (root.success()) {
    // check for instruction types
    if(root["servo"].is<JsonObject&>()) {
      // servo update
      servo_message(server, client, root["servo"] );
    } else if(root["fx"].is<JsonObject&>()) {
      // fx update
      fx_message(server, client, root["fx"] );
    } else if(root["wifi"].is<JsonObject&>()) {
      // wifi update
      wifi_message(server, client, root["wifi"] );
    } else if(root["scan"].is<JsonObject&>()) {
      // scan control
      scan_message(server, client, root["scan"] );
    } else if(root["log"].is<JsonObject&>()) {
      // log control
      log_message(server, client, root["log"] );
    } else if(root["config"].is<JsonObject&>()) {
      // config control
      config_message(server, client, root["config"] );
    }
  } else {
     // end block with an error code
    client->text("{\"error\":\"json parse failed\"}");
  }
  
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    //Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);
      // check if the first character looks like a JSON object delimeter
      if( (len > 0) && (data[0]=='{') ) {
        // parse the message json
        accept_json_message(server, client, type, arg, data, len);
      } else {
        // send back an error code
        client->text("{\"error\":\"expected JSON object\"}");
      }
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0){
          //Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        }
        //Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }
      //Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);
      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      //Serial.printf("%s\n",msg.c_str());
      if((info->index + len) == info->len){
        //Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          //Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}


void websockets_setup() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

}

