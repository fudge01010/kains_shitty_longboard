/**

   Beginning of Longboard Remote transmitter
   Jack Fuge
   Green = Y. yellow = C. blue = Z. red = +ve. black = -ve.
*/


// Libraries
#include <SPI.h>
#include "RF24.h"
#include "printf.h"
#include "LowPower.h"
#include "FastLED.h"

//Are we debugging!?!?!?!?!
//#define DEBUG
//comment ^^^^^^^ out if we aren't debugging

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x) 
#else
#define DEBUG_PRINT(x)
#endif

//Our potentiometer:
//int speedPot = A0;
int speedValue = 0;

int receiverVals[3];
int prevVals[3];

int sleepCount = 0;

//hardcoded remote max/mins
#define xMax 1024
#define xMin 0
#define yMax 1024
#define yMin 0

//sleeptimeout = ~20 x (num of seconds till sleep)
#define sleepTimeout 1200

#define ADCPin 6
#define cButton 8
#define zButton 7
#define led_Power 4
#define ledData 3
#define xAxis A0
#define yAxis A1
#define batPin A2

CRGB led[1];

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 (CE & CS)
RF24 radio(9, 10);

// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("\nLongboard RF Transmitter test\n\rPot on A0, chip on 9 & 10 (CE & CS)");
  printf_begin();                               //begin the radio class.

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);                //set TX power level. MIN, LOW, HIGH, MAX. trying low for batt saving.
  if ( ! radio.setDataRate( RF24_250KBPS )) {
    //data rate failed to change.
    DEBUG_PRINT("data rate failed to change. not a + unit.");
  }
  DEBUG_PRINT("Radio has begun\n\r");
  // Open pipes to other nodes for communication
  radio.openWritingPipe(pipe);
  DEBUG_PRINT("Pipes are opened.\n\r");
  radio.printDetails();

  //setup the z and c buttons with inbuilt pullups
  pinMode(cButton, INPUT_PULLUP);
  pinMode(zButton, INPUT_PULLUP);

  //set up DIO to turn on the ADC, and do just that:
  pinMode(ADCPin, OUTPUT);
  digitalWrite(ADCPin, HIGH);

  //ENABLE SLEEP - this enables the sleep mode
  SMCR |= (1 << 2); //power down mode
  SMCR |= 1;//enable sleep
  
  pinMode(led_Power, OUTPUT);
  ledPower(true);
  FastLED.addLeds<WS2812B, ledData, GRB>(led, 1);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,20); 
  FastLED.setBrightness(255);
  DEBUG_PRINT("led shit done, here comes animation");
  ledanim();
}

bool sleepflash = false;
void loop(void)
{
  //read the y axis into the first byte of arraya

  receiverVals[0] = analogRead(yAxis);
  //receiverVals[0] = map(receiverVals[0], yMin, yMax, 0, 1024);      //to add code later after testing range of thumbstick

  if (digitalRead(cButton)) {
    //c button not pressed
    receiverVals[1] = 2222;
  } else {
    //c button pressed
    receiverVals[1] = 4444;
  }

  if (digitalRead(zButton)) {
    //z button not pressed
    receiverVals[2] = 6666;
  } else {
    //z button pressed
    receiverVals[2] = 8888;
  }

  //we have array of vals, send that shit
  radio.write(&receiverVals, sizeof(receiverVals));
  DEBUG_PRINT(receiverVals[0]);
  DEBUG_PRINT(receiverVals[1]);
  DEBUG_PRINT(receiverVals[2]);

  //check if values are in defaults:
  if ( (receiverVals[0] >= 510 && receiverVals[0] <= 530 ) && (receiverVals[1] == 2222) && (receiverVals[2] == 6666) ) {
    //buttons are the same and joystick is pretty much in the center.

    //copy current vals to prev vals
    //    memcpy(prevVals, receiverVals, 3*sizeof(int));

    //increment the "shit hasn't happened in a while" counter
    sleepCount++;

    //if we have 15 secs till sleep, flash the light
    if ( sleepCount == (sleepTimeout - 200) )
    {
      //15 secs to go, turn the red light on
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      DEBUG_PRINT("SLEEPING SOON!!!!!!!!!!!!!!!!!!!!!");
      sleepflash = true;
      ledPower(true);
    }

    if (sleepflash) {
      led[0].red == 255;
      EVERY_N_MILLISECONDS(250) {
        DEBUG_PRINT("in loop");
        DEBUG_PRINT(led[0].red);
        if (led[0].red <= 1)
        led[0].red = 255;
        FastLED.show();
        
      } else {
        led[0].red = 0;
        FastLED.show();
      }
    }

    if ( sleepCount >= sleepTimeout )
    {
      
      //time to sleep fam!
      setupWDTforSleep();
      
    }
  } else {
    sleepCount = 0;
    sleepflash = false;
  }
  
  //delay for 50ms. ~18-20Hz. to be confirmed if this is suitable.
  delay(50);
}


void setupWDTforSleep(void) {
  //turn off NRF radio - should go into deep sleep and consume ~0.9uA
  radio.powerDown();
  DEBUG_PRINT("Radio gone to sleep. good night!");

  //end serial:
  Serial.end();

  //  //TO WAKE UP ADC: ADCSRA |= (1 << 7)
  //  ADCSRA &= ~(1 << 7);
  //  //SETUP WATCHDOG TIMER
  //  WDTCSR = (24);//change enable and WDE - also resets
  //  WDTCSR = (33);//prescalers only - get rid of the WDE and WDCE bit
  //  WDTCSR |= (1<<6);//enable interrupt mode

  //turn off LED
  led[0] = 0x000000;
  FastLED.show();
  ledPower(false);
  sleepflash = false;

  //NOW GO TO SLEEP:
  goToSleep();
}

void goToSleep() {
//  //TO WAKE UP ADC: ADCSRA |= (1 << 7)
//  ADCSRA &= ~(1 << 7);
//  //SETUP WATCHDOG TIMER
//  WDTCSR = (24);//change enable and WDE - also resets
//  WDTCSR = (33);//prescalers only - get rid of the WDE and WDCE bit
//  WDTCSR |= (1 << 6); //enable interrupt mode
//
//  //BOD DISABLE - this must be called right before the __asm__ sleep instruction
//  MCUCR |= (3 << 5); //set both BODS and BODSE at the same time
//  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //then set the BODS bit and clear the BODSE bit at the same time
//  __asm__  __volatile__("sleep");//in line assembler to go to sleep

//  Turn off that MF adc enable pin
    digitalWrite(ADCPin, LOW);
    
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
    //turn on ADCpin and wait for a few uS to stabilize
    digitalWrite(ADCPin, HIGH);
    delayMicroseconds(10);
    if ( ((!digitalRead(cButton)) && (!digitalRead(zButton)) ) )
  {
    digitalWrite(ADCPin, LOW);
    checkWakeup();
  } else {
    //nope, both buttons aren't held. back to sleep!
    digitalWrite(ADCPin, LOW);
    goToSleep();
  }
}

void comeOutOfSlumber() {
  sleepCount = 0;
  //buttons held or whatever, we ready to wake up!
  Serial.begin(115200);
  DEBUG_PRINT("rise and shine sleepy-head!");
  radio.powerUp();
  DEBUG_PRINT("radio rebooted.");
  ADCSRA |= (1 << 7);
  DEBUG_PRINT("adc re-booted.");
  WDTCSR = 0x00;
  DEBUG_PRINT("watchdog disabled.");
  ledanim();
}

void checkWakeup() {
    ////both buttons pressed on wakeup!
    delay(3000);
    digitalWrite(ADCPin, HIGH);
    delayMicroseconds(10);
    if ( ((!digitalRead(cButton)) && (!digitalRead(zButton)) ) )
    {
      //both buttons still pressed! wake tf up
      comeOutOfSlumber();
    } else {
      //nope, buttons released. back to sleep.
      goToSleep();
      digitalWrite(ADCPin, LOW);
    }
}
//
////WDT wakeup function called after ~8sec of sleep:
//ISR(WDT_vect) {
//  ADCSRA |= (1 << 7);
//  WDTCSR = 0x00;
//  digitalWrite(13, HIGH);
//  delay(250);
//  digitalWrite(13, LOW);
//  if ( ((!digitalRead(cButton)) && (!digitalRead(zButton)) ) )
//  {
//    checkWakeup();
//  } else {
//    //nope, both buttons aren't held. back to sleep!
////    goToSleep();
//  }
//}

void ledPower(bool ledPower) {
  if (ledPower) {
    //turn them on
    digitalWrite(led_Power, HIGH);
    delay(2);
    //allow power to settle
  } else {
    //turn them off
    digitalWrite(led_Power, LOW);
  }
}

void ledanim () {
  //corny ass little led boot flash thingy
  ledPower(true);
  led[0].setRGB(255,255,255);
  FastLED.show();
  delay(50);
  led[0].setRGB(0,0,0);
  FastLED.show();
  delay(250);
  led[0].setRGB(255,255,255);
  FastLED.show();
  delay(50);
  led[0].setRGB(0,0,0);
  FastLED.show();
  delay(450);
  for (int i=0; i<=255; i+=2) {
    led[0].setRGB(i/2,i,i);
    FastLED.show();
    delay(8);
  }
  delay(1500);
  led[0].setRGB(0,0,0);
  FastLED.show();
  ledPower(false);
}

