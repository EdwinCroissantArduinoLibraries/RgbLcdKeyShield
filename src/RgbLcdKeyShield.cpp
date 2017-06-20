/*
 * This is a library for the Adafruit RGB LCD Shield Kit and the
 * RobotDyn LCD RGB 16x2 + keypad + Buzzer Shield for Arduino
 *
 * Uses the Wire and SimpleKeyHandler library
 *
 * Copyright (C) 2017 Edwin Croissant
 *
 *  This program is free software: you can redistribute it and/or modify
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

#include <RgbLcdKeyShield.h>

//--------------------------------SimpleKeyHandler----------------------------

SimpleKeyHandler::SimpleKeyHandler() {
	clear();
	_companion = nullptr;
	_nextValidRead = 0;
	_previousState = keyOff;
	_count = 0;
}
/*
 * Clears all the callback pointer;
 */
void SimpleKeyHandler::clear() {
	onShortPress = nullptr;
	onLongPress = nullptr;
	onBothPress = nullptr;
	onRepPress = nullptr;
	onRepPressCount = nullptr;
}

/*
 * To be placed in the main loop. Expect TRUE if a key is pressed.
 */
void SimpleKeyHandler::read(bool keyState) {
	switch (_previousState) {
	case keyOff:
		if (keyState) {
			// when on advance to the next state
			_previousState = keyToOn;
			_nextValidRead = millis() + debounce;
		}
		break;
	case keyToOn:
		// ignore the key until debounce time expired
		if (millis() >= _nextValidRead) {
			if (keyState) {
				// when still on advance to the next state
				_previousState = keyOn;
				_nextValidRead = millis() + longPress;
			} else
				// otherwise it was a glitch
				_previousState = keyOff;
		}
		break;
	case keyOn:
		if (!keyState) {
			// when off advance to the next state
			_previousState = keyToOff;
			_nextValidRead = millis() + debounce;
		} else {
			// callback after long press and repeat after the repeat interval
			if (millis() >= _nextValidRead) {
				_nextValidRead = millis() + repeatInterval;
				if (onLongPress && _count == 0)
					onLongPress();
				if (onRepPressCount)
					onRepPressCount(_count);
				if (onRepPress)
					onRepPress();
				_count++;
			} else if (_count == 0 && _companion
					&& _companion->_previousState == keyOn
					&& _companion->_count == 0) {
				if (onBothPress)
					onBothPress();
				else if (_companion->onBothPress)
					_companion->onBothPress();
				// prevent each key to callback shortpress
				_count++;
				_companion->_count++;
			}
		}
		break;
	case keyToOff:
		// ignore the key until debounce time expired
		if (millis() >= _nextValidRead) {
			if (!keyState) {
				// when off advance to the next state
				_previousState = keyOff;
				// if key was released within the long press time callback
				if (onShortPress && _count == 0)
					onShortPress();
				else
					_count = 0;
			} else
				// otherwise it was a glitch
				_previousState = keyOn;
		}
		break;
	}
}

/*
 * Checks if the key is in the on stage
 */
bool SimpleKeyHandler::isPressed() {
	return _previousState == keyOn;
}

/*
 * Couples a companion key. When both keys are pressed the
 * onBothPressed is executed. Only one key has to be coupled
 */
void SimpleKeyHandler::setCompanion(SimpleKeyHandler* companion) {
	if (companion) {
		if (_companion) {
			_companion->_companion = nullptr;
		}
		_companion = companion;
		_companion->_companion = this;
	} else {
		if (_companion) {
			_companion->_companion = nullptr;
		}
		_companion = companion;
	}
}

//--------------------------------RgbLcdKeyShield----------------------------

RgbLcdKeyShield::RgbLcdKeyShield() {
	_shadowGPIOA = B11000000; // set bit 6 (red led) and 7 (green led) high
	_shadowGPIOB = B00100001; // set bit 0 (blue led) and 5 (lcd enable) high
	_shadowDisplayControl = displayControl | displayOnFlag; // set on, no cursor and no blinking
	_shadowEntryModeSet = entryModeSet;
}

/*
 * initialize the MCP23017 and the LCD
 */
void RgbLcdKeyShield::begin(void) {
	// give the lcd some time to get ready
	delay(100);
	/*
	 * Set the MCP23017 in 8 bit mode , sequential addressing
	 * disabled and slew rate disabled by writing to
	 * register 0x0b.
	 * As this register is not present in
	 * 16 bit mode we can safely write to it in case the
	 * MCP23017 is already in 8 bit mode which is possible
	 * as the hardware reset of the device is not used.
	 */
	_wireTransmit(IOCON, B10101000);
	// set bit 6 (red led) and 7 (green led) high
	_wireTransmit(GPIOA, _shadowGPIOA);
	// make bit 7 and 6 outputs
	_wireTransmit(IODIRA, B00111111);
	// enable pull-ups on input pins
	_wireTransmit(GPPUA, B00111111);
	// set bit 0 (blue led) and 5 (lcd enable) high
	_wireTransmit(GPIOB, _shadowGPIOB);
	// set all to output
	_wireTransmit(IODIRB, B00000000);
	// invert the 5 bits connected to the keys so that key pressed is high now
	_wireTransmit(IPOLA, B00011111);

	/* Initialize the lcd display
	 * For an explanation what is going on see the Wikipedia
	 * Hitachi HD44780 LCD controller entry
	 */
	Wire.beginTransmission(I2Caddr);
	Wire.write(GPIOB);
	_lcdWrite4(B0011, true);
	Wire.endTransmission();

	delay(5);

	Wire.beginTransmission(I2Caddr);
	Wire.write(GPIOB);
	_lcdWrite4(B0011, true);
	_lcdWrite4(B0011, true);
	// should be in 8 bit mode now so set to 4 bit mode
	_lcdWrite4(B0010, true);
	// set 2 lines and 5x8 dots
	_lcdWrite8(B00101000, true);
	_lcdWrite8(_shadowDisplayControl, true);
	Wire.endTransmission();

	clear();
}

/*
 * Clear the display and set the cursor in the upper left corner,
 * takes about two milliseconds.
 */
void RgbLcdKeyShield::clear() {
	_lcdTransmit(clearDisplay, true);
	delay(2);
}

/*
 * Set the cursor in the upper left corner,
 * takes about two milliseconds.
 */
void RgbLcdKeyShield::home() {
	_lcdTransmit(returnHome, true);
	delay(2);
}

/*
 * Sets the position of the cursor at which subsequent characters
 * will appear.
 */
void RgbLcdKeyShield::setCursor(uint8_t col, uint8_t row) {
	_lcdTransmit(setDdRamAdr | (col + row * 0x40), true);
}

/*
 * Sets the color of the backlight of the display.
 */
void RgbLcdKeyShield::setColor(colors color) {
	bitWrite(_shadowGPIOA, 6, !(color & clRed));
	bitWrite(_shadowGPIOA, 7, !(color & clGreen));
	bitWrite(_shadowGPIOB, 0, !(color & clBlue));
	_wireTransmit(GPIOA, _shadowGPIOA);
	_wireTransmit(GPIOB, _shadowGPIOB);
}

/*
 * turn the display pixels on
 */
void RgbLcdKeyShield::display() {
	_shadowDisplayControl |= displayOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * turn the display pixels off
 */
void RgbLcdKeyShield::noDisplay() {
	_shadowDisplayControl &= ~displayOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * Enables the blinking of the selected character
 */
void RgbLcdKeyShield::blink() {
	_shadowDisplayControl |= blinkOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * Disables the blinking of the selected character
 */
void RgbLcdKeyShield::noBlink() {
	_shadowDisplayControl &= ~blinkOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * Enables the cursor
 */
void RgbLcdKeyShield::cursor() {
	_shadowDisplayControl |= cursorOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * Disables the cursor
 */
void RgbLcdKeyShield::noCursor() {
	_shadowDisplayControl &= ~cursorOnFlag;
	_lcdTransmit(_shadowDisplayControl, true);
}

/*
 * Scrolls the display to the right
 */
void RgbLcdKeyShield::scrollDisplayRight() {
	_lcdTransmit(curOrDispShift | displayShiftFlag | shiftRightFlag, true);
}

/*
 * Scrolls the display to the left
 */
void RgbLcdKeyShield::scrollDisplayLeft() {
	_lcdTransmit(curOrDispShift | displayShiftFlag, true);
}

/*
 * All subsequent characters written to the display will go
 * from left to right.
 */
void RgbLcdKeyShield::leftToRight() {
	_shadowEntryModeSet &= ~right2LeftFlag;
	_lcdTransmit(_shadowEntryModeSet, true);
}

/*
 * All subsequent characters written to the display will go
 * from right to left.
 */
void RgbLcdKeyShield::rightToLeft() {
	_shadowEntryModeSet |= right2LeftFlag;
	_lcdTransmit(_shadowEntryModeSet, true);
}

/*
 * Moves the cursor to the right
 */
void RgbLcdKeyShield::moveCursorRight() {
	_lcdTransmit(curOrDispShift | shiftRightFlag, true);
}

/*
 * Moves the cursor to the left
 */
void RgbLcdKeyShield::moveCursorLeft() {
	_lcdTransmit(curOrDispShift, true);
}

/*
 * Turns the automatic scrolling of the display on.
 * New characters will appear at the same location and
 * the content of the display will scroll right or left
 * depending of the write direction.
 */

void RgbLcdKeyShield::autoscroll() {
	_shadowEntryModeSet |= autoShiftFlag;
	_lcdTransmit(_shadowEntryModeSet, true);
}

/*
 * Turns off automatic scrolling of the display.
 */
void RgbLcdKeyShield::noAutoscroll() {
	_shadowEntryModeSet &= ~autoShiftFlag;
	_lcdTransmit(_shadowEntryModeSet, true);
}

/*
 * Loads a special character
 * The cursor position is lost after this call
 */
void RgbLcdKeyShield::createChar(uint8_t location,  const uint8_t *charmap) {
	location &= 0x7;   // we only have 8 memory locations 0-7
	_lcdTransmit(setCgRamAdr | location << 3, true);
	write(charmap, 8);
	_lcdTransmit(setDdRamAdr, true); // cursor position is lost
}

#ifdef __AVR__
/*
 * Loads a special character from program memory
 * The cursor position is lost after this call
 */
void RgbLcdKeyShield::createCharP(uint8_t location, const uint8_t *charmap) {
	location &= 0x7;   // we only have 8 memory locations 0-7
	_lcdTransmit(setCgRamAdr | location << 3, true);
	writeP(charmap, 8);
	_lcdTransmit(setDdRamAdr, true); // cursor position is lost
}

/*
 * Writes a string in program memory to the display making full
 * use of the wire transmit buffer.
 */
size_t RgbLcdKeyShield::printP(const char str[]) {
	/*
	 * The Wire transmit buffer size is 32 bytes, for each character
	 * Each character takes 4 bytes so a maximum of seven characters
	 * are sent in one transmission
	 */
	size_t n = 0;
	char c = pgm_read_byte(&str[n]);
	while (c) {
		Wire.beginTransmission(I2Caddr);
		Wire.write(GPIOB);
		do { _lcdWrite8(c, false);
			c = pgm_read_byte(&str[++n]);
			} while (c && ( n % 7));
		Wire.endTransmission();
	};
	return n;
}

/*
 * does the same as write(const uint8_t* buffer, size_t size)
 * but from program memory instead
 */
size_t RgbLcdKeyShield::writeP(const uint8_t* buffer, size_t size) {
	/*
	 * The Wire transmit buffer size is 32 bytes, for each character
	 * Each character takes 4 bytes so a maximum of seven characters
	 * are sent in one transmission
	 */
	size_t n = 0;
	while (n < size) {
		Wire.beginTransmission(I2Caddr);
		Wire.write(GPIOB);
		do {
			_lcdWrite8(pgm_read_byte(&buffer[n++]), false);
		} while (n < size && (n % 7));
		Wire.endTransmission();
	}
	return n;
}
#endif // __AVR__

/*
 * Writes a character to the screen
 */
size_t RgbLcdKeyShield::write(uint8_t c) {
	_lcdTransmit(c, false);
	return 1;
}

/*
 * Overrides the standard implementation to make full use of the
 * wire transmit buffer.
 */
size_t RgbLcdKeyShield::write(const uint8_t* buffer, size_t size) {
	/*
	 * The Wire transmit buffer size is 32 bytes, for each character
	 * Each character takes 4 bytes so a maximum of seven characters
	 * are sent in one transmission
	 */
	size_t n = 0;
	while (n < size) {
		Wire.beginTransmission(I2Caddr);
		Wire.write(GPIOB);
		do {
			_lcdWrite8(buffer[n++], false);
		} while (n < size && (n % 7));
		Wire.endTransmission();
	}
	return n;
}

/*
 * Read the keys. To be placed in the main loop.
 */
void RgbLcdKeyShield::readKeys() {
	uint8_t keyState;
	Wire.beginTransmission(I2Caddr);
	Wire.write(GPIOA);
	Wire.endTransmission();
	Wire.requestFrom(I2Caddr, 1);
	keyState = Wire.read();
	keyLeft.read(keyState & B0010000);
	keyUp.read(keyState & B0001000);
	keyDown.read(keyState & B00000100);
	keyRight.read(keyState & B00000010);
	keySelect.read(keyState & B00000001);
}

// Private declarations--------------------------------------------

/*
 * Helper function to write a value to a register of the MCP23017
 */
void RgbLcdKeyShield::_wireTransmit(uint8_t reg, uint8_t value) {
	Wire.beginTransmission(I2Caddr);
	Wire.write(reg);
	Wire.write(value);
	Wire.endTransmission();
}

/*
 * Helper function to write a nibble to the display
 */
void RgbLcdKeyShield::_lcdWrite4(uint8_t value, bool lcdInstruction) {
	/*
	 * WTF? DB4 is connected to GPB4, DB5 to GPB3, DB6 to GPB2 and DB7 to GPB1
	 * Fasted way translating this is a 15 byte lookup table. Additionally:
	 * GPB7 (RS (register select)) is set as most traffic is to the data register
	 * GPB6 (R/W) is set low as this is a write operation
	 * GPB5 (E) is set high
	 */
#ifdef __AVR__
	const static uint8_t nibbleToPin[] PROGMEM = {
#else
	const static uint8_t nibbleToPin[]  = {
#endif // __AVR__
		B10100000,	// 0000
		B10110000,	// 0001
		B10101000,	// 0010
		B10111000,	// 0011
		B10100100,	// 0100
		B10110100,	// 0101
		B10101100,	// 0110
		B10111100,	// 0111
		B10100010,	// 1000
		B10110010,	// 1001
		B10101010,	// 1010
		B10111010,	// 1011
		B10100110,	// 1100
		B10110110,	// 1101
		B10101110,	// 1110
		B10111110	// 1111
	};

	// clear the lcd bits of shadowB
	_shadowGPIOB &= B00000001;
	// Translate the least nibble only
#ifdef __AVR__
	_shadowGPIOB |= pgm_read_byte(&nibbleToPin[value & B00001111]);
#else
	_shadowGPIOB |= nibbleToPin[value & B00001111];
#endif // __AVR__
	// if the instruction register is addressed clear bit 7
	if (lcdInstruction)
	_shadowGPIOB &= B01111111;
	// sent the data
	Wire.write(_shadowGPIOB);
	// Toggle the enable bit
	_shadowGPIOB ^= B00100000;
	// and sent again
	Wire.write(_shadowGPIOB);
}

/*
 * Helper function to write a byte to the display
 */
void RgbLcdKeyShield::_lcdWrite8(uint8_t value, bool lcdInstruction) {
	uint8_t temp = value;
	_lcdWrite4(temp >> 4, lcdInstruction);
	_lcdWrite4(value, lcdInstruction);
}

/*
 * Helper function to transmit a byte to the display
 */
void RgbLcdKeyShield::_lcdTransmit(uint8_t value, bool lcdInstruction) {
	Wire.beginTransmission(I2Caddr);
	Wire.write(GPIOB);
	_lcdWrite8(value, lcdInstruction);
	Wire.endTransmission();
}
