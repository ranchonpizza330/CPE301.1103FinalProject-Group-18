#include "DHT.h"
#include <LiquidCrystal.h>
#define DHT11_PIN 53

//
DHT dht11(DHT11_PIN, DHT11);
const int RS = 33, EN = 35, D4 = 37, D5 = 39, D6 = 41, D7 = 43;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
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

enum BUTTON {
    STOP = PA0, // pin 22
    START = PA2, // pin 24
    RESET = PA4 // pin 26
};

State currentState = DISABLED;
float currentTemperature = 0;
float currentHumidity = 0;
float currentWaterLevel = 0;
float thresholdWaterLevel = 0;


void setup(){
    initUART(9600);
    // initButtons();
    // initLEDS();
    // initDHT11();
    // initLCD();

    // initWaterLevel();
    
    // initFanMotor();
    // initStepperMotor();
}

void loop(){
    
    // mainFunctionality();

    // // testing section
    // testUART();
    // testButtons();
    // testLEDS();
    // testDHT11();
    // testLCD();
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
    DDRA &= ~(1 << STOP) & ~(1 << START) & ~(1 << RESET);
    PORTA |= (1 << STOP) | (1 << START) | (1 << RESET);
}
bool readButton(BUTTON type){
    if (!(PINA & (1 << type))) {
        return true;
    }
    return false;
}
/*
STOP PIN 22
START PIN 24
RESET PIN 26
*/

// LEDS
void initLEDS(){
    DDRA |= (1 << RED) | (1 << YELLOW) | (1 << GREEN) | (1 << BLUE);
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
void initWaterLevel(){}
float readWaterLevel(){}

// FAN
void initFanMotor(){}
void startFan(){}
void stopFan(){}

// VENT
void initStepperMotor(){}

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
        if (readButton(STOP)){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        U0putchar('\n');
        U0putstring("START button: ");
        if (readButton(START)){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        U0putchar('\n');
        U0putstring("RESET button: ");
        if (readButton(RESET)){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
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
void testWaterLevel(){}
void testFan(){}
void testStepper(){}

