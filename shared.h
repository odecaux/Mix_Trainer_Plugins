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



#endif //SHARED_H
