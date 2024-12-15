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

enum BUTTON {
    START = INT3,
    STOP = INT4,
    RESET = INT5
};

State currentState = DISABLED;
float currentTemperature = 0;
float currentHumidity = 0;
float currentWaterLevel = 0;
float waterLevelThreshold = 0;

float tempThreshold = 20.6;

volatile bool startPressed = false;
volatile bool stopPressed = false;
volatile bool resetPressed = false;

bool isTriggered = false;
int lastMinute = -1;

// UART (Serial Monitor) (Kevin)
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

// BUTTONS (Kevin)
void initButtons(){
  DDRD &= ~(1 << PD3); // Configure buttons as INPUT
  DDRE &= ~(1 << PE4) & ~(1 << PE5); 
  PORTD |= (1 << PD3); // Enable pull-up resistors
  PORTE |= (1 << PE4) | (1 << PE5);

  // Enable external interrupt
  EIMSK |= (1 << INT3) | (1 << INT4) | (1 << INT5);  // Enable INT interrupt
  EICRA |= (1 << ISC31); // Falling edge trigger
  EICRB |= (1 << ISC41) | (1 << ISC51); // Falling edge trigger
}
void toggleButton(bool state, BUTTON type){
    // Toggle interrupt using EIMSK
    if (state){
        EIMSK |= (1 << type);
        return;
    }
    EIMSK &= ~(1 << type);
}

volatile unsigned long lastStartPressTime = 0;
volatile unsigned long lastStopPressTime = 0;
volatile unsigned long lastResetPressTime = 0;
const unsigned long debounceDelay = 50;

ISR(INT3_vect) { // START button
    unsigned long currentTime = millis();
    if (currentTime - lastStartPressTime >= debounceDelay) {
        startPressed = true;
        lastStartPressTime = currentTime;
    }
}

ISR(INT4_vect) { // STOP button
    unsigned long currentTime = millis();
    if (currentTime - lastStopPressTime >= debounceDelay) {
        stopPressed = true;
        lastStopPressTime = currentTime;
    }
}

ISR(INT5_vect) { // RESET button
    unsigned long currentTime = millis();
    if (currentTime - lastResetPressTime >= debounceDelay) {
        resetPressed = true;
        lastResetPressTime = currentTime;
    }
}

/*
START PIN 18
STOP PIN 2
RESET PIN 3
*/

// LEDS (Kevin)
void initLEDS(){
    DDRA |= (1 << RED) | (1 << YELLOW) | (1 << GREEN) | (1 << BLUE); // Configure pin as OUTPUT
    PORTA &= ~(1 << RED) & ~(1 << YELLOW) & ~(1 << GREEN) & ~(1 << BLUE); // Init set pin as LOW
}
void writeLED(bool state, LED color){
    // Write HIGH or LOW to pin using enum as ID
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

// DHT11 (Kevin)
void initDHT11(){
    dht11.begin();
}
float readTemperature(){
    return dht11.readTemperature();
}
float readHumidity(){
    return dht11.readHumidity();
}
bool paramHasErrors(){
    if (isnan(currentTemperature) || isnan(currentHumidity)){
        return true;
    }
    return false;
}
/*
DHT11 PIN 53
*/

// LCD (Kevin)
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

// WATER LEVEL (Alexis)
void initWaterLevel(){
    DDRL |= (1 << PL4); // Configure pin as OUTPUT
    PORTL &= (1 << PL4); // Init set pin to LOW

    ADCSRA |= 0b10000000; // Enable ADC
    ADCSRA &= 0b11011111; // Disable auto trigger
    ADCSRA &= 0b11110111; // Disable interrupt
    ADCSRA &= 0b11111000; // Set prescaler to slow reading
    
    ADCSRB &= 0b11110111; // Reset channel and gain bits
    ADCSRB &= 0b11111000; // Set to free running mode
    
    ADMUX &= 0b01111111; // Enable AVCC analog reference
    ADMUX |= 0b01000000; 
    ADMUX &= 0b11011111; // Right adjust
    ADMUX &= 0b11100000; // Reset channel and gain bits
}
float readWaterLevel()
{
    PORTL |= (1 << PL4);
    ADMUX  &= 0b11100000; // Clear channel selection
    ADCSRB &= 0b11110111;
    ADCSRB |= 0b00001000; // Set for Channel 8 or higher
    ADMUX  += 7; // Set Channel to A15
    ADCSRA |= 0x40; // Start conversion
    while((ADCSRA & 0x40) != 0); // Wait for conversion to finish
    unsigned int result = ADCL; // Obtain result
    result |= (ADCH << 8);
    PORTL &= ~(1 << PL4);
    return result;
}
/*
+ 5V
- GND
S PIN A15
*/

// FAN (Alexis)
void initFanMotor(){
    DDRA |= (1 << PA6); // Configure pin as OUTPUT
    PORTA &= ~(1 << PA6); // Init set pin to LOW
}
void startFan(){
    PORTA |= (1 << PA6); // Set pin to HIGH
}
void stopFan(){
    PORTA &= ~(1 << PA6); // Set pin to LOW
}
/*
VSS ARDUINO 5V
IN4 PIN 28
OUT4 MOTOR POSITIVE
GND PSU GND
OUT3 MOTOR GND
ARDUINO GND TO PSU GND
*/

// VENT (Alexis)
void initStepperMotor(){
    // Buttons for controlling vent direction
    DDRL &= ~(1 << PL7) & ~(1 << PL5); // Configure pin as OUTPUT
    PORTL |= (1 << PL7) | (1 << PL5); // Init set pins to LOW

    stepper.setSpeed(1000);
}
void ventUp(){ // Handle moving vent 'up', prints time and vent status upon activation
    stepper.step(256);
    displayTime();
    U0putstring("Vent UP\n");
}
void ventDown(){ // Handle moving vent 'down', prints time and vent status upon activation
    stepper.step(-256);
    displayTime();
    U0putstring("Vent DOWN\n");
}
/*
1N1 PIN 46
1N2 PIN 48
1N3 PIN 50
1N4 PIN 52
BUTTONS PIN 42, PIN 44
*/

// CLOCK (Kevin)
void initClock(){
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
DateTime now;
void displayTime(){
    now = rtc.now();
    char buffer[4];
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

// PROGRAM HELPERS (Kevin)
void initParameters(){
    currentTemperature = readTemperature();
    currentHumidity = readHumidity();
    currentWaterLevel = readWaterLevel();
    waterLevelThreshold = 150; // adjust later
}
void displayParams(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Humidity: ");
    lcd.print(currentHumidity);
    lcd.print(" %");
    lcd.setCursor(0,1);
    lcd.print("Temp: ");
    lcd.print(currentTemperature);
    lcd.print(" C");
}
void mainFunctionality(){ // Machine executes a particular function based on current state
    delay(50);
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

void preStateFunctionality(){
    if (!(PINL & (1 << PL5))){ // INPUT pins default to HIGH when buttons unpressed, therefore if vent button returns LOW, activate stepper/vent
        ventUp();
    }
    if (!(PINL & (1 << PL7))){
        ventDown();
    }
    if (currentState == DISABLED){ // All logic after this is functionality for when state machine is not DISABLED
        return;
    }
    now = rtc.now();
    if (now.minute() != lastMinute){ // Updates every minute, gets temperature, humidity, and water level, and displays on LCD
        lastMinute = now.minute();
        currentTemperature = readTemperature();
        currentHumidity = readHumidity();
        currentWaterLevel = readWaterLevel();
        displayParams();

        // FOR TESTING
        
        // Serial.print("Date: ");
        // Serial.print(now.year());
        // Serial.print('/');
        // Serial.print(now.month());
        // Serial.print('/');
        // Serial.print(now.day());
        // Serial.print(' ');
        // Serial.print(now.hour());
        // Serial.print(':');
        // Serial.print(now.minute());
        // Serial.print(':');
        // Serial.println(now.second());
        // Serial.print("now.minute(): ");
        // Serial.println(now.minute());
        // Serial.print("lastminute: ");
        // Serial.println(lastMinute);
        // delay(10);
        // Serial.println("Params updated/minute passed\n");
    }
    // currentTemperature = readTemperature();
    // currentHumidity = readHumidity();
    // currentWaterLevel = readWaterLevel();
    // displayParams();
    
}
// Helper functions for handling STATE
void processDISABLED(){
    if (startPressed){
        startPressed = false;
        enterIDLE();
    }
}
void enterDISABLED(){
    currentState = DISABLED;
    displayTime();
    U0putstring("DISABLED\n\n");
    writeLED(false, RED);
    writeLED(true, YELLOW);
    writeLED(false, GREEN);
    writeLED(false, BLUE);
    toggleButton(true, START);
    toggleButton(false, STOP);
    toggleButton(false, RESET);
}
void processIDLE(){
    if (currentWaterLevel <= waterLevelThreshold){
        enterERROR();
    }
    else if (currentTemperature > tempThreshold){
        enterRUNNING();
    }
}
void enterIDLE(){
    currentState = IDLE;
    displayTime();
    U0putstring("IDLE\n\n");
    toggleButton(false, START);
    toggleButton(true, STOP);
    toggleButton(false, RESET);
    writeLED(false, RED);
    writeLED(false, YELLOW);
    writeLED(true, GREEN);
    writeLED(false, BLUE);
    stopFan();
}
void processRUNNING(){
    if (currentWaterLevel <= waterLevelThreshold){
        stopFan();
        enterERROR();
    }
    else if (currentTemperature <= tempThreshold){
        stopFan();
        enterIDLE();
    }
    else if (stopPressed){
        stopPressed = false;
        stopFan();
        enterDISABLED();
    }
}
void enterRUNNING(){
    currentState = RUNNING;
    startFan();
    displayTime();
    U0putstring("RUNNING\n\n");
    toggleButton(false, START);
    toggleButton(true, STOP);
    toggleButton(false, RESET);
    writeLED(false, RED);
    writeLED(false, YELLOW);
    writeLED(false, GREEN);
    writeLED(true, BLUE);
}
void processERROR(){
    if (stopPressed){
        stopPressed = false;
        resetPressed = false;
        enterDISABLED();
    }
    if (resetPressed){
        stopPressed = false;
        resetPressed = false;
        enterIDLE();
    }
}
void enterERROR(){
    currentState = ERROR;
    displayTime();
    U0putstring("ERROR\n\n");
    toggleButton(false, START);
    toggleButton(true, STOP);
    toggleButton(true, RESET);
    writeLED(true, RED);
    writeLED(false, YELLOW);
    writeLED(false, GREEN);
    writeLED(false, BLUE);
    U0putstring("Water level is too low\n\n");
}


// TESTING (ensure each component works individually)
void testUART(){
    while(true){
        U0putstring("Hello World!\n");
        delay(1000);
    }
}
void testButtons(){
    // disableButton(START);
    // disableButton(STOP);
    // disableButton(RESET);
    // enableButton(START);
    // enableButton(STOP);
    // enableButton(RESET);
    while(true){
        for (int i = 0; i < 10; i++){
            U0putchar('\n');
        }
        U0putstring("START button: ");
        if (startPressed){
            U0putstring("PRESSED");
        }
        else{
            U0putstring("unpressed");
        }
        U0putchar('\n');
        U0putstring("STOP button: ");
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
        delay(100);
        writeLED(true, YELLOW);
        delay(100);
        writeLED(true, GREEN);
        delay(100);
        writeLED(true, BLUE);
        delay(100);
        writeLED(false, RED);
        delay(100);
        writeLED(false, YELLOW);
        delay(100);
        writeLED(false, GREEN);
        delay(100);
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
        delay(1000);
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

void setup(){
    // Serial.begin(9600);
    initUART(9600);
    initButtons();
    initLEDS();
    initDHT11();
    initLCD();
    initWaterLevel();
    initFanMotor();
    initStepperMotor();
    initClock();

    enterDISABLED();
    initParameters();
}

void loop(){
    preStateFunctionality();
    mainFunctionality();

    // TESTING

    // testUART();
    // testButtons();
    // testLEDS();
    // testDHT11();
    // testLCD();
    // testWaterLevel();
    // testFan();
    // testStepper();
    // testClock();
}

// SCHEMATIC (Alexis)