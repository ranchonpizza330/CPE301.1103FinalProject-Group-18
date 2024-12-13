#include "DHT.h"
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#define DHT11_PIN 53

//
DHT dht11(DHT11_PIN, DHT11);
const int RS = 33, EN = 35, D4 = 37, D5 = 39, D6 = 41, D7 = 43;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
Stepper stepper(32, 46, 50, 48, 52);
RTC_DS1307 rtc;
//

enum State {
    DISABLED,
    IDLE,
    RUNNING,
    ERROR
};

enum LED {
    RED = PA1, // pin 23
    YELLOW = PA3, // pin 25
    GREEN = PA5, // pin 27
    BLUE = PA7 // pin 29
};

State currentState = DISABLED;
float currentTemperature = 0;
float currentHumidity = 0;
float currentWaterLevel = 0;
float thresholdWaterLevel = 0;

volatile bool startPressed = false;
volatile bool stopPressed = false;
volatile bool resetPressed = false;

bool isTriggered = false;

void setup(){
    initUART(9600);
    // initButtons();
    // initLEDS();
    // initDHT11();
    // initLCD();
    // initWaterLevel();
    // initFanMotor();
    // initStepperMotor();
    initClock();
}

void loop(){
    // mainFunctionality();

    // // testing section
    // testUART();
    // testButtons();
    // testLEDS();
    // testDHT11();
    // testLCD();
    // testWaterLevel();
    // testFan();
    // testStepper();
    testClock();

}

// UART (Serial Monitor)
void initUART(unsigned long baud){
    unsigned long FCPU = 16000000;
    unsigned int tbaud = (FCPU / 16 / baud - 1);
    UCSR0A = 0x20;
    UCSR0B = 0x18;
    UCSR0C = 0x06;
    UBRR0 = tbaud;
}
void U0putchar(unsigned char U0pdata)
{
    while (!(UCSR0A & (1 << UDRE0))){};
    UDR0 = U0pdata;
}
void U0putstring(char* str)
{
    int i = 0;
    while (str[i] != '\0'){
        U0putchar(str[i]);
        i++;
    }
}
void floatToString(float num, char* str, int precision){
    int intPart = (int)num;
    float fractionPart = num - intPart;

    if (num < 0){
        *str++ = '-';
        intPart = -intPart;
        fractionPart = -fractionPart;
    }

    int i = 0;
    do{
        str[i++] = (intPart % 10) + '0';
        intPart /= 10;
    } while (intPart > 0);

    for (int j = 0; j < i / 2; j++){
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    str[i] = '\0';
    if (precision == 0){
        return;
    }
    str[i++] = '.';

    for (int j = 0; j < precision; j++){
        fractionPart *= 10;
        int digit = (int)fractionPart;
        str[i++] = digit + '0';
        fractionPart -= digit;
    }

    str[i] = '\0';
}

// BUTTONS 
void initButtons(){
  // Set up Digital Pin 19 (PD2) as input with pull-up resistor
  DDRD &= ~(1 << PD2) & ~(1 << PD1) & ~(1 << PD0); // Set PD2 as input
  PORTD |= (1 << PD2) | (1 << PD1) | (1 << PD0); // Enable pull-up resistor on PD2

  // Enable external interrupt
  EIMSK |= (1 << INT2) | (1 << INT1) | (1 << INT0);  // Enable INT
  EICRA |= (1 << ISC21) | (1 << ISC11) | (1 << ISC01); // Falling edge trigger
}
ISR(INT2_vect) {
  startPressed = true;
}
ISR(INT1_vect) {
  stopPressed = true;
}
ISR(INT0_vect) {
  resetPressed = true;
}
/*
START PIN 21
STOP PIN 20
RESET PIN 19
*/

// LEDS
void initLEDS(){
    DDRA |= (1 << RED) | (1 << YELLOW) | (1 << GREEN) | (1 << BLUE);
    PORTA &= ~(1 << RED) & ~(1 << YELLOW) & ~(1 << GREEN) & ~(1 << BLUE);
}
void writeLED(bool state, LED color){
    if (state){
        PORTA |= (1 << color);
        return;
    }
    PORTA &= ~(1 << color);
    return;
}
/*
RED PIN 23
YELLOW PIN 25
GREEN PIN 27
BLUE PIN 29
*/

// DHT11
void initDHT11(){
    dht11.begin();
}
float readTemperature(){
    return dht11.readTemperature();
}
float readHumidity(){
    return dht11.readHumidity();
}
void checkForErrors(){
    if (isnan(currentTemperature) || isnan(currentHumidity) || isnan(currentWaterLevel)){
        currentState = DISABLED;
        // print to Serial Monitor "Error finding values, cooler DISABLED"
        U0putstring("Error finding Humidity and Temperature, cooler DISABLED\n");
        return;
    }
}
/*
DHT11 PIN 53
*/

// LCD
void initLCD(){
    lcd.begin(16, 2);
}
/*
VSS POTENTIOMETER GND
VDD POSITIVE
V0 POTENTIOMETER OUTPUT
RS PIN 33
RW GND
E PIN 35
D4 PIN 37
D5 PIN 39
D6 PIN 41
D7 PIN 43
A  POSITIVE WITH 330 OHM RESISTOR
K GND
*/

// WATER LEVEL
void initWaterLevel(){
    ADCSRA |= 0b10000000;
    ADCSRA &= 0b11011111;
    ADCSRA &= 0b11110111;
    ADCSRA &= 0b11111000;
    ADCSRB &= 0b11110111;
    ADCSRB &= 0b11111000;
    ADMUX &= 0b01111111;
    ADMUX |= 0b01000000;
    ADMUX &= 0b11011111;
    ADMUX &= 0b11100000;
}
float readWaterLevel()
{
    ADMUX  &= 0b11100000;
    ADCSRB &= 0b11110111;
    ADCSRB |= 0b00001000;
    ADMUX  += 7;
    ADCSRA |= 0x40;
    while((ADCSRA & 0x40) != 0);
    unsigned int result = ADCL;
    result |= (ADCH << 8);
    return result;
}
/*
+ 5V
- GND
S PIN A15
*/

// FAN
void initFanMotor(){
    DDRA |= (1 << PA6);
    PORTA &= ~(1 << PA6);
}
void startFan(){
    PORTA |= (1 << PA6);
}
void stopFan(){
    PORTA &= ~(1 << PA6);
}
/*
VSS ARDUINO 5V
IN4 PIN 28
OUT4 MOTOR POSITIVE
GND PSU GND
OUT3 MOTOR GND
ARDUINO GND TO PSU GND
*/

// VENT
void initStepperMotor(){
    // Buttons for controlling vent direction
    DDRL &= ~(1 << PL7) & ~(1 << PL5);
    PORTL |= (1 << PL7) | (1 << PL5);

    stepper.setSpeed(1000);
}
/*
1N1 PIN 46
1N2 PIN 48
1N3 PIN 50
1N4 PIN 52
BUTTONS PIN 42, PIN 44
*/

// CLOCK
void initClock(){
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
void displayTime(){
    char buffer[4];
    DateTime now = rtc.now();
    floatToString(now.year(), buffer, 0);
    U0putstring(buffer);
    U0putchar('/');
    floatToString(now.month(), buffer, 0);
    U0putstring(buffer);
    U0putchar('/');
    floatToString(now.day(), buffer, 0);
    U0putstring(buffer);
    U0putchar(' ');
    floatToString(now.hour(), buffer, 0);
    U0putstring(buffer);
    U0putchar(':');
    floatToString(now.minute(), buffer, 0);
    U0putstring(buffer);
    U0putchar(':');
    floatToString(now.second(), buffer, 0);
    U0putstring(buffer);
    U0putchar(' ');
}


// PROGRAM HELPERS
void initParameters(){
    currentTemperature = readTemperature();
    currentHumidity = readHumidity();
    currentWaterLevel = readWaterLevel();
    thresholdWaterLevel = 0; // adjust later
}
void obtainParameters(){
    currentTemperature = readTemperature();
    currentHumidity = readHumidity();
    currentWaterLevel = readWaterLevel();
}
void displayError(){}
void mainFunctionality(){
    obtainParameters();
    checkForErrors();
    switch(currentState){
        case DISABLED:
            processDISABLED();
            break;
        case IDLE:
            processIDLE();
            break;
        case RUNNING:
            processRUNNING();
            break;
        case ERROR:
            processERROR();
            break;
    }
}
void processDISABLED(){
    // pass
}
void processIDLE(){
    // pass
}
void processRUNNING(){
    // pass
}
void processERROR(){
    // pass
}


// TESTING
void testUART(){
    while(true){
        U0putstring("Hello World!\n");
        delay(1000);
    }
}
void testButtons(){

    while(true){
        for (int i = 0; i < 10; i++){
            U0putchar('\n');
        }
        U0putstring("STOP button: ");
        if (startPressed){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        U0putchar('\n');
        U0putstring("START button: ");
        if (stopPressed){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        U0putchar('\n');
        U0putstring("RESET button: ");
        if (resetPressed){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        startPressed = false;
        stopPressed = false;
        resetPressed = false;
        U0putchar('\n');
        delay(1000);
    }
    
}
void testLEDS(){
    while(true){
        writeLED(true, RED);
        writeLED(true, YELLOW);
        writeLED(true, GREEN);
        writeLED(true, BLUE);
        delay(100);
        writeLED(false, RED);
        writeLED(false, YELLOW);
        writeLED(false, GREEN);
        writeLED(false, BLUE);
        delay(100);
    }
}
void testDHT11(){
    while(true){
        char buffer[3];
        floatToString(readTemperature(), buffer, 1);
        U0putstring("Temperature: ");
        U0putstring(buffer);
        U0putchar('\n');
        floatToString(readHumidity(), buffer, 1);
        U0putstring("Humidity: ");
        U0putstring(buffer);
        U0putchar('\n');
        U0putchar('\n');
        delay(1000);
    }
}
void testLCD(){
    float floatvalue = 12.345;
    while(true){
        lcd.setCursor(0,0);
        lcd.print("Hello World!");
        lcd.setCursor(0,1);
        lcd.print("Testing!");
        lcd.print(floatvalue);
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("World Hello!");
        lcd.setCursor(0,1);
        lcd.print("Testing!");
        delay(1000);
        lcd.clear();
    }
}
void testWaterLevel(){
    char buffer[4];
    while(true){
        U0putstring("Current Water Level: ");
        floatToString(readWaterLevel(), buffer, 0);
        U0putstring(buffer);
        U0putchar('\n');
        delay(100);
    }
}
void testFan(){
    while(true){
        startFan();
        delay(4000);
        stopFan();
        delay(4000);
    }
}
void testStepper(){
    while(true){
        if (!(PINL & (1 << PL5))){
            stepper.step(256);
        }
        if (!(PINL & (1 << PL7))){
            stepper.step(-256);
        }
    }
}
void testClock(){
    while(true){
        displayTime();
        U0putchar('\n');
        delay(1000);
    }
}

// 1 IMPLEMENT ISR FOR BUTTONS
// 2 IMPLEMENT MAIN FUNCTIONALITY