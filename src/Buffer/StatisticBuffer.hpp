#ifndef STATISTIC_BUFFER_HPP
#define STATISTIC_BUFFER_HPP

#include "RingBuffer.hpp"
#include "Utils/Log.h"
#include <algorithm>

namespace Xerxes
{

    template <class T>
    class StatisticBuffer : public RingBuffer<T>
    {
    protected:
        float min;
        float max;
        float mean;
        float stdDev;
        float median;

    public:
        using RingBuffer<T>::RingBuffer; // inherit constructors
        void updateStatistics();

        const float &getMean();
        const float &getStdDev();
        const float &getMax();
        const float &getMin();
        const float &getMedian();
        void getStatistics(T *min,
                           T *max,
                           T *mean,
                           T *stdDev,
                           T *median = nullptr);
    };

    template <class T>
    void StatisticBuffer<T>::getStatistics(T *min,
                                           T *max,
                                           T *mean,
                                           T *stdDev,
                                           T *median)
    {
        if (min != nullptr)
        {
            *min = this->min;
        }

        if (max != nullptr)
        {
            *max = this->max;
        }

        if (mean != nullptr)
        {
            *mean = this->mean;
        }

        if (stdDev != nullptr)
        {
            *stdDev = this->stdDev;
        }

        if (median != nullptr)
        {
            *median = this->median;
        }
    }

    template <class T>
    void StatisticBuffer<T>::updateStatistics()
    {
        mean = 0;
        min = INFINITY;
        max = -INFINITY;

        double sumOfElements{0};
        double sumOfSquaredErrors{0};
        std::vector<T> sortedBuffer(this->maxCursor);

        for (int i = 0; i < this->maxCursor; i++)
        {
            T el = this->buffer[i];
            sumOfElements += el;

            if (el < min)
            {
                min = el;
            }

            if (el > max)
            {
                max = el;
            }

            sortedBuffer[i] = el;
        }

        mean = sumOfElements / this->maxCursor;

        for (int i = 0; i < this->maxCursor; i++)
        {
            sumOfSquaredErrors += powf(this->buffer[i] - mean, 2);
        }

        stdDev = sqrtf(sumOfSquaredErrors / this->maxCursor);

        // Calculate the median
        std::sort(sortedBuffer.begin(), sortedBuffer.end());
        if (this->maxCursor % 2 == 0)
        {
            // If the number of elements is even, take the average of the middle two elements
            median = (sortedBuffer[this->maxCursor / 2 - 1] + sortedBuffer[this->maxCursor / 2]) / 2.0;
        }
        else
        {
            // If the number of elements is odd, take the middle element
            median = sortedBuffer[this->maxCursor / 2];
        }

        // print content of the sorted buffer
        for (int i = 0; i < this->maxCursor; i++)
        {
            xlog_trace("Sorted buffer[" << i << "]: " << sortedBuffer[i]);
        }
        xlog_debug("Median: " << median);
    }

    template <class T>
    const float &StatisticBuffer<T>::getStdDev()
    {
        return stdDev;
    }

    template <class T>
    const float &StatisticBuffer<T>::getMean()
    {
        return mean;
    }

    template <class T>
    const float &StatisticBuffer<T>::getMin()
    {
        return this->min;
    }

    template <class T>
    const float &StatisticBuffer<T>::getMax()
    {
        return max;
    }

    template <class T>
    const float &StatisticBuffer<T>::getMedian()
    {
        return median;
    }

} // namespace Xerxes

#endif // !STATISTIC_BUFFER_HPP