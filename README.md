# kicad-workshop
Material for a Kicad workshop

![3D render](slides/kicadworkshop.png)

![Photo](slides/PCBphoto.jpg)

# Overview
A three session workshop on designing and assembling PCBs with Kicad. A small group of students will design and build a USB widget. The instructor will provide the basic circuit and guidance. Materials cost will be approximately $20 including PCB fabrication, components, shipping and solder paste stencil. We will place a shared order to reduce shipping costs.

## The device
 - [Silicon Labs EFM32HG309 microcontroller](https://www.silabs.com/products/mcu/32-bit/efm32-happy-gecko), ARM Cortex M0+, 64 kiB flash, 8 kiB ram USB device interface. About $1.50.
 - [128x32 pixel OLED](https://www.aliexpress.com/item/0-91-inch-128x32-I2C-IIC-Serial-Blue-OLED-LCD-Display-Module-0-91-12832-SSD1306/32788923016.html?ws_ab_test=searchweb0_0,searchweb201602_6_10065_10130_10068_10890_10547_319_10546_317_10548_10545_10696_453_10084_454_10083_10618_10307_537_536_10059_10884_10887_321_322_10103-10890,searchweb201603_51,ppcSwitch_0&algo_expid=ce015d67-aa3b-42e3-a658-b8d4b17154e0-2&algo_pvid=ce015d67-aa3b-42e3-a658-b8d4b17154e0) Need to get these ordered early due to China shipping.
 - [tiny RGB LED](https://www.mouser.com/datasheet/2/90/ds-UHD1110-FKA-1149141.pdf)
 - rotary encoder with push switch
 - USB for power and communication, will provide Python code to communicate with it from a PC
 - This is about $5 in parts yet demonstrates quite a few things

## Prerequisites
 - Laptop with [Kicad 5.x](http://www.kicad-pcb.org/download/) (preferably the latest release, 5.1.4) installed. 5.1.3 was not released due to a [major bug](https://bugs.launchpad.net/kicad/+bug/1838446), but made it into some distros. Avoid that. I have one spare laptop with Kicad available.
 - [Interactive HTML BOM plugin installed](https://github.com/openscopeproject/InteractiveHtmlBom) (optional)
 - Electronics knowledge is not required, but helpful.
 - Programming knowledge is not required, but helpful.
 - USB C cable (C-C for newer laptops, A-C for older) by last session. I'll have a few on hand.
