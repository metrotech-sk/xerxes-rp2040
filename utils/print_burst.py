#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
from struct import unpack

# Serial settings
serial_port = "/dev/ttyUSB0"
baud_rate = 200000

# Initialize serial
ser = serial.Serial(serial_port, baud_rate, timeout=0.01)
of = open("burst_data.csv", "w")
while True:
    try:
        while ser.read() != b"\x01" or ser.read() != b"\x02":
            pass

        timestamp = ser.read(8)
        # Read 4x4 floats
        inputs = ser.read(16)
        etxeot = ser.read(2)
        if etxeot == b"\x03\x04":
            timestamp = unpack("Q", timestamp)[0]
            vals = unpack("4f", inputs)

            print(
                f"time: {timestamp/1e6:.3f}s, ai0: {vals[0]:.5f}, ai1: {vals[1]:.5f}, ai2: {vals[2]:.5f}, ai3: {vals[3]:.5f}"
            )
            # save to file
            of.write(
                f"{timestamp/1e6:.3f},{vals[0]:.5f},{vals[1]:.5f},{vals[2]:.5f},{vals[3]:.5f}\n"
            )
    except KeyboardInterrupt:
        ser.close()
        of.close()
        break
