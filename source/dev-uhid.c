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
/*              USB Access - BSD's UHID driver                  */
/*                                                              */
/****************************************************************/

// Thanks to Todd T Fries for help getting this working on OpenBSD
// and to Chris Kuethe for the initial patch to use UHID.

#include <dev/usb/usb.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#ifndef USB_GET_DEVICEINFO
#include <dev/usb/usb_ioctl.h>
#endif

int open_usb_device(int vid, int pid)
{
	int                    r, fd;
	DIR*                   dir;
	struct dirent*         d;
	struct usb_device_info info;
	char                   buf[256];

	dir = opendir("/dev");
	if (!dir)
		return -1;
	while ((d = readdir(dir)) != NULL) {
		if (strncmp(d->d_name, "uhid", 4) != 0)
			continue;
		snprintf(buf, sizeof(buf), "/dev/%s", d->d_name);
		fd = open(buf, O_RDWR);
		if (fd < 0)
			continue;
		r = ioctl(fd, USB_GET_DEVICEINFO, &info);
		if (r < 0) {
			// NetBSD: added in 2004
			// OpenBSD: added November 23, 2009
			// FreeBSD: missing (FreeBSD 8.0) - USE_LIBUSB works!
			die("Error: your uhid driver does not support"
				" USB_GET_DEVICEINFO, please upgrade!\n");
			close(fd);
			closedir(dir);
			exit(1);
		}
		//printf("%s: v=%d, p=%d\n", buf, info.udi_vendorNo, info.udi_productNo);
		if (info.udi_vendorNo == vid && info.udi_productNo == pid) {
			closedir(dir);
			return fd;
		}
		close(fd);
	}
	closedir(dir);
	return -1;
}

static int uhid_teensy_fd = -1;

int teensy_open(void)
{
	teensy_close();
	uhid_teensy_fd = open_usb_device(0x16C0, 0x0478);
	if (uhid_teensy_fd < 0)
		return 0;
	return 1;
}

int teensy_write(void* buf, int len, double timeout)
{
	int r;

	// TODO: imeplement timeout... how??
	r = write(uhid_teensy_fd, buf, len);
	if (r == len)
		return 1;
	return 0;
}

void teensy_close(void)
{
	if (uhid_teensy_fd >= 0) {
		close(uhid_teensy_fd);
		uhid_teensy_fd = -1;
	}
}

int hard_reboot(void)
{
	int r, rebootor_fd;

	rebootor_fd = open_usb_device(0x16C0, 0x0477);
	if (rebootor_fd < 0)
		return 0;
	r = write(rebootor_fd, "reboot", 6);
	delay(0.1);
	close(rebootor_fd);
	if (r == 6)
		return 1;
	return 0;
}

int soft_reboot(void)
{
	printf("Soft reboot is not implemented for UHID\n");
	return 0;
}
