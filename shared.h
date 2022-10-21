/* date = October 14th 2022 5:21 pm */

#ifndef SHARED_H
#define SHARED_H

#include <vector>
#include <algorithm>
#include <functional>

struct ChannelDSPState{
    double gain;
    double low_shelf_gain;
    double low_shelf_freq;
    double high_shelf_gain;
    double high_shelf_freq;
};

static ChannelDSPState ChannelDSP_on()
{
    return {
        .gain = 1.0,
        .low_shelf_gain = 1.0,
        .high_shelf_gain = 1.0,
    };
}

static ChannelDSPState ChannelDSP_off()
{
    return {
        .gain = 0.0,
    };
}

struct Settings{
    float difficulty;
};

struct Stats {
    int total_score;
};


#endif //SHARED_H
