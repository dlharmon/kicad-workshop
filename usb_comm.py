#!/usr/bin/env python3

import usb.core
import usb.util

import sys
import time
from struct import pack, unpack
from datetime import datetime



class USBSpiDev:
    def __init__(self, product):
        self.dev = usb.core.find(idVendor=0x10c4, idProduct=0x8b00)
        dev = self.dev
        if self.dev is None:
            raise ValueError('Device not found')
        self.dev.set_configuration()
        self.cfg = self.dev.get_active_configuration()
        self.intf = self.cfg[(0,0)]
        print(self.dev)
        print("Manufacturer =", usb.util.get_string(dev, dev.iManufacturer))
        print("Product = ", usb.util.get_string(dev, dev.iProduct))
        print("Serial = ", usb.util.get_string(dev, dev.iSerialNumber))

    def out_2(self, a, b):
        self.dev.write(1, pack("II", a, b))
    def out_3(self, a, b, c):
        self.dev.write(1, pack("III", a, b, c))
    def bulk_in(self, n):
        return bytes(self.dev.read(0x82, n,100))
    def bulk_in_32(self):
        (rv,) = unpack("i", self.bulk_in(4))
        return rv
    def r(self,a):
        self.out_2(5, (1<<23) | (a<<16))
        return self.bulk_in_32()
    def w(self, a, d):
        self.out_2(4, (a<<16) | (d&0xFFFF))
    def w32(self, a, d):
        self.w(a+1, d&0xFFFF)
        self.w(a, d>>16)
    def devinfo(self):
        self.out_2(0, 0)
        rv = unpack("IIII", self.bulk_in(16))
        print("uc manufactured",
              datetime.utcfromtimestamp(rv[0]).strftime('%Y-%m-%d %H:%M'))
        for i in range(4):
            print('info {} = 0x{:8X}'.format(i, rv[i]))
        return rv

    def adc(self, source, ref):
        self.out_3(3, source, ref)
        return self.bulk_in_32()
    def get_temp(self):
        return self.adc(8,0)/10.0
    def get_vcc(self):
        return 1.25*3.0*self.adc(9,0)/4096.0

if __name__ == "__main__":

    dev = USBSpiDev("Kicad workshop")
    dev.devinfo()
    print("temp", dev.get_temp(), "C")
    print("vcc", dev.get_vcc(), "V")
