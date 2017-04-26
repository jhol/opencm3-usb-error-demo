#!/bin/bash

set -e

tty=/dev/ttyACM1

while : ; do

#
# Reset the device with OpenOCD
#

telnet localhost 4444 <<EOF >/dev/null 2>&1 || true
reset
EOF

#
# Wait for the serial device node to disappear and reappear
#

while [ -c $tty ]; do : ; done
while [ ! -c $tty ]; do : ; done

#
# Use pyusb to send a vendor control message
#

sudo python3 <<EOF || exit 1
import sys
import usb.core

dev = usb.core.find(idVendor=0x0925, idProduct=0xD100)
if dev is None:
  print('FAIL - Device not found')
  sys.exit(1)

try:
  dev.ctrl_transfer(0x40, 1, 0x01, 0, b'\x01\x02\x02\x04')
except:
  print('FAIL - control failed')
  sys.exit(1)

print('PASS')
EOF

done
