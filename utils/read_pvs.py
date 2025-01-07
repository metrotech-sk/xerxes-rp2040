#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Module Name: Read PVs
Description: This script performs reads and writes to the Xerxes device, 
                and prints the values in a tight loop. Use Ctrl+C to exit.
Author: theMladyPan
Version: 1.0
Date: 2024-07-10
"""

import os
import sys
import argparse
from serial import Serial
import time
import logging
import struct

from xerxes_protocol import (
    XerxesRoot,
    XerxesNetwork,
    Leaf,
    DebugSerial,
    memory,
)

# parse arguments
parser = argparse.ArgumentParser(
    description="Read process values from Xerxes Cutter device and print them in tight loop. Use Ctrl+C to exit."
)
parser.add_argument(
    "-a",
    "--address",
    metavar="ADDR",
    required=False,
    type=int,
    default=0,
    help="address of Xerxes Cutter device, default is 0",
)
parser.add_argument(
    "-b",
    "--baudrate",
    metavar="BAUD",
    required=False,
    type=int,
    default=115200,
    help="baudrate for serial communication, default is 115200",
)
parser.add_argument(
    "-i",
    "--interval",
    metavar="MS",
    required=False,
    type=int,
    default=100,
    help="interval in milliseconds between reads, default is 100ms",
)
parser.add_argument(
    "-p",
    "--port",
    metavar="PORT",
    required=False,
    type=str,
    default="/dev/ttyUSB0",
    help="port on which Xerxes Cutter device is listening, default is /dev/ttyUSB0",
)
# add argument whether to use debug serial or not
parser.add_argument(
    "-d",
    "--debug",
    action="store_true",
    help="use debug serial, default is False",
)
parser.add_argument(
    "-t",
    "--timeout",
    metavar="TIMEOUT",
    required=False,
    type=float,
    default=0.02,
    help="timeout in seconds for serial communication, default is 0.02s",
)
parser.add_argument(
    "--loglevel",
    metavar="LOGLEVEL",
    required=False,
    type=str,
    default="INFO",
    help="log level, default is INFO",
)
parser.add_argument(
    "--message",
    action="store_true",
    required=False,
    default=False,
    help="read message buffer from Xerxes device",
)


# whether to show history or not in output formating
parser.add_argument("--history", help="show history in output", action="store_true")

args = parser.parse_args()

if args.debug:
    level = logging.DEBUG
else:
    level = logging.getLevelName(args.loglevel)

logging.basicConfig(level=level, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
log = logging.getLogger(__name__)

# create XerxesNetwork object
if args.debug:
    port = DebugSerial(args.port)
else:
    port = Serial(args.port, timeout=args.timeout)
XN = XerxesNetwork(port)
XN.init(args.baudrate, args.timeout)

# create XerxesRoot object
XR = XerxesRoot(0xFE, XN)

# create Leaf object
leaf = Leaf(args.address, XR)
log.debug(f"Leaf parameters: {dir(leaf)}")

if __name__ == "__main__":
    exit_val = 0
    time_start = time.perf_counter()

    while True:
        try:
            tstart = time.perf_counter()
            payload = leaf.read_reg_net(memory.PV0_OFFSET, 4 * 4)
            vals = struct.unpack("4f", payload)
            pv0, pv1, pv2, pv3 = vals
            if args.message:
                # read message buffer from sensor
                payload = leaf.read_reg_net(memory.MESSAGE_OFFSET, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 128, 128)
                print(payload)

            dt = time.perf_counter() - time_start
            time_start = time.perf_counter()
            print(
                f"PV0: {pv0:.4f} PV1: {pv1:.4f} PV2: {pv2:.4f} PV3: {pv3:.4f}, dt: {dt:.4f}s" + " " * 10,
                end="\r",
            )
            if args.history:
                print()

            tcycle = time.perf_counter() - tstart
            to_sleep = (args.interval / 1000) - tcycle
            if to_sleep > 0:
                time.sleep(to_sleep)
        except KeyboardInterrupt:
            print("Exiting...")
            break
        except TimeoutError:
            log.warning(f"TimeoutError while reading from {leaf}")
        except Exception as e:
            log.error(f"Exception {e} while reading from {leaf}")

    port.close()
    sys.exit(exit_val)
