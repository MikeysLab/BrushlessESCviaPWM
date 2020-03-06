/*
 Name:		EscPWMTesting.ino
 Created:	2/15/2020 10:10:00 PM
 Author:	Mikey
*/

//these values copied from the BlHeli configuration
#define ESC_MIN_THROTTLE		        1070
#define ESC_MAX_THROTTLE		        1852
#define ESC_REVERSE_THROTTLE	        1488

#define ESC_ARM_SIGNAL			        1000
#define ESC_ARM_TIME			        2000

#define ESC1_PIN                        8

#define ESC_DEADZONE_RANGE              100
#define ESC_FLUTTER_RANGE               10

//ibus defines
#define IBUS_BUFFSIZE			        32    // Max iBus packet size (2 byte header, 14 channels x 2 bytes, 2 byte checksum)
#define IBUS_MAX_CHANNELS		        10    // My TX only has 10 channels, no point in polling the rest
#define IBUS_THROTTLE_CHANNEL           1

#include <Servo.h>

static uint8_t ibusIndex = 0;
static uint8_t ibus[IBUS_BUFFSIZE] = { 0 };
static uint16_t rcValue[IBUS_MAX_CHANNELS];

double rcScaleValue = 1;
Servo esc;

void setup()
{
    Serial.begin(115200);

    Identify();

    Serial.println("Initializing");

    esc.attach(ESC1_PIN, ESC_MIN_THROTTLE, ESC_MAX_THROTTLE);

    InitESC();

    rcScaleValue = DetermineRCScale();

    Serial.println("Done");
}

void loop()
{
    if (ReadRx())
    {
        // cleaner version: WriteSpeed(ScaleRCInput(rcValue[IBUS_THROTTLE_CHANNEL]));

        int valueRaw = rcValue[IBUS_THROTTLE_CHANNEL];
        int valueSpeed = ScaleRCInput(valueRaw);
        WriteSpeed(valueSpeed);
    }
}

void Identify()
{
    Serial.println("Firmware:		EscPWMTesting.ino 2/28/2020 2:27");
}

double DetermineRCScale()
{
    double range = ESC_MAX_THROTTLE - ESC_MIN_THROTTLE;
    Serial.println(range / 1000);
    return (range / 1000); //1000 is the normal range for PWM signals (1000us to 2000us)
}

int ScaleRCInput(int rcValue)
{
    return ((rcValue - 1000) * rcScaleValue) + ESC_MIN_THROTTLE;
}

void InitESC()
{
    esc.writeMicroseconds(ESC_ARM_SIGNAL);
    unsigned long now = millis();
    while (millis() < now + ESC_ARM_TIME)
    {
    }
}

bool isDeadzone(int speed)
{
    if (speed >= (ESC_REVERSE_THROTTLE - ESC_DEADZONE_RANGE) && speed <= (ESC_REVERSE_THROTTLE + ESC_DEADZONE_RANGE))
    {
        return true;
    }
    return false;
}

void WriteSpeed(int speed)
{
    if (isDeadzone(speed)) speed == ESC_REVERSE_THROTTLE;

    int curSpeed = esc.readMicroseconds();

    if (curSpeed >= (speed - ESC_FLUTTER_RANGE) && curSpeed <= (speed + ESC_FLUTTER_RANGE))
    {
        return;
    }

    esc.writeMicroseconds(speed);
}

bool ReadRx()
{
    if (Serial.available())
    {
        uint8_t val = Serial.read();
        // Look for 0x20 0x40 as start of packet
        if ((ibusIndex == 0 && val != 0x20) || (ibusIndex == 1 && val != 0x40))
        {
            return false;
        }
        
        if (ibusIndex == IBUS_BUFFSIZE)
        {
            ibusIndex = 0;
            memcpy(rcValue, ibus + 2, IBUS_MAX_CHANNELS * sizeof(uint16_t));
            return true;
        }
        else
        {
            ibus[ibusIndex] = val;
            ibusIndex++;
            return false;
        }
    }
}
