# Teensy Loader - Command Line Version#

The Teensy Loader is available in a command line version for advanced users who want to automate programming, typically using a Makefile. For most uses, the graphical version in Automatic Mode is much easier. 

http://www.pjrc.com/teensy/loader_cli.html

## Compiling From Source

The command line version is provided as source code for most platforms. To compile, you must have gcc or mingw installed. Edit the Makefile to select your operating system, then just type "make". If you have a BSD compatible make, replace "Makefile" with "Makefile.bsd".
Version 2.0 has been tested on Ubuntu 9.04, Mac OS-X 10.5, Windows XP, FreeBSD 8.0, OpenBSD (20-Jan-2010 snapshot), and NetBSD 5.0.1. All versions of NT-based Windows with USB support (2000 and later) are believed to work.

On Ubuntu, you may need to install "libusb-dev" to compile.

  `sudo apt-get install libusb-dev`

Other Linux systems may [require other package installation](https://forum.pjrc.com/threads/40965-Linux-64bit-Arduino-1-6-13-Issues-starting-Teensy-Loader-and-libusb-0-1-so-4-error?p=127873&viewfull=1#post127873) to compile.

## Usage and Command Line Options

A typical usage from the command line may look like this:

`teensy_loader_cli --mcu=mk20dx256 -w blink_slow_Teensy32.hex`

Required command line parameters:

```
--mcu=<MCU> : Specify Processor. You must specify the target processor. This syntax is the same as used by gcc, which makes integrating with your Makefile easier. Valid options are:
--mcu=mk66fx1m0 : 	Teensy 3.6
--mcu=mk64fx512 : 	Teensy 3.5
--mcu=mk20dx256 : 	Teensy 3.2 & 3.1
--mcu=mk20dx128 : 	Teensy 3.0
--mcu=mkl26z64 : 	Teensy LC
--mcu=at90usb1286 : 	Teensy++ 2.0
--mcu=atmega32u4 : 	Teensy 2.0
--mcu=at90usb646 : 	Teensy++ 1.0
--mcu=at90usb162 : 	Teensy 1.0
```

Caution: HEX files compiled with USB support must be compiled for the correct chip. If you load a file built for a different chip, often it will hang while trying to initialize the on-chip USB controller (each chip has a different PLL-based clock generator). On some PCs, this can "confuse" your USB port and a cold reboot may be required to restore USB functionality. When a Teensy has been programmed with such incorrect code, the reset button must be held down BEFORE the USB cable is connected, and then released only after the USB cable is fully connected.

Optional command line parameters:

`-w` : Wait for device to appear. When the pushbuttons has not been pressed and HalfKay may not be running yet, this option makes teensy_loader_cli wait. It is safe to use this when HalfKay is already running. The hex file is read before waiting to verify it exists, and again immediately after the device is detected.

`-r` : Use hard reboot if device not online. Perform a hard reset using a second Teensy 2.0 running this [rebooter](rebootor) code, with pin C7 connected to the reset pin on your main Teensy. While this requires using a second board, it allows a Makefile to fully automate reprogramming your Teensy. This method is recommended for fully automated usage, such as Travis CI with PlatformIO. No manual button press is required!

`-s` : Use soft reboot (Linux only) if device not online. Perform a soft reset request by searching for any Teensy running USB Serial code built by Teensyduino. A request to reboot is transmitted to the first device found.

`-n` : No reboot after programming. After programming the hex file, do not reboot. HalfKay remains running. This option may be useful if you wish to program the code but do not intend for it to run until the Teensy is installed inside a system with its I/O pins connected.

`-v` : Verbose output. Normally teensy_loader_cli prints only error messages if any operation fails. This enables verbose output, which can help with troubleshooting, or simply show you more status information.

## System Specific Setup

Linux requires UDEV rules for non-root users.

http://www.pjrc.com/teensy/49-teensy.rules

FreeBSD requires a [device configuration file](freebsd-teensy.conf) for non-root users.

OpenBSD's make is incompatible with most AVR makefiles. Use "`pkg_add -r gmake`", and then compile code with "`gmake all`" to obtain the .hex file.

On Macintosh OS-X 10.8, Casey Rodarmor shared this tip:

I recently had a little trouble getting the teensy cli loader working on Mac OSX 10.8. Apple moved the location of the SDKs around, so that they now reside inside of the xcode app itself. This is the line in the makefile that got it working for me:
SDK ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk

## Makefile Integration

You can use teensy_loader_cli from your Makefile, to autoamtically program your freshly compiled code. Here is an example:

```
# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
        @echo
        @echo $(MSG_FLASH) $@
        $(OBJCOPY) -O $(FORMAT) -R .eeprom -R .fuse -R .lock -R .signature $< $@
        teensy_loader_cli --mcu=$(MCU) -w -v $@
```

Make requires the white space before any command to be a tab character (not 8 spaces), so please make sure you use tab.

If you connect a second Teensy using the rebootor code, add the "-r" option and your code will always be programmed automaticallly without having to manually press the reset button!

Scott Bronson contributed a [Makefile patch](http://www.pjrc.com/teensy/loader_cli.makefile.patch) to allow "make program" to work for the blinky example.

## PlatformIO Integration

[Platformio](http://platformio.org) includes support for loading via teensy_loader.

## Errata

Compiling on Mac OS-X 10.6 may require adding "-mmacosx-version-min=10.5" to the Makefile. Thanks to Morgan Sutherland for reporting this.

`$(CC) $(CFLAGS) -DUSE_APPLE_IOKIT -isysroot $(SDK) -o teensy_loader_cli teensy_loader_cli.c -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation -mmacosx-version-min=10.5`
