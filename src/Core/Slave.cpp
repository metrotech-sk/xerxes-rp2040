#include "Slave.hpp"
#include "Utils/Log.h"

namespace Xerxes
{

    Slave::Slave()
    {
    }

    Slave::Slave(Protocol *protocol, const uint8_t address) : xp(protocol), address(address)
    {
    }

    Slave::~Slave()
    {
    }

    void Slave::bind(const msgid_t msgId, std::function<void(const Message &)> _f)
    {
        bindings.emplace(msgId, _f);
    }

    void Slave::call(const Message &msg)
    {
        if (bindings.contains(msg.msgId))
        {
            // call a function bound to messageId
            bindings[msg.msgId](msg);
        }
    }

    bool Slave::send(const uint8_t destinationAddress, const msgid_t msgId)
    {
        Message message(address, destinationAddress, msgId);
        return xp->sendMessage(message);
    }

    bool Slave::send(const uint8_t destinationAddress, const msgid_t msgId, const std::vector<uint8_t> &payload)
    {
        Message message(address, destinationAddress, msgId, payload);
        return xp->sendMessage(message);
    }

    bool Slave::sync(uint32_t timeoutUs)
    {
        // check for incoming message

        Message incoming = Message();
        if (!xp->readMessage(incoming, timeoutUs))
        {
            // if no message is in buffer
            xlog_dbg("No message in buffer");
            return false;
        }

        // call appropriate function
        xlog_dbg("Calling function for message, msgid: " << std::hex << incoming.msgId << std::dec);
        call(incoming);
        return true;
    }

} // namespace Xerxes