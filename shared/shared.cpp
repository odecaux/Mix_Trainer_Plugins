#include "pch.h"
#include "shared.h"

Channel_DSP_State ChannelDSP_on()
{
    Channel_DSP_State state = {
        .gain = 1.0,
    };
    assert(state.bands[0].type == Filter_None);
    return state;
}

Channel_DSP_State ChannelDSP_off()
{
    Channel_DSP_State state = {
        .gain = 0.0,
    };
    assert(state.bands[0].type == Filter_None);
    return state;
}


Channel_DSP_State ChannelDSP_gain(double gain)
{
    Channel_DSP_State state = {
        .gain = gain,
    };
    assert(state.bands[0].type == Filter_None);
    return state;
}

bool equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

size_t db_to_slider_pos(double db, const std::vector<double> &db_values)
{
    for(size_t i = 0; i < db_values.size(); i++)
    {
        if(equal_double(db, db_values[i], 0.001)) return i;
    }
    jassertfalse;
    return 0;
}

size_t gain_to_slider_pos(double gain, const std::vector<double> &db_values)
{ 
    return db_to_slider_pos(juce::Decibels::gainToDecibels(gain), db_values);
}

double slider_pos_to_gain(size_t pos, const std::vector<double> &db_values)
{
    return juce::Decibels::decibelsToGain(db_values[pos]);
}