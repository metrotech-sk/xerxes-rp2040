#ifndef __SCL3300_HPP
#define __SCL3300_HPP

#include "SCL3X00.hpp"

namespace Xerxes
{


class SCL3300 : public SCL3X00
{
private:

    /**
     * @brief Get the Deg From Packet object
     * 
     * @param packet    - received data
     * @return double  - angle in degrees 
     */
    double getDegFromPacket(const std::unique_ptr<SclPacket_t>& packet);

public:
    using SCL3X00::SCL3X00;
    void init();
    void update();
};


} //namespace Xerxes

#endif // !__SCL3300_HPP