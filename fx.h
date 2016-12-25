
// fastled library includes
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

// neopixel setup

// How many leds are in the strip?
#define NUM_LEDS 1

// Data pin that led data will be written out over
#define DATA_PIN 2
//#define DATA_PIN 16

// global array for the current LED state
CRGB leds[NUM_LEDS];


// servo pheripherals
#include <Servo.h>
#define SERVO_COUNT 2
#define SERVO_POWER_TIMEOUT 500

Servo servo[SERVO_COUNT];
int servo_pin[SERVO_COUNT] = { 14,12 };
int servo_state[SERVO_COUNT] = { 0,0 };
int servo_current[SERVO_COUNT] = { 1000,1000 };
int servo_target[SERVO_COUNT] = { 1000,1000 };
int servo_speed[SERVO_COUNT] = { 4,4 };
int servo_timeout[SERVO_COUNT] = { 0,0 };

int servos_state = 0;
int servos_state_change = 0;

#define SERVO_STATE_OFF      0
#define SERVO_STATE_START    1
#define SERVO_STATE_ACTIVE   2
#define SERVO_STATE_STOP     3

void servo_update_state(int index) {
  // sanity check
  if(index<0) return;
  if(index>1) return;
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
          // change to invalid PWM value
          // sv.writeMicroseconds( 10 );
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
  servo_update_state(0);
  servo_update_state(1);
}


void servos_start() {
  // Serial.print("servo_setup\n");
  servos_state = SERVO_STATE_ACTIVE;
  // servo[0].attach(12); servo[0].writeMicroseconds(10);
  // servo[1].attach(14); servo[1].writeMicroseconds(10);
  // update servos @ 20Hz
  servos_update.attach(0.05, servos_update_tick);
  // notify everyone of the state change
  events.send("{\"servo\":{\"state\":\"started\"}}", "servo");
}

void servos_stop() {
  // Serial.print("servo_setup\n");
  servos_state = SERVO_STATE_OFF;
  // servo[0].detach();
  // servo[1].detach();
  // update servos @ 20Hz
  servos_update.detach();
  // notify everyone of the state change
  events.send("{\"servo\":{\"state\":\"stopped\"}}", "servo");
}

void servo_move(int servo, int angle, int speed) {
  if(speed==0) speed = 10;
  // convert angle to milliseconds
  if(angle<0) angle = 0;
  if(angle>180) angle = 180;
  // int t = 760 + angle * 6;
  int t = 650 + angle * 6;
  servo_target[servo] = t;
  servo_speed[servo] = speed;
}

// neopixels

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
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  // starting color
  leds[0] = CRGB::Green;
  // Show the leds 
  FastLED.show();
  // initialize the x/y and time values
  random16_set_seed(8934);
  // random16_add_entropy(analogRead(3));
  noise_hxy = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_x = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_y = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_v_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
  noise_hue_time = (uint32_t)((uint32_t)random16() << 16) + (uint32_t)random16();
}

void fastled_noise() {
  // fill the led array 2/16-bit noise values
  fill_2dnoise16(LEDS.leds(), 1, 1, false,
                octaves,noise_x,xscale,noise_y,yscale,noise_v_time,
                hue_octaves,noise_hxy,hue_scale,noise_hxy,hue_scale,noise_hue_time, false);

  // adjust the intra-frame time values
  noise_x += x_speed;
  noise_y += y_speed;
  noise_v_time += time_speed;
  noise_hue_time += hue_speed;
  // delay(50);
}

void fx_loop() {
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
  //
  
}



