#ifndef __BIND_WRAPPER_HPP


#include "Definitions.h"
#include "Core/Register.hpp"
#include <functional>
#include <Message.hpp>

extern Xerxes::Register _reg;


namespace Xerxes
{


/**
 * @brief Decorate a function to be unicast targeted
 * 
 * a function is called only on targeted unicast packet
 * 
 * @tparam Func 
 * @param f function to call for unicast packets
 * @return std::function<void(const Xerxes::Message &)> decorated lambda function
 */
template <typename Func>
std::function<void(const Xerxes::Message &)> unicast(Func f) 
{
  // The returned function is a lambda function that takes the same arguments as the original
  // function and calls it with the given arguments.
    return [f](const Xerxes::Message &msg) {
        if(msg.dstAddr!= 0xff && *_reg.devAddress == msg.dstAddr)
        {
        // Call the original function with the given arguments.
            f(msg);
        }
        else
        {       
            // do nothing
        }
    };
}


/**
 * @brief Decorate a function to be broadcast targeted
 * 
 * a function is called on targeted unicast packet or broadcast packet
 * 
 * @tparam Func 
 * @param f function to call for unicast packets
 * @return std::function<void(const Xerxes::Message &)> decorated lambda function
 */
template <typename Func>
std::function<void(const Xerxes::Message &)> broadcast(Func f) 
{
    return [f](const Xerxes::Message &msg) {
        if(msg.dstAddr == 0xff || *_reg.devAddress == msg.dstAddr)
        {
        // Call the original function with the given arguments.
            f(msg);
        }
        else
        {       
            // do nothing
        }
    };
}


} // namespace Xerxes

#endif // !__BIND_WRAPPER_HPP   