#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
import argparse
from serial import Serial
import struct

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test high speed")
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="verbose mode"
    )
    parser.add_argument(
        "-p", "--port", type=str, default="/dev/ttyUSB0", help="port"
    )
    parser.add_argument(
        "-b", "--baudrate", type=int, default=115200, help="baudrate"
    )
    parser.add_argument(
        "-n", "--num", type=int, default=4, help="number of devices"
    )
    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)

    logging.info("Start high speed test")

    com = Serial(args.port, args.baudrate, timeout=0.1)

    while True:
        # wait for sync packet
        # sync packet is <SOH> <STX> <timestamp:8> <ETX> <EOT>
        while com.read(1) != b"\x01" or com.read(1) != b"\x02":
            pass
        timestamp = com.read(8)
        timestamp = struct.unpack("<Q", timestamp)[0]
        etxeot = com.read(2)
        if etxeot != b"\x03\x04":
            logging.debug("Not a sync packet")
            continue

        data = []
        for _ in range(args.num):
            sohstx = com.read(2)
            if sohstx != b"\x01\x02":
                logging.warning("Not a valid header")
                break
            payload = com.read(16)
            eotetx = com.read(2)
            if eotetx != b"\x03\x04":
                logging.warning(f"Not a valid footer, got {eotetx}")
                break
            # unpack 4 floats:
            data = struct.unpack("<ffff", payload)
            print(timestamp, data)

    logging.debug("End high speed test")
