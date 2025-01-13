/*
Adapted from https://github.com/Xinyuan-LilyGO/T-keyboard
For use with LilyGo T-WATCH-Keyboard

Keyboard pins:
Row 1 - PIN 0
Row 2 - PIN 3
Row 3 - PIN 19
Row 4 - PIN 12
Row 5 - PIN 18
Row 6 - PIN 6
Row 7 - PIN 7
Column 1 - PIN 1
Column 2 - PIN 4
Column 3 - PIN 5
Column 4 - PIN 11
Column 5 - PIN 13
Backlight - PIN 9

UART:
RX - PIN 20
TX - PIN 21

I2C:
SDA - PIN 2
SCL - PIN 10
INT - PIN 8

Further documentation of the keyboard itself can be found here: https://github.com/arturo182/bbq10kbd
*/

// TODO: sym + enter = meta key

#include "Arduino.h"
#include <SPI.h>
#include "Wire.h"
#include <string.h>  // Required for strncpy()

/*
0x1f is the address used by arturo182 in his original keyboard firmware
It is kept the same here to maintain compatibility with software that expects this address, like his keyboard library
https://github.com/arturo182/arduino_bbq10kbd
*/
#define I2C_ADDR 0x1f
#define SDA_PIN 2
#define SCL_PIN 10
#define keyboard_BL_PIN 9
#define MAX_KEYSTROKE_LENGTH 20
#define REPEAT_DELAY 1000 // 1 second in millis
#define REPEAT_SPEED 33 // Millis of delay before repeating a held down keystroke - 1s delay before repeating every 33ms is the default in Windows

byte rows[] = { 0, 3, 19, 12, 18, 6, 7 };
const int rowCount = sizeof(rows) / sizeof(rows[0]);

byte cols[] = { 1, 4, 5, 11, 13 };
const int colCount = sizeof(cols) / sizeof(cols[0]);

bool keys[colCount][rowCount];
bool lastValue[colCount][rowCount];
bool changedValue[colCount][rowCount];
unsigned long startMillis;
unsigned long lastMillis;

// Use const char* to indicate these are string literals
const char* keyboard[colCount][rowCount];
const char* keyboard_symbol[colCount][rowCount];

bool allowKeystroke = true;
bool symbolSelected;
bool keyboard_BL_state = true;
bool case_locking = false;
bool alt_active = false;
char keystroke[MAX_KEYSTROKE_LENGTH] = "";  // Use char array directly

void readMatrix();
void printMatrix();
bool keyPressed(int colIndex, int rowIndex);
bool keyActive(int colIndex, int rowIndex);
bool isPrintableKey(int colIndex, int rowIndex);
void set_keyboard_BL(bool state);
void print();
uint32_t i = 0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Setup");

  Serial.print("I2C Address: 0x");
  Serial.println(I2C_ADDR, HEX);
  Serial.print("SDA pin: ");
  Serial.println(SDA_PIN);
  Serial.print("SCL pin: ");
  Serial.println(SCL_PIN);

  // Initalise our device as an I2C slave using the correct pins
  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin((uint8_t)I2C_ADDR);
  // Set I2C clock speed at standard mode
  Wire.setClock(100000);

  // Column 1
  keyboard[0][0] = "q";
  keyboard[0][1] = "w";
  keyboard[0][2] = "SYM";  // symbol
  keyboard[0][3] = "a";
  keyboard[0][4] = "ALT";  // ALT
  keyboard[0][5] = " ";
  keyboard[0][6] = "MIC";  // Mic

  // Column 2
  keyboard[1][0] = "e";
  keyboard[1][1] = "s";
  keyboard[1][2] = "d";
  keyboard[1][3] = "p";
  keyboard[1][4] = "x";
  keyboard[1][5] = "z";
  keyboard[1][6] = "LSHIFT";  // Left Shift

  // Column 3
  keyboard[2][0] = "r";
  keyboard[2][1] = "g";
  keyboard[2][2] = "t";
  keyboard[2][3] = "RSHIFT";  // Right Shift
  keyboard[2][4] = "v";
  keyboard[2][5] = "c";
  keyboard[2][6] = "f";

  // Column 4
  keyboard[3][0] = "u";
  keyboard[3][1] = "h";
  keyboard[3][2] = "y";
  keyboard[3][3] = "ENTER";  // Enter
  keyboard[3][4] = "b";
  keyboard[3][5] = "n";
  keyboard[3][6] = "j";

  // Column 5
  keyboard[4][0] = "o";
  keyboard[4][1] = "l";
  keyboard[4][2] = "i";
  keyboard[4][3] = "BACKSPACE";  // Backspace
  keyboard[4][4] = "$";
  keyboard[4][5] = "m";
  keyboard[4][6] = "k";

  // Column 1 (alt keys)
  keyboard_symbol[0][0] = "#";
  keyboard_symbol[0][1] = "1";
  keyboard_symbol[0][2] = "";
  keyboard_symbol[0][3] = "*";
  keyboard_symbol[0][4] = "";
  keyboard_symbol[0][5] = "";
  keyboard_symbol[0][6] = "0";

  // Column 2 (alt keys)
  keyboard_symbol[1][0] = "2";
  keyboard_symbol[1][1] = "4";
  keyboard_symbol[1][2] = "5";
  keyboard_symbol[1][3] = "@";
  keyboard_symbol[1][4] = "8";
  keyboard_symbol[1][5] = "7";
  keyboard_symbol[1][6] = "";

  // Column 3 (alt keys)
  keyboard_symbol[2][0] = "3";
  keyboard_symbol[2][1] = "/";
  keyboard_symbol[2][2] = "(";
  keyboard_symbol[2][3] = "";
  keyboard_symbol[2][4] = "?";
  keyboard_symbol[2][5] = "9";
  keyboard_symbol[2][6] = "6";

  // Column 4 (alt keys)
  keyboard_symbol[3][0] = "_";
  keyboard_symbol[3][1] = ":";
  keyboard_symbol[3][2] = ")";
  keyboard_symbol[3][3] = "";
  keyboard_symbol[3][4] = "!";
  keyboard_symbol[3][5] = ",";
  keyboard_symbol[3][6] = ";";

  // Column 5 (alt keys)
  keyboard_symbol[4][0] = "+";
  keyboard_symbol[4][1] = "\"";
  keyboard_symbol[4][2] = "-";
  keyboard_symbol[4][3] = "";
  keyboard_symbol[4][4] = "SPEAKER";  //Speaker key
  keyboard_symbol[4][5] = ".";
  keyboard_symbol[4][6] = "\'";

  delay(100);
  pinMode(keyboard_BL_PIN, OUTPUT);
  set_keyboard_BL(keyboard_BL_state);

  for (int x = 0; x < rowCount; x++) {
    Serial.print(rows[x]);
    Serial.println(" as input");
    pinMode(rows[x], INPUT);
  }

  for (int x = 0; x < colCount; x++) {
    Serial.print(cols[x]);
    Serial.println(" as input-pullup");
    pinMode(cols[x], INPUT_PULLUP);
  }

  symbolSelected = false;

  delay(100);
}


void loop() {
  if (alt_active && keyPressed(3, 4)) {  // alt+b, change keyboard backlight status
    alt_active = false;
    keyboard_BL_state = !keyboard_BL_state;
    set_keyboard_BL(keyboard_BL_state);
    startMillis = millis();
  }

  if (keyPressed(2, 3)) {  // Right shift, toggle case locking
    case_locking = !case_locking;
  }

  if (true) {
    readMatrix();
    printMatrix();
    print();
  }
}

// Keyboard backlight status
void set_keyboard_BL(bool state) {
  digitalWrite(keyboard_BL_PIN, state);
}

void readMatrix() {
  // iterate the columns
  for (int colIndex = 0; colIndex < colCount; colIndex++) {
    // col: set to output to low
    byte curCol = cols[colIndex];
    pinMode(curCol, OUTPUT);
    digitalWrite(curCol, LOW);

    // row: interate through the rows
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
      byte rowCol = rows[rowIndex];
      pinMode(rowCol, INPUT_PULLUP);
      delay(1);  // arduino is not fast enought to switch input/output modes so wait 1 ms

      bool buttonPressed = (digitalRead(rowCol) == LOW);

      keys[colIndex][rowIndex] = buttonPressed;
      if (lastValue[colIndex][rowIndex] != buttonPressed) {
        changedValue[colIndex][rowIndex] = true;
      } else {
        changedValue[colIndex][rowIndex] = false;
      }

      lastValue[colIndex][rowIndex] = buttonPressed;
      pinMode(rowCol, INPUT);
    }
    // disable the column
    pinMode(curCol, INPUT);
  }

  if (keyPressed(0, 2)) {
    symbolSelected = true;
    // symbolSelected = !symbolSelected;
  }
}

bool keyPressed(int colIndex, int rowIndex) {
  return changedValue[colIndex][rowIndex] && keys[colIndex][rowIndex] == true;
}

bool keyHeld(int colIndex, int rowIndex) {
  return changedValue[colIndex][rowIndex] == false && keys[colIndex][rowIndex] == true;
}


bool keyActive(int colIndex, int rowIndex) {
  return keys[colIndex][rowIndex] == true;
}

bool isPrintableKey(int colIndex, int rowIndex) {
  if (symbolSelected){
    return (keyboard_symbol[colIndex][rowIndex] != NULL && strlen(keyboard_symbol[colIndex][rowIndex]) == 1);
  }
  else{
    return (keyboard[colIndex][rowIndex] != NULL && strlen(keyboard[colIndex][rowIndex]) == 1);
  }
  //return (keyboard_symbol[colIndex][rowIndex] != NULL || keyboard[colIndex][rowIndex] != NULL);
}


void printMatrix() {
  for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    for (int colIndex = 0; colIndex < colCount; colIndex++) {
      // we only want to print if the key is pressed and it is a printable character
      if ((keyPressed(colIndex, rowIndex) || keyHeld(colIndex, rowIndex)) && isPrintableKey(colIndex, rowIndex)) {
        //Check to see if this keypress has changed since the last check
        if (keyPressed(colIndex, rowIndex)){
          startMillis = millis();
          lastMillis = millis();
          allowKeystroke = true;
        }
        //If not, allow the keystroke to repeat if enough time has elapsed since the first keystroke and make sure it isn't repeating too fast
        else{
          if((millis() > (startMillis + REPEAT_DELAY)) && millis() > lastMillis && isPrintableKey(colIndex, rowIndex)){
            lastMillis = millis() + REPEAT_SPEED;
            allowKeystroke = true;
          }
        }

        if(allowKeystroke){
          if (symbolSelected) {
            symbolSelected = false;
            strncpy(keystroke, keyboard_symbol[colIndex][rowIndex], MAX_KEYSTROKE_LENGTH - 1);
          } else {
            strncpy(keystroke, keyboard[colIndex][rowIndex], MAX_KEYSTROKE_LENGTH - 1);
          }
          keystroke[MAX_KEYSTROKE_LENGTH - 1] = '\0';  // Ensure null termination

          if (keyActive(0, 4)) {
            alt_active = true;
            keys[0][4] = false;
            return;
          }
          // keys 1,6 and 2,3 are shift keys, so we want to upper case
          if (case_locking || keyActive(1, 6)) {
            for (int i = 0; keystroke[i]; i++) {
              keystroke[i] = toupper(keystroke[i]);
            }
          }
        }
        allowKeystroke = false;
      }
    }
  }
}

void print() {
  if (strlen(keystroke) != 0) {
    Serial.println(keystroke);
    size_t len = strlen(keystroke);
    uint8_t buffer[len];
    for (size_t i = 0; i < len; i++) {
      buffer[i] = (uint8_t)keystroke[i];
    }
    Wire.write(buffer, len);
    // Clear keystroke buffer
    memset(keystroke, 0, MAX_KEYSTROKE_LENGTH);
  }
}