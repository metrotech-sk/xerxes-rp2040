#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from xerxes_protocol import Leaf, LeafConfig
import time
import logging

_log = logging.getLogger(__name__)


__author__ = "theMladyPan"
__date__ = "2023-02-22"


def test_read_mean_val(cleanLeaf: Leaf):
    """Reads 4 mean values and checks if any of them are not 0"""

    # cleanLeaf.device_config = LeafConfig.freeRun | LeafConfig.calcStat

    # wait for the leaf to collect some data
    time.sleep(0.2)

    # assert that at least one value is not 0
    assert any(
        [
            cleanLeaf.mean_pv0,
            cleanLeaf.mean_pv1,
            cleanLeaf.mean_pv2,
            cleanLeaf.mean_pv3,
        ]
    )


def test_read_last_val(cleanLeaf: Leaf):
    """Reads 4 last values and checks if any of them are not 0"""

    # put device into free running mode
    # cleanLeaf.device_config = LeafConfig.freeRun | LeafConfig.calcStat

    # wait for the leaf to collect some data
    time.sleep(0.2)

    # assert that at least one value is not 0
    assert any([cleanLeaf.pv0, cleanLeaf.pv1, cleanLeaf.pv2, cleanLeaf.pv3])


def test_read_stddev(cleanLeaf: Leaf):
    """Reads 4 standard deviation values and checks if any of them are not 0"""

    # put device into free running mode
    # cleanLeaf.device_config = LeafConfig.freeRun | LeafConfig.calcStat

    # wait for the leaf to collect some data
    time.sleep(0.2)

    # assert that at least one value is not 0
    assert any(
        [
            cleanLeaf.std_dev_pv0,
            cleanLeaf.std_dev_pv1,
            cleanLeaf.std_dev_pv2,
            cleanLeaf.std_dev_pv3,
        ]
    )


def test_read_min(cleanLeaf: Leaf):
    """Reads 4 minimum values and checks if any of them are not 0"""

    # put device into free running mode
    # cleanLeaf.device_config = LeafConfig.freeRun | LeafConfig.calcStat

    # wait for the leaf to collect some data
    time.sleep(0.2)

    # assert that at least one value is not 0
    assert any(
        [
            cleanLeaf.min_pv0,
            cleanLeaf.min_pv1,
            cleanLeaf.min_pv2,
            cleanLeaf.min_pv3,
        ]
    )


def test_read_max(cleanLeaf: Leaf):
    """Reads 4 maximum values and checks if any of them are not 0"""

    # put device into free running mode
    # cleanLeaf.device_config = LeafConfig.freeRun | LeafConfig.calcStat

    # wait for the leaf to collect some data
    time.sleep(0.2)

    # assert that at least one value is not 0
    assert any(
        [
            cleanLeaf.max_pv0,
            cleanLeaf.max_pv1,
            cleanLeaf.max_pv2,
            cleanLeaf.max_pv3,
        ]
    )


def test_read_msg_buf(cleanLeaf: Leaf):
    """Reads most bytes from register 512+256 and prints them as chars

    Args:
        cleanLeaf (Leaf): Leaf to test with.
    """

    # amount of bytes to read, max is 248 (255 - 7 (header + footer))
    NUM_BYTES = 248  # 255 - 7 (header + footer)

    cleanLeaf.network.timeout = 0.2

    xm = cleanLeaf.read_reg(512 + 256, NUM_BYTES)
    # print payload as chars
    _log.info(xm.payload.decode("utf-8"))

    assert len(xm.payload) == NUM_BYTES
