/*
    This file is part of LedInvaders x.x.

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

const PROGMEM uint8_t screenRightBounds [10] = { 9, 19, 29, 39, 49, 59, 69, 79, 89, 99};

const PROGMEM uint8_t screenLeftBounds [10] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90};

uint8_t ledInvadersTitle1[5][9] = { // title "LED"
  {1, 0, 0, 1, 1, 1, 1, 1, 0},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 0, 0, 1, 1, 1, 1, 0, 1},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 0}
};

uint8_t ledInvadersTitle2[5][9] = {//title "INV"
  { 0, 1, 0, 1, 1, 1, 1, 0, 1},
  { 0, 0, 0, 1, 0, 1, 1, 0, 1},
  { 0, 1, 0, 1, 0, 1, 1, 0, 1},
  { 0, 1, 0, 1, 0, 1, 1, 0, 1},
  { 0, 1, 0, 1, 0, 1, 0, 1, 0}
};

uint8_t ledInvadersTitle3[5][9] = {//title "ADE"
  {0, 1, 0, 1, 1, 0, 1, 1, 1},
  {1, 0, 1, 1, 0, 1, 1, 0, 0},
  {1, 1, 1, 1, 0, 1, 1, 1, 1},
  {1, 0, 1, 1, 0, 1, 1, 0, 0},
  {1, 0, 1, 1, 1, 0, 1, 1, 1}
};

uint8_t ledInvadersTitle4[5][9] = {//title "RS"
  {1, 1, 0, 0, 1, 1, 0, 0, 0},
  {1, 0, 1, 1, 0, 0, 0, 0, 0},
  {1, 1, 0, 0, 1, 0, 0, 0, 0},
  {1, 0, 1, 0, 0, 1, 0, 0, 0},
  {1, 0, 1, 1, 1, 0, 0, 0, 0}
};

uint8_t levelText1[5][9] = {//title "LEV"
  {1, 0, 0, 1, 1, 1, 1, 0, 1},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 0, 0, 1, 1, 1, 1, 0, 1},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 0, 1, 0}
};

uint8_t levelText2[5][9] = {//title "EL"
  {1, 1, 1, 1, 0, 0, 0, 0, 0},
  {1, 0, 0, 1, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 0, 0, 0, 0, 0},
  {1, 0, 0, 1, 0, 0, 0, 0, 0},
  {1, 1, 1, 1, 1, 1, 0, 0, 0}
};

// all arrays cointains one extra element [0] to know array number of elements.
//numbers
const PROGMEM uint8_t playerShip [5] = {5,  1,
                                        10, 11, 12
                                       };

const PROGMEM uint8_t playerShipDead [5] = {5,  11,
                                            0, 1, 2
                                           };

const PROGMEM uint8_t enemies0 [9] = {9, 2, 4, 6,
                                      13, 15,
                                      22, 24, 26
                                     };

const PROGMEM uint8_t enemies1 [10] = {10,  2,  6,
                                       11, 13, 15, 17,
                                       22, 24, 26
                                      };

const PROGMEM uint8_t enemies2 [10] = {10, 2, 4, 6,
                                       12, 14, 16,
                                       22, 24, 26
                                      };

const PROGMEM uint8_t enemies3 [8] = {8, 2, 4, 6,
                                      13, 15,
                                      23, 25,
                                     };

const PROGMEM uint8_t enemies4 [8] = {8, 3, 5,
                                      12, 14, 16,
                                      23, 25
                                     };

const PROGMEM uint8_t enemies5 [8] = {8, 11, 13, 15, 17,
                                      22, 24, 26
                                     };

const PROGMEM uint8_t enemies6 [10] = {10, 2, 4, 6,
                                       11, 13, 15, 17,
                                       22, 26
                                      };

const PROGMEM uint8_t enemies7 [9] = {9, 1, 3, 5,
                                      11, 13, 15,
                                      22, 24
                                     };

const PROGMEM uint8_t enemies8 [9] = {9, 2, 4, 6,
                                      11, 13, 15, 17,
                                      24
                                     };


uint16_t enemies [9] = {enemies0, enemies1, enemies2, enemies3, enemies4, enemies5, enemies6, enemies7, enemies8};

const PROGMEM uint16_t enemySpeed [4] {450, 125, 150 , 200}; // element 0 is default gameSpeed, elements from 1 to 3 are speeds of remaining enemies 1 is the fastest


//numbers from 0 to 9
const PROGMEM uint8_t number0  [13] = {13, 0, 1, 2,
                                       10,   12,
                                       20,   22,
                                       30,   32,
                                       40, 41, 42
                                      };


const PROGMEM  uint8_t number1 [6] = { 6, 2,
                                       12,
                                       22,
                                       32,
                                       42
                                     };

const PROGMEM uint8_t number2 [12] = {12, 0, 1, 2,
                                      12,
                                      20, 21, 22,
                                      30,
                                      40, 41, 42
                                     };

const PROGMEM uint8_t number3 [12] = {12, 0, 1, 2,
                                      12,
                                      20, 21, 22,
                                      32,
                                      40, 41, 42
                                     };

const PROGMEM uint8_t number4 [10] = {10, 0,    2,
                                      10,   12,
                                      20, 21, 22,
                                      32,
                                      42
                                     };

const PROGMEM uint8_t number5 [12] = {12, 0, 1, 2,
                                      10,
                                      20, 21, 22,
                                      32,
                                      40, 41, 42
                                     };


const PROGMEM uint8_t number6 [11] = {11, 0,
                                      10,
                                      20, 21, 22,
                                      30,   32,
                                      40, 41, 42
                                     };


const PROGMEM uint8_t number7 [8] = {8, 0, 1, 2,
                                     12,
                                     22,
                                     32,
                                     42
                                    };


const PROGMEM uint8_t number8 [14] = {14, 0, 1, 2,
                                      10,   12,
                                      20, 21, 22,
                                      30,   32,
                                      40, 41, 42
                                     };


const PROGMEM uint8_t number9 [11] = {11, 0, 1, 2,
                                      10,   12,
                                      20, 21, 22,
                                      32,
                                      42
                                     };


// Array containing all numbers array
const PROGMEM uint16_t number[10] = { number0, number1, number2, number3, number4,
                                      number5, number6, number7, number8, number9
                                    };


//letters
const PROGMEM uint8_t letterA [11] = {11,    1,
                                      10,   12,
                                      20, 21, 22,
                                      30,   32,
                                      40,   42
                                     };



const PROGMEM uint8_t letterB [11] = {11, 0, 1,
                                      10,   12,
                                      20, 21,
                                      30,   32,
                                      40, 41
                                     };

const PROGMEM uint8_t letterC [8] = {8,     1, 2,
                                     10,
                                     20,
                                     30,
                                     41, 42
                                    };

const PROGMEM uint8_t letterD [11] = {11, 0, 1,
                                      10,   12,
                                      20,   22,
                                      30,   32,
                                      40, 41
                                     };

const PROGMEM uint8_t letterE [12] = {12, 0, 1, 2,
                                      10,
                                      20, 21, 22,
                                      30,
                                      40, 41, 42
                                     };

const PROGMEM uint8_t letterF [10] = {10,  0, 1, 2,
                                      10,
                                      20, 21, 22,
                                      30,
                                      40,
                                     };

const PROGMEM uint8_t letterH [12] = {12, 0,    2,
                                      10,   12,
                                      20, 21, 22,
                                      30,   32,
                                      40,   42
                                     };

const PROGMEM uint8_t letterI [5] = {5, 1,

                                     21,
                                     31,
                                     41
                                    };

const PROGMEM uint8_t letterL[8] = {8, 0,
                                    10,
                                    20,
                                    30,
                                    40, 41, 42
                                   };

const PROGMEM uint8_t letterM [12] = {12, 0,    2,
                                      10, 11, 12,
                                      20,    22,
                                      30,    32,
                                      40,    42
                                     };

const PROGMEM uint8_t letterN [12] = {12, 0, 1,  2,
                                      10,    12,
                                      20,    22,
                                      30,    32,
                                      40,    42
                                     };


const PROGMEM uint8_t letterO [9] = {9,      1,
                                     10,   12,
                                     20,   22,
                                     30,   32,
                                     41
                                    };


const PROGMEM uint8_t letterR [11] = {11, 0, 1,
                                      10,   12,
                                      20, 21,
                                      30,   32,
                                      40,   42
                                     };

const PROGMEM uint8_t letterS [8] = {8, 1, 2,
                                     10,
                                     21,
                                     32,
                                     40, 41
                                    };

const PROGMEM uint8_t letterT [8] = {8, 0, 1, 2,
                                     11,
                                     21,
                                     31,
                                     41
                                    };

const PROGMEM uint8_t letterU [12] = {12,  0,    2,
                                          10,    12,
                                           20,    22,
                                           30,   32,
                                           40, 41, 42
                                     };


const PROGMEM uint8_t letterV [10] = {10, 0,   2,
                                      10,   12,
                                      20,   22,
                                      30,   32,
                                      41
                                     };

const PROGMEM uint8_t letterW [12] = {12, 0,    2,
                                      10,   12,
                                      20,   22,
                                      30, 31, 32,
                                      40,   42
                                     };
