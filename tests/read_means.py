from xerxes_protocol import (
    XerxesNetwork,
    XerxesRoot,
    Leaf
)

from serial import Serial
import logging
import time

log = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO)

XN = XerxesNetwork(Serial(port="/dev/ttyUSB0", baudrate=115200, timeout=0.02))
XN.init()
XR = XerxesRoot(0xFE, XN)

for i in range(32):
    try:
        l = Leaf(i, XR)
        l.ping()
        break
    except TimeoutError:
        continue
    
log.info(f"Found leaf at address {l.address}")
log.info(f"UUID: {l.device_uid}")
    
mean_vals = {}
labels = ["pv0", "pv1", "pv2", "pv3"]
while True:
    try:
        for label in labels:
            mean_vals[label] = getattr(l, f"mean_{label}")
            print(f"{label}:\t{mean_vals[label]:.4f}", end="\t")
        
        print(
            end="\r"
        )
        time.sleep(.04)
    except KeyboardInterrupt:
        del XN
        break
    except TimeoutError:
        log.warning(f"TimeoutError reading leaf {l.address}")
    