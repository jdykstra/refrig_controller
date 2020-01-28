/*    Refrig_Controller.ino - Refrigerator Power Controller Arduino sketch
 *     
 */


/*  Build options */
#define OFF_TIMEOUT     90*60     /* Max seconds we stay in off state */
#define COMPRESSOR_TIMEOUT 10*60  /* Min seconds after off before we can go to on */
#define WARBLE_PERIOD   500       /* Pilot "warble" period in ms. */

/*  Hardware definitions.  */
#define PIN_CMD_IN      2       /* IR receive input */
#define PIN_REFRIG      6       /* Refrigerator power */
#define PIN_LED         4       /* Extra LED */
#define PIN_PILOT       5       /* Pilot light */



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
   *  Configure input and output pins.
   */
  pinMode(PIN_CMD_IN, INPUT);
  pinMode(PIN_PILOT, OUTPUT);
  pinMode(PIN_REFRIG, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_PILOT, HIGH);
  digitalWrite(PIN_REFRIG, LOW);

  Serial.println(F("Initialization complete."));
}


void loop() {

  /*  Event loop */
  switch (currentSysState){
    case SYS_POWER_ON:
      if (digitalRead(PIN_CMD_IN) == LOW)
        cmdPowerOff();
      break;

    case SYS_POWER_OFF:
      if ((digitalRead(PIN_CMD_IN) == HIGH) || (millis() - offTime > 1000UL*OFF_TIMEOUT))
        cmdPowerOn();
      break;

    case SYS_COMPRESSOR_TIMEOUT:
      if (digitalRead(PIN_CMD_IN) == LOW)
        cmdPowerOff();
      else if (millis() - offTime > 1000UL*COMPRESSOR_TIMEOUT)
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
