#include "RS485.hpp"
#include "Utils/Log.h"
#include "Utils/Gpio.hpp"

namespace Xerxes
{

    RS485::RS485(queue_t *queueTx, queue_t *queueRx) : qtx(queueTx), qrx(queueRx)
    {
    }

    RS485::~RS485()
    {
    }

    bool RS485::sendData(const Packet &toSend) const
    {
        // FIXME: this can check if queue has enough space before trying to send data
        uint16_t sent{0};
        for (const auto &el : toSend.getData())
        {
            // try to add byte to the queue, if it fails, break the loop
            if (!queue_try_add(qtx, &el))
                break;
            sent++; // byte was sent successfully
        }

        // return true if whole packet was sent
        return sent == toSend.size();
    }

    bool RS485::readData(const uint64_t timeoutUs, Packet &packet)
    {
        // start timer
        uint64_t start = time_us_64();

        // if RX queue is empty, immediately return
        if (queue_is_empty(qrx))
        {
            return false;
        }

        if (receivePacket(timeoutUs))
        {
            packet = Packet(incomingMessage);

            return true;
        }

        return false;
    }

    bool RS485::receivePacket(const uint64_t timeoutUs)
    {
        // check if the packet is in fifo buffer
        uint8_t nextVal = 0;

        uint64_t tout = time_us_64() + timeoutUs;

        uint8_t chks = Xerxes::SOH;

        uint8_t msgLen = 0;
        volatile bool waitForSoh = true;

        // clear buffer
        incomingMessage.clear();

        while (!time_reached(tout))
        {
            if (waitForSoh)
            {
                if (queue_try_remove(qrx, &nextVal))
                {
                    if (nextVal == Xerxes::SOH)
                    {
                        waitForSoh = false;
                    }
                }
            }
            else
            {
                // SOH was found, get msgLen
                if (queue_try_remove(qrx, &msgLen))
                {
                    // add msglen to checksum
                    chks += msgLen;
                    break; // break from this loop, continue to receive all data
                }
            }
        }

        msgLen -= 3; // SOH and len was received, checksum is received at the end hence -3
        while (!time_reached(tout) && msgLen)
        {
            if (queue_try_remove(qrx, &nextVal))
            {
                // something was received, slap it to the incoming vector
                incomingMessage.emplace_back(nextVal);
                chks += nextVal;
                msgLen--;
            }
        }

        while (!time_reached(tout))
        {
            // wait for checksum byte
            if (queue_try_remove(qrx, &nextVal))
            {
                chks += nextVal;
                if (chks == 0)
                {
                    // successfully received whole message
                    xlog_dbg("Successfully received packet: " << incomingMessage.size() << " bytes");
                    return true;
                }
                else
                {
                    xlog_warn("Checksum error, expected 0, got " << std::hex << chks << std::dec);
                    return false;
                }
            }
        }

        xlog_warn("Timeout while receiving packet");
        return false;
    }

    uint32_t remainingTime(const uint64_t &start, const uint64_t &timeout)
    {
        auto current_time = time_us_64();
        return timeout - (current_time - start);
    }

} // namespace Xerxes