#ifndef __SLAVE_HPP
#define __SLAVE_HPP

#include "Communication/Protocol.hpp"
#include <unordered_map>
#include <functional>
#include "Communication/MessageId.h"

namespace Xerxes
{


class Slave
{
private:
    Protocol *xp;
    std::unordered_map<msgid_t, std::function<void(const Message&)>> bindings;
    uint8_t address;

public:
    Slave();
    Slave(Protocol *protocol, const uint8_t address);
    ~Slave();
    void bind(const msgid_t msgId, std::function<void(const Message&)> _f);
    void call(const Message &msg);

    bool send(const uint8_t destinationAddress, const msgid_t msgId);
    bool send(const uint8_t destinationAddress, const msgid_t msgId, const std::vector<uint8_t> &payload);
    bool sync(const uint32_t timeoutUs);
};


} // namespace Xerxes

#endif // !__SLAVE_HPP