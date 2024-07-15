#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from read_pvs import *

l1, l2, l3, l4, l5 = [Leaf(i, XR) for i in range(1, 6)]
leaves = [l1, l2, l3, l4, l5]

s1_0, s2_0, s3_0, s4_0, s5_0 = [l.mean_pv0 for l in leaves]

with open("data.log", "w") as f:
    while True:
        try:
            # clear the screen
            s1, s2, s3, s4, s5 = [l.mean_pv0 for l in leaves]
            s1 = int(s1 - s1_0) // 100
            s2 = int(s2 - s2_0) // 100
            s3 = int(s3 - s3_0) // 100
            s4 = int(s4 - s4_0) // 100
            s5 = int(s5 - s5_0) // 100
            # os.system("clear")
            # read all the leaves
            string = f"S1: {s1:8d}, S2: {s2:8d}, S3: {s3:8d}, S4: {s4:8d}, S5: {s5:8d}"
            print(
                string,
                end="\r",
            )
            f.write(string + "\n")
            f.flush()
            time.sleep(1)
        except KeyboardInterrupt:
            print("Exiting...")
            break
