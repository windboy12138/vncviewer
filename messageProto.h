#ifndef MESSAGE_PROTO_H
#define MESSAGE_PROTO_H
#include <stdint.h>

typedef struct {
	uint8_t type;			/* always rfbPointerEvent */
	//uint8_t buttonMask;		/* bits 0-7 are buttons 1-8, 0=up, 1=down */
	//uint16_t x;
	//uint16_t y;
	int32_t buttonMask;
	int32_t x;
	int32_t y;
} rfbPointerEventMsg;

#define rfbButton1Mask 1
#define rfbButton2Mask 2
#define rfbButton3Mask 4
#define rfbButton4Mask 8
#define rfbButton5Mask 16
#define rfbWheelUpMask rfbButton4Mask
#define rfbWheelDownMask rfbButton5Mask

#define sz_rfbPointerEventMsg 6

typedef struct {
	uint8_t type;			/* always rfbKeyEvent */
	uint8_t down;			/* true if down (press), false if up */
	uint16_t pad;
	uint32_t key;			/* key is specified as an X keysym */
} rfbKeyEventMsg;

#define sz_rfbKeyEventMsg 8

#define Swap16IfLE(s) \
    ((uint16_t) ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))
#define Swap32IfLE(l) \
    ((uint32_t) ((((l) & 0xff000000) >> 24) | \
     (((l) & 0x00ff0000) >> 8)  | \
	 (((l) & 0x0000ff00) << 8)  | \
	 (((l) & 0x000000ff) << 24)))
#endif