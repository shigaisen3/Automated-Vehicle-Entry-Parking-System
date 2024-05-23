#include <Arduino.h>
#include <Servo.h>
#include "string.h"
#define OPEN_SERVO 80, 180
#define CLOSE_SERVO 180, 80
#define ULTRASONIC_DISTANCE 5
#define ULTRASONIC_THRESHOLD 15
#define LED_ON(pin) digitalWrite(pin, HIGH)
#define LED_OFF(pin) digitalWrite(pin, LOW)
#define GREEN_LED 31
#define RED_LED 30
typedef struct _ultrasonic
{
    unsigned trigger_pin;
    unsigned echo_pin;
} Ultrasonic;
const byte displayPins[] = {22, 23, 24, 25, 26, 27, 28, 29, 30};
const byte charMap[][7] = {
    // g, f, a, b, e, d, c
    {0, 1, 1, 1, 1, 1, 1}, // 0
    {0, 0, 0, 1, 0, 0, 1}, // 1
    {1, 0, 1, 1, 1, 1, 0}, // 2
    {1, 0, 1, 1, 0, 1, 1}, // 3
    {1, 1, 0, 1, 0, 0, 1}, // 4
    {1, 1, 1, 0, 0, 1, 1}, // 5
    {1, 1, 1, 0, 1, 1, 1}, // 6
    {0, 0, 1, 1, 0, 0, 1}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 0, 1}  // 9
};
Ultrasonic entryEntranceUltrasonic = {2, 3};
Ultrasonic entryExitUltrasonic = {4, 5};
Ultrasonic exitEntranceUltrasonic = {6, 7};
Ultrasonic exitExitUltrasonic = {8, 9};

Servo entryServo;
Servo exitServo;
void setup()
{
    Serial.begin(115200);
    entryServo.attach(10);
    exitServo.attach(11);
    pinMode(entryEntranceUltrasonic.trigger_pin, OUTPUT);
    pinMode(entryEntranceUltrasonic.echo_pin, INPUT);
    pinMode(entryExitUltrasonic.trigger_pin, OUTPUT);
    pinMode(entryExitUltrasonic.echo_pin, INPUT);
    pinMode(exitEntranceUltrasonic.trigger_pin, OUTPUT);
    pinMode(exitEntranceUltrasonic.echo_pin, INPUT);
    pinMode(exitExitUltrasonic.trigger_pin, OUTPUT);
    pinMode(exitExitUltrasonic.echo_pin, INPUT);
    for (int i = 0; i < 7; i++)
    {
        pinMode(displayPins[i], OUTPUT);
    }
    LED_ON(30);
    LED_OFF(31);
}

typedef enum _states
{
    IDLE,
    ENTRY,
    ENTRY_OPEN,
    ENTRY_CLOSE,
    EXIT,
    EXIT_OPEN,
    EXIT_CLOSE,
    FULL
} States;
void writeToDisplay(int num)
{
    char c = num + '0'; 
    int index = c - '0';

    for (int i = 0; i < 7; i++)
    {
        digitalWrite(displayPins[i], charMap[index][i]);
    }
}
void sweepServo(Servo &servo, unsigned from, unsigned to)
{
    if (from > to)
    {
        for (int i = from; i > to; i--)
        {
            servo.write(i);
            delay(10);
        }
    }
    else if (from < to)
    {
        for (int i = from; i < to; i++)
        {
            servo.write(i);
            delay(10);
        }
    }
    else
    {
        servo.write(from);
        delay(10);
    }
}
long measureDistance(Ultrasonic &ultrasonic)
{
    long duration, distance_cm;

    // Send a pulse to the trigger pin
    digitalWrite(ultrasonic.trigger_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(ultrasonic.trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(ultrasonic.trigger_pin, LOW);

    // Measure the duration of the pulse on the echo pin
    duration = pulseIn(ultrasonic.echo_pin, HIGH);

    // Calculate distance in centimeters
    distance_cm = duration * 0.034 / 2;
    return distance_cm;
}
char *StateToString(int state)
{
    switch (state)
    {
    case IDLE:
        return "IDLE";
    case ENTRY:
        return "ENTRY";
    case ENTRY_OPEN:
        return "ENTRY_OPEN";
    case ENTRY_CLOSE:
        return "ENTRY_CLOSE";
    case EXIT:
        return "EXIT";
    case EXIT_OPEN:
        return "EXIT_OPEN";
    case EXIT_CLOSE:
        return "EXIT_CLOSE";
    case FULL:
        return "FULL";
    default:
        return "UNKNOWN";
    }
}
void startCarPark()
{
    States initialState = IDLE;
    States currentState = initialState;
    long timeout = 0;
    int spotsAvailable = 3;
    while (true)
    {
        Serial.println(StateToString(currentState));
        switch (currentState)
        {
        case IDLE:
            if (spotsAvailable == 0)
                currentState = FULL;
            else if (measureDistance(entryEntranceUltrasonic) < ULTRASONIC_DISTANCE)
                currentState = ENTRY;
            else if (measureDistance(exitEntranceUltrasonic) < ULTRASONIC_DISTANCE)
                currentState = EXIT;
            break;
        case ENTRY:
            sweepServo(entryServo, OPEN_SERVO);
            currentState = ENTRY_OPEN;
            timeout = millis();
            break;
        case ENTRY_OPEN:
            LED_ON(GREEN_LED);
            LED_OFF(RED_LED);
            if (measureDistance(entryExitUltrasonic) < ULTRASONIC_DISTANCE)
            {
                currentState = ENTRY_CLOSE;
            }
            else
            {
                if (measureDistance(entryEntranceUltrasonic) < ULTRASONIC_DISTANCE)
                    timeout = millis();
                if (millis() - timeout > 5000)
                {

                    sweepServo(entryServo, CLOSE_SERVO);
                    LED_ON(RED_LED);
                    LED_OFF(GREEN_LED);
                    currentState = IDLE;
                }
            }
            break;
        case ENTRY_CLOSE:
            sweepServo(entryServo, CLOSE_SERVO);
            currentState = IDLE;
            spotsAvailable--;
            LED_ON(RED_LED);
            LED_OFF(GREEN_LED);
            break;
        case EXIT:
            sweepServo(exitServo, OPEN_SERVO);
            currentState = EXIT_OPEN;
            timeout = millis();
            break;
        case EXIT_OPEN:
            LED_ON(GREEN_LED);
            LED_OFF(RED_LED);
            if (measureDistance(exitExitUltrasonic) < ULTRASONIC_DISTANCE)
            {
                currentState = EXIT_CLOSE;
            }
            else
            {
                if (measureDistance(exitEntranceUltrasonic) < ULTRASONIC_DISTANCE)
                    timeout = millis();
                if (millis() - timeout > 5000)
                {
                    sweepServo(exitServo, CLOSE_SERVO);
                    LED_ON(RED_LED);
                    LED_OFF(GREEN_LED);
                    currentState = IDLE;
                }
            }
            break;
        case EXIT_CLOSE:
            sweepServo(exitServo, CLOSE_SERVO);
            currentState = IDLE;
            spotsAvailable++;
            LED_ON(RED_LED);
            LED_OFF(GREEN_LED);
            break;
        case FULL:
            if (spotsAvailable > 0)
                currentState = IDLE;
            if (measureDistance(exitEntranceUltrasonic) < ULTRASONIC_DISTANCE)
                currentState = EXIT;
            break;
        }
        initialState = currentState;
        writeToDisplay(spotsAvailable);
        delay(250);
    }
}

void loop()
{
    startCarPark();
}