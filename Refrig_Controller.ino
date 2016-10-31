/*    Refrig_Controller.ino - Refrigerator Power Controller Arduino sketch
 *     
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_REFRIG      11      /* Refrigerator power */
#define PIN_LED         12      /* Extra LED */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;
IRsendNEC mySender;



/* System states. */
enum {
  SYS_IDLE,
  SYS_ACTIVATED
};

int currentSysState = SYS_IDLE;

void stateSysIdle(int newSysState)
{
  switch (newSysState){
    case SYS_LIVING_ROOM_UP:
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_SRC_PWR, ON);
      delay(7*1000);
      digitalWrite(PIN_LOW_PWR, ON);
      delay(1*1000);
      digitalWrite(PIN_HIGH_PWR, ON);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_SRC_PWR, ON);
      delay(7*1000);
      digitalWrite(PIN_HIGH_PWR, ON);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateActivated(int newSysState)
{
  switch (newSysState){
    case SYS_DOWN:
      digitalWrite(PIN_PILOT, OFF);
      digitalWrite(PIN_HIGH_PWR, OFF);
      digitalWrite(PIN_LOW_PWR, OFF);
      delay(5*1000);
      digitalWrite(PIN_SRC_PWR, OFF);

      /* Turn on the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_LOW_PWR, OFF);

      /* Turn on the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateOfficeUp(int newSysState)
{
  switch (newSysState){
    case SYS_LIVING_ROOM_UP:
      digitalWrite(PIN_LOW_PWR, ON);
      digitalWrite(PIN_SPKR_SW, OFF);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_DOWN:
      digitalWrite(PIN_PILOT, OFF);
      digitalWrite(PIN_HIGH_PWR, OFF);
      delay(5*1000);
      digitalWrite(PIN_SRC_PWR, OFF);
      digitalWrite(PIN_SPKR_SW, OFF);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


/* Move to the specified system state. */
void setSysState(int newSysState)
{
  switch (currentSysState){
    case SYS_DOWN:
      stateSysDown(newSysState);
      break;

    case SYS_LIVING_ROOM_UP:
      stateLivingRoomUp(newSysState);
      break;

    case SYS_OFFICE_UP:
      stateOfficeUp(newSysState);
      break;
  }

  currentSysState = newSysState;
}


void cmdActivate()
{
  switch (currentSysState){
    case SYS_DOWN:
      setSysState(SYS_ACTIVATED);
      break;

    case SYS_LIVING_ROOM_UP:
      setSysState(SYS_DOWN);
      break;

    case SYS_OFFICE_UP:
      setSysState(SYS_LIVING_ROOM_UP);
      break;
  }
}


void cmdIdle()
{
  setSysState(SYS_IDLE);
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
  pinMode(PIN_PWR_SW, INPUT_PULLUP);
  pinMode(PIN_OVER_SW, INPUT_PULLUP);
  pinMode(PIN_SRC_PWR, OUTPUT);
  pinMode(PIN_HIGH_PWR, OUTPUT);
  pinMode(PIN_LOW_PWR, OUTPUT);
  pinMode(PIN_SPKR_SW, OUTPUT);
 
  /* Initialize IRLib2. */
  /*  
   *   The IR repeater lengthens marks, at least partially due to
   *   the slow fall time of the line from the receivers to the 
   *   central unit due to line capacitance.  This compensory 
   *   value was determined empirically.
   */
  myReceiver.markExcess = 4*myReceiver.markExcess;
  
  IRLib_NoOutput();
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
        case 0x8322718E:
          cmdLivingRoomPower();
          break;
         case 0x83228C73:
           cmdOfficePower();
           break;
      }
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }
}

