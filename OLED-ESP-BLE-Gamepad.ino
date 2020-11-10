#include <Arduino.h>

#include "button.h"
#include "joystick.h"
#include "battery.h"
#include "ble.h"

button btn0 = { .pin = 0 };
button btn1 = { .pin = 35 };
button btn2 = { .pin = 19 };
button btn3 = { .pin = 22 };
button btn4 = { .pin = 21 };
button btnup = { .pin = 33};
button btndown = { .pin = 25};
button btnleft = { .pin =  26};
button btnright = { .pin = 27};
button* btn[] = { &btn0, &btn1, &btn2, &btn3, &btn4, &btnup, &btndown, &btnleft, &btnright };
const int N_BUTTONS = sizeof(btn) / sizeof(btn[0]);

battery bat = { .pin = 35, .R1 = 10e3, .R2 = 10e3 };
#define IWIDTH  160
#define IHEIGHT 128


#include <SPI.h>
#include <TFT_eSPI.h>

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

// With 1024 stars the update rate is ~65 frames per second
#define NSTARS 256
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};

uint8_t za, zb, zc, zx;

// Fast 0-255 random number generator from http://eternityforest.com/Projects/rng.php:
uint8_t __attribute__((always_inline)) rng()
{
  zx++;
  za = (za^zc^zx);
  zb = (zb+za);
  zc = (zc+(zb>>1)^za);
  return zc;
}

void setup() {
  Serial.begin(115200);
  initBattery(&bat);
//  drawBattery(&bat, 10, 0);
     za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);

  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // fastSetup() must be used immediately before fastPixel() to prepare screen
  // It must be called after any other graphics drawing function call if fastPixel()
  // is to be called again
  //tft.fastSetup(); // Prepare plot window range for fast pixel plotting

  for (int i = 0; i < N_BUTTONS; i++) {
    initButton(btn[i]);
    readButton(btn[i]);
//    drawButton(btn[i], i, 0);
  }


  initBLE();
}

void loop() {
  readBattery(&bat);
  if (bat.state != bat.prevState) {
    //drawBattery(&bat, 10, 0);
  }

  for (int i = 0; i < N_BUTTONS; i++) {
    readButton(btn[i]);
  }
  
  bool stateChange;

  for (int i = 0; i < N_BUTTONS; i++) {
    if (btn[i]->state != btn[i]->prevState) {
      stateChange = true;
//      drawButton(btn[i], i, 0);
      Serial.printf("%lu\tbtn %i\t%i\n", millis(), i, btn[i]->state);
    }
  }


  // Send BLE update.
  if (connected && stateChange) {

    uint8_t b = 0;
    for (int i = 0; i < N_BUTTONS; i++) {
      b |= btn[i]->state << i;
    }
 //  for (int i = 0; i < N_JOYSTICK_AXES; i++) {
 //   if (axis[i]->state != axis[i]->prevState) {
 //     stateChange = true;
 //     drawJoystick(axis[i], 4*i, 2);
 //     Serial.printf("%lu\taxis %i\t%i\n", millis(), i, axis[i]->state);
 //   }
 //   input->notify();
  }

  unsigned long t0 = micros();
  uint8_t spawnDepthVariation = 255;

  for(int i = 0; i < NSTARS; ++i)
  {
    if (sz[i] <= 1)
    {
      sx[i] = 160 - 120 + rng();
      sy[i] = rng();
      sz[i] = spawnDepthVariation--;
    }
    else
    {
      int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
      int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

      // This is a faster pixel drawing function for occassions where many single pixels must be drawn
      tft.drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

      sz[i] -= 2;
      if (sz[i] > 1)
      {
        int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
        int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;
  
        if (screen_x >= 0 && screen_y >= 0 && screen_x < 320 && screen_y < 240)
        {
          uint8_t r, g, b;
          r = g = b = 255 - sz[i];
          tft.drawPixel(screen_x, screen_y, tft.color565(r,g,b));
        }
        else
          sz[i] = 0; // Out of screen, die.
      }
    }
  }
  unsigned long t1 = micros();
  //static char timeMicros[8] = {};

 // Calcualte frames per second
  Serial.println(1.0/((t1 - t0)/1000000.0));
    delay(5);

}
