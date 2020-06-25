

#ifndef HELPER_HPP_
#define HELPER_HPP_

#include <cmath>
#include <algorithm>


template <typename T>
inline T clamp(T input, T range_min, T range_max)
{
    return std::min(std::max(input, range_min), range_max);
}


// Modified from https://blog.csdn.net/fengbingchun/article/details/73323475
template <typename T>
void meanStdDev(T* mat, double mean, double variance, double stddev, int w, int h)
{
    double sum{ 0.0 }, sqsum{ 0.0 };

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            double v = static_cast<double>(mat[y * w + x]);
            sum += v;
            sqsum += v * v;
        }
    }

    double scale = 1.0 / (h * w);

    mean = sum * scale;
    variance = std::max(sqsum * scale - (mean) * (mean), 0.0);
    stddev = std::sqrt(variance);
}

#endif
