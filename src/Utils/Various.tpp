#ifndef __VARIOUS_TPP
#define __VARIOUS_TPP

#include "hardware/watchdog.h"

namespace std
{
    template <typename T>
    string to_string(const complex<T> &c)
    {
        return "\n" + to_string(c.real()) + "+" + to_string(c.imag()) + "i";
    }

    template <typename T>
    string to_string(const vector<T> &vec)
    {
        stringstream ss;
        for (const auto &el : vec)
        {
            ss << to_string(el) << ", ";
        }
        return ss.str();
    }

    template <typename T>
    string to_string(const vector<T> *vec)
    {
        stringstream ss;
        for (const auto &el : *vec)
        {
            ss << to_string(el) << ", ";
        }
        return ss.str();
    }

    template <typename T>
    string to_string(const shared_ptr<vector<T>> &vec)
    {
        stringstream ss;
        for (const auto &el : *vec)
        {
            ss << to_string(el) << ", ";
        }
        return ss.str();
    }

    template <typename T, size_t _l>
    string to_string(const array<T, _l> *arr)
    {
        stringstream ss;
        for (const auto &el : *arr)
        {
            ss << to_string(el) << ", ";
        }
        return ss.str();
    }

    template <typename T, size_t _l>
    string to_string(const shared_ptr<array<T, _l>> &arr)
    {
        stringstream ss;
        for (const auto &el : *arr)
        {
            ss << to_string(el) << ", ";
        }
        return ss.str();
    }

    template <typename T>
    void print_fft(const vector<complex<T>> *vec, const uint16_t freq)
    {
        const size_t n = vec->size();
        const float bin_size = (float)freq / n; // Hz

        for (size_t i = 0; i < n / 2; i++)
        {
            auto ampl = vec->at(i).real();
            cout << to_string(i * bin_size) << " " << sqrt(ampl * ampl) << endl;
        }
    }
}

#endif // __VARIOUS_TPP