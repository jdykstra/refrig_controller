/*    Refrig_Controller.ino - Refrigerator Power Controller Arduino sketch
 *     
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_REFRIG      6       /* Refrigerator power */
#define PIN_LED         4       /* Extra LED */
#define PIN_PILOT       5       /* Pilot light */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;


/* System states. */
enum {
  SYS_POWER_ON,
  SYS_POWER_OFF
};

int currentSysState = SYS_POWER_OFF;  // The way the pins come up

void statePowerOff(int newSysState)
{
  switch (newSysState){
    case SYS_POWER_ON:
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_REFRIG, ON);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void statePowerOn(int newSysState)
{
  switch (newSysState){
    case SYS_POWER_OFF:
      digitalWrite(PIN_PILOT, OFF);
      digitalWrite(PIN_REFRIG, OFF);
      break;
            
    default:
      Serial.println(F("Null state transition."));
  }
}


/* Move to the specified system state. */
void setSysState(int newSysState)
{
  switch (currentSysState){
    case SYS_POWER_OFF:
      statePowerOff(newSysState);
      break;

    case SYS_POWER_ON:
      statePowerOn(newSysState);
      break;
  }
      
  currentSysState = newSysState;
}


void cmdPowerOn()
{
  setSysState(SYS_POWER_ON);
}


void cmdPowerOff()
{
  setSysState(SYS_POWER_OFF);
}


void setup()
{
  /*  Configure serial port for debugging. */
  Serial.begin(9600);
  delay(2000);while(!Serial);     //delay for Leonardo

  /* 
   *  Configure input and output pins, except those managed
   *  by IRLib2.
   */
  pinMode(PIN_PILOT, OUTPUT);
  pinMode(PIN_REFRIG, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  setSysState(SYS_POWER_ON);

 
  /* Initialize IRLib2. */
  myReceiver.enableIRIn();
  
  Serial.println(F("Initialization complete."));
}


void loop() {

  /* Process IR commands. */
  if (myReceiver.getResults()) {
    myDecoder.decode();
    Serial.print(F("IR protocol "));
    Serial.print(myDecoder.protocolNum, DEC);
    Serial.print(F(" value "));
    Serial.println(myDecoder.value, HEX);
    if (myDecoder.protocolNum == NEC){
      switch (myDecoder.value){
        case 0x8322639C:
          cmdPowerOff();
          break;
        case 0x8322629D:
          cmdPowerOn();
          break;
      }
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }
}

