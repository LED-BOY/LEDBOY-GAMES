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

#include "Arduino.h"

class encoder{

  public:
  encoder(void);
  uint8_t encoderRead(uint8_t rotaryMin, uint8_t rotaryMax, bool loopValue, uint8_t latchMode);
  void encoderTick(uint8_t latchMode);
  void encoderGetDirection(void);
  void encoderSetPosition(int newPosition, uint8_t latchMode);
};

extern encoder encoder;
