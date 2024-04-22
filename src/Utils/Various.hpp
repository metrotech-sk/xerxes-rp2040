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
    void print_fft_output(const vector<complex<T>> *vec, const uint16_t freq, const size_t num_samples = 0);

    template <typename T>
    void sort_fft_output(vector<complex<T>> *vec);

    uint32_t getTotalHeap(void);

    uint32_t getFreeHeap(void);

    uint32_t getUsedHeap(void);

    template <typename T>
    T stddev_signal(const vector<complex<T>> *vec);
}

#include "Various.tpp"

#endif // __VARIOUS_HPP