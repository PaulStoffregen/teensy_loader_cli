/* Teensy Loader, Command Line Interface
 * Program and Reboot Teensy Board with HalfKay Bootloader
 * http://www.pjrc.com/teensy/loader_cli.html
 * Copyright 2008-2016, PJRC.COM, LLC
 *
 * You may redistribute this program and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 */

/* Want to incorporate this code into a proprietary application??
 * Just email paul@pjrc.com to ask.  Usually it's not a problem,
 * but you do need to ask to use this code in any way other than
 * those permitted by the GNU General Public License, version 3  */

/* For non-root permissions on ubuntu or similar udev-based linux
 * http://www.pjrc.com/teensy/49-teensy.rules
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dev.h"
#include "ihex.h"
#include "misc.h"
//#include "param.h"

// options (from user via command line args)
int         wait_for_device_to_appear = 0;
int         hard_reboot_device        = 0;
int         soft_reboot_device        = 0;
int         reboot_after_programming  = 1;
int         verbose                   = 0;
int         boot_only                 = 0;
int         code_size = 0, block_size = 0;
const char* filename = NULL;

/****************************************************************/
/*                                                              */
/*                       Main Program                           */
/*                                                              */
/****************************************************************/

int main(int argc, char** argv)
{
	unsigned char buf[2048];
	int           num, addr, r, write_size;

	int block_count = 0, waited = 0;

	// parse command line arguments
	parse_options(argc, argv);
	if (!filename && !boot_only) {
		usage("Filename must be specified");
	}
	if (!code_size) {
		usage("MCU type must be specified");
	}
	printf_verbose("Teensy Loader, Command Line, Version 2.3\n");

	if (block_size == 512 || block_size == 1024) {
		write_size = block_size + 64;
	} else {
		write_size = block_size + 2;
	};

	if (!boot_only) {
		// read the intel hex file
		// this is done first so any error is reported before using USB
		num = read_intel_hex(filename);
		if (num < 0)
			die("error reading intel hex file \"%s\"", filename);
		printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n", filename, num, (double)num / (double)code_size * 100.0);
	}

	// open the USB device
	while (1) {
		if (teensy_open())
			break;
		if (hard_reboot_device) {
			if (!hard_reboot())
				die("Unable to find rebootor\n");
			printf_verbose("Hard Reboot performed\n");
			hard_reboot_device        = 0; // only hard reboot once
			wait_for_device_to_appear = 1;
		}
		if (soft_reboot_device) {
			if (soft_reboot()) {
				printf_verbose("Soft reboot performed\n");
			}
			soft_reboot_device        = 0;
			wait_for_device_to_appear = 1;
		}
		if (!wait_for_device_to_appear)
			die("Unable to open device (hint: try -w option)\n");
		if (!waited) {
			printf_verbose("Waiting for Teensy device...\n");
			printf_verbose(" (hint: press the reset button)\n");
			waited = 1;
		}
		delay(0.25);
	}
	printf_verbose("Found HalfKay Bootloader\n");

	if (boot_only) {
		boot(buf, write_size);
		teensy_close();
		return 0;
	}

	// if we waited for the device, read the hex file again
	// perhaps it changed while we were waiting?
	if (waited) {
		num = read_intel_hex(filename);
		if (num < 0)
			die("error reading intel hex file \"%s\"", filename);
		printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n", filename, num, (double)num / (double)code_size * 100.0);
	}

	// program the data
	printf_verbose("Programming");
	fflush(stdout);
	for (addr = 0; addr < code_size; addr += block_size) {
		if (block_count > 0) {
			// don't waste time on blocks that are unused,
			// but always do the first one to erase the chip
			if (!ihex_bytes_within_range(addr, addr + block_size - 1))
				continue;
			if (memory_is_blank(addr, block_size))
				continue;
		}
		printf_verbose(".");
		if (block_size <= 256 && code_size < 0x10000) {
			buf[0] = addr & 255;
			buf[1] = (addr >> 8) & 255;
			ihex_get_data(addr, block_size, buf + 2);
			write_size = block_size + 2;
		} else if (block_size == 256) {
			buf[0] = (addr >> 8) & 255;
			buf[1] = (addr >> 16) & 255;
			ihex_get_data(addr, block_size, buf + 2);
			write_size = block_size + 2;
		} else if (block_size == 512 || block_size == 1024) {
			buf[0] = addr & 255;
			buf[1] = (addr >> 8) & 255;
			buf[2] = (addr >> 16) & 255;
			memset(buf + 3, 0, 61);
			ihex_get_data(addr, block_size, buf + 64);
			write_size = block_size + 64;
		} else {
			die("Unknown code/block size\n");
		}
		r = teensy_write(buf, write_size, block_count <= 4 ? 45.0 : 0.5);
		if (!r)
			die("error writing to Teensy\n");
		block_count = block_count + 1;
	}
	printf_verbose("\n");

	// reboot to the user's new code
	if (reboot_after_programming) {
		boot(buf, write_size);
	}
	teensy_close();
	return 0;
}
