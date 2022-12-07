constexpr int max_f = 12800;
constexpr int min_f = 50;

static int totalSections = 9;

static int ratioToHz(float ratio) {
    double exponent = 0.5 + (double)ratio * double(totalSections - 1);
    return int(min_f * std::pow(2, exponent));
}
static float hzToRatio(int hz) {
    double a = std::log((double)hz / (double)min_f) / std::log(2.0);
    double ratio = (a - 0.5) / double(totalSections - 1);
    return (float)ratio;
}

static float frequencyToX(int frequency, juce::Rectangle<int> bounds)
{
    //float ratio_x = std::powf((frequency - min_f) / (max_f - min_f), 1.0f / power);
    float ratio_x = hzToRatio(frequency);
    return ratio_x * bounds.getWidth() + bounds.getX();
}

static float ratioInRect(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    return float(relative.x) / float(bounds.getWidth());
}

static int positionToFrequency(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    auto ratio = ratioInRect(position, bounds);
    if(ratio == -1.0f)
        return -1;
    return ratioToHz(ratio);
}

struct FrequencyWidget : public juce::Component 
{
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds();

        auto drawLineAt = [&] (float marginTop, float marginBottom, float x) {
            auto a = juce::Point<float>(x, (float)r.getY() + marginTop);
            auto b = juce::Point<float>(x, (float)r.getBottom() - marginBottom);
            g.drawLine( { a, b });
        };

        //outline
        g.setColour(juce::Colour(85, 85, 85));
        g.fillRoundedRectangle(r.toFloat(), 8.0f);
        
        //frequency guide lines
        g.setColour(juce::Colour(170, 170, 170));
        for (int line_freq : { 100, 200, 400, 800, 1600, 3200, 6400, 12800 })
        {
            auto x = frequencyToX(line_freq, r);
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float> { 40.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        
        {
            auto mouse_position = getMouseXYRelative();
            auto mouse_ratio = ratioInRect(mouse_position, r);
            float cursor_ratio = is_cursor_locked ?
                hzToRatio(locked_cursor_frequency) :
                mouse_ratio;
            int cursor_frequency = is_cursor_locked ?
                locked_cursor_frequency :
                positionToFrequency(mouse_position, r);
            juce::Line<int> cursor_line;
    
            //cursor
            if (cursor_frequency != -1)
            {
                float window_right = std::min(1.0f, cursor_ratio + correct_answer_window);
                float window_left = std::max(0.0f, cursor_ratio - correct_answer_window);
                auto window_right_x = r.getX() + r.getWidth() * window_right;
                auto window_left_x = r.getX() + r.getWidth() * window_left;
    
                auto a = juce::Point<float>(window_left_x, (float)r.getY());
                auto b = juce::Point<float>(window_right_x, (float)r.getBottom());
                auto window_rect = juce::Rectangle<float> { a, b };
                if (display_window)
                {
                    g.setColour(juce::Colours::yellow);
                    g.setOpacity(0.5f);
                    g.fillRect(window_rect);
                }
                g.setOpacity(1.0f);
                g.setColour(juce::Colours::black);
                auto cursor_x = frequencyToX(cursor_frequency, r);
                drawLineAt(0.0f, 0.0f, cursor_x);
    
                juce::Rectangle text_bounds = { 200, 200 };
                text_bounds.setCentre(window_rect.getCentre().toInt());
                g.setColour(juce::Colour(255, 240, 217));
                g.drawText(juce::String(cursor_frequency), text_bounds, juce::Justification::centred, false);
            }
            
            //target
            if (display_target)
            {
                g.setColour(juce::Colours::black);
                auto x = frequencyToX(target_frequency, r);
                drawLineAt(0.0f, 0.0f, x);
            }
    #if 0
            //cursor text
            if (cursor_frequency != -1.0f)
            {
                juce::Rectangle rect = { 200, 200 };
                rect.setCentre(mouse_position);
                g.setColour(juce::Colour(255, 240, 217));
                g.drawText(juce::String(cursor_frequency), rect, juce::Justification::centred, false);
            }
    #endif
    
        }
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(r.toFloat(), 8.0f, 2.0f);
    }

    
private:
    void mouseMove (const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        //static_assert(false); //mouse_move
        repaint();
    }
    
    
    void mouseDrag (const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        //static_assert(false); //mouse_move ?
        repaint();
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        if(is_cursor_locked)
            return;
        auto clicked_freq = positionToFrequency(getMouseXYRelative(), getLocalBounds());
        onClick(clicked_freq);
    }

public:
    bool display_target;
    int target_frequency;
    bool is_cursor_locked;
    int locked_cursor_frequency;
    bool display_window;
    float correct_answer_window;
    std::function < void(int) > onClick;
};
