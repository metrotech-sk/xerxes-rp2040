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

import sys
import time
import struct

from read_pvs import XR, args, log, port, Leaf
from xerxes_protocol import memory
import signal


class Accelerometer(Leaf):
    @property
    def fft(self):

        # read message buffer from sensor
        payload = self.read_reg_net(memory.MESSAGE_OFFSET, 128)
        payload += self.read_reg_net(memory.MESSAGE_OFFSET + 128, 128)
        # unpack 64 floats into list:
        vals = struct.unpack("64f", payload)
        data = {}
        data["spectrum"] = []
        for i in range(0, 60, 2):
            f = {
                "f": vals[i],
                "a": vals[i + 1],
            }
            data["spectrum"].append(f)
        data["main"] = {
            "f": vals[60],
            "a": vals[61],
        }
        return data


# create Leaf object
leaf = Accelerometer(args.address, XR)


def on_ctrl_z(_signum, _frame):
    leaf.reset_soft()


signal.signal(signal.SIGTSTP, on_ctrl_z)

if __name__ == "__main__":
    exit_val = 0
    time_start = time.perf_counter()
    moving_avg_ampl = 0

    while True:
        try:
            pv0, pv1, pv2, pv3 = (
                leaf.pv0,
                leaf.pv1,
                leaf.pv2,
                leaf.pv3,
            )
            if args.message:
                # read message buffer from sensor
                payload = leaf.read_reg_net(memory.MESSAGE_OFFSET, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 128, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 256, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 384, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 512, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 640, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 768, 128)
                payload += leaf.read_reg_net(memory.MESSAGE_OFFSET + 896, 128)

                data = struct.unpack("256f", payload)

                # clear screen:
                print("\033[H\033[J")
                # data are in pairs, so print them in pairs
                for i in range(0, len(data), 2):
                    print(f"{data[i]:.4f} {data[i+1]:.4f}")

            dt = time.perf_counter() - time_start
            time_start = time.perf_counter()
            amplitude = abs((pv0**2 + pv1**2 + pv2**2) ** 0.5 - 1)

            print(
                f"PV0: {pv0:.4f}, PV1: {pv1:.4f}, PV2: {pv2:.4f}, PV3: {pv3:.4f}, Sum: {amplitude:.4f}, dt: {dt:.4f}s"
                + " " * 10
            )
            if args.history:
                print()
            time.sleep(args.interval / 1000)
        except KeyboardInterrupt:
            print("Exiting...")
            break
        except TimeoutError:
            log.warning(f"TimeoutError while reading from {leaf}")
        except Exception as e:
            log.error(f"Exception {e} while reading from {leaf}")

    port.close()
    sys.exit(exit_val)
