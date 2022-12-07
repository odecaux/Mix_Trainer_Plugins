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

juce::dsp::IIR::Coefficients<float>::Ptr make_coefficients(DSP_Filter_Type type, double sample_rate, float frequency, float quality, float gain)
{
    switch (type) {
        case Filter_None:
            return new juce::dsp::IIR::Coefficients<float> (1, 0, 1, 0);
        case Filter_Low_Pass:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass (sample_rate, frequency, quality);
        case Filter_LowPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sample_rate, frequency);
        case Filter_LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf (sample_rate, frequency, quality, gain);
        case Filter_BandPass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass (sample_rate, frequency, quality);
        case Filter_AllPass:
            return juce::dsp::IIR::Coefficients<float>::makeAllPass (sample_rate, frequency, quality);
        case Filter_AllPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (sample_rate, frequency);
        case Filter_Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch (sample_rate, frequency, quality);
        case Filter_Peak:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sample_rate, frequency, quality, gain);
        case Filter_HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf (sample_rate, frequency, quality, gain);
        case Filter_HighPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sample_rate, frequency);
        case Filter_HighPass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass (sample_rate, frequency, quality);
        case Filter_LastID:
        default:
            return nullptr;
    }
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