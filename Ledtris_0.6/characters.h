const PROGMEM uint8_t tetromino1A [5] = {5,  0,
                                         10,
                                         20, 21
                                        };

const PROGMEM uint8_t tetromino1B [5] = {5,       2,
                                         10, 11, 12
                                        };

const PROGMEM uint8_t tetromino1C [5] = {5, 0, 1,
                                         11,
                                         21
                                        };

const PROGMEM uint8_t tetromino1D [5] = {5,  0, 1, 2,
                                         10
                                        };

const PROGMEM uint8_t tetromino2A [5] = {5,  0,  1,
                                         10, 11
                                        };

const PROGMEM uint8_t tetromino3A [4] = {4,  0,
                                         10,
                                         20,
                                        };

const PROGMEM uint8_t tetromino3B [4] = {4,  0, 1, 2
                                        };

const PROGMEM uint8_t tetromino4A [5] = {5,   1,
                                         10, 11, 12
                                        };

const PROGMEM uint8_t tetromino4B [5] = {5,   1,
                                         10, 11,
                                         21
                                        };

const PROGMEM uint8_t tetromino4C [5] = {5, 0, 1, 2,
                                         11
                                        };

const PROGMEM uint8_t tetromino4D [5] = {5,   1,
                                         11, 12,
                                         21
                                        };

const PROGMEM uint8_t tetromino5A [5] = {5,  0,  1,
                                         11, 12
                                        };

const PROGMEM uint8_t tetromino5B [5] = {5,  1,
                                         10, 11,
                                         20
                                        };


uint16_t tetromino[5][4] = {
  {tetromino1A, tetromino1B, tetromino1C, tetromino1D},
  {tetromino2A, tetromino2A, tetromino2A, tetromino2A},
  {tetromino3A, tetromino3B, tetromino3A, tetromino3B},
  {tetromino4A, tetromino4B, tetromino4C, tetromino4D},
  {tetromino5A, tetromino5B, tetromino5A, tetromino5B}
};

const PROGMEM uint8_t wallLeft [10] = {0,  10, 20, 30, 40, 50, 60,
                                       70, 80, 90
                                      };

const PROGMEM uint8_t wallRight [10] = { 9,  19, 29, 39, 49, 59, 69,
                                         79, 89, 99
                                       };

const PROGMEM uint8_t bottomFloor [10] = {90, 91,  92, 93, 94, 95, 96, 97,
                                          98, 99,
                                         };


uint8_t ledTrisTitle1[5][9] = { // title "LED"
  {1, 0, 0, 1, 1, 1, 1, 1, 0},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 0, 0, 1, 1, 1, 1, 0, 1},
  {1, 0, 0, 1, 0, 0, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 0}
};

uint8_t ledTrisTitle2[5][9] = {//title "TRI"
  { 1, 1, 1, 1, 1, 0, 0, 1, 0},
  { 0, 1, 0, 1, 0, 1, 0, 0, 0},
  { 0, 1, 0, 1, 1, 0, 0, 1, 0},
  { 0, 1, 0, 1, 0, 1, 0, 1, 0},
  { 0, 1, 0, 1, 0, 1, 0, 1, 0}
};

uint8_t ledTrisTitle3[5][9] = {//title "TRI"
  { 0, 1, 1, 0, 0, 0, 0, 0, 0},
  { 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 1, 0, 0, 0, 0, 0, 0},
  { 1, 1, 0, 0, 0, 0, 0, 0, 0}
};

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
