#include <LiquidCrystal.h>

// LCD pins <--> Arduino pins
const int RS = 27, EN = 25, D4 = 37, D5 = 35, D6 = 33, D7 = 31;
int right=0,up=0;
int dir1=0,dir2=0;
byte customChar[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte customChar2[8] = {
  0b00000,
  0b00000,
  0b01110,
  0b01110,
  0b01110,
  0b00000,
  0b00000,
  0b00000
};
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int xlength = 16;
int ylength = 2;
int xpos = 0, ypos = 0;
int xprev = 0, yprev = 0;
bool hasSquare[16][2];

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2); // set up number of columns and rows

  resetBoard();

}

void resetBoard(){
  lcd.clear();
  // Initialize Squares

  for (int i = 0; i < 16; i++){
    for (int j = 0; j < 2; j++){
      hasSquare[i][j] = false;
    }
  }

  // Create Squares

  lcd.createChar(2, customChar2); // create Square
  createSquare(5, 1);
  createSquare(9, 1);
  createSquare(6, 0);
  createSquare(15, 1);
  
  // Create Heart

  lcd.createChar(1, customChar); // create Heart
  createHeart(0, 0);

}

void loop()
{
  
}

void createHeart(int x, int y){
  if (x >= xlength or y >= ylength){
    return;
  }
  lcd.setCursor(x, y);
  lcd.write((byte)1);
}

void createSquare(int x, int y){
  if (x >= xlength or y >= ylength){
    return;
  }
  if (hasSquare[x][y]){
    return;
  }
  lcd.setCursor(x, y);
  lcd.write((byte)2);
  hasSquare[x][y] = true;
}
    

