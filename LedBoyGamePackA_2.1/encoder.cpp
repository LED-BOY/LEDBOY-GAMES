/*
    This file is part of LedBoyGamePackA x.x.

    Foobar is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.

    To contact us: ledboy.net
    ledboyconsole@gmail.com
*/

#include "encoder.h"

int rotaryCounter = 0;
int encoderPosition;        // Internal position (4 times _positionExt)
int encoderPositionExt;     // External position
int encoderPositionExtPrev; // External position (used only for direction checking)
int8_t oldState;
uint32_t encoderPositionExtTime;     // The time the last position change was detected.
uint32_t encoderPositionExtTimePrev; // The time the previous position change was detected.

const int8_t encoderDir[] = {
  0, -1, 1, 0,
  1, 0, 0, -1,
  -1, 0, 0, 1,
  0, 1, -1, 0
};

encoder::encoder(void) {}

uint8_t encoder::encoderRead(uint8_t rotaryMin, uint8_t rotaryMax, bool loopValue, uint8_t latchMode) {

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
  return rotaryCounter;
}

void encoder::encoderTick(uint8_t latchMode) {
  int8_t thisState = ((!(PORTA.IN & PIN5_bm)) | !(PORTA.IN & PIN6_bm)  << 1);

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

void encoder::encoderGetDirection(void) {

  if (encoderPositionExtPrev > encoderPositionExt) {
    encoderPositionExtPrev = encoderPositionExt;

  } else if (encoderPositionExtPrev < encoderPositionExt) {
    encoderPositionExtPrev = encoderPositionExt;

  } else {
    encoderPositionExtPrev = encoderPositionExt;
  }
}

void encoder::encoderSetPosition(int newPosition, uint8_t latchMode) {
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
