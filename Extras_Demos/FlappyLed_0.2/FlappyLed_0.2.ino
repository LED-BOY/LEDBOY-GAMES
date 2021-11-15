
/*FlappyLed for LEDBOY and any Attiny series 1 compatible using neopixels
  Flash CPU Speed 8MHz.
  this code is released under GPL v3, you are free to use and modify.
  released on 2021.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    To contact us: ledboy.net
    ledboyconsole@gmail.com
*/


#include <tinyNeoPixel.h>
#include "characters.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>



// pins

  #define NUMLEDS 100
  #define RIGHT PIN_PA1
  #define LEFT PIN_PA2
  #define SLEEPMENU PIN_PA3
  #define MOSFET PIN_PA4
  #define TOUCH PIN_PA5
  #define POT PIN_PA6
  #define EXT PIN_PA7
  #define NEOPIN PIN_PB0
  #define BUZZER PIN_PB1

uint8_t red = 100; // red led intensity
uint8_t blue = 0;// blue led intensity
uint8_t green = 0;// green led intensity
uint16_t score;
uint16_t arrayNumberValue[1];
uint32_t buttonLeftTimeStamp; // button press time
uint32_t buttonRightTimeStamp; // button press time
uint32_t buttonMenuTimeStamp; // button press time
bool sound = true;
volatile uint8_t interruptTimer = 0;

tinyNeoPixel strip = tinyNeoPixel(NUMLEDS, NEOPIN, NEO_GRB);

void setup() {


  pinMode(SLEEPMENU, INPUT);
  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(POT, INPUT);
  pinMode(TOUCH, INPUT);
  pinMode(EXT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(MOSFET, OUTPUT);

  digitalWrite(MOSFET, LOW);// P CHANNEL mosfet low to activate

  strip.begin();
  strip.setBrightness(50);// brightness select from 0 to 255

  TCB0.INTCTRL = TCB_CAPT_bm; // Setup Timer B as compare capture mode to trigger interrupt
  TCB0_CCMP = 8000;// CLK / 2
  TCB0_CTRLA = (1 << 1) | TCB_ENABLE_bm;


  sei(); // enable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);// configure sleep mode
}

void loop() {
  game();
}

void game() {
  uint8_t randomWall = 0;
  uint8_t flappyLedIput = 0;
  uint8_t flappyLedPos = 53;
  uint8_t lastflappyLedPos;
  uint8_t wallPos = 9;
  uint8_t passage1;
  uint8_t passage2;
  uint16_t flappyLedTimer = 0;
  uint16_t wallTimer = 0;
  uint16_t gameSpeed = 800;

  while (true) {

    arrayNumberValue[0] = pgm_read_word_near(walls + randomWall);

    if (interruptTimer) {
      flappyLedTimer += interruptTimer;
      wallTimer += interruptTimer;
    }

    if (buttonDebounce(0, 0, 1, 100)) {
      while (!buttonDebounce(1, 1, 0, 50)) {

      }
    }

    if (flappyLedPos > 3 && buttonDebounce(1, 1, 0, 50)) {
      flappyLedPos -= 10;
      beep(25, 25);
    }

    strip.setPixelColor(flappyLedPos, 0, 100, 0);

    if (flappyLedPos != lastflappyLedPos) {
      strip.setPixelColor(lastflappyLedPos, 0, 0, 0);
    }
    lastflappyLedPos = flappyLedPos;

    if (flappyLedTimer >= 900 && flappyLedPos < 93) {
      flappyLedTimer = 0;
      flappyLedPos += 10;
    }

    if (wallTimer > gameSpeed && wallPos >= 0) {
      wallTimer = 0;
      drawCharacter(wallPos, arrayNumberValue[0], 100, 0, 0);
      if (wallPos < 9) {
        drawCharacter((wallPos + 1), arrayNumberValue[0], 0, 0, 0);
      }
      if (wallPos == 0) {
        wallPos = 9;
        drawCharacter(0, arrayNumberValue[0], 0, 0, 0);
        randomWall = random(0 , 4);
      } else {
        wallPos --;

        if (wallPos == 2) {
          if (score <= 999) {
            score += 2;
          }
        }
      }
      if (gameSpeed > 200) {
        gameSpeed--;
      }
    }


    passage1 = pgm_read_byte_near (arrayNumberValue[0] + 9);
    passage2 = pgm_read_byte_near (arrayNumberValue[0] + 10);

    if (wallPos == 3 && (flappyLedPos < passage1 || flappyLedPos > (passage2 + 14))) {
      for (uint16_t x = 0; x < 600; x += 100) {// game music
        beep(100, x);
        beep(100, x);
      }
      strip.clear();
      while (!buttonDebounce(1, 1, 0, 50)) {
        showScore(score);
      }
      delay(1000);
      score = 0;
      strip.clear();
      break;
    }
  }
}

void showScore(uint16_t valueShort) {// draws all score using showTime function
  uint16_t valueDecimals = valueShort;
  if (valueShort <= 999) {

    if (valueShort < 100) { // functions to draw units in digit 3, decimals in digit 2 and hundredths in digit 1
      showTime(53, valueShort);
    }

    if (valueShort >= 100 && valueShort <= 999) {
      valueShort /= 100;
      arrayNumberValue[0] = pgm_read_word_near(number + valueShort);
      drawCharacter(50, arrayNumberValue[0], red, green, blue);
      valueShort *= 100;

      for (uint16_t x = 0; x < valueShort ; x++) {
        valueDecimals--;
      }
      showTime(53, valueDecimals);
    }
  }
}

void showTime (uint8_t firstLed, uint8_t value) {// this function takes the digit and value from characters.h and draws it without the 0 to the left in hours
  uint8_t valueUnits = value;                      // always draws 2 digits.

  if (value < 10) {
    if (firstLed == 0) {
      for (uint8_t x = 0; x < 13; x++) {
        strip.setPixelColor(x, 0, 0, 0);
      }
    } else {
      drawCharacter(firstLed, number0, red, green, blue);
    }
    arrayNumberValue[0] = pgm_read_word_near(number + value);
    drawCharacter((firstLed + 4), arrayNumberValue[0], red, green, blue);
  } else {
    value /= 10; // some math to substract the 0 from the left in hours digits
    arrayNumberValue[0] = pgm_read_word_near(number + value);
    drawCharacter(firstLed, arrayNumberValue[0], red, green, blue);
    value *= 10;

    for (uint8_t x = 0; x < value ; x++) {
      valueUnits--;
    }
    arrayNumberValue[0] = pgm_read_word_near(number + valueUnits);
    drawCharacter((firstLed + 4), arrayNumberValue[0], red, green, blue);
  }
}

void drawCharacter (uint8_t firstLed, uint8_t arrayName[], uint8_t red, uint8_t green, uint8_t blue) {// draws on digit form 0 to 3 taking values from array in characters

  uint8_t led = 0;
  uint8_t arrayElements = pgm_read_byte_near (arrayName + 0);

  for (uint8_t x = 0; x < arrayElements; x++) {
    if (x != 0) {
      led = pgm_read_byte_near (arrayName + x);
      strip.setPixelColor((led + firstLed), red, green, blue);
    }
  }
}

bool buttonDebounce(uint8_t left, uint8_t right, uint8_t menu, uint32_t debounceTime ) {// code to debounce button press

  if ( digitalRead(RIGHT) == LOW && right == 1 && (millis() - buttonRightTimeStamp) > debounceTime) {
    buttonRightTimeStamp = millis();
    return true;
  } else if (digitalRead(RIGHT) == HIGH) {
    buttonRightTimeStamp = millis();
  }

  if ( digitalRead(LEFT) == LOW && left == 1 && (millis() - buttonLeftTimeStamp) > debounceTime) {
    buttonLeftTimeStamp = millis();
    return true;
  } else if (digitalRead(LEFT) == HIGH) {
    buttonLeftTimeStamp = millis();
  }

  if ( digitalRead(SLEEPMENU) == LOW && menu == 1 && (millis() - buttonMenuTimeStamp) > debounceTime) {
    buttonMenuTimeStamp = millis();
    return true;
  } else if (digitalRead(SLEEPMENU) == HIGH) {
    buttonMenuTimeStamp = millis();
  }

  return false;
}

void beep(int Count, int Delay) { // sound function
  if (!sound) {
    return;
  }
  for (int x = 0; x <= Count; x++) {
    digitalWrite(BUZZER, HIGH);
    for (int y = 0; y < Delay; y++) {
      __asm__("nop\n\t"); //small code dealy needed to generate sound
    } digitalWrite(BUZZER, LOW);
    for (int y = 0; y < Delay; y++) {
      __asm__("nop\n\t");
    }
  }
}

ISR(TCB0_INT_vect) {// anoter interrupt to show pixels on screen, because micros are lost using RTC and .show cna´t be called one after another  this is the best way I found to call "strip.show"
  strip.show();
  if (interruptTimer > 0) { // used in game to keep track of time
    interruptTimer = 0;
  } else {
    interruptTimer = 5;
  }
  TCB0_INTFLAGS = 1; // clear flag
}
