#ifndef __VARIOUS_HPP
#define __VARIOUS_HPP

#include <bits/stdc++.h>
#include <malloc.h>

namespace std
{
    template <typename T>
    string to_string(const complex<T> &c);

    template <typename T>
    string to_string(const vector<T> &vec);

    template <typename T>
    string to_string(const vector<T> *vec);

    template <typename T>
    string to_string(const shared_future<vector<T>> &vec);

    template <typename T, size_t _l>
    string to_string(const array<T, _l> *arr);

    template <typename T, size_t _l>
    string to_string(const shared_ptr<array<T, _l>> &arr);

    template <typename T>
    void print_fft(const vector<complex<T>> *vec, const uint16_t freq);

    uint32_t getTotalHeap(void);

    uint32_t getFreeHeap(void);

    uint32_t getUsedHeap(void);
}

#include "Various.tpp"

#endif // __VARIOUS_HPP