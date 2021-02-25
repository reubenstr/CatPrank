/* Cat Prank
   Reuben Strangelove
   Fall 2019

   Device plays kittens meows until the sensor triggers the angry cat.   

   MCU: Arduino Mini (ATmega328P)
      Powered by 3.3v (XY-V17B 3.3v output supply)

   Sound module: XY-V17B
      Powered by 5.0v
      3.3v out by onboard LDO, 100ma
      Set up to use UART (9600 BAUD) for control
      3.3v TTL required (UART)

   PIR motion sensor:
      Connected to 5.0v
      3.3v logic high/low output on trigger

   Light sensor: photoresistor
      low value: dark
      high value: light

  Notes:
    Beeps after angry cat to alert user to power down device.
*/

// Setting DEBUG to 1 prints debug data, sound will not work.
// Program timings will be noticably distorted due to the time required to print debug statements.
#define DEBUG 0
#define SWITCH_TURNED_ON 0

// Heartbeat LED timings
#define HEARTBEAT_DELAY_MEOW 500
#define HEARBEAT_DELAY_ANGRY 250

// Hardware pins.
#define PIN_MOTION_SWITCH 10
#define PIN_LIGHT_SWITCH 11
#define PIN_MOTION_SENSOR 12
#define PIN_PHOTO_RESISTOR A0

// Volumes
#define VOLUME_MAX 30 // Per XY-V17B specifications.
#define VOLUME_START 20
#define VOLUME_INCREASE_TIME 10000 // milliseconds

// Sounds are called by alphabetical order.
// Where 1 is the first sound, 2 is the second sound, etc.
#define KITTEN_MEOW_SOUND 1
#define ANGRY_CAT_SOUND 2
#define BEEP_SOUND 3

// Sound lengths are required for sound repeats.
// TODO: query sound device for play status.
#define KITTEN_MEOW_TIME_MS 19500
#define DELAY_BEFORE_BEEPS_START_MS 60000
#define DELAY_BETWEEN_BEEPS_MS 15000

long oldMillis;
long ledCounter;
long timeCounter;
long volumeCounter;
enum State {Meow, Angry, Wait, Beep} state;
int lightReadingTrigger;
bool ledState;
bool HasPlayedOneFullLoop;
bool MotionTriggerFlag;
int ledDelay;
int volume;


void setup()
{
  pinMode(PIN_MOTION_SWITCH, INPUT);
  digitalWrite(PIN_MOTION_SWITCH, HIGH);       // turn on pullup resistors
  
  pinMode(PIN_LIGHT_SWITCH, INPUT);
  digitalWrite(PIN_LIGHT_SWITCH, HIGH);       // turn on pullup resistors
 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_MOTION_SENSOR, INPUT);

  Serial.begin(9600); // per XY-V17B requirements
  if (DEBUG) Serial.println("Cat Prank starting up...");

  // Set light reading trigger by adding 25% to the start up average light sensor reading.
  int lightReadingAccumulator = 0;
  for (int i = 0; i < 16; i++)
  {
    lightReadingAccumulator += analogRead(PIN_PHOTO_RESISTOR);
  }
  int lightReadingAverage = lightReadingAccumulator / 16;
  lightReadingTrigger = lightReadingAverage + (lightReadingAverage / 4);

  // Give the XY-V17B to time startup.
  delay(100);

  state = Meow;

  MotionTriggerFlag = false;

  // Change of plan: allow angry cat to trigger at anytime.
  // HasPlayedOneFullLoop = false;
  HasPlayedOneFullLoop = true;

  timeCounter = 0;
  playSound(KITTEN_MEOW_SOUND);

  ledDelay = 500;
    
  volume = VOLUME_START;
}


// Timer is called approximately every one millisecond.
void timerEvent()
{
  volumeCounter++;
  timeCounter++;

  // Heartbeat indicator.
  ledCounter++;
  if (ledCounter > ledDelay)
  {
    ledCounter = 0;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}


void loop()
{
  int motionSwitch = digitalRead(PIN_MOTION_SWITCH);
  int lightSwitch = digitalRead(PIN_LIGHT_SWITCH);
  
  int lightReading = analogRead(PIN_PHOTO_RESISTOR);
  int motionReading = digitalRead(PIN_MOTION_SENSOR);
 
  if (motionReading == 1)
  {
    MotionTriggerFlag = true;
  }

  if (DEBUG)
  {
    Serial.print("Light switch: ");
    Serial.println(lightSwitch);
   
    Serial.print("Motion switch: ");
    Serial.println(motionSwitch);
    
    Serial.print("Light trigger: ");
    Serial.println(lightReadingTrigger);
   
    Serial.print("Motion reading: ");
    Serial.println(motionReading);

    Serial.print("Volume: ");
    Serial.println(volume); 
  }  

  if (state == Meow)
  {  

    if (volumeCounter > VOLUME_INCREASE_TIME)
    {
      volumeCounter = 0;
      volume++;
      if (volume > VOLUME_MAX)
      {
        volume = VOLUME_MAX;
      }
      setVolume(volume); 
    }    
   
    if (lightReading > lightReadingTrigger && HasPlayedOneFullLoop == true && lightSwitch == SWITCH_TURNED_ON)
    {
      state = Angry;
    }     
    else  if (MotionTriggerFlag && HasPlayedOneFullLoop == true && motionSwitch == SWITCH_TURNED_ON)
    {
      state = Angry;
    }      
    else if (timeCounter > KITTEN_MEOW_TIME_MS)
    {
      timeCounter = 0;
      playSound(KITTEN_MEOW_SOUND);
      HasPlayedOneFullLoop = true;
    }
  }
  else if (state == Angry)
  {
    setVolume(VOLUME_MAX);
    playSound(ANGRY_CAT_SOUND);
    state = Wait;
    ledDelay = HEARBEAT_DELAY_ANGRY;
    timeCounter = 0;    
  }
  else if (state == Wait)
  {
    if (timeCounter > DELAY_BEFORE_BEEPS_START_MS)
    {
      state = Beep;      
    }
  }  
  else if (state == Beep)
  {
    // Beep periodically to alert user to power system down.
    if (timeCounter > DELAY_BETWEEN_BEEPS_MS)
    {
      timeCounter = 0;
      playSound(BEEP_SOUND);
    }
  }  

  // Timer.
  if (oldMillis + 1 <= millis())
  {
    oldMillis = millis();
    timerEvent();
  }
}

void play()
{
  Serial.write(0xAA);
  Serial.write(0x02);
  Serial.write(0x00);
  Serial.write(0xAC);
}

void stopPlayback()
{
  Serial.write(0xAA);
  Serial.write(0x04);
  Serial.write(0x00);
  Serial.write(0xAE);
}

void setSound(unsigned int s)
{
  Serial.write(0xAA);
  Serial.write(0x1F);
  Serial.write(0x02);
  Serial.write(s >> 8);
  Serial.write(s & 0xFF);
  Serial.write((0xAA + 0x1F + 0x02 + (s >> 8) + (s & 0xFF)) & 0x00FF);
}

void playSound(unsigned int s)
{
  Serial.write(0xAA);
  Serial.write(0x07);
  Serial.write(0x02);
  Serial.write((byte)s >> 8);
  Serial.write(s & 0x00FF);
  Serial.write((0xAA + 0x07 + 0x02 + ((byte)s >> 8) + (s & 0x00FF)) & 0x00FF);
}

// set volume: 0 = min; 30 = max per XY-V17B specifications
void setVolume(unsigned int volume)
{
  if (volume > 30) volume = 30;
  Serial.write(0xAA);
  Serial.write(0x13);
  Serial.write(0x01);
  Serial.write(volume);
  Serial.write((0xAA + 0x13 + 0x01 + volume) & 0x00FF);
}

void QueryPlayStatus()
{
  Serial.write(0xAA);
  Serial.write(0x01);
  Serial.write(0x00);
  Serial.write(0xAB);
}
