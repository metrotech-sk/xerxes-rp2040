#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from xerxes_protocol import XerxesNetwork, XerxesRoot, Leaf
import logging
import argparse
import time

log = logging.getLogger(__name__)


def main():
    parser = argparse.ArgumentParser(
        description="Discover Xerxes devices on the network"
    )
    parser.add_argument(
        "-t", "--timeout", type=float, default=0.021, help="timeout in seconds"
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="enable debug logging"
    )
    parser.add_argument(
        "-p", "--port", type=str, default="/dev/ttyUSB0", help="serial port"
    )
    parser.add_argument(
        "-b", "--baudrate", type=int, default=115200, help="baudrate"
    )
    parser.add_argument(
        "-a", "--address", type=int, default=0xFF, help="direct address"
    )
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)
        from xerxes_protocol import DebugSerial as Serial
    else:
        logging.basicConfig(level=logging.INFO)
        from serial import Serial

    serial = Serial(args.port)
    XN = XerxesNetwork(serial).init(args.baudrate, args.timeout)

    XR = XerxesRoot(0xFE, XN)

    cont = True
    while cont:
        log.info("Discovering devices...")
        for i in range(32):
            try:
                leaf = Leaf(i, XR)
                pr = leaf.ping()
                log.info(f"Found leaf {i} at address {i}")
                log.info(f"DevID: {pr.dev_id}, latency: {pr.latency}s")
            except TimeoutError:
                log.debug(f"Leaf {i} not found")
            except ValueError:
                log.warning(f"Error reading values from leaf {i}")
            except KeyboardInterrupt:
                cont = False
                break
        time.sleep(0.5)

    serial.close()


if __name__ == "__main__":
    main()
