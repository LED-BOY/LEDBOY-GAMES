/*LedBoyGamePackA with Clock for LEDBOY and any Attiny series 1 compatible using neopixels
  Flash CPU Speed 8MHz.
  this code is released under GPL v3, you are free to use and modify.
  released on 2021-2022.

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


#include <tinyNeoPixel_Static.h>
#include "characters.h"
#include "encoder.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#if defined(MILLIS_USE_TIMERD0) || defined(MILLIS_USE_TIMERB0) || defined(MILLIS_USE_TIMERA0)
#error "This sketch does't use TCD0, TCB0 nor TCA0 , select RTC whit crystal as millis counter, only enable interrupts in enabled ports."
#endif

// pins
#define NUMLEDS 100
//#define RIGHT PIN_PA1
//#define LEFT PIN_PA2
//#define SLEEPMENU PIN_PA3
//#define MOSFET PIN_PA4
#define EXT PIN_PA7
#define NEOPIN PIN_PB0
#define BUZZER PIN_PB1
//#define ROTARYA PIN_PA5
//#define ROTARYB  PIN_PA6
#define PB3_INTERRUPT PORTA.INTFLAGS & PIN3_bm
#define PB3_CLEAR_INTERRUPT_FLAG PORTA.INTFLAGS &= PIN3_bm
#define RIGHTLOW !(PORTA.IN & PIN1_bm)
#define LEFTLOW !(PORTA.IN & PIN2_bm)
#define SLEEPMENULOW !(PORTA.IN & PIN3_bm)
#define RIGHTHIGH (PORTA.IN & PIN1_bm)
#define LEFTHIGH (PORTA.IN & PIN2_bm)
#define SLEEPMENUHIGH (PORTA.IN & PIN3_bm)

// clock variables
uint8_t red = 0; // red led intensity
uint8_t blue = 0;// blue led intensity
uint8_t green = 0;// green led intensity
uint8_t volatile hours = 0; //keep the hours to display
uint8_t volatile minutes = 0;//keep the minutes to display
uint8_t volatile seconds = 0;//keep the seconds to display
uint8_t color; // store color value Red, blue, green, white
uint8_t brightLevel;
uint8_t lastMinute = 60; // used to update the digits only once per minute
uint8_t lastSecond = 0;
uint8_t lastHour = 0;
int voltageReading;
//game variables
uint8_t LedKanoidNumberOfplayers;
uint8_t gameLevel = 1;
uint8_t ledInvaderLives = 2;
uint8_t enemiesLevel = 0;
uint16_t arrayNumberValue[1]; // stores one array pointer to array number in characters.h
uint16_t gameSpeed;
uint16_t ledKanoidScore = 0;
uint16_t ledInvadersScore = 0;
bool sound;
bool endGame = false;
bool contGame = true;
//menu interrupt variables
int gameSelect = 0;
bool enterGame = false;
uint32_t timer = 0;
uint32_t buttonLeftTimeStamp; // button press time
uint32_t buttonRightTimeStamp; // button press time
uint32_t buttonMenuTimeStamp; // button press time
volatile bool shouldWakeUp = true;
volatile uint8_t interruptTimer = 0;

byte pixels[NUMLEDS * 3];
tinyNeoPixel strip = tinyNeoPixel(NUMLEDS, NEOPIN, NEO_GRB, pixels);

void setup() {

  PORTA.DIR = 0b10010000;//configs port A pins as input or output
  PORTB.DIR = 0b00000011;//configs port B pins as input or output

  PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
  PORTA.PIN6CTRL = PORT_PULLUPEN_bm;

  PORTA.OUT &= ~PIN4_bm;// P CHANNEL mosfet low to activate

  if (eeprom_read_byte((uint8_t*)255) != 131) { // prevents excessive flash write and default values if first time used or no colors are choose
    eeprom_write_byte((uint8_t*)255, 131); // 255B of aviable EEPROM on the ATTINY1614
    eeprom_write_byte((uint8_t*)2, 1);//color
    eeprom_write_byte((uint8_t*)3, 2);//brightness level
    eeprom_write_byte((uint8_t*)4, true);//sound on/off
    writeScoreToEEPROM(5, 0); // LedIvaders Score function to store uint16 in two uint8 eeprom slots
    //eeprom_write_byte((uint8_t*)6, 0);This is reserved to ledInvadersScore don´t uncoment and don´t use in any other part of code.
    writeScoreToEEPROM(7, 0); // Ledtris score
    //eeprom_write_byte((uint8_t*)8, 0);// reserved for ledtris score
    writeScoreToEEPROM(9, 0); // FlappyLed score
    //eeprom_write_byte((uint8_t*)10, 0);// reserved for ledtris score
    writeScoreToEEPROM(11, 0); // Ledkanoid score
    //eeprom_write_byte((uint8_t*)12, 0);// reserved for ledkanoid score
  }

  //read from EEPROM
  color = eeprom_read_byte((uint8_t*)2); //read stored color
  sound = eeprom_read_byte((uint8_t*)4); //read if mute on/off

  strip.begin();
  brightLevel = eeprom_read_byte((uint8_t*)3); // read brightness from eeprom
  strip.setBrightness(brightLevel * 3);// brightness select from 0 to 255

  RTC.PITINTCTRL = RTC_PI_bm; /* Periodic Interrupt: enabled */
  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 32768 */
                 | RTC_PITEN_bm; /* Enable: enabled */

  TCB0.INTCTRL = TCB_CAPT_bm; // Setup Timer B as compare capture mode to trigger interrupt
  TCB0_CCMP = 8000;// CLK
  TCB0_CTRLA = (1 << 1) | TCB_ENABLE_bm;

  sei(); // enable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);// configure sleep mode
}

void loop() {
  showclock();

  if (seconds != lastSecond) {
    lastSecond = seconds;
    battStatus();// update batt status when seconds change
  }

  if ((millis() - timer) > 10000 && shouldWakeUp) { // sleep code while in clock mode
    randomSeed(analogRead(EXT));
    timer = millis();
    strip.clear();
    strip.show();
    lastMinute = 60;
    shouldWakeUp = false;
    PORTA.OUT |= PIN4_bm;// P CHANNEL mosfet High to deactivate
    PORTA.PIN3CTRL = PORT_ISC_BOTHEDGES_gc; //attach interrupt to portA pin 3
    sleep_enable();
    sleep_cpu();// go to sleep
  }

  if (LEFTLOW && RIGHTLOW) clockAlwaysOn (); // Clock always on function

  if (buttonDebounce(0, 0, 1, 400)) { // enters menu options
    setOptions();
    lastMinute = 60;// so time will be updated
  }

  if (buttonDebounce(1, 1, 0, 700)) { // enters game mode
    strip.clear();
    gameSelection();
  }

  while (enterGame) { // end game or continue

    if (endGame) {
      endGame = false;
      enterGame = false;
    } else {
      gameMusic();
      strip.clear();
      switch (gameSelect) {
        case 0:
          gameSpeed =  pgm_read_word_near (enemySpeed + 0);
          levelTitle ();
          gameLedInvaders();
          break;
        case 1:
          gameLedTris();
          break;
        case 2:
          gameFlappyLed();
          break;
        case 3:
          levelTitle ();
          gameLedKanoid();
          break;
      }
    }
  }
}

void clockAlwaysOn (void) {
  encoder.encoderSetPosition(5, 2);
  set_sleep_mode(SLEEP_MODE_IDLE);// configure sleep mode
  uint8_t lastRotaryCounter = 0;

  while (LEFTLOW || RIGHTLOW);
  while (LEFTHIGH && RIGHTHIGH) {
    battStatus();
    showclock();
    uint8_t actualValue =  encoder.encoderRead(2, 25, false, 2);

    while (actualValue != lastRotaryCounter) {
      strip.setBrightness(actualValue);// brightness select from 0 to 255
      lastRotaryCounter = actualValue;
      lastMinute = 60;
    }
    strip.setPixelColor(61, red, green, blue);
    strip.setPixelColor(81, red, green, blue);
    sleep_enable();
    sleep_cpu();// go to sleep
  }
  strip.clear();
  strip.setBrightness(6);// brightness select from 0 to 255
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);// configure sleep mode

  while (LEFTLOW || RIGHTLOW);
  timer = millis();
  lastMinute = 60;
}

//game selection menu
void gameSelection() {
  strip.clear();
  enterGame = true;
  bool gameSelection = true;

  while (RIGHTLOW && LEFTLOW); //prevents immediately enter a game

  while (gameSelection) {

    if (buttonDebounce(0, 1, 0, 500)) {

      if (gameSelect++, gameSelect > 4) gameSelect = 0;
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 500)) {

      if (gameSelect--, gameSelect < 0) gameSelect = 4;
      strip.clear();
    }

    switch (gameSelect) { // select game ad scroll game title
      case 0:
        drawCharacter(50, arrowLeft, 100, 0, 0);
        drawCharacter(57, arrowRight, 100, 0, 0);
        strip.setPixelColor(73, 0, 0, 100);

        for (uint8_t x = 0; x < 5 && RIGHTHIGH && LEFTHIGH; x++) {
          switch (x) {
            case 1:
              scrollPixels(ledInvadersTitle1, 0, 100, 0);
              break;
            case 2:
              scrollPixels(ledInvadersTitle2, 0, 100, 0);
              break;
            case 3:
              scrollPixels(ledInvadersTitle3, 0, 100, 0);
              break;
            case 4:
              scrollPixels(ledInvadersTitle4, 0, 100, 0);
              break;
          }
        }
        break;
      case 1:
        drawCharacter(50, arrowLeft, 100, 0, 0);
        drawCharacter(57, arrowRight, 100, 0, 0);
        strip.setPixelColor(74, 0, 0, 100);

        for (uint8_t x = 0; x < 4 && RIGHTHIGH && LEFTHIGH; x++) {
          switch (x) {
            case 1:
              scrollPixels(ledInvadersTitle1, 50, 80, 0);
              break;
            case 2:
              scrollPixels(ledTrisTitle2, 50, 80, 0);
              break;
            case 3:
              scrollPixels(ledTrisTitle3, 50, 80, 0);
              break;
          }
        }
        break;
      case 2:
        drawCharacter(50, arrowLeft, 100, 0, 0);
        drawCharacter(57, arrowRight, 100, 0, 0);
        strip.setPixelColor(75, 0, 0, 100);

        for (uint8_t x = 0; x < 4 && RIGHTHIGH && LEFTHIGH; x++) {
          switch (x) {
            case 1:
              scrollPixels(flappyLedTitle1, 0, 100, 50);
              break;
            case 2:
              scrollPixels(flappyLedTitle2, 0, 100, 50);
              break;
            case 3:
              scrollPixels(ledInvadersTitle1, 0, 100, 50);
              break;
          }
        }
        break;
      case 3:
        drawCharacter(50, arrowLeft, 100, 0, 0);
        drawCharacter(57, arrowRight, 100, 0, 0);
        strip.setPixelColor(76, 0, 0, 100);

        for (uint8_t x = 0; x < 4 && RIGHTHIGH && LEFTHIGH; x++) {
          switch (x) {
            case 1:
              scrollPixels(ledInvadersTitle1, 0, 50, 80);
              break;
            case 2:
              scrollPixels(ledKanoidTitle1, 0, 50, 80);
              break;
            case 3:
              scrollPixels(ledKanoidTitle2, 0, 50, 80);
              break;
          }
        }

        if (SLEEPMENULOW || (LEFTLOW && RIGHTLOW)) { // ledkanoid number of players select.
          encoder.encoderSetPosition(1, 1);
          strip.clear();
          while (RIGHTLOW && LEFTLOW); //prevents immediately enter a game

          while (RIGHTHIGH || LEFTHIGH) {
            uint8_t actualValue = encoder.encoderRead(1, 2, true, 1);
            arrayNumberValue[0] = pgm_read_word_near(number + actualValue);
            drawCharacter(50, arrayNumberValue[0], 0, 0, 100);
            drawCharacter(0, letterS, 100, 0, 0);
            drawCharacter(3, letterE, 100, 0, 0);
            drawCharacter(7, letterL, 100, 0, 0);
            drawCharacter(54, letterP, 100, 0, 0);

            if (LedKanoidNumberOfplayers != actualValue) {
              strip.clear();
              beep(50, 50);
              LedKanoidNumberOfplayers = actualValue;
            }
          }
          gameSelection = false;
        }
        break;
      case 4:
        drawCharacter(50, arrowLeft, 100, 0, 0);
        drawCharacter(57, arrowRight, 100, 0, 0);
        drawCharacter(0, letterE, 100, 0, 0);
        drawCharacter(3, letterN, 100, 0, 0);
        drawCharacter(6, letterD, 100, 0, 0);

        if (SLEEPMENULOW || (LEFTLOW && RIGHTLOW)) {
          enterGame = false;
          lastMinute = 60;
          timer = millis();
        }
        break;
    }
    if (SLEEPMENULOW || (LEFTLOW && RIGHTLOW)) gameSelection = false;
  }
}

void levelTitle () {// level description
  strip.clear();
  while (RIGHTLOW || LEFTLOW); // button debounce
  red = 100;
  green = 0;
  blue = 0;
  showTime(51, gameLevel);

  for (uint8_t x = 0; x < 3; x++) {
    switch (x) {
      case 1:
        scrollPixels(levelText1, 0, 0, 100);
        break;
      case 2:
        scrollPixels(levelText2, 0, 0, 100);
        break;
    }
  }
  strip.clear();
}

//Game ledKanoid code
void gameLedKanoid() {
  uint8_t arrayElements = 24; // reads enemy position
  uint8_t actualBlocks [arrayElements];// because enemies array cointains 1 extra element with number of elements information
  uint8_t blocksDestroyed [arrayElements];// another array that contains only enemies killed
  uint8_t blocksRemaining = arrayElements;
  uint8_t playerOnePos = 94;
  uint8_t lastPlayerOnePos = 0;
  uint8_t playerTwoPos = 4;
  uint8_t lastPlayerTwoPos = 0;
  uint8_t ballPosition = 84;
  uint8_t ledKanoidLives = 1;
  uint8_t blocksPosition = 0;
  uint16_t ballTimer = 0;
  uint16_t ballSpeed = 800;
  uint16_t playerOneScore = 0;
  uint16_t playerTwoScore = 0;
  int ballEffect = 1;
  bool ballMovement = false;
  bool ballDirectionUp = true;
  bool colitionCheck = true;
  bool ballOnPlatform = false;

  if ( LedKanoidNumberOfplayers == 2) {
    blocksPosition = 20;
  }

  for (uint8_t x = 0; x < arrayElements; x++) {
    actualBlocks[x] = pgm_read_byte_near (blocks + (x + 1)); // first enemy draw on Leds
    strip.setPixelColor((actualBlocks[x] + blocksPosition), 100, 0, 0);// led enemy position musn´t be less than 0
    blocksDestroyed [x] = 99; // fill enemiesKilled array with a number never used some times memory stills holds a value that shouldn´t be there
  }

  if (gameLevel < 30 && LedKanoidNumberOfplayers == 1) {
    ballSpeed = 812 - (gameLevel * 12);
  } else {
    ballSpeed = 600;
  }
  encoder.encoderSetPosition (playerOnePos, 2);

  while (ledKanoidLives != 0 && blocksRemaining != 0) {

    if (interruptTimer) ballTimer += interruptTimer;// keeps time based on TCB0 interrupt

    playerOnePos = encoder.encoderRead(90, 97, false, 2);

    if (playerOnePos != lastPlayerOnePos) {
      drawCharacter(lastPlayerOnePos, playerPlatform, 0, 0, 0);

      if (!ballMovement || ballOnPlatform) {
        strip.setPixelColor(ballPosition, 0 , 0 , 0);
        ballPosition = (encoder.encoderRead(90, 97, false, 2) - 9);
        strip.setPixelColor(ballPosition, 80 , 80 , 0);
      }
      lastPlayerOnePos = playerOnePos;
    } else {
      drawCharacter(playerOnePos, playerPlatform, 0, 100, 0);
    }

    if (LedKanoidNumberOfplayers == 2) {

      if (buttonDebounce(1, 0, 0, 50) && playerTwoPos > 0) {
        playerTwoPos--;
      }

      if (buttonDebounce(0, 1, 0, 50) && playerTwoPos < 7) {
        playerTwoPos++;
      }

      if (playerTwoPos != lastPlayerTwoPos) {
        drawCharacter(lastPlayerTwoPos, playerPlatform, 0, 0, 0);
      } else {
        drawCharacter(playerTwoPos, playerPlatform, 0, 0, 100);
      }
      lastPlayerTwoPos = playerTwoPos;
    }

    if (LEFTLOW || RIGHTLOW  && ballOnPlatform) {
      ballMovement = true;
      ballOnPlatform = false;
    }

    if ((ballTimer >= ballSpeed) && ballMovement) {
      ballTimer = 0;
      colitionCheck = true;
      ballOnPlatform = false;
      strip.setPixelColor(ballPosition, 0 , 0 , 0);

      if (ballDirectionUp & ballPosition > 9) {
        ballPosition -= 10 + ballEffect;
      } else {
        ballPosition += 10 - ballEffect;
      }
      strip.setPixelColor(ballPosition, 80 , 80 , 0);

      for (uint8_t x = 0; x < arrayElements && colitionCheck; x++) {// if enemies are killed another array is created containing only killed enemies
        if (ballPosition  == (actualBlocks[x] + blocksPosition) && blocksDestroyed [x] != x) {
          blocksDestroyed [x] = x;
          blocksRemaining--;
          ledKanoidScore ++;
          ballDirectionUp = !ballDirectionUp;
          colitionCheck = false;
          beep(50, 150);
          strip.setPixelColor((actualBlocks[x] + blocksPosition), 0, 0, 0);
        }
      }
    }

    for (uint8_t x = 0; x < 10 && colitionCheck; x++) {

      if ( ballPosition == pgm_read_byte_near (screenRightBounds + x)) {
        ballEffect = 1;
        colitionCheck = false;
        beep(50, 50);
      }

      if ( ballPosition == pgm_read_byte_near (screenLeftBounds + x)) {
        ballEffect = -1;
        colitionCheck = false;
        beep(50, 50);
      }
    }

    if (ballPosition < 10 && LedKanoidNumberOfplayers == 1) {
      ballDirectionUp =  false;
    } else if (ballPosition < 10) {
      playerOneScore += ledKanoidScore;
      ledKanoidLives = 0;
    }

    if (ballPosition > 89) {
      if (LedKanoidNumberOfplayers == 2) {
        playerTwoScore += ledKanoidScore;
      }
      ledKanoidLives = 0;
    }

    for (uint8_t x = 10; x > 7 && !ballOnPlatform; x--) { // checks if ball touched player 1 platform and effect to the ball

      if (ballPosition == (playerOnePos - x)) {
        ballDirectionUp = true;
        switch (x) {
          case (10):

            if (ballPosition == 80) {
              ballEffect = -1;
            } else {
              ballEffect = 1;
            }
            break;
          case (8):

            if (ballPosition == 89) {
              ballEffect = 1;
            } else {
              ballEffect = -1;
            }
            break;
        }
        ballTimer = 250;
        ballOnPlatform = true;
      }
    }

    for (uint8_t x = 10; x < 13 && !ballOnPlatform && LedKanoidNumberOfplayers == 2; x++) { // checks if ball touched player 2 platform and effect to the ball

      if (ballPosition == (playerTwoPos + x)) {
        ballDirectionUp = false;
        switch (x) {
          case (10):

            if (ballPosition == 10) {
              ballEffect = -1;
            } else {
              ballEffect = 1;
            }
            break;
          case (12):

            if (ballPosition == 19) {
              ballEffect = 1;
            } else {
              ballEffect = -1;
            }
            break;
        }
        ballTimer = 250;
        ballOnPlatform = true;
      }
    }

    if (buttonDebounce(0, 0, 1, 200)) {//Pause menu
      gameMenu ();
      for (uint8_t x = 0; x < arrayElements; x++) {
        if (x != blocksDestroyed [x])  strip.setPixelColor(actualBlocks[x], 100, 0, 0);// led enemy position musn´t be less than 0
      }
    }

    if (endGame)  break;
  }
  strip.clear();
  timer = millis();

  if (blocksRemaining == 0) {// go to next level
    gameLevel++;

    if (gameLevel > 99) {// for now  game continues only to level 99
      drawCharacter(20, letterW, 100, 0, 0);
      drawCharacter(23, letterI, 100, 0, 0);
      drawCharacter(26, letterN, 100, 0, 0);
      delay(2000);
      contGame = false;
    }
  }

  if (ledKanoidLives == 0) gameLevel = 1;// Game over

  if (ledKanoidScore > 999) ledKanoidScore = 999;

  if ( LedKanoidNumberOfplayers == 2) {
    drawCharacter(0, letterP, 100, 0, 0);

    if (playerOneScore > playerTwoScore) {
      drawCharacter(3, number1, 0, 0, 100);
      showScore(playerOneScore);
    } else {
      drawCharacter(3, number2, 0, 0, 100);
      showScore(playerTwoScore);
    }
    delay(600);
  } else if (ledKanoidScore > readScoreFromEEPROM(11)) {
    drawCharacter(0, letterN, 100, 0, 0);
    drawCharacter(3, letterE, 100, 0, 0);
    drawCharacter(6, letterW, 100, 0, 0);
    showScore(ledKanoidScore);
    writeScoreToEEPROM(11, ledKanoidScore);
  } else {
    drawCharacter(0, letterH, 100, 0, 0);
    drawCharacter(3, letterI, 100, 0, 0);
    showScore(readScoreFromEEPROM(11));
  }

  while (!buttonDebounce(1, 1, 0, 100)) {

    if (buttonDebounce(0, 0, 1, 2000)) {
      strip.clear();
      writeScoreToEEPROM(11, 0);
      drawCharacter(0, letterC, 100, 0, 0);
      drawCharacter(3, letterL, 100, 0, 0);
      drawCharacter(6, letterR, 100, 0, 0);
    }

    if ((millis() - timer) > 30000) {// if no button is pressed in 30sec ends game
      contGame = false;
      endGame = true;
      break;
    }
  }
  ledKanoidScore = 0;

  if (contGame) {
    levelTitle();
    gameLedKanoid();
  } else {
    contGame = true;
    strip.clear();
    timer = millis();
    lastMinute = 60;
  }
}

//Game FlappyLed code

void gameFlappyLed() {
  uint8_t randomWall = 0;
  uint8_t flappyLedLives = 1;
  uint8_t flappyLedIput = 0;
  uint8_t flappyLedPos = 53;
  uint8_t lastflappyLedPos = 0;
  uint8_t wallPos = 9;
  uint8_t passage1;
  uint8_t passage2;
  uint16_t flappyLedTimer = 0;
  uint16_t wallTimer = 0;
  uint16_t gameSpeed = 1800;
  uint16_t flappyLedscore = 0;

  delay(1000);

  while (flappyLedLives != 0) {

    arrayNumberValue[0] = pgm_read_word_near(walls + randomWall);

    if (interruptTimer) {
      flappyLedTimer += interruptTimer;
      wallTimer += interruptTimer;
    }

    if (flappyLedPos > 3 && buttonDebounce(1, 1, 0, 80)) {
      flappyLedPos -= 10;
      beep(22, 22);
    }

    strip.setPixelColor(flappyLedPos, 0, 100, 0);

    if (flappyLedPos != lastflappyLedPos) {
      strip.setPixelColor(lastflappyLedPos, 0, 0, 0);
    }
    lastflappyLedPos = flappyLedPos;

    if (flappyLedTimer >= 3000 && flappyLedPos < 93) {
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
        randomWall = random(0 , 5);
      } else {
        wallPos --;

        if (wallPos == 2) {
          if (flappyLedscore <= 999) {
            flappyLedscore += 2;
          }
        }
      }
      if (gameSpeed > 120) gameSpeed--;
    }

    passage1 = pgm_read_byte_near (arrayNumberValue[0] + 9);
    passage2 = pgm_read_byte_near (arrayNumberValue[0] + 10);

    if (wallPos == 3 && (flappyLedPos < passage1 || flappyLedPos > (passage2 + 14))) {
      flappyLedLives = 0;
      for (uint16_t x = 0; x < 600; x += 100) {// game music
        beep(200, x);
      }
    }

    if (buttonDebounce(0, 0, 1, 200)) gameMenu ();//Pause menu

    if (endGame) break;
  }
  timer = millis();
  strip.clear();

  if (!endGame) {

    if (flappyLedscore > readScoreFromEEPROM(9)) {
      drawCharacter(0, letterN, 100, 0, 0);
      drawCharacter(3, letterE, 100, 0, 0);
      drawCharacter(6, letterW, 100, 0, 0);
      showScore(flappyLedscore);
      writeScoreToEEPROM(9, flappyLedscore);
    } else {
      drawCharacter(0, letterH, 100, 0, 0);
      drawCharacter(3, letterI, 100, 0, 0);
      showScore(readScoreFromEEPROM(9));
    }
    while (!buttonDebounce(1, 1, 0, 100)) {

      if (buttonDebounce(0, 0, 1, 2000)) {
        strip.clear();
        writeScoreToEEPROM(9, 0);
        drawCharacter(0, letterC, 100, 0, 0);
        drawCharacter(3, letterL, 100, 0, 0);
        drawCharacter(6, letterR, 100, 0, 0);
      }

      if ((millis() - timer) > 30000) {// if no button is pressed in 30sec ends game
        contGame = false;
        endGame = true;
        break;
      }
    }
  }

  if (contGame) {
    strip.clear();
    gameFlappyLed();
  } else {
    flappyLedscore = 0;
    contGame = true;
    strip.clear();
    timer = millis();
    lastMinute = 60;
  }
}

// Game LedTris code
void gameLedTris() {
  uint8_t tetrominoRotation;
  uint8_t tetrominoLastRotation = 0;
  uint8_t randomTetromino = 0;
  uint8_t ledtrisLives = 1;// if a tetromino cant`t go lower than row 2, is game over
  uint16_t actualTetromino[1] = {tetromino1A};// Load tetromino from "character.h"
  uint16_t lastTetromino[1] = {tetromino1A};// Last tetromino used
  uint16_t tetrominoInPos [10][10]; // variable to compare if the screen position has a tetromino
  uint16_t tetrominoTimer = 0;// overal timer used todrive the game
  uint16_t ledTrisSpeed = 400;
  uint16_t actualLedTrisSpeed = 400;
  uint16_t ledTrisScore = 0;
  int tetrominoLastPos = -6;
  int tetrominoPos = -6;
  bool tetris = false; // if a line is completed
  bool tetrominoMoveRight = true; // checks if moving is allowed
  bool tetrominoMoveLeft = true;
  bool tetrominoStop = false;// if the piece touch nother or the bottom sceen
  bool moveTetrominos = false;// once a line is complete allow remaining tetrominos to get down

  //fills all the tetrominos pos with 0.
  for ( uint8_t row = 0; row < 10; row++ ) {
    for ( uint8_t column = 0; column < 10; column++ ) {
      tetrominoInPos [row][column] = 0;
    }
  }

  //creates the left and right walls
  for (uint8_t x = 0; x < 10; x++) {
    tetrominoInPos [x][0] = pgm_read_byte_near (wallLeft + x);
    tetrominoInPos [x][9] = pgm_read_byte_near (wallRight + x);
    strip.setPixelColor(tetrominoInPos [x][0], 0, 0, 50);
    strip.setPixelColor(tetrominoInPos [x][9], 0, 0, 50);
  }

  // Game code
  while (ledtrisLives > 0 && !endGame) {

    tetrominoRotation = encoder.encoderRead(0, 3, true, 1);
    actualTetromino[0] = tetromino [randomTetromino][tetrominoRotation];//here the actual plyer controlled tetromino is created

    if (tetrominoRotation != tetrominoLastRotation && sound) beep(40, 40);

    // Advace time all the game speed is cotolled here
    if (interruptTimer) tetrominoTimer += interruptTimer;

    //only move down the tetromino if there isn`t any obstacle
    if (tetrominoTimer >= ledTrisSpeed && tetrominoPos < 99 && !tetrominoStop) {
      tetrominoTimer = 0;
      tetrominoPos += 10;
    }
    //we asume that the tetromino can`t move
    tetrominoStop = false;
    tetrominoMoveRight = true;
    tetrominoMoveLeft = true;

    //All the math to allow or not the player to move the tetromino
    for (uint8_t x = 1; x < pgm_read_byte_near (actualTetromino[0] + 0); x++) {// reads all the leds forming the tetromino position

      for ( uint8_t row = 0; row < 10; row++ ) {
        for ( uint8_t column = 0; column < 10; column++ ) {

          // allow or not to move left and right
          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos - 1)) == tetrominoInPos [row][column]) tetrominoMoveLeft = false;

          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos + 1)) == tetrominoInPos [row][column]) tetrominoMoveRight = false;

          if (tetrominoRotation != tetrominoLastRotation) {// only if rotation happen the tetromino is checked and positioned in a valid place

            if (!tetrominoMoveRight) {
              if ((pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) == tetrominoInPos [row][column]) {
                tetrominoPos --;
              }
            }

            if (!tetrominoMoveLeft) {
              if ((pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) == tetrominoInPos [row][column]) tetrominoPos++;
            }
          } else {
            // checks if the tetromino touched another or the bottom floor

            if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos + 10)) == tetrominoInPos [row][column] || (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) > 89) {
              tetrominoStop = true;

              if ((pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) < 9) {//  if its stops on the top row game is over
                ledtrisLives = 0;
                break;
              }
            }
          }
        }
      }
    }

    if (ledtrisLives == 0) { //stops the while loop
      break;
    }

    //The tetromino3B looks odd if not moved one place when rotated
    if (tetrominoRotation != tetrominoLastRotation && actualTetromino[0] == tetromino3B && tetrominoMoveLeft) tetrominoPos--;

    if (tetrominoRotation != tetrominoLastRotation && actualTetromino[0] == tetromino3A && tetrominoMoveRight) tetrominoPos++;

    //Here it draws the screen
    if (tetrominoPos != tetrominoLastPos || tetrominoRotation != tetrominoLastRotation) {
      drawCharacter(tetrominoLastPos, lastTetromino[0], 0, 0, 0);
      tetrominoLastPos = tetrominoPos;
      tetrominoLastRotation = tetrominoRotation;
      lastTetromino[0] = actualTetromino[0];
      drawCharacter(tetrominoPos, actualTetromino[0], 0, 50, 0);
    }

    if (tetrominoStop) {//All calcultions done when a tetromino stops moving

      if (RIGHTHIGH && LEFTHIGH || tetrominoTimer > 400) {//Some conditions have to be met before stoping so some time is given to last moment moves
        beep(50, 150);
        for (uint8_t x = 1; x < pgm_read_byte_near (actualTetromino[0] + 0); x++) {// reads all tetromino leds position
          uint8_t valueDecimal = (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos) / 10 ;// takes the decimal of the position for example: from 91 takes the 9
          uint8_t valueUnits = pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos;// same as above but takes the units

          for (uint8_t y = 0; y < (valueDecimal * 10) ; y++) {// here actually get the units
            valueUnits--;
          }
          tetrominoInPos [valueDecimal][valueUnits] = (pgm_read_byte_near (actualTetromino[0] + x) + tetrominoPos);// stores the tetromino position in an array to know the position cant`t be used
          strip.setPixelColor(tetrominoInPos [valueDecimal][valueUnits], 50, 0, 0);
        }

        //If a line is complete the tetrominos above it are moved one space dow, the check is done 3 times because the tetrominos max height is 3 leds so the player can complete 3 lines at once
        for ( uint8_t repeatTetrisCheck = 0; repeatTetrisCheck < 3; repeatTetrisCheck++ ) {

          for ( uint8_t row = 9; row > 0; row-- ) {
            tetris = true;

            for ( uint8_t column = 1; column < 9; column++ ) {

              if (moveTetrominos && tetrominoInPos [row][column] != 0) {
                tetrominoInPos [(row + 1)][column] = (tetrominoInPos [row][column] + 10);
                strip.setPixelColor(tetrominoInPos [row][column], 0, 0, 0);
                beep(80, 80);
                strip.setPixelColor(tetrominoInPos [(row + 1)][column], 0, 0, 50);
                tetrominoInPos [row][column] = 0;
              }
              if (tetrominoInPos [row][column] == 0)   tetris = false;// if there is nothig on the position it stops because the line isnt`t complete
            }

            if (tetris) {
              moveTetrominos = true;

              if (actualLedTrisSpeed > 200) actualLedTrisSpeed -= 10; // make game faster

              if (repeatTetrisCheck < 3) { // if 3 rows are completed, more points
                ledTrisScore ++;
              } else {
                ledTrisScore += 4;
              }
              for ( uint8_t tetrisColumn = 1; tetrisColumn < 9; tetrisColumn++ ) {
                strip.setPixelColor(tetrominoInPos [row][tetrisColumn], 0, 0, 0);
                tetrominoInPos [row][tetrisColumn] = 0;
              }
            }
          }

          moveTetrominos = false;
        }

        // the new tetromino is generated and positioned
        tetrominoPos = -6;
        randomTetromino = random(5);
        tetrominoLastPos = tetrominoPos;
        tetrominoLastRotation = tetrominoRotation;
      }
    }

    //Left or right moves
    if (tetrominoPos > 0) {

      if (RIGHTLOW && LEFTLOW) {
        ledTrisSpeed = 60;
      } else {
        ledTrisSpeed = actualLedTrisSpeed;
      }

      if (buttonDebounce(1, 0, 0, 120) && RIGHTHIGH && tetrominoMoveLeft)  tetrominoPos--;

      if (buttonDebounce(0, 1, 0, 120) && LEFTHIGH && tetrominoMoveRight) tetrominoPos++;
    }

    if (buttonDebounce(0, 0, 1, 200)) {//Pause menu
      gameMenu ();
      for ( uint8_t row = 0; row < 10; row++ ) {
        for ( uint8_t column = 0; column < 10; column++ ) {
          strip.setPixelColor (tetrominoInPos[row][column], 0, 0, 50);
        }
      }
    }
  }

  if (!endGame) {
    for (uint8_t x = 99; x > 0; x--) {// end screen animation
      strip.setPixelColor(x, 50, 0, 0);
      delay(5);
    }

    timer = millis();
    strip.clear();

    if (ledTrisScore > 999) ledTrisScore = 999;

    if (ledTrisScore > readScoreFromEEPROM(7)) {
      drawCharacter(0, letterN, 100, 0, 0);
      drawCharacter(3, letterE, 100, 0, 0);
      drawCharacter(6, letterW, 100, 0, 0);
      showScore(ledTrisScore);
      writeScoreToEEPROM(7, ledTrisScore);
    } else {
      drawCharacter(0, letterH, 100, 0, 0);
      drawCharacter(3, letterI, 100, 0, 0);
      showScore(readScoreFromEEPROM(7));
    }
    while (!buttonDebounce(1, 1, 0, 100)) {

      if (buttonDebounce(0, 0, 1, 2000)) {
        strip.clear();
        writeScoreToEEPROM(7, 0);
        drawCharacter(0, letterC, 100, 0, 0);
        drawCharacter(3, letterL, 100, 0, 0);
        drawCharacter(6, letterR, 100, 0, 0);
      }

      if ((millis() - timer) > 30000) {// if no button is pressed in 30sec ends game
        contGame = false;
        endGame = true;
        break;
      }
    }
  }
  strip.clear();

  if (contGame) {
    gameLedTris();
  } else {
    contGame = true;
    timer = millis();
    lastMinute = 60;
    ledTrisScore = 0;
  }
}

// LedInvaders Game code
void gameLedInvaders () {
  uint8_t playerPos = 84;
  uint8_t lastEnemyPos = 10;
  uint8_t lastPlayerPos = 0;
  uint8_t playerShotPosition = 99;
  uint8_t arrayElements = pgm_read_byte_near (enemies[enemiesLevel] + 0) - 1; // reads enemy position
  int actualEnemies [arrayElements];// because enemies array cointains 1 extra element with number of elements information
  uint8_t enemiesKilled [arrayElements];// another array that contains only enemies killed
  uint8_t enemiesRemaining = arrayElements;
  uint8_t setEnemyFire = 0;
  uint8_t closestEnemy = 0;
  uint8_t enemiesheightTimer = 0;
  uint8_t enemyShotPosition = 90;
  uint16_t enemiesMoveTimer = 0;
  uint16_t enemyShotTimer = 0;
  uint16_t motherShipTimer = 0;
  uint16_t playerShotTimer = 0;
  int motherShipMovement = 9; // int because working with negative numbers
  int enemyPos = 10; // int because working with negative numbers
  bool callMotherShip = false;
  bool playerShot = false;
  bool allowRightMove = true;
  enemiesKilled [0] = 100; // default 0 value will be used later, so is changed to a value never used.

  for (uint8_t x = 0; x < arrayElements; x++) {
    actualEnemies[x] = pgm_read_byte_near (enemies[enemiesLevel] + (x + 1)); // first enemy draw on Leds
    enemiesKilled [x] = 99; // fill enemiesKilled array with a number never used some times memory stills holds a value that shouldn´t be there
  }

  while (enemiesRemaining > 0 && ledInvaderLives > 0) {

    if (interruptTimer) {// keeps time based on TCB0 interrupt
      enemiesMoveTimer += interruptTimer;
      enemyShotTimer += interruptTimer;
      motherShipTimer += interruptTimer;
      playerShotTimer += interruptTimer;
    }

    for (uint8_t x = 0; x < arrayElements; x++) {

      if (enemiesKilled [x] != x) {

        if (enemyPos != lastEnemyPos) {
          strip.setPixelColor((actualEnemies[x] + lastEnemyPos), 0, 0, 0); // clear last enemy position
        }

        strip.setPixelColor((actualEnemies[x] + enemyPos), 100, 0, 0);// led enemy position musn´t be less than 0strip.setPixelColor((actualEnemies[x] + enemyPos), 100, 0, 0);// led enemy position musn´t be less than 0

        for (uint8_t y = 0; y < 9  ; y++) {// code to check bounds of enemies

          if (pgm_read_byte_near (screenRightBounds + y) == (actualEnemies[x] + enemyPos)) allowRightMove = false;

          if (pgm_read_byte_near (screenLeftBounds + y) == (actualEnemies[x] + enemyPos))  allowRightMove = true;
        }

        if (random(0 , 300) == 150 && enemyShotPosition > 99) enemyShotPosition = (actualEnemies[x] + enemyPos);

        if ((actualEnemies[x] + enemyPos) > closestEnemy)  closestEnemy = (actualEnemies[x] + enemyPos);// some math to know  enemy positon closest to player.
      }
    }

    // player position
    playerPos = encoder.encoderRead(80, 87, false, 2);

    if (playerPos != lastPlayerPos) {
      drawCharacter(lastPlayerPos, playerShip, 0, 0, 0);
      lastPlayerPos = playerPos;
    }

    if (ledInvaderLives > 1 ) {// player ship color change depending on lives left
      drawCharacter(playerPos, playerShip, 0, 100, 0);
    } else {
      drawCharacter(playerPos, playerShip, 100, 100, 0);
    }

    if (buttonDebounce (1, 1, 0, 0) && !playerShot) {// generates player shot
      beep(40, 40);
      playerShot = true;
      playerShotPosition = (playerPos + 1);
      if ( playerShotPosition < 80) {
        strip.setPixelColor(playerShotPosition, 0 , 0 , 100);
      }
    }

    if ((playerShotTimer >= 150) && playerShot) {//player shot check with enemies and mother ship
      playerShotTimer = 0;
      strip.setPixelColor(playerShotPosition, 0 , 0 , 0);

      if (playerShotPosition < 10) {
        playerShot = false;
      } else {
        playerShotPosition -= 10;
      }

      for (uint8_t x = 0; x < arrayElements; x++) {// if enemies are killed another array is created containing only killed enemies
        if (playerShotPosition == (actualEnemies[x] + enemyPos) && enemiesKilled [x] != x) {
          enemiesKilled [x] = x;
          playerShot = false;
          enemiesRemaining--;
          ledInvadersScore += 1;
          strip.setPixelColor((actualEnemies[x] + enemyPos), 0, 0, 0);
        }
      }

      if (playerShotPosition == motherShipMovement || playerShotPosition == (motherShipMovement + 1)) {// mother ship hit detection
        motherShipMovement = 9;
        callMotherShip = false;
        ledInvaderLives = 2;
        ledInvadersScore += 3;
        for (uint8_t x = 0; x < 9; x++) {
          strip.setPixelColor(x, 0 , 0 , 0);
        }
        beep(50, 150);
      }

      if (playerShot) {
        strip.setPixelColor(playerShotPosition, 0 , 0 , 100);
      } else {
        playerShotPosition = (playerPos + 1);
      }
    }

    if (enemyShotTimer >= 250 && closestEnemy < 79 && enemyShotPosition < 100) {// enemiees fire code
      enemyShotTimer = 0;
      strip.setPixelColor(enemyShotPosition, 0 , 0 , 0);
      enemyShotPosition += 10;
      strip.setPixelColor(enemyShotPosition, 100 , 70 , 0);
    }

    if (enemyShotPosition == playerPos || enemyShotPosition == (playerPos + 1) || enemyShotPosition == (playerPos + 2) || closestEnemy > 79) {

      if (closestEnemy > 79) {// if the enemies enters player position END game
        ledInvaderLives = 0;
      } else {
        ledInvaderLives --;
      }

      strip.setPixelColor(enemyShotPosition, 0 , 0 , 0);
      enemyShotPosition = 100;

      if (ledInvaderLives > 0) {
        beep(50, 150);
      } else {
        for (uint16_t x = 0; x < 600; x += 100) {// game music
          beep(100, x);
          drawCharacter(playerPos, playerShip, 0, 0, 0);
          drawCharacter(playerPos, playerShipDead, 0, 100, 0);
          beep(100, x);
          drawCharacter(playerPos, playerShipDead, 0, 0, 0);
          drawCharacter(playerPos, playerShip, 100, 100, 0);
        }
      }
    }

    //mother ship movement
    if (random(0 , 2000) == 500)  callMotherShip = true;

    if ((motherShipTimer >= 200) && callMotherShip) {
      motherShipTimer = 0;
      if (motherShipMovement < 0) {
        strip.setPixelColor((motherShipMovement + 1), 0 , 0 , 0);
        motherShipMovement = 9;
        callMotherShip = false;
      } else {
        motherShipMovement--;
        strip.setPixelColor(motherShipMovement, 70 , 0 , 70);
        if (motherShipMovement < 8) {
          strip.setPixelColor((motherShipMovement + 2), 0 , 0 , 0);
        }
      }
    }

    //enemies movement

    if (enemiesRemaining < 4)  gameSpeed = pgm_read_word_near (enemySpeed + enemiesRemaining);// reads enemies speed from array "enemySpeed" element 0 is reserved for Hard mode, not used for now

    if (enemiesMoveTimer > gameSpeed) {
      enemiesMoveTimer = 0;
      lastEnemyPos = enemyPos;
      enemiesheightTimer++;

      beep(25, 25);

      if (allowRightMove) {// cheked before in screenRightBounds and screenLeftBounds
        enemyPos++;
      } else {
        enemyPos--;
      }

      if (enemiesheightTimer == 22) { // allow enemies to get closer to player
        enemiesheightTimer = 0;
        enemyPos += 10;
      }
    }

    if (buttonDebounce(0, 0, 1, 200))   gameMenu();     //enemiesRemaining = 0;// uncoment to cheat and advance level with one button ;)

    if (endGame)  break;

    if (ledInvadersScore > 999) {
      ledInvadersScore = 999;
    }
  }
  strip.clear();
  timer = millis();

  if (enemiesRemaining == 0) {// go to next level
    gameLevel++;
    enemiesLevel++;

    if (enemiesLevel > 8)   enemiesLevel = 0;

    if (gameLevel < 15) {// speeds up level up to level 15 then speeds is the same
      gameSpeed = pgm_read_word_near (enemySpeed + 0) - (gameLevel * 15);
    } else {
      gameSpeed = pgm_read_word_near (enemySpeed + 0) - 250;
    }

    if (gameLevel > 99) {// for now  game continues only to level 99
      drawCharacter(20, letterW, 100, 0, 0);
      drawCharacter(23, letterI, 100, 0, 0);
      drawCharacter(26, letterN, 100, 0, 0);
      delay(2000);
      endGame = true;
    }
  }

  if (ledInvaderLives == 0) {// Game over
    gameSpeed =  pgm_read_word_near (enemySpeed + 0);
    enemiesLevel = 0;
    ledInvaderLives = 2;
    gameLevel = 1;

    if (ledInvadersScore > 999) ledInvadersScore = 999;

    if (ledInvadersScore > readScoreFromEEPROM(5)) {
      drawCharacter(0, letterN, 100, 0, 0);
      drawCharacter(3, letterE, 100, 0, 0);
      drawCharacter(6, letterW, 100, 0, 0);
      showScore(ledInvadersScore);
      writeScoreToEEPROM(5, ledInvadersScore);
    } else {
      drawCharacter(0, letterH, 100, 0, 0);
      drawCharacter(3, letterI, 100, 0, 0);
      showScore(readScoreFromEEPROM(5));
    }
    while (!buttonDebounce(1, 1, 0, 100)) {

      if (buttonDebounce(0, 0, 1, 2000)) {
        strip.clear();
        writeScoreToEEPROM(5, 0);
        drawCharacter(0, letterC, 100, 0, 0);
        drawCharacter(3, letterL, 100, 0, 0);
        drawCharacter(6, letterR, 100, 0, 0);
      }

      if ((millis() - timer) > 30000) {// if no button is pressed in 30sec ends game
        contGame = false;
        endGame = true;
        break;
      }
    }
    ledInvadersScore = 0;
  }

  if (contGame) {
    levelTitle();
    gameLedInvaders();
  } else {
    contGame = true;
    strip.clear();
    timer = millis();
    lastMinute = 60;
  }
}

void gameMenu () {// menu only available in game mode
  strip.clear();
  int8_t option = 0;
  uint8_t actualValue = 0;
  encoder.encoderSetPosition(sound, 1);

  while (SLEEPMENULOW);

  while (option < 3) {

    if (buttonDebounce(0, 1, 0, 300)) {
      if (option++, option > 2) option = 0;
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 300)) {
      if (option--, option < 0) option = 2;
      strip.clear();
    }

    switch (option) { // sound on or off
      case 0:
        actualValue = encoder.encoderRead(0, 1, true, 1);
        drawCharacter(0, letterS, 100, 0, 0);
        drawCharacter(3, letterN, 100, 0, 0);
        drawCharacter(6, letterD, 100, 0, 0);
        if (sound) {
          drawCharacter(50, letterO, 100, 0, 0);
          drawCharacter(53, letterN, 100, 0, 0);
        } else {
          drawCharacter(50, letterO, 100, 0, 0);
          drawCharacter(53, letterF, 100, 0, 0);
          drawCharacter(56, letterF, 100, 0, 0);
        }

        if (actualValue != sound) {
          strip.clear();
          sound = !sound;
          eeprom_write_byte((uint8_t*)6, sound);
        }
        break;
      case 1:// resume game
        drawCharacter(20, letterR, 100, 0, 0);
        drawCharacter(23, letterE, 100, 0, 0);
        drawCharacter(26, letterS, 100, 0, 0);
        if (buttonDebounce(0, 0, 1, 50)) {
          option = 3;
        }
        break;
      case 2:// end game
        drawCharacter(20, letterE, 100, 0, 0);
        drawCharacter(23, letterN, 100, 0, 0);
        drawCharacter(26, letterD, 100, 0, 0);
        if (buttonDebounce(0, 0, 1, 50)) {
          endGame = true;
          contGame = false;
          option = 3;
        }
        break;
    }
  }
  strip.clear();
}

void scrollPixels(uint8_t arrayName[5][9], uint8_t red, uint8_t green, uint8_t blue) {// code to scroll the tiltle
  uint8_t led;
  uint8_t pixel;
  uint32_t pixelColor;

  for (uint8_t pixelY = 0; pixelY < 9 && LEFTHIGH && RIGHTHIGH; pixelY++) {

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

bool buttonDebounce(bool left, bool right, bool menu, uint32_t debounceTime ) {// code to debounce button press

  if (RIGHTLOW && right && (millis() - buttonRightTimeStamp) > debounceTime) {
    buttonRightTimeStamp = millis();
    return true;
  } else if (!RIGHTLOW) {
    buttonRightTimeStamp = millis();
  }

  if (LEFTLOW && left && (millis() - buttonLeftTimeStamp) > debounceTime) {
    buttonLeftTimeStamp = millis();
    return true;
  } else if (!LEFTLOW) {
    buttonLeftTimeStamp = millis();
  }

  if (SLEEPMENULOW && menu && (millis() - buttonMenuTimeStamp) > debounceTime) {
    buttonMenuTimeStamp = millis();
    return true;
  } else if (!SLEEPMENULOW) {
    buttonMenuTimeStamp = millis();
  }

  return false;
}

//draw characters

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

void showScore(uint16_t valueShort) {// draws all ledInvadersScore using showTime function
  uint16_t valueDecimals = valueShort;
  red = 100;
  green = 0;
  blue = 0;

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

//write score to eeprom
void writeScoreToEEPROM(uint8_t address, uint16_t number) { // ledInvadersScore is an uint16_t so needs to be stored in 2 eeprom slots of 8bits

  eeprom_write_byte((uint8_t*)address, number >> 8);// shift all the bits from the number to the right, 8 times
  eeprom_write_byte((uint8_t*)(address + 1), number & 0xFF);// only store 8bits of the right
}

uint16_t readScoreFromEEPROM(uint8_t address) { // reads and reconstruct the number

  byte byte1 = eeprom_read_byte((uint8_t*)address);
  byte byte2 = eeprom_read_byte((uint8_t*)address + 1);

  return (byte1 << 8) + byte2;
}

// clock

void showclock () {
  colorSelect ();
  if (minutes != lastMinute) { // time update occurs only once every minute
    lastMinute = minutes;
    strip.clear();
    showTime(53, minutes);
    showTime(0, hours);
  }
}

// main screen options menu
void setOptions() {
  strip.clear();
  int8_t option = 0;
  uint8_t actualValue = 0;
  encoder.encoderSetPosition(color, 1);

  while (SLEEPMENULOW) {
    drawCharacter(0, letterM, red, green, blue);
    drawCharacter(4, letterE, red, green, blue);
    drawCharacter(52, letterN, red, green, blue);
    drawCharacter(56, letterU, red, green, blue);
    delay(1500);
    strip.clear();
  }

  while (option < 5) {

    if (RIGHTLOW) {
      if (option++, option > 4)  option = 0;
      strip.clear();
    }

    if (LEFTLOW) {
      if (option--, option < 0)  option = 4;
      strip.clear();
    }

    while (RIGHTLOW || LEFTLOW); // button debounce

    switch (option) { // color NeoPixel LED number from first to last
      case 0:
        drawCharacter(0, letterC, red, green, blue); // color selection
        drawCharacter(4, letterO, red, green, blue);
        showTime(53, color);
        actualValue = encoder.encoderRead(1, 4, true, 1);

        if (actualValue != color) {
          strip.clear();
          color = actualValue;
          if (color > 4) {
            color = 1;
          }
        }
        colorSelect();
        break;
      case 1:
        drawCharacter(0, letterB, red, green, blue);
        drawCharacter(4, letterR, red, green, blue);
        showTime(53, brightLevel);
        actualValue = encoder.encoderRead(1, 5, true, 1);

        if (actualValue != brightLevel) {
          strip.clear();
          brightLevel = actualValue;

          if (brightLevel > 5) {
            brightLevel = 1;
          }
          if (brightLevel == 1) {
            strip.setBrightness(5);
          } else {
            strip.setBrightness((brightLevel * 3));
          }
        }
        break;
      case 2:
        drawCharacter(0, letterH, red, green, blue);
        drawCharacter(4, letterR, red, green, blue);
        showTime(53, hours);
        actualValue = encoder.encoderRead(0, 23, true, 1);

        if (actualValue != hours ) {
          hours = actualValue;
          strip.clear();
        }
        break;
      case 3:
        drawCharacter(0, letterM, red, green, blue);
        drawCharacter(4, letterI, red, green, blue);
        showTime(53, minutes);
        actualValue = encoder.encoderRead(0, 59, true, 1);

        if (actualValue != minutes) {
          minutes = actualValue;
          strip.clear();
        }
        break;
      case 4:
        drawCharacter(20, letterE, red, green, blue);
        drawCharacter(23, letterN, red, green, blue);
        drawCharacter(26, letterD, red, green, blue);
        if (buttonDebounce(0, 0, 1, 50)) {
          option = 5;
        }
        break;
    }
    eeprom_write_byte((uint8_t*)2, color); // values are store in EEPROM
    eeprom_write_byte((uint8_t*)3, brightLevel);
  }
}

void colorSelect () {
  switch (color) { // color NeoPixel LED number from first to last
    case 1:
      red = 80;
      green = 0;
      blue = 0;
      break;
    case 2:
      red = 0;
      green = 80;
      blue = 0;
      break;
    case 3:
      red = 0;
      green = 0;
      blue = 80;
      break;
    case 4:
      red = 80;
      green = 80;
      blue = 80;
      break;
  }
}

void battStatus() { // show batt status color coded green is full, red empty
  voltageReading = map(readSupplyVoltage(), 2980, 3970, 0, 9);
  if (voltageReading > 5) {
    strip.setPixelColor(9, 0, 100, 0);
  }
  if (voltageReading <= 5 && voltageReading > 2) {
    strip.setPixelColor(9, 100, 100, 0);
  }
  if (voltageReading <= 2) {
    strip.setPixelColor(9, 100, 0, 0);
  }
}

uint16_t readSupplyVoltage() { //returns value in millivolts  taken from megaTinyCore example
  analogReference(VDD);
  VREF.CTRLA = VREF_ADC0REFSEL_1V5_gc;
  // there is a settling time between when reference is turned on, and when it becomes valid.
  // since the reference is normally turned on only when it is requested, this virtually guarantees
  // that the first reading will be garbage; subsequent readings taken immediately after will be fine.
  // VREF.CTRLB|=VREF_ADC0REFEN_bm;
  // delay(10);
  uint16_t reading = analogRead(ADC_INTREF);
  reading = analogRead(ADC_INTREF);
  uint32_t intermediate = 1023UL * 1500;
  reading = intermediate / reading;
  return reading;
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

void gameMusic() {
  for (uint16_t x = 0; x < 500; x += 25) {// game start music
    beep(50, x);
  }
}

ISR(RTC_PIT_vect) {// rtc interupt

  /* Clear flag by writing '1': */
  RTC.PITINTFLAGS = RTC_PI_bm;

  if (seconds++, seconds > 59) {// acutal time keeping
    seconds = 0;
    if (minutes++, minutes > 59) {
      minutes = 0;
      if (hours++, hours > 23) {
        hours = 0;
      }
    }
  }
  if (!shouldWakeUp) {// beacause of interrupt only if true wake up
    sleep_cpu();
  }

  if (SLEEPMENULOW && RIGHTLOW && LEFTLOW) { // if all 3 button are pressed, reset.
    _PROTECTED_WRITE(RSTCTRL.SWRR, 1);
  }
}

ISR(TCB0_INT_vect) {// anoter interrupt to show pixels on screen, because micros are lost using RTC and .show cna´t be called one after another  this is the best way I found to call "strip.show"
  strip.show();
  if (interruptTimer > 0) { // used in game to keep track of time
    interruptTimer = 0;
  } else {
    interruptTimer = 10;
  }
  TCB0_INTFLAGS = 1; // clear flag
}

ISR(PORTA_PORT_vect) {

  if (PB3_INTERRUPT) {
    PB3_CLEAR_INTERRUPT_FLAG;// clear interrupt flag
    sleep_disable();
    PORTA.PIN3CTRL &= ~ PORT_ISC_BOTHEDGES_gc; // deattach interrupt
    PORTA.OUT &= ~PIN4_bm;// P CHANNEL mosfet low to activate
    shouldWakeUp = true;
  }
}
