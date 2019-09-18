/*
 * Electronic Checkers:
 * 
 * Team #66
 *  Joseph Canning (jec2)
 *  Raja Patel (hpate85)
 *  Andrew Wirtz (awirtz5)
 * 
 * Description (Project Abstract):
 *    Our project uses three Arduinos to run a two-player game of checkers. Two Arduinos are connected with one LCD screen and four push-buttons 
 *  each to serve as player inputs. Players select a piece to move (labelled from) and a place to move it to (labelled to) on their controllers. 
 *  These input controllers are slaved to the third Arduino which runs the game program, controlling an LED matrix used as the board. 
 *  The I2C communication protocol is used to connect the Arduinos together. I2C permits a master Arduino to request transmissions from a number 
 *  of slave devices, which allows for turns to be easily implemented in software. The input controllers run nearly identical programs that transmit 
 *  move information upon hitting a send button but have different I2C addresses. 
 *  
 *  Additional information:
 *    - The LED matrix used is an Adafruit NeoPixel and has its DIN connected to digital pin 6 on the game Arduino
 *    - I2C communication requires all Arduinos' analog pins 5 (SDA) and 6 (SCL) to be connected together
 *    - I2C also requires that all Arduinos share a common ground
 *    
 *  References used:  
 *    - https://www.arduino.cc/en/Tutorial/MasterReader - Software
 *    - https://www.arduino.cc/en/reference/wire - Software
 *    - https://electronics.stackexchange.com/questions/25278/how-to-connect-multiple-i2c-interface-devices-into-a-single-pin-a4-sda-and-a5 - Hardware
 */

#include <Adafruit_NeoPixel.h> // NeoPixel library (LEDs)
#include <Wire.h> // I2C communication library

#ifdef __AVR__
 #include <avr/power.h>
#endif

Adafruit_NeoPixel board(64, 6, NEO_GRB + NEO_KHZ800); // 8x8 LED board on pin 6
// colors
const uint32_t p1Color = board.Color(200, 20, 10);
const uint32_t p1KingColor = board.Color(200, 50, 175);
const uint32_t p2Color = board.Color(30, 100, 175);
const uint32_t p2KingColor = board.Color(120, 255, 150);
const uint32_t noColor = board.Color(0, 0, 0);
// how many pieces each player has
int p1Pieces = 12;
int p2Pieces = 12;
boolean p1Turn = true; // 
char* data = (char*) malloc(4*sizeof(char)); // move data from player controller
int state[8][8]; // software representation of game board; 0 = empty, 1 = player 1, 2 = player 2, 3 = p. 1 king, 4 = p. 2 king

void setup() {
  
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif

  Wire.begin(); // allow for I2C communication with slave devices

  board.begin(); // enable LEDs                      
  board.setBrightness(2); // keep between 1 and 20, otherwise the LEDs are too bright
  colorWipe(board.Color(125, 200, 20), 10); // play start animation/test LEDs
  board.clear();
  board.show();
  delay(500);
  initPieces(); // start game
  
}

// players take turns until someone has lost all of their pieces
void loop() {

  // player 1's turn
  while (p1Turn) {

    checkEnd();
    Wire.requestFrom(0, 4);

    if (Wire.available()) {

      for (int i = 0; i < 4; i++) {
        data[i] = Wire.read();
      }

      int from = getLEDAt(data[1], data[0]);
      int to = getLEDAt(data[3], data[2]);
      evalTurn(from, to, true);
    
    }

    delay(500);
    
  }

  // player 2's turn
  while (!p1Turn) {

    checkEnd();
    Wire.requestFrom(1, 4); // player 2 controller has address of 1; move signal has 4 chars (bytes)

    if (Wire.available()) { // read transmission, update board

      for (int i = 0; i < 4; i++) {
        data[i] = Wire.read();
      }

      int from = getLEDAt(data[1], data[0]);
      int to = getLEDAt(data[3], data[2]);
      evalTurn(from, to, false);
    
    }

    delay(500);
    
  }
  
}

// converts to transmitted coordinate from player to an int representing an LED
int getLEDAt(char y, int x) {

  int yInt = y - 'A';
  return (8 * yInt) + (x % 8);
  
}

// performs capture of a piece including the "hop" movement
void capturePiece(int from, int to, uint32_t fromColor, uint32_t toColor, bool isLeft, bool isP1) {

  state[to/8][to%8] = 0; // update game state
  board.setPixelColor(to, noColor);

  // subtract piece
  if (isP1) {
    p2Pieces--;
  } else {
    p1Pieces--;
  }

  // invoke "hop" move
  if (isLeft && isP1) {
    movePiece(from, to + 7, fromColor, toColor);
  } else if (isLeft && !isP1) {
    movePiece(from, to - 9, fromColor, toColor);
  } else if (!isLeft && isP1) {
    movePiece(from, to + 9, fromColor, toColor);
  } else {
    movePiece(from, to - 7, fromColor, toColor);
  }
  
}

// play simple animation to indicate a piece has been moved from a location to a location
void movePiece(int from, int to, uint32_t fromColor, uint32_t toColor) {

  state[from/8][from%8] = 0;

  if (toColor == p1Color) {
    state[to/8][to%8] = 1;
  } else if (toColor == p2Color) {
    state[to/8][to%8] = 2;
  } else if (toColor == p1KingColor) {
    state[to/8][to%8] = 3;
  } else {
    state[to/8][to%8] = 4;
  }
  
  board.setPixelColor(from, fromColor);
  board.show();
  delay(500);
  board.setPixelColor(to, toColor);
  board.show();
  
}

// set the all pieces to their position at the start of a game
void initPieces() {

  // set all squares to empty in state
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      state[i][j] = 0;
    }
  }

  board.clear();
  int i = 1;

  while (i < 24) {

    board.setPixelColor(i, p1Color);
    state[i/8][i%8] = 1;

    if (i == 7) {
      i++;
    } else if (i == getLEDAt('B', 6)) {
      i += 3; 
    } else {
      i += 2;
    }
    
  }

  i = getLEDAt('F', 0);

  while (i < 64) {

    board.setPixelColor(i, p2Color);
    state[i/8][i%8] = 2;

    if (i == getLEDAt('F', 6)) {
      i += 3;
    } else if (i == getLEDAt('G', 7)) {
      i++;
    } else {
      i += 2;
    }
    
  }

  board.show();
  
}

// is a move valid? NOTE: this function checks for nonsense moves, but does not eliminate the possibility of an invalid capture (see loop)
bool isValid(int from, int to, bool isP1, bool isKing) {

  if (isKing && isP1) {

    if ((from + 7 == to || from + 9 == to || from - 7 == to || from - 9 == to) && (state[from/8][from%8] == 1 || state[from/8][from%8] == 3) && (state[to/8][to%8] == 0 || state[to/8][to%8] == 2 || state[to/8][to%8] == 4)) { // kings can move freely along diagonals
      return true;
    } else {
      return false;
    }
    
  } else if (isKing && !isP1) { 

    if ((from + 7 == to || from + 9 == to || from - 7 == to || from - 9 == to) && (state[from/8][from%8] == 2 || state[from/8][from%8] == 4) && (state[to/8][to%8] == 0 || state[to/8][to%8] == 1 || state[to/8][to%8] == 3)) { // kings can move freely along diagonals
      return true;
    } else {
      return false;
    }
  
  } else if (isP1) {

    if ((from + 7 == to || from + 9 == to) && (state[from/8][from%8] == 1 || state[from/8][from%8] == 3) && (state[to/8][to%8] == 0 || state[to/8][to%8] == 2 || state[to/8][to%8] == 4)) { // to must be below and diagonal relative to from and empty
      return true;
    } else {
      return false;
    }
    
  } else {

    if ((from - 7 == to || from - 9 == to) && (state[from/8][from%8] == 2 || state[from/8][from%8] == 4) && (state[to/8][to%8] == 0 || state[to/8][to%8] == 1 || state[to/8][to%8] == 3)) { // to must be above and diagonal relative to from and empty
      return true;
    } else {
      return false;
    }
    
  }
  
}

// given that a move is valid, does it capture a piece?
bool isCapture(int from, int to, bool isP1, bool isKing) {

  if (isKing) {

    if (isP1) {

      if (from + 7 == to) { // moving down and left
  
        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 0 && state[(to/8) + 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p1KingColor, true, true); 
          return true;
        }
        
      } else if (from + 9 == to) { // moving down and right

        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 7 && state[(to/8) + 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p1KingColor, false, true);
          return true;
        }
         
      } else if (from - 9 == to) { // moving up and left
  
        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 0 && state[(to/8) - 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p1KingColor, true, false);
          return true;
        }
  
      } else { // moving up and right

        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 7 && state[(to/8) - 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p1KingColor, false, false);
          return true;
        }
        
      }

    } else {

      if (from + 7 == to) { // moving down and left
  
        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 0 && state[(to/8) + 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p2KingColor, true, true); 
          return true;
        }
        
      } else if (from + 9 == to) { // moving down and right

        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 7 && state[(to/8) + 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p2KingColor, false, true);
          return true;
        }
         
      } else if (from - 9 == to) { // moving up and left

        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 0 && state[(to/8) - 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p2KingColor, true, false);
          return true;
        }
  
      } else { // moving up and right

        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 7 && state[(to/8) - 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p2KingColor, false, false);
          return true;
        }
        
      }
      
    }
    
  } else {

    if (isP1) {
  
      // to has opponent's piece and there is an empty space to "hop" to
      if (from + 7 == to) { // moving down and left
  
        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 0 && state[(to/8) + 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p1Color, true, true); 
          return true;
        }
        
      } else { // moving down and right
  
        if ((state[to/8][to%8] == 2 || state[to/8][to%8] == 4) && to%8 != 7 && state[(to/8) + 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p1Color, false, true);
          return true;
        }
        
      }
      
    } else {
  
      // to has opponent's piece and there is an empty space to "hop" to
      if (from - 9 == to) { // moving up and left
  
        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 0 && state[(to/8) - 1][(to%8) - 1] == 0) {
          capturePiece(from, to, noColor, p2Color, true, false);
          return true;
        }
        
      } else { // moving up and right
  
        if ((state[to/8][to%8] == 1 || state[to/8][to%8] == 3) && to%8 != 7 && state[(to/8) - 1][(to%8) + 1] == 0) {
          capturePiece(from, to, noColor, p2Color, false, false);
          return true;
        }
        
      }
      
    }

  }

  return false;
  
}

// handles player's input and switches turn
void evalTurn(int from, int to, bool isP1) {

  bool isKing = false;

  if (state[from/8][from%8] == 3 || state[from/8][from%8] == 4) {
    isKing = true;
  }

  if (isValid(from, to, isP1, isKing)) {
        
        if (!isCapture(from, to, isP1, isKing)) { // if capture is not possible, do a normal move; if is capture is true, it invokes capturePiece (see isCapture())

          if (isP1) {

            if (state[to/8][to%8] != 2 && state[to/8][to%8] != 4) { // non-capture movement into an opponent's piece is invalid

              if ((to >= 56 && to <= 63) || state[from/8][from%8] == 3) {
                movePiece(from, to, noColor, p1KingColor);  
              } else {
                movePiece(from, to, noColor, p1Color);
              }
              
            }
            
          } else {

            if (state[to/8][to%8] != 1 && state[to/8][to%8] != 3) { // non-capture movement into an opponent's piece is invalid

              if ((to >= 0 && to <= 7) || state[from/8][from%8] == 4) {
                movePiece(from, to, noColor, p2KingColor);
              } else {
                movePiece(from, to, noColor, p2Color);
              }
              
            }
            
          }
            
        }

        p1Turn = !isP1; // other player's turn
        
   }
  
}

// if a player has no pieces left, the game ends with a repeating animation in the victorious player's color
void checkEnd() {

  if (p1Pieces == 0 || p2Pieces == 0) {

      board.clear();
      delay(200);

      if (p2Pieces == 0) {
        colorWipe(p1Color, 10);
      } else {
        colorWipe(p2Color, 10);
      }
      
  }
  
}

// animate a wipe effect of a single color over the game board
void colorWipe(uint32_t color, int wait) {
  
  for(int i = 0; i < board.numPixels(); i++) {
    
    board.setPixelColor(i, color);        
    board.show();                       
    delay(wait);                     
    
  }
  
}
