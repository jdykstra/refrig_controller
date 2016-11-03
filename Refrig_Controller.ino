/*    Refrig_Controller.ino - Refrigerator Power Controller Arduino sketch
 *     
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>

/*  Build options */
#define OFF_TIMEOUT     1*60*60   /* Max seconds we stay in off state */
#define COMPRESSOR_TIMEOUT 10*60  /* Min seconds after off before we can go to on */
#define WARBLE_PERIOD   500       /* Pilot "warble" period in ms. */

/* 
 *  IR Command codes.  Some of these are shared with the
 *  power controller, and ideally would be in a 
 *  shared header file.
 */
#define CODE_REFRIG_OFF   0x8322639C
#define CODE_REFRIG_ON    0x8322629D

/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_REFRIG      6       /* Refrigerator power */
#define PIN_LED         4       /* Extra LED */
#define PIN_PILOT       5       /* Pilot light */

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;


/* System states. */
enum {
  SYS_POWER_ON,
  SYS_POWER_OFF,
  SYS_COMPRESSOR_TIMEOUT
};

int currentSysState = SYS_POWER_ON;  
unsigned long offTime;                          // ms. counter when power turned off

void statePowerOff(int newSysState)
{
  switch (newSysState){
    case SYS_POWER_ON:
      digitalWrite(PIN_PILOT, HIGH);
      digitalWrite(PIN_REFRIG, LOW);
      break;
      
    case SYS_COMPRESSOR_TIMEOUT:
      /* 
       *  Nothing to do here.  We leave the power off,
       *  and the main loop will take care of 
       *  "warbling" the pilot LED.
       */
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void statePowerOn(int newSysState)
{
  switch (newSysState){
    case SYS_POWER_OFF:
      digitalWrite(PIN_PILOT, LOW);
      digitalWrite(PIN_REFRIG, HIGH);

      offTime = millis();
      break;
            
    case SYS_COMPRESSOR_TIMEOUT:
      /* This transition shouldn't happen. */
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}

void stateCompressorTimeout(int newSysState)
{
  switch (newSysState){
    case SYS_POWER_ON:
      digitalWrite(PIN_PILOT, HIGH);
      digitalWrite(PIN_REFRIG, LOW);
      break;
      
    case SYS_POWER_OFF:
      digitalWrite(PIN_PILOT, LOW);
      digitalWrite(PIN_REFRIG, HIGH);
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

    case SYS_COMPRESSOR_TIMEOUT:
      stateCompressorTimeout(newSysState);
      break;
  }
      
  currentSysState = newSysState;
}


void cmdPowerOn()
{
  if (millis() - offTime > 1000UL*COMPRESSOR_TIMEOUT)
    setSysState(SYS_POWER_ON);
  else
    setSysState(SYS_COMPRESSOR_TIMEOUT);
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

  digitalWrite(PIN_PILOT, HIGH);
  digitalWrite(PIN_REFRIG, LOW);

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
        case CODE_REFRIG_OFF:
          cmdPowerOff();
          break;
        case CODE_REFRIG_ON:
          cmdPowerOn();
          break;
      }
    }
    else if (myDecoder.protocolNum == 0){

      //  Attempt to kludge around an apparent bug that hangs
      //  the IR receiver after it gets an all-zero code.
      myReceiver.disableIRIn();    //  Restart receiver
      delay(500);
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }

  /*  Check for timer expiration. */
  switch (currentSysState){
    case SYS_POWER_ON:
      /*  Timer not used in this state. */
      break;

    case SYS_POWER_OFF:
      if (millis() - offTime > 1000UL*OFF_TIMEOUT)
        cmdPowerOn();
      break;

    case SYS_COMPRESSOR_TIMEOUT:
      if (millis() - offTime > 1000UL*COMPRESSOR_TIMEOUT)
        cmdPowerOn();
      break;
  }

  /*  If we're in SYS_COMPRESSOR_TIMEOUT, "warble" the pilot LED. */
  if (currentSysState == SYS_COMPRESSOR_TIMEOUT){
    int proportion = (millis() % WARBLE_PERIOD) * 0x1ff / WARBLE_PERIOD;
    if (proportion > 0xff)
      proportion = 0xff - (proportion - 0x100);
    analogWrite(PIN_PILOT, proportion);
  }

}

