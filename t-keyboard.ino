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
*/

// TODO: sym + enter = meta key

#define keyboard_BL_PIN 9

#include "Arduino.h"
#include <SPI.h>

byte rows[] = { 0, 3, 19, 12, 18, 6, 7 };
const int rowCount = sizeof(rows) / sizeof(rows[0]);

byte cols[] = { 1, 4, 5, 11, 13 };
const int colCount = sizeof(cols) / sizeof(cols[0]);

bool keys[colCount][rowCount];
bool lastValue[colCount][rowCount];
bool changedValue[colCount][rowCount];

char keyboard[colCount][rowCount];
char keyboard_symbol[colCount][rowCount];


bool symbolSelected;
bool keyboard_BL_state = true;
bool case_locking = false;
bool alt_active = false;
unsigned long previousMillis_1 = 0;  //Millisecond time record
unsigned long previousMillis_2 = 0;  //Millisecond time record

void readMatrix();
void printMatrix();
bool keyPressed(int colIndex, int rowIndex);
bool keyActive(int colIndex, int rowIndex);
bool isPrintableKey(int colIndex, int rowIndex);
void set_keyboard_BL(bool state);

void setup() {
  Serial.begin(115200);
  Serial.printf("setup \n");
  keyboard[0][0] = 'q';
  keyboard[0][1] = 'w';
  keyboard[0][2] = NULL;  // symbol
  keyboard[0][3] = 'a';
  keyboard[0][4] = NULL;  // ALT
  keyboard[0][5] = ' ';
  keyboard[0][6] = NULL;  // Mic

  keyboard[1][0] = 'e';
  keyboard[1][1] = 's';
  keyboard[1][2] = 'd';
  keyboard[1][3] = 'p';
  keyboard[1][4] = 'x';
  keyboard[1][5] = 'z';
  keyboard[1][6] = NULL;  // Left Shift

  keyboard[2][0] = 'r';
  keyboard[2][1] = 'g';
  keyboard[2][2] = 't';
  keyboard[2][3] = NULL;  // Right Shit
  keyboard[2][4] = 'v';
  keyboard[2][5] = 'c';
  keyboard[2][6] = 'f';

  keyboard[3][0] = 'u';
  keyboard[3][1] = 'h';
  keyboard[3][2] = 'y';
  keyboard[3][3] = NULL;  // Enter
  keyboard[3][4] = 'b';
  keyboard[3][5] = 'n';
  keyboard[3][6] = 'j';

  keyboard[4][0] = 'o';
  keyboard[4][1] = 'l';
  keyboard[4][2] = 'i';
  keyboard[4][3] = NULL;  // Backspace
  keyboard[4][4] = '$';
  keyboard[4][5] = 'm';
  keyboard[4][6] = 'k';

  keyboard_symbol[0][0] = '#';
  keyboard_symbol[0][1] = '1';
  keyboard_symbol[0][2] = NULL;
  keyboard_symbol[0][3] = '*';
  keyboard_symbol[0][4] = NULL;
  keyboard_symbol[0][5] = NULL;
  keyboard_symbol[0][6] = '0';

  keyboard_symbol[1][0] = '2';
  keyboard_symbol[1][1] = '4';
  keyboard_symbol[1][2] = '5';
  keyboard_symbol[1][3] = '@';
  keyboard_symbol[1][4] = '8';
  keyboard_symbol[1][5] = '7';
  keyboard_symbol[1][6] = NULL;

  keyboard_symbol[2][0] = '3';
  keyboard_symbol[2][1] = '/';
  keyboard_symbol[2][2] = '(';
  keyboard_symbol[2][3] = NULL;
  keyboard_symbol[2][4] = '?';
  keyboard_symbol[2][5] = '9';
  keyboard_symbol[2][6] = '6';

  keyboard_symbol[3][0] = '_';
  keyboard_symbol[3][1] = ':';
  keyboard_symbol[3][2] = ')';
  keyboard_symbol[3][3] = NULL;
  keyboard_symbol[3][4] = '!';
  keyboard_symbol[3][5] = ',';
  keyboard_symbol[3][6] = ';';

  keyboard_symbol[4][0] = '+';
  keyboard_symbol[4][1] = '"';
  keyboard_symbol[4][2] = '-';
  keyboard_symbol[4][3] = NULL;
  keyboard_symbol[4][4] = NULL;
  keyboard_symbol[4][5] = '.';
  keyboard_symbol[4][6] = '\'';

  delay(500);
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

  delay(500);
}


void loop() {
  if (alt_active && keyPressed(3, 4)) {  //alt+b, change keyboard backlight status
    alt_active = false;
    keyboard_BL_state = !keyboard_BL_state;
    set_keyboard_BL(keyboard_BL_state);
  }

  if (keyPressed(2, 3)) {  //Right Shit, toggle case locking
    case_locking = !case_locking;
  }

  if (true) {
    readMatrix();
    printMatrix();

    // key 3,3 is the enter key
    if (keyPressed(3, 3)) {
      Serial.println();
    }
    //BACKSPACE
    if (keyPressed(4, 3)) {
      // TODO: Implement backspace logic
    }

    //SHIFT
    if (keyPressed(1, 6)) {
      //TODO: Implement shift logic
    }
    //alt+left shit, trigger ctrl+shift(Switch the input method)
    if (keyActive(0, 4) && keyPressed(1, 6)) {
      // TODO: Implement ctrl+shift logic
    }

  }

  else {
    if (millis() - previousMillis_2 > display_Wait_blue_time) {
      display_connected = true;
      previousMillis_2 = millis();
    }
  }
}

// Keyboard backlight status
void set_keyboard_BL(bool state) {
  digitalWrite(keyboard_BL_PIN, state);
}

void readMatrix() {
  int delayTime = 0;
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
      if ((lastValue[colIndex][rowIndex] != buttonPressed)) {
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

bool keyActive(int colIndex, int rowIndex) {
  return keys[colIndex][rowIndex] == true;
}

bool isPrintableKey(int colIndex, int rowIndex) {
  return keyboard_symbol[colIndex][rowIndex] != NULL || keyboard[colIndex][rowIndex] != NULL;
}


void printMatrix() {

  for (int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
    for (int colIndex = 0; colIndex < colCount; colIndex++) {
      // we only want to print if the key is pressed and it is a printable character
      if (keyPressed(colIndex, rowIndex) && isPrintableKey(colIndex, rowIndex)) {

        String toPrint;
        if (symbolSelected) {
          symbolSelected = false;
          toPrint = String(keyboard_symbol[colIndex][rowIndex]);
        } else {
          toPrint = String(keyboard[colIndex][rowIndex]);
        }

        if (keyActive(0, 4)) {
          alt_active = true;
          keys[0][4] = false;
          return;
        }
        // keys 1,6 and 2,3 are shift keys, so we want to upper case
        if (case_locking || keyActive(1, 6)) {
          toPrint.toUpperCase();
        }

        char c[2];
        strcpy(c, toPrint.c_str());
        Serial.println(toPrint);
        previousMillis_1 = millis();
      }
    }
  }
}