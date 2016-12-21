#!/usr/bin/env python
import serial
import sys
from serial.tools import list_ports
import logging
import os
from time import gmtime, strftime, localtime,sleep
from optparse import OptionParser
import pyprind

parser = OptionParser(usage="python %prog [options]")
parser.add_option("-f", dest="bin_path", help="path of bin to be upload")
parser.add_option("-c", dest="com_port", help="COM port, can be COM1, COM2, ..., COMx")
(opt, args) = parser.parse_args()
s = serial.Serial(opt.com_port, 115200)
gba_file = open(opt.bin_path, 'rb')

statinfo_bin = os.stat(opt.bin_path)

print statinfo_bin.st_size

print ((statinfo_bin.st_size - 96*2)/4)

sleep(3)

s.write('C')
s.write(str(int(statinfo_bin.st_size)))

gba_file_length = (statinfo_bin.st_size+0xf)&(~0xf);

while 1:
  datain = s.readline()
  print datain
  if datain =="S\r\n":
    #Timming is critical, data needs to send right after headder
    bar = pyprind.ProgBar(96)
    for x in xrange(0,96):
      #print "package:" + str(x)
      data1 = gba_file.read(1)
      data2 = gba_file.read(1)
      data = data1+data2
      #print "Data:" + data.encode("HEX")
      s.write(data)
      bar.update()
      #s.readline()
      
      s.readline()

      s.readline()
    break
while 1:
  datain = s.readline()
  print datain
  if datain =="S2\r\n":
    bar2 = pyprind.ProgBar((gba_file_length - 96*2)/4)
    for x in xrange(0xc0,gba_file_length,4):
      #print x.encode("HEX")

      data = gba_file.read(4)
      if data == '':
        data = '00000000'.decode("HEX")
        pass
      #print data.encode("HEX")
#      data2 = gba_file.read(1)
##      data3 = gba_file.read(1)
#      data4 = gba_file.read(1)
#      data = data4+data3+data2+data1
      #print "Data:" + data.encode("HEX")
      s.write(data)
      bar2.update()
      din = s.readline()
      din = s.readline()
      din = s.readline()
    break
  pass
while 1:
  datain = s.readline()
  print datain
  if datain.startswith("CRC"):
    exit()
    pass
  