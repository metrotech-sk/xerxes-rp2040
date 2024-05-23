#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# echo 1 | sudo tee /sys/bus/usb-serial/devices/ttyUSB0/latency_timer

import serial
from struct import unpack

# Serial settings
serial_port = "/dev/ttyUSB0"
baud_rate = 300000

# Initialize serial
ser = serial.Serial(serial_port, baud_rate, timeout=0.01)
of = open("burst_data.csv", "w")

last_t = 0
while True:
    try:
        while ser.read() != b"\x01" or ser.read() != b"\x02":
            pass

        payload = ser.read(24)
        etxeot = ser.read(2)
        if etxeot == b"\x03\x04":
            timestamp = payload[:8]
            inputs = payload[8:]
            timestamp = unpack("Q", timestamp)[0]
            vals = unpack("4f", inputs)

            print(
                f"{timestamp/1e6:.6f},{vals[0]:.5f},{vals[1]:.5f},{vals[2]:.5f},{vals[3]:.5f},{timestamp-last_t}"
            )
            last_t = timestamp
        else:
            print("Error: wrong ETX/EOT")
    except KeyboardInterrupt:
        ser.close()
        of.close()
        break
