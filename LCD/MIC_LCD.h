#ifndef MIC_LCD_h
#define MIC_LCD_h

// Instruction format: Entry Mode Set
typedef struct
{
	BYTE _ShiftDisplay : 1; // bit (0), SET = shift display, CLEAR = shift cursot
	BYTE _shiftRight : 1;	// bit (1), SET = Increament (shift to right), CLEAR = Decreament (shift to left)
	BYTE _instruction : 6;	// bit (7-2), instruction for entry mode (0b000001)
} ENTRYMODESET;

// Instruction format: Display ON OFF
typedef struct
{
	BYTE _blink : 1;	   // bit (0), SET = cursor blink on, CLEAR = cursor blink off
	BYTE _cursor : 1;	   // bit (1), SET = cursor on, CLEAR = cursor off
	BYTE _display : 1;	   // bit (2), SET = display on, CLEAR = display off
	BYTE _instruction : 5; // bit (7-3), instruction for display ON/OFF (0b00001)
} DISPLAYONOFF;

// Instruction format: Cursor or Display Shift
typedef struct
{
	BYTE _reserved : 2;		// bit (1-0), reserved bits
	BYTE _shiftRight : 1;	// bit (2), SET = shift right, CLEAR = shift left
	BYTE _shiftDisplay : 1; // bit (3), SET = shift display, CLEAR = shift cursor
	BYTE _instruction : 4;	// bit (7-4), instruction for cursor or display shift (0b0001)
} CURSORDISPLAYSHIFT;

// Instruction format: Function Set
typedef struct
{
	BYTE _reserved : 2;	   // bit (1-0), reserved bits
	BYTE _5x11Format : 1;  // bit 2, SET = 5x11 dots format, CLEAR = 5x8 dots format
	BYTE _2LineMode : 1;   // bit 3, SET = 2-line display mode, CLEAR = 1-line display mode
	BYTE _8BitBus : 1;	   // bit 4, SET = 8-bit bus mode, CLEAR = 4-bit bus mode
	BYTE _instruction : 3; // bit (7-5), instruction for function set (0b001)
} FUNCTIONSET;

// Status register format
typedef struct
{
	BYTE AC : 7;   // bit (6-0), Address Counter
	BYTE busy : 1; // bit 7, busy status
} MIC_LCD_STATUS;

class MIC_LCD
{
public:
	// LCD setup
	// If LCD is configured as 4 bit bus mode, DB3-DB0 should be set to 0xff.
	// This program does not perform boundary check for PIN number assignment.
	MIC_LCD(BYTE RS, BYTE EN, BYTE RW,
			BYTE DB7, BYTE DB6, BYTE DB5, BYTE DB4, BYTE DB3, BYTE DB2, BYTE DB1, BYTE DB0);

	// Currently, this program supports 1, 2, 3 and 4 lines mode (3 and 4 lines modes have not been tested yet)
	// This program does not perform row and column boundary test
	// All PIN modes are set to output after PORST
	// default functions are set as:
	// Entry Mode: Cursor shift left
	// Bus mode: if DB3-DB0 are set to 0xff, it's 4 bit mode. Bus mode can not be changed and is only defined by PIN assignment
	// Display mode: 2-line
	// Font format: 5x8 dots
	// Display: ON
	// Cursor: OFF
	// Blink: OFF

	MIC_RC PORST(BYTE row, BYTE column);

	MIC_RC clearDisplay(void);
	MIC_RC returnHome(void);

	MIC_RC entryModeCursorLeft(void);
	MIC_RC entryModeCursorRight(void);
	MIC_RC entryModeDisplayLeft(void);
	MIC_RC entryModeDisplayRight(void);

	MIC_RC DisplayMode1Line(void);
	MIC_RC DisplayMode2Line(void);
	MIC_RC FontFormat5x8(void);
	MIC_RC FontFormat5x11(void);

	MIC_RC displayON(void);
	MIC_RC displayOFF(void);
	MIC_RC cursorON(void);
	MIC_RC cursorOFF(void);
	MIC_RC blinkON(void);
	MIC_RC blinkOFF(void);

	MIC_RC cursorShiftLEFT(void);
	MIC_RC cursorShiftRIGHT(void);
	MIC_RC displayShiftLEFT(void);
	MIC_RC displayShiftRIGHT(void);

	MIC_RC setCursor(BYTE row, BYTE column); // Input: row number and column number (all starts from 1)
	MIC_RC displayStr(BYTE row, BYTE column, CHAR8 *string, BYTE strLen);
	MIC_RC displayNum(BYTE row, BYTE column, INT32 number);
	MIC_RC displayTime(BYTE row, BYTE column, BYTE hr, BYTE min, BYTE sec);

private:
	// Variables
	struct
	{
		BYTE _RS_PIN;
		BYTE _EN_PIN;
		BYTE _RW_PIN;
		BYTE _DB_PIN[8];

		BYTE _column;
		BYTE _row;

		ENTRYMODESET _entryModeSet;
		DISPLAYONOFF _displayONOFF;
		CURSORDISPLAYSHIFT _cursorDisplayShift;
		FUNCTIONSET _functionSet;

	} _LCD_Attributes;

	// Private functions
	// Function: BYTE _readBits(void)
	// For 4 bits bus mode, DB7 - 4 are read to higher 4 bits;
	// For 8 bits bus mode, all bits are read out.
	BYTE _readBits(void);

	// Function: BYTE _readBYTE(void)
	BYTE _readBYTE(void);

	// Function: void _writeBits(BYTE bitsWritten)
	// For 4 bits bus mode: higher 4 bits are written in the cycle
	// For 8 bits bus mode: all 8 bits are written in the cycle
	void _writeBits(BYTE bitsWritten);

	// Function: void _write_BYTE (BYTE byte)
	void _writeBYTE(BYTE byte);

	MIC_LCD_STATUS _readStatus(void);
	MIC_RC _LCDReady(void);

	MIC_RC _writeInstruction(BYTE instruction);
	MIC_RC _readData(BYTE *data);
	MIC_RC _writeData(BYTE data);
};

#endif
