#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Module Name: calibrate.py
Description: This script performs zeroing of the Xerxes sensor 
Use Ctrl+C to exit.
Author: theMladyPan
Version: 1.0
Date: 2024-06-13
"""


import argparse
from serial import Serial
import logging
import time
import sys

from xerxes_protocol import XerxesRoot, XerxesNetwork, Leaf, DebugSerial

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
    "--delay",
    metavar="DELAY",
    required=False,
    type=float,
    default=10,
    help="delay in seconds before and during zeroing, default is 10s",
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
    "--pv3",
    action="store_true",
    help="read and print process value 3 too",
)

# whether to show history or not in output formating
parser.add_argument(
    "--history", help="show history in output", action="store_true"
)

args = parser.parse_args()

level = logging.DEBUG if args.debug else logging.getLevelName(args.loglevel)
logging.basicConfig(
    level=level, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
log = logging.getLogger(__name__)


def write_param_safe(leaf: Leaf, param: str, offset: float) -> None:
    """Blocking call"""

    log.debug(f"Writing leaf.{param} = {offset}")
    while True:
        try:
            setattr(leaf, param, offset)
            break

        except Exception as e:
            log.error(f"Failed to write leaf.{param}, exception {e}")


if __name__ == "__main__":
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

    [pv0s, pv1s, pv2s, pv3s] = [[], [], [], []]
    pvs = [pv0s, pv1s, pv2s, pv3s]

    exit_val = 0

    # clear offsets first
    write_param_safe(leaf, "offset_pv0", 0)
    write_param_safe(leaf, "offset_pv1", 0)
    write_param_safe(leaf, "offset_pv2", 0)
    write_param_safe(leaf, "offset_pv3", 0)
    write_param_safe(leaf, "gain_pv0", 1)
    write_param_safe(leaf, "gain_pv1", 1)
    write_param_safe(leaf, "gain_pv2", 1)
    write_param_safe(leaf, "gain_pv3", 1)

    log.info(f"Waiting for {args.delay} seconds to stabilize.")
    leaf.reset_soft()
    time.sleep(args.delay)
    log.info("Zeroing process values...")

    time_start = time.perf_counter()
    while time.perf_counter() - time_start < args.delay:
        pv0s.append(leaf.mean_pv0)
        pv1s.append(leaf.mean_pv1)
        pv2s.append(leaf.mean_pv2)
        pv3s.append(leaf.mean_pv3)
        print(
            f" {- time.perf_counter() + time_start + args.delay:.1f}s remaining",
            end="\r",
        )

    avgpv0 = sum(pv0s) / len(pv0s)
    avgpv1 = sum(pv1s) / len(pv1s)
    avgpv2 = sum(pv2s) / len(pv2s)
    avgpv3 = sum(pv3s) / len(pv3s)

    stdevpv0 = (sum((x - avgpv0) ** 2 for x in pv0s) / len(pv0s)) ** 0.5
    stdevpv1 = (sum((x - avgpv1) ** 2 for x in pv1s) / len(pv1s)) ** 0.5
    stdevpv2 = (sum((x - avgpv2) ** 2 for x in pv2s) / len(pv2s)) ** 0.5
    stdevpv3 = (sum((x - avgpv3) ** 2 for x in pv3s) / len(pv3s)) ** 0.5

    write_param_safe(leaf, "offset_pv0", avgpv0)
    write_param_safe(leaf, "offset_pv1", avgpv1)
    write_param_safe(leaf, "offset_pv2", avgpv2)
    if args.pv3:
        write_param_safe(leaf, "offset_pv3", avgpv3)

    leaf.reset_soft()
    time.sleep(0.2)
    log.info("Zeroing process values done.")
    log.info(
        f"Offsets written to device, offset_pv0: {leaf.offset_pv0:.4f}, offset_pv1: {leaf.offset_pv1:.4f}, offset_pv2: {leaf.offset_pv2:.4f}, offset_pv3: {leaf.offset_pv3:.4f}"
    )

    print(f"Offset PV0: {avgpv0:.4f} ± {stdevpv0:.4f}")
    print(f"Offset PV1: {avgpv1:.4f} ± {stdevpv1:.4f}")
    print(f"Offset PV2: {avgpv2:.4f} ± {stdevpv2:.4f}")
    print(f"Offset PV3: {avgpv3:.4f} ± {stdevpv3:.4f}")

    port.close()
