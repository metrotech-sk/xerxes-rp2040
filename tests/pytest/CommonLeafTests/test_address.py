#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pytest
from xerxes_protocol import Leaf
import time
import logging

_log = logging.getLogger(__name__)


__author__ = "theMladyPan"
__date__ = "2023-02-22"


def test_change_address(cleanLeaf: Leaf):

    start_t = time.perf_counter_ns()
    test_addr = 0x55

    try:
        # change address, effective immediately
        cleanLeaf.device_address = test_addr
    except TimeoutError:
        _log.info("Device went offline during address change, not a problem")

    # device should not be reachable at old address
    with pytest.raises(TimeoutError):
        cleanLeaf.ping(1)

    # create new leaf with new address
    leaf = Leaf(test_addr, cleanLeaf.root)

    # test if new address is correct, read address from the device
    new_addr = leaf.device_address

    assert new_addr == test_addr

    try:
        # change address back to default, should be effective immediately
        leaf.device_address = 0
    except TimeoutError:
        _log.info("Device went offline during address change, not a problem")
    time.sleep(1)

    # device should be reachable at new address because we used "address" property which
    # handles the address change internally

    assert cleanLeaf.root.isPingLatest(leaf.ping())
    _log.info(
        f"Address change test passed in: {(time.perf_counter_ns() - start_t) / 1e9:.2f}s"
    )
