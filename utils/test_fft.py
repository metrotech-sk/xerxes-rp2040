#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import struct
import signal

from read_pvs import XR, args, log, port, Leaf
from xerxes_protocol import memory

class Accelerometer(Leaf):
    @property
    def fft(self):
        # Read message buffer (2 chunks of 128 bytes)
        payload = self.read_reg_net(memory.MESSAGE_OFFSET, 128)
        payload += self.read_reg_net(memory.MESSAGE_OFFSET + 128, 128)
        
        # Unpack 64 floats
        vals = struct.unpack("64f", payload)
        
        data = {"spectrum": [], "main": {}}
        
        # Bins 0-29 are freq/ampl pairs
        for i in range(0, 60, 2):
            data["spectrum"].append({"f": vals[i], "a": vals[i + 1]})
            
        # Bins 30-31 are the main peak
        data["main"] = {"f": vals[60], "a": vals[61]}
        return data

# Shared object for the wrapper
leaf = Accelerometer(args.address, XR)

def on_ctrl_z(_signum, _frame):
    print("\nSoft resetting sensor...")
    leaf.reset_soft()

signal.signal(signal.SIGTSTP, on_ctrl_z)

def run_fft_reader():
    print(f"Reading FFT from Address: {leaf.address}")
    while True:
        try:
            data = leaf.fft
            print("\033[H\033[J") # Clear screen
            print(f"--- FFT SPECTRUM (Addr: {leaf.address}) ---")
            for entry in data["spectrum"]:
                if entry['a'] > 0.0001: # Filter noise
                    print(f"{entry['f']:10.2f} Hz | {entry['a']:10.4f}")
            
            print("-" * 30)
            print(f"MAIN PEAK: {data['main']['f']:.2f} Hz ({data['main']['a']:.4f})")
            time.sleep(args.interval / 1000)
        except KeyboardInterrupt:
            break
        except Exception as e:
            log.error(f"Error: {e}")
            time.sleep(1)

if __name__ == "__main__":
    run_fft_reader()
    port.close()