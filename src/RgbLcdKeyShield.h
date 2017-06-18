/*
 * This is a library for the Adafruit RGB LCD Shield Kit and the
 * RobotDyn LCD RGB 16x2 + keypad + Buzzer Shield for Arduino
 *
 * Uses the Wire and SimpleKeyHandler library
 *
 * Copyright (C) 2017 Edwin Croissant
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See the README.md file for additional information.
 */
/*
 * HISTORY:
 * version 0.0 2017/06/18 initial version
 */

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega32U4__)

#ifndef RgbLcdKeyShield_H
#define RgbLcdKeyShield_H

#include "Arduino.h"
#include "Wire.h"
#include "SimpleKeyHandler.h"

class RgbLcdKeyShield: public Print {
public:
	enum colors {
		clBlack = 0,
		clRed = 1,
		clGreen = 2,
		clYellow = 3,
		clBlue = 4,
		clViolet = 5,
		clTeal = 6,
		clWhite = 7
	};

	using Print::write; // pull in write(str) and write(buf, size) from Print

	RgbLcdKeyShield();

	void begin(void);
	void clear();
	void home();
	void setCursor(uint8_t col, uint8_t row);
	void setColor(colors color);
	void display();
	void noDisplay();
	void blink();
	void noBlink();
	void cursor();
	void noCursor();
	void scrollDisplayRight();
	void scrollDisplayLeft();
	void leftToRight();
	void rightToLeft();
	void moveCursorRight();
	void moveCursorLeft();
	void autoscroll();
	void noAutoscroll();
	void createChar(uint8_t location, const char *charmap);
	virtual size_t write(uint8_t c);
	virtual size_t write(const char* s);
	size_t write(const uint8_t *buffer, size_t size) override;
	void readKeys();
	SimpleKeyHandler keyLeft;
	SimpleKeyHandler keyRight;
	SimpleKeyHandler keyUp;
	SimpleKeyHandler keyDown;
	SimpleKeyHandler keySelect;
private:
	// 8 bit mode MCP23017 register addresses
	enum MCP23017 {
		I2Caddr = 0x20,
		IOCON = 0x0b,
		IODIRA = 0x00,
		IPOLA = 0x01,
		IODIRB = 0x10,
		GPIOA = 0x09,
		GPIOB = 0x19,
		GPPUA = 0x06
	};

	// HD44780 constants
	enum HD44780 {
		// commands
		clearDisplay = 0x01,
		returnHome = 0x02,
		entryModeSet = 0x04,
		displayControl = 0x08,
		curOrDispShift = 0x10,
		functionSet = 0x20,
		setCgRamAdr = 0x40,
		setDdRamAdr = 0x80,
		// flags for entry mode set
		autoShiftFlag =  0x01,
		right2LeftFlag = 0x02, // 1 = right to left, 0 = left to right
		// flags for display on/off control
		displayOnFlag = 0x04,
		cursorOnFlag = 0x02,
		blinkOnFlag = 0x01,
		// flags for cursor/display shift
		displayShiftFlag = 0x08, // 1 = display, 0 is cursor
		shiftRightFlag = 0x04, // 1 = right, 0 = left
		// flags for function set
		bitMode8Flag = 0x10, // 8 bit = 1, 4 bit = 0
		lineMode2Flag = 0x08, // 2 line = 1, 1 line = 0
		dots5x10Flag = 0x04 // 5x10 dots = 1, 5x8 dots = 0
	};

	// shadow registers  MCP23017 GPIOA and GPIOB
	uint8_t _shadowGPIOA;
	uint8_t _shadowGPIOB;

	// shadow registers HD44780 display control and Entry Mode Set
	uint8_t _shadowDisplayControl;
	uint8_t _shadowEntryModeSet;

	void _wireTransmit(uint8_t reg, uint8_t value);
	void _lcdWrite4(uint8_t value, bool lcdInstruction);
	void _lcdWrite8(uint8_t value, bool lcdInstruction);
	void _lcdTransmit(uint8_t value, bool lcdInstruction);
};

#endif //  RgbLcdKeyShield_H

#else
#error not a suitable ATmega microcontroller...
#endif