# Teensy Loader - Command Line Version
The Teensy Loader is available in a command line version for advanced users who want to automate programming, typically using a Makefile. For most uses, the graphical version in Automatic Mode is much easier. 

http://www.pjrc.com/teensy/loader_cli.html

## System-specific Prerequisites
- **Debian / Ubuntu**
  - libUSB Package
    ```
    apt install libusb-0.1.4
    ```
  - For non-root users, install the [udev rules file](https://www.pjrc.com/teensy/00-teensy.rules).
- **Arch Linux (and Arch-likes)**
  - libUSB Package
    ```
    pacman -S libusb libusb-compat
    ```
- **FreeBSD**  
  - For non-root users, install the [device configuration file](extra/freebsd-teensy.conf).


## Usage
A typical usage from the command line may look like this:

`teensy_loader_cli --mcu=mk20dx256 -w blink_slow_Teensy32.hex`

Required command line parameters:

```
--mcu=<MCU> : Specify Processor. You must specify the target processor. This syntax is the same as used by gcc, which makes integrating with your Makefile easier, we also now support passing in a logical name. Valid options are:
--mcu=TEENSY2           Teensy 2.0
--mcu=TEENSY2PP         Teensy++ 2.0
--mcu=TEENSYLC          Teensy LC
--mcu=TEENSY30          Teensy 3.0
--mcu=TEENSY31          Teensy 3.1
--mcu=TEENSY32          Teensy 3.2
--mcu=TEENSY35          Teensy 3.5
--mcu=TEENSY36          Teensy 3.6
--mcu=TEENSY40          Teensy 4.0
--mcu=TEENSY41          Teensy 4.1
--mcu=TEENSY_MICROMOD   MicroMod Teensy
--mcu=imxrt1062         Teensy 4.0
--mcu=mk66fx1m0         Teensy 3.6
--mcu=mk64fx512         Teensy 3.5
--mcu=mk20dx256         Teensy 3.2 & 3.1
--mcu=mk20dx128         Teensy 3.0
--mcu=mkl26z64          Teensy LC
--mcu=at90usb1286       Teensy++ 2.0
--mcu=atmega32u4        Teensy 2.0
--mcu=at90usb646        Teensy++ 1.0
--mcu=at90usb162        Teensy 1.0
```

Caution: HEX files compiled with USB support must be compiled for the correct chip. If you load a file built for a different chip, often it will hang while trying to initialize the on-chip USB controller (each chip has a different PLL-based clock generator). On some PCs, this can "confuse" your USB port and a cold reboot may be required to restore USB functionality. When a Teensy has been programmed with such incorrect code, the reset button must be held down BEFORE the USB cable is connected, and then released only after the USB cable is fully connected.

### Optional command line parameters:

`-w` : Wait for device to appear. When the pushbuttons has not been pressed and HalfKay may not be running yet, this option makes teensy_loader_cli wait. It is safe to use this when HalfKay is already running. The hex file is read before waiting to verify it exists, and again immediately after the device is detected.

`-r` : Use hard reboot if device not online. Perform a hard reset using a second Teensy 2.0 running this [rebooter](rebootor) code, with pin C7 connected to the reset pin on your main Teensy. While this requires using a second board, it allows a Makefile to fully automate reprogramming your Teensy. This method is recommended for fully automated usage, such as Travis CI with PlatformIO. No manual button press is required!

`-s` : Use soft reboot (Linux only) if device not online. Perform a soft reset request by searching for any Teensy running USB Serial code built by Teensyduino. A request to reboot is transmitted to the first device found.

`-n` : No reboot after programming. After programming the hex file, do not reboot. HalfKay remains running. This option may be useful if you wish to program the code but do not intend for it to run until the Teensy is installed inside a system with its I/O pins connected.

`-v` : Verbose output. Normally teensy_loader_cli prints only error messages if any operation fails. This enables verbose output, which can help with troubleshooting, or simply show you more status information.

## Building from Source

### Prerequisites
- **Windows**  
  - A compatible compiler:  
    - [Visual Studio](https://visualstudio.microsoft.com/vs/) for C & C++ Development
    - LLVM and MinGW work, but require additional configuration.
  - [CMake](https://cmake.org/) 3.30.0 or newer
  - [Windows Software Development Kit](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)
  - [Windows Driver Development Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk#download-icon-for-wdk-step-3-install-wdk) (must have the same version as the Windows SDK!)
- **Debian / Ubuntu**  
  - Essential Build Tools  
    ```
    apt install build-essential make
    ```
  - At least one of:  
      - GCC 12 (or newer)  
        ```
        apt install gcc-12 g++-12
        ```
      - [LLVM](https://releases.llvm.org/) Clang 14 (or newer)  
        ```
        bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
        ```
  - At least one of:  
      - ld or gold  
        ````
        apt install binutils
        ```
      - [LLVM](https://releases.llvm.org/) lld  
        ```
        bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
        ```
      - [mold](https://github.com/rui314/mold)  
        ```
        apt install mold
        ```
  - libUSB Development Packages  
    ```
    apt install libusb-dev
    ```
  - [CMake](https://cmake.org/) 3.30.0 or newer  
    Please install the latest version from the link above.
- **Arch Linux (and Arch-likes)**
  - Build Tools  
    ```
    pacman -S base-devel gcc make
    ```
  - libUSB Development Packages  
    ```
    pacman -S libusb libusb-compat
    ```
  - [CMake](https://cmake.org/) 3.30.0 or newer  
    ```
    pacman -S cmake
    ```
- **FreeBSD & BSD-like**
  - [CMake](https://cmake.org/) 3.30.0 or newer  
    Please install the latest version from the link above.

### Configuring & Compiling
1. Clone the repository, or download a snapshot of it.
2. Run this command in the directory containing the CMakeLists.txt file:  
    ```
    cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX=build/install -DCMAKE_BUILD_TYPE=Release
    ```
3. Run this command in the directory containing the CMakeLists.txt file:  
    ```
    cmake --build build --target install --config Release
    ```
4. Done, you should now have a binary file at `build/install`.

## Special Mentions
- Scott Bronson contributed a [Makefile patch](http://www.pjrc.com/teensy/loader_cli.makefile.patch) to allow "make program" to work for the blinky example.
- [PlatformIO](http://platformio.org) includes support for loading via teensy_loader.
