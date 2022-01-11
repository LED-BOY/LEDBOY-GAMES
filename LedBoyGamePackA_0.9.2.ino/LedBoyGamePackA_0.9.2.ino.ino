/*LedBoy3in1A with Clock for LEDBOY and any Attiny series 1 compatible using neopixels
  Flash CPU Speed 16MHz.
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


#include <tinyNeoPixel_Static.h>
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
#define EXT PIN_PA7
#define NEOPIN PIN_PB0
#define BUZZER PIN_PB1
// Rotary Encoder Inputs
#define ROTARYA PIN_PA5
#define ROTARYB  PIN_PA6

// rotary encoder variables
int rotaryCounter = 0;
int lastRotaryCounter = 0;
int encoderPosition;        // Internal position (4 times _positionExt)
int encoderPositionExt;     // External position
int encoderPositionExtPrev; // External position (used only for direction checking)
int8_t oldState;
uint32_t encoderPositionExtTime;     // The time the last position change was detected.
uint32_t encoderPositionExtTimePrev; // The time the previous position change was detected.
// clock variables
uint8_t red = 0; // red led intensity
uint8_t blue = 0;// blue led intensity
uint8_t green = 0;// green led intensity
uint8_t hours = 0; //keep the hours to display
uint8_t minutes = 0;//keep the minutes to display
uint8_t seconds = 0;//keep the seconds to display
uint8_t color; // store color value Red, blue, green, white
uint8_t brightLevel;
uint8_t lastMinute = 60; // used to update the digits only once per minute
uint8_t lastSecond = 0;
uint8_t lastHour = 0;
//game variables
uint8_t gameLevel = 1;
uint8_t ledInvaderLives = 2;
uint8_t enemiesLevel = 0;
uint16_t arrayNumberValue[1]; // stores one array pointer to array number in characters.h
uint16_t gameSpeed;
uint16_t ledInvadersScore;
uint16_t ledTrisScore;
//menu interrupt variables
uint32_t buttonLeftTimeStamp; // button press time
uint32_t buttonRightTimeStamp; // button press time
uint32_t buttonMenuTimeStamp; // button press time
uint32_t timer = 0;
int voltageReading;
bool sound;
bool endGame = false;
bool contGame = true;
volatile boolean shouldWakeUp = true;
volatile uint8_t interruptTimer = 0;

byte pixels[NUMLEDS * 3];
tinyNeoPixel strip = tinyNeoPixel(NUMLEDS, NEOPIN, NEO_GRB, pixels);

void setup() {


  pinMode(NEOPIN, OUTPUT);
  pinMode(SLEEPMENU, INPUT);
  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(EXT, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(MOSFET, OUTPUT);
  pinMode(ROTARYA, INPUT);
  pinMode(ROTARYB, INPUT);

  digitalWrite(MOSFET, LOW);// P CHANNEL mosfet low to activate

  oldState = digitalRead(ROTARYA) | (digitalRead(ROTARYB) << 1);

  if (eeprom_read_byte((uint8_t*)255) != 131) { // prevents excessive flash write and default values if first time used or no colors are choose
    eeprom_write_byte((uint8_t*)255, 131); // 255B of aviable EEPROM on the ATTINY1614
    eeprom_write_byte((uint8_t*)2, 1);//color
    eeprom_write_byte((uint8_t*)3, 2);//brightness level
    eeprom_write_byte((uint8_t*)4, true);//sound on/off
    writeScoreToEEPROM(5, 0); // LedIvaders Score function to store uint16 in two uint8 eeprom slots
    //eeprom_write_byte((uint8_t*)6, 0);This is reserved to ledInvadersScore don´t uncoment and don´t use in any other part of code.
    writeScoreToEEPROM(7, 0); // Ledtris score
    //eeprom_write_byte((uint8_t*)8, 0);// reserved for ledtris score
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
    digitalWrite(MOSFET, HIGH);
    shouldWakeUp = false;
    ADC0.CTRLA  &= ~ ADC_ENABLE_bm; //adc disable
    attachInterrupt(SLEEPMENU, sleepMenuISR, LOW); //atach interupt to wake up.
    sleep_enable();
    sleep_cpu();// go to sleep
  }

  if (buttonDebounce(0, 0, 1, 1000)) { // enters menu options
    setOptions();
    lastMinute = 60;// so time will be updated
  }

  if (digitalRead(LEFT) == LOW && digitalRead(RIGHT) == LOW && digitalRead(SLEEPMENU) == LOW) {
    _PROTECTED_WRITE(RSTCTRL.SWRR, 1);
  }

if (digitalRead(LEFT) == LOW && digitalRead(RIGHT) == LOW) {
    strip.clear();
    gameFlappyLed();
  } else {
    if (buttonDebounce(1, 0, 0, 800)) { // enters game mode
      strip.clear();
      ledInvadersMainTitle();
    }

    if (buttonDebounce(0, 1, 0, 800)) { // enters game mode
      strip.clear();
      ledTrisMainTitle();
    }
  }
}

void ledInvadersMainTitle () { // title of the game
  for (uint8_t x = 0; x <= 4; x++) {
    switch (x) {
      case 1:
        scrollPixels(ledInvadersTitle1, 100, 100, 0);
        break;
      case 2:
        scrollPixels(ledInvadersTitle2, 100, 100, 0);
        break;
      case 3:
        scrollPixels(ledInvadersTitle3, 100, 100, 0);
        break;
      case 4:
        scrollPixels(ledInvadersTitle4, 100, 100, 0);
        break;
    }

    if (buttonDebounce(1, 1, 0, 0)) {
      break;
    }
    gameSpeed =  pgm_read_word_near (enemySpeed + 0);
  }

  for (uint16_t x = 0; x < 500; x += 25) {// game music
    beep(100, x);
  }
  levelTitle ();
}

void ledTrisMainTitle () { // title of the game
  for (uint8_t x = 0; x <= 3; x++) {
    switch (x) {
      case 1:
        scrollPixels(ledTrisTitle1, 100, 100, 0);
        break;
      case 2:
        scrollPixels(ledTrisTitle2, 100, 100, 0);
        break;
      case 3:
        scrollPixels(ledTrisTitle3, 100, 100, 0);
        break;
    }

    if (buttonDebounce(1, 1, 0, 0)) {
      break;
    }
  }
  strip.clear();
  gameLedTris();
}


void levelTitle () {// level description
  strip.clear();
  red = 100;
  green = 0;
  blue = 0;
  showTime(51, gameLevel);

  for (uint8_t x = 0; x <= 2; x++) {
    switch (x) {
      case 1:
        scrollPixels(levelText1, 0, 0, 100);
        break;
      case 2:
        scrollPixels(levelText2, 0, 0, 100);
        break;
    }

    if (buttonDebounce(1, 1, 0, 250)) {
      break;
    }
  }
  strip.clear();
  gameLedInvaders();
}

//Game FlappyLed code

void gameFlappyLed() {
  uint8_t randomWall = 0;
  uint8_t flappyLedIput = 0;
  uint8_t flappyLedPos = 53;
  uint8_t lastflappyLedPos;
  uint8_t wallPos = 9;
  uint8_t passage1;
  uint8_t passage2;
  uint16_t flappyLedTimer = 0;
  uint16_t wallTimer = 0;
  uint16_t gameSpeed = 1800;
  uint16_t score = 0;

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
      beep(44, 44);
    }

    strip.setPixelColor(flappyLedPos, 0, 100, 0);

    if (flappyLedPos != lastflappyLedPos) {
      strip.setPixelColor(lastflappyLedPos, 0, 0, 0);
    }
    lastflappyLedPos = flappyLedPos;

    if (flappyLedTimer >= 1600 && flappyLedPos < 93) {
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
      timer = millis();
      lastMinute = 60;
      break;
    }
  }
}

// Game LedTris code
void gameLedTris() {
  uint16_t actualTetromino[1] = {tetromino1A};// Load tetromino from "character.h"
  uint16_t lastTetromino[1] = {tetromino1A};// Last tetromino used
  uint16_t tetrominoInPos [10][10]; // variable to compare if the screen position has a tetromino
  int tetrominoLastPos = -6;
  int tetrominoPos = -6;
  uint8_t tetrominoRotation;
  uint8_t tetrominoLastRotation = 0;
  uint8_t randomTetromino = 0;
  uint8_t ledtrisLives = 1;// if a tetromino cant`t go lower than row 2, is game over
  uint16_t tetrominoTimer = 0;// overal timer used todrive the game
  uint16_t ledTrisSpeed = 400;
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
    strip.setPixelColor(tetrominoInPos [x][0], 101, 67, 37);
    strip.setPixelColor(tetrominoInPos [x][9], 101, 67, 37);
  }

  // Game code
  while (ledtrisLives > 0 && !endGame) {

    encoderRead(0, 3, true, 1);
    tetrominoRotation = rotaryCounter;
    actualTetromino[0] = tetromino [randomTetromino][tetrominoRotation];//here the actual plyer controlled tetromino is created

    // Advace time all the game speed is cotolled here
    if (interruptTimer) {
      tetrominoTimer += interruptTimer;
    }

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
          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos - 1)) == tetrominoInPos [row][column]) {
            tetrominoMoveLeft = false;
          }

          if ((pgm_read_byte_near (actualTetromino[0] + x) + (tetrominoPos + 1)) == tetrominoInPos [row][column]) {
            tetrominoMoveRight = false;
          }

          if (tetrominoRotation != tetrominoLastRotation) {// only if rotation happen the tetromino is checked and positioned in a valid place

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
    if (tetrominoRotation != tetrominoLastRotation && actualTetromino[0] == tetromino3B && tetrominoMoveLeft) {
      tetrominoPos--;
    }

    if (tetrominoRotation != tetrominoLastRotation && actualTetromino[0] == tetromino3A && tetrominoMoveRight) {
      tetrominoPos++;
    }

    //Here it draws the screen
    if (tetrominoPos != tetrominoLastPos || tetrominoRotation != tetrominoLastRotation) {
      drawCharacter(tetrominoLastPos, lastTetromino[0], 0, 0, 0);
      tetrominoLastPos = tetrominoPos;
      tetrominoLastRotation = tetrominoRotation;
      lastTetromino[0] = actualTetromino[0];
      drawCharacter(tetrominoPos, actualTetromino[0], 0, 50, 0);
    }

    if (tetrominoStop) {//All calcultions done when a tetromino stops moving

      if (digitalRead(RIGHT) == HIGH && digitalRead(LEFT) == HIGH  || tetrominoTimer > 280) {//Some conditions have to be met before stoping so some time is given to last moment moves
        beep(100, 250);
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
                beep(120, 120);
                strip.setPixelColor(tetrominoInPos [(row + 1)][column], 0, 0, 50);
                tetrominoInPos [row][column] = 0;
              }
              if (tetrominoInPos [row][column] == 0) {// if there is nothig on the position it stops because the line isnt`t complete
                tetris = false;
              }
            }

            if (tetris) {
              moveTetrominos = true;

              if (repeatTetrisCheck < 3) {
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

      if (digitalRead(RIGHT) == LOW && digitalRead(LEFT) == LOW) {
        ledTrisSpeed = 60;
      } else {
        ledTrisSpeed = 400;
      }

      if (buttonDebounce(1, 0, 0, 125) && digitalRead(RIGHT) == HIGH && tetrominoMoveLeft) {
        tetrominoPos--;
      }

      if (buttonDebounce(0, 1, 0, 125) && digitalRead(LEFT) == HIGH && tetrominoMoveRight) {
        tetrominoPos++;
      }
    }

    if (buttonDebounce(0, 0, 1, 200)) {//Pause menu
      gameMenu ();
      drawCharacter(0, wallLeft, 101, 67, 37);
      drawCharacter(9, wallRight, 101, 67, 37);
    }
  }

  if (!endGame) {
    for (uint8_t x = 99; x > 0; x--) {// end screen animation
      strip.setPixelColor(x, 50, 0, 0);
      delay(5);
    }

    timer = millis();
    strip.clear();

    if (ledTrisScore > 999) {
      ledTrisScore = 999;
    }
    
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
        break;
      }
    }
    timer = millis();
    endGame = true;
  }

  ledTrisScore = 0;

  if (endGame) {
    endGame = false;
    strip.clear();
    lastMinute = 60;
    timer = millis();
  }

  if (contGame) {
    ledTrisMainTitle();
  } else {
    contGame = true;
  }
}

// LedInvaders Game code
void gameLedInvaders () {
  // enemies variables
  uint8_t arrayElements = pgm_read_byte_near (enemies[enemiesLevel] + 0) - 1; // reads enemy position
  int actualEnemies [arrayElements];// because enemies array cointains 1 extra element with number of elements information, may change in the future
  uint8_t enemiesKilled [arrayElements];// another array that contains only enemies killed
  uint8_t enemiesRemaining = arrayElements;
  uint8_t setEnemyFire = 0;
  uint8_t closestEnemy = 0;
  uint8_t enemiesheightTimer = 0;
  uint8_t enemyShotPosition = 32;
  uint16_t enemiesMoveTimer = 0;
  uint16_t enemyShotTimer = 0;
  uint16_t motherShipTimer = 0;
  uint16_t setEnemyFireTimer = 0;
  int motherShipMovement = 9; // int because working with negative numbers
  int enemyPos = 10; // int because working with negative numbers
  bool enemyShot = false;
  bool callMotherShip = false;

  // player variables
  uint8_t playerPos = 84;
  uint8_t lastEnemyPos;
  uint8_t lastPlayerPos = 0;
  uint8_t playerShotPosition = 99;
  uint16_t playerShotTimer = 0;
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
      setEnemyFireTimer += interruptTimer;
    }

    for (uint8_t x = 0; x < arrayElements; x++) {

      if (enemiesKilled [x] != x) {
        strip.setPixelColor((actualEnemies[x] + lastEnemyPos), 0, 0, 0); // clear last enemy position
        strip.setPixelColor((actualEnemies[x] + enemyPos), 100, 0, 0);// led enemy position musn´t be less than 0

        for (uint8_t y = 0; y < 9  ; y++) {// code to check bounds of enemies

          if (pgm_read_byte_near (screenRightBounds + y) == (actualEnemies[x] + enemyPos)) {
            allowRightMove = false;
          }
          if (pgm_read_byte_near (screenLeftBounds + y) == (actualEnemies[x] + enemyPos)) {
            allowRightMove = true;
          }
        }
        // kind of random function I think???? to generate enemies shots, arduinio random() functon was slowing too much the code here and shots where placed anyware.
        if (setEnemyFireTimer > 220) {
          setEnemyFire = enemyPos + actualEnemies[x]; // sets fire position
          setEnemyFireTimer = 0;
        }
        if (strip.getPixelColor(setEnemyFire) > 0) { // waits until some enemy hopfully pass the position to allow fire.
          enemyShot = true;
        }


        if ((actualEnemies[x] + enemyPos) > closestEnemy) {// some math to know  enemy positon closest to player.
          closestEnemy = (actualEnemies[x] + enemyPos);
        }
      }
    }

    // player position


    if ((millis() - timer) > 10) { // reads encoder ever 10 millis
      encoderRead(80, 87, false, 2);
    }
    playerPos = rotaryCounter;

    if (playerPos != lastPlayerPos) {
      drawCharacter(lastPlayerPos, playerShip, 0, 0, 0);
      lastPlayerPos = playerPos;
      timer = millis();
    }

    if (ledInvaderLives > 1 ) {// player ship color change depending on lives left
      drawCharacter(playerPos, playerShip, 0, 100, 0);
    } else {
      drawCharacter(playerPos, playerShip, 100, 70, 0);
    }

    if (buttonDebounce (1, 1, 0, 0) && !playerShot) {// generates player shot
      beep(125, 125);
      playerShot = true;
      playerShotPosition = (playerPos + 1);
      if ( playerShotPosition < 80) {
        strip.setPixelColor(playerShotPosition, 0 , 0 , 100);
      }
    }

    if ((playerShotTimer >= 120) && playerShot) {//player shot check with enemies and mother ship
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
        beep(100, 250);
      }

      if (playerShot) {
        strip.setPixelColor(playerShotPosition, 0 , 0 , 100);
      } else {
        playerShotPosition = (playerPos + 1);
      }
    }

    if ((enemyShotTimer >= 180) && enemyShot) {// enemiees fire code
      enemyShotTimer = 0;
      strip.setPixelColor(enemyShotPosition, 0 , 0 , 0);

      if (enemyShotPosition > 99) {
        enemyShotPosition = setEnemyFire;
        enemyShot = false;
      } else {
        enemyShotPosition += 10;
      }
    }

    if (enemyShot) {
      strip.setPixelColor(enemyShotPosition, 100 , 100 , 0);
    }

    if (enemyShotPosition == playerPos || enemyShotPosition == (playerPos + 1) || enemyShotPosition == (playerPos + 2)) {
      ledInvaderLives --;
      strip.setPixelColor(enemyShotPosition, 0 , 0 , 0);
      enemyShotPosition = 100;
      if (ledInvaderLives > 0) {
        beep(100, 250);
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
    if (random(0 , 2000) == 500) {
      callMotherShip = true;
    }

    if ((motherShipTimer >= 180) && callMotherShip) {
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

    if (enemiesRemaining < 4) {
      gameSpeed = pgm_read_word_near (enemySpeed + enemiesRemaining);// reads enemies speed from array "enemySpeed" element 0 is reserved for Hard mode, not used for now
    }

    if (enemiesMoveTimer > gameSpeed) {
      enemiesMoveTimer = 0;
      lastEnemyPos = enemyPos;
      enemiesheightTimer++;

      beep(40, 40);

      if (allowRightMove) {// cheked before in screenRightBounds and screenLeftBounds
        enemyPos++;
      } else {
        enemyPos--;
      }

      if (enemiesheightTimer == 22) { // allow enemies to get closer to player
        enemiesheightTimer = 0;
        enemyPos += 10;
      }

      if (closestEnemy > 79) {// if the enemies enters player position END game
        ledInvaderLives = 0;
      }
    }

    if (buttonDebounce(0, 0, 1, 200)) {
      gameMenu();
      //enemiesRemaining = 0;// uncoment to cheat and advance level with one button ;)
    }

    if (endGame) {
      break;
    }

    if (ledInvadersScore > 999) {
      ledInvadersScore = 999;
    }
  }
  strip.clear();

  if (enemiesRemaining == 0) {// go to next level
    gameLevel++;
    enemiesLevel++;

    if (enemiesLevel > 8) {
      enemiesLevel = 0;
    }

    if (gameLevel < 25) {// speeds up level up to level 24 then speeds is the same
      gameSpeed = pgm_read_word_near (enemySpeed + 0) - (gameLevel * 10);
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
    timer = millis();

    if (ledInvadersScore > 999) {
      ledInvadersScore = 999;
    }
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
        break;
      }
    }
    timer = millis();
    endGame = true;
  }

  if (endGame) {
    ledInvadersScore = 0;
    gameSpeed =  pgm_read_word_near (enemySpeed + 0);
    enemiesLevel = 0;
    gameLevel = 1;
    ledInvaderLives = 2;
    endGame = false;
    strip.clear();
    lastMinute = 60;
    timer = millis();
  }

  if (contGame) {
    levelTitle();
  } else {
    contGame = true;
  }
}

void gameMenu () {// menu only available in game mode
  strip.clear();
  encoderSetPosition(sound, 1);

  while (digitalRead(SLEEPMENU) == LOW) {
  }
  int option = 0;

  while (option < 3) {

    if (buttonDebounce(0, 1, 0, 50)) {
      while ( digitalRead(RIGHT) == LOW) { //change option when transition from low to high
      }
      option++;
      if (option > 2) {
        option = 0;
      }
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 50)) {
      while ( digitalRead(LEFT) == LOW) {
      }
      option--;
      if (option < 0) {
        option = 2;
      }
      strip.clear();
    }

    switch (option) { // sound on or off
      case 0:
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
        encoderRead(0, 1, true, 1);

        if (rotaryCounter != sound) {
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

bool buttonDebounce(bool left, bool right, bool menu, uint32_t debounceTime ) {// code to debounce button press

  if ( digitalRead(RIGHT) == LOW && right && (millis() - buttonRightTimeStamp) > debounceTime) {
    buttonRightTimeStamp = millis();
    return true;
  } else if (digitalRead(RIGHT) == HIGH) {
    buttonRightTimeStamp = millis();
  }

  if ( digitalRead(LEFT) == LOW && left && (millis() - buttonLeftTimeStamp) > debounceTime) {
    buttonLeftTimeStamp = millis();
    return true;
  } else if (digitalRead(LEFT) == HIGH) {
    buttonLeftTimeStamp = millis();
  }

  if ( digitalRead(SLEEPMENU) == LOW && menu && (millis() - buttonMenuTimeStamp) > debounceTime) {
    buttonMenuTimeStamp = millis();
    return true;
  } else if (digitalRead(SLEEPMENU) == HIGH) {
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

//write ledInvadersScore to eeprom
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
  int option = 0;

  while (digitalRead(SLEEPMENU) == LOW) {
    drawCharacter(0, letterM, red, green, blue);
    drawCharacter(4, letterE, red, green, blue);
    drawCharacter(52, letterN, red, green, blue);
    drawCharacter(56, letterU, red, green, blue);
    delay(1500);
    strip.clear();
  }

  while (option < 5) {

    if (buttonDebounce(0, 1, 0, 50)) {
      while ( digitalRead(RIGHT) == LOW) { //change option when transition from low to high
      }
      option++;
      if (option > 4) {
        option = 0;
      }
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 50)) {
      while ( digitalRead(LEFT) == LOW) {
      }
      option--;
      if (option < 0) {
        option = 4;
      }
      strip.clear();
    }

    switch (option) { // color NeoPixel LED number from first to last
      case 0:
        drawCharacter(0, letterC, red, green, blue); // color selection
        drawCharacter(4, letterO, red, green, blue);
        showTime(53, color);
        encoderRead(1, 4, true, 1);

        if (rotaryCounter != color) {
          strip.clear();
          color = rotaryCounter;
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
        encoderRead(1, 5, true, 1);

        if (rotaryCounter != brightLevel) {
          strip.clear();
          brightLevel = rotaryCounter;

          if (brightLevel > 5) {
            brightLevel = 1;
          }
          strip.setBrightness((brightLevel * 3));
        }
        break;
      case 2:
        drawCharacter(0, letterH, red, green, blue);
        drawCharacter(4, letterR, red, green, blue);
        showTime(53, hours);
        encoderRead(0, 23, true, 1);

        if (rotaryCounter != hours ) {
          hours = rotaryCounter;
          strip.clear();
        }
        break;
      case 3:
        drawCharacter(0, letterM, red, green, blue);
        drawCharacter(4, letterI, red, green, blue);
        showTime(53, minutes);
        encoderRead(0, 59, true, 1);

        if (rotaryCounter != minutes) {
          minutes = rotaryCounter;
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


void encoderRead(uint8_t rotaryMin, uint8_t rotaryMax, bool loopValue, uint8_t latchMode) {

  encoderTick(latchMode);
  // get the current physical position and calc the logical position
  rotaryCounter = encoderPositionExt;

  if (loopValue) {
    if (rotaryCounter < rotaryMin) {
      encoderSetPosition(rotaryMax, latchMode);
      rotaryCounter = encoderPositionExt;

    } else if (rotaryCounter > rotaryMax) {
      encoderSetPosition(rotaryMin, latchMode);
      rotaryCounter = encoderPositionExt;
    }
  } else {

    if (rotaryCounter < rotaryMin) {
      encoderSetPosition(rotaryMin, latchMode);
      rotaryCounter = rotaryMin;

    } else if (rotaryCounter > rotaryMax) {
      encoderSetPosition(rotaryMax, latchMode);
      rotaryCounter = rotaryMax;
    }
  }
}

void encoderTick(uint8_t latchMode) {
  int8_t thisState = digitalRead(ROTARYA) | (digitalRead(ROTARYB) << 1);

  if (oldState != thisState) {
    encoderPosition += encoderDir[thisState | (oldState << 2)];
    oldState = thisState;
  }
  switch (latchMode) {
    case 1:
      if (thisState == 3) {
        // The hardware has 4 steps with a latch on the input state 3
        encoderPositionExt = encoderPosition >> 2;
        encoderPositionExtTimePrev = encoderPositionExtTime;
        encoderPositionExtTime = millis();
      }
      break;
    case 2:
      if ((thisState == 0) || (thisState == 3)) {
        // The hardware has 2 steps with a latch on the input state 0 and 3
        encoderPositionExt = encoderPosition >> 1;
        encoderPositionExtPrev = encoderPositionExtTime;
        encoderPositionExtTime = millis();
      }
      break;
  }
}

void encoderGetDirection() {

  if (encoderPositionExtPrev > encoderPositionExt) {
    encoderPositionExtPrev = encoderPositionExt;

  } else if (encoderPositionExtPrev < encoderPositionExt) {
    encoderPositionExtPrev = encoderPositionExt;

  } else {
    encoderPositionExtPrev = encoderPositionExt;
  }
}

void encoderSetPosition(int newPosition, uint8_t latchMode) {
  switch (latchMode) {
    case 1:
      // only adjust the external part of the position.
      encoderPosition = ((newPosition << 2) | (encoderPosition & 0x03L));
      encoderPositionExt = newPosition;
      encoderPositionExtPrev = newPosition;
      break;

    case 2:
      // only adjust the external part of the position.
      encoderPosition = ((newPosition << 1) | (encoderPosition & 0x01L));
      encoderPositionExt = newPosition;
      encoderPositionExt = newPosition;
      break;
  } // switch
}

void battStatus() { // show batt status color coded green is full, red empty
  voltageReading = map(readSupplyVoltage(), 2980, 3970, 0, 9);
  if (voltageReading > 5) {
    strip.setPixelColor(9, 0, 100, 0);
  }
  if (voltageReading <= 5 && voltageReading > 2) {
    strip.setPixelColor(9, 80, 80, 0);
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

void sleepMenuISR() {// side button interript to wake up.
  detachInterrupt(SLEEPMENU);
  sleep_disable();
  ADC0.CTRLA = ADC_ENABLE_bm; //adc enable
  digitalWrite(MOSFET, LOW);
  delay(10);
  shouldWakeUp = true;
  lastMinute = 60;
}
