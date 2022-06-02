#include "Arduino.h"

#include "MIC_GeneralDef.h"
#include "MIC_LCD.h"

// LCD layout support, set cursor needs to be modified if this is changed to more than 4 rows
#define MIC_LCD_MAXCOLUMN		16
#define MIC_LCD_MAXROW			4

// LCD signal definition
#define _RS_INSTRUCTION			LOW
#define _RS_DATA				HIGH
#define _RW_WRITE				LOW
#define _RW_READ				HIGH
#define _EN_DISABLE				LOW
#define _EN_ENABLE				HIGH

// Instruction Description
// Clear Display
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   0   0   0   0   0   1
// Clear all the display data by writing "20H" (space code) to add DDRAM addresses and set DDRAM address to "00H" into AC (Address Counter).
// Return cursor to the original status, namely, bring the cursor to the left edge on first line of the display.
// Make entry mode increment (I/D = 1 )
#define MIC_LCD_INST_CLEARDISPLAY	0x01

// Return Home
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   0   0   0   0   1   x
// Return Home is cursor return home instruction. Set DDRAM address to "00H" into the address counter.
// Return cursor to its original site and return display to its original status, if shifted.
// Contents of DDRAM does not change
#define MIC_LCD_INST_RETURNHOME		0x02

// Entry Mode Set
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   0   0   0   1   I/D S
// Sets the moving direction of cursor and display.
// I/D: Increment/decrement of DDRAM address (cursor or blink)
// I/D = 0: shift to the left
// I/D = 1: shift to the right
// S: Shift of entire display (How about AC value???)
// S = 0: shift cursor/blink
// S = 1: shift entire display on write

// Display ON/OFF
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   0   0   1   D   C   B
// Control display/cursor/blink ON/OFF
// D = 1: entire display on
// C = 1: cursor on
// B = 1: cursor position on

// Cursor or display shift
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   0   1   S/C R/L x   x
// Shift cursor or display
//  S/C     R/L     Description                 AC Value
//  0       0       Shift cursor to the left    AC = AC - 1
//  0       1       Shift cursor to the right   AC = AC + 1
//  1       0       Shift display to the left   AC = AC
//  1       1       Shift display to the right  AC = AC

// Function Set
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   0   1   DL  N   F   x   x
// DL = 0: 4 bit bus
// DL = 1: 8 bit bus
// N = 0: 1-line display mode
// N = 1: 2-line display mode
// F = 0: 5x8 dots format
// F = 1: 5x11 dots format, only available under 1-line display mode

// Set CGRAM Address
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   0   1   AC5 AC4 AC3 AC2 AC1 AC0
#define MIC_LCD_INST_SETCGRAMADDR			0x40
#define MIC_LCD_INST_SETCGRAMADDR_ADDRMASK	0x3F

// Set DDRAM Address
//      RS  R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// code 0   0   1   AC6 AC5 AC4 AC3 AC2 AC1 AC0
#define MIC_LCD_INST_SETDDRAMADDR			0x80
#define MIC_LCD_INST_SETDDRAMADDR_ADDRMASK	0x7F

// Private functions
// Function: BYTE _readBits (void)
// For 4 bits bus mode, DB7 - 4 are read to higher 4 bits;
// For 8 bits bus mode, all bits are read out.
BYTE MIC_LCD::_readBits (void)
{
	BYTE counter = 0;
	BYTE bitsRead = 0;

	for (counter = 0; counter <8;  counter++)
	{
		if (_LCD_Attributes._DB_PIN[counter] != 0xff)
		{
			pinMode(_LCD_Attributes._DB_PIN[counter], INPUT);
		}
	}

	digitalWrite(_LCD_Attributes._RW_PIN, _RW_READ);
	delayMicroseconds(1);				// Address set-up time, (RS, R/#W to E, tAS = 40ns min)

	bitsRead = 0x00;

	digitalWrite(_LCD_Attributes._EN_PIN, _EN_ENABLE);
	delayMicroseconds(1);				// Data setup time (TDDR = 320ns Max)

	for (counter = 0; counter <8; counter++)
	{
		if (_LCD_Attributes._DB_PIN[counter] != 0xff)
		{
			bitsRead += (((BYTE)digitalRead(_LCD_Attributes._DB_PIN[counter])) & 0x01) << counter;
		}
	}

	digitalWrite(_LCD_Attributes._EN_PIN, _EN_DISABLE);
	delayMicroseconds(1);				// Enable Cycle Time (TC = 1200ns Min, TDDR consumed 1000ns)

	for (counter = 0; counter < 8; counter++)
	{
		if (_LCD_Attributes._DB_PIN[counter] != 0xff)
		{
			pinMode(_LCD_Attributes._DB_PIN[counter], OUTPUT);
		}
	}

	return bitsRead;
}

// Function: BYTE _readBYTE (void)
BYTE MIC_LCD::_readBYTE(void)
{
	BYTE byteRead = 0;

	byteRead = _readBits();

	if (_LCD_Attributes._functionSet._8BitBus == CLEAR)
	{
		byteRead = (byteRead & 0xf0)+ ((_readBits() & 0xf0) >> 4);
	}

	return byteRead;
}

// Function: void _writeBits(BYTE bitsWritten)
// For 4 bits bus mode: higher 4 bits are written in the cycle
// For 8 bits bus mode: all 8 bits are written in the cycle
void MIC_LCD::_writeBits(BYTE bitsWritten)
{
	BYTE counter = 0;

	digitalWrite(_LCD_Attributes._RW_PIN, _RW_WRITE);
	delayMicroseconds(1); // Address set-up time, (RS, R/#W to E, tAS = 40ns min)

	digitalWrite(_LCD_Attributes._EN_PIN, _EN_ENABLE);

	for (counter = 0; counter < 8; counter++)
	{
		if (_LCD_Attributes._DB_PIN[counter] != 0xff)
		{
			digitalWrite(_LCD_Attributes._DB_PIN[counter], ((bitsWritten >> counter) & 0x01));
		}
	}

	delayMicroseconds(1);				// Data setup time (TDSW = 80ns Min)
	digitalWrite(_LCD_Attributes._EN_PIN, _EN_DISABLE);
	delayMicroseconds(1);				// Enable Cycle Time (TC = 1200ns Min, TDSW consumed 1000ns)

	return ;
}

// Function: void _writeBYTE (BYTE byte)
void MIC_LCD::_writeBYTE(BYTE byte)
{
	_writeBits(byte);

	if (_LCD_Attributes._functionSet._8BitBus == CLEAR)
	{
		_writeBits((byte & 0x0f) << 4);
	}

	return;
}

// Function: MIC_LCD_STATUS _readStatus (void)
MIC_LCD_STATUS MIC_LCD::_readStatus (void)
{
	BYTE byteRead;

	digitalWrite(_LCD_Attributes._RS_PIN, _RS_INSTRUCTION);

	byteRead = _readBYTE();

	return *(MIC_LCD_STATUS *)(&byteRead);
}

// Function: MIC_RC _LCD_Ready(void);
// Return MIC_RC_SUCCESS on ready. Error when time out (set to 1024ms)
MIC_RC MIC_LCD::_LCDReady(void)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	MIC_LCD_STATUS status = {0, SET};
	unsigned long currentMillis = 0;

	status = _readStatus();

	currentMillis = (millis() / 1000);		// time out set to 1000 ms
	while ((status.busy == SET) && (returnCode == MIC_RC_SUCCESS) && (currentMillis == (millis() / 1000)))
	{
		status = _readStatus();
	}

	if (status.busy == SET)
	{
		returnCode = MIC_RC_LCD_ERROR;
	}

	return returnCode;
}

// Function: MIC_RC _write_Instruction (BYTE instruction)
// Input: instruction byte pointer
MIC_RC MIC_LCD::_writeInstruction (BYTE instruction)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;

	returnCode = _LCDReady();

	if (returnCode == MIC_RC_SUCCESS)
	{
		digitalWrite(_LCD_Attributes._RS_PIN, _RS_INSTRUCTION);
		_writeBYTE(instruction);
	}

	return returnCode;
}

// Function: MIC_RC _read_Data (BYTE *data)
// Input: data byte pointer
MIC_RC MIC_LCD::_readData (BYTE *data)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	BYTE dataRead;

	returnCode = _LCDReady();

	if (returnCode == MIC_RC_SUCCESS)
	{
		digitalWrite(_LCD_Attributes._RS_PIN, _RS_DATA);
		*data = _readBYTE();
	}

	return returnCode;
}

// Function: MIC_RC _write_Data (BYTE data)
// Input: data byte pointer
MIC_RC MIC_LCD::_writeData (BYTE data)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;

	returnCode = _LCDReady();

	if (returnCode == MIC_RC_SUCCESS)
	{
		digitalWrite(_LCD_Attributes._RS_PIN, _RS_DATA);
		_writeBYTE(data);
	}

	return returnCode;
}

// Public functions
//Function: MIC_LCD (	BYTE RS, BYTE EN, BYTE RW,
//						BYTE DB7, BYTE DB6, BYTE DB5, BYTE DB4, BYTE DB3, BYTE DB2, BYTE DB1, BYTE DB0)
MIC_LCD::MIC_LCD (	BYTE RS, BYTE EN, BYTE RW,
BYTE DB7, BYTE DB6, BYTE DB5, BYTE DB4, BYTE DB3, BYTE DB2, BYTE DB1, BYTE DB0)
{
	//Function pins
	_LCD_Attributes._RS_PIN = RS;
	_LCD_Attributes._EN_PIN = EN;
	_LCD_Attributes._RW_PIN = RW;

	//DB pins
	_LCD_Attributes._DB_PIN[0] = DB0;
	_LCD_Attributes._DB_PIN[1] = DB1;
	_LCD_Attributes._DB_PIN[2] = DB2;
	_LCD_Attributes._DB_PIN[3] = DB3;
	_LCD_Attributes._DB_PIN[4] = DB4;
	_LCD_Attributes._DB_PIN[5] = DB5;
	_LCD_Attributes._DB_PIN[6] = DB6;
	_LCD_Attributes._DB_PIN[7] = DB7;

	//Function setup
	_LCD_Attributes._functionSet._2LineMode = SET;
	_LCD_Attributes._functionSet._5x11Format = CLEAR;
	// Default bus mode is set to 8 bits. This will be adjusted by pin assignment after PORST;
	_LCD_Attributes._functionSet._8BitBus = SET;
	_LCD_Attributes._functionSet._instruction = 0x01;
	_LCD_Attributes._functionSet._reserved = 0x00;

	_LCD_Attributes._cursorDisplayShift._shiftRight = SET;
	_LCD_Attributes._cursorDisplayShift._shiftDisplay = CLEAR;
	_LCD_Attributes._cursorDisplayShift._instruction = 0x01;
	_LCD_Attributes._cursorDisplayShift._reserved = 0x00;

	_LCD_Attributes._entryModeSet._ShiftDisplay = CLEAR;
	_LCD_Attributes._entryModeSet._shiftRight = SET;
	_LCD_Attributes._entryModeSet._instruction = 0x01;

	_LCD_Attributes._displayONOFF._blink = CLEAR;
	_LCD_Attributes._displayONOFF._cursor = CLEAR;
	_LCD_Attributes._displayONOFF._display = SET;
	_LCD_Attributes._displayONOFF._instruction = 0x01;

	return;
}

//Function: PROST (void)
MIC_RC MIC_LCD::PORST (BYTE row, BYTE column)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	BYTE counter = 0;

	// Set up PIN input/output mode
	pinMode(_LCD_Attributes._RS_PIN, OUTPUT);
	pinMode(_LCD_Attributes._EN_PIN, OUTPUT);
	pinMode(_LCD_Attributes._RW_PIN, OUTPUT);

	for (counter = 0; counter <8; counter++)
	{
		if (_LCD_Attributes._DB_PIN[counter] != 0xff)
		{
			pinMode(_LCD_Attributes._DB_PIN[counter], OUTPUT);
		}
	}

	// Set row and column
	if ((row > MIC_LCD_MAXROW) || (column > MIC_LCD_MAXCOLUMN))
	{
		returnCode = MIC_RC_LCD_ERROR;
	}
	else
	{
		_LCD_Attributes._row = row;
		_LCD_Attributes._column = column;

		if (row == 1)
		{
			// Set LCD to 1 line mode and try to use a bigger font
			_LCD_Attributes._functionSet._2LineMode = CLEAR;
			_LCD_Attributes._functionSet._5x11Format = SET;
		}

		// Wait 40ms, after VCC rises to 2.7V, use 50ms
		delay(50);

		digitalWrite(_LCD_Attributes._RS_PIN, _RS_INSTRUCTION);

		// Do not check busy flag, set 8-bit interface
		_writeBits(*((BYTE*)&_LCD_Attributes._functionSet));

		// Wait 4.1ms
		delay(5);

		// Do not check busy flag, set 8-bit interface
		_writeBits(*((BYTE*)&_LCD_Attributes._functionSet));

		// Wait 100us
		delayMicroseconds(150);

		// Do not check busy flag, set 8-bit interface
		_writeBits(*((BYTE*)&_LCD_Attributes._functionSet));

		// Do not check busy flag, set interface based on DB pin assignment
		// Any pin assignment of DB3 - DB0 will define bus mode as 8 pin.
		if ((_LCD_Attributes._DB_PIN[3] == 0xff) || (_LCD_Attributes._DB_PIN[2] == 0xff) ||
		(_LCD_Attributes._DB_PIN[1] == 0xff) || (_LCD_Attributes._DB_PIN[0] == 0xff))
		{
			_LCD_Attributes._functionSet._8BitBus = CLEAR;

			_writeBits(*((BYTE*)&_LCD_Attributes._functionSet));
		}

		// Set display row and font
		returnCode = _writeInstruction(*((BYTE*)&_LCD_Attributes._functionSet));

		// Display ON/OFF
		if (returnCode == MIC_RC_SUCCESS)
		{
			returnCode = _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
		}

		// Clear display
		if (returnCode == MIC_RC_SUCCESS)
		{
			returnCode = _writeInstruction(MIC_LCD_INST_CLEARDISPLAY);
		}

		// Entry Mode Set
		if (returnCode == MIC_RC_SUCCESS)
		{
			returnCode = _writeInstruction(*((BYTE*)&_LCD_Attributes._entryModeSet));
		}

		delay(2);
	}

	return returnCode;
}

// Function: MIC_RC clearDisplay (void)
MIC_RC MIC_LCD::clearDisplay (void)
{
	return _writeInstruction(MIC_LCD_INST_CLEARDISPLAY);
}

//Function: MIC_RC returnHome (void)
MIC_RC MIC_LCD::returnHome (void)
{
	return _writeInstruction(MIC_LCD_INST_RETURNHOME);
}

// Function: entryModeCursorLeft(void)
MIC_RC MIC_LCD::entryModeCursorLeft(void)
{
	_LCD_Attributes._entryModeSet._ShiftDisplay = CLEAR;
	_LCD_Attributes._entryModeSet._shiftRight = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._entryModeSet));
}

// Function: entryModeCursorRight(void)
MIC_RC MIC_LCD::entryModeCursorRight(void)
{
	_LCD_Attributes._entryModeSet._ShiftDisplay = CLEAR;
	_LCD_Attributes._entryModeSet._shiftRight = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._entryModeSet));
}

// Function: entryModeDisplayLeft(vod)
MIC_RC MIC_LCD::entryModeDisplayLeft(void)
{
	_LCD_Attributes._entryModeSet._ShiftDisplay = SET;
	_LCD_Attributes._entryModeSet._shiftRight = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._entryModeSet));
}

// Function: entryModeDisplayRight(vod)
MIC_RC MIC_LCD::entryModeDisplayRight(void)
{
	_LCD_Attributes._entryModeSet._ShiftDisplay = SET;
	_LCD_Attributes._entryModeSet._shiftRight = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._entryModeSet));

}

// Function: MIC_RC DisplayMode1Line(void)
MIC_RC MIC_LCD::DisplayMode1Line(void)
{
	_LCD_Attributes._functionSet._2LineMode = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._functionSet));
}

// Function: MIC_RC DisplayMode2Line(void)
MIC_RC MIC_LCD::DisplayMode2Line(void)
{
	_LCD_Attributes._functionSet._2LineMode = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._functionSet));
}

// Function: MIC_RC FontFormat5x8(void)
MIC_RC MIC_LCD::FontFormat5x8(void)
{
	_LCD_Attributes._functionSet._5x11Format = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._functionSet));
}

// Function: MIC_RC FontFormat5x11(void)
MIC_RC MIC_LCD::FontFormat5x11(void)
{
	_LCD_Attributes._functionSet._5x11Format = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._functionSet));
}

// Function: MIC_RC displayON (void)
MIC_RC MIC_LCD::displayON (void)
{
	_LCD_Attributes._displayONOFF._display = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

//Function: MIC_RC displayOFF (void)
MIC_RC MIC_LCD::displayOFF (void)
{
	_LCD_Attributes._displayONOFF._display = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

// Function: MIC_RC cursorON (void)
MIC_RC MIC_LCD::cursorON (void)
{
	_LCD_Attributes._displayONOFF._cursor = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

//Function: MIC_RC cursorOFF (void)
MIC_RC MIC_LCD::cursorOFF (void)
{
	_LCD_Attributes._displayONOFF._cursor = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

// Function: MIC_RC blinkON (void)
MIC_RC MIC_LCD::blinkON (void)
{
	_LCD_Attributes._displayONOFF._blink = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

//Function: MIC_RC blinkOFF (void)
MIC_RC MIC_LCD::blinkOFF (void)
{
	_LCD_Attributes._displayONOFF._blink = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._displayONOFF));
}

//Function: MIC_RC cursorShiftLEFT (void)
MIC_RC MIC_LCD::cursorShiftLEFT (void)
{
	_LCD_Attributes._cursorDisplayShift._shiftDisplay = CLEAR;
	_LCD_Attributes._cursorDisplayShift._shiftRight = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._cursorDisplayShift));
}

//Function: MIC_RC cursorShiftRIGHT (void)
MIC_RC MIC_LCD::cursorShiftRIGHT (void)
{
	_LCD_Attributes._cursorDisplayShift._shiftDisplay = CLEAR;
	_LCD_Attributes._cursorDisplayShift._shiftRight = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._cursorDisplayShift));
}

//Function: MIC_RC displayShiftLEFT (void)
MIC_RC MIC_LCD::displayShiftLEFT (void)
{
	_LCD_Attributes._cursorDisplayShift._shiftDisplay = SET;
	_LCD_Attributes._cursorDisplayShift._shiftRight = CLEAR;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._cursorDisplayShift));
}

//Function: MIC_RC displayShiftRIGHT (void)
MIC_RC MIC_LCD::displayShiftRIGHT (void)
{
	_LCD_Attributes._cursorDisplayShift._shiftDisplay = SET;
	_LCD_Attributes._cursorDisplayShift._shiftRight = SET;
	return _writeInstruction(*((BYTE*)&_LCD_Attributes._cursorDisplayShift));
}

//Display

//FUnction: MIC_RC setCursor (BYTE row, BYTE column)
//Input: column number and row number (all starts from 1)
MIC_RC MIC_LCD::setCursor (BYTE row, BYTE column)
{
	const BYTE AC_baseAddr[4] = {0, 0x40, 0x14, 0x54};
	MIC_RC returnCode = MIC_RC_SUCCESS;
	BYTE AC = 0;
	BYTE instruciton = 0;

	if ((column > _LCD_Attributes._column) || (row > _LCD_Attributes._row))
	{
		returnCode = MIC_RC_LCD_ERROR;
	}
	else
	{
		AC = AC_baseAddr[(row - 1)] + (column - 1);
		returnCode = _writeInstruction(MIC_LCD_INST_SETDDRAMADDR + (AC & MIC_LCD_INST_SETDDRAMADDR_ADDRMASK));
	}

	return returnCode;
}

//Show a string from a specific screen location
MIC_RC MIC_LCD::displayStr (BYTE row, BYTE column, CHAR8* string, BYTE strLen)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	INT8 counter = 0;

	if ((row > _LCD_Attributes._row) || ((column + strLen - 1) > _LCD_Attributes._column))
	{
		returnCode = MIC_RC_LCD_ERROR;
	}

	if (strlen(string) > strLen)
	{
		returnCode = MIC_RC_LCD_ERROR;
	}

	if (returnCode == MIC_RC_SUCCESS)
	{
		returnCode = setCursor(row, column);
	}

	if (returnCode == MIC_RC_SUCCESS)
	{
		for (counter = 0; counter < strLen; counter++)
		{
			if (returnCode == MIC_RC_SUCCESS)
			{
				returnCode = _writeData(string[counter]);
			}
		}
	}

	return returnCode;
}

//Show a number from a specific screen location
MIC_RC MIC_LCD::displayNum (BYTE row, BYTE column, INT32 number)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	CHAR8 numStr[11] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};		//INT32 has 10 digits in decimal
	UINT8 strLen = 0;

	strLen = sprintf(numStr, "%d", number);

	if ((row <= _LCD_Attributes._row) && ((column + strLen - 1) <= _LCD_Attributes._column))
	{
		returnCode = displayStr(row, column, numStr, strLen);
	}
	else
	{
		returnCode = MIC_RC_LCD_ERROR;
	}

	return returnCode;
}

// Function: MIC_RC displayTime (BYTE row, BYTE column, BYTE hr, BYTE min, BYTE sec)
MIC_RC MIC_LCD::displayTime(BYTE row, BYTE column, BYTE hr, BYTE min, BYTE sec)
{
	MIC_RC returnCode = MIC_RC_SUCCESS;
	CHAR8 timeStr[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	BYTE strLen = 0;

	if ((hr > 23) || (min > 59) || (sec > 59))
	{
		returnCode = MIC_RC_LCD_ERROR;
	}
	else
	{
		strLen = sprintf(timeStr, "%02d:%02d:%02d", hr, min, sec);
		returnCode = displayStr(row, column, timeStr, strLen);
	}

	return returnCode;
}
