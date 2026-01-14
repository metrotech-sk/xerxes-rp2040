#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from xerxes_protocol import (
    XerxesNetwork,
    XerxesRoot,
    Leaf,
    MessageIncomplete,
    NetworkError,
    ChecksumError,
)
import logging
import argparse
import time

log = logging.getLogger(__name__)


def main():
    parser = argparse.ArgumentParser(description="Discover Xerxes devices on the network")
    parser.add_argument("-t", "--timeout", type=float, default=0.1, help="timeout in seconds")
    parser.add_argument("-d", "--debug", action="store_true", help="enable debug logging")
    parser.add_argument("-p", "--port", type=str, default="/dev/ttyUSB0", help="serial port")
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="baudrate")
    parser.add_argument("-a", "--address", type=int, default=0xFF, help="direct address")
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
        leaves = []
        log.info("Discovering devices...")
        for i in range(32):
            try:
                leaf = Leaf(i, XR)
                pr = leaf.ping()
                log.info(f"Found leaf {i} at address {i}")
                log.info(f"DevID: {pr.dev_id}, latency: {pr.latency}s")
                leaves.append(leaf)
            except (TimeoutError, MessageIncomplete, NetworkError, ChecksumError):
                log.debug(f"Leaf {i} not found")
            except KeyboardInterrupt:
                cont = False
                break
            except Exception as e:
                log.warning(f"Error pinging leaf {i}: {e}")

        # clear console:
        print("\033[H\033[J")

        for l in leaves:
            try:
                print(l.info)
                print(
                    f"Leaf {l.address}[{int(l.address)}], pv0-3: {l.pv0:2.3f}, {l.pv1:2.3f}, {l.pv2:2.3f}, {l.pv3:2.3f}"
                )
                print(
                    f"Leaf {l.address}[{int(l.address)}], offset_pv0-3: {l.offset_pv0:2.3f}, {l.offset_pv1:2.3f}, {l.offset_pv2:2.3f}, {l.offset_pv3:2.3f}"
                )
                print(
                    f"Leaf {l.address}[{int(l.address)}], gain_pv0-3: {l.gain_pv0:2.3f}, {l.gain_pv1:2.3f}, {l.gain_pv2:2.3f}, {l.gain_pv3:2.3f}"
                )
            except (TimeoutError, MessageIncomplete, NetworkError, ChecksumError) as e:
                log.warning(f"Communication error with leaf {l.address}: {e}")
            except KeyboardInterrupt:
                cont = False
                break
            except Exception as e:
                log.warning(f"Error reading values from leaf {l.address}: {e}")
        time.sleep(0.5)
        log.debug(f"Found {leaves}")

    serial.close()


if __name__ == "__main__":
    main()
