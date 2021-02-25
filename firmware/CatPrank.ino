/* 	Cat Prank
	Reuben Strangelove
	Fall 2019

	Device plays kittens meows until the sensor triggers
	the angry cat to startle your friend or foe.

	MCU:
		Arduino Mini (ATmega328P)
			Powered by 3.3v (XY-V17B 3.3v output supply)

	Sound module:
		XY-V17B
			Powered by 5.0v
			3.3v out by onboard LDO, 100ma
			Set up to use UART (9600 BAUD) for control
			3.3v TTL required (UART)

	Motion sensor:
		PIR (infrared) generic sensor module.
			Connected to 5.0v
			3.3v logic high/low output on trigger

	Light sensor:
		photoresistor (generic, likely 10k)
			low value: dark
			high value: light

	Notes:
    Device is powered on by a remote control relay.
		Beeps after angry cat to alert user to power down device.

   TODO:
    Query sound module for playing status for meow repeats.
    This will support future meows of different lengths.
*/

// Setting DEBUG to 1 prints debug data, sound will not work.
#define DEBUG 0

// Hardware pins.
#define PIN_MOTION_SWITCH 10
#define PIN_LIGHT_SWITCH 11
#define PIN_MOTION_SENSOR 12
#define PIN_PHOTO_RESISTOR A0

// Volumes
const int volumeMax = 30; // Per XY-V17B specifications.
const int volumeStart = 20;
const int volumeIncreaseTime = 10000; // milliseconds

// Sounds are called by alphabetical order.
// Where 1 is the first sound, 2 is the second sound, etc.
const int soundKittenMeow = 1;
const int soundAngryCat = 2;
const int soundBeep = 3;

// Sound lengths are required for sound repeats.
// TODO: query sound device for play status.
const int kittenPlayTimeMs = 19500;
const int delayBeforeBeepsStartMs = 30000;
const int delayBetweenBeepsMs = 10000;

// Running variables.
enum State {Meow, Angry, Wait, Beep} state;
int lightReadingTrigger;
bool motionTriggerFlag;
int volume;
unsigned long beepStart;

const int heartbeatFlashDelay = 250;
const int switchActivated = 0;

void flashOnboardLED()
{
  static unsigned long start = millis();
  if (millis() - start > heartbeatFlashDelay)
  {
    start = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

// Return 125% of the average n samples of the light sensor.
int captureLightTriggerValue()
{
  int lightReadingAccumulator = 0;
  for (int i = 0; i < 16; i++)
  {
    lightReadingAccumulator += analogRead(PIN_PHOTO_RESISTOR);
  }
  int lightReadingAverage = lightReadingAccumulator / 16;
  return lightReadingAverage + (lightReadingAverage / 4);
}

void setup()
{
  pinMode(PIN_MOTION_SWITCH, INPUT_PULLUP);
  pinMode(PIN_LIGHT_SWITCH, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_MOTION_SENSOR, INPUT);

  Serial.begin(9600); // per XY-V17B requirements
  if (DEBUG) Serial.println("Cat Prank starting up...");

  lightReadingTrigger = captureLightTriggerValue();

  // Give the XY-V17B to time startup.
  delay(100);

  state = Meow;
  motionTriggerFlag = false;
  playSound(soundKittenMeow);
  volume = volumeStart;
}

void loop()
{
  flashOnboardLED();

  int motionSwitch = digitalRead(PIN_MOTION_SWITCH);
  int lightSwitch = digitalRead(PIN_LIGHT_SWITCH);
  int lightReading = analogRead(PIN_PHOTO_RESISTOR);
  int motionReading = digitalRead(PIN_MOTION_SENSOR);

  if (DEBUG)
  {
    static unsigned long printStart = millis();
    if (millis() - printStart > 250)
    {
      printStart = millis();
      Serial.print("LS: ");
      Serial.println(lightSwitch);
      Serial.print("MS: ");
      Serial.println(motionSwitch);
      Serial.print("LT: ");
      Serial.println(lightReadingTrigger);
      Serial.print("MR: ");
      Serial.println(motionReading);
      Serial.print("Volume: ");
      Serial.println(volume);
    }
  }

  if (motionReading == 1)
  {
    motionTriggerFlag = true;
  }

  if (state == Meow)
  {
    static unsigned long meowStart = millis();
     
    static unsigned long volumeIncreaseStart = millis();
    if (millis() - volumeIncreaseStart > volumeIncreaseTime)
    {
      volumeIncreaseStart = millis();
      if (volume < volumeMax)
      {
        volume++;
        setVolume(volume);
      }
    }

    if (lightReading > lightReadingTrigger && lightSwitch == switchActivated)
    {
      state = Angry;
    }
    else if (motionTriggerFlag && motionSwitch == switchActivated)
    {
      state = Angry;
    }
    else if (millis() - meowStart > kittenPlayTimeMs)
    {
      meowStart = millis();
      playSound(soundKittenMeow);
    }
  }
  else if (state == Angry)
  {
    setVolume(volumeMax);
    playSound(soundAngryCat);
    beepStart = millis();
    state = Wait;
  }
  else if (state == Wait)
  {
    if (millis() - beepStart > delayBeforeBeepsStartMs)
    {
      state = Beep;
    }
  }
  else if (state == Beep)
  {   
    static unsigned long beepStart = millis();
    if (millis() - beepStart > delayBetweenBeepsMs)
    {
      beepStart = millis();
      playSound(soundBeep);
    }
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
