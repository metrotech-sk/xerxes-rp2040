from xerxes_protocol import MemoryVolatile, Leaf, XerxesRoot, Addr, MsgId

import time
import struct
import pytest


__author__ = "theMladyPan"
__date__ = "2023-02-23"


def test_reset_soft(leaf: Leaf):
    leaf.reset_soft()
    with pytest.raises(TimeoutError):
        leaf.ping(1)

    # wait for the leaf to reboot
    time.sleep(0.5)

    # check if leaf is still connected to the network
    assert leaf.root.isPingLatest(leaf.ping())


def test_reset_hard(cleanLeaf: Leaf):
    # write gain to PV2
    cleanLeaf.gain_pv2 = 3.14

    # check that the gain is set correctly
    assert cleanLeaf.gain_pv2 == pytest.approx(3.14, abs=1e-6)

    cleanLeaf.reset_hard()
    # wait for the cleanLeaf to reboot
    time.sleep(0.5)

    # check if cleanLeaf is still connected to the network
    assert cleanLeaf.root.isPingLatest(cleanLeaf.ping())
    # check that the gain is reset to 1
    assert cleanLeaf.gain_pv2 == pytest.approx(1, abs=1e-6)


def test_hard_reset_fail(cleanLeaf: Leaf, XR: XerxesRoot):
    """Test if the hard reset fails when the magic bytes are wrong."""

    # change PV2 offset to pi=3.14
    cleanLeaf.offset_pv2 = 3.14

    # write the wrong magic bytes to the memory lock register
    mem_lock_offset = MemoryVolatile.memory_lock.elem_addr

    invalid_magic_bytes = struct.pack("I", 0xAA55AA55)
    cleanLeaf.write_reg(mem_lock_offset, invalid_magic_bytes)

    # confirm that the magic bytes are wrong
    assert cleanLeaf.read_reg_net(mem_lock_offset, 4) == invalid_magic_bytes

    # send a message to the leaf, which should still be alive
    reply = cleanLeaf.exchange(bytes(MsgId.RESET_HARD))
    assert reply is not None

    # reset should fail because of wrong magic bytes, so the reply should be NOK
    assert reply.message_id == MsgId.ACK_NOK

    # send a ping to the leaf, which should still be alive
    assert XR.isPingLatest(cleanLeaf.ping())

    # check that the PV2 offset is still approximately pi
    leaf = Leaf(Addr(0), XR)
    assert leaf.offset_pv2 == pytest.approx(3.14, abs=1e-6)
