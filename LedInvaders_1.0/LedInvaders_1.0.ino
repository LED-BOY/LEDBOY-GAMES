/*Led Invaders with Clock for LEDBOY and any Attiny series 1 compatible using neopixels
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
uint8_t gameLevel = 1;
uint8_t lives = 2;
uint8_t potValue;
uint8_t enemiesLevel = 0;
uint8_t voltageReading = 0;
uint16_t arrayNumberValue[1]; // stores one array pointer to array number in characters.h
uint16_t gameSpeed;
uint16_t score = 0;
uint32_t buttonLeftTimeStamp; // button press time
uint32_t buttonRightTimeStamp; // button press time
uint32_t buttonMenuTimeStamp; // button press time
uint32_t timer = 0;
uint32_t pixelColor;
bool displayOn = true; // if display is on or off
bool blinkColon = false;
bool sound;
bool endGame = false;
bool contGame = true;
volatile boolean shouldWakeUp = true;
volatile boolean interrupt = false;
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

  if (eeprom_read_byte((uint8_t*)255) != 131) { // prevents excessive flash write and default values if first time used or no colors are choose
    eeprom_write_byte((uint8_t*)255, 131); // 255B of aviable EEPROM on the ATTINY1614
    eeprom_write_byte((uint8_t*)2, 1);//color
    eeprom_write_byte((uint8_t*)3, 2);//brightness level
    writeScoreToEEPROM(4, 0); // function to store uint16 in two uint8 eeprom slots
    //eeprom_write_byte((uint8_t*)5, 0);This is reserved to Score don´t uncoment and don´t use in any other part of code.
    eeprom_write_byte((uint8_t*)6, true);//brightness level
  }

  //read from EEPROM
  color = eeprom_read_byte((uint8_t*)2); //read stored color
  sound = eeprom_read_byte((uint8_t*)7); //read if mute on/off

  strip.begin();
  brightLevel = eeprom_read_byte((uint8_t*)3); // read brightness from eeprom
  strip.setBrightness(brightLevel * 4);// brightness select from 0 to 255

  RTC.PITINTCTRL = RTC_PI_bm; /* Periodic Interrupt: enabled */

  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 32768 */
                 | RTC_PITEN_bm; /* Enable: enabled */

  TCB0.INTCTRL = TCB_CAPT_bm; // Setup Timer B as compare capture mode to trigger interrupt
  TCB0_CCMP = 8000;// CLK / 2
  TCB0_CTRLA = (1 << 1) | TCB_ENABLE_bm;

  sei(); // enable interrupts
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);// configure sleep mode
}

void loop() {

  showclock();

  if ((millis() - timer) > 10000 && shouldWakeUp) { // sleep code while in clock mode
    timer = millis();
    strip.clear();
    strip.show();
    ADC0.CTRLA  &= ~ ADC_ENABLE_bm; //adc disable
    digitalWrite(MOSFET, HIGH);
    shouldWakeUp = false;
    attachInterrupt(SLEEPMENU, sleepMenuISR, LOW); //atach interupt to wake up.
    sleep_enable();
    sleep_cpu();// go to sleep
  }

  if (buttonDebounce(0, 0, 1, 1000)) { // enters batt display and menu options
    strip.clear();
    timer = millis();
    voltageReading = map(readSupplyVoltage(), 3020, 3720, 0, 99);
    if (voltageReading > 95) {
      drawCharacter(50, number1, red, green, blue);
      drawCharacter(53, number0, red, green, blue);
      drawCharacter(56, number0, red, green, blue);
    } else {
      showTime(53, voltageReading);
    }
    drawCharacter(0, letterB, red, green, blue);
    drawCharacter(3, letterA, red, green, blue);
    drawCharacter(6, letterT, red, green, blue);

    while (digitalRead(SLEEPMENU) == LOW) { //if 2 sec pass with button pressed enters menu
      if  ((millis() - timer) > 2000) {
        timer = millis();
        strip.clear();
        setOptions();
        break;
      }
    }
    lastMinute = 60;
    timer = millis();
  }

  if (buttonDebounce(1, 1, 0, 800)) { // enters game mode
    strip.clear();
    mainTitle ();
  }
}

void mainTitle () { // title of the game
  for (uint8_t x = 0; x <= 4; x++) {
    switch (x) {
      case 1:
        scrollPixels(ledInvadersTitle1, true, 100, 100, 0);
        break;
      case 2:
        scrollPixels(ledInvadersTitle2, true, 100, 100, 0);
        break;
      case 3:
        scrollPixels(ledInvadersTitle3, true, 100, 100, 0);
        break;
      case 4:
        scrollPixels(ledInvadersTitle4, true, 100, 100, 0);
        break;
    }

    if (buttonDebounce(1, 1, 0, 250)) {
      break;
    }
    gameSpeed =  pgm_read_word_near (enemySpeed + 0);
  }

  for (uint16_t x = 0; x < 500; x += 25) {// game music
    beep(50, x);
  }
  levelTitle ();
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
        scrollPixels(levelText1, false, 0, 0, 100);
        break;
      case 2:
        scrollPixels(levelText2, false, 0, 0, 100);
        break;
    }

    if (buttonDebounce(1, 1, 0, 250)) {
      break;
    }
  }
  strip.clear();
  game();
}

// Game code
void game () {
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
  }

  randomSeed(analogRead(TOUCH));

  while (enemiesRemaining > 0 && lives > 0) {

    if (interruptTimer) {// keeps time based on TCB0 interrupt
      enemiesMoveTimer += interruptTimer;
      enemyShotTimer += interruptTimer;
      motherShipTimer += interruptTimer;
      playerShotTimer += interruptTimer;
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
        if (interruptTimer == 10 && (closestEnemy / 10) == (enemyPos + actualEnemies[x]) / 10) {
          setEnemyFire = enemyPos + actualEnemies[x]; // sets fire position
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
    playerPos = map(analogReadEnh(POT, 11), 0, 2047, 80, 88);

    if (playerPos != lastPlayerPos) {
      drawCharacter(lastPlayerPos, playerShip, 0, 0, 0);
      lastPlayerPos = playerPos;
    }

    if (lives > 1 ) {// player ship color change depending on lives left
      drawCharacter(playerPos, playerShip, 0, 100, 0);
    } else {
      drawCharacter(playerPos, playerShip, 100, 70, 0);
    }

    if (buttonDebounce (1, 1, 0, 0) && !playerShot) {// generates player shot
      beep(100, 100);
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
          score += 2;
          strip.setPixelColor((actualEnemies[x] + enemyPos), 0, 0, 0);
        }
      }

      if (playerShotPosition == motherShipMovement || playerShotPosition == (motherShipMovement + 1)) {// mother ship hit detection
        motherShipMovement = 9;
        callMotherShip = false;
        lives = 2;
        score += 5;
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

    if ((enemyShotTimer >= 150) && enemyShot) {// enemiees fire code
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
      if (enemyShotPosition == (playerPos + 1)) {
        lives = 0;
      } else {
        lives --;
      }
      strip.setPixelColor(enemyShotPosition, 0 , 0 , 0);
      enemyShotPosition = 100;
      if (lives > 0) {
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
    if (random(0 , 1000) == 500) {
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

      beep(22, 22);

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
        lives = 0;
      }
    }

    if (buttonDebounce(0, 0, 1, 300)) {
      gameMenu();
      //enemiesRemaining = 0;// uncoment to cheat and advance level with one button ;)
    }

    if (endGame) {
      break;
    }

    if (score > 999) {
      score = 0;
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

    if (gameLevel > 99) {// for now  game continues only to level 9
      drawCharacter(20, letterW, 100, 0, 0);
      drawCharacter(23, letterI, 100, 0, 0);
      drawCharacter(26, letterN, 100, 0, 0);
      delay(2000);
      endGame = true;
    }
  }

  if (lives == 0) {// Game over
    if (score > 999) {
      score = 0;
    }
    if (score > readScoreFromEEPROM(4)) {
      drawCharacter(0, letterN, 100, 0, 0);
      drawCharacter(3, letterE, 100, 0, 0);
      drawCharacter(6, letterW, 100, 0, 0);
      showScore(score);
      writeScoreToEEPROM(4, score);
    } else {
      drawCharacter(0, letterH, 100, 0, 0);
      drawCharacter(3, letterI, 100, 0, 0);
      showScore(readScoreFromEEPROM(4));
    }
    while (!buttonDebounce(1, 1, 0, 100)) {

      if (buttonDebounce(0, 0, 1, 2000)) {
        strip.clear();
        writeScoreToEEPROM(4, 0);
        drawCharacter(0, letterC, 100, 0, 0);
        drawCharacter(3, letterL, 100, 0, 0);
        drawCharacter(6, letterR, 100, 0, 0);
      }
    }
    endGame = true;
  }

  if (endGame) {
    score = 0;
    gameSpeed =  pgm_read_word_near (enemySpeed + 0);
    enemiesLevel = 0;
    gameLevel = 1;
    lives = 2;
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
  uint8_t option = 0;

  while (option != 3) {

    if (buttonDebounce(0, 1, 0, 300) && option < 2) {
      option++;
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 300) && option > 0) {
      option--;
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
        if (buttonDebounce(0, 0, 1, 500)) {
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

void scrollPixels(uint8_t arrayName[5][9], bool animateTitle, uint8_t red, uint8_t green, uint8_t blue) {// code to scroll the tiltle
  uint8_t shipPos;
  uint8_t shipPosLast = 85;
  uint8_t led;
  uint8_t pixel;
  uint32_t pixelColor;
  shipPos = random(80, 88);

  for (uint8_t pixelY = 0; pixelY < 9; pixelY++) {

    for (uint8_t x = 0; x < 5; x++) { // scroll text from right to left only on colum of information is draw
      led = pgm_read_byte_near (screenRightBounds + x);
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

    if (animateTitle) {// ship animation on main title

      if (millis() >= (timer + 500)) {// scroll code
        timer = millis();
        strip.setPixelColor((shipPos + 1), 0 , 0 , 0);


        if (shipPosLast == shipPos) {
          shipPos = random(80, 88);
        }
        if (pixelY == 8) {
          shipPos = 80;
        }

        for (uint8_t z = 80; z <= 100; z++) {
          strip.setPixelColor(z, 0);
        }
        if (shipPosLast < shipPos) {
          shipPosLast++;
        }
        if (shipPosLast > shipPos) {
          shipPosLast--;
        }
        drawCharacter(shipPosLast, playerShip, 0, 100, 0);
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

//write score to eeprom
void writeScoreToEEPROM(uint8_t address, uint16_t number) { // score is an uint16_t so needs to be stored in 2 eeprom slots of 8bits

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
    if (displayOn) {
      strip.clear();
      showTime(53, minutes);
      showTime(0, hours);
    }
  }
}

// main screen options menu
void setOptions() {
  strip.clear();
  timer = millis();
  uint8_t option = 0;

  while (digitalRead(SLEEPMENU) == LOW) {
    drawCharacter(0, letterM, red, green, blue);
    drawCharacter(4, letterE, red, green, blue);
    drawCharacter(52, letterN, red, green, blue);
    drawCharacter(56, letterU, red, green, blue);
    delay(500);
    strip.clear();
  }

  while (option < 5) {

    if (buttonDebounce(0, 1, 0, 300) && option < 4) {
      option++;
      strip.clear();
    }

    if (buttonDebounce(1, 0, 0, 300) && option > 0) {
      option--;
      strip.clear();
    }

    switch (option) { // color NeoPixel LED number from first to last
      case 0:
        drawCharacter(0, letterC, red, green, blue); // color selection
        drawCharacter(4, letterO, red, green, blue);
        showTime(53, color);
        if (buttonDebounce(0, 0, 1, 800)) {
          strip.clear();
          color++;
        }
        if (color > 4) {
          color = 1;
        }
        colorSelect();
        break;
      case 1:
        drawCharacter(0, letterB, red, green, blue);
        drawCharacter(4, letterR, red, green, blue);
        showTime(53, brightLevel);
        if (buttonDebounce(0, 0, 1, 800)) {
          strip.clear();
          brightLevel++;
          if (brightLevel > 5) {
            brightLevel = 1;
          }
          strip.setBrightness((brightLevel * 4));
        }
        break;
      case 2:
        drawCharacter(0, letterH, red, green, blue);
        drawCharacter(4, letterR, red, green, blue);
        showTime(53, hours);
        hours = map(analogReadEnh(POT, 12), 0, 4095, 0, 24);
        if (hours != lastHour) {
          lastHour = hours;
          strip.clear();
        }
        break;
      case 3:
        drawCharacter(0, letterM, red, green, blue);
        drawCharacter(4, letterI, red, green, blue);
        showTime(53, minutes);
        minutes = map(analogReadEnh(POT, 12), 0, 4095, 0, 60);
        if (minutes != lastMinute) {
          lastMinute = minutes;
          drawCharacter(53, number8, 0, 0, 0);
          drawCharacter(57, number8, 0, 0, 0);
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
      red = 100;
      green = 0;
      blue = 0;
      break;
    case 2:
      red = 0;
      green = 100;
      blue = 0;
      break;
    case 3:
      red = 0;
      green = 0;
      blue = 100;
      break;
    case 4:
      red = 80;
      green = 80;
      blue = 80;
      break;
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
  Serial.print(reading);
  Serial.println(" (discarded)");
  reading = analogRead(ADC_INTREF);
  Serial.println(reading);
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
    interruptTimer = 10;
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
