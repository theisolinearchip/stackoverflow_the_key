/* Name: main.c
 * Project: hid-mouse, a very simple HID example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-07
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.

We use VID/PID 0x046D/0xC00E which is taken from a Logitech mouse. Don't
publish any hardware using these IDs! This is for demonstration only!
*/

/* 
	v-usb to PB2 and PB3
	8-bit shifter input: PB2
	8-bit shifter latch: PB0
	8-bit shifter clock: PB5
	button: PB4

	8-bit shifter: the first ones are shifted to the end (so in a 8 loop
	the 7th	position will on the first chip output)
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

#define PIN_BUTTON_URL				PB0 // type "stack overflow"
#define PIN_BUTTON_COPY				PB4 // ctrl + c
#define PIN_BUTTON_PASTE			PB2 // ctrl + v

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[35] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

#define MOD_CONTROL_LEFT    (1<<0)
#define MOD_SHIFT_LEFT      (1<<1)
#define MOD_ALT_LEFT        (1<<2)
#define MOD_GUI_LEFT        (1<<3)
#define MOD_CONTROL_RIGHT   (1<<4)
#define MOD_SHIFT_RIGHT     (1<<5)
#define MOD_ALT_RIGHT       (1<<6)
#define MOD_GUI_RIGHT       (1<<7)

#define KEY_C				0x06
#define KEY_V				0x19

#define BUTTON_URL_LEN		18
const char buttonUrl[BUTTON_URL_LEN] = {
	0x16, 0x17, 0x04, 0x06, 0x0e, 0x12, 0x19, 0x08, 0x15, 0x09, 0x0f, 0x12, 0x1a, 0x37, 0x06, 0x12, 0x10, 0x28
};
// full url + ENTER key (18), "stackoverflow.com"
// notice that with this method we cannot send any mod key on the "type-the-url option"
// (can create a pair of key + mod in case the characters for the url need some kind
// of mod key according to the text and the keyboard layout; pending for now)

typedef struct{
	uchar modifier;
	uchar key;
}report_t;

static report_t reportBuffer;

static uchar    idleRate;   /* repeat rate for keyboards, never used for mice */

static uchar buttonUrlPressed;
static uchar buttonUrlPressedPreviously;
static uchar buttonCopyPressed;
static uchar buttonCopyPressedPreviously;
static uchar buttonPastePressed;
static uchar buttonPastePressedPreviously;

static uchar currentKey;
static uchar currentMod;
static urlIndex;

static uchar processAvailable;


/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
usbRequest_t    *rq = (void *)data;

	/* The following requests are never used. But since they are required by
	 * the specification, we implement them in this example.
	 */
	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
		if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			usbMsgPtr = (void *)&reportBuffer;
			return sizeof(reportBuffer);
		}else if(rq->bRequest == USBRQ_HID_GET_IDLE){
			usbMsgPtr = &idleRate;
			return 1;
		}else if(rq->bRequest == USBRQ_HID_SET_IDLE){
			idleRate = rq->wValue.bytes[1];
		}
	}else{
		/* no vendor specific requests implemented */
	}
	return 0;   /* default for not implemented requests: return no data back to host */
}

static void buildReport(void) {
	reportBuffer.key = currentKey;
	reportBuffer.modifier = currentMod;

	currentKey = 0;
	currentMod = 0;

	processAvailable = 1;
}

static void processData(void) {

	if (processAvailable) {

		buttonUrlPressed = !(PINB & (1 << PIN_BUTTON_URL));
		buttonCopyPressed = !(PINB & (1 << PIN_BUTTON_COPY));
		buttonPastePressed = !(PINB & (1 << PIN_BUTTON_PASTE));

		if (urlIndex > 0) {
			// continue typing the url

			if (urlIndex < BUTTON_URL_LEN) {
				currentKey = buttonUrl[urlIndex];
				currentMod = 0;

				urlIndex++;
			} else {
				urlIndex = 0;
			}

			processAvailable = 0;
		} else {

			// handle regular buttons
			if (buttonUrlPressed && !buttonUrlPressedPreviously) {
				// set first url element and advance

				currentKey = buttonUrl[0];
				currentMod = 0;

				urlIndex = 1;
			} else if (buttonCopyPressed && !buttonCopyPressedPreviously) {
				currentKey = KEY_C;
				currentMod = MOD_CONTROL_LEFT;
			} else if (buttonPastePressed && !buttonPastePressedPreviously) {
				currentKey = KEY_V;
				currentMod = MOD_CONTROL_LEFT;
			} else {
				// nothing pressed, do nothing
				currentKey = 0;
				currentMod = 0;
			}

			processAvailable = 0;
		}

		buttonUrlPressedPreviously = buttonUrlPressed;
		buttonCopyPressedPreviously = buttonCopyPressed;
		buttonPastePressedPreviously = buttonPastePressed;
	}
}

/* ------------------------------------------------------------------------- */


int main(void) {

	PORTB |= (1 << PIN_BUTTON_URL);
	DDRB &= ~(1 << PIN_BUTTON_URL);

	PORTB |= (1 << PIN_BUTTON_COPY);
	DDRB &= ~(1 << PIN_BUTTON_COPY);

	PORTB |= (1 << PIN_BUTTON_PASTE);
	DDRB &= ~(1 << PIN_BUTTON_PASTE);

	buttonUrlPressed = 0;
	buttonUrlPressedPreviously = 0;

	buttonCopyPressed = 0;
	buttonCopyPressedPreviously = 0;

	buttonPastePressed = 0;
	buttonPastePressedPreviously = 0;
	
	currentKey = 0;
	currentMod = 0;
	urlIndex = 0;

	processAvailable = 1;

	uchar i;

	wdt_enable(WDTO_1S);
	/* Even if you don't use the watchdog, turn it off here. On newer devices,
	 * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
	 */
	/* RESET status: all port bits are inputs without pull-up.
	 * That's the way we need D+ and D-. Therefore we don't need any
	 * additional hardware initialization.
	 */
	usbInit();
	usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
	i = 0;
	while(--i) {             /* fake USB disconnect for > 250 ms */
		wdt_reset(); 
		_delay_ms(1);
	}
	usbDeviceConnect();
	sei();
	while(1) {

		processData();

		wdt_reset();
		usbPoll();
		if (usbInterruptIsReady()) {
			buildReport();

			usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
		}

		_delay_ms(10);
	}
}

/* ------------------------------------------------------------------------- */
