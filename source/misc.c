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

#include "misc.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dev.h"
#include "param.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif

/****************************************************************/
/*                                                              */
/*                       Misc Functions                         */
/*                                                              */
/****************************************************************/

int printf_verbose(const char* format, ...)
{
	va_list ap;
	int     r;

	va_start(ap, format);
	if (verbose) {
		r = vprintf(format, ap);
		fflush(stdout);
		return r;
	}
	return 0;
}

void delay(double seconds)
{
#ifdef WIN32
	Sleep(seconds * 1000.0);
#else
	usleep(seconds * 1000000.0);
#endif
}

void die(const char* str, ...)
{
	va_list ap;

	va_start(ap, str);
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
	exit(1);
}

#if defined(WIN32)
#define strcasecmp stricmp
#endif

static const struct {
	const char* name;
	int         code_size;
	int         block_size;
} MCUs[] = {
	{"at90usb162", 15872, 128},
	{"atmega32u4", 32256, 128},
	{"at90usb646", 64512, 256},
	{"at90usb1286", 130048, 256},
#if defined(USE_LIBUSB) || defined(USE_APPLE_IOKIT) || defined(USE_WIN32)
	{"mkl26z64", 63488, 512},
	{"mk20dx128", 131072, 1024},
	{"mk20dx256", 262144, 1024},
	{"mk66fx1m0", 1048576, 1024},
	{"mk64fx512", 524288, 1024},
	{"imxrt1062", 2031616, 1024},

	// Add duplicates that match friendly Teensy Names
	// Match board names in boards.txt
	{"TEENSY2", 32256, 128},
	{"TEENSY2PP", 130048, 256},
	{"TEENSYLC", 63488, 512},
	{"TEENSY30", 131072, 1024},
	{"TEENSY31", 262144, 1024},
	{"TEENSY32", 262144, 1024},
	{"TEENSY35", 524288, 1024},
	{"TEENSY36", 1048576, 1024},
	{"TEENSY40", 2031616, 1024},
	{"TEENSY41", 8126464, 1024},
	{"TEENSY_MICROMOD", 16515072, 1024},
#endif
	{NULL, 0, 0},
};

void list_mcus()
{
	int i;
	printf("Supported MCUs are:\n");
	for (i = 0; MCUs[i].name != NULL; i++)
		printf(" - %s\n", MCUs[i].name);
	exit(1);
}

void read_mcu(char* name)
{
	int i;

	if (name == NULL) {
		fprintf(stderr, "No MCU specified.\n");
		list_mcus();
	}

	for (i = 0; MCUs[i].name != NULL; i++) {
		if (strcasecmp(name, MCUs[i].name) == 0) {
			code_size  = MCUs[i].code_size;
			block_size = MCUs[i].block_size;
			return;
		}
	}

	fprintf(stderr, "Unknown MCU type \"%s\"\n", name);
	list_mcus();
}

void parse_flag(char* arg)
{
	int i;
	for (i = 1; arg[i]; i++) {
		switch (arg[i]) {
		case 'w':
			wait_for_device_to_appear = 1;
			break;
		case 'r':
			hard_reboot_device = 1;
			break;
		case 's':
			soft_reboot_device = 1;
			break;
		case 'n':
			reboot_after_programming = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'b':
			boot_only = 1;
			break;
		default:
			fprintf(stderr, "Unknown flag '%c'\n\n", arg[i]);
			usage(NULL);
		}
	}
}

void parse_options(int argc, char** argv)
{
	int   i;
	char* arg;

	for (i = 1; i < argc; i++) {
		arg = argv[i];

		//backward compatibility with previous versions.
		if (strncmp(arg, "-mmcu=", 6) == 0) {
			read_mcu(strchr(arg, '=') + 1);
		}

		else if (arg[0] == '-') {
			if (arg[1] == '-') {
				char* name = &arg[2];
				char* val  = strchr(name, '=');
				if (val == NULL) {
					//value must be the next string.
					val = argv[++i];
				} else {
					//we found an =, so split the string at it.
					*val = '\0';
					val  = &val[1];
				}

				if (strcasecmp(name, "help") == 0)
					usage(NULL);
				else if (strcasecmp(name, "mcu") == 0)
					read_mcu(val);
				else if (strcasecmp(name, "list-mcus") == 0)
					list_mcus();
				else {
					fprintf(stderr, "Unknown option \"%s\"\n\n", arg);
					usage(NULL);
				}
			} else
				parse_flag(arg);
		} else
			filename = arg;
	}
}

void boot(unsigned char* buf, int write_size)
{
	printf_verbose("Booting\n");
	memset(buf, 0, write_size);
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = 0xFF;
	teensy_write(buf, write_size, 0.5);
}

void usage(const char* err)
{
	if (err != NULL)
		fprintf(stderr, "%s\n\n", err);
	fprintf(stderr,
			"Usage: teensy_loader_cli --mcu=<MCU> [-w] [-h] [-n] [-b] [-v] <file.hex>\n"
			"\t-w : Wait for device to appear\n"
			"\t-r : Use hard reboot if device not online\n"
			"\t-s : Use soft reboot if device not online (Teensy 3.x & 4.x)\n"
			"\t-n : No reboot after programming\n"
			"\t-b : Boot only, do not program\n"
			"\t-v : Verbose output\n"
			"\nUse `teensy_loader_cli --list-mcus` to list supported MCUs.\n"
			"\nFor more information, please visit:\n"
			"http://www.pjrc.com/teensy/loader_cli.html\n");
	exit(1);
}
