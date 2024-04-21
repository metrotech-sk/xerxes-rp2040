#include "FFT.hpp"
#include "Utils/Log.h"

namespace Xerxes
{
    void fft(std::vector<cf> &a, bool invert)
    {
        int n = a.size();
        xlog_debug("fft n: " << n);
        if (n == 1)
            return;

        // allocate this in heap
        std::vector<cf> *a0 = new std::vector<cf>(n / 2);
        std::vector<cf> *a1 = new std::vector<cf>(n / 2);
        // std::vector<cf> a0(n / 2), a1(n / 2);
        for (int i = 0; 2 * i < n; i++)
        {
            a0->at(i) = a[2 * i];
            a1->at(i) = a[2 * i + 1];
        }
        fft(*a0, invert);
        fft(*a1, invert);

        double ang = 2 * PI / n * (invert ? -1 : 1);
        cf w(1), wn(cos(ang), sin(ang));
        for (int i = 0; 2 * i < n; i++)
        {
            a[i] = a0->at(i) + w * a1->at(i);
            a[i + n / 2] = a0->at(i) - w * a1->at(i);
            if (invert)
            {
                a[i] /= 2;
                a[i + n / 2] /= 2;
            }
            w *= wn;
        }
        delete a0;
        delete a1;
    }

    void fft(std::vector<cf> *a)
    {
        int n = a->size();
        if (n == 1)
            return;

        std::vector<cf> *a0 = new std::vector<cf>(n / 2);
        std::vector<cf> *a1 = new std::vector<cf>(n / 2);

        for (int i = 0; 2 * i < n; i++)
        {
            a0->at(i) = a->at(2 * i);
            a1->at(i) = a->at(2 * i + 1);
        }
        fft(a0);
        fft(a1);

        float ang = 2 * PI / n;
        cf w(1), wn(cos(ang), sin(ang));
        for (int i = 0; 2 * i < n; i++)
        {
            a->at(i) = a0->at(i) + w * a1->at(i);
            a->at(i + n / 2) = a0->at(i) - w * a1->at(i);
            w *= wn;
        }
        delete a0;
        delete a1;
    }
}