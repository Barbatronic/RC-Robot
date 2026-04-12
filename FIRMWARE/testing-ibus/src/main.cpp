#include <Arduino.h>
#include <IBusBM.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN    D10
#define LED_COUNT  36
#define BAND_SIZE  18        // LEDs par bande (gauche / droite)
#define BAND_HALF  9         // Centre virtuel entre LED 8 et 9

const int pinIbus = D2;
const int pinSDA  = D4;
const int pinSCL  = D5;

IBusBM IBus;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ─── Prototypes ──────────────────────────────────────────────────────────────
void displayBand(int startLed, int level, int speedPercent);

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  IBus.begin(Serial1, 1, pinIbus);
  Wire.begin(pinSDA, pinSCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);

  strip.begin();
  strip.clear();
  strip.show();
  strip.setBrightness(255);

  Serial.println("Start iBUS monitor");
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  int ch1 = IBus.readChannel(1-1); // Accélération (avant / arrière)
  int ch3 = IBus.readChannel(3-1); // Rotation     (gauche / droite)
  int ch6 = IBus.readChannel(6-1);

  // Centré sur 1500 → plage [-100 ; +100]
  int acc = map(ch1, 1000, 2000, -100, 100);
  int rot = map(ch3, 1000, 2000, -100, 100);

  // Vitesses différentielles (style char à chenilles / tank)
  int leftSpeed  = constrain(acc + rot, -100, 100);
  int rightSpeed = constrain(acc - rot, -100, 100);

  // Niveau LED : -9 (arrière max) → 0 (stop) → +9 (avant max)
  int leftLevel  = map(leftSpeed,  -100, 100, BAND_HALF, -BAND_HALF);
  int rightLevel = map(rightSpeed, -100, 100, BAND_HALF, -BAND_HALF);

  // --- LEDs ---
  strip.clear();
  displayBand(0,         leftLevel,  leftSpeed);   // Bande gauche : LED  0-17
  displayBand(BAND_SIZE, rightLevel, rightSpeed);  // Bande droite : LED 18-35
  strip.show();

  // --- Yeux ---
  int pan  = map(IBus.readChannel(0), 1000, 2000, -2, 2);
  int tilt = map(IBus.readChannel(2), 1000, 2000, -2, 2);

  if      (pan ==  0 && tilt ==  0) { roboEyes.setPosition(DEFAULT); roboEyes.setMood(DEFAULT); }
  else if (pan ==  0 && tilt == -2) { roboEyes.setPosition(S);       roboEyes.setMood(ANGRY);   }
  else if (pan ==  0 && tilt ==  2)   roboEyes.setPosition(N);
  else if (pan == -2 && tilt ==  0)   roboEyes.setPosition(W);
  else if (pan ==  2 && tilt ==  0)   roboEyes.setPosition(E);
  else if (pan == -2 && tilt == -2)   roboEyes.setPosition(SW);
  else if (pan ==  2 && tilt == -2)   roboEyes.setPosition(SE);
  else if (pan == -2 && tilt ==  2)   roboEyes.setPosition(NW);
  else if (pan ==  2 && tilt ==  2)   roboEyes.setPosition(NE);

  if (map(ch6, 1000, 2000, 0, 1) == 1) roboEyes.anim_laugh();
  roboEyes.update();

  delay(10);
}

// ─── displayBand ─────────────────────────────────────────────────────────────
// startLed    : index de la première LED de la bande (0 ou 18)
// level       : nombre de LEDs allumées [-9 ; +9], 0 = éteint
// speedPercent: vitesse en % [-100 ; +100], utilisé pour la couleur
//
// Disposition physique (18 LEDs, centre virtuel entre 8 et 9) :
//   Arrière  [0 .. 8] | [9 .. 17]  Avant
//                        ↑ level > 0 s'allume vers la droite
//            ↑ level < 0 s'allume vers la gauche
// ─────────────────────────────────────────────────────────────────────────────
void displayBand(int startLed, int level, int speedPercent) {
  int magnitude = abs(speedPercent);                      // 0-100
  uint8_t r = map(magnitude, 0, 100,   0, 255);           // 0 = vert, 100 = rouge
  uint8_t g = map(magnitude, 0, 100, 255,   0);
  uint32_t color = strip.Color(r, g, 0);

  for (int i = 0; i < BAND_SIZE; i++) {
    bool lit = false;

    if (level > 0) {
      // Avant : LEDs 9 → 9+level-1
      lit = (i >= BAND_HALF && i < BAND_HALF + level);
    } else if (level < 0) {
      // Arrière : LEDs 8 → 8+level+1  (level est négatif)
      lit = (i < BAND_HALF && i >= BAND_HALF + level);
    }

    strip.setPixelColor(startLed + i, lit ? color : 0);
  }
  // Pas de strip.show() ici → appelé une seule fois dans loop()
}