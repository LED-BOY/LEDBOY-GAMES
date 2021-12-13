
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

uint16_t arrayNumberValue[1];
uint16_t gameSpeed = 600;
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
  //pinMode(TOUCH, INPUT);
  //pinMode(EXT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(MOSFET, OUTPUT);

  digitalWrite(MOSFET, LOW);// P CHANNEL mosfet low to activate

  strip.begin();
  strip.setBrightness(10);// brightness select from 0 to 255

  TCB0.INTCTRL = TCB_CAPT_bm; // Setup Timer B as compare capture mode to trigger interrupt
  TCB0_CCMP = 8000;// CLK / 2
  TCB0_CTRLA = (1 << 1) | TCB_ENABLE_bm;


  sei(); // enable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);// configure sleep mode
}


void loop() {
  mainTitle ();
}

void mainTitle () { // title of the game
  for (uint8_t x = 0; x <= 3; x++) {
    switch (x) {
      case 1:
        scrollPixels(ledTrisTitle1, true, 100, 100, 0);
        break;
      case 2:
        scrollPixels(ledTrisTitle2, true, 100, 100, 0);
        break;
      case 3:
        scrollPixels(ledTrisTitle3, true, 100, 100, 0);
        break;
    }

    if (buttonDebounce(1, 1, 0, 0)) {
      break;
    }
  }
  strip.clear();
  game();
}

void game() {
  uint8_t actualTetromino[1] = {tetromino1A};
  uint8_t lastTetromino[1] = {tetromino1A};
  uint16_t tetrominoInPos [10][10];
  int tetrominoLastPos = -6;
  int tetrominoPos = -6;
  uint8_t tetrominoRotation;
  uint8_t tetrominoLastRotation;
  uint8_t randomTetromino = 0;
  uint8_t lives = 1;
  uint8_t tetrominoStopPlace = 0;
  uint16_t tetrominoTimer = 0;
  bool tetris = false;
  bool tetrominoMoveRight = true;
  bool tetrominoMoveLeft = true;
  bool tetrominoStop = false;
  bool moveTetrominos = false;
  bool levelComplete = false;

  for ( uint8_t row = 0; row < 10; row++ ) {
    for ( uint8_t column = 0; column < 10; column++ ) {
      tetrominoInPos [row][column] = 0;
    }
  }

  for (uint8_t x = 0; x < 10; x++) {
    //tetrominoInPos [9][x] = pgm_read_byte_near (bottomFloor + x);
    tetrominoInPos [x][0] = pgm_read_byte_near (wallLeft + x);
    tetrominoInPos [x][9] = pgm_read_byte_near (wallRight + x);
    strip.setPixelColor(tetrominoInPos [x][0], 101, 67, 37);
    strip.setPixelColor(tetrominoInPos [x][9], 101, 67, 37);
  }

  while (lives > 0) {

    tetrominoRotation = map(analogReadEnh(POT, 11), 0, 2000, 0, 3);
    actualTetromino[0] = tetromino [randomTetromino][tetrominoRotation];

    if (interruptTimer) {
      tetrominoTimer += interruptTimer;
    }

    if (tetrominoTimer >= gameSpeed && tetrominoPos < 99) {
      tetrominoTimer = 0;
      tetrominoPos += 10;
    }

    tetrominoStop = false;
    tetrominoMoveRight = true;
    tetrominoMoveLeft = true;

    for (uint8_t x = 1; x < pgm_read_byte_near (actualTetromino[0] + 0); x++) {

      for ( uint8_t row = 0; row < 10; row++ ) {
        for ( uint8_t column = 0; column < 10; column++ ) {

          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos - 1)) == tetrominoInPos [row][column]) {
            tetrominoMoveLeft = false;
          }

          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos + 1)) == tetrominoInPos [row][column]) {
            tetrominoMoveRight = false;
          }

          if (tetrominoRotation != tetrominoLastRotation) {

            if (!tetrominoMoveRight) {
              if ((pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) == tetrominoInPos [row][column]) {
                tetrominoPos --;
              }
            }

            if (!tetrominoMoveLeft) {
              if ((pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) == tetrominoInPos [row][column]) {
                tetrominoPos++;
              }
            }


          } else {

            if (row == 9 && (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) > 89) {
              tetrominoStop = true;
              break;
            }

            if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos + 10)) == tetrominoInPos [row][column]) {
              tetrominoStop = true;

              if (row < 3) {
                lives = 0;
              }
              break;
            }
          }
        }
      }
    }

    if (tetrominoPos != tetrominoLastPos || tetrominoRotation != tetrominoLastRotation) {
      drawCharacter(tetrominoLastPos, lastTetromino[0], 0, 0, 0);
      tetrominoLastPos = tetrominoPos;
      tetrominoLastRotation = tetrominoRotation;
      lastTetromino[0] = actualTetromino[0];
      drawCharacter(tetrominoPos, actualTetromino[0], 0, 50, 0);
    }

    if (tetrominoStop) {
      beep(50, 150);
      for (uint8_t x = 1; x < pgm_read_byte_near (actualTetromino[0] + 0); x++) {
        uint8_t valueDecimal = (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) / 10 ;
        uint8_t valueUnits = pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos;

        for (uint8_t y = 0; y < (valueDecimal * 10) ; y++) {
          valueUnits--;
        }
        tetrominoInPos [valueDecimal][valueUnits] = (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos);
        strip.setPixelColor(tetrominoInPos [valueDecimal][valueUnits], 50, 0, 0);
      }

      for (int cont = 3; cont > 0; cont--) {
        for ( uint8_t row = 9; row > 2; row-- ) {
          tetris = true;

          for ( uint8_t column = 1; column < 9; column++ ) {

            if (moveTetrominos && tetrominoInPos [row][column] != 0) {
              tetrominoInPos [(row + 1)][column] = (tetrominoInPos [row][column] + 10);
              strip.setPixelColor(tetrominoInPos [row][column], 0, 0, 0);
              beep(110, 110);
              strip.setPixelColor(tetrominoInPos [(row + 1)][column], 0, 0, 50);
              tetrominoInPos [row][column] = 0;
            }
            if (tetrominoInPos [row][column] == 0) {
              tetris = false;
              levelComplete = false;
            }
          }

          if (tetris) {
            levelComplete = true;
            moveTetrominos = true;
            for ( uint8_t tetrisColumn = 1; tetrisColumn < 9; tetrisColumn++ ) {
              strip.setPixelColor(tetrominoInPos [row][tetrisColumn], 0, 0, 0);
              tetrominoInPos [row][tetrisColumn] = 0;
            }
          }
        }

        moveTetrominos = false;
      }
      tetrominoPos = -6;
      randomTetromino = random(5);
      tetrominoLastPos = tetrominoPos;
      tetrominoLastRotation = tetrominoRotation;
    }

    if (tetrominoPos > 0) {

      if (digitalRead(RIGHT) == LOW && digitalRead(LEFT) == LOW) {
        gameSpeed = 100;
      } else {
        gameSpeed = 600;
      }

      if (buttonDebounce(1, 0, 0, 100) && digitalRead(RIGHT) == HIGH && tetrominoMoveLeft) {
        tetrominoPos--;
      }

      if (buttonDebounce(0, 1, 0, 100) && digitalRead(LEFT) == HIGH && tetrominoMoveRight) {
        tetrominoPos++;
      }
    }

    if (buttonDebounce(0, 0, 1, 200)) {
      strip.clear();
      while (!buttonDebounce(0, 0, 1, 500)) {
      }
    }

    if (levelComplete) {
      gameSpeed -= 20;
      break;
    }
  }

  gameSpeed = 600;
  strip.clear();
  drawCharacter(20, letterE, 100, 0, 0);
  drawCharacter(23, letterN, 100, 0, 0);
  drawCharacter(26, letterD, 100, 0, 0);

  while (!buttonDebounce(1, 1, 0, 500)) {
  }
  strip.clear();
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

void scrollPixels(uint8_t arrayName[5][9], bool animateTitle, uint8_t red, uint8_t green, uint8_t blue) {// code to scroll the tiltle
  uint8_t led;
  uint8_t pixel;
  uint32_t pixelColor;

  for (uint8_t pixelY = 0; pixelY < 9; pixelY++) {

    for (uint8_t x = 0; x < 5; x++) { // scroll text from right to left only on colum of information is draw
      led = pgm_read_byte_near (wallRight + x);
      pixel = arrayName  [x] [pixelY];
      if (pixel == 1) {
        strip.setPixelColor(led, red, green, blue);
      } else {
        strip.setPixelColor(led, 0, 0, 0);
      }
    }
    delay(150);
    for (uint8_t y = 0; y < 50; y++) {// then the getPixel comand is used to grab the pixel info an scroll it to the left
      pixelColor = strip.getPixelColor(y);
      strip.setPixelColor(y, 0);
      if ( y != 0 && pixelColor > 0) {
        strip.setPixelColor((y - 1), red, green, blue);
      }
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
    interruptTimer = 15;
  }
  TCB0_INTFLAGS = 1; // clear flag
}
