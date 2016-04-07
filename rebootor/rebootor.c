/* Teensy Rebootor
 * http://www.pjrc.com/teensy
 * Copyright (c) 2010 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

int main(void)
{
	// set for 1 MHz clock
	CPU_PRESCALE(4);

	// set all pins as inputs with pullup resistors
	#if defined(PORTA)
	DDRF = 0;
	PORTF = 0xFF;
	#endif
	DDRB = 0;
	PORTB = 0xFF;
	DDRC = 0;
	PORTC = 0xFF;
	DDRD = 0;
	PORTD = 0xFF;
	#if defined(PORTE)
	DDRE = 0;
	PORTE = 0xFF;
	#endif
	#if defined(PORTF)
	DDRF = 0;
	PORTF = 0xFF;
	#endif

	// initialize USB
	usb_init();

	// do nothing (USB code handles reboot message)
	while (1) {
		_delay_ms(1);
		// put the CPU into low power idle mode
		set_sleep_mode(SLEEP_MODE_IDLE);
		cli();
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
	}
}



