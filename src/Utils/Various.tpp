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
    void print_fft_output(const vector<complex<T>> *vec, const uint16_t freq, const size_t num_samples)
    {
        const size_t n = num_samples <= 0 ? vec->size() : num_samples; // if num_samples is 0 or less, print all
        cout << "Frequency, Magnitude" << endl;

        for (size_t i = 0; i < n; i++)
        {
            auto magn = vec->at(i).real();
            auto freq = vec->at(i).imag();
            cout << freq << ", " << abs(magn) << endl;
        }
    }

    template <typename T>
    void sort_fft_output(vector<complex<T>> *vec)
    {
        // remove DC component - we dont care about it
        vec->at(0) = complex<T>(0, 0);

        // sort by magnitude which is stored in real part
        sort(vec->begin(), vec->end(), [](const complex<T> &a, const complex<T> &b)
             { return sqrt(a.real() * a.real()) > sqrt(b.real() * b.real()); });
    }

    template <typename T>
    T stddev_signal(const vector<complex<T>> *vec)
    {
        T mean = 0;
        for (auto &el : *vec)
        {
            mean += el.real();
        }
        mean /= vec->size();

        T variance = 0;
        for (auto &el : *vec)
        {
            variance += (el.real() - mean) * (el.real() - mean);
        }
        variance /= vec->size();

        return sqrt(variance);
    }
}

#endif // __VARIOUS_TPP