static uint32_t denormalize_frequency(float ratio, uint32_t min_f, uint32_t num_octaves) 
{
    float exponent = ratio * static_cast<float>(num_octaves);
    return static_cast<uint32_t>(static_cast<float>(min_f) * powf(2, exponent));
}

static float normalize_frequency(uint32_t hz, uint32_t min_f, uint32_t num_octaves) 
{
    float a = logf(static_cast<float>(hz) / min_f) / logf(2.0f);
    float ratio = (a) / static_cast<float>(num_octaves);
    return ratio;
}

static float frequency_to_x(uint32_t frequency, juce::Rectangle<int> bounds, uint32_t min_f, uint32_t num_octaves)
{
    float ratio_x = normalize_frequency(frequency, min_f, num_octaves);
    return ratio_x * static_cast<float>(bounds.getWidth()) + static_cast<float>(bounds.getX());
}

static float normalize_position(juce::Point<int> position, juce::Rectangle<int> bounds)
{
    if(!bounds.contains(position))
        return -1.0f;
    auto relative = position - bounds.getTopLeft();
    return float(relative.x) / float(bounds.getWidth());
}

static uint32_t x_to_frequency(juce::Point<int> position, juce::Rectangle<int> bounds, uint32_t  min_f, uint32_t num_octaves)
{
    auto ratio = normalize_position(position, bounds);
    if(ratio == -1.0f)
        return 0;
    return denormalize_frequency(ratio, min_f, num_octaves);
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
        for (int line_freq : { 50, 100, 200, 400, 800, 1600, 3200, 6400, 12800 })
        {
            auto x = frequency_to_x(line_freq, r, min_frequency, num_octaves);
            if (x <= 0)
                continue;
            drawLineAt(10.0f, 45.0f, x);
            juce::Point<float> textCentre = { x, 0.0f }; //TODO yolo
            auto textBounds = juce::Rectangle<float> { 40.0f, 40.0f }.withCentre(textCentre).withY((float)r.getBottom() - 45.0f);
            g.drawText(juce::String(line_freq), textBounds, juce::Justification::centred, false);
        }
        
        {
            auto mouse_position = getMouseXYRelative();
            auto mouse_ratio = normalize_position(mouse_position, r);
            float cursor_ratio = is_cursor_locked ?
                normalize_frequency(locked_cursor_frequency, min_frequency, num_octaves) :
                mouse_ratio;
            uint32_t cursor_frequency = is_cursor_locked ?
                locked_cursor_frequency :
                x_to_frequency(mouse_position, r, min_frequency, num_octaves);
            
            //juce::Line<int> cursor_line;
    
            //cursor
            if (cursor_frequency != 0)
            {
                float window_left = std::max(0.0f, cursor_ratio - correct_answer_window);
                float window_right = std::min(1.0f, cursor_ratio + correct_answer_window);
                float window_left_x = static_cast<float>(r.getX()) + static_cast<float>(r.getWidth()) * window_left;
                float window_right_x = static_cast<float>(r.getX()) + static_cast<float>(r.getWidth()) * window_right;
    
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
                auto cursor_x = frequency_to_x(cursor_frequency, r, min_frequency, num_octaves);
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
                auto x = frequency_to_x(target_frequency, r, min_frequency, num_octaves);
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
        auto clicked_freq = x_to_frequency(getMouseXYRelative(), getLocalBounds(), min_frequency, num_octaves);
        onClick(clicked_freq);
    }

public:
    bool display_target;
    uint32_t target_frequency;
    bool is_cursor_locked;
    uint32_t locked_cursor_frequency;
    bool display_window;
    float correct_answer_window;
    uint32_t min_frequency;
    uint32_t num_octaves;
    std::function < void(uint32_t) > onClick;
};
