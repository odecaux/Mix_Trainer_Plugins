#include "pch.h"
#include "shared.h"

Channel_DSP_State ChannelDSP_on()
{
    Channel_DSP_State state = {
        .gain = 1.0,
    };
    jassert(state.bands[0].type == Filter_None);
    state.bands[0].type = Filter_Low_Pass;
    state.bands[0].frequency = 500.0f;
    state.bands[0].quality = 0.7f;
    return state;
}

Channel_DSP_State ChannelDSP_off()
{
    Channel_DSP_State state = {
        .gain = 0.0,
    };
    jassert(state.bands[0].type == Filter_None);
    return state;
}


Channel_DSP_State ChannelDSP_gain(double gain)
{
    Channel_DSP_State state = {
        .gain = gain,
    };
    jassert(state.bands[0].type == Filter_None);
    return state;
}

double equal_double(double a, double b, double theta)
{
    return std::abs(a - b) < theta;
}

int db_to_slider_pos(double db, const std::vector<double> &db_values)
{
    for(size_t i = 0; i < db_values.size(); i++)
    {
        if(equal_double(db, db_values[i], 0.001)) return (int)i;
    }
    jassertfalse;
    return -1;
}

int gain_to_slider_pos(double gain, const std::vector<double> &db_values)
{ 
    return db_to_slider_pos(juce::Decibels::gainToDecibels(gain), db_values);
}

double slider_pos_to_gain(int pos, const std::vector<double> &db_values)
{
    return juce::Decibels::decibelsToGain(db_values[pos]);
}