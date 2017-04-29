
/**
 *
 * Beginning of Longboard board receiver
 * Jack Fuge 
 */

#include <SPI.h>
#include "enums.h"
#include "RF24.h"
#include "printf.h"
#include <Servo.h>

//Are we debugging!?!?!?!?!
#define DEBUG
//comment ^^^^^^^ out if we aren't debugging

#ifdef DEBUG
 #define DEBUG_PRINT(x) Serial.println(x)
#else
 #define DEBUG_PRINT(x)
#endif

//pin definitons:
#define escPin A0

//define set vals:
#define bootVal 1000
#define coastVal 1117
#define powerLevel 60
#define maxVal 1950*powerLevel
#define brakeVal 1020

//set up enum for states.
//enum boardStates {
//  wait,   //idle state (can't use idle word =[ )
//  coast,  //coast state
//  accel,  //accelerating / driving but no cruise control
//  cruise, //cruise control is set
//  brake,  //brake state
//  boot    //booting up state.
//};

enum boardStates currentState = boot;
enum boardStates nextState;

//globals
int speedValue = 0;
int cruiseValue = 0;
int receiverVals[3];
bool zButton;
bool cButton;
bool cruiseHeld = false;

bool inZWait = false;
unsigned long brakeCountdown;

//---------------set up objects:--------------------

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 (CE & CS)
RF24 radio(9,10);
// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

//set up servo object (esc pin for control)
Servo myESC;  // create servo object to control the ESC

void setup() {
  myESC.attach(escPin);
  Serial.begin(115200);
  Serial.println("Longboard RF RX\n\r ESC on pin 5, chip on 9 & 10 (CE & CS)");
  printf_begin();
  radio.begin();
  DEBUG_PRINT("Radio has begun");
  if ( ! radio.setDataRate( RF24_250KBPS )) {
    //data rate failed to change.
    DEBUG_PRINT("data rate failed to change. not a + unit.");
  }

  radio.openReadingPipe(1,pipe);
  DEBUG_PRINT("Pipes are opened.");
  radio.startListening();
  radio.printDetails();
}

String debugStr;
void loop() {
  
  if(radio.available()){
    while (radio.available()) {
      radio.read(&receiverVals,sizeof(receiverVals));
    }
//    for (int i = 0; i < 3; i++) { DEBUG_PRINT(receiverVals[i]); DEBUG_PRINT(i); }
    //if we have new data, update the cached button states:
    if (receiverVals[1] == 2222) {
      //c button not pressed
      cButton = false;
    } else if (receiverVals[1] == 4444) {
      //c button pressed.
      cButton = true;
    }
    if (receiverVals[2] == 6666) {
      //z Button not pressed
      zButton = false;
    } else if (receiverVals[2] == 8888 ) {
      //z button pressed.
      zButton = true;
    }
  } //have received all data in buffer now.

  #ifdef DEBUG
  //only assemble string if we need to.
    if (currentState != nextState) {
    //only print if differs otherwise console spam.
      debugStr = "current state is: ";
      debugStr.concat(stringFromEnum(nextState));
      DEBUG_PRINT(debugStr); //tell us verbosely current state.
  }
  #endif
  
  switch(currentState) {
    //
    case coast:
      myESC.writeMicroseconds(coastVal);
      if (zButton) nextState = coast;
      if (zButton && (receiverVals[0] >=  525)) nextState = accel;
      if (receiverVals[0] <= 300) nextState = brake;
      if (!zButton) {
        if (inZWait) {
          //we already in countdown
          if (millis() - brakeCountdown < 3000) {
            //we still have a chance to press Z! Have we?
            if (zButton) {
              inZWait = false;
              nextState = coast;
              brakeCountdown = 0;
            }
          } else {
            //over 3 seconds have passed. brake fam.
            nextState = brake;
            inZWait = false;
          }
        } else {
          //no z button pressed. start z countdown and set z-wait state. go back to coast for now.
          brakeCountdown = millis();
          inZWait = true;
          nextState = coast;
        }
      }
      //show lights maybe?
      break;
      
    case accel:
      //linear scaling of pot to duty cycle atm. to change l8r.
      speedValue = map( constrain(receiverVals[0], 515, 1024) , 515, 1024, 1116, 1950); //wtf is even this line, give me object oriented pls
//      DEBUG_PRINT(speedValue);
      myESC.writeMicroseconds(speedValue);
      if (zButton) nextState = accel;
      if ((receiverVals[0] <= 300) && zButton) nextState = brake;
      if (cButton && zButton) {
        nextState = cruise;
        cruiseValue = speedValue;
        cruiseHeld = true;
      }
      if (!zButton) nextState = coast;
      break;

    case cruise:
      //if cruise button has been held, see if it has been released yet:
      if (cruiseHeld) {
        //button was pressed, see if it's been released before we do any cruise mod shit:
        if (cButton) {
          //c button still pressed. write the set cruise speed and be done with it. no mods allowed.
          myESC.writeMicroseconds(cruiseValue);
        } else {
          //c button has been released! update the held state and wait for next iteration for memes.
          cruiseHeld = false;
        }
        if (!zButton) nextState=coast;
      } else {
        //allow thumbstick to fine tune cruise value
        speedValue = cruiseValue + map( constrain(receiverVals[0], 0, 1024), 0, 1024, -250, 250);
        //we have a +/- 250 power value. add that to current speed value and write that (without overwriting set cruise value in memory.
        myESC.writeMicroseconds(speedValue);
        if (cButton) {
          //if c button pressed, save new value into memory.
          cruiseValue = speedValue;
          //update c button held state:
          cruiseHeld = true;
        }
        if (!zButton) nextState = coast;
      }
      break;

    case brake:
      //brake state. here be dragons. make sure we reaaaally need to be here to get here.
      myESC.writeMicroseconds(brakeVal);
      if (zButton) nextState = coast;
      if (zButton && (receiverVals[0] <= 300) ) nextState = brake;
      break;

    case boot:
      //boot that shit fam.
      myESC.writeMicroseconds(1030);
      //wait 5 secs for ESC to initialize.
      DEBUG_PRINT("SLEEPIN FOR BOOT");
      delay(7500);
      DEBUG_PRINT("OUT OF BOOT");
      nextState = coast;
      break;
      
    default:
      myESC.writeMicroseconds(coastVal);
      nextState = coast;
      break;
  }

   currentState = nextState;
}

//    myESC.writeMicroseconds(map(constrain(receiverVals[0],512,1023),512, 1023, 1000, 1950));
