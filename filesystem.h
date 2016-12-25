
void string_to_csv(File stream, String s) {
  const char * p = s.c_str(); // get the character pointer
  // go through the characters
  char ch;
  while(ch = *p) {
    if(ch == '"') {
      stream.print("\"\""); // double the quotes
    } else {
      stream.print(ch); // no escape
    }
    p++;
  }
}

void string_to_json(char * buffer, int length, String s) {
  int i=0;
  int r=length-1;
  const char * p = s.c_str(); // get the character pointer
  // go through the characters
  char ch; 
  char cn = 0;
  bool more = (ch = *p) != 0;
  while(more) {
    if(ch < 0x20) {
      // a select few get named escapes
      if(ch == 0x08) { cn = 'b'; }
      else if(ch == 0x09) { cn = 't'; }
      else if(ch == 0x0A) { cn = 'n'; }
      else if(ch == 0x0C) { cn = 'f'; }
      else if(ch == 0x0D) { cn = 'r'; }
      // everything else gets an escape code
      else { cn = 'u'; }
    } else if(ch < 0x60) {
      // a select few get named escapes
      if(ch == 0x22) { cn = '"'; }
      else if(ch == 0x2F) { cn = '/'; }
      else if(ch == 0x5c) { cn = '\\'; }
      // and everything else is left alone
      else { cn = 0; }
    } else {
      // direct code
      cn = 0;
    }
    // what kind of code?
    if(cn == 0) {
      // direct character code
      r -= 1;
      if(r>=0) {
        buffer[i++] = ch;
      } else {
        more = false;
      }
    } else if(cn == 'u') {
      // unicode translation
      r -= 6;
      if(r>=0) {
        buffer[i++] = '\\';
        buffer[i++] = 'u';
        buffer[i++] = '0' + ( (ch >> 12) & 0x0f );
        buffer[i++] = '0' + ( (ch >> 8)  & 0x0f );
        buffer[i++] = '0' + ( (ch >> 4)  & 0x0f );
        buffer[i++] = '0' + ( (ch >> 0)  & 0x0f );
      } else {
        more = false;
      }
    } else {
      // named character
      r -= 2;
      if(r>=0) {
        buffer[i++] = '\\';
        buffer[i++] = cn;
      } else {
        more = false;
      }
    }
    if(more) {
      p++;
      more = (ch = *p) != 0;
    }
  }
  // terminate the string
  buffer[i] = 0;
}

boolean parse_hex_char(char ch) {
  if( (ch>='0') && (ch<='9') ) return ch - '0';
  if( (ch>='A') && (ch<='F') ) return ch - 'A' + 10;
  // if( (ch>='a') && (ch<='f') ) return ch - 'a' + 10;
  return -1;
}

boolean parse_bssid_hex(String id,  uint8_t * bssid, int p, int i ) {
  int c1 = parse_hex_char(id[p++]);
  int c2 = parse_hex_char(id[p]);
  if(c1<0) return false;
  if(c2<0) return false;
  bssid[i] = (c1 << 4) + c2;
  return true;
}

boolean parse_bssid(String id, uint8_t * bssid) {
  // is it long enough?
  if(id.length() == 17) {
    // check all the separators are there
    if(id[2] != ':') return false;
    if(id[5] != ':') return false;
    if(id[8] != ':') return false;
    if(id[11] != ':') return false;
    if(id[14] != ':') return false;
    // parse each hex group
    int p=0;
    for(int i=0; i<6; i++) {
      if( !parse_bssid_hex(id,  bssid, p, i ) )return false;
      p+=3;
    }
  }
  // we made it
  return true;
}


//
void load_file_string(const char * file_name, char * buffer, int length, const char * lacuna) {
  // open the file
  File file = SPIFFS.open(file_name, "r");
  if(!file) {
    // use the lacuna value, and include the terminator
    int r = strlen(lacuna)+1;
    if(r >= length) { r = length-1; }
    memcpy(buffer,lacuna,r);
  } else {
    int q = file.available();
    if( q>=length ) { q = length - 1; }
    int r = file.readBytesUntil('\n',buffer, q );
    if(r<0) { r = 0; }
    else if(r>=length) { r = length-1; }
    buffer[r] = '\0'; // make sure the string is terminated
    file.close();
  }
}

// 
void load_hostname() {
  char buffer[64];
  load_file_string("/etc/hostname", buffer, 64, "esp");
  String s = String(buffer);
  local_hostname = s;
}

// 
void save_hostname(String hostname) {
  // open the file
  File file = SPIFFS.open("/etc/hostname", "w");
  if(!file) {
    // could not open
    events.send("{\"error\":\"could not write\", \"file\":\"/etc/hostname\"}");
  } else {
    file.print(hostname);
    file.close();  
  }
}


void load_json_then(String filename, int limit, std::function<void(JsonObject&)> succeed, std::function<void()> fail) {
  // open the file
  File file = SPIFFS.open(filename, "r");
  if(!file) {
    // could not open
    char m[128];
    sprintf(m,"{\"error\":\"JSON file missing\", \"file\":\"%s\"}",filename.c_str());
    events.send(m);
  } else {
    int q = file.available();
    if( q>=limit ) {
      // too big to load
      char m[128];
      sprintf(m,"{\"error\":\"JSON file too large\", \"file\":\"%s\", \"limit\":%d}",filename.c_str(),limit);
      events.send(m);
    } else {
      // create a buffer and load the file data
      char data[q+1];
      int r = file.readBytes(data, q);
      if(r<0) { r = 0; }
      else if(r>=limit) { r = limit-1; }
      data[r] = '\0'; // make sure the string is terminated
      file.close();
      // now parse the contents as json
      StaticJsonBuffer<128> json;
      JsonObject& root = json.parse(data);
      if (root.success()) {
        return succeed(root);
      } else {
        char m[128];
        sprintf(m,"{\"error\":\"JSON parseObject() failed\", \"file\":\"%s\"}",filename.c_str());
        events.send(m);
      }
    }
  }
  // didn't work
  return fail();
}


//
void load_htaccess_config() {
  // open the file
  load_json_then("/etc/htaccess", 1024, [](JsonObject& root){
    // success. look for the ssid and password fields
    String json_user = root["username"];
    if( json_user ) htaccess_username = json_user;      
    String json_password = root["password"];
    if( json_password ) htaccess_password = json_password;      
  }, [](){
    // failure.
    //events.send("{\"error\":\"could not load config /etc/htaccess \"}");
  });
}

//
void save_htaccess_config(String username, String password) {
  // open the file
  File file = SPIFFS.open("/etc/htaccess", "w");
  if(!file) {
    // could not open
    events.send("{\"error\":\"could not write\", \"file\":\"/etc/htaccess\"}");
  } else {
    StaticJsonBuffer<200> json;
    JsonObject& root = json.createObject();
    root["username"] = username;
    root["password"] = password;    
    root.printTo(file);
    file.close();
    events.send("{\"message\":\"saved /etc/htaccess\"}");  
  }
}
//
void load_ap_config() {
  // open the file
  load_json_then("/etc/ap", 1024, [](JsonObject& root){
    // success. look for the ssid and password fields
    String json_ssid = root["ssid"];
    if( json_ssid ) ap_ssid = json_ssid;      
    String json_password = root["password"];
    if( json_password ) ap_password = json_password;
  }, [](){
    // failure.
    //events.send("{\"error\":\"could not load config /etc/ap \"}");
  });
}

//
void save_ap_config(String ssid, String password) {
  // open the file
  File file = SPIFFS.open("/etc/ap", "w");
  if(!file) {
    // could not open
    events.send("{\"error\":\"could not write\", \"file\":\"/etc/ap\"}");
  } else {
    StaticJsonBuffer<200> json;
    JsonObject& root = json.createObject();
    root["ssid"] = ssid;
    root["password"] = password;
    root.printTo(file);
    file.close();
    events.send("{\"message\":\"saved /etc/ap\"}");  
  }
}


//
String load_wifi_password(String filename) {
  String password = String("");
  // open the file
  load_json_then(filename.c_str(), 1024, [&password](JsonObject& root){
    // success. look for the password field
    if(root.containsKey("password")) {
      // concatenate our value, since json storage will dissapear as soon as we leave scope
      password.concat( root["password"].asString() );
    }
  }, [](){
    // failure.
    //events.send("{\"error\":\"could not load config /etc/ap \"}");
  });
  return password;
}


String wifi_lookup_password(String ssid, String bssid, boolean has_bssid) {
  if(has_bssid) {
    // try to look up the bssid
    String filename = String("/wifi/mac/")+bssid;
    const char * fn = filename.c_str();
    if(SPIFFS.exists(fn)) {
      return load_wifi_password( fn );
    }      
  }
  if(ssid) {
    // try to look up the ssid
    String filename = String("/wifi/ap/")+ssid;
    const char * fn = filename.c_str();
    if(SPIFFS.exists(fn)) {
      return load_wifi_password( fn );
    }            
  }
  // default no password
  return String("");
}
