/*
Lab Section B Group #1
Names: Elizabeth Connacher, Jesse Koontz, Chris Westerhoff, Jason Zhang
Lab 5

*/

#include <Arduino.h>
#include "SPI.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include "I2C.h"
#include "timer.h"
#include "switch.h"
#include "pwm.h"

//These are defined values from registers for dealing with the accelerometer
#define SLA 0x68  // MPU_6050 address with PIN AD0 grounded
#define PWR_MGMT  0x6B
#define WAKEUP 0x00
#define SL_MEMA_XAX_HIGH  0x3B
#define SL_MEMA_XAX_LOW   0x3C
#define SL_MEMA_YAX_HIGH  0x3D
#define SL_MEMA_YAX_LOW   0x3E
#define SL_MEMA_ZAX_HIGH  0x3F
#define SL_MEMA_ZAX_LOW   0x40

//Enums for the button states
typedef enum {
  WAIT_PRESS,
  DEBOUNCE_PRESS,
  WAIT_RELEASE,
  DEBOUNCE_RELEASE
} stateType;

//Enum for the face states
typedef enum {
  FROWN,
  SMILE
} faceType;

volatile stateType currentState = WAIT_PRESS;
volatile faceType faceState = SMILE; 
volatile boolean alarm = false;

int main () {

  Serial.begin(9600);  // use serial port for print in debug
  sei();

  int sound = 0;

  signed int X_val = 0;
  signed int Y_val = 0;
  signed int Z_val = 0;

  //Initialize componenets and setup
  initI2C();
  startI2C_Trans(SLA);
  write(PWR_MGMT);
  write(WAKEUP);
  stopI2C_Trans();

  initSwitchPB3();

  SPI_MASTER_Init(); // initialize SPI module and set the data rate

  initPWMTimer3();

// initialize 8 x 8 LED array (info from MAX7219 datasheet)
write_execute(0x0A, 0x08);  // brightness control
write_execute(0x0B, 0x07); // scanning all rows and columns
write_execute(0x0C,0x01); // set shutdown register to normal operation (0x01)
write_execute(0x0F, 0x00); // display test register - set to normal operation (0x01)
  while(true){
     //_delay_ms(1000);

    switch(currentState){
        case WAIT_PRESS: //Waiting for user to press the switch
        break;
      case DEBOUNCE_PRESS: //Debouncing the signal when user pressed the switch
        delayMs(1);
        currentState = WAIT_RELEASE;
        break;
      case WAIT_RELEASE: //Waiting for user to release the switch
          break;
      case DEBOUNCE_RELEASE: //Debouncing the signal when user let go of the switch
        delayMs(1);
        currentState = WAIT_PRESS;
        break;
      }
    switch(faceState){
      case FROWN:
        //SAD face
        alarm = true;
        write_execute(0x01, 0b00000000); // all LEDS in Row 1 are off
        write_execute(0x02, 0b00100100); // row 2 LEDS 
        write_execute(0x03, 0b00000000); // row 3 LEDS
        write_execute(0x04, 0b00000000); // row 4 LEDS
        write_execute(0x05, 0b00000000); // row 5 LEDS
        write_execute(0x06, 0b01111110); // row 6 LEDS
        write_execute(0x07, 0b01000010); // row 7 LEDS
        write_execute(0x08, 0b01000010); // row 8 LEDS
        //alarm = true;
      break;
      case SMILE:
        //Happy Face
        write_execute(0x01, 0b00000000); // all LEDS in Row 1 are off
        write_execute(0x02, 0b00100100); // row 2 LEDS 
        write_execute(0x03, 0b00000000); // row 3 LEDS
        write_execute(0x04, 0b01000010); // row 4 LEDS
        write_execute(0x05, 0b01000010); // row 5 LEDS
        write_execute(0x06, 0b01111110); // row 6 LEDS
        write_execute(0x07, 0b00000000); // row 7 LEDS
        write_execute(0x08, 0b00000000); // row 8 LEDS
      break;

    }
    //If the alarm is on, we can change the duty cycle sound
  if(alarm == true){
    _delay_ms(100);
    changeDutyCycle(sound);
    sound = sound + 2500;
  }
  else{
   changeDutyCycle(0);
  }

  // Accelerometer 
  X_val= read_data(); // read upper value
 
  read_from(SLA,SL_MEMA_XAX_LOW);
  X_val = (X_val << 8 )| read_data(); // append lower value
    read_from(SLA,SL_MEMA_XAX_HIGH);
  
  Y_val= read_data(); // read upper value
 
  read_from(SLA,SL_MEMA_YAX_LOW);
  Y_val = (Y_val << 8 )| read_data(); // append lower value
    read_from(SLA,SL_MEMA_YAX_HIGH);

  Z_val= read_data(); // read upper value
 
  read_from(SLA,SL_MEMA_ZAX_LOW);
  Z_val = (Z_val << 8 )| read_data(); // append lower value
    read_from(SLA,SL_MEMA_ZAX_HIGH);
  
  //Prints out the accelerometer data
  Serial.print("X: ");
  Serial.print(X_val);
  Serial.print(", Y: ");
  Serial.print(Y_val);
  Serial.print(", Z: ");
  Serial.println(Z_val);

  stopI2C_Trans();

  //Determines face state based on accelerometer position and rotation
  if(abs(Y_val) > 10000 || abs(Z_val) > 10000){
    faceState = FROWN;
  }
  else{
    faceState = SMILE;
  }

  }
}
//ISR
ISR(PCINT0_vect){
  if(currentState == WAIT_PRESS){
    currentState = DEBOUNCE_PRESS;
  }
  else if(currentState == WAIT_RELEASE){
    //If in wait release state and alarm is on, we turn off the alarm. Else turn alarm on
    if(alarm == true){
      alarm = false;
    }
    else {
      alarm = true;
    }
    currentState = DEBOUNCE_RELEASE;
  }
  
}
