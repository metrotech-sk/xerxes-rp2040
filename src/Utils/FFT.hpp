#include <bits/stdc++.h>

const float PI = acos(-1);

namespace Xerxes
{
    using cf = std::complex<float>;

    void fft(std::vector<cf> *a);

    void phase_to_freq(std::vector<cf> *a, const uint16_t freq);

    void rectify_fft_output(std::vector<cf> *vec);

    void truncate_fft_output(std::vector<cf> *vec);
}