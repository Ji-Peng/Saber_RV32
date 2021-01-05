#!/usr/bin/env python3
import serial.tools.list_ports
import sys

# macos_dev = "/dev/tty.usbserial"
# linux_dev = "/dev/ttyUSB0"


# ports = serial.tools.list_ports.comports()
# for port in ports:
#     print(port.device, port.description)
    # if 'VCP' in port.description or 'Serial' in port.description:
    #     dev = serial.Serial(port.device, 115200, timeout=10)
    #     print("Listen on port {} from {}".format(port.device, port.description))
    #     break

dev=serial.Serial("/dev/ttyACM0",115200, timeout=0.5)

while True:
    x = dev.read()
    sys.stdout.buffer.write(x)
    sys.stdout.flush()