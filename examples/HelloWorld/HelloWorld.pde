#include "Arduino.h"
#include <Wire.h>
#include <RgbLcdKeyShield.h>

// make an instance of RgbLcdKeyShield
RgbLcdKeyShield lcd;

/*
 * Strings in program memory can be displayed with the printP function
 */

const char text[] PROGMEM = "Hello world, how are you today?";

/*
 * Special characters in program memory can be loaded with the
 * createCharP function Note that the characters are 8 bit high
 * the last byte in the array is displayed at the cursor line
 */

const uint8_t PROGMEM smiley[] = {
B00000,
B10001,
B00100,
B00100,
B10001,
B01110,
B00000,
B00000 };

/*
 * The buzzer is connected to pin 2 (the board says pin 3 :( )
 */

enum pins {
	buzzerPin = 2
};

uint32_t time;
uint32_t n;

void setup() {
	Wire.begin();
	// set the wire bus to 400 kHz
	Wire.setClock(400000L);
	lcd.begin();
	// attach the functions to the keys
	lcd.keyUp.onShortPress = setRed;
	lcd.keyUp.onLongPress = beepHigh;
	lcd.keyDown.onShortPress = setGreen;
	lcd.keyDown.onLongPress = beepLow;
	lcd.keyLeft.onShortPress = setBlue;
	lcd.keyLeft.onRepPress = moveLeft;
	lcd.keyRight.onShortPress = setYellow;
	lcd.keyRight.onRepPress = moveRight;
	lcd.keySelect.onShortPress = setWhite;
	lcd.keySelect.onLongPress = setBlack;

	lcd.createCharP(0,smiley);

	lcd.setCursor(4,0);
	lcd.print(F("RobotDyn"));
	lcd.setCursor(1,1);
	lcd.print(F("Backlight test"));

	for (int i = 0; i < 8; ++i) {
		lcd.setColor(RgbLcdKeyShield::colors(i));
		delay(500);
	}
	lcd.clear();
	lcd.setColor(RgbLcdKeyShield::clWhite);

	time = micros();
	n = lcd.printP(text);
	time = micros() - time;
	lcd.setCursor(0,1);
	lcd.print(n);
	lcd.print(" ch in ");
	lcd.print(time);
	lcd.print(' ');
	lcd.print(char(0xE4));
	lcd.print(F("s = "));
	lcd.print(n * 1000000 / time);
	lcd.print(F(" char/s "));
	lcd.print(char(0));

	for (int i = 0; i < 40; i++) {
		lcd.scrollDisplayLeft();
		delay(300);
	}
}

void loop() {
	  lcd.readKeys();
}

void setRed() {
	lcd.setColor(RgbLcdKeyShield::clRed);
}

void setGreen() {
	lcd.setColor(RgbLcdKeyShield::clGreen);
}

void setBlue() {
	lcd.setColor(RgbLcdKeyShield::clBlue);
}

void setYellow() {
	lcd.setColor(RgbLcdKeyShield::clYellow);
}

void setWhite() {
	lcd.setColor(RgbLcdKeyShield::clWhite);
}

void setBlack() {
	lcd.setColor(RgbLcdKeyShield::clBlack);
}

void moveLeft() {
	lcd.scrollDisplayLeft();
}

void moveRight() {
	lcd.scrollDisplayRight();
}

void beepLow () {
	tone (buzzerPin,622,1000);
}

void beepHigh () {
	tone (buzzerPin,784,1000);
}
