#include "pch.h"
#include "shared.h"

Channel_DSP_State ChannelDSP_on()
{
    Channel_DSP_State state = {
        .gain_db = 0.0,
    };
    assert(state.eq_bands[0].type == Filter_None);
    return state;
}

Channel_DSP_State ChannelDSP_off()
{
    Channel_DSP_State state = {
        .gain_db = -100.0,
    };
    assert(state.eq_bands[0].type == Filter_None);
    return state;
}


Channel_DSP_State ChannelDSP_gain_db(double gain_db)
{
    Channel_DSP_State state = {
        .gain_db = gain_db,
    };
    assert(state.eq_bands[0].type == Filter_None);
    return state;
}

DSP_EQ_Band eq_band_peak(float frequency, float quality, float gain)
{
    return {
        .type = Filter_Peak,
        .frequency = frequency,
        .quality = quality,
        .gain = gain
    };
}

juce::dsp::IIR::Coefficients<float>::Ptr make_coefficients(DSP_EQ_Band band, double sample_rate)
{
    switch (band.type) {
        case Filter_None:
            return new juce::dsp::IIR::Coefficients<float> (1, 0, 1, 0);
        case Filter_Low_Pass:
            return juce::dsp::IIR::Coefficients<float>::makeLowPass (sample_rate, band.frequency, band.quality);
        case Filter_LowPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (sample_rate, band.frequency);
        case Filter_LowShelf:
            return juce::dsp::IIR::Coefficients<float>::makeLowShelf (sample_rate, band.frequency, band.quality, band.gain);
        case Filter_BandPass:
            return juce::dsp::IIR::Coefficients<float>::makeBandPass (sample_rate, band.frequency, band.quality);
        case Filter_AllPass:
            return juce::dsp::IIR::Coefficients<float>::makeAllPass (sample_rate, band.frequency, band.quality);
        case Filter_AllPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (sample_rate, band.frequency);
        case Filter_Notch:
            return juce::dsp::IIR::Coefficients<float>::makeNotch (sample_rate, band.frequency, band.quality);
        case Filter_Peak:
            return juce::dsp::IIR::Coefficients<float>::makePeakFilter (sample_rate, band.frequency, band.quality, band.gain);
        case Filter_HighShelf:
            return juce::dsp::IIR::Coefficients<float>::makeHighShelf (sample_rate, band.frequency, band.quality, band.gain);
        case Filter_HighPass1st:
            return juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sample_rate, band.frequency);
        case Filter_HighPass:
            return juce::dsp::IIR::Coefficients<float>::makeHighPass (sample_rate, band.frequency, band.quality);
        case Filter_LastID:
        default:
            return nullptr;
    }
}

void channel_dsp_update_chain(Channel_DSP_Chain *dsp_chain,
                              Channel_DSP_State state,
                              juce::CriticalSection *lock,
                              double sample_rate)
{
    for (auto i = 0; i < 1 /* une seule bande pour l'instant */; ++i) {
        auto new_coefficients = make_coefficients(state.eq_bands[i], sample_rate);
        assert(new_coefficients);
        {
            juce::ScopedLock processLock (*lock);
            if (i == 0)
                *dsp_chain->get<0>().state = *new_coefficients;
        }
    }
    //compressor
    dsp_chain->setBypassed<1>(!state.comp.is_on);
    dsp_chain->setBypassed<2>(!state.comp.is_on);
    if (state.comp.is_on)
    {
        dsp_chain->get<1>().setThreshold(state.comp.threshold_gain);
        dsp_chain->get<1>().setRatio(state.comp.ratio);
        dsp_chain->get<1>().setAttack(state.comp.attack);
        dsp_chain->get<1>().setRelease(state.comp.release);
        //makeup gain
        dsp_chain->get<2>().setGainLinear ((float)state.comp.makeup_gain);
    }
    //gain
    dsp_chain->get<3>().setGainDecibels ((float)state.gain_db);
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

template<>
float string_to(juce::String string)
{
    return string.getFloatValue();
}
template<>
int string_to(juce::String string)
{
    return string.getIntValue();
}

template<>
uint32_t string_to(juce::String string)
{
    return checked_cast<uint32_t>(string.getIntValue());
}

template<>
int64_t string_to(juce::String string)
{
    return string.getLargeIntValue();
}
