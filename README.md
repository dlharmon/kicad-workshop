# kicad-workshop
Material for a Kicad workshop

![Photo](slides/PCBphoto.jpg)

## Overview

A three session workshop on designing and assembling PCBs with
Kicad. A small group of students will design and build a USB
widget. The instructor will provide the basic circuit and
guidance. Materials cost will be approximately $20 including PCB
fabrication, components, shipping and solder paste stencil. We will
place a shared order to reduce shipping costs.

## [Slides](https://raw.githubusercontent.com/dlharmon/kicad-workshop/master/slides/kicad-workshop.pdf)

## The device

- [Silicon Labs EFM32HG309 microcontroller](https://www.silabs.com/products/mcu/32-bit/efm32-happy-gecko), ARM Cortex M0+, 64 kiB flash, 8 kiB ram USB device interface. About $1.50.
 - Connector for [128x64 pixel OLED](https://www.aliexpress.com/item/32847040077.html?spm=a2g0s.9042311.0.0.6e644c4dlFzZIQ) Display is not included in the parts kit, order one yourself if you want it.
 - LEDs
 - rotary encoder with push switch
 - USB for power and communication, will provide Python code to communicate with it from a PC
 - This is about $3 in parts yet demonstrates quite a few things

## Prerequisites
 - Laptop with [Kicad 5.1.4](http://www.kicad-pcb.org/download/) 5.1.2 is probably OK. Last time we had a few students using 5.0.x and it was problematic.
 - [Interactive HTML BOM plugin installed](https://github.com/openscopeproject/InteractiveHtmlBom) (optional, may require installing python3-wxgtk4)
 - Electronics knowledge is not required, but helpful.
 - Programming knowledge is not required, but helpful.
 - USB C cable (C-C for newer laptops, A-C for older) by last session. I'll have a few on hand.
 - Clone or download [https://github.com/sethhillbrand/kicad_templates.git](https://github.com/sethhillbrand/kicad_templates.git)

## Firmware
 - This is entirely optional. The instructor can flash firmware for you if you are not interested.
 - [ARM Cortex compiler](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) Alternately, your Linux distro may have a package `gcc-arm-none-eabi` or similar which may work.
 - [OpenOCD](http://openocd.org/) required to flash the firmware the first time and until a bootloader is developed.
 - In the fw directory, `make flash` will build and flash the firmware. It assumes an FT232H based programmer with the default USB ID.
 - usb_comm.py can be used to interact with the device from Python over USB.

### udev rules

On Linux, to avoid having to run as root (change dlharmon to your user), add the following to `/etc/udev/rules.d/99-local.rules`:

```
ATTR{idVendor}=="0403", ATTR{idProduct}=="6014", SYMLINK+="ftdi-%k", MODE="660", GROUP="dlharmon"
SUBSYSTEM=="usb", ATTR{idVendor}=="10c4", ATTR{idProduct}=="8b00", MODE="0660", GROUP="dlharmon"
```