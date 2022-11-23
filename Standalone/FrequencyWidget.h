constexpr float max_f = 12800.0f;
constexpr float min_f = 50.0f;
float power = 2.0f;

int totalSections = 9;

float positionToHz(float pos) {
    return min_f * pow(2, totalSections * pos);
}
float hzToPosition(float hz) {
    return std::log(hz / min_f) / (totalSections * std::log(2.0));
}

float frequencyToX(float frequency, juce::Rectangle<int> bounds)
{
    //float ratio_x = std::powf((frequency - min_f) / (max_f - min_f), 1.0f / power);
    float ratio_x = hzToPosition(frequency);
    return ratio_x * bounds.getWidth() + bounds.getX();
}

float positionToFrequency(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    auto ratio_x = float(relative.x) / float(bounds.getWidth());
    
    return positionToHz(ratio_x);
}

class FrequencyWidget : public juce::Component 
{
public:
    FrequencyWidget()
    {
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(4);
    }

    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().reduced(4);
        juce::Rectangle<int> upperBounds = r.withHeight(20);
        juce::Rectangle<int> lowerBounds = r.withTrimmedTop(20);

        //outline
        g.setColour(juce::Colour(85, 85, 85));
        g.fillRoundedRectangle(lowerBounds.toFloat(), 8.0f);
        
        
        auto mousePosition = getMouseXYRelative();
        auto freq = positionToFrequency(mousePosition, lowerBounds);

        
        g.setColour(juce::Colour(170, 170, 170));
        for (float line_freq : { 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f })
        {
            auto x = frequencyToX(line_freq, lowerBounds);
            auto a = juce::Point<float>(x, (float)lowerBounds.getY());
            auto b = juce::Point<float>(x, (float)lowerBounds.getBottom() - 45.0f);
            g.drawLine( { a, b } );
            auto textBounds = juce::Rectangle<float>{ 30.0f, 40.0f }.withCentre(b).withBottom((float)lowerBounds.getBottom());
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }

        if(freq != -1.0f)
        {
            g.setColour(juce::Colours::black);
            auto a = juce::Point<float>((float)mousePosition.x, (float)lowerBounds.getY());
            auto b = juce::Point<float>((float)mousePosition.x, (float)lowerBounds.getBottom());
            g.drawLine( { a, b } );

            juce::Rectangle rect = { 200, 200 };
            rect.setCentre(mousePosition);
            g.setColour(juce::Colour(255, 240, 217));
            g.drawText(juce::String(freq), rect, juce::Justification::centred, false);
        }

        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(lowerBounds.toFloat(), 8.0f, 2.0f);
    }

    void mouseMove (const juce::MouseEvent& event) override
    {
        repaint();
    }

private:
};