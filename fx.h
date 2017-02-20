// fx system config
String fx_servo_start = String("off");
String fx_color_start = String("#0000FF");
String fx_color_connect = String("#FFFF00");
String fx_color_active = String("#00FF00");
String fx_color_idle = String("#000000");

// fastled library includes
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

// neopixel setup

// How many leds are in the strip?
#define LED_COUNT 1

// Data pin that led data will be written out over
#define DATA_PIN 2
//#define DATA_PIN 16

// global array for the current LED state
CRGB    leds[LED_COUNT];

// several systems need to animate RGB color vectors
struct RGBPose {
  boolean animate;
  CRGB    origin;
  CRGB    target;
  CRGB    color;
  int     rate;
  int     time;  
};

// the pose system has a target color and animation time information for each led
RGBPose color_pose[LED_COUNT];
// and also for the noise parameters
RGBPose noise_pose[LED_COUNT];

/*
// the pose system has a target color and animation time information for each led
boolean led_pose_animate[LED_COUNT];
CRGB    led_pose_begin[LED_COUNT];
CRGB    led_pose_target[LED_COUNT];
CRGB    led_pose_color[LED_COUNT];
int     led_pose_rate[LED_COUNT];
int     led_pose_time[LED_COUNT];
*/

// How many leds are in the strip?
#define SERVO_COUNT 2

// the pose system has a target and 'twitch' timer
int     servo_twitch_rate = 0;
int     servo_twitch_counter = 0;
int     servo_pose_target[SERVO_COUNT];
boolean servo_pose_twitch[SERVO_COUNT];
int     servo_pose_tremble[SERVO_COUNT];
int     servo_pose_counter[SERVO_COUNT];
int     servo_pose_speed[SERVO_COUNT];

// the total array brightness is also allowed to pulse.
unsigned int pulse_index = 0;
unsigned int pulse_limit = 100 * 256;
int          pulse_velocity = 0;
bool         pulse_loop = false;
float        pulse_gamma = 1.4;
int          pulse_min = 64;
int          pulse_max = 255;

int pulse_brightness() {
  // are we pulsing the brightness?
  if( pulse_velocity != 0 ) {
    // update the pulse time
    pulse_index += pulse_velocity;
    if(pulse_index >= pulse_limit) {
      if(pulse_loop) {
        // keep looping
        pulse_index -= pulse_limit;
      } else {
        // stop looping
        pulse_index = 0;
        pulse_velocity = 0;
      }
    }
    // compute the pulse time index
    float pulse_time = (float)pulse_index / (float)pulse_limit;
    // turn that into a 'distance from zero' ramp
    float pulse_ramp = abs( (pulse_time - 0.5) * 2 );
    // apply the pulse gamma curve to the linear ramp, and interpolate the range
    int pulse_bright = pulse_ramp * 255; // mix_f( pow(pulse_ramp, pulse_gamma), 0, 1, pulse_min, pulse_max);
    // use that to set the brightness
    return  pulse_bright;
  }
  // no pulsing
  return 255;
}


// servo pheripherals
#include <Servo.h>
#define SERVO_COUNT 2
#define SERVO_POWER_TIMEOUT 200

struct ServoConfig {
  int pin;
  int min_micros;
  int max_micros;
  int zero_micros;
  float unit_scaling;
};

/* not yet
struct ServoState {
  int state;
  int current;
  int target;
  int speed;
  int timeout;
};
*/

Servo servo[SERVO_COUNT];
ServoConfig servo_config[SERVO_COUNT];

int servo_pin[SERVO_COUNT] = { 14,12 };
int servo_state[SERVO_COUNT] = { 0,0 };
int servo_current[SERVO_COUNT] = { 1000,1000 };
int servo_target[SERVO_COUNT] = { 1000,1000 };
int servo_speed[SERVO_COUNT] = { 4,4 };
int servo_timeout[SERVO_COUNT] = { 0,0 };

int servos_state = 0;
int servos_state_change = 0;

#define SERVO_STATE_OFF      0
#define SERVO_STATE_ON       1
#define SERVO_STATE_AUTO     2

String servo_state_string(int state) {
  switch(state) {
    case SERVO_STATE_OFF : return "off";
    case SERVO_STATE_ON  : return "on";
    case SERVO_STATE_AUTO: return "auto"; 
  }
  return "off";
}

int string_servo_state(String state) {
  if(state == "off")  return SERVO_STATE_OFF;
  if(state == "on")   return SERVO_STATE_ON;
  if(state == "auto") return SERVO_STATE_AUTO;
  return SERVO_STATE_OFF;
}

// utilities
int clamp_i(int v, int a, int b) {
  if(a<b) {
    if(v<a) return a;
    if(v>b) return b;
  } else {
    if(v>a) return a;
    if(v<b) return b;    
  }
  return v;
}

float clamp_f(float v, float a, float b) {
  if(a<b) {
    if(v<a) return a;
    if(v>b) return b;
  } else {
    if(v>a) return a;
    if(v<b) return b;    
  }
  return v;
}

int mix_i(int v, int a, int b, int ma, int mb) {
  float m1 = (clamp_f(v,a,b)) - a / (b-a);
  return (mb - ma) * m1 + ma;
}

float mix_f(float v, float a, float b, float ma, float mb) {
  float m1 = (clamp_f(v,a,b)) - a / (b-a);
  return (mb - ma) * m1 + ma;
}


//
boolean load_fx_config() {
  // open the file
  boolean r = false;
  load_json_then("/etc/fx.json", 1024, [&r](JsonObject& root){
    // success. look for the fields
    if(root["servo_start"].is<const char*>()) {
      fx_servo_start = root["servo_start"].asString();
    }
    r = true;
  }, [](){
    // failure.
    // events.send("{\"error\":\"could not open\", \"file\":\"/etc/fx.json\"}");
  });
  return r;
}

//
void save_fx_config() {
  // open the file
  File file = SPIFFS.open("/etc/fx.json", "w");
  if(!file) {
    // could not open
    events.send("{\"error\":\"could not write\", \"file\":\"/etc/fx.json\"}");
  } else {
    StaticJsonBuffer<256> json;
    JsonObject& root = json.createObject();
    root["servo_start"] = fx_servo_start;
    root.printTo(file);
    file.close();
    events.send("{\"message\":\"saved /etc/fx.json\"}");  
  }
}


//
boolean load_servo_config(int index, ServoConfig &cfg) {
  // open the file
  String filename = String("/etc/servo") + String(index) + String(".json");
  boolean r = false;
  load_json_then(filename.c_str(), 1024, [&cfg, &r](JsonObject& root){
    // success. look for the ssid and password fields
    cfg.pin = root["pin"];
    cfg.min_micros = root["min_micros"];
    cfg.max_micros = root["max_micros"];
    cfg.zero_micros = root["zero_micros"];
    cfg.unit_scaling = root["unit_scaling"];
    r = true;
  }, [](){
    // failure.
    // events.send("{\"error\":\"could not open\", \"file\":\"/etc/servo\"}");
  });
  return r;
}

//
void save_servo_config(int index, ServoConfig &cfg) {
  // open the file
  String filename = String("/etc/servo") + String(index);
  File file = SPIFFS.open(filename.c_str(), "w");
  if(!file) {
    // could not open
    events.send("{\"error\":\"could not write\", \"file\":\"/etc/servo\"}");
  } else {
    StaticJsonBuffer<256> json;
    JsonObject& root = json.createObject();
    root["pin"] = cfg.pin;
    root["min_micros"] = cfg.min_micros;
    root.printTo(file);
    file.close();
    events.send("{\"message\":\"saved /etc/servo\"}");  
  }
}

// update a servo, called from timer handler
void servo_update_state(int index) {
  // get the servo
  Servo sv = servo[index];
  // where are we now?
  int cv = servo_current[index];
  // where we are going?
  int ct = servo_target[index];
  // what's the current servo state?
  switch(servo_state[index]) {
    case 0: // initial state / powered down
      if(cv != ct) {
        // target has changed. 
        servo_state[index] = 2;
        // power up and begin moving
        sv.attach(servo_pin[index]);
        // set to last known
        sv.writeMicroseconds( cv );        
      }
      break;
    case 1: // powered up, waiting
      if(cv != ct) {
        // moving again
        servo_state[index] = 2;
      } else {
        // timeout counting down
        if(servo_timeout[index] > 0) {
          servo_timeout[index]--;
        } else {
          // time to power down
          servo_state[index] = 0;
          sv.detach();
        }
      }
      break;
    case 2: // powered up, moving
      int sd = (ct-cv); // signed difference
      int ud = abs(sd); // unsigned difference
      if(sd == 0) {
        // we have reached the target, change state to waiting
        servo_state[index] = 1;
        // reset the timeout
        servo_timeout[index] = SERVO_POWER_TIMEOUT;
      } else {
        // limit by the speed
        int sspeed = servo_speed[index];
        if(ud>sspeed) {
          ud = sspeed;
          sd = sd < 0 ? -sspeed : sspeed;
        }
        // move in that direction
        servo_current[index] += sd;
        sv.write( servo_current[index] );
      }
      break;
      
  }
}

Ticker servos_update;
void servos_update_tick() {
  for(int i=0; i<SERVO_COUNT; i++) servo_update_state(i);
}


void servos_start() {
  servos_state = SERVO_STATE_ON;
  // update servos @ 20Hz
  servos_update.attach(0.05, servos_update_tick);
  // notify everyone of the state change
  events.send("{\"servo\":{\"state\":\"on\"}}", "servo");
}

void servos_stop() {
  servos_state = SERVO_STATE_OFF;
  // update servos @ 20Hz
  servos_update.detach();
  // notify everyone of the state change
  events.send("{\"servo\":{\"state\":\"off\"}}", "servo");
}

void servos_auto() {
  servos_state = SERVO_STATE_AUTO;
  // notify everyone of the state change
  events.send("{\"servo\":{\"state\":\"auto\"}}", "servo");
}

void servo_move(int servo, float angle, float speed) {
  if(speed==0) speed = 10;
  // get the servo config
  ServoConfig &cfg = servo_config[servo];
  // convert angle to microseconds
  int t = cfg.zero_micros + angle * cfg.unit_scaling;
  if(t<cfg.min_micros) t = cfg.min_micros;
  if(t>cfg.max_micros) t = cfg.max_micros;
  servo_target[servo] = t;
  servo_speed[servo] = speed * abs(cfg.unit_scaling);
}


// neopixels

CRGB parse_color_hex(String hex, boolean rgb=true, CRGB lacuna = CRGB::Black ) {
  byte b[3];
  // we need six characters
  if(hex.length()!=6) return lacuna;
  int p=0;
  for(int i=0;i<3;i++) {
    int c1 = parse_hex_char(hex[p++]);
    int c2 = parse_hex_char(hex[p++]);
    if(c1<0) return lacuna;
    if(c2<0) return lacuna;
    b[i] = (c1 << 4) + c2;
  }
  if(rgb) {
    return CRGB( b[0],b[1],b[2] );
  } else {
    return CHSV( b[0],b[1],b[2] );    
  }
}


CRGB parse_color_html(String s, CRGB lacuna = CRGB::Black) {
  // is it long enough?
  if(s.length() != 7) return lacuna;
  if(s[0] != '#') return lacuna;
  byte rgb[3];
  // parse each hex group
  int p=1;
  for(int i=0; i<3; i++) {
    if( !parse_bssid_hex(s,  rgb, p, i ) ) return lacuna;
    p+=2;
  }
  // we made it
  return CRGB( rgb[0], rgb[1], rgb[2] );
}

void update_rgb_pose(RGBPose * pose) {
  if(pose->animate) {
    int r = pose->rate;
    // update the time value and check where we are
    int t = pose->time++;
    if(t >= r) {
      // animation is over
      pose->color = pose->target;
      pose->animate = false;
    } else {
      // tween the values at the current step
      float tf = (float)t / (float)r * 255.0;
      int tfi = tf;
      pose->color = CRGB(
        lerp8by8(pose->origin[0], pose->target[0], tfi),
        lerp8by8(pose->origin[1], pose->target[1], tfi), 
        lerp8by8(pose->origin[2], pose->target[2], tfi)
      );
    }
  }
}

// update the pose
void increment_pose_time() {
  // go through the animated LEDS
  for(int i=0; i<LED_COUNT; i++) {
    update_rgb_pose(&color_pose[i]);
    update_rgb_pose(&noise_pose[i]);
  }
  // do we need to occasionally move the servos?
  if( (servo_twitch_rate>0) && (servos_state == SERVO_STATE_AUTO) ) {
    // still counting down?
    if(servo_twitch_counter > 0) {
      // decrement
      servo_twitch_counter--;
    } else {
      // twitch time's up. tremble the servos.
      boolean any = false;
      for(int i=0; i<SERVO_COUNT; i++) {
        // if the servo is in the twitch list...
        if(servo_pose_twitch[i]) {
          any = true;
          // tremble the servo.
          float v = servo_pose_target[i] + random16(servo_pose_tremble[i]);
          servo_move(i, v, servo_pose_speed[i]);
        }
      }
      // schedule another twitch timeout
      servo_twitch_counter = 1 + random16(servo_twitch_rate);
    }
  }
}

void rgb_animate(RGBPose * pose, CRGB target, int rate) {
  if(rate>0) {
    // slow animation
    pose->animate = true; // animate
    pose->origin = pose->color; // from the current colour
    pose->target = target; // towards the target final color
    pose->rate = rate; // at the given rate 
    pose->time = 0; // zero the clock
  } else {
    // immediate update
    pose->animate = false;
    pose->color = target;
  }
}

void led_animate(int index, CRGB target, int rate) {
  if(index<0) return;
  if(index>=LED_COUNT) return;
  rgb_animate(&color_pose[index], target, rate);
}


// Play with the values of the variables below and see what kinds of effects they
// have!  More octaves will make things slower.

// how many octaves to use for the brightness and hue functions
uint8_t octaves=5;
uint8_t hue_octaves=7;

// the 'distance' between points on the x and y axis
int xscale=57771;
int yscale=57771;

// the 'distance' between x/y points for the hue noise
int hue_scale=1;

// how fast we move through time & hue noise
int time_speed=1111;
int hue_speed=31;

// adjust these values to move along the x or y axis between frames
int x_speed=331;
int y_speed=1111;

// x,y, & time values
uint32_t noise_x,noise_y,noise_v_time,noise_hue_time,noise_hxy;


void fastled_setup() {
  // initialize
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, LED_COUNT);
  // initialize noise x/y and time values
  random16_set_seed(1337);
  // random16_add_entropy(analogRead(3));
  noise_hxy = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_x = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_y = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_v_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_hue_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
}

void fastled_noise(CRGB * data, int count) {
  // fill the led array 2/16-bit noise values
  fill_2dnoise16(data, count, 1, false,
                octaves, noise_x, xscale, noise_y, yscale, noise_v_time,
                hue_octaves, noise_hxy, hue_scale, noise_hxy, hue_scale, noise_hue_time, false);
  // adjust the intra-frame time values
  noise_x += x_speed;
  noise_y += y_speed;
  noise_v_time += time_speed;
  noise_hue_time += hue_speed;
}

// convert a degrees-per-second number into servo milliseconds-per-loop
float dps_servo_speed(float dps) {
  return clamp_f(dps, 0, 1000) / 50.0;
}

void fx_pose(String name, boolean do_servos = false) {
  // check if the file exists
  String filename = String("/pose/") + name + String(".json");
  const char * fn = filename.c_str();
  if(!SPIFFS.exists(fn)) return;
  // open the json file
  load_json_then(fn, 2048, [&do_servos](JsonObject& root){
    // success. read the pose settings
    if(root["led"].is<JsonObject&>()) {
      JsonObject& led = root["led"];
      // check each led index
      for(int index = 0; index<LED_COUNT; index++) {
        String n = String(index+1);
        if(led[n].is<JsonObject&>()) {
          JsonObject& ledn = led[n];
          // we may have a time, convert that into a "rate" (in loops)
          float led_t = ledn["t"];
          int rate = clamp_i( led_t * 50.0, 0,100 ); // how many 20-ish millisecond loops will the transition take
          // do we have a noise color?
          if(ledn["n"].is<const char *>()) {
            const char * led_n = ledn["n"];
            // assign that to the pose color array
            CRGB col = parse_color_html( led_n, true );
            // begin the transition
            rgb_animate(&noise_pose[index], col, rate);
          }
          // do we have a led color?
          if(ledn["c"].is<const char *>()) {
            const char * led_c = ledn["c"];
            // assign that to the pose color array
            CRGB col = parse_color_html( led_c, true );
            // begin the transition
            // led_animate(index, col, r);
            rgb_animate(&color_pose[index], col, rate);
            // report new color
            /*
            if(index==0){
              if(strlen(led_c)<64) { // sanity check
                char p[128];
                sprintf(p, "{\"status\":{\"color\":\"%s\"}}", led_c );
                events.send(p,"status");
              }
            }
            */
          }
        }
      }
    }
    // are we allowed to mess with the servos?
    if( do_servos || (servos_state == SERVO_STATE_AUTO) ) {
      // make sure the servos are on, of we're not already in auto mode
      if(servos_state == SERVO_STATE_OFF) servos_state_change == SERVO_STATE_ON;
      if(root["servo"].is<JsonObject&>()) {
        JsonObject& servo = root["servo"];
        // check each servo index
        for(int index = 0; index<SERVO_COUNT; index++) {
          String n = String(index+1);
          if(servo[n].is<JsonObject&>()) {
            JsonObject& servon = servo[n];
            double move_p = servon["p"]; 
            double move_v = servon["v"];
            float  pos = clamp_f(move_p, -90, 90); // limit to -90, 90
            float  spd = dps_servo_speed(move_v);
            servo_move(index,pos,spd);
            servo_pose_target[index] = pos;
            servo_pose_speed[index] = spd;
            // also update the random twitch and tremble if we have them
            if(servon.containsKey("tw")) {
              servo_pose_tremble[index] = (float)servon["tw"]; // tremble in degrees
            }
            /*
            // report new servo position
            char p[64];
             sprintf(p, "{\"servo\":{\"i\":%d, \"p\":%d, \"v\":%d }}", 
             index, int(move_p), int(move_v)
            );
            events.send(p);
            */
          }
        }
      }
      // servo twitch parameters?
      if(root["twitch"].is<JsonObject&>()) {
        JsonObject& twitch = root["twitch"];
        if(twitch.containsKey("t")) {
          servo_twitch_rate = (float)twitch["t"] * 50.0; // convert seconds to loops, assume about 20ms per loop
        }
        if(twitch["s"].is<JsonArray&>()) {
          // clear the old flags
          for(int i=0; i<SERVO_COUNT; i++) servo_pose_twitch[i] = false;
          // flip on the listed servos
          JsonArray& servo_list = twitch["s"];
          for(JsonArray::iterator it=servo_list.begin(); it!=servo_list.end(); ++it) {
            // *it contains the JsonVariant which can be casted as usuals
            int servo_number = *it;
            if( (servo_number > 0) && (servo_number <= SERVO_COUNT) ) {
              // that servo can twitch
              servo_pose_twitch[servo_number-1] = true;
            }
          }
        }
      }

    }
  }, [](){
    // failure.
    //events.send("{\"error\":\"could not load config /etc/htaccess \"}");
  });
}

void fx_loop() {
  // manage the servo system
  switch(servos_state_change) {
    case SERVO_STATE_OFF: 
      switch(servos_state) {
        case SERVO_STATE_ON: 
        case SERVO_STATE_AUTO: servos_stop(); break;
      }
      break;
    case SERVO_STATE_ON: 
      switch(servos_state) {
        case SERVO_STATE_OFF: 
        case SERVO_STATE_AUTO: servos_start(); break;
      }
      break;
    case SERVO_STATE_AUTO: 
      switch(servos_state) {
        case SERVO_STATE_OFF: servos_start();
        case SERVO_STATE_ON: servos_auto(); break;
      }
      break;
  }
  // update animations
  increment_pose_time();
  // check if any of the leds have a noise color
  boolean noisy = false;
  for(int i=0; i<LED_COUNT; i++) if(noise_pose[i].color) noisy = true;
  // update the LEDs with latest color+noise
  if(noisy) {
    // compute a block of noise with the current parameters
    CRGB led_noise[LED_COUNT];
    fastled_noise(led_noise, LED_COUNT);
    // now go through the leds and build the current color
    for(int i=0; i<LED_COUNT; i++) {
      RGBPose * pose = &color_pose[i];
      RGBPose * noise = &noise_pose[i];
      if(noise->color) {
        // scale the noise using the animated filter color, and add to the base pose color
        leds[i] = pose->color + CRGB( 
          lerp8by8(0, led_noise[i][0], noise->color[0]),
          lerp8by8(0, led_noise[i][1], noise->color[1]),
          lerp8by8(0, led_noise[i][2], noise->color[2])
        );
      } else {
        // no noise. copy this color values directly
        leds[i] = pose->color;
      }
    }
    
  } else {
    // copy all the color values directly
    for(int i=0; i<LED_COUNT; i++) {
      RGBPose * pose = &color_pose[i];
      leds[i] = pose->color;
    }
  }
  LEDS.setBrightness( pulse_brightness() );
  LEDS.show();
}

