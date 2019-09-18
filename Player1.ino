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

#include <LiquidCrystal.h>
#include <Wire.h>
LiquidCrystal lcd(6, 7, 5, 4, 3, 2);
const int numOfInputs = 4;
const int inputPins[numOfInputs] = {8, 9, 10, 11};
int inputState[numOfInputs];
int lastInputState[numOfInputs] = {LOW, LOW, LOW, LOW};
bool inputFlags[numOfInputs] = {LOW, LOW, LOW, LOW};
long lastDebounceTime[numOfInputs] = {0, 0, 0, 0};
long debounceDelay = 50;



const int boardSize = 8;
int fromXPOS = 0;
int fromYPOS = 0;
int toXPOS = 0;
int toYPOS = 0;
int inputSwitch = 0;
int endTurn = 0;
int XArray[boardSize] = {0, 1, 2, 3, 4, 5, 6, 7};
String YArray[boardSize] = {"A", "B", "C", "D", "E", "F", "G", "H"};
//String sendArray[4] = {0,0,0,0};


void setup() {
  lcd.begin(16, 2);
  //lcd.print("hello, world!");
  Wire.begin(0);
//  Wire.onReceive(receiveEvent);
  for (int i = 0; i < numOfInputs; i++) {
    pinMode(inputPins[i], INPUT);
    digitalWrite(inputPins[i], HIGH);
  }
}

//void receiveEvent(int ) {
//
//  
//  
//}

void requestEvent() {
  String sendArray = String(XArray[fromXPOS] + YArray[fromYPOS] + XArray[toXPOS] + YArray[toYPOS]);
  Wire.write(sendArray.c_str());

  //Wire.write(XArray[fromXPOS]);
  //Wire.write(YArray[fromYPOS]);
  //Wire.write(XArray[toXPOS]);
  //Wire.write(YArray[toYPOS]);
}


void setInputFlags() {
  for (int i = 0; i < numOfInputs; i++) {

    int reading = digitalRead(inputPins[i]);

    if (reading != lastInputState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (reading != inputState[i]) {
        inputState[i] = reading;
        if (inputState[i] == HIGH) {
          inputFlags[i] = HIGH;
        }
      }
    }


    lastInputState[i] = reading;

  }
}



void resolveInputFlags() {

  for (int i = 0; i < numOfInputs; i++) {
    if (inputFlags[i] == HIGH) {
      inputAction(i);
      inputFlags[i] = LOW;
      printScreen();
    }
  }
}



void inputAction(int input) {

  if (input == 0) {

    if (inputSwitch == 0) {

      fromXPOS++;

      if (fromXPOS > 7) {
        fromXPOS = 0;
      }
      if (fromXPOS < 0) {
        fromXPOS = 7;
      }
    }

    else if (inputSwitch == 1) {

      toXPOS++;

      if (toXPOS > 7) {
        toXPOS = 0;
      }
      if (toXPOS < 0) {
        toXPOS = 7;
      }
    }

  }

  else if (input == 1) {

    if (inputSwitch == 0) {

      fromYPOS++;

      if (fromYPOS > 7) {
        fromYPOS = 0;
      }
      if (fromYPOS < 0) {
        fromYPOS = 7;
      }
    }

    else if (inputSwitch == 1) {

      toYPOS++;

      if (toYPOS > 7) {
        toYPOS = 0;
      }
      else if (toYPOS < 0) {
        toYPOS = 7;
      }

    }

  }

  else if (input == 2) {

    if (inputSwitch == 0) {
      inputSwitch = 1;
    }

    else if (inputSwitch == 1) {
      inputSwitch = 0;
    }

    else {
      //Computational error
      exit;
    }
  }

  else if (input == 3) {

    //if (endTurn == 0){
    //    endTurn = 1;
    //}

    Wire.begin(0);
    Wire.onRequest(requestEvent);
    delay(500);
    Wire.end();



    inputSwitch = 0;
    fromXPOS = 0;
    fromYPOS = 0;
    toXPOS = 0;
    toYPOS = 0;


    //
    //Send signals to game-board arduino regarding moves
    //Send signals to other input Arduino to set endTurn variable back to '0'
    //

    //Serial.begin(9600)


  }

}



void printScreen() {
  lcd.clear();


  if ( inputSwitch == 0) {
    lcd.setCursor(0, 0);
    lcd.print(String("->FROM X:") + String(XArray[fromXPOS]) + String(" Y:") + String(YArray[fromYPOS]));
    lcd.setCursor(0, 1);
    lcd.print(String("  TO   X:") + String(XArray[toXPOS]) + String(" Y:") + String(YArray[toYPOS]));
  }

  else if ( inputSwitch == 1) {
    lcd.setCursor(0, 0);
    lcd.print(String("  FROM X:") + String(XArray[fromXPOS]) + String(" Y:") + String(YArray[fromYPOS]));
    lcd.setCursor(0, 1);
    lcd.print(String("->TO   X:") + String(XArray[toXPOS]) + String(" Y:") + String(YArray[toYPOS]));
  }
  lcd.print(" ");

}

void loop() {
  //printScreen();

  //if( endTurn == 1){
  //  delay(200);
  //}
  //else if( endTurn == 0){
  setInputFlags();
  resolveInputFlags();
  //}
}
