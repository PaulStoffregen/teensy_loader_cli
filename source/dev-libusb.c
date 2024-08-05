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

#include "dev.h"

/****************************************************************/
/*                                                              */
/*             USB Access - libusb (Linux & FreeBSD)            */
/*                                                              */
/*  Uses libusb v0.1. To install:                               */
/*  - [debian, ubuntu, mint] apt install libusb-dev             */
/*  - [redhat, centos]       yum install libusb-devel           */
/*  - [fedora]               dnf install libusb-devel           */
/*  - [arch linux]           pacman -S libusb-compat            */
/*  - [gentoo]               emerge dev-libs/libusb-compat      */
/*                                                              */
/*  - [freebsd]              seems to be preinstalled           */
/****************************************************************/

// http://libusb.sourceforge.net/doc/index.html
#include <stdio.h>
#include <unistd.h>
#include <usb.h>
#include "misc.h"

usb_dev_handle* open_usb_device(int vid, int pid)
{
	struct usb_bus*    bus;
	struct usb_device* dev;
	usb_dev_handle*    h;
	char               buf[128];
	int                r;

	usb_init();
	usb_find_busses();
	usb_find_devices();
	//printf_verbose("\nSearching for USB device:\n");
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			//printf_verbose("bus \"%s\", device \"%s\" vid=%04X, pid=%04X\n",
			//	bus->dirname, dev->filename,
			//	dev->descriptor.idVendor,
			//	dev->descriptor.idProduct
			//);
			if (dev->descriptor.idVendor != vid)
				continue;
			if (dev->descriptor.idProduct != pid)
				continue;
			h = usb_open(dev);
			if (!h) {
				printf_verbose("Found device but unable to open\n");
				continue;
			}
#ifdef LIBUSB_HAS_GET_DRIVER_NP
			r = usb_get_driver_np(h, 0, buf, sizeof(buf));
			if (r >= 0) {
				r = usb_detach_kernel_driver_np(h, 0);
				if (r < 0) {
					usb_close(h);
					printf_verbose("Device is in use by \"%s\" driver\n", buf);
					continue;
				}
			}
#endif
// Mac OS-X - removing this call to usb_claim_interface() might allow
// this to work, even though it is a clear misuse of the libusb API.
// normally Apple's IOKit should be used on Mac OS-X
#if !defined(MACOSX)
			r = usb_claim_interface(h, 0);
			if (r < 0) {
				usb_close(h);
				printf_verbose("Unable to claim interface, check USB permissions\n");
				continue;
			}
#endif

			return h;
		}
	}
	return NULL;
}

static usb_dev_handle* libusb_teensy_handle = NULL;

int teensy_open(void)
{
	teensy_close();
	libusb_teensy_handle = open_usb_device(0x16C0, 0x0478);
	if (libusb_teensy_handle)
		return 1;
	return 0;
}

int teensy_write(void* buf, int len, double timeout)
{
	int r;

	if (!libusb_teensy_handle)
		return 0;
	while (timeout > 0) {
		r = usb_control_msg(libusb_teensy_handle, 0x21, 9, 0x0200, 0, (char*)buf, len, (int)(timeout * 1000.0));
		if (r >= 0)
			return 1;
		//printf("teensy_write, r=%d\n", r);
		usleep(10000);
		timeout -= 0.01; // TODO: subtract actual elapsed time
	}
	return 0;
}

void teensy_close(void)
{
	if (!libusb_teensy_handle)
		return;
	usb_release_interface(libusb_teensy_handle, 0);
	usb_close(libusb_teensy_handle);
	libusb_teensy_handle = NULL;
}

int hard_reboot(void)
{
	usb_dev_handle* rebootor;
	int             r;

	rebootor = open_usb_device(0x16C0, 0x0477);
	if (!rebootor)
		return 0;
	r = usb_control_msg(rebootor, 0x21, 9, 0x0200, 0, "reboot", 6, 100);
	usb_release_interface(rebootor, 0);
	usb_close(rebootor);
	if (r < 0)
		return 0;
	return 1;
}

int soft_reboot(void)
{
	usb_dev_handle* serial_handle = NULL;

	serial_handle = open_usb_device(0x16C0, 0x0483);
	if (!serial_handle) {
		char* error = usb_strerror();
		printf("Error opening USB device: %s\n", error);
		return 0;
	}

	char reboot_command[] = {0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08};
	int  response         = usb_control_msg(serial_handle, 0x21, 0x20, 0, 0, reboot_command, sizeof reboot_command, 10000);

	usb_release_interface(serial_handle, 0);
	usb_close(serial_handle);

	if (response < 0) {
		char* error = usb_strerror();
		printf("Unable to soft reboot with USB error: %s\n", error);
		return 0;
	}

	return 1;
}
