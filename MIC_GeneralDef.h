#ifndef _MIC_GENERALDEF_H_
#define _MIC_GENERALDEF_H_

// Base Types
// Primitive Types
#ifndef NULL
#define NULL (0)
#endif

//		Type		Name		Description
#ifndef UINT8
typedef uint8_t		UINT8;		// unsigned, 8-bit integer
#endif

#ifndef BYTE
typedef uint8_t 	BYTE;		// unsigned 8-bit integer
#endif

#ifndef INT8
typedef int8_t		INT8;		// signed, 8-bit integer
#endif

#ifndef CHAR8
typedef char		CHAR8;		// charater, 8-bit integer, defined in uEFI
#endif

#ifndef BOOL
typedef BYTE		BOOL;		// a bit in a BYTE
								// This is not used across the interface but is used in many places in the code. If the type were sent on the interface, it would have to have a type with a specific number of bytes.
#endif

#ifndef YES_NO
typedef BYTE 		YES_NO;		// a bit in a BYTE
#endif

#ifndef UINT16
typedef uint16_t	UINT16;		// unsigned, 16-bit integer
#endif

#ifndef INT16
typedef int16_t		INT16;		// signed, 16-bit integer
#endif

#ifndef UINT32
typedef uint32_t	UINT32;		// unsigned, 32-bit integer
#endif

#ifndef INT32
typedef int32_t		INT32;		// signed, 32-bit integer
#endif



// 5.2 Specification Logic Value Constants
// Table 4 — Defines for Logic Values
//		Name		Value		Description
#ifndef TRUE
#define TRUE		1
#endif

#ifndef FALSE
#define FALSE		0
#endif

#ifndef YES
#define YES			1
#endif

#ifndef NO
#define NO			0
#endif

#ifndef SET
#define SET			1
#endif

#ifndef CLEAR
#define CLEAR		0
#endif

// General data structure
typedef UINT16		MIC_RC;		// return code
#define MIC_RC_SUCCESS			0x00
#define MIC_RC_LCD_ERROR		0x0100

// Debug outputs
#define MIC_DEBUG_MAXNAMESTRINGLEN			128
#define MIC_DEBUG_ARRAYDIGITSPERLINE		16
#define MIC_DEBUG_MAXCONSTABLELEN			128 // includes end of table item (string = NULL)

typedef struct
{
	UINT32 constant;			// all cast to UINT32
	CHAR8 *name;				// all values assigned with a name should be populated with a name string. Last item in table should be with NULL string
} MIC_DEBUG_CONSTANTNAME;

#endif