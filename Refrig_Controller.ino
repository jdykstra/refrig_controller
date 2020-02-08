/*    Refrig_Controller.ino - Refrigerator Power Controller Arduino sketch

*/

/*  Build options */
#define MAXIMUM_OFF_TIMEOUT     2*60     /* Max seconds we stay in commanded off state */ ////////////
#define COMPRESSOR_TIMEOUT      10*60     /* Min seconds after off before we can go to on */ 
#define WARBLE_PERIOD           500       /* Pilot "warble" period in ms. */

/*  Hardware definitions.  */
#define PIN_CMD_IN              2         /* IR receive input */
#define PIN_REFRIG              6         /* Refrigerator power */
#define PIN_LED                 4         /* Extra LED */
#define PIN_PILOT               5         /* Pilot light */

/* Command input is low when active.  */
#define CMD_ACTIVE              LOW       /* Turn refrigerator off */
#define CMD_INACTIVE            HIGH      /* Turn refrigerator on */

/* System states. */
enum {
  SYSTEM_IDLE_ON,
  SYSTEM_COMMANDED_OFF,
  SYSTEM_COMPRESSOR_PROTECT_OFF,
  SYSTEM_MAX_TIME_ON
};

/* System state names, for debugging messages. */
char *stateNames[] = {
  "SYSTEM_IDLE_ON",
  "SYSTEM_COMMANDED_OFF",
  "SYSTEM_COMPRESSOR_PROTECT_OFF",
  "SYSTEM_MAX_TIME_ON"
};

/* Refrigerator power states */
enum {
  FRIDGE_POWER_ON,
  FRIDGE_POWER_OFF
};

/* Command input states */
enum {
  COMMAND_INPUT_INACTIVE,
  COMMAND_INPUT_ACTIVE
};

/* Global variables */
int currentSysState;
int currentFridgeState;
int currentInputState;
unsigned long fridgeOffTimestamp;                 /* ms. counter when power turned off */


void fridgePowerOn()
{
  if (currentFridgeState == FRIDGE_POWER_OFF) {
    digitalWrite(PIN_PILOT, HIGH);
    digitalWrite(PIN_REFRIG, LOW);

    currentFridgeState = FRIDGE_POWER_ON;
  }
}


void fridgePowerOff()
{
  if (currentFridgeState == FRIDGE_POWER_ON) {
    digitalWrite(PIN_PILOT, LOW);
    digitalWrite(PIN_REFRIG, HIGH);

    currentFridgeState = FRIDGE_POWER_OFF;
    fridgeOffTimestamp = millis();
  }
}


/* Move to the specified system state. */
void moveToState(int newSysState)
{
  switch (newSysState) {
    case SYSTEM_IDLE_ON:
    case SYSTEM_MAX_TIME_ON:
      fridgePowerOn();
      break;

    case SYSTEM_COMMANDED_OFF:
    case SYSTEM_COMPRESSOR_PROTECT_OFF:
      fridgePowerOff();
      break;
  }

  currentSysState = newSysState;
  Serial.println(stateNames[currentSysState]);
}


void setup()
{
  /*  Configure serial port for debugging. */
  Serial.begin(9600);
  delay(2000); while (!Serial);   //delay for Leonardo

  /*
      Configure input and output pins.
  */
  pinMode(PIN_CMD_IN, INPUT);
  pinMode(PIN_PILOT, OUTPUT);
  pinMode(PIN_REFRIG, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_PILOT, HIGH);
  digitalWrite(PIN_REFRIG, LOW);
  currentFridgeState = FRIDGE_POWER_ON;

  currentSysState = SYSTEM_IDLE_ON;
  currentInputState = COMMAND_INPUT_INACTIVE;

  Serial.println("Setup() complete.");
}


void loop() {

  /*  Event loop */
  if (digitalRead(PIN_CMD_IN) == CMD_ACTIVE) {
    if (currentInputState == COMMAND_INPUT_INACTIVE) {

      /*
         Command input just went active.

         We're seeing false triggering, perhaps because of noise.
         Work around it by requiring the command input to stay active
         for one second before acting on it.
      */
      unsigned long activeTImestamp = millis();
      while ((millis() - activeTImestamp) < 1 * 1000UL)
        if (digitalRead(PIN_CMD_IN) == CMD_INACTIVE) {
          Serial.println(F("Spurious off command ignored."));
          break;
        }
      if ((millis() - activeTImestamp) >= 1 * 1000UL)
        moveToState(SYSTEM_COMMANDED_OFF);

      currentInputState = COMMAND_INPUT_ACTIVE;
    }
  }
  else if (currentInputState == COMMAND_INPUT_ACTIVE) {

    /* Command input just went inactive. */
    if (millis() - fridgeOffTimestamp > 1000UL * COMPRESSOR_TIMEOUT)
      moveToState(SYSTEM_IDLE_ON);
    else
      moveToState(SYSTEM_COMPRESSOR_PROTECT_OFF);

    currentInputState = COMMAND_INPUT_INACTIVE;
  }

  /* Handle end of compressor protect timeout. */
  if ((currentSysState == SYSTEM_COMPRESSOR_PROTECT_OFF) &&
      (millis() - fridgeOffTimestamp > 1000UL * COMPRESSOR_TIMEOUT))
    moveToState(SYSTEM_IDLE_ON);


  /* Handle maximum off time timeout. */
  if ((currentSysState == SYSTEM_COMMANDED_OFF) &&
      (millis() - fridgeOffTimestamp > 1000UL * MAXIMUM_OFF_TIMEOUT))
    moveToState(SYSTEM_MAX_TIME_ON);

  /*  If we're in SYSTEM_COMPRESSOR_PROTECT_OFF, "warble" the pilot LED. */
  if (currentSysState == SYSTEM_COMPRESSOR_PROTECT_OFF) {
    int proportion = (millis() % WARBLE_PERIOD) * 0x1ff / WARBLE_PERIOD;
    if (proportion > 0xff)
      proportion = 0xff - (proportion - 0x100);
    analogWrite(PIN_PILOT, proportion);
  }
}
