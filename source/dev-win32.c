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
/*               USB Access - Microsoft WIN32                   */
/*                                                              */
/****************************************************************/

// http://msdn.microsoft.com/en-us/library/ms790932.aspx
#include <inttypes.h>
#include <stdio.h>
#include <windows.h>
#include <hidclass.h>
#include <hidsdi.h>
#include <setupapi.h>

HANDLE open_usb_device(int vid, int pid)
{
	GUID                             guid;
	HDEVINFO                         info;
	DWORD                            index, required_size;
	SP_DEVICE_INTERFACE_DATA         iface;
	SP_DEVICE_INTERFACE_DETAIL_DATA* details;
	HIDD_ATTRIBUTES                  attrib;
	HANDLE                           h;
	BOOL                             ret;

	HidD_GetHidGuid(&guid);
	info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE)
		return NULL;
	for (index = 0; 1; index++) {
		iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		ret          = SetupDiEnumDeviceInterfaces(info, NULL, &guid, index, &iface);
		if (!ret) {
			SetupDiDestroyDeviceInfoList(info);
			break;
		}
		SetupDiGetInterfaceDeviceDetail(info, &iface, NULL, 0, &required_size, NULL);
		details = (SP_DEVICE_INTERFACE_DETAIL_DATA*)malloc(required_size);
		if (details == NULL)
			continue;
		memset(details, 0, required_size);
		details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ret             = SetupDiGetDeviceInterfaceDetail(info, &iface, details, required_size, NULL, NULL);
		if (!ret) {
			free(details);
			continue;
		}
		h = CreateFile(details->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		free(details);
		if (h == INVALID_HANDLE_VALUE)
			continue;
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		ret         = HidD_GetAttributes(h, &attrib);
		if (!ret) {
			CloseHandle(h);
			continue;
		}
		if (attrib.VendorID != vid || attrib.ProductID != pid) {
			CloseHandle(h);
			continue;
		}
		SetupDiDestroyDeviceInfoList(info);
		return h;
	}
	return NULL;
}

int write_usb_device(HANDLE h, void* buf, int len, int timeout)
{
	static HANDLE event = NULL;
	unsigned char tmpbuf[1089];
	OVERLAPPED    ov;
	DWORD         n, r;

	if (len > sizeof(tmpbuf) - 1)
		return 0;
	if (event == NULL) {
		event = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (!event)
			return 0;
	}
	ResetEvent(&event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = event;
	tmpbuf[0] = 0;
	memcpy(tmpbuf + 1, buf, len);
	if (!WriteFile(h, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING)
			return 0;
		r = WaitForSingleObject(event, timeout);
		if (r == WAIT_TIMEOUT) {
			CancelIo(h);
			return 0;
		}
		if (r != WAIT_OBJECT_0)
			return 0;
	}
	if (!GetOverlappedResult(h, &ov, &n, FALSE))
		return 0;
	if (n <= 0)
		return 0;
	return 1;
}

void print_win32_err(void)
{
	char  buf[256];
	DWORD err;

	err = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, sizeof(buf), NULL);
	printf("err %ld: %s\n", err, buf);
}

static HANDLE win32_teensy_handle = NULL;

int teensy_open(void)
{
	teensy_close();
	win32_teensy_handle = open_usb_device(0x16C0, 0x0478);
	if (win32_teensy_handle)
		return 1;
	return 0;
}

int teensy_write(void* buf, int len, double timeout)
{
	int      r;
	uint32_t begin, now, total;

	if (!win32_teensy_handle)
		return 0;
	total = (uint32_t)(timeout * 1000.0);
	begin = timeGetTime();
	now   = begin;
	do {
		r = write_usb_device(win32_teensy_handle, buf, len, total - (now - begin));
		if (r > 0)
			return 1;
		Sleep(10);
		now = timeGetTime();
	} while (now - begin < total);
	return 0;
}

void teensy_close(void)
{
	if (!win32_teensy_handle)
		return;
	CloseHandle(win32_teensy_handle);
	win32_teensy_handle = NULL;
}

int hard_reboot(void)
{
	HANDLE rebootor;
	int    r;

	rebootor = open_usb_device(0x16C0, 0x0477);
	if (!rebootor)
		return 0;
	r = write_usb_device(rebootor, "reboot", 6, 100);
	CloseHandle(rebootor);
	return r;
}

int soft_reboot(void)
{
	printf("Soft reboot is not implemented for Win32\n");
	return 0;
}
