constexpr int max_f = 12800;
constexpr int min_f = 50;
double power = 2.0;

int totalSections = 9;

int ratioToHz(float pos) {
    return int(min_f * std::pow(2, totalSections * pos));
}
float hzToRatio(int hz) {
    return float(std::log((double)hz / (double)min_f) / (totalSections * std::log(2.0)));
}

float frequencyToX(int frequency, juce::Rectangle<int> bounds)
{
    //float ratio_x = std::powf((frequency - min_f) / (max_f - min_f), 1.0f / power);
    float ratio_x = hzToRatio(frequency);
    return ratio_x * bounds.getWidth() + bounds.getX();
}

float ratioInRect(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    return float(relative.x) / float(bounds.getWidth());
}

int positionToFrequency(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    auto ratio = ratioInRect(position, bounds);
    if(ratio == -1.0f)
        return -1.0f;
    return ratioToHz(ratio);
}

class FrequencyWidget : public juce::Component 
{
public:
    FrequencyWidget(std::function<void(float)> onClick, std::function<void(int)> scoreChanged) : 
        onClick(std::move(onClick)),
        scoreChanged(std::move(scoreChanged))
    {
        targetFrequency = ratioToHz(juce::Random::getSystemRandom().nextFloat());
        window = 0.1f;
        this->scoreChanged(score);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(4);
    }

    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().reduced(4);

        auto drawLineAt = [&] (float marginTop, float marginBottom, float x) {
            auto a = juce::Point<float>(x, (float)r.getY() + marginTop);
            auto b = juce::Point<float>(x, (float)r.getBottom() - marginBottom);
            g.drawLine( { a, b } );
        };

        //outline
        g.setColour(juce::Colour(85, 85, 85));
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        
        auto mousePosition = getMouseXYRelative();
        auto mouseRatio = ratioInRect(mousePosition, r);
        auto mouseFrequency = positionToFrequency(mousePosition, r);

        g.setColour(juce::Colour(170, 170, 170));
        for (float line_freq : { 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f })
        {
            auto x = frequencyToX(line_freq, r);
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float>{ 30.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        if(mouseFrequency != -1.0f)
        {
            float windowMax = std::min(1.0f, mouseRatio + window);
            float windowMin = std::max(0.0f, mouseRatio - window);
            auto windowMaxX = r.getX() + r.getWidth() * windowMax;
            auto windowMinX = r.getX() + r.getWidth() * windowMin;

            auto a = juce::Point<float>(windowMinX, (float)r.getY());
            auto b = juce::Point<float>(windowMaxX, (float)r.getBottom());
            
            g.setColour(juce::Colours::yellow);
            g.setOpacity(0.5f);
            g.fillRect({ a, b });
            
            g.setOpacity(1.0f);
            g.setColour(juce::Colours::black);
            auto x = frequencyToX(mouseFrequency, r);
            drawLineAt(0.0f, 0.0f, x);

            juce::Rectangle rect = { 200, 200 };
            rect.setCentre(mousePosition);
            g.setColour(juce::Colour(255, 240, 217));
            g.drawText(juce::String(mouseFrequency), rect, juce::Justification::centred, false);
        }
        
        //target
        {
            g.setColour(juce::Colours::black);
            auto x = frequencyToX(targetFrequency, r);
            drawLineAt(0.0f, 0.0f, x);
        }
        //hover
        if(mouseFrequency != -1.0f)
        {
            juce::Rectangle rect = { 200, 200 };
            rect.setCentre(mousePosition);
            g.setColour(juce::Colour(255, 240, 217));
            g.drawText(juce::String(mouseFrequency), rect, juce::Justification::centred, false);
        }

        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(r.toFloat(), 8.0f, 2.0f);
    }

private:

    
    void mouseMove (const juce::MouseEvent& event) override
    {
        repaint();
    }
    
    
    void mouseDrag (const juce::MouseEvent& event) override
    {
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        auto clickedFreq = positionToFrequency(getMouseXYRelative(), getLocalBounds());
        auto clicked_ratio = hzToRatio(clickedFreq);
        auto target_ratio = hzToRatio(targetFrequency);
        auto distance = std::abs(clicked_ratio - target_ratio);
        if (distance < window)
        {
            score++;
            window *= 0.95f;
            scoreChanged(score);
        }
       DBG("score : " << score);
        onClick(clickedFreq);

        targetFrequency = ratioToHz(juce::Random::getSystemRandom().nextFloat());
        repaint();
    }

    int targetFrequency;
    float window;
    int score = 0;
    std::function < void(float) > onClick;
    std::function < void(int) > scoreChanged;
};