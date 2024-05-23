#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from struct import unpack
import numpy as np

# Serial settings
serial_port = "/dev/ttyUSB0"
baud_rate = 300000

# Initialize serial
ser = serial.Serial(serial_port, baud_rate, timeout=0.01)

# Setup plot
fig, ax = plt.subplots()
timestamps = []
input1 = []
input2 = []
input3 = []
input4 = []

(line1,) = ax.plot([], [], label="Input 1")
(line2,) = ax.plot([], [], label="Input 2")
(line3,) = ax.plot([], [], label="Input 3")
(line4,) = ax.plot([], [], label="Input 4")

ax.legend()
plt.xlabel("Timestamp")
plt.ylabel("Analog Inputs")
plt.title("Real-time Analog Input Data")

of = open("burst_data.csv", "w")


def update(frame):
    try:
        ser.reset_input_buffer()
        while ser.read() != b"\x01" or ser.read() != b"\x02":
            pass

        # Read 8 bytes: timestamp in us, uint64_t format
        timestamp = ser.read(8)
        # Read 4x4 floats
        inputs = ser.read(16)
        etxeot = ser.read(2)
        if etxeot != b"\x03\x04":
            print("Error: wrong ETX/EOT")
            return line1, line2, line3, line4

        timestamp = unpack("Q", timestamp)[0]
        timestamp = timestamp / 1e6  # Convert to seconds
        vals = unpack("4f", inputs)

        of.write(
            f"{timestamp:.4f},{vals[0]:.5f},{vals[1]:.5f},{vals[2]:.5f},{vals[3]:.5f}\n"
        )

        timestamps.append(timestamp)
        input1.append(vals[0])
        input2.append(vals[1])
        input3.append(vals[2])
        input4.append(vals[3])

        if len(timestamps) > 100:
            timestamps.pop(0)
            input1.pop(0)
            input2.pop(0)
            input3.pop(0)
            input4.pop(0)

        # Update plot limits
        ax.set_xlim(timestamps[0], timestamps[-1])
        # ax.set_ylim(
        #    min(min(input1), min(input2), min(input3), min(input4)) - 0.01,
        #    max(max(input1), max(input2), max(input3), max(input4)) + 0.01,
        # )

        ax.set_ylim(0, 1)

        line1.set_data(timestamps, input1)
        line2.set_data(timestamps, input2)
        line3.set_data(timestamps, input3)
        line4.set_data(timestamps, input4)

    except KeyboardInterrupt:
        ser.close()
        of.close()
        plt.close()

    return line1, line2, line3, line4


ani = animation.FuncAnimation(fig, update, blit=True, interval=1)
plt.show()

ser.close()
of.close()
