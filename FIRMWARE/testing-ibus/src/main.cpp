#include <Arduino.h>
#include <IBusBM.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN    3
#define LED_COUNT  10

IBusBM IBus; 
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
RoboEyes<Adafruit_SSD1306> roboEyes(display); 

void setup() {
  Serial.begin(115200);
  IBus.begin(Serial1, 1, 18, 17);
  Wire.begin(2, 1);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C or 0x3D
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100); // screen-width, screen-height, max framerate

  // Define some automated eyes behaviour
  roboEyes.setAutoblinker(ON, 3, 2); // Start auto blinker animation cycle -> bool active, int interval, int variation -> turn on/off, set interval between each blink in full seconds, set range for random interval variation in full seconds
  roboEyes.setIdleMode(ON, 2, 2);

  strip.begin();
  strip.clear();
  strip.show();
  strip.setBrightness(100);

  Serial.println("Start iBUS monitor");
}


void loop() {

  int channelValue01 = IBus.readChannel(1-1);
  int channelValue02 = IBus.readChannel(2-1);

  int channelValue03 = IBus.readChannel(3-1);
  int channelValue04 = IBus.readChannel(4-1);
  int channelValue06 = IBus.readChannel(6-1);

  int mappedValue03 =  map(channelValue03, 1000, 2000, 0, LED_COUNT-1);
  int mappedValue04 =  map(channelValue04, 1000, 2000, 0, 255);
  int mappedValue06 =  map(channelValue06, 1000, 2000, 0, 1);

  int pan = map(channelValue01, 1000, 2000, -2, 2);
  int tilt = map(channelValue03, 1000, 2000, -2, 2);

  if      (pan == 0 && tilt == 0) roboEyes.setPosition(DEFAULT);
  else if (pan == 0 && tilt == -2) roboEyes.setPosition(N);
  else if (pan == 0 && tilt == 2) roboEyes.setPosition(S);
  else if (pan == -2 && tilt == 0) roboEyes.setPosition(E);
  else if (pan == 2 && tilt == 0) roboEyes.setPosition(W);
  else if (pan == -2 && tilt == -2) roboEyes.setPosition(NE);
  else if (pan == 2 && tilt == -2 ) roboEyes.setPosition(NW);
  else if (pan == -2 && tilt == 2 ) roboEyes.setPosition(SE);
  else if (pan == 2 && tilt == 2) roboEyes.setPosition(SW);


  strip.clear();
  Serial.println(channelValue03);
  strip.setPixelColor(mappedValue03, strip.Color(mappedValue04, mappedValue04-255, 0));
  strip.show();


  if(mappedValue06 == 1)
  {
    roboEyes.anim_laugh();
  }

  roboEyes.update();
  //delay(10);
}