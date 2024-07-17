#include "FFT.hpp"

namespace Xerxes
{
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

    void phase_to_freq(std::vector<cf> *a, const uint16_t freq)
    {
        const size_t n = a->size();
        const float bin_size = (float)freq / n; // Hz

        for (size_t i = 0; i < n; i++)
        {
            // cout << to_string(i * bin_size) << " " << vec->at(i).real() << endl;
            a->at(i).imag(i * bin_size);
        }
    }

    void rectify_fft_output(std::vector<cf> *vec)
    {
        for (auto &el : *vec)
        {
            el.real(fabs(el.real()));
        }
    }

    void truncate_fft_output(std::vector<cf> *vec)
    {
        // remove the SECOND half of the vector
        vec->resize(vec->size() / 2);
    }

    float carrier_freq(std::vector<cf> *vec, uint16_t n_neighbours = 0)
    {
        float max = 0;
        size_t max_i = 0;
        for (size_t i = 1; i < vec->size(); i++) // skip 0 Hz
        {
            if (vec->at(i).real() > max)
            {
                max = vec->at(i).real();
                max_i = i;
            }
        }

        if (!n_neighbours)
        {
            return vec->at(max_i).imag();
        }
        else
        {
            float sum = 0;
            float sum_mag = 0;
            for (size_t i = max_i - n_neighbours; i <= max_i + n_neighbours; i++)
            {
                sum += vec->at(i).imag() * vec->at(i).real();
                sum_mag += vec->at(i).real();
            }
            return sum / sum_mag;
        }
    }

} // namespace Xerxes