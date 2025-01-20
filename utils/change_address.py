#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from xerxes_protocol import XerxesNetwork, XerxesRoot, Leaf, memory, XerxesPingReply
import logging
import argparse
import time
import struct

log = logging.getLogger(__name__)

parser = argparse.ArgumentParser(
    description="Change address of the xerxes leaf",
)
parser.add_argument(
    "-t",
    "--timeout",
    type=float,
    default=0.05,
    help="timeout in seconds",
)
parser.add_argument(
    "-d",
    "--debug",
    action="store_true",
    help="enable debug logging",
)
parser.add_argument(
    "-p",
    "--port",
    type=str,
    default="/dev/ttyUSB0",
    help="serial port, default: /dev/ttyUSB0",
)
parser.add_argument(
    "-b",
    "--baudrate",
    type=int,
    default=115200,
    help="baudrate, default: 115200",
)
parser.add_argument(
    "-o",
    "--old_address",
    type=int,
    help="current address, default: 0",
    default=0,
)
parser.add_argument(
    "new_address",
    type=int,
    help="new address",
)
args = parser.parse_args()


def main():

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
        from xerxes_protocol import DebugSerial as Serial
    else:
        logging.basicConfig(level=logging.INFO)
        from serial import Serial

    serial = Serial(args.port)
    XN = XerxesNetwork(serial).init(args.baudrate, args.timeout)

    XR = XerxesRoot(0xFE, XN)

    try:
        leaf = Leaf(args.old_address, XR)

        ping: XerxesPingReply = leaf.ping()
        log.info(f"Leaf id: {ping.dev_id}, version: {ping.v_maj}.{ping.v_min}, latency: {ping.latency * 1000:.1f}ms")

        # read message buffer from sensor
        payload = leaf.read_reg_net(memory.MESSAGE_OFFSET, 128)
        payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 128, 128)
        log.info("Device info:")
        log.info(payload.decode("utf-8"))

        leaf.address = args.new_address
        log.info(f"Address changed from {args.old_address} to {args.new_address}")
        log.info(f"resetting the leaf")
        leaf.reset_soft()

        time.sleep(0.5)
        leaf = Leaf(args.new_address, XR)
        log.info(f"Reading the new address: {leaf.device_address}")

        payload = leaf.read_reg_net(memory.PV0_OFFSET, 4 * 4)
        vals = struct.unpack("4f", payload)
        pv0, pv1, pv2, pv3 = vals
        log.info(f"PV0: {pv0:.3f}, PV1: {pv1:.3f}, PV2: {pv2:.3f}, PV3: {pv3:.3f}")

    except TimeoutError as e:
        log.error("TimeoutError, try again")
        log.error(e)

    finally:
        serial.close()


if __name__ == "__main__":
    main()
